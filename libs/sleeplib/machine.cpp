#include <tr1/random>
#include <wx/colour.h>
#include <wx/log.h>
#include <wx/progdlg.h>
#include "binary_file.h"
#include "machine.h"
#include "profiles.h"
#include <algorithm>


extern wxProgressDialog *loader_progress;

map<MachineID,Machine *> MachList;

map<MachineType,wxString> MachineTypeString= {
    {MT_UNKNOWN, 	wxT("Unknown")},
    {MT_CPAP,	 	wxT("CPAP")},
    {MT_OXIMETER,	wxT("Oximeter")},
    {MT_SLEEPSTAGE,	wxT("SleepStage")}
};

map<wxString,MachineType> MachineTypeLookup= {
    { MachineTypeString[MT_UNKNOWN].Lower(),	MT_UNKNOWN },
    { MachineTypeString[MT_CPAP].Lower(), 		MT_CPAP },
    { MachineTypeString[MT_OXIMETER].Lower(), 	MT_OXIMETER },
    { MachineTypeString[MT_SLEEPSTAGE].Lower(), MT_SLEEPSTAGE}
};

// This is technically gui related.. however I have something in mind for it.
const map<MachineCode,FlagType> DefaultFlagTypes= {
    {CPAP_Obstructive,	FT_BAR},
    {CPAP_Hypopnea,		FT_BAR},
    {CPAP_RERA,			FT_BAR},
    {CPAP_VSnore,		FT_BAR},
    {PRS1_VSnore2,		FT_BAR},
    {CPAP_FlowLimit,	FT_BAR},
    {CPAP_ClearAirway,	FT_BAR},
    {CPAP_CSR,			FT_SPAN},
    {PRS1_PressurePulse,FT_DOT},
    {CPAP_Pressure,		FT_DOT}
};

const unsigned char flagalpha=0x80;
const map<MachineCode,wxColour> DefaultFlagColours= {
    {CPAP_Obstructive,	wxColour(0x80,0x80,0xff,flagalpha)},
    {CPAP_Hypopnea,		wxColour(0x00,0x00,0xff,flagalpha)},
    {CPAP_RERA,			wxColour(0x40,0x80,0xff,flagalpha)},
    {CPAP_VSnore,		wxColour(0xff,0x20,0x20,flagalpha)},
    {CPAP_FlowLimit,	wxColour(0x20,0x20,0x20,flagalpha)},
    {CPAP_ClearAirway,	wxColour(0xff,0x40,0xff,flagalpha)},
    {CPAP_CSR,			wxColour(0x40,0xff,0x40,flagalpha)},
    {PRS1_VSnore2,		wxColour(0xff,0x20,0x20,flagalpha)},
    {PRS1_PressurePulse,wxColour(0xff,0x40,0xff,flagalpha)}
};
map<CPAPMode,wxString> CPAPModeNames={
    {MODE_CPAP,wxT("CPAP")},
    {MODE_APAP,wxT("APAP")},
    {MODE_BIPAP,wxT("BIPAP")},
    {MODE_ASV,wxT("ASV")}
};
map<PRTypes,wxString> PressureReliefNames={
    {PR_NONE,_("None")},
    {PR_CFLEX,wxT("C-Flex")},
    {PR_CFLEXPLUS,wxT("C-Flex+")},
    {PR_AFLEX,wxT("A-Flex")},
    {PR_BIFLEX,wxT("Bi-Flex")},
    {PR_EPR,wxT("Exhalation Pressure Relief (EPR)")},
    {PR_SMARTFLEX,wxT("SmartFlex")}
};

// Master list. Look up local name table first.. then these if not found.
map<MachineCode,wxString> DefaultMCShortNames= {
    {CPAP_Obstructive,	wxT("OA")},
    {CPAP_Hypopnea,		wxT("H")},
    {CPAP_RERA,			wxT("RE")},
    {CPAP_ClearAirway,	wxT("CA")},
    {CPAP_CSR,			wxT("CSR")},
    {CPAP_VSnore,		wxT("VS")},
    {CPAP_FlowLimit,	wxT("FL")},
    {CPAP_Pressure,		wxT("P")},
    {CPAP_Leak,			wxT("LR")},
    {CPAP_EAP,			wxT("EAP")},
    {CPAP_IAP,			wxT("IAP")},
    {PRS1_VSnore2,		wxT("VS")},
    {PRS1_PressurePulse,wxT("PP")}
};

// Master list. Look up local name table first.. then these if not found.
map<MachineCode,wxString> DefaultMCLongNames= {
    {CPAP_Obstructive,	wxT("Obstructive Apnea")},
    {CPAP_Hypopnea,		wxT("Hypopnea")},
    {CPAP_RERA,			wxT("Respiratory Effort / Arrousal")},
    {CPAP_ClearAirway,	wxT("Clear Airway Apnea")},
    {CPAP_CSR,			wxT("Cheyne Stokes Respiration")},
    {CPAP_VSnore,		wxT("Vibratory Snore")},
    {CPAP_FlowLimit,	wxT("Flow Limitation")},
    {CPAP_Pressure,		wxT("Pressure")},
    {CPAP_Leak,			wxT("Leak Rate")},
    {CPAP_EAP,			wxT("BIPAP Exhalation Pressure")},
    {CPAP_IAP,			wxT("BIPAP Inhalation Pressure")},
    {PRS1_VSnore2,		wxT("Vibratory Snore")},
    {PRS1_PressurePulse,wxT("Pressue Pulse")}
};

