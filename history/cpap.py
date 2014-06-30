#!/usr/bin/env python
'''
     This python script is free software; you can redistribute it and/or
     modify it under the terms of the GNU Lesser General Public
     License as published by the Free Software Foundation; either
     version 2.1 of the License, or (at your option) any later version.

     This python script is distributed in the hope that it will be useful,
     but WITHOUT ANY WARRANTY; without even the implied warranty of
     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
     Lesser General Public License for more details.

     You should have received a copy of the GNU Lesser General Public
     License along with this script; if not, write to the Free Software
     Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
'''

#  Author: Mark Watkins <jedimark64[at]users.sourceforge.net>
#    Date: 09/03/2011
# Purpose: CPAP Support
# License: GPL

#Attempt at faster CPAP Loader

import sys
import os
from struct import *
from datetime import datetime as DT
from datetime import timedelta,date,time  #,datetime,date,time
import time
from matplotlib.dates import drange
from matplotlib.figure import Figure
from matplotlib.backends.backend_gtkagg import FigureCanvasGTKAgg as FigureCanvas
from pylab import *

#from pytz import timezone
import pytz
import gobject
import gtk

MYTIMEZONE="Australia/Queensland";


localtz=pytz.timezone(MYTIMEZONE)
utc = pytz.utc
utcoff=time.timezone / -(60*60)

Device_Types={
    "Unknown":0,
    "PAP":1,
    "Oximeter":2,
    "ZEO":3
    }

def LookupDeviceType(type):
    for k,v in Device_Types.iteritems():
        if type.lower()==k.lower():
            return v
    return 0


class Event:
    code=0
    time=None
    data=[]
    def __init__(self,time,code,data):
        self.time=time
        self.code=code
        self.data=data

class Waveform:
    time=None
    def __init__(self,time,waveform,size,duration,format,rate):
        self.time=time
        self.waveform=waveform
        self.size=size
        self.duration=duration
        self.format=format
        self.rate=rate

       
class Machine:
    def __init__(self,brand,model,type):
        self.brand=brand
        self.model=model
        self.type=LookupDeviceType(type)

    def Open(self):
        print "in Machine.Open()";

class OxiMeter(Machine):
    CodeTypes=['Error','Pulse','SpO2']

    def __init__(self,brand,model):
        Machine.__init__(self,brand,model,"Oximeter")

import serial
class CMS50X(OxiMeter):
    Baudrate=19200
    Timeout=5
    Home=os.path.expanduser('~')
    
    #LogDirectory=Home+os.sep+"CMS50"
    #os.system("mkdir "+LogDirectory)

    def __init__(self):
        OxiMeter.__init__(self,"Contec","CMS50X")
        self.Device=None
        self.devopen=False
        # Borrowed from PySerial
        if (os.name=="nt") or (sys.platform=="win32"):
            self.ports = []
            for i in range(256):
                try:
                    s = serial.Serial(i)
                    self.ports.append( (i, s.portstr))
                    s.close()   # explicit close 'cause of delayed GC in java
                except serial.SerialException:
                    pass
            self.Device=self.ports[5]
        elif os.name=='posix':
            import glob

            self.ports=glob.glob('/dev/ttyUSB*')
            if (len(self.ports)>0):
                self.Device=self.ports[0]

    def Open(self):
        if not self.Device:
            print "No serial device detected"
            return False

        if self.devopen: 
            print "Device is already open"
            return True

        try:
            self.ser=serial.Serial(self.Device,self.Baudrate,timeout=self.Timeout)
        except:
            print "Couldn't open",self.Device
            return False

        self.ser.flushInput()

        self.lastpulse=0
        self.lastspo2=0
        self.devopen=True
        return True

    def Close(self):
        if (self.devopen):
            ser.close()
            self.devopen=False

    def Read(self):
        while self.devopen:
            while self.devopen:
                h=self.ser.read(1)
                if len(h)>0:
                    if (ord(h[0]) & 0x80): # Sync Bit
                        break
                else:
                    #print "Timeout!";
                    self.devopen=False
                    return None

            c=self.ser.read(4)
            if len(c)==4:
                break
            else:
                #print "Sync error";
                self.devopen=False
                return None
        

        hdr=ord(h)
        if (hdr & 0x10):
            alarm=True
        else: alarm=False

        if (hdr & 0x8):# or (hdr & 0x20): # (hdr & 0x10)==alarm
            signal=True
        else: signal=False

        wave1=ord(c[0]) 
        wave2=ord(c[1])
        pulse=ord(c[2])
        spo2=ord(c[3])

        return [signal,alarm,wave1,wave2,pulse,spo2]

    def Save(self,time,code,value):
        if (not self.start): return

        delta=time-self.lasttime
        s=int(((delta.seconds*1000)+delta.microseconds/1000))

        self.events[self.start].append(s>>8)
        self.events[self.start].append(s&255)
        self.events[self.start].append(code)
        self.events[self.start].append(value)
        self.evcnt+=1
 
        self.lasttime=time

    def Record(self,path):
        if self.devopen: return (None,None)

        self.starttime=DT.utcnow()
        self.lasttime=self.starttime
        lt=self.lasttime

        self.Open()

        lastpulse=0
        lastspo2=0
        lastalarm=True
        lastsignal=True
        self.evcnt=0

        wave=[dict(),dict()]
        wavestart=None
        self.start=None
        self.events=dict()
        
        while self.devopen:
            D=self.Read()
            if not D: continue

            t=DT.utcnow();
            d=t-lt

            if D[0]!=lastsignal or ((t-self.lasttime)>timedelta(seconds=64)):
                self.Save(t,0,D[0])
                lastsignal=D[0]

            if D[1]!=lastalarm or ((t-self.lasttime)>timedelta(seconds=64)):
                self.Save(t,1,D[1])
                lastalarm=D[1]

            if (not self.start or (d>timedelta(microseconds=30000))): #Lost serial sync for wavefom
                self.start=t
                print "Starting new event chunk",self.start
                self.events[self.start]=bytearray()

            lt=t

            if D[1]: continue

            if (not wavestart or (d>timedelta(microseconds=30000))): #Lost serial sync for wavefom
                wavestart=t
                wave[0][wavestart]=bytearray()
                wave[1][wavestart]=bytearray()
                print "Starting new wave chunk",wavestart

            wave[0][wavestart].append(D[2])
            wave[1][wavestart].append(D[3])

            if (D[4]!=lastpulse) or ((t-self.lasttime)>timedelta(seconds=64)):
                self.Save(t,2,D[4])
                lastpulse=D[4]

            if (D[5]!=lastspo2) or ((t-self.lasttime)>timedelta(seconds=64)):
                self.Save(t,3,D[5])
                lastspo2=D[5]


        self.Close()

        if (path[-1]!=os.sep): path+=os.sep

        ed=sorted(self.events.keys())
        basename=path+ed[0].strftime("CMS50-%Y%m%d-%H%M%S")
        efname=basename+".001"
        
        magic=0x35534d43 #CMS5
        f=open(efname,"wb");
        j=0
        for k,v in self.events.iteritems():
            header=bytearray(16)
            timestamp=time.mktime(k.timetupple())
            l=len(v)
            struct.pack_into('<IHIIH',header,4,magic,1,timestamp,l,j)
            f.write(header)
            f.write(v)
            j+=1

        fclose(f)

        wfname=basename+".002"
        f=open(wfname,"wb");
        j=0
        for k,v in wave[0].iteritems():
            header=bytearray(16)
            timestamp=time.mktime(k.timetupple())
            l=len(v)
            struct.pack_into('<IHIIH',header,4,magic,2,timestamp,l,j)
            f.write(header)
            f.write(v)
            j+=1
            
        fclose(f)

        wfname=basename+".003"
        f=open(wffname,"wb");
        j=0
        for k,v in wave[0].iteritems():
            header=bytearray(16)
            timestamp=time.mktime(k.timetupple())
            l=len(v)
            struct.pack_into('<IHIIH',header,4,magic,3,timestamp,l,j)
            f.write(header)
            f.write(v)
            j+=1

        fclose(f)


        return (self.events,wave)
        
            
