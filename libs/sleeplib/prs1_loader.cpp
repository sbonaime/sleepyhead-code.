/*

SleepLib PRS1 Loader Implementation

Author: Mark Watkins <jedimark64@users.sourceforge.net>
License: GPL
*/

#include <wx/dir.h>
#include <wx/filename.h>
#include <wx/ffile.h>
#include <wx/textfile.h>
#include <wx/string.h>
#include <wx/datetime.h>
#include <wx/log.h>
#include <wx/progdlg.h>

#include <initializer_list>

#include "prs1_loader.h"


extern wxProgressDialog *loader_progress;

map<int,wxString> ModelMap= {
    {34,wxT("RemStar Pro with C-Flex+")},
    {35,wxT("RemStar Auto with A-Flex")}
};

PRS1::PRS1(Profile *p,MachineID id):CPAP(p,id)
{
    m_class=wxT("PRS1");
    properties[wxT("Brand")]=wxT("Philips Respironics");
    properties[wxT("Model")]=wxT("System One");

    SleepFlags= { CPAP_RERA, PRS1_VSnore2, CPAP_FlowLimit, CPAP_Hypopnea, CPAP_Obstructive, CPAP_ClearAirway, CPAP_CSR };
}
PRS1::~PRS1()
{

}


PRS1Loader::PRS1Loader()
{
}

PRS1Loader::~PRS1Loader()
{
    for (auto i=PRS1List.begin(); i!=PRS1List.end(); i++) {
        delete i->second;
    }
}
Machine *PRS1Loader::CreateMachine(wxString serial,Profile *profile)
{
    wxLogMessage(wxT("Create Machine")+serial);
    if (!profile) {
        wxLogMessage(wxT("No Profile!"));
        return NULL;

    }
    vector<Machine *> ml=profile->GetMachines(MT_CPAP);
    bool found=false;
    for (auto i=ml.begin(); i!=ml.end(); i++) {
        if (((*i)->GetClass()==wxT("PRS1")) && ((*i)->properties[wxT("Serial")]==serial)) {
            PRS1List[serial]=*i; //static_cast<CPAP *>(*i);
            found=true;
            break;
        }
    }
    if (found) return PRS1List[serial];

    //assert(PRS1List.find(serial)==PRS1List.end())
    //wxPuts(wxT("Creating CPAP Machine ")+serial);
    Machine *m=new PRS1(profile,0);

    PRS1List[serial]=m;
    profile->AddMachine(m);

    m->properties[wxT("Serial")]=serial;

    return m;
}
bool PRS1Loader::Open(wxString & path,Profile *profile)
{

    wxString newpath;
    wxString sep=wxFileName::GetPathSeparator();
    wxString pseries=wxT("P-Series");
    if (path.Right(pseries.Len()+sep.Len())==pseries) {
        newpath=path;
    } else {
        newpath=path+sep+pseries;
    }
    wxDir dir;
    dir.Open(newpath);
    if (!dir.IsOpened()) return 0;

    list<wxString> SerialNumbers;
    list<wxString>::iterator sn;

    wxString filename;
    bool cont=dir.GetFirst(&filename);

    int c=0;
    while (cont) {
        if ((filename[0]=='P') && (isdigit(filename[1])) && (isdigit(filename[2]))) {
            SerialNumbers.push_back(filename);
        } else if (filename.Lower()==wxT("last.txt")) { // last.txt points to the current serial number
            wxTextFile f(newpath+sep+filename);
            f.Open();
            last=f.GetFirstLine();
            last.Strip();
            f.Close();
        }

        cont=dir.GetNext(&filename);
    }

    if (SerialNumbers.empty()) return 0;

    Machine *m;
    for (sn=SerialNumbers.begin(); sn!=SerialNumbers.end(); sn++) {
        wxString s=*sn;
        m=CreateMachine(s,profile);

        if (m) OpenMachine(m,newpath+wxFileName::GetPathSeparator()+(*sn));
    }

    return PRS1List.size();

    return c;
}
bool PRS1Loader::ParseProperties(Machine *m,wxString filename)
{
    wxTextFile f(filename);
    f.Open();
    if (!f.IsOpened()) return false;

    wxString line;
    map<wxString,wxString> prop;

    wxString s=f.GetFirstLine();
    wxChar sep=wxChar('=');
    wxString key,value;
    while (!f.Eof()) {
        key=s.BeforeFirst(sep);
        if (key==s) continue;
        value=s.AfterFirst(sep).Strip();
        if (value==s) continue;
        prop[key]=value;
        s=f.GetNextLine();
    }

    if (prop[wxT("ProductType")].IsNumber()) {
        long i;
        prop[wxT("ProductType")].ToLong(&i);
        if (ModelMap.find(i)!=ModelMap.end()) {
            m->properties[wxT("SubModel")]=ModelMap[i];
        }
    }
    if (prop[wxT("SerialNumber")]!=m->properties[wxT("Serial")]) {
        wxLogWarning(wxT("Serial Number in PRS1 properties.txt doesn't match directory structure"));
    } else prop.erase(wxT("SerialNumber")); // already got it stored.

    for (auto i=prop.begin(); i!=prop.end(); i++) {
        m->properties[i->first]=i->second;
    }

    f.Close();
    return true;
}