//////////////////////////////////////////////////////////////////////////////////////////
// Machine Base-Class implmementation
//////////////////////////////////////////////////////////////////////////////////////////
Machine::Machine(Profile *p,MachineID id)
{
    profile=p;
    if (!id) {
        std::tr1::minstd_rand gen;
        std::tr1::uniform_int<MachineID> unif(1, 0x7fffffff);
        gen.seed((unsigned int) time(NULL));
        MachineID temp;
        do {
            temp = unif(gen); //unif(gen) << 32 |
        } while (profile->machlist.find(temp)!=profile->machlist.end());

        m_id=temp;

    } else m_id=id;
    m_type=MT_UNKNOWN;
    firstsession=true;
}
Machine::~Machine()
{
    for (auto d=day.begin();d!=day.end();d++) {
        delete d->second;
    }
}
Session *Machine::SessionExists(SessionID session)
{
    if (sessionlist.find(session)!=sessionlist.end()) {
        return sessionlist[session];
    } else {
        return NULL;
    }
}
void Machine::AddSession(Session *s)
{
    wxDateTime date=s->first();
    date.ResetTime();
    date-=wxTimeSpan::Day();

    bool ok=false;
    if (day.find(date)!=day.end()) {
        wxTimeSpan span=s->first()-day[date]->last();
        if (span<wxTimeSpan::Hours(4)) ok=true;
        else {

            wxDateTime t=s->first();
            t.ResetTime();
            wxTimeSpan j=s->first()-t;
            if (j<wxTimeSpan::Hours(6)) {
                ok=true;
            }
        }
    } else {
        wxDateTime t=s->first();
        t.ResetTime();
        wxTimeSpan j=s->first()-t;
        if (j<wxTimeSpan::Hours(6)) {
            ok=true;
        }
    }

    if (!ok) {
        date+=wxTimeSpan::Day();
    }


    sessionlist[s->session()]=s;

/*    wxDateTime s1=s->first();
    wxDateTime s2=s->last();
    if (s1==s2) return; // leave this session out of daylist.. it's dodgy. DELETE?


    wxDateTime date,d1=s1;
    date=d1;
    d1.SetHour(15);

    if (s1<d1) {
        date-=wxTimeSpan::Hours(24);
    } */

    if (!firstsession) {
        if (firstday>date) firstday=date;
        if (lastday<date) lastday=date;
    } else {
        firstday=lastday=date;
        firstsession=false;
    }
    if (day.find(date)==day.end()) {
        day[date]=new Day();
        //wxLogMessage(wxT("Creating Day ")+date.Format()+wxT(" to store ")+s1.Format());
    }
        //wxLogMessage(wxT("Using Day ")+date.Format()+wxT(" to store ")+s1.Format()+wxT(" ")+s2.Format());

    day[date]->AddSession(s);
}

bool Machine::Load()
{
    wxString path=profile->Get("DataFolder")+wxFileName::GetPathSeparator()+hexid();
    wxDir dir;
    wxLogMessage(wxT("Loading ")+path);
    dir.Open(path);
    if (!dir.IsOpened()) return false;

    wxString filename;
    bool cont=dir.GetFirst(&filename);

    typedef vector<wxString> StringList;
    map<SessionID,StringList> sessfiles;
    while (cont) {
        wxString ext_s=filename.AfterLast(wxChar('.'));
        wxString session_s=wxT("0x")+filename.BeforeLast(wxChar('.'));

        SessionID sessid;
        long ext;
        session_s.ToLong(&sessid,16);
        ext_s.ToLong(&ext);
        if (sessfiles[sessid].capacity()==0) sessfiles[sessid].resize(3);
        wxString fullname=path+wxFileName::GetPathSeparator()+filename;
        if (ext==0) sessfiles[sessid][0]=fullname;
        else if (ext==1) sessfiles[sessid][1]=fullname;
        else if (ext==2) sessfiles[sessid][2]=fullname;

        cont=dir.GetNext(&filename);
    }
    for (auto s=sessfiles.begin(); s!=sessfiles.end(); s++) {
        Session *sess=new Session(this,s->first);
        if (sess->LoadSummary(s->second[0])) {
            //sess->SetEventFile(sessfiles[sess->id()][1]);
            //sess->SetWaveformFile(sessfiles[sess->id()][1]);
            //wxString sx=sess->first().Format()+wxT(" ")+sess->last().Format();
            //	wxPrintf(s+wxT(" O=%i H=%i CA=%i \n"),sess->summary[CPAP_Obstructive].GetLong(),sess->summary[CPAP_Hypopnea].GetLong(),sess->summary[CPAP_ClearAirway].GetLong());
            sess->LoadEvents(s->second[1]);
            sess->LoadWaveforms(s->second[2]);
            AddSession(sess);
        } else {
            delete sess;
        }
    }
    return true;
}
bool Machine::Save()
{

    wxString path=profile->Get("DataFolder")+wxFileName::GetPathSeparator()+hexid();
    int size=sessionlist.size();
    int cnt=0;
    for (auto s=sessionlist.begin(); s!=sessionlist.end(); s++) {
        cnt++;
        if (loader_progress) loader_progress->Update(50+(float(cnt)/float(size)*50.0));
        if (s->second->IsChanged()) s->second->Store(path);
    }
    return true;
}
//////////////////////////////////////////////////////////////////////////////////////////
// Day Class implmementation
//////////////////////////////////////////////////////////////////////////////////////////
Day::Day()
{
    d_firstsession=true;
    sessions.clear();
}
Day::~Day()
{

}
void Day::AddSession(Session *s)
{
    if (!s) {
        wxLogWarning(wxT("Day::AddSession called with NULL session object"));
        return;
    }
    if (d_firstsession) {
        d_firstsession=false;
        d_first=s->first();
        d_last=s->last();
    } else {
        if (d_first > s->first()) d_first = s->first();
        if (d_last < s->last()) d_last = s->last();
    }
    sessions.push_back(s);
}
EventDataType Day::summary_max(MachineCode code)
{
    EventDataType val=0,tmp;
    bool fir=true;
    // Cache this?

    for (auto s=sessions.begin();s!=sessions.end();s++) {
        Session & sess=*(*s);
        if (sess.summary.find(code)!=sess.summary.end()) {
            tmp=sess.summary[code].GetDouble();
            if (tmp>val) val=tmp;
        }
    }
    return val;
}
EventDataType Day::summary_min(MachineCode code)
{
    EventDataType val=0,tmp;
    bool fir=true;
    // Cache this?

    for (auto s=sessions.begin();s!=sessions.end();s++) {
        Session & sess=*(*s);
        if (sess.summary.find(code)!=sess.summary.end()) {
            tmp=sess.summary[code].GetDouble();
            if (fir) {
                val=tmp;
                fir=false;
            } else {
                if (val<tmp) val=tmp;
            }
        }
    }
    return val;
}

