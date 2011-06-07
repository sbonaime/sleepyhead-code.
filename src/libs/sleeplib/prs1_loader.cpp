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
#include <wx/msgdlg.h>

#include "prs1_loader.h"


extern wxProgressDialog *loader_progress;

map<int,wxString> ModelMap;

// This class technically isn't needed now.. as long as m_class, Brand & Model is set, things should be fine.
PRS1::PRS1(Profile *p,MachineID id):CPAP(p,id)
{
    m_class=wxT("PRS1");
    properties[wxT("Brand")]=wxT("Philips Respironics");
    properties[wxT("Model")]=wxT("System One");

    //SleepFlags= { CPAP_RERA, PRS1_VSnore2, CPAP_FlowLimit, CPAP_Hypopnea, CPAP_Obstructive, CPAP_ClearAirway, CPAP_CSR };
}
PRS1::~PRS1()
{

}


PRS1Loader::PRS1Loader()
{
    m_buffer=new unsigned char [max_load_buffer_size]; //allocate once and reuse.
}

PRS1Loader::~PRS1Loader()
{
    for (map<wxString,Machine *>::iterator i=PRS1List.begin(); i!=PRS1List.end(); i++) {
        delete i->second;
    }
    delete [] m_buffer;
}
Machine *PRS1Loader::CreateMachine(wxString serial,Profile *profile)
{
    wxLogMessage(wxT("Create Machine ")+serial);
    if (!profile) {  // shouldn't happen..
        wxLogMessage(wxT("No Profile!"));
        return NULL;

    }
    vector<Machine *> ml=profile->GetMachines(MT_CPAP);
    bool found=false;
    for (vector<Machine *>::iterator i=ml.begin(); i!=ml.end(); i++) {
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
    if (path.Right(pseries.Len()+sep.Len())==sep+pseries) {
        newpath=path;
    } else {
        newpath=path+sep+pseries;
    }
	wxLogDebug( wxT("PRS1Loader::Open newpath=")+newpath );
    wxDir dir;
    dir.Open(newpath);
    if (!dir.IsOpened()) return 0;

	wxLogDebug ( wxT("PRS1Loader::Open dir.IsOpened was true") );
    list<wxString> SerialNumbers;
    list<wxString>::iterator sn;

    wxString filename;
    bool cont=dir.GetFirst(&filename);

	if(!cont) wxLogDebug( wxT("PRS1Loader::Open - Failed to get first directory entry. '") + filename + wxT("'") );

    while (cont) {
        if ((filename[0]=='P') && (isdigit(filename[1])) && (isdigit(filename[2]))) {
            SerialNumbers.push_back(filename);
			wxLogDebug( wxT("Skipping filename ")+filename );
        } else if (filename.Lower()==wxT("last.txt")) { // last.txt points to the current serial number
            wxTextFile f(newpath+sep+filename);
			wxLogDebug( wxT("Opening filename ")+newpath+sep+filename );
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
        try {
            if (m) OpenMachine(m,newpath+wxFileName::GetPathSeparator()+(*sn),profile);
        } catch(OneTypePerDay e) {
            profile->DelMachine(m);
            PRS1List.erase(s);
            wxMessageBox(_("This Machine Record cannot be imported in this profile.\nThe Day records overlap with already existing content."),_("Import Error"),wxOK|wxCENTER);
            delete m;
        }
    }

    return PRS1List.size();

   // return c;
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

    for (map<wxString,wxString>::iterator i=prop.begin(); i!=prop.end(); i++) {
        m->properties[i->first]=i->second;
    }

    f.Close();
    return true;
}

int PRS1Loader::OpenMachine(Machine *m,wxString path,Profile *profile)
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
    for (list<wxString>::iterator p=paths.begin(); p!=paths.end(); p++) {
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
    for (map<SessionID,StringList>::iterator s=sessfiles.begin(); s!=sessfiles.end(); s++) {
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
        //wxLogMessage(sess->first().Format(wxT("%Y-%m-%d %H:%M:%S"))+wxT(" ")+sess->last().Format(wxT("%Y-%m-%d %H:%M:%S")));

        //sess->SetSessionID(sess->start().GetTicks());
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
        const double ignore_thresh=300.0/3600.0;// Ignore useless sessions under 5 minute
        if (sess->hours()<=ignore_thresh) {
            delete sess;
            continue;
        }
        m->AddSession(sess,profile);

        //if (sess->summary.find(CPAP_Obstructive)==sess->summary.end()) {
            sess->summary[CPAP_Obstructive]=(long)sess->count_events(CPAP_Obstructive);
            sess->summary[CPAP_Hypopnea]=(long)sess->count_events(CPAP_Hypopnea);
            sess->summary[CPAP_ClearAirway]=(long)sess->count_events(CPAP_ClearAirway);
            sess->summary[CPAP_RERA]=(long)sess->count_events(CPAP_RERA);
            sess->summary[CPAP_FlowLimit]=(long)sess->count_events(CPAP_FlowLimit);
        //}

        sess->summary[CPAP_CSR]=sess->sum_event_field(CPAP_CSR,0);
        sess->summary[CPAP_VSnore]=(long)sess->count_events(CPAP_VSnore);
        sess->summary[PRS1_VSnore2]=sess->sum_event_field(PRS1_VSnore2,0);


        if (sess->count_events(CPAP_IAP)>0) {
            sess->summary[CPAP_Mode]=(long)MODE_BIPAP;
            if (sess->summary[CPAP_PressureReliefType].GetInteger()!=(long)PR_NONE) {
                sess->summary[CPAP_PressureReliefType]=(long)PR_BIFLEX;
            }
            sess->summary[CPAP_PressureMedian]=(sess->avg_event_field(CPAP_EAP,0)+sess->avg_event_field(CPAP_IAP,0))/2.0;
            sess->summary[CPAP_PressureAverage]=(sess->weighted_avg_event_field(CPAP_IAP,0)+sess->weighted_avg_event_field(CPAP_EAP,0))/2.0;
            sess->summary[CPAP_PressureMinAchieved]=sess->min_event_field(CPAP_IAP,0);
            sess->summary[CPAP_PressureMaxAchieved]=sess->max_event_field(CPAP_EAP,0);

            sess->summary[BIPAP_IAPAverage]=sess->weighted_avg_event_field(CPAP_IAP,0);
            sess->summary[BIPAP_IAPMin]=sess->min_event_field(CPAP_IAP,0);
            sess->summary[BIPAP_IAPMax]=sess->max_event_field(CPAP_IAP,0);
            sess->summary[BIPAP_EAPAverage]=sess->weighted_avg_event_field(CPAP_EAP,0);
            sess->summary[BIPAP_EAPMin]=sess->min_event_field(CPAP_EAP,0);
            sess->summary[BIPAP_EAPMax]=sess->max_event_field(CPAP_EAP,0);
        } else {
            sess->summary[CPAP_PressureMedian]=sess->avg_event_field(CPAP_Pressure,0);
            //sess->summary[CPAP_PressureAverage]=sess->weighted_avg_event_field(CPAP_Pressure,0);
            sess->summary[CPAP_PressureMinAchieved]=sess->min_event_field(CPAP_Pressure,0);
            sess->summary[CPAP_PressureMaxAchieved]=sess->max_event_field(CPAP_Pressure,0);
            if (sess->summary.find(CPAP_PressureMin)==sess->summary.end()) {
                sess->summary[CPAP_BrokenSummary]=true;
                sess->set_last(sess->first());
                if (sess->summary[CPAP_PressureMinAchieved]==sess->summary[CPAP_PressureMaxAchieved]) {
                    sess->summary[CPAP_Mode]=(long)MODE_CPAP;
                } else {
                    sess->summary[CPAP_Mode]=(long)MODE_UNKNOWN;
                }
                sess->summary[CPAP_PressureReliefType]=(long)PR_UNKNOWN;
            }

        }
        if (sess->summary[CPAP_Mode]==(long)MODE_CPAP) {
            sess->summary[CPAP_PressureAverage]=sess->summary[CPAP_PressureMin];
            sess->summary[CPAP_PressureMax]=sess->summary[CPAP_PressureMin];
        }

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
    unsigned char ext,sum;

    //wxLogMessage(wxT("Opening PRS1 Summary ")+filename);
    wxFFile f(filename,wxT("rb"));

    if (!f.IsOpened())
        return false;

    int hl=16;

    br=f.Read(header,hl);

    if (header[0]!=header[5])
        return false;

    sequence=size=timestamp=seconds=ext=0;
    sequence=(header[10] << 24) | (header[9] << 16) | (header[8] << 8) | header[7];
    timestamp=(header[14] << 24) | (header[13] << 16) | (header[12] << 8) | header[11];
    size=(header[2] << 8) | header[1];
    ext=header[6];

    if (ext!=1)
        return false;

    //size|=(header[3]<<16) | (header[4]<<24); // the jury is still out on the 32bitness of one. doesn't matter here anyway.

    size-=(hl+2);
    sum=0;
    for (int i=0; i<hl-1; i++) sum+=header[i];
    if (sum!=header[hl-1])
        return false;

    if (size<=19) {
        wxLogWarning(wxT("Ignoring short session file ")+filename);
        return false;
    }

    wxDateTime date(timestamp);
    //wxDateTime tmpdate=date;
    //wxLogMessage(date.Format()+wxT(" UTC=")+tmpdate.Format());
    //int ticks=date.GetTicks();

    if (!date.IsValid()) return false;

    memset(m_buffer,0,size);
    unsigned char * buffer=m_buffer;
    br=f.Read(buffer,size);
    if (br<size) {
        return false;
    }
    if (size<0x30)
        return true;

    session->set_first(date);
    session->set_last(date);
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

    unsigned char * b=&buffer[0x14];
    wxUint16 bb=*(wxUint16*)b;
    int duration=bb;// | (buffer[0x15] << 8);
    session->summary[CPAP_Duration]=(long)duration;
    //wxLogMessage(wxString::Format(wxT("ID: %i %i"),session->session(),duration));
    float hours=float(duration)/3600.0;
    session->set_hours(hours);

    session->set_last(session->first()+wxTimeSpan::Seconds(duration));

    session->summary[CPAP_PressureMinAchieved]=buffer[0x16]/10.0;
    session->summary[CPAP_PressureMaxAchieved]=buffer[0x17]/10.0;
    session->summary[CPAP_PressureAverage]=buffer[0x18]/10.0;
    session->summary[CPAP_PressurePercentValue]=buffer[0x19]/10.0;
    session->summary[CPAP_PressurePercentName]=90.0;

    if (max==0) {
        session->summary[CPAP_PressureAverage]=session->summary[CPAP_PressureMin];
    }
    if (size==0x4d) {

        session->summary[CPAP_Obstructive]=(long)buffer[0x1C] | (buffer[0x1D] << 8);
        session->summary[CPAP_ClearAirway]=(long)buffer[0x20] | (buffer[0x21] << 8);
        session->summary[CPAP_Hypopnea]=(long)buffer[0x2A] | (buffer[0x2B] << 8);
        session->summary[CPAP_RERA]=(long)buffer[0x2E] | (buffer[0x2F] << 8);
        session->summary[CPAP_FlowLimit]=(long)buffer[0x30] | (buffer[0x31] << 8);
    }
    return true;
}

bool PRS1Loader::Parse002(Session *session,unsigned char *buffer,int size,time_t timestamp)
{
    MachineCode Codes[]={
        PRS1_Unknown00, PRS1_Unknown01, CPAP_Pressure, CPAP_EAP, PRS1_PressurePulse, CPAP_RERA, CPAP_Obstructive, CPAP_ClearAirway,
        PRS1_Unknown08, PRS1_Unknown09, CPAP_Hypopnea, PRS1_Unknown0B, CPAP_FlowLimit, CPAP_VSnore, PRS1_Unknown0E, CPAP_CSR, PRS1_Unknown10,
        CPAP_Leak, PRS1_Unknown12
    };
    int ncodes=sizeof(Codes)/sizeof(MachineCode);

    EventDataType data[4];

    wxDateTime start;
    start.Set(timestamp);
    wxDateTime t=start;
    //t.Set(timestamp);
    int pos=0;
    int cnt=0;
    while (pos<size) {
        char code=buffer[pos++];
        assert(code<ncodes);
        short size;
        if (code!=0x12) {
            size=buffer[pos] | buffer[pos+1] << 8;
            pos+=2;
            t+=wxTimeSpan::Seconds(size);
        }
    //    float data0,data1,data2;
        MachineCode cpapcode=Codes[(int)code];
        wxDateTime tt=t;
        cnt++;
        switch (code) {
        case 0x00: // Unknown
        case 0x01: // Unknown
        case 0x02: // Pressure
            data[0]=buffer[pos++]/10.0;
            session->AddEvent(new Event(t,cpapcode, data,1));
            break;
        case 0x04: // Pressure Pulse
            data[0]=buffer[pos++];
            session->AddEvent(new Event(t,cpapcode, data,1));
            break;

        case 0x05: // RERA
        case 0x06: // Obstructive Apoanea
        case 0x07: // Clear Airway
        case 0x0a: // Hypopnea
        case 0x0c: // Flow Limitation
            data[0]=buffer[pos];
            tt-=wxTimeSpan::Seconds((buffer[pos++])); // Subtract Time Offset
            session->AddEvent(new Event(tt,cpapcode,data,1));
            break;
        case 0x0d: // Vibratory Snore
            session->AddEvent(new Event(t,cpapcode, data,0));
            break;
        case 0x03: // BIPAP Pressure
        case 0x0b: // Unknown
        case 0x11: // Leak Rate
            data[0]=buffer[pos++];
            data[1]=buffer[pos++];
            if (code==0x11) {
                session->AddEvent(new Event(t,cpapcode, data,1));
                session->AddEvent(new Event(t,PRS1_VSnoreGraph,&data[1],1));
                if (data[1]>0) {
                    session->AddEvent(new Event(t,PRS1_VSnore2, &data[1],1));
                }
            } else if (code==0x03) {
                data[0]/=10.0;
                data[1]/=10.0;
                session->AddEvent(new Event(t,CPAP_EAP, &data[1], 1));
                session->AddEvent(new Event(t,CPAP_IAP, data, 1));
            } else {
                session->AddEvent(new Event(t,cpapcode, data, 2));
            }
            break;
        case 0x0e: // Unknown
        case 0x10: // Unknown
            data[0]=buffer[pos++]; // << 8) | buffer[pos];
            //pos+=2;
            data[1]=buffer[pos++];
            data[2]=buffer[pos++];
            //tt-=wxTimeSpan::Seconds(data[0]);
            session->AddEvent(new Event(t,cpapcode, data, 3));
            //wxLogMessage(tt.FormatTime()+wxString::Format(wxT(" %i: %.2f %.2f %.2f"),code,data[0],data[1],data[2]));
            break;
        case 0x0f: // Cheyne Stokes Respiration
            data[0]=buffer[pos+1]<<8 | buffer[pos];
            pos+=2;
            data[1]=buffer[pos++];
            tt-=wxTimeSpan::Seconds(data[1]);
            session->AddEvent(new Event(tt,cpapcode, data, 2));
            break;
        case 0x12: // Summary
            data[0]=buffer[pos++];
            data[1]=buffer[pos++];
            data[2]=buffer[pos+1]<<8 | buffer[pos];
            pos+=2;
            session->AddEvent(new Event(t,cpapcode, data,3));
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
    unsigned char header[24]; // use m_buffer?
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

    unsigned char *buffer=(unsigned char *)m_buffer;
    br=f.Read(buffer,size);
    if (br<size) {
        return false;
    }
    Parse002(session,buffer,size,timestamp);

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
    long samples=0;
    int duration=0;
    char * buffer=(char *)m_buffer;

    while (true) {
        br=f.Read(header,hl);

        if (br<hl) {
            if (cnt==0)
                return false;
            break;
        }

        if (header[0]!=header[5]) {
            if (cnt==0)
                return false;
            break;
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
            break;
        }

        unsigned char sum=0;
        for (int i=0; i<hl-1; i++) sum+=header[i];
        if (sum!=header[hl-1])
            return false;

        if (samples+size>=max_load_buffer_size) {
            wxLogError(wxT("max_load_buffer_size is too small in PRS1 Loader"));
            if (cnt==0) return false;
            break;
        }
        br=f.Read((char *)&buffer[samples],size);
        if (br<size) {
            delete [] buffer;
            break;
        }

        //wavedata.push_back(buffer);
        //wavesize.push_back(size);
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

    // Convert to SampleFormat
    SampleFormat *data=new SampleFormat [samples];

    SampleFormat min=0,max=0;
    bool first=true;

    SampleFormat c;

    for (long i=0;i<samples;i++) {
        data[i]=buffer[i];
        c=data[i];
        if (first) {
            min=max=c;
            first=false;
        }
        if (min>c) min=c;
        if (max<c) max=c;
    }
    //wxLogMessage(wxT("Samples Per Breath: ")+wxString::Format(wxT("%.2f"),double(breath_total)/double(breaths)));



    Waveform *w=new Waveform(start,CPAP_FlowRate,data,samples,duration,min,max);
    //wxLogMessage(wxString::Format(wxT("%i %i %i %i %i"),start,samples,duration,min,max));
    session->AddWaveform(w);

    //wxLogMessage(wxT("Done PRS1 Waveforms ")+filename);
    return true;
}

void InitModelMap()
{
    ModelMap[34]=wxT("RemStar Pro with C-Flex+");
    ModelMap[35]=wxT("RemStar Auto with A-Flex");
    ModelMap[37]=wxT("RemStar BIPAP Auto with Bi-Flex");
};


bool initialized=false;
void PRS1Loader::Register()
{
    if (initialized) return;
    wxLogVerbose(wxT("Registering PRS1Loader"));
    RegisterLoader(new PRS1Loader());
    InitModelMap();
    initialized=true;
}