int PRS1Loader::OpenMachine(Machine *m,wxString path)
{

    wxLogDebug(wxT("Opening PRS1 ")+path);
    //wxPuts(wxT("opening "+path));
    wxDir dir;
    dir.Open(path);
    if (!dir.IsOpened()) return false;

    wxString pathname,filename;

    bool cont=dir.GetFirst(&filename);
    list<wxString> paths;

    if(loader_progress) {
        loader_progress->Update(0);
    }
    while (cont) {
        pathname=path+wxFileName::GetPathSeparator()+filename;
        if ((filename[0]==wxChar('p')) && (isdigit(filename[1]))) {
            paths.push_back(pathname);
        } else if (filename.Lower()==wxT("properties.txt")) {
            ParseProperties(m,pathname);
        } else if (filename.Lower()==wxT("e")) {
            // don't really give a crap about .004 files yet.
        }
        if (loader_progress) loader_progress->Pulse();
        cont=dir.GetNext(&filename);
    }

    SessionID session;
    long ext;
    typedef vector<wxString> StringList;
    map<SessionID,StringList> sessfiles;
    int size=paths.size();
    int cnt=0;
    for (auto p=paths.begin(); p!=paths.end(); p++) {
        dir.Open(*p);
        if (!dir.IsOpened()) continue;;
        bool cont=dir.GetFirst(&filename);

        while (cont) {
            wxString ext_s=filename.AfterLast(wxChar('.'));
            wxString session_s=filename.BeforeLast(wxChar('.'));

            if (!ext_s.IsNumber()) continue;
            if (!session_s.IsNumber()) continue;

            session_s.ToLong(&session);
            ext_s.ToLong(&ext);
            if (sessfiles[session].capacity()==0) sessfiles[session].resize(3);

            wxString fullname=*p+wxFileName::GetPathSeparator()+filename;
            if (ext==1) {
                sessfiles[session][0]=fullname;
            } else if (ext==2) {
                sessfiles[session][1]=fullname;
            } else if (ext==5) {
                sessfiles[session][2]=fullname;
            }
            cnt++;
            if (loader_progress) loader_progress->Pulse(); //Update((float(cnt)/float(size)*25));
            //if (loader_progress) loader_progress->Update((float(cnt)/float(size)*25.0));
            cont=dir.GetNext(&filename);
        }
    }
    if (sessfiles.size()==0) return 0;

    size=sessfiles.size();
    cnt=0;
    for (auto s=sessfiles.begin(); s!=sessfiles.end(); s++) {
        session=s->first;
        cnt++;
        if (loader_progress) loader_progress->Update(25.0+(float(cnt)/float(size)*25.0));

        if (m->SessionExists(session)) continue;
        if (!s->second[0]) continue;

        Session *sess=new Session(m,session);
        if (!OpenSummary(sess,s->second[0])) {
            wxLogWarning(wxT("PRS1Loader: Could'nt open summary file ")+s->second[0]);

            delete sess;
            continue;
        }
        const double ignore_thresh=300.0/3600.0;// Ignore useless sessions under 5 minute
        if (sess->hours()<ignore_thresh) {
            delete sess;
            continue;
        }

        //sess->SetSessionID(sess->start().GetTicks());
        m->AddSession(sess);
        if (!s->second[1].IsEmpty()) {
            if (!OpenEvents(sess,s->second[1])) {
                wxLogWarning(wxT("PRS1Loader: Couldn't open event file ")+s->second[1]);
            }
        }

        if (!s->second[2].IsEmpty()) {
            if (!OpenWaveforms(sess,s->second[2])) {
                wxLogWarning(wxT("PRS1Loader: Couldn't open event file ")+s->second[2]);
            }
        }

        sess->summary[CPAP_CSR]=sess->sum_event_field(CPAP_CSR,0);
        sess->summary[CPAP_VSnore]=(long)sess->count_events(CPAP_VSnore);
        sess->summary[PRS1_VSnore2]=sess->sum_event_field(PRS1_VSnore2,0);
        sess->summary[CPAP_PressureMedian]=sess->avg_event_field(CPAP_Pressure,0);

        sess->summary[CPAP_LeakMinimum]=sess->min_event_field(CPAP_Leak,0);
        sess->summary[CPAP_LeakMaximum]=sess->max_event_field(CPAP_Leak,0); // should be merged..
        sess->summary[CPAP_LeakMedian]=sess->avg_event_field(CPAP_Leak,0);
        sess->summary[CPAP_LeakAverage]=sess->weighted_avg_event_field(CPAP_Leak,0);

        //wxPrintf(sess->start().Format()+wxT(" avgsummary=%.3f avgmine=%.3f\n"),sess->summary[CPAP_PressureAverage].GetDouble(),sess->weighted_avg_event_field(CPAP_Pressure,0));
        sess->SetChanged(true);
    }
    m->Save(); // Save any new sessions to disk in our format
    if (loader_progress) loader_progress->Update(100);
    return true;
}