class CPAP(Machine):
    CodeTypes=['UN','PR','BP','PP','RE','OA','CA','H','FL','CSR','VS','LR']

    def __init__(self,brand,model):
        Machine.__init__(self,brand,model,"PAP")
        self.session=dict()
        self.sessiontimes=dict()
        self.flowrate=dict()
        self.flowtimes=dict();
        self.machine=dict()

    def GetBedtime(self,dt,hr=13):
        if (type(dt).__name__=='date'):
            
            start=DT(dt.year,dt.month,dt.day,hr,0,0)
            
        elif (type(dt).__name__=='datetime'):
            start=dt.replace(hour=hr,minute=0,second=0,microsecond=0)
        else:
            return (None,None)
        
        end=localtz.localize(start,is_dst=False)
        #start=utc.localize(start)
        start=end-timedelta(hours=24)

        sess=self.GetEvSessions(start,end)


        if len(sess):
            start=self.sessiontimes[sess[0]][0];
            end=self.sessiontimes[sess[-1]][1];
            return (start,end)
        else:
            return (None,None)

    def GetEvSessions(self,start=None,end=None):
        A=[]
        for sess,time in self.sessiontimes.iteritems():
            if not start or not end:
                A.append(sess)
            else:
                if (start>time[1]): continue
                if (end<time[0]): continue
                A.append(sess)

        return sorted(A)

    def GetFlowSessions(self,start=None,end=None):
        A=[]
        for sess,time in self.flowtimes.iteritems():
            if not start or not end:
                A.append(sess)
            else:
                if (start>time[1]): continue
                if (end<time[0]): continue
                A.append(sess)

        return sorted(A)

    def CountEvents(self,type,start,end):
        if type not in self.CodeTypes:
            print "Unrecognized cpap code",field
            return None

        #print "Searching",type,"between",start.astimezone(localtz),"and",end.astimezone(localtz)
        sesslist=self.GetEvSessions(start,end)
        val=0
        #print sesslist
        for s in sesslist:
            if type not in self.session[s].keys():
                break;
            if (start<self.sessiontimes[s][0]) and (end>self.sessiontimes[s][1]):
                val+=len(self.session[s][type]);

            else:
                for e in self.session[s][type]:
                    if (e.time>=start) and (e.time<=end):
                        val+=1
        return val

    def FirstLastEventTime(self,field,start,end):
        sess=self.GetEvSessions(start,end)

        a1=self.sessiontimes[sess[0]][0]
        a2=self.sessiontimes[sess[-1]][1]
        #if (a1>=start):
        #    st=a1
        #else:
        st=a1
        for e in self.session[sess[0]][field]:
            if (e.time>=start):
                st=e.time
                break;

        #if (a2<=end):
            #et=a2
        #else:
        et=a2
        for e in self.session[sess[-1]][field]:
            if (e.time>=end):
                et=e.time
                break;

        return (st,et)

                

    def GetTotalTime(self,start,end):
        t=timedelta(seconds=0)
        sess=self.GetEvSessions(start,end)
        for s in sess:
            #print self.sessiontimes[s]
            a1=self.sessiontimes[s][0]
            a2=self.sessiontimes[s][1]

            d=a2-a1

            if a1<start:
                d-=start-a1

            if a2>end:
                d-=a2-end

            #print s,a2-a1
            t+=d

        return t

    def GetEvents(self,type,start,end):
        if type not in self.CodeTypes:
            print "Unrecognized cpap code",field
            return None

        sess=self.GetEvSessions(start,end)
        E=[]
        for s in sess:
            for e in self.session[s][type]:
                if e.time>=start and e.time<=end:
                    E.append(e)
        return E

    def GetEventsPlot(self,type,start,end,dc=None,di=0,padsession=False):
        if type not in self.CodeTypes:
            print "Unrecognized cpap code",field
            return None

        sess=self.GetEvSessions(start,end)
        T=[]
        D=[]
        laste=None
        firste=None
        for s in sess:
            for e in self.session[s][type]:
                if e.time>=start and e.time<=end:
                    if not firste and padsession: 
                        firste=e
                        D.append(0)
                        T.append(e.time)
                    T.append(e.time)
                    if dc:
                        D.append(dc)
                    else:
                        D.append(e.data[di])
                    laste=e

            if padsession:
                if laste:
                    D.append(0)
                    T.append(laste.time)

        return (T,D)

    def GetFlowPlots(self,start,end):
        sess=self.GetFlowSessions(start,end)
        T=[]
        D=[]
        for s in sess:
            X=[]
            Y=[]
            for w in self.flowrate[s]:
                d=timedelta(microseconds=w.rate*1000000.0)
                t=w.time
                for i in w.waveform:
                    if t>=start and t<=end:
                        Y.append(i)
                        X.append(t)
                    t+=d
            T.append(X)
            D.append(Y)
        return (T,D)

    def ScanMachines(self,path):
        print "Pure virtual function"
        exit(1)

    def OpenSD(self):
        self.machine=dict()
        self.session=dict()
        self.sessiontimes=dict()
        self.flowrate=dict()
        self.flowtimes=dict();

        if os.name=="posix":
            posix_mountpoints=["/media","/mnt"]
            d=[]
            for i in posix_mountpoints:
                try:
                    a=os.listdir(i)
                    for j in range(0,len(a)): 
                        a[j]=i+os.sep+a[j]
                        #print j
                    d.extend(a)
                except:
                    1

        elif (os.name=="nt") or (sys.platform=="win32"):
            #Meh.. i'll figure this out later.
            d=['D:','E:','F:','G:','H:','I:','J:']
        #elif sys.platform=="darwin": #Darwin is posix aswell, but where?
        #    d=[]

        r=0
        if not len(d):
            print "I've have no idea where for an SDCard on",os.name,sys.platform
            return 0

        print "Looking for CPAP data in",d
        for i in d:
            if self.ScanMachines(i): 
                r+=1

        return r

    def GetDays(self,numdays=7,date=None):
        DAYS=[]
        if (not date):
            dt=DT.now()#localtz.localize(DT.utcnow())-timedelta(hours=24);
        else:
            dt=date

        for i in range(0,numdays):
            d=dt.date();
            
            (sleep,wake)=cpap.GetBedtime(dt)
            if sleep!=None: 
                ln=wake-sleep
                b=cpap.GetTotalTime(sleep,wake)
                DAYS.append([d,sleep,wake,ln,b])
            dt-=timedelta(hours=24)
        return DAYS