EventDataType Day::summary_avg(MachineCode code)
{
    EventDataType val=0,tmp=0;
    bool fir=true;
    // Cache this?

    int cnt=0;
    for (auto s=sessions.begin();s!=sessions.end();s++) {
        Session & sess=*(*s);
        if (sess.summary.find(code)!=sess.summary.end()) {
            tmp+=sess.summary[code].GetDouble();
            cnt++;
        }
    }
    val=tmp/EventDataType(cnt);
    return val;
}

EventDataType Day::min(MachineCode code,int field)
{
    EventDataType val=0,tmp;
    bool fir=true;
    // Cache this?

    for (auto s=sessions.begin();s!=sessions.end();s++) {
        Session & sess=*(*s);
        if (sess.events.find(code)!=sess.events.end()) {
            tmp=sess.min_event_field(code,field);
            if (fir) {
                val=tmp;
                fir=false;
            } else {
                if (val>tmp) val=tmp;
            }
        }
    }
    return val;
}

EventDataType Day::max(MachineCode code,int field)
{
    EventDataType val=0,tmp;
    bool fir=true;
    // Cache this?

    // Don't assume sessions are in order.
    for (auto s=sessions.begin();s!=sessions.end();s++) {
        Session & sess=*(*s);
        if (sess.events.find(code)!=sess.events.end()) {
            tmp=sess.max_event_field(code,field);
            if (fir) {
                val=tmp;
                fir=false;
            } else {
                if (val<tmp) val=tmp;
            }
        }
    }
    return val;
}

EventDataType Day::avg(MachineCode code,int field)
{
    double val=0;
    // Cache this?
    int cnt=0;
    // Don't assume sessions are in order.
    for (auto s=sessions.begin();s!=sessions.end();s++) {
        Session & sess=*(*s);
        if (sess.events.find(code)!=sess.events.end()) {
            val+=sess.avg_event_field(code,field);
            cnt++;
        }
    }
    if (cnt==0) cnt=1;
    return EventDataType(val/cnt);
}

EventDataType Day::sum(MachineCode code,int field)
{
    // Cache this?
    EventDataType val=0;
    for (auto s=sessions.begin();s!=sessions.end();s++) {
        Session & sess=*(*s);
        if (sess.events.find(code)!=sess.events.end()) {
            val+=sess.sum_event_field(code,field);
        }
    }
    return val;
}

EventDataType Day::count(MachineCode code)
{
    EventDataType val=0;
    // Cache this?
    // Don't assume sessions are in order.
    for (auto s=sessions.begin();s!=sessions.end();s++) {
        Session & sess=*(*s);
        if (sess.events.find(code)!=sess.events.end()) {
            val+=sess.count_events(code);
        }
    }
    return val;
}
EventDataType Day::weighted_avg(MachineCode code,int field)
{
    double val=0;
    // Cache this?
    int cnt=0;
    // Don't assume sessions are in order.
    for (auto s=sessions.begin();s!=sessions.end();s++) {
        Session & sess=*(*s);
        if (sess.events.find(code)!=sess.events.end()) {
            val+=sess.weighted_avg_event_field(code,field);
            cnt++;
        }
    }
    if (cnt==0) cnt=1;
    return EventDataType(val/cnt);
}
wxTimeSpan Day::total_time()
{
    //if (d_totaltime>wxTimeSpan::Seconds(0)) return d_totaltime;

    d_totaltime=wxTimeSpan::Seconds(0);
    for (auto s=sessions.begin();s!=sessions.end();s++) {
        Session & sess=*(*s);
        d_totaltime+=sess.last()-sess.first();
        if (d_totaltime>wxTimeSpan::Hours(15)) {
            int c=1;
        }
    }
    return d_totaltime;
}
EventDataType Day::percentile(MachineCode code,int field,double percent)
{
    double val=0;
    // Cache this?
    int cnt=0;
    // Don't assume sessions are in order.
    for (auto s=sessions.begin();s!=sessions.end();s++) {
        Session & sess=*(*s);
        if (sess.events.find(code)!=sess.events.end()) {
            val+=sess.percentile(code,field,percent);
            cnt++;
        }
    }
    if (cnt==0) return 0;
    return EventDataType(val/cnt);

}