bool PRS1Loader::OpenSummary(Session *session,wxString filename)
{
    int size,sequence,seconds,br;
    time_t timestamp;
    unsigned char header[24];
    unsigned char ext;

    //wxLogMessage(wxT("Opening PRS1 Summary ")+filename);
    wxFFile f(filename,wxT("rb"));

    if (!f.IsOpened()) return false;

    int hl=16;

    br=f.Read(header,hl);

    if (header[0]!=header[5]) return false;

    sequence=size=timestamp=seconds=ext=0;
    sequence=(header[10] << 24) | (header[9] << 16) | (header[8] << 8) | header[7];
    timestamp=(header[14] << 24) | (header[13] << 16) | (header[12] << 8) | header[11];
    size=(header[2] << 8) | header[1];
    ext=header[6];

    if (ext!=1) return false;
    //size|=(header[3]<<16) | (header[4]<<24); // the jury is still out on the 32bitness of one. doesn't matter here anyway.

    size-=(hl+2);

    unsigned char sum=0;
    for (int i=0; i<hl-1; i++) sum+=header[i];
    if (sum!=header[hl-1]) return false;

    wxDateTime date(timestamp);
    //wxDateTime tmpdate=date;
    //wxLogMessage(date.Format()+wxT(" UTC=")+tmpdate.Format());
    //int ticks=date.GetTicks();



    if (!date.IsValid()) return false;

    unsigned char *buffer=(unsigned char *)malloc(size);
    br=f.Read(buffer,size);
    if (br<size) {
        delete buffer;
        return false;
    }

    session->set_first(date);
    double max;
    session->summary[CPAP_PressureMin]=(double)buffer[0x03]/10.0;
    session->summary[CPAP_PressureMax]=max=(double)buffer[0x04]/10.0;
    session->summary[CPAP_RampTime]=(long)buffer[0x06]; // Minutes. Convert to seconds/hours here?
    session->summary[CPAP_RampStartingPressure]=(double)buffer[0x07]/10.0;

    if (max>0) { // Ignoring bipap until I see some more data.
        session->summary[CPAP_Mode]=(long)MODE_APAP;
    } else session->summary[CPAP_Mode]=(long)MODE_CPAP;

    // This is incorrect..
    if (buffer[0x08] & 0x80) { // Flex Setting
        if (buffer[0x08] & 0x08) {
            if (max>0) session->summary[CPAP_PressureReliefType]=(long)PR_AFLEX;
            else session->summary[CPAP_PressureReliefType]=(long)PR_CFLEXPLUS;
        } else session->summary[CPAP_PressureReliefType]=(long)PR_CFLEX;
    } else session->summary[CPAP_PressureReliefType]=(long)PR_NONE;

    session->summary[CPAP_PressureReliefSetting]=(long)buffer[0x08] & 3;
    session->summary[CPAP_HumidifierSetting]=(long)buffer[0x09]&0x0f;
    session->summary[CPAP_HumidifierStatus]=(buffer[0x09]&0x80)==0x80;
    session->summary[PRS1_SystemLockStatus]=(buffer[0x0a]&0x80)==0x80;
    session->summary[PRS1_SystemResistanceStatus]=(buffer[0x0a]&0x40)==0x40;
    session->summary[PRS1_SystemResistanceSetting]=(long)buffer[0x0a]&7;
    session->summary[PRS1_HoseDiameter]=(long)((buffer[0x0a]&0x08)?15:22);
    session->summary[PRS1_AutoOff]=(buffer[0x0c]&0x10)==0x10;
    session->summary[PRS1_MaskAlert]=(buffer[0x0c]&0x08)==0x08;
    session->summary[PRS1_ShowAHI]=(buffer[0x0c]&0x04)==0x04;

    int duration=buffer[0x14] | (buffer[0x15] << 8);
    float hours=float(duration)/3600.0;
    session->set_hours(hours);

    session->set_last(session->first()+wxTimeSpan::Seconds(duration));

    session->summary[CPAP_PressureMinAchieved]=buffer[0x16]/10.0;
    session->summary[CPAP_PressureMaxAchieved]=buffer[0x17]/10.0;
    session->summary[CPAP_PressurePercentValue]=buffer[0x18]/10.0;
    session->summary[CPAP_PressurePercentName]=90.0;
    session->summary[CPAP_PressureAverage]=buffer[0x19]/10.0;

    if (max==0) {
        session->summary[CPAP_PressureAverage]=session->summary[CPAP_PressureMin];
    }

    session->summary[CPAP_Obstructive]=(long)buffer[0x1C] | (buffer[0x1D] << 8);
    session->summary[CPAP_ClearAirway]=(long)buffer[0x20] | (buffer[0x21] << 8);
    session->summary[CPAP_Hypopnea]=(long)buffer[0x2A] | (buffer[0x2B] << 8);
    session->summary[CPAP_RERA]=(long)buffer[0x2E] | (buffer[0x2F] << 8);
    session->summary[CPAP_FlowLimit]=(long)buffer[0x30] | (buffer[0x31] << 8);

    delete buffer;
    return true;
}

