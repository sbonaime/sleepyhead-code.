/*
SleepLib CMS50X Loader Implementation

Author: Mark Watkins <jedimark64@users.sourceforge.net>
License: GPL
*/

#include <wx/log.h>
#include <wx/progdlg.h>
#include "cms50_loader.h"
#include "sleeplib/machine.h"

extern wxProgressDialog *loader_progress;

CMS50Loader::CMS50Loader()
{
    //ctor
}

CMS50Loader::~CMS50Loader()
{
    //dtor
}
bool CMS50Loader::Open(wxString & path,Profile *profile)
{
    // CMS50 folder structure detection stuff here.

    // Not sure whether to both supporting SpO2 files here, as the timestamps are utterly useless for overlay purposes.
    // May just ignore the crap and support my CMS50 logger

    // Contains three files
    // Data Folder
    // SpO2 Review.ini
    // SpO2.ini

    wxString tmp=path+wxFileName::GetPathSeparator()+wxT("Data");
    if ((wxFileExists(path+wxFileName::GetPathSeparator()+wxT("SpO2 Review.ini")))
        && (wxFileExists(path+wxFileName::GetPathSeparator()+wxT("SpO2.ini")))
        && (wxDirExists(tmp))) {
             // Their software

            return OpenCMS50(tmp,profile);
    } else {
        // My Logger
    }

    return false;
}
bool CMS50Loader::OpenCMS50(wxString & path, Profile *profile)
{
    wxDir dir;
    if (!dir.Open(path)) return false;
    list<wxString> files;

    wxString filename,pathname;

    bool cont=dir.GetFirst(&filename);

    while (cont) {
        if (filename.EndsWith(wxT(".spoR"))) {
            pathname=path+wxFileName::GetPathSeparator()+filename;
            files.push_back(pathname);
        }
        cont=dir.GetNext(&filename);
    }
    int size=files.size();
    if (size==0) return false;

    Machine *mach=CreateMachine(profile);
    int cnt=0;
    for (list<wxString>::iterator n=files.begin();n!=files.end();n++,++cnt) {
        if (loader_progress) loader_progress->Update((float(cnt)/float(size)*100.0));
        OpenSPORFile((*n),mach,profile);
    }
    mach->Save();
    return true;
}
bool CMS50Loader::OpenSPORFile(wxString path,Machine *mach,Profile *profile)
{
    wxFFile f;
    unsigned char tmp[256];
    if (!f.Open(path,wxT("rb"))) return false;

    wxString str=path.AfterLast(wxChar('_'));
    wxString id=str.BeforeFirst(wxChar('.'));
    wxDateTime dt;
    dt.ParseFormat(id,wxT("%Y%m%d%H%M"));

    SessionID sessid=dt.GetTicks(); // Import date becomes session id

    if (mach->SessionExists(sessid))
        return false; // Already imported

    // Find everything after the last _
    short data_starts;
    short some_code;
    int num_records;
    short some_more_code;
    int br;



    f.Read(tmp,2);
    data_starts=tmp[0] | (tmp[1] << 8);
    f.Read(tmp,2);
    some_code=tmp[0] | (tmp[1] << 8); // == 512
    f.Read(tmp,2);
    num_records=tmp[0] | (tmp[1] << 8);
    num_records <<= 1;

    f.Read(tmp,2);
    some_more_code=tmp[0] | (tmp[1] << 8); // == 512

    f.Read(tmp,16);

    wxString datestr;
    for (int i=0;i<16;i+=2) {
        datestr=datestr+tmp[i];
    }
    wxDateTime date;
    date.ParseFormat(datestr,wxT("%m/%d/%y"));


    f.Seek(data_starts,wxFromStart);
    buffer=new char [num_records*2];
    br=f.Read(buffer,num_records);
    if (br!=num_records) {
        wxLogDebug(wxT("Short .spoR File: ")+path);
    }
    char last_pulse=buffer[0];
    char last_spo2=buffer[1];
    EventDataType data[2];
    wxDateTime last_pulse_time=date;
    wxDateTime last_spo2_time=date;
    data[0]=last_pulse;
    data[1]=last_spo2;

    Session *sess=new Session(mach,sessid);
    sess->set_first(date);
    sess->AddEvent(new Event(date,OXI_Pulse,data,1));
    sess->AddEvent(new Event(date,OXI_SPO2,&data[1],1));
    //wxDateTime
    wxDateTime tt=date;
    for (int i=2;i<num_records;i+=2) {
        if (last_pulse!=buffer[i]) {
            data[0]=buffer[i];
            sess->AddEvent(new Event(tt,OXI_Pulse,data,1));
        }
        if (last_spo2!=buffer[i+1]) {
            data[1]=buffer[i+1];
            sess->AddEvent(new Event(tt,OXI_SPO2,&data[1],1));
        }
        last_pulse=buffer[i];
        last_spo2=buffer[i+1];
        tt+=wxTimeSpan::Seconds(1);
    }
    sess->AddEvent(new Event(tt,OXI_Pulse,data,1));
    sess->AddEvent(new Event(tt,OXI_SPO2,&data[1],1));

    sess->set_last(tt);
    wxTimeSpan t=sess->last()-sess->first();

    double hours=(t.GetSeconds().GetLo()/3600.0);
    sess->set_hours(hours);

    sess->summary[OXI_PulseAverage]=sess->weighted_avg_event_field(OXI_Pulse,0);
    sess->summary[OXI_PulseMin]=sess->min_event_field(OXI_Pulse,0);
    sess->summary[OXI_PulseMax]=sess->max_event_field(OXI_Pulse,0);
    sess->summary[OXI_SPO2Average]=sess->weighted_avg_event_field(OXI_SPO2,0);
    sess->summary[OXI_SPO2Min]=sess->min_event_field(OXI_SPO2,0);
    sess->summary[OXI_SPO2Max]=sess->max_event_field(OXI_SPO2,0);

    mach->AddSession(sess,profile);
    sess->SetChanged(true);
    delete buffer;

    return true;
}
Machine *CMS50Loader::CreateMachine(Profile *profile)
{
    if (!profile) {  // shouldn't happen..
        wxLogError(wxT("No Profile!"));
        return NULL;
    }

    // NOTE: This only allows for one CMS50 machine per profile..
    // Upgrading their oximeter will use this same record..

    vector<Machine *> ml=profile->GetMachines(MT_OXIMETER);

    for (vector<Machine *>::iterator i=ml.begin(); i!=ml.end(); i++) {
        if ((*i)->GetClass()==wxT("CMS50"))  {
            return (*i);
            break;
        }
    }

    wxLogDebug(wxT("Create CMS50 Machine Record"));

    Machine *m=new Oximeter(profile,0);
    m->SetClass(wxT("CMS50"));
    m->properties[wxT("Brand")]=wxT("Contec");
    m->properties[wxT("Model")]=wxT("CMS50X");

    profile->AddMachine(m);

    return m;
}



static bool cms50_initialized=false;

void CMS50Loader::Register()
{
    if (cms50_initialized) return;
    wxLogVerbose(wxT("Registering CMS50Loader"));
    RegisterLoader(new CMS50Loader());
    //InitModelMap();
    cms50_initialized=true;
}