const wxDateTime & Day::first(MachineCode code)
{
    static wxDateTime date;
    wxDateTime tmp;
    bool fir=true;
    // Cache this?

    // Don't assume sessions are in order.
    for (auto s=sessions.begin();s!=sessions.end();s++) {
        Session & sess=*(*s);
        if (sess.events.find(code)!=sess.events.end()) {
            tmp=sess.events[code][0]->time();
            if (fir) {
                date=tmp;
                fir=false;
            } else {
                if (tmp<date) date=tmp;
            }
        }
    }
    return date;
}

const wxDateTime & Day::last(MachineCode code)
{
    size_t cnt;
    static wxDateTime date;
    wxDateTime tmp;
    bool fir=true;
    // Cache this?

    // Don't assume sessions are in order.
    for (auto s=sessions.begin();s!=sessions.end();s++) {
        Session & sess=*(*s);
        if (sess.events.find(code)!=sess.events.end()) {
            auto i=sess.events[code].rbegin();
            assert(i!=sess.events[code].rend());
            tmp=(*i)->time();
            if (fir) {
                date=tmp;
                fir=false;
            } else {
                if (tmp>date) date=tmp;
            }
        }
    }
    return date;
}



//////////////////////////////////////////////////////////////////////////////////////////
// Event implmementation
//////////////////////////////////////////////////////////////////////////////////////////
Event::Event(wxDateTime time,MachineCode code,list<EventDataType> data)
    :e_time(time),e_code(code)
{
    e_fields=0;
    for (auto i=data.begin(); i!=data.end(); i++) {
        e_data.push_back(*i);
        e_fields++;
    }

}
Event::~Event()
{

};


//////////////////////////////////////////////////////////////////////////////////////////
// Waveform implmementation
//////////////////////////////////////////////////////////////////////////////////////////
Waveform::Waveform(wxDateTime time,MachineCode code, SampleFormat *data,int samples,float duration,SampleFormat min, SampleFormat max)
    :w_time(time),w_code(code),w_data(data),w_samples(samples),w_duration(duration)
{
    w_totalspan=wxTimeSpan::Seconds(duration);
    double rate=duration/samples;
    w_samplespan=wxTimeSpan::Milliseconds(rate*1000.0);
    Min=min;
    Max=max;
}
Waveform::~Waveform()
{
    delete [] w_data;
}
//////////////////////////////////////////////////////////////////////////////////////////
// CPAP implmementation
//////////////////////////////////////////////////////////////////////////////////////////
CPAP::CPAP(Profile *p,MachineID id):Machine(p,id)
{
    m_type=MT_CPAP;

    FlagColours=DefaultFlagColours;
}

CPAP::~CPAP()
{
}

//////////////////////////////////////////////////////////////////////////////////////////
// Oximeter Class implmementation
//////////////////////////////////////////////////////////////////////////////////////////
Oximeter::Oximeter(Profile *p,MachineID id):Machine(p,id)
{
    m_type=MT_OXIMETER;
}

Oximeter::~Oximeter()
{
}

//////////////////////////////////////////////////////////////////////////////////////////
// SleepStage Class implmementation
//////////////////////////////////////////////////////////////////////////////////////////
SleepStage::SleepStage(Profile *p,MachineID id):Machine(p,id)
{
    m_type=MT_SLEEPSTAGE;
}
SleepStage::~SleepStage()
{
}

//////////////////////////////////////////////////////////////////////////////////////////
// Session implmementation
// This stuff contains the base calculation smarts
//////////////////////////////////////////////////////////////////////////////////////////
Session::Session(Machine * m,SessionID session)
{
    s_machine=m;
    s_session=session;
    s_changed=false;
}
Session::~Session()
{
    TrashEvents();
    TrashWaveforms();
}
double Session::min_event_field(MachineCode mc,int field)
{
    if (events.find(mc)==events.end()) return 0;

    bool first=true;
    double min;
    for (auto i=events[mc].begin(); i!=events[mc].end(); i++) {
        if (field>(*i)->e_fields) throw BoundsError();
        if (first) {
            first=false;
            min=(*(*i))[field];
        } else {
            if (min>(*(*i))[field]) min=(*(*i))[field];
        }
    }
    return min;
}
double Session::max_event_field(MachineCode mc,int field)
{
    if (events.find(mc)==events.end()) return 0;

    bool first=true;
    double max;
    for (auto i=events[mc].begin(); i!=events[mc].end(); i++) {
        if (field>(*i)->e_fields) throw BoundsError();
        if (first) {
            first=false;
            max=(*(*i))[field];
        } else {
            if (max<(*(*i))[field]) max=(*(*i))[field];
        }
    }
    return max;
}