bool PRS1Loader::Parse002(Session *session,unsigned char *buffer,int size,time_t timestamp)
{
    vector<MachineCode> Codes= {
        PRS1_Unknown00, PRS1_Unknown01, CPAP_Pressure, CPAP_EAP, PRS1_PressurePulse, CPAP_RERA, CPAP_Obstructive, CPAP_ClearAirway,
        PRS1_Unknown08, PRS1_Unknown09, CPAP_Hypopnea, PRS1_Unknown0B, CPAP_FlowLimit, CPAP_VSnore, PRS1_Unknown0E, CPAP_CSR, PRS1_Unknown10,
        CPAP_Leak, PRS1_Unknown12
    };
    wxDateTime start;
    start.Set(timestamp);
    wxDateTime t=start;
    //t.Set(timestamp);
    int pos=0;
    int cnt=0;
    while (pos<size) {
        char code=buffer[pos++];
        short size;
        if (code!=0x12) {
            size=buffer[pos] | buffer[pos+1] << 8;
            pos+=2;
            t+=wxTimeSpan::Seconds(size);
        }
        float data0,data1,data2;
        MachineCode cpapcode=Codes[(int)code];
        wxDateTime tt=t;
        cnt++;
        switch (code) {
        case 0x00: // Unknown
        case 0x01: // Unknown
        case 0x02: // Pressure
            data0=buffer[pos++]/10.0;
            session->AddEvent(new Event(t,cpapcode, {data0}));
            break;
        case 0x04: // Pressure Pulse
            data0=buffer[pos++];
            session->AddEvent(new Event(t,cpapcode, {data0}));
            break;

        case 0x05: // RERA
        case 0x06: // Obstructive Apoanea
        case 0x07: // Clear Airway
        case 0x0a: // Hypopnea
        case 0x0c: // Flow Limitation
            tt-=wxTimeSpan::Seconds((buffer[pos++])); // Subtract Time Offset
            session->AddEvent(new Event(tt,cpapcode, {}));
            break;
        case 0x0d: // Vibratory Snore
            session->AddEvent(new Event(t,cpapcode, {}));
            break;
        case 0x03: // BIPAP Pressure
        case 0x0b: // Unknown
        case 0x11: // Leak Rate
            data0=buffer[pos++];
            data1=buffer[pos++];
            if (code==0x11) {
                session->AddEvent(new Event(t,cpapcode, {data0}));
                if (data1>0) {
                    session->AddEvent(new Event(tt,PRS1_VSnore2, {data1}));
                }
            } else if (code==0x03) {
                session->AddEvent(new Event(t,CPAP_EAP, {data0,data1}));
                session->AddEvent(new Event(t,CPAP_IAP, {data1}));
            } else {
                session->AddEvent(new Event(t,cpapcode, {data0,data1}));
            }
            break;
        case 0x0e: // Unknown
        case 0x10: // Unknown
            data0=buffer[pos++];
            data1=buffer[pos++];
            data2=buffer[pos++];
            session->AddEvent(new Event(t,cpapcode, {data0,data1,data2}));
            break;
        case 0x0f: // Cheyne Stokes Respiration
            data0=buffer[pos+1]<<8 | buffer[pos];
            pos+=2;
            data1=buffer[pos++];
            tt-=wxTimeSpan::Seconds(data1);
            session->AddEvent(new Event(tt,cpapcode, {data0,data1}));
            break;
        case 0x12: // Summary
            data0=buffer[pos++];
            data1=buffer[pos++];
            data2=buffer[pos+1]<<8 | buffer[pos];
            pos+=2;
            session->AddEvent(new Event(t,cpapcode, {data0,data1,data2}));
            break;
        default:
            // ERROR!!!
            throw exception(); // UnknownCode();
            break;
        }
    }
    return true;
}