class PRS1(CPAP):
    codes=dict()
    codes[0]=['UN1',[2,1]]
    codes[1]=['UN2',[2,1]]
    codes[2]=['PR',[2,1]]
    codes[3]=['BP',[2,1,1]]
    codes[4]=['PP',[2,1]]
    codes[5]=['RE',[2,1]]
    codes[6]=['OA',[2,1]]
    codes[7]=['CA',[2,1]]
    codes[0xa]=['H',[2,1]]
    codes[0xb]=['UNB',[2,2]]
    codes[0xc]=['FL',[2,1]]
    codes[0xd]=['VS',[2]]
    codes[0xe]=['UNE',[2,1,1,1]]
    codes[0xf]=['CSR',[2,2,1]]
    codes[0x10]=['UN10',[2,2,1]]
    codes[0x11]=['LR',[2,1,1]]
    codes[0x12]=['SUM',[1,1,2]]
    
    def __init__(self):
        CPAP.__init__(self,"Philips Respironics","System One")
    
    def ScanMachines(self,path):
        try:
            d=os.listdir(path);
            r=d.index("P-Series");
        except:
            return False

        path+=os.sep+d[r];
        try:
            d=os.listdir(path);
        except:
            print "Path",path,"unreadable"
            return False

        prs1unit=[]
        l=0
        for f in d:
            if (f[0]!='P'): continue
            if (f[1].isdigit()):
                if f not in self.machine.keys():
                    self.machine[f]=[]
                self.machine[f].append(path+os.sep+f)
                l+=1

        if not l:
            print "No",self.model,"machine data stored under",path
            return False

        return True

    def OpenMachine(self,serial):
        if serial not in self.machine.keys():
            print "Couldn't open device!"
            return False
        
        self.session=dict()
        self.sessiontimes=dict()
        self.flowrate=dict()
        self.flowtimes=dict();

        for path in self.machine[serial]:
            self.ReadMachineData(path,serial)


    def ReadMachineData(self,path,serial):
        try:
            d=os.listdir(path);
            r=d.index("p0");
        except:
            print "Expected PRS1's p0 directory, and couldn't find it",path
            return False

        path+=os.sep+"p0"
        try:
            df=os.listdir(path);
        except:
            print "Couldn't read directory"
            return False
       

        r=0
        for f in df:
            filename=f
            e2=f.rfind('.')
            if (e2<0): continue
            ext=int(f[e2+1:])
            seq=int(f[0:e2])

            if (ext==2) and (not seq in self.session.keys()): 
                if self.Read002(path,filename):
                    r+=1
            elif (ext==5) and (not seq in self.flowrate.keys()):
                if self.Read005(path,filename):
                    r+=1
        if (r>0):
            print "Loaded",r,"files for",serial
        return True

    def Read002(self,path,filename):
        fn=path+os.sep+filename
        try:
            f=open(fn,'rb');
        except:
            print "Couldn't Open File",fn
            return False

        header=f.read(16)
        if (len(header)<16):
            print "Not enough header data in",filename
            f.close()
            return False

        sm=0
        for i in range(0,15): sm+=ord(header[i])
        sm&=0xff

        h1=ord(header[0]);
        filesize,=unpack_from('<I',header,1)
        h2=ord(header[5]);
        filetype=ord(header[6]);
        sequence,=unpack_from('<I',header,7)
        timestamp,=unpack_from('<I',header,11)
        checksum=ord(header[15])

        if (checksum!=sm):
            print "Header checksum error"
            f.close();
            return False

        if (filetype!=2):
            print "Dodgy File",filename
            f.close()
            return False

        if (sequence in self.session.keys()):
            f.close()
            return False;

        starttime=utc.localize(DT.utcfromtimestamp(timestamp))
        
        bytes=16;
        done=0
       
        datasize=filesize-16-2
        data=f.read(datasize);
        if len(data)<datasize:
            print "Short file",filename
            f.close();
            return False
        chksum=f.read(2)
        f.close()

        offsets=[5,6,7,0x0a,0x0c]
        td=starttime
        i=0
        #print "Opening",filename

        self.session[sequence]=dict()
        self.sessiontimes[sequence]=dict()

        for j in self.CodeTypes:
            self.session[sequence][j]=[]

        while (i<datasize):
            c=ord(data[i])
            i+=1
            if c not in self.codes.keys():
                print "Unknown PRS1 code",hex(c),"in file",filename
                exit(1)
            gc=0
            if self.codes[c][0] in self.CodeTypes:
                gc=self.CodeTypes.index(self.codes[c][0]);
            fields=[]
            
            for j in self.codes[c][1]:
                if (j==1):
                    val,=unpack_from('!B',data,i) 
                elif (j==2):
                    val,=unpack_from('<h',data,i)
                else:
                    val=0
                i+=j
                fields.append(val)

            if c!=0x12: #0x12 is a pressure summary and doesn't have a time field
                td+=timedelta(seconds=fields.pop(0))

            d=td
            if c in offsets: #Don't adjust the actual time with these offsets or things will break
                d-=timedelta(seconds=fields[0])

            if (c==0x11 and fields[1]>0): #These events are also classed as vibratory snore
                E=Event(td,c,fields)
                self.session[sequence]['VS'].append(E);

            if (c==2) or (c==3): #CPAP Pressure
                fields[0]/=10.0
                if c==3: fields[1]/=10.0 #Bipap 

            E=Event(d,c,fields)
            #print E.time.astimezone(localtz),self.CodeTypes[gc],fields
            self.session[sequence][self.CodeTypes[gc]].append(E);
            
        self.sessiontimes[sequence]=[starttime,td]
        return True

    def Read005(self,path,filename):
        #print "Importing file",filename
        fn=path+os.sep+filename
        try:
            f=open(fn,'rb');
        except:
            print "Couldn't Open File",fn
            return False

        done=0
        blocks=0;

        starts=None
        while not done:
            header=f.read(24)
            if (len(header)<24):
                if (blocks==0):
                    print "Not enough header data in",filename
                    f.close()
                    return False
                done=1
                break;

            sm=0
            for i in range(0,23): sm+=ord(header[i])
            sm&=0xff

            h1=ord(header[0]);
            blocksize,=unpack_from('<h',header,1)
            h2=ord(header[5]);
            filetype=ord(header[6]);
            sequence,=unpack_from('<I',header,7)
            timestamp,=unpack_from('<I',header,11)
            seconds,=unpack_from('<h',header,15)
            checksum=ord(header[23])

            if (checksum!=sm):
                print filename,"Header checksum error",hex(checksum),hex(sm)
                f.close();
                return False

            if (filetype!=5):
                print "Dodgy File",filename
                f.close()
                return False

            if (blocks==0):
                if sequence in self.flowrate.keys():
                    f.close()
                    print "early close"
                    return False

            starttime=utc.localize(DT.utcfromtimestamp(timestamp))

            if not starts: starts=starttime

            readsize=(blocksize-24)-2; #blocksize-header-checksum
            wfdata=f.read(readsize);
            chksum=f.read(2)

            l=len(wfdata)
            if (l<readsize):
                print "Short data in file",filename
                done=1
                # not sure whether to quit or let it add a dodge record.??

            SampleRate=(5.0*60.0)/1500.0 #or seconds/readsize
            if (blocks==0):
                self.flowrate[sequence]=[]

