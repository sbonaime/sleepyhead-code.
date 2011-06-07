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
    assert(profile!=NULL);

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
    wxString filename,pathname;
    list<wxString> files;
    wxDir dir;

    if (!dir.Open(path)) return false;

    if(loader_progress) {
        loader_progress->Update(0);
    }

    bool cont=dir.GetFirst(&filename);

    while (cont) {
        if (filename.Lower().EndsWith(wxT(".spor"))) {
            pathname=path+wxFileName::GetPathSeparator()+filename;
            files.push_back(pathname);
        }
        cont=dir.GetNext(&filename);
        if (loader_progress) loader_progress->Pulse();

    }
    int size=files.size();
    if (size==0) return false;

    Machine *mach=CreateMachine(profile);
    int cnt=0;
    for (list<wxString>::iterator n=files.begin();n!=files.end();n++,++cnt) {
        if (loader_progress) loader_progress->Update((float(cnt)/float(size)*50.0));
        OpenSPORFile((*n),mach,profile);
    }
    mach->Save();
    if (loader_progress) loader_progress->Update(100);
    return true;
}
bool CMS50Loader::OpenSPORFile(wxString path,Machine *mach,Profile *profile)
{
    assert(mach!=NULL);
    assert(profile!=NULL);

    wxFFile f;
    unsigned char tmp[256];

    wxInt16 data_starts;
    wxInt16 some_code;
    wxInt16 some_more_code;
    int num_records;
    int br;

    if (!f.Open(path,wxT("rb")))
        return false;

    // Find everything after the last _

    wxString str=path.AfterLast(wxChar('_'));
    wxString id=str.BeforeFirst(wxChar('.'));
    wxDateTime dt;
    dt.ParseFormat(id,wxT("%Y%m%d%H%M"));

    SessionID sessid=dt.GetTicks(); // Import date becomes session id

    if (mach->SessionExists(sessid))
        return false; // Already imported


    br=f.Read(tmp,2);
    if (br!=2) return false;
    data_starts=tmp[0] | (tmp[1] << 8);

    br=f.Read(tmp,2);
    if (br!=2) return false;
    some_code=tmp[0] | (tmp[1] << 8); // == 512

    br=f.Read(tmp,2);
    if (br!=2) return false;
    num_records=tmp[0] | (tmp[1] << 8);
    if (num_records<300) return false; // dont bother.

    num_records <<= 1;

    br=f.Read(tmp,2);
    if (br!=2) return false;
    some_more_code=tmp[0] | (tmp[1] << 8);  // == 0

    br=f.Read(tmp,34);
    if (br!=34) return false;

    // Widechar date string in format 'DD/MM/YY HH:MM:SS'
    for (int i=0;i<17;i++) {
        tmp[i]=tmp[i << 1];
    }
    tmp[17]=0;
    wxString datestr((char *)tmp,wxConvUTF8);

    wxDateTime date;
    date.ParseFormat(datestr,wxT("%m/%d/%y %H:%M:%S"));
    //wxLogMessage(datestr);

    f.Seek(data_starts,wxFromStart);

    buffer=new char [num_records];
    br=f.Read(buffer,num_records);
    if (br!=num_records) {
        wxLogError(wxT("Short .spoR File: ")+path);
        delete [] buffer;
        return false;
    }

    wxDateTime last_pulse_time=date;
    wxDateTime last_spo2_time=date;

    EventDataType last_pulse=buffer[0];
    EventDataType last_spo2=buffer[1];
    EventDataType cp,cs;

    Session *sess=new Session(mach,sessid);
    sess->set_first(date);
    sess->AddEvent(new Event(date,OXI_Pulse,&last_pulse,1));
    sess->AddEvent(new Event(date,OXI_SPO2,&last_spo2,1));

    EventDataType PMin=0,PMax=0,SMin=0,SMax=0,PAvg=0,SAvg=0;
    int PCnt=0,SCnt=0;
    //wxDateTime
    wxDateTime tt=date;
    wxDateTime lasttime=date;
    bool first_p=true,first_s=true;

    for (int i=2;i<num_records;i+=2) {
        cp=buffer[i];
        cs=buffer[i+1];
        if (last_pulse!=cp) {
            sess->AddEvent(new Event(tt,OXI_Pulse,&cp,1));
            if (tt>lasttime) lasttime=tt;
            if (cp>0) {
                if (first_p) {
                    PMin=cp;
                    first_p=false;
                } else {
                    if (PMin>cp) PMin=cp;
                }
                PAvg+=cp;
                PCnt++;
            }
        }
        if (last_spo2!=cs) {
            sess->AddEvent(new Event(tt,OXI_SPO2,&cs,1));
            if (tt>lasttime) lasttime=tt;
            if (cs>0) {
                if (first_s) {
                    SMin=cs;
                    first_s=false;
                } else {
                    if (SMin>cs) SMin=cs;
                }
                SAvg+=cs;
                SCnt++;
            }
        }
        last_pulse=cp;
        last_spo2=cs;
        if (PMax<cp) PMax=cp;
        if (SMax<cs) SMax=cs;
        tt+=wxTimeSpan::Seconds(1);
    }
    if (cp) sess->AddEvent(new Event(tt,OXI_Pulse,&cp,1));
    if (cs) sess->AddEvent(new Event(tt,OXI_SPO2,&cs,1));

    sess->set_last(lasttime);
    wxTimeSpan t=sess->last()-sess->first();

    double hours=(t.GetSeconds().GetLo()/3600.0);
    sess->set_hours(hours);

    EventDataType pa=0,sa=0;
    if (PCnt>0) pa=PAvg/double(PCnt);
    if (SCnt>0) sa=SAvg/double(SCnt);
    sess->summary[OXI_PulseAverage]=pa;
    sess->summary[OXI_PulseMin]=PMin;
    sess->summary[OXI_PulseMax]=PMax;
    sess->summary[OXI_SPO2Average]=sa;
    sess->summary[OXI_SPO2Min]=SMin;
    sess->summary[OXI_SPO2Max]=SMax;

    mach->AddSession(sess,profile);
    sess->SetChanged(true);
    delete [] buffer;

    return true;
}
Machine *CMS50Loader::CreateMachine(Profile *profile)
{
    assert(profile!=NULL);

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