double Session::sum_event_field(MachineCode mc,int field)
{
    if (events.find(mc)==events.end()) return 0;

    double sum=0;
    for (auto i=events[mc].begin(); i!=events[mc].end(); i++) {
        if (field>(*i)->e_fields) throw BoundsError();
        sum+=(*(*i))[field];
    }
    return sum;
}
double Session::avg_event_field(MachineCode mc,int field)
{
    if (events.find(mc)==events.end()) return 0;

    double sum=0;
    int cnt=0;
    for (auto i=events[mc].begin(); i!=events[mc].end(); i++) {
        if (field>(*i)->e_fields) throw BoundsError();
        sum+=(*(*i))[field];
        cnt++;
    }

    return sum/cnt;
}

bool sortfunction (double i,double j) { return (i<j); }

double Session::percentile(MachineCode mc,int field,double percent)
{
    if (events.find(mc)==events.end()) return 0;

    vector<EventDataType> array;

    for (auto e=events[mc].begin(); e!=events[mc].end(); e++) {
        Event & ev = *(*e);
        array.push_back(ev[0]);
    }
    std::sort(array.begin(),array.end(),sortfunction);
    int size=array.size();
    double i=size*percent;
    double t;
    double q=modf(i,&t);
    int j=t;
    if (j>=size-1) return array[j];

    double a=array[j-1];
    double b=array[j];
    if (a==b) return a;
    double c=(b-a);
    double d=c*q;
    return array[j]+q;
}

double Session::weighted_avg_event_field(MachineCode mc,int field)
{
    if (events.find(mc)==events.end()) return 0;

    int cnt=0;

    bool first=true;
    wxDateTime last;
    int lastval,val;
    const int max_slots=2600;
    wxTimeSpan vtime[max_slots]=wxTimeSpan(0);

    double mult;
    if ((mc==CPAP_Pressure) || (mc==CPAP_EAP) ||  (mc==CPAP_IAP)) {
        mult=10.0;

    } else mult=10.0;
    for (auto i=events[mc].begin(); i!=events[mc].end(); i++) {
        Event & e =(*(*i));
        val=e[field]*mult;
        if (field > e.e_fields) throw BoundsError();
        if (first) {
            first=false;
        } else {
            wxTimeSpan d=e.e_time-last;
            if (lastval>max_slots) {
                wxLogError(wxT("max_slots to small in Session::weighted_avg_event_fied()"));
            }

            vtime[lastval]+=d;

        }
        cnt++;
        last=e.e_time;
        lastval=val;
    }

    wxTimeSpan total(0);
    for (int i=0; i<max_slots; i++) total+=vtime[i];
    //double hours=total.GetSeconds().GetLo()/3600.0;

    double s0=0,s1=0,s2=0;
    for (int i=0; i<max_slots; i++) {
        if (vtime[i]>wxTimeSpan(0)) {
            s0=(vtime[i].GetSeconds().GetLo()/3600.0);
            s1+=i*s0;
            s2+=s0;
        }
    }

    return (s1/hours())/mult;
}

void Session::AddEvent(Event * e)
{
    events[e->code()].push_back(e);
  //  if (e->time()>s_last) s_last=e->time();
   // wxLogMessage(e->time().Format(wxT("%Y-%m-%d %H:%M:%S"))+wxString::Format(wxT(" %04i %02i "),e->code(),e->fields()));
}
void Session::AddWaveform(Waveform *w)
{
    waveforms[w->code()].push_back(w);
    //wxLogMessage(w->start().Format(wxT("%Y-%m-%d %H:%M:%S"))+wxT(" ")+w->end().Format(wxT("%Y-%m-%d %H:%M:%S"))+wxString::Format(wxT(" %i %.1f"),w->samples(), w->duration()));

    // Could parse the flow data for Breaths per minute info here..


}
void Session::TrashEvents()
// Trash this sessions Events and release memory.
{
    for (auto i=events.begin(); i!=events.end(); i++) {
        for (auto j=i->second.begin(); j!=i->second.end(); j++) {
            delete *j;
        }
    }
    events.clear();
}
void Session::TrashWaveforms()
// Trash this sessions Waveforms and release memory.
{
    for (auto i=waveforms.begin(); i!=waveforms.end(); i++) {
        for (auto j=i->second.begin(); j!=i->second.end(); j++) {
            delete *j;
        }
    }
    waveforms.clear();
}


const int max_pack_size=128;
template <class T>
int pack(char * buf,T * var)
{
    int c=0;
    int s=sizeof(T);
    if (s>max_pack_size) return 0;

    char * dat=(char *)var;
    if (wxIsPlatformLittleEndian()) {
        for (int i=0; i<s; i++) {
            buf[i]=dat[i];
        }
    } else {
        for (int i=0; i<s; i++) {
            buf[s-i]=dat[i];
        }
    }
    return s;
}

/*bool Session::Load(wxString path) // Not Used..
// Load Session Data from our format
// {DataDir}/{MachineID}/{SessionID}.{ext}
{
	path=path+wxFileName::GetPathSeparator()+s_machine->hexid();
	wxString base;
	base.Printf(wxT("%08x"),s_session);
	base=path+wxFileName::GetPathSeparator()+base;
	wxPuts(wxT("Loading Session: ")+base);
	bool a,b,c;
	summary.clear();
	TrashEvents();
	TrashWaveforms();
	//try...
	a=LoadSummary(base+wxT(".000"));
	b=LoadEvents(base+wxT(".001"));
	c=LoadWaveforms(base+wxT(".002"));
	return a;
}*/