#                def __init__(self,time,waveform,size,duration,format,rate):

            wave=[]
            for i in wfdata: 
                wave.append((ord(i)^128)-128)

            w=Waveform(starttime,wave,l,timedelta(seconds=seconds),1,SampleRate)
            self.flowrate[sequence].append(w) 

            blocks+=1
        
        ends=starttime+timedelta(seconds=seconds)
        self.flowtimes[sequence]=[starts,ends]
        return True



class Graph:
    xlimits=[];
    ylimits=[];
    name=""
    HL=dict();

    def __init__(self,name):
        self.name=name

    def Create(self,w=15,h=1.5,width=600,height=150):
        self.fig=Figure(figsize=(w,h),facecolor='w',dpi=100)
        self.canvas=FigureCanvas(self.fig) 
        self.canvas.set_size_request(width,height)
        self.ax=self.fig.add_axes([0.05, 0.15, 0.93, 0.7])

    def Redraw(self):
        self.canvas.draw()
    def SetAxis(self,ax):
        self.ax=ax

    def SetTitle(self,name=None,size=12,weight='bold',ha='left',va='bottom'):
        if name:
            self.name=name
        self.ax.set_title(name,ha=ha,va=va,position=[0,1],size=size,weight=weight)

    def Highlight(self,start,end,color,index=0):
        try:
            self.ax.patches.remove(self.HL[index])
            #self.ax.draw_patches(self.HL[index])
        except: 1

        if (start<self.xlimits[0]) or (end>self.xlimits[1]):        #check start and end are within xlimits
            print "Creating Highlights out of xlimit area is a sucky idea in matplotlib";

        self.HL[index]=self.ax.axvspan(start,end,facecolor=color,alpha=0.5)
        #self.ax.draw_patches(self.HL[index])
        self.ResetLimits()

    def SetXLim(self,start,end):
        self.xlimits=[start,end]
        self.ax.set_xlim(self.xlimits)

    def SetYLim(self,bottom,top):
        self.ylimits=[bottom,top]
        self.ax.set_ylim(self.ylimits)

    def ResetLimits(self):
        self.ax.set_xlim(self.xlimits)
        self.ax.set_ylim(self.ylimits)

    def SetDateTicks(self):
        e=self.xlimits[1]-self.xlimits[0]
        self.ax.xaxis.set_major_formatter(DateFormatter("%H:%M",tz=localtz))
        if e>=timedelta(hours=10):
            self.ax.xaxis.set_major_locator(HourLocator(range(0,100,2),tz=localtz))
            self.ax.xaxis.set_minor_locator(MinuteLocator(range( 0,100,10),tz=localtz))
        elif e>=timedelta(hours=4):
            self.ax.xaxis.set_major_locator(HourLocator(range(0,100,1),tz=localtz))
            self.ax.xaxis.set_minor_locator(MinuteLocator(range( 0,100,5),tz=localtz))
        elif e>=timedelta(seconds=3600):
            self.ax.xaxis.set_major_locator(MinuteLocator(range(0,100,30),tz=localtz))
            self.ax.xaxis.set_minor_locator(MinuteLocator(range( 0,100,1),tz=localtz))
        elif e>=timedelta(seconds=1200):
            self.ax.xaxis.set_major_locator(MinuteLocator(range(0,100,5),tz=localtz))
            self.ax.xaxis.set_minor_locator(SecondLocator(range( 0,100,15),tz=localtz))
        elif e>=timedelta(seconds=300):
            self.ax.xaxis.set_major_locator(MinuteLocator(range(0,100,1),tz=localtz))
            self.ax.xaxis.set_minor_locator(SecondLocator(range(0,100,5),tz=localtz))
        else:
            self.ax.xaxis.set_major_locator(SecondLocator(range(0,100,30),tz=localtz))
            self.ax.xaxis.set_minor_locator(SecondLocator(range(0,100,1),tz=localtz))
            self.ax.xaxis.set_major_formatter(DateFormatter("%H:%M:%S",tz=localtz))


class LeaksGraph(Graph):
    def __init__(self,cpap,xlim=[0,0],ylim=[0,129],grid=True):
        Graph.__init__(self,"Leak Rate")
        #self.ax=ax
        self.machine=cpap
        self.Create()
        self.ylimits=ylim
        self.grid=grid
        self.xlimits=xlim
        self.T=[]
        self.D=[]

    def Update(self,start,end):
        if self.xlimits:
            if (start==self.xlimits[0]) and (end==self.xlimits[1]):
                return
        
        #(start,end)=self.machine.FirstLastEventTime('LR',start,end)
        
        (self.T,self.D)=self.machine.GetEventsPlot('LR',start=start,end=end,padsession=True)
        self.xlimits=[self.T[0],self.T[-1]]
       
        avg=sum(self.D)/len(self.D)
        for i in range(0,len(self.D)):
            self.D[i]-=avg;

        print "Average Leaks:",avg

    def Plot(self):
        self.ax.cla()
        self.SetTitle(self.name)
        if (self.grid): self.ax.grid(True);
        
        
        if len(self.T)>0:
            self.ax.plot_date(self.T,self.D,'black',aa=True,tz=localtz)
            self.ax.fill_between(self.T,self.D,0,color='gray')

        
        self.SetDateTicks()
        self.ax.yaxis.set_major_locator(MultipleLocator(20))
        self.ax.yaxis.set_minor_locator(MultipleLocator(5))

        self.ResetLimits()