bool PRS1Loader::OpenEvents(Session *session,wxString filename)
{
    int size,sequence,seconds,br;
    time_t timestamp;
    unsigned char header[24];
    unsigned char ext;

    //wxLogMessage(wxT("Opening PRS1 Events ")+filename);
    wxFFile f(filename,wxT("rb"));

    int hl=16;

    br=f.Read(header,hl);

    if (header[0]!=header[5]) return false;

    sequence=size=timestamp=seconds=ext=0;
    sequence=(header[10] << 24) | (header[9] << 16) | (header[8] << 8) | header[7];
    timestamp=(header[14] << 24) | (header[13] << 16) | (header[12] << 8) | header[11];
    size=(header[2] << 8) | header[1];
    ext=header[6];

    if (ext!=2) return false;

    //size|=(header[3]<<16) | (header[4]<<24); // the jury is still out on the 32bitness of one. doesn't matter here anyway.

    size-=(hl+2);

    unsigned char sum=0;
    for (int i=0; i<hl-1; i++) sum+=header[i];
    if (sum!=header[hl-1]) return false;

    unsigned char *buffer=(unsigned char *)malloc(size);
    br=f.Read(buffer,size);
    if (br<size) {
        delete buffer;
        return false;
    }
    Parse002(session,buffer,size,timestamp);

    delete buffer;
    return true;
}