bool Session::Store(wxString path)
// Storing Session Data in our format
// {DataDir}/{MachineID}/{SessionID}.{ext}
{

//	path=path+wxFileName::GetPathSeparator()+s_machine->hexid();
    if (!wxDirExists(path)) wxMkdir(path);
    wxString base;
    base.Printf(wxT("%08lx"),s_session);
    base=path+wxFileName::GetPathSeparator()+base;
    //wxPuts(wxT("Storing Session: ")+base);
    bool a,b,c;
    a=StoreSummary(base+wxT(".000"));
    b=StoreEvents(base+wxT(".001"));
    c=StoreWaveforms(base+wxT(".002"));
    return a&b&c;
}

const wxUint32 magic=0xC73216AB;

bool Session::StoreSummary(wxString filename)
{
    BinaryFile f;
    f.Open(filename,BF_WRITE);

    f.Pack((wxUint32)magic); 			// Magic Number
    f.Pack((wxUint32)s_machine->id());	// Machine ID
    f.Pack((wxUint32)s_session);		// Session ID
    f.Pack((wxUint16)0);				// File Type 0 == Summary File
    f.Pack((wxUint16)0);				// File Version

    time_t starttime=s_first.GetTicks();
    time_t duration=s_last.GetTicks()-starttime;

    f.Pack(s_first);					// Session Start Time
    f.Pack((wxInt16)duration);			// Duration of sesion in seconds.
    f.Pack((wxInt16)summary.size());

    map<MachineCode,MCDataType> mctype;

    // First output the Machine Code and type for each summary record
    for (auto i=summary.begin(); i!=summary.end(); i++) {
        MachineCode mc=i->first;

        wxString type=i->second.GetType(); // Urkk.. this is a mess.
        if (type==wxT("bool")) {
            mctype[mc]=MC_bool;
        } else if (type==wxT("long")) {
            mctype[mc]=MC_long;
        } else if (type==wxT("float")) {
            mctype[mc]=MC_float;
        } else if (type==wxT("double")) {
            mctype[mc]=MC_double;
        } else if (type==wxT("string")) {
            mctype[mc]=MC_string;
        } else if (type==wxT("datetime")) {
            mctype[mc]=MC_string;
        } else {
            wxPuts(wxT("Error in Session->StoreSummary: Can't pack variant type ")+type);
            exit(1);
        }
        f.Pack((wxInt16)mc);
        f.Pack((wxInt8)mctype[mc]);
    }
    // Then dump out the actual data, according to format.
    for (auto i=summary.begin(); i!=summary.end(); i++) {
        MachineCode mc=i->first;
        if (mctype[mc]==MC_bool) {
            f.Pack((wxInt8)i->second.GetBool());
        } else if (mctype[mc]==MC_long) {
            f.Pack((wxInt32)i->second.GetLong());
        } else if (mctype[mc]==MC_float) {
            f.Pack((float)i->second.GetDouble());  // gah
        } else if (mctype[mc]==MC_double) {
            f.Pack((double)i->second.GetDouble());
        } else if (mctype[mc]==MC_string) {
            f.Pack(i->second.GetString());
        } else if (mctype[mc]==MC_datetime) {
            f.Pack(i->second.GetDateTime());
        }
    }
    f.Close();
    wxFFile j;
    return true;
}
bool Session::LoadSummary(wxString filename)
{
    //wxPuts(wxT("Loading Summary ")+filename);
    BinaryFile f;
    if (!f.Open(filename,BF_READ)) {
        wxPuts(wxT("Couldn't open file")+filename);
        return false;
    }

    wxUint32 t32;
    wxUint16 t16;
    wxUint8 t8;

    wxInt16 sumsize;
    map<MachineCode,MCDataType> mctype;
    vector<MachineCode> mcorder;

    if (!f.Unpack(t32)) throw UnpackError();		// Magic Number
    if (t32!=magic) throw UnpackError();

    if (!f.Unpack(t32)) throw UnpackError();		// MachineID

    if (!f.Unpack(t32)) throw UnpackError();		// Sessionid;
    s_session=t32;

    if (!f.Unpack(t16)) throw UnpackError();		// File Type
    if (t16!=0) throw UnpackError(); 				//wrong file type

    if (!f.Unpack(t16)) throw UnpackError();		// File Version
    // dont care yet

    if (!f.Unpack(s_first)) throw UnpackError();	// Start time
    if (!f.Unpack(t16)) throw UnpackError();		// Duration // (16bit==Limited to 18 hours)

    s_last=s_first+wxTimeSpan::Seconds(t16);
    s_hours=t16/3600.0;

    if (!f.Unpack(sumsize)) throw UnpackError();	// Summary size (number of Machine Code lists)

    for (int i=0; i<sumsize; i++) {
        if (!f.Unpack(t16)) throw UnpackError();	// Machine Code
        MachineCode mc=(MachineCode)t16;
        if (!f.Unpack(t8)) throw UnpackError();		// Data Type
        mctype[mc]=(MCDataType)t8;
        mcorder.push_back(mc);
    }

    for (int i=0; i<sumsize; i++) {					// Load each summary entry according to type
        MachineCode mc=mcorder[i];
        if (mctype[mc]==MC_bool) {
            bool b;
            if (!f.Unpack(b)) throw UnpackError();
            summary[mc]=b;
        } else if (mctype[mc]==MC_long) {
            if (!f.Unpack(t32)) throw UnpackError();
            summary[mc]=(long)t32;
        } else if (mctype[mc]==MC_float) {
            float fl;
            if (!f.Unpack(fl)) throw UnpackError();
            summary[mc]=(double)fl;
        } else if (mctype[mc]==MC_double) {
            double dl;
            if (!f.Unpack(dl)) throw UnpackError();
            summary[mc]=(double)dl;
        } else if (mctype[mc]==MC_string) {
            wxString s;
            if (!f.Unpack(s)) throw UnpackError();
            summary[mc]=s;
        } else if (mctype[mc]==MC_datetime) {
            wxDateTime dt;
            if (!f.Unpack(dt)) throw UnpackError();
            summary[mc]=dt;
        }

    }
    return true;
}