class PressureGraph(Graph):
    def __init__(self,cpap,xlim=[0,0],ylim=[1,20],grid=True):
        self.name="Pressure"
        #self.ax=ax
        self.machine=cpap
        self.Create()
        self.ylimits=ylim
        self.grid=grid
        self.xlimits=xlim
        self.T=[]
        self.D=[]

    def Update(self,start,end):
        if self.xlimits:
            if (start==self.xlimits[0]) and (end==self.xlimits[1]):
                return
        
        #(start,end)=self.machine.FirstLastEventTime('LR',start,end)
        self.xlimits=[start,end]

        (self.T,self.D)=self.machine.GetEventsPlot('PR',start=start,end=end)
        (self.T1,self.D1)=self.machine.GetEventsPlot('BP',start=start,end=end,di=0)
        (self.T2,self.D2)=self.machine.GetEventsPlot('BP',start=start,end=end,di=1)
        
        #for i in range(0,len(self.D)):
        #    self.D[i]/=10.0;
        #avg=sum(self.D)/len(self.D)

        #print "Average Pressure:",avg

    def Plot(self):
        self.ax.cla()
        self.SetTitle(self.name)
        if (self.grid): self.ax.grid(True);

        if len(self.T)>0:
            self.ax.plot_date(self.T,self.D,'green',aa=True,tz=localtz)
        if (len(self.T1)>0):
            self.ax.plot_date(self.T1,self.D2,'orange',aa=True,tz=localtz)
        if (len(self.T2)>0):
            self.ax.plot_date(self.T2,self.D2,'purple',aa=True,tz=localtz)

        self.SetDateTicks()
        self.ax.yaxis.set_major_locator(MultipleLocator(5))
        self.ax.yaxis.set_minor_locator(MultipleLocator(1))

        self.ResetLimits()


class SleepFlagsGraph(Graph):
    colours=['','y','r','k','b','c','m','g']
    flags=['','RE','VS','FL','H','OA','CA','CSR']
    barcolors=['w','#ffffd0','#ffdfdf','#efefef','#d0d0ff','#cfefff','#ebcdef','#dfffdf','w'];

    marker='.'

    def __init__(self,cpap,waveform,xlim=[0,0],ylim=[0,10],grid=True):
        self.name="Sleep Flags"
        self.waveform=waveform
        #self.ax=ax
        self.machine=cpap
        self.Create(height=175)
        self.ylimits=[0,len(self.flags)]
        self.grid=grid
        self.xlimits=xlim
        self.T=dict()
        self.D=dict()
        self.canvas.mpl_connect('pick_event', self.onpick)
        self.canvas.mpl_connect('button_press_event', self.on_press)
        self.canvas.mpl_connect('button_release_event', self.on_release)
        #self.canvas.mpl_connect('scroll_event', self.on_scroll)
        self.lastscroll=DT.now()
        self.scrollsteps=0

        #self.canvas.mpl_connect('motion_notify_event', self.on_motion)

    def onpick(self,event):
        N = len(event.ind)
        if not N: return True
        return True
        thisline = event.artist
        xdata, ydata = thisline.get_data()
        ind = event.ind
        wavedelta=timedelta(seconds=300) #self.waveform.xlimits[1]-self.waveform.xlimits[0]

        d=timedelta(seconds=wavedelta.seconds/2)
        self.waveform.xlimits[0]=xdata[ind][0]-d
        if (self.waveform.xlimits[0]<self.xlimits[0]): self.waveform.xlimits[0]=self.xlimits[0]
        if (self.waveform.xlimits[0]>(self.xlimits[1]-wavedelta)): self.waveform.xlimits[0]=self.xlimits[1]-wavedelta
        self.waveform.xlimits[1]=self.waveform.xlimits[0]+wavedelta;

        self.Highlight(self.waveform.xlimits[0],self.waveform.xlimits[1],'orange')
        self.Redraw()
        self.waveform.ResetLimits()
        self.waveform.SetDateTicks()
        self.waveform.Redraw()

    def do_scroll(self,steps,event):
        ct=DT.now()
        if (ct<self.scrolltime): 
            return 

        if steps!=self.scrollsteps: 
            return False

        #print "Do Scroll...",steps,event
        centre=self.WaveStart+timedelta(seconds=self.WaveDelta.seconds/2,microseconds=self.WaveDelta.microseconds/2)

        ZoomValue=60;

        s=self.WaveDelta.seconds
        steps=-steps
        self.WaveDelta+=timedelta(seconds=ZoomValue*steps)
        if (self.WaveDelta<timedelta(seconds=60)):
            self.WaveDelta=timedelta(seconds=60)
        self.WaveStart=centre-timedelta(seconds=self.WaveDelta.seconds/2)
        self.WaveEnd=self.WaveStart+self.WaveDelta

        self.Highlight(self.WaveStart,self.WaveEnd,'orange')
        self.Redraw()
        self.waveform.SetXLim(self.WaveStart,self.WaveEnd)
        self.waveform.SetDateTicks()
        self.waveform.Redraw()
        

        return False

    def on_scroll(self,event):
        if event.inaxes != self.ax: return False

        ct=DT.now()
        lt=ct-self.lastscroll;
        self.lastscroll=ct
        if (self.scrollsteps==0 or lt<timedelta(microseconds=200000)):
            self.scrollsteps+=event.step
            self.scrolltime=ct+timedelta(microseconds=400000);
            self.scrolltimer=gobject.timeout_add(500, self.do_scroll,self.scrollsteps,event)
            #callback in 500ms
            return


        self.scrollsteps=0
        return True

    def on_press(self,event):
        if event.inaxes != self.ax: return
 
        contains, attrd = self.ax.patch.contains(event)
        if not contains: return
        #print 'event contains', self.ax.patch.xy
        x0, y0 = self.ax.patch.xy
        self.press = event.xdata, event.ydata

    def on_release(self,event):
        if event.inaxes != self.ax: return
        #if event.inaxes != self.ax: return
        minx=min(event.xdata,self.press[0]);
        maxx=max(event.xdata,self.press[0]);
        d1=num2date(minx,tz=localtz)
        d2=num2date(maxx,tz=localtz)

        wd=self.waveform.xlimits[1]-self.waveform.xlimits[0]

        if (d2-d1)<timedelta(seconds=120):
            if (wd>timedelta(seconds=3600)): wd=timedelta(seconds=300)
            self.waveform.xlimits[0]=d1-timedelta(seconds=wd.seconds/2);
            self.waveform.xlimits[1]=self.waveform.xlimits[0]+wd
        else:
            self.waveform.xlimits[0]=d1
            self.waveform.xlimits[1]=d2

        if (self.waveform.xlimits[0]<self.xlimits[0]):
            self.waveform.xlimits[0]=self.xlimits[0]
            self.waveform.xlimits[1]=self.xlimits[0]+wd

        if (self.waveform.xlimits[1]>self.xlimits[1]):
            self.waveform.xlimits[1]=self.xlimits[1]
            self.waveform.xlimits[0]=self.xlimits[1]-wd

        self.Highlight(self.waveform.xlimits[0],self.waveform.xlimits[1],'orange')
        self.Redraw()

        self.waveform.ResetLimits();
        self.waveform.SetDateTicks()
        self.waveform.Redraw()

    def Update(self,start,end):
        if self.xlimits:
            if (start==self.xlimits[0]) and (end==self.xlimits[1]):
                return
        
        #(start,end)=self.machine.FirstLastEventTime('LR',start,end)
        self.xlimits=[start,end]

        #if self.waveform:
            #self.waveform.xlimits[0]=start
            #self.WaveDelta=timedelta(seconds=300)
            #self.waveform.xlimits[1]=start+self.WaveDelta

        j=0
        for i in self.flags:
            if (i=="CSR"):
                self.T[i]=[]
                E=self.machine.GetEvents(i,start=start,end=end)
                for e in E:
                    r=e.time-timedelta(seconds=e.data[1])-timedelta(seconds=e.data[0]/2)
                    self.T[i].append(r)
                self.D[i]=[j]*len(self.T[i])
            elif (i!=""):
                (self.T[i],self.D[i])=self.machine.GetEventsPlot(i,start=start,end=end,dc=j)
                
            j+=1

    def Plot(self):
        self.ax.cla()
        self.SetTitle(self.name)
        if (self.grid): self.ax.grid(True);
      
        j=0;
        for i in self.flags:
            if (i!=""):
                if (len(self.T[i])>0):
                    self.ax.plot_date(self.T[i],self.D[i],self.colours[j]+self.marker,picker=5,aa=False,tz=localtz,alpha=1)
            j+=1

        self.SetDateTicks()
        self.ax.yaxis.set_major_locator(MultipleLocator(1))
        self.ax.set_yticklabels(self.flags)

        yTicks=[0]
        yTicks.extend(self.ax.get_yticks())
        h=(yTicks[1]-yTicks[0])
        for i in range(1,len(yTicks)):
            yTicks[i]-=h/2
        a1=date2num(self.xlimits[0])
        a2=date2num(self.xlimits[1])
        self.ax.barh(yTicks, [a2-a1]*len(yTicks), height=h, left=a1, color=self.barcolors,alpha=0.5) 

        self.ResetLimits()