bool PRS1Loader::OpenWaveforms(Session *session,wxString filename)
{
    //wxLogMessage(wxT("Opening PRS1 Waveforms ")+filename);

    int size,sequence,seconds,br;
    unsigned cnt=0;
    time_t timestamp;

    time_t start=0;
    unsigned char header[24];
    unsigned char ext;

    wxFFile f(filename,wxT("rb"));
    int hl=24;
    vector<char *> wavedata;
    vector<int> wavesize;
    int samples=0;
    int duration=0;
    while (true) {
        br=f.Read(header,hl);

        if (br<hl) {
            if (cnt==0)
                return false;
            else break;
        }

        if (header[0]!=header[5]) {
            if (cnt==0)
                return false;
            else break;
        }

        sequence=size=timestamp=seconds=ext=0;
        sequence=(header[10] << 24) | (header[9] << 16) | (header[8] << 8) | header[7];
        timestamp=(header[14] << 24) | (header[13] << 16) | (header[12] << 8) | header[11];
        size=(header[2] << 8) | header[1];
        ext=header[6];

        if (sequence==328) {
            seconds=0;
        }
        if (!start) start=timestamp;
        seconds=(header[16] << 8) | header[15];

        size-=(hl+2);

        if (ext!=5) {
            if (cnt==0)
                return false;
            else break;
        }

        unsigned char sum=0;
        for (int i=0; i<hl-1; i++) sum+=header[i];
        if (sum!=header[hl-1])
            return false;

        char * buffer=new char [size];
        br=f.Read(buffer,size);
        if (br<size) {
            delete [] buffer;
            break;
        }
        wavedata.push_back(buffer);
        wavesize.push_back(size);
        cnt++;

        duration+=seconds;
        samples+=size;

        char chkbuf[2];
        wxInt16 chksum;
        br=f.Read(chkbuf,2);
        if (br<2)
            return false;
        chksum=chkbuf[0] << 8 | chkbuf[1];

    }

    if (samples==0)
        return false;

    //double rate=double(duration)/double(samples);

    SampleFormat *data=new SampleFormat [samples];
    int s=0;

    SampleFormat min,max,c;
    bool first=true;
    //assert (cnt==wavedata.size());
    cnt=wavedata.size();
    for (unsigned i=0; i<cnt; i++) {
        for (int j=0; j<wavesize[i]; j++) {
            c=wavedata[i][j];
            if (first) {
                min=max=c;
            }
            if (min>c) min=c;
            if (max<c) max=c;
            data[s++]=c;
        }
        delete wavedata[i];
    }
    Waveform *w=new Waveform(start,CPAP_FlowRate,data,samples,duration,min,max);
    session->AddWaveform(w);
    //wxLogMessage(wxT("Done PRS1 Waveforms ")+filename);
    return true;
}


bool initialized=false;
void PRS1Loader::Register()
{
    if (initialized) return;
    wxLogVerbose(wxT("Registering PRS1Loader"));
    RegisterLoader(new PRS1Loader());
    initialized=true;
}