bool Session::StoreEvents(wxString filename)
{
    BinaryFile f;
    f.Open(filename,BF_WRITE);

    f.Pack((wxUint32)magic); 			// Magic Number
    f.Pack((wxUint32)s_machine->id());  // Machine ID
    f.Pack((wxUint32)s_session);		// This session's ID
    f.Pack((wxUint16)1);				// File type 1 == Event
    f.Pack((wxUint16)0);				// File Version

    time_t starttime=s_first.GetTicks();
    time_t duration=s_last.GetTicks()-starttime;

    f.Pack(s_first);
    f.Pack((wxInt16)duration);

    f.Pack((wxInt16)events.size()); // Number of event categories

    for (auto i=events.begin(); i!=events.end(); i++) {
        f.Pack((wxInt16)i->first); // MachineID
        f.Pack((wxInt16)i->second.size()); // count of events in this category
        auto j=i->second.begin();
        f.Pack((wxInt8)(*j)->fields()); 	// number of data fields in this event type
    }
    bool first;
    float tf;
    time_t last,eventtime,delta;

    for (auto i=events.begin(); i!=events.end(); i++) {
        first=true;
        for (auto j=i->second.begin(); j!=i->second.end(); j++) {
            eventtime=(*j)->time().GetTicks();
            if (first) {
                f.Pack((*j)->time());
                first=false;
            } else {
                delta=eventtime-last;
                if (delta>0xffff) {
                    wxPuts(wxT("StoreEvent: Delta too big.. needed to use bigger value"));
                    exit(1);
                }
                f.Pack((wxInt16)delta);
            }
            for (int k=0; k<(*j)->fields(); k++) {
                tf=(*(*j))[k];
                f.Pack((float)tf);
            }
            last=eventtime;
        }
    }
    f.Close();
    return true;
}
bool Session::LoadEvents(wxString filename)
{
    BinaryFile f;
    if (!f.Open(filename,BF_READ)) {
        wxPuts(wxT("Couldn't open file")+filename);
        return false;
    }

    wxUint32 t32;
    wxUint16 t16;
    wxUint8 t8;
    wxInt16 i16;

//    wxInt16 sumsize;

    if (!f.Unpack(t32)) throw UnpackError();		// Magic Number
    if (t32!=magic) throw UnpackError();

    if (!f.Unpack(t32)) throw UnpackError();		// MachineID

    if (!f.Unpack(t32)) throw UnpackError();		// Sessionid;
    s_session=t32;

    if (!f.Unpack(t16)) throw UnpackError();		// File Type
    if (t16!=1) throw UnpackError(); 				//wrong file type

    if (!f.Unpack(t16)) throw UnpackError();		// File Version
    // dont give a crap yet..

    if (!f.Unpack(s_first)) throw UnpackError();	// Start time
    if (!f.Unpack(t16)) throw UnpackError();		// Duration // (16bit==Limited to 18 hours)

    s_last=s_first+wxTimeSpan::Seconds(t16);
    s_hours=t16/3600.0;

    wxInt16 evsize;
    if (!f.Unpack(evsize)) throw UnpackError();	// Summary size (number of Machine Code lists)

    map<MachineCode,wxInt16> mcsize;
    map<MachineCode,wxInt16> mcfields;
    vector<MachineCode> mcorder;

    MachineCode mc;
    for (int i=0; i<evsize; i++) {
        if (!f.Unpack(t16)) throw UnpackError();  // MachineID
        mc=(MachineCode)t16;
        mcorder.push_back(mc);
        if (!f.Unpack(t16)) throw UnpackError();  // Count of this Event
        mcsize[mc]=t16;
        if (!f.Unpack(t8)) throw UnpackError();  // Count of this Event
        mcfields[mc]=t8;
    }

    for (int i=0; i<evsize; i++) {
        mc=mcorder[i];
        bool first=true;
        wxDateTime d;
        float fl;
        events[mc].clear();
        for (int e=0; e<mcsize[mc]; e++) {
            if (first) {
                if (!f.Unpack(d)) throw UnpackError();  // Timestamp
                first=false;
            } else {
                if (!f.Unpack(i16)) throw UnpackError();  // Time Delta
                d+=wxTimeSpan::Seconds(i16);
            }
            list<EventDataType> ED;
            for (int c=0; c<mcfields[mc]; c++) {
                if (!f.Unpack(fl)); //throw UnpackError();  // Data Fields in float format
                ED.push_back(fl);
            }
            Event *ev=new Event(d,mc,ED);


/*            wxPrintf(d.Format("%Y-%m-%d %H:%M:%S")+wxT(" %i %i ["),mc,e);
            for (auto ed=ED.begin();ed!=ED.end();ed++) {
                wxPrintf(wxT("%.2f "),(*ed));
            }
            wxPrintf(wxT("]\n")); */

            AddEvent(ev);
        }
    }
    return true;
}