class WaveformGraph(Graph):
    colours=['y','r','k','b','c','m','g']
    flags=['RE','VS','FL','H','OA','CA']

    def __init__(self,cpap,xlim=[0,0],ylim=[-69,69],grid=True):
        self.name="Flow Rate Waveform"
        #self.ax=ax
        self.machine=cpap
        self.Create()
        self.ylimits=ylim
        self.grid=grid
        self.xlimits=xlim
        self.T=[]
        self.D=[]
        self.FT=dict()
        self.FD=dict()
        self.canvas.mpl_connect('button_press_event', self.on_press)
        self.canvas.mpl_connect('button_release_event', self.on_release)
        self.canvas.mpl_connect('scroll_event', self.on_scroll)
        self.lastscroll=DT.now()
        self.scrollsteps=0
        self.sg=None

    def set_sleepgraph(self,sg):
        self.sg=sg
    
    def on_press(self,event):
        if event.inaxes != self.ax: return
 
        contains, attrd = self.ax.patch.contains(event)
        if not contains: return
        #print 'event contains', self.ax.patch.xy
        x0, y0 = self.ax.patch.xy
        self.press = event.xdata, event.ydata

    def on_release(self,event):
        if event.inaxes != self.ax: return
        #if event.inaxes != self.ax: return
        #minx=min(event.xdata,self.press[0]);
        #maxx=max(event.xdata,self.press[0]);
        d1=num2date(self.press[0],tz=localtz)
        d2=num2date(event.xdata,tz=localtz)
        d=d2-d1
        self.xlimits[0]-=d
        self.xlimits[1]-=d
        if self.xlimits[0]<self.sg.xlimits[0]:
            self.xlimits[0]=self.sg.xlimits[0]
        if self.xlimits[1]>self.sg.xlimits[1]:
            self.xlimits[1]=self.sg.xlimits[1]
            self.xlimits[0]=self.xlimits[1]-d

        #update SleepGraph
        if (self.sg):
            self.sg.Highlight(self.xlimits[0],self.xlimits[1],'orange')
            self.sg.Redraw()

        self.ResetLimits()
        self.SetDateTicks()
        self.Redraw()

    def do_scroll(self,steps,event):
        ct=DT.now()
        if (ct<self.scrolltime): 
            return 

        if steps!=self.scrollsteps: 
            return False

        #print "Do Scroll...",steps,event
        
        wavedelta=self.xlimits[1]-self.xlimits[0];
        centre=self.xlimits[0]+timedelta(seconds=wavedelta.seconds/2,microseconds=wavedelta.microseconds/2)
        #centre=num2date(event.xdata,tz=localtz)
        ZoomValue=60;

        steps=-steps
        wavedelta+=timedelta(seconds=ZoomValue*steps)
        if (wavedelta<timedelta(seconds=60)):
            wavedelta=timedelta(seconds=60)
        self.xlimits[0]=centre-timedelta(seconds=wavedelta.seconds/2)
        self.xlimits[1]=self.xlimits[0]+wavedelta

        if (self.sg):
            self.sg.Highlight(self.xlimits[0],self.xlimits[1],'orange')
            self.sg.Redraw()
        self.SetDateTicks()
        self.ResetLimits();
        self.Redraw()
        

        return False

    def on_scroll(self,event):
        if event.inaxes != self.ax: return False

        ct=DT.now()
        lt=ct-self.lastscroll;
        self.lastscroll=ct
        if (self.scrollsteps==0 or lt<timedelta(microseconds=200000)):
            self.scrollsteps+=event.step
            self.scrolltime=ct+timedelta(microseconds=400000);
            self.scrolltimer=gobject.timeout_add(500, self.do_scroll,self.scrollsteps,event)
            #callback in 500ms
            return


        self.scrollsteps=0
        return True


    def Update(self,start,end):
        if self.xlimits:
            if (start==self.xlimits[0]) and (end==self.xlimits[1]):
                return
        
        self.xlimits=[start,end]
        (self.T,self.D)=self.machine.GetFlowPlots(start=start,end=end)

        for i in self.flags:
            (self.FT[i],self.FD[i])=self.machine.GetEventsPlot(i,start=start,end=end,dc=50)
        
        self.FT['CSR']=self.machine.GetEvents('CSR',start=start,end=end)

        (self.FT['PP'],self.FD['PP'])=self.machine.GetEventsPlot('PP',start=start,end=end,dc=20)

        (self.FT['PR'],self.FD['PR'])=self.machine.GetEventsPlot('PR',start=start,end=end)

        self.FD['PUP']=[]
        self.FT['PUP']=[]
        self.FD['PDN']=[]
        self.FT['PDN']=[]

        lastpressure=0
        for i in range(0,len(self.FD['PR'])):
            cod=None
            p=self.FD['PR'][i]
            if p>lastpressure: cod="PUP"
            elif p<lastpressure: cod="PDN"
            
            if (cod):
                self.FT[cod].append(self.FT['PR'][i])
                self.FD[cod].append(-65)

            lastpressure=p

    def Plot(self):
        self.ax.cla()
        self.SetTitle(self.name)
        if (self.grid): self.ax.grid(True);

        j=0
        for i in self.flags:
            if (len(self.FT[i])>0):
                self.ax.plot_date(self.FT[i],self.FD[i],self.colours[j]+'d',aa=True,alpha=.8,tz=localtz)
                self.ax.vlines(self.FT[i],50,-50,self.colours[j],lw=1,alpha=0.4)
            j+=1

        j=0

        for i in range(0,len(self.T)):
            self.ax.plot_date(self.T[i],self.D[i],'green',aa=True,tz=localtz,alpha=0.7)

        self.SetDateTicks()
        self.ax.yaxis.set_major_locator(MultipleLocator(20))
        self.ax.yaxis.set_minor_locator(MultipleLocator(5))

        if (len(self.FT['PUP'])):
            self.ax.plot_date(self.FT['PUP'],self.FD['PUP'],'k^',aa=True,alpha=.8,tz=localtz)
        if (len(self.FT['PDN'])):
            self.ax.plot_date(self.FT['PDN'],self.FD['PDN'],'kv',aa=True,alpha=.8,tz=localtz)

        if (len(self.FT['PP'])):
            self.ax.plot_date(self.FT['PP'],self.FD['PP'],'r.',aa=True,alpha=.8,tz=localtz)

        for E in self.FT['CSR']:
            e=E.time-timedelta(seconds=E.data[1]);
            s=e-timedelta(seconds=E.data[0])
            self.ax.axvspan(s,e,facecolor='#d0ffd0');
            #self.Highlight(s,e,color="#d0ffd0",index=j)

        self.ResetLimits()

def AboutBox(a):
    txt='''<big>SleepyHead v0.02</big>

<b>Details:</b>
Author: Mark Watkins (jedimark)
Homepage: <a href='http://sleepyhead.sourceforge.net'>http://sleepyhead.sourceforge.net</a>
Please report any bugs <a href='https://sourceforge.net/projects/sleepyhead/support'>on sourceforge</a>.

<b>License:</b>
This software is released under the <i>GNU Public Licence</i>.

<b>Disclaimer:</b>
This is <b>not</b> medical software. Any output this program
produces should <b>not</b> be used to make medical decisions.

<b>Special Thanks:</b>
Mike Hoolehan - Check out his awesome <a href='http://www.hoolehan.com/onkor/'>Onkor Project</a>
Troy Schultz - For great technical advice
Mark Bruscke - For encouragement and advice

and to the very awesome <a href='http://www.cpaptalk.com'>CPAPTalk Forum</a>
'''
    msg=gtk.MessageDialog(flags=gtk.DIALOG_MODAL,type=gtk.MESSAGE_INFO,buttons=gtk.BUTTONS_CLOSE)
    msg.set_markup(txt)

    msg.run()
    msg.destroy()

def CreateMenu():
    file_menu = gtk.Menu()
    open_item = gtk.MenuItem("_Backup SD Card")
    save_item = gtk.MenuItem("_Print")
    quit_item = gtk.MenuItem("E_xit")
    file_menu.append(open_item)
    file_menu.append(save_item)
    file_menu.append(quit_item)
    quit_item.connect_object ("activate", lambda x: gtk.main_quit(), "file.quit")
    open_item.show()
    save_item.show()
    quit_item.show()

    help_menu = gtk.Menu()
    about_item = gtk.MenuItem("_About")
    about_item.connect_object("activate",AboutBox,"help.about")
    help_menu.append(about_item)
    about_item.show()

    file_item = gtk.MenuItem("_File")
    file_item.show()
    help_item = gtk.MenuItem("_Help")
    help_item.show()

    menu_bar = gtk.MenuBar()
    menu_bar.show()
    file_item.set_submenu(file_menu)
    menu_bar.append(file_item)

    help_item.set_submenu(help_menu)
    menu_bar.append(help_item)
    
    return menu_bar