bool Session::StoreWaveforms(wxString filename)
{
    BinaryFile f;
    f.Open(filename,BF_WRITE);

    wxUint16 t16;

    f.Pack((wxUint32)magic); 			// Magic Number
    f.Pack((wxUint32)s_machine->id());  // Machine ID
    f.Pack((wxUint32)s_session);		// This session's ID
    f.Pack((wxUint16)2);				// File type 2 == Waveform
    f.Pack((wxUint16)0);				// File Version

    time_t starttime=s_first.GetTicks();
    time_t duration=s_last.GetTicks()-starttime;

    f.Pack(s_first);
    f.Pack((wxInt16)duration);

    f.Pack((wxInt16)waveforms.size());	// Number of different waveforms

    for (auto i=waveforms.begin(); i!=waveforms.end(); i++) {
        f.Pack((wxInt16)i->first); 		// Machine Code
        t16=i->second.size();
        f.Pack(t16); 					// Number of (hopefully non-linear) waveform chunks

        for (auto j=i->second.begin(); j!=i->second.end(); j++) {

            Waveform &w=*(*j);
            f.Pack(w.start()); 			// Start time of first waveform chunk

            //wxInt32 samples;
            //double seconds;

            f.Pack((wxInt32)w.samples());					// Total number of samples
            f.Pack((float)w.duration());            // Total number of seconds
            f.Pack((wxInt16)w.min());
            f.Pack((wxInt16)w.max());
            f.Pack((wxInt8)sizeof(SampleFormat));  	// Bytes per sample
            f.Pack((wxInt8)0); // signed.. all samples for now are signed 16bit.
            //t8=0; // 0=signed, 1=unsigned, 2=float

            // followed by sample data.
            if (wxIsPlatformLittleEndian()) {
                f.Write(w.GetBuffer(),w.samples()*sizeof(SampleFormat));
            } else {
                for (int k=0; k<(*j)->samples(); k++) f.Pack((wxInt16)w[k]);
            }
        }
    }
    return true;
}


bool Session::LoadWaveforms(wxString filename)
{
    BinaryFile f;
    if (!f.Open(filename,BF_READ)) {
        wxPuts(wxT("Couldn't open file")+filename);
        return false;
    }

    wxUint32 t32;
    wxUint16 t16;
    wxUint8 t8;

    if (!f.Unpack(t32)) throw UnpackError();		// Magic Number
    if (t32!=magic) throw UnpackError();

    if (!f.Unpack(t32)) throw UnpackError();		// MachineID

    if (!f.Unpack(t32)) throw UnpackError();		// Sessionid;
    s_session=t32;

    if (!f.Unpack(t16)) throw UnpackError();		// File Type
    if (t16!=2) throw UnpackError(); 				//wrong file type?

    if (!f.Unpack(t16)) throw UnpackError();		// File Version
    // dont give a crap yet..

    if (!f.Unpack(s_first)) throw UnpackError();	// Start time
    if (!f.Unpack(t16)) throw UnpackError();		// Duration // (16bit==Limited to 18 hours)

    s_last=s_first+wxTimeSpan::Seconds(t16);
    s_hours=t16/3600.0;

    wxInt16 wvsize;
    if (!f.Unpack(wvsize)) throw UnpackError();	// Summary size (number of Machine Code lists)

    MachineCode mc;
    wxDateTime date;
    float seconds;
    wxInt32 samples;
    int chunks;
    SampleFormat min,max;
    for (int i=0; i<wvsize; i++) {
        if (!f.Unpack(t16)) throw UnpackError();		// Machine Code
        mc=(MachineCode)t16;
        if (!f.Unpack(t16)) throw UnpackError();		// Machine Code
        chunks=t16;

        for (int i=0; i<chunks; i++) {

            if (!f.Unpack(date)) throw UnpackError();		// Waveform DateTime
            if (!f.Unpack(samples)) throw UnpackError();	// Number of samples
            if (!f.Unpack(seconds)) throw UnpackError();	// Duration in seconds
            if (!f.Unpack(min)) throw UnpackError();		// Min value
            if (!f.Unpack(max)) throw UnpackError();		// Min value
            if (!f.Unpack(t8)) throw UnpackError();			// sample size in bytes
            if (!f.Unpack(t8)) throw UnpackError();			// Number of samples

            SampleFormat *data=new SampleFormat [samples];

            for (int k=0; k<samples; k++) {
                if (!f.Unpack(t16)) throw UnpackError();			// Number of samples
                data[k]=t16;
            }
            Waveform *w=new Waveform(date,mc,data,samples,seconds,min,max);
            AddWaveform(w);
        }
    }
    return true;
}