class DailyGraphs:
    def __init__(self,cpap):
        self.cpap=cpap
        self.layout=gtk.ScrolledWindow()
        self.layout.set_policy(gtk.POLICY_AUTOMATIC,gtk.POLICY_AUTOMATIC)
        self.vbox = gtk.VBox()
        self.layout.add_with_viewport(self.vbox)

        self.graph=dict();
        self.graph['Waveform']=WaveformGraph(cpap)
        self.graph['Leaks']=LeaksGraph(cpap)
        self.graph['Pressure']=PressureGraph(cpap)
        self.graph['SleepFlags']=SleepFlagsGraph(cpap,self.graph['Waveform'])
        self.graph['Waveform'].set_sleepgraph(self.graph['SleepFlags'])

        self.vbox.pack_start(self.graph['SleepFlags'].canvas,expand=False)
        self.vbox.pack_start(self.graph['Waveform'].canvas,expand=False)
        self.vbox.pack_start(self.graph['Leaks'].canvas,expand=False)
        self.vbox.pack_start(self.graph['Pressure'].canvas,expand=False)
        
        self.machines=gtk.combo_box_new_text()

        for mach in cpap.machine.keys():
            self.machines.append_text(mach)

        self.datesel=gtk.Calendar()

        self.textbox=gtk.TextView(buffer=None)
        self.textbox.set_editable(False)

        self.datesel.connect('month_changed',self.cal_month_selected,cpap)
        self.datesel.connect('day_selected',self.cal_day_selected)

        self.machines.connect("changed",self.select_machine,cpap)

        self.databox = gtk.VBox(homogeneous=False)
        self.rescanbutton=gtk.Button("_Rescan Media")
        self.rescanbutton.connect('pressed',self.pushed_rescan)
        #self.zeobutton=gtk.Button("Load _ZEO Data")
        #self.oxibutton=gtk.Button("Load _Oximeter Data")
        self.databox.pack_start(self.machines,expand=False,padding=2)
        self.databox.pack_start(self.datesel,expand=False,padding=2)
        self.databox.pack_start(self.rescanbutton,expand=False,padding=0)
        #self.databox.pack_start(self.zeobutton,expand=False,padding=0)
        #self.databox.pack_start(self.oxibutton,expand=False,padding=0)
        self.databox.pack_start(self.textbox,expand=True,padding=2)

        self.machines.set_active(0)
        #self.cal_month_selected(self.datesel,cpap)
        #self.cal_day_selected(self.datesel)

    def select_machine(self,combo,cpap):
        msg=gtk.MessageDialog(type=gtk.MESSAGE_INFO,buttons=gtk.BUTTONS_NONE,message_format="Please wait, Loading CPAP Data")
        msg.show_all()
        gtk.gdk.window_process_all_updates() 

        mach=combo.get_active_text();
        cpap.OpenMachine(mach)
        msg.destroy()
        self.cal_month_selected(self.datesel,self.cpap)
        self.cal_day_selected(self.datesel)

    def pushed_rescan(self,event):

        self.cpap.OpenSD()

        mach=self.machines.get_active_text();
        self.machines.get_model().clear()
        j=0
        cmi=-1
        for m in cpap.machine.keys():
            i=self.machines.insert_text(j,m)
            if (m==mach): cmi=j
            j+=1

        if (cmi>=0):
            self.machines.set_active(cmi)
        else:
            self.machines.set_active(0)

        
        #cpap.OpenMachine(mach)

        self.cal_month_selected(self.datesel,self.cpap)
        #self.cal_day_selected(self.datesel)

    def Draw(self):
        for k,v in self.graph.iteritems():
            v.Redraw()

    def ShowGraphs(self,show):
        if show: 
            vis=True
        else:
            vis=False

        for i in self.graph.keys():
            self.graph[i].canvas.set_visible(vis)

    def Update(self,start,end):
        for k,v in self.graph.iteritems():
            v.Update(start,end)
        sess=cpap.GetFlowSessions(start,end)
        if (len(sess)>0):
            wvis=True;
        else: wvis=False;
        self.graph['Waveform'].canvas.set_visible(wvis)
        text="Date: "+start.astimezone(localtz).strftime("%Y-%m-%d")+"\n\n"
        text+="Bedtime: "+start.astimezone(localtz).strftime("%H:%M:%S")+"\n"
        text+="Waketime: "+end.astimezone(localtz).strftime("%H:%M:%S")+"\n\n"

        tt=cpap.GetTotalTime(start,end)
        text+="Total Time: "+str(tt)+"\n\n"

        if not wvis:
            text+="No Waveform Data Available\n\n"
        oa=cpap.CountEvents('OA',start,end)
        h=cpap.CountEvents('H',start,end)
        ah=oa+h
        ca=cpap.CountEvents('CA',start,end)
        fl=cpap.CountEvents('FL',start,end)
        vs=cpap.CountEvents('VS',start,end)
        re=cpap.CountEvents('RE',start,end)

        PR=cpap.GetEvents('PR',start,end)
        
        if (len(PR)>0):
            avgp=0
            laste=PR[0]
            lastp=int(PR[0].data[0]*10)
            lastt=PR[0].time
            TPR=[timedelta(seconds=0)]*256
            don=False
            totalptime=timedelta(0)
            for e in PR[1:]: 
                p=int(e.data[0]*10)
                TPR[lastp]+=(e.time-lastt)
                totalptime+=(e.time-lastt)
                lastt=e.time
                lastp=p

       
        #if (not don):
        #    TPR[lastp]+=lastt-

            np=timedelta(seconds=totalptime.seconds*.9)
            npc=timedelta(seconds=0)
            npp=0
            lastp=0
            for i in range(0,256):
                lpc=npc
                npc+=TPR[i]
                if (npc>=np):
                    s2=1-(float(lpc.seconds)/float(npc.seconds))
                    d=(i-lastp)/10.0
                    npp=(lastp/10.0)+(s2*d)
                    break

                if TPR[i]>timedelta(seconds=0):
                    lastp=i

            avgp=0

            sm1=0
            sm2=0
            sm3=0
            for i in range(0,256):
                if TPR[i]>timedelta(seconds=0):
                    s=float(TPR[i].seconds)/float(totalptime.seconds)
                    sm1+=s*float(i)
                    sm2+=s
                    sm3=(float(i)/10.0)*TPR[i].seconds

            #avgp=sm3/totalptime.seconds
            avgp=sm1/sm2/10.0   #Weighted Average
        else:
            avgp=0
            npp=0

        LK=cpap.GetEvents('LR',start,end);
        avgl=0
        for e in LK: avgl+=e.data[0]-19
        avgl/=len(LK)
            
           

        CSR=cpap.GetEvents('CSR',start,end);
        dur=0
        for e in CSR: dur+=e.data[0];
        csr=(100.0/tt.seconds)*dur

        text+="Average Pressure=%(#)0.2f\n"%{'#':avgp}
        text+="90%% Pressure=%(#)0.2f\n\n"%{'#':npp}

        text+="CSR %% of night=%(#)0.2f\n" % {"#":csr}

        s=tt.seconds/3600.0
        text+="OA=%(#)0.2f\n"%{'#':oa/s}
        text+="H=%(#)0.2f\n"%{'#':h/s}
        text+="CA=%(#)0.2f\n"%{'#':ca/s}
        text+="FL=%(#)0.2f\n"%{'#':fl/s}
        text+="VS=%(#)0.2f\n"%{'#':vs/s}
        text+="RE=%(#)0.2f\n"%{'#':re/s}
        text+="AHI=%(#)0.2f\n\n"%{'#':ah/s}

        text+="Leak=%(#)0.2f\n"%{'#':avgl}


        buf=self.textbox.get_buffer()
        buf.set_text(text)
            #self.date.set_text())
#        self.bedtime.set_text("Bedtime: "+start.astimezone(localtz).strftime("%H:%M:%S"))
#        self.waketime.set_text("Waketime: "+end.astimezone(localtz).strftime("%H:%M:%S"))

    def Plot(self):
        for k,v in self.graph.iteritems():
            v.Plot()
        self.graph['Waveform'].ResetLimits()

    def cal_month_selected(self,cal,cpap):
        (y,m,d)=cal.get_date();

        d=1
        m+=2
    
        if (m>11):
            y+=1
            m%=12

        ldom=DT(y,m,d,0,0,0)-timedelta(hours=1)
        #print "Getting",ldom.day,"days back from",ldom
        D=cpap.GetDays(ldom.day,date=ldom)
        cal.freeze()
        for i in range(0,ldom.day-1):
            cal.unmark_day(i)

        for i in D:
            cal.mark_day(i[0].day)

        cal.thaw()

    def cal_day_selected(self,cal):
        (y,m,d)=cal.get_date()
        dat=DT(y,m+1,d,0,0,0)
        (st,et)=cpap.GetBedtime(dat)

        if st:
            msg=gtk.MessageDialog(type=gtk.MESSAGE_INFO,buttons=gtk.BUTTONS_NONE,message_format="Updating Plots - Please wait")
            msg.show_all()
            gtk.gdk.window_process_all_updates() 

            self.ShowGraphs(True)
            #print "Bedtime",st.astimezone(localtz),"Wakeup",et.astimezone(localtz)
            self.Update(st,et)
            self.Plot()
            self.Draw()
            msg.destroy()
        else:
            self.ShowGraphs(False)
            text="No data available for selected date"
            buf=self.textbox.get_buffer()
            buf.set_text(text)
            

path="/home/mark/.sleepyhead/CMS50"
#cms50=CMS50X()
#(event,wave)=cms50.Record(path)

#exit(1)

cpap=PRS1()
cpap.OpenSD()

win=gtk.Window()
win.connect("destroy", lambda x: gtk.main_quit())
win.set_default_size(1200,680)
win.set_title("SleepyHead v0.02")

mainbox=gtk.VBox()
mainbox.pack_start(CreateMenu(),expand=False)

notebook=gtk.Notebook()
notebook.unset_flags(gtk.CAN_FOCUS)

dailybox=gtk.HBox()

spo2box=gtk.HBox()


mainbox.pack_start(notebook,expand=True)

DG=DailyGraphs(cpap)
dailybox.pack_start(DG.databox,expand=False)
dailybox.pack_start(DG.layout,expand=True)

page1=notebook.insert_page(dailybox,gtk.Label("Daily"))
#page2=notebook.insert_page(dailybox,gtk.Label("Overview"))
#page3=notebook.insert_page(spo2box,gtk.Label("SpO2"))
notebook.set_current_page(page1)

win.add(mainbox)


win.show_all()
gtk.main()



