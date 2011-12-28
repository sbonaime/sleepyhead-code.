/*

SleepLib Profiles Header

Author: Mark Watkins <jedimark64@users.sourceforge.net>
License: GPL

*/

#ifndef PROFILES_H
#define PROFILES_H

#include <QString>
#include <QCryptographicHash>
#include <QThread>
#include "machine.h"
#include "machine_loader.h"
#include "preferences.h"
#include "common.h"

class Machine;

enum Gender { GenderNotSpecified, Male, Female };
enum MaskType { Mask_Unknown, Mask_NasalPillows, Mask_Hybrid, Mask_StandardNasal, Mask_FullFace };
enum OverlayDisplayType { ODT_Bars, ODT_TopAndBottom };

class DoctorInfo;
class UserInfo;
class UserSettings;
class OxiSettings;
class CPAPSettings;
class AppearanceSettings;
class SessionSettings;



/*!
  \class Profile
  \author Mark Watkins
  \date 28/04/11
  \brief The User profile system, containing all information for a user, and an index into all Machine data
 */
class Profile:public Preferences
{
public:
    //! \brief Creates a new Profile object 'name' (which is usually just set to "Profile", the XML filename is derived from this)
    Profile(QString name);

    //! \brief Create a new empty Profile object
    Profile();
    virtual ~Profile();

    //! \brief Save Profile object (This is an extension to Preference::Save(..))
    virtual bool Save(QString filename="");

    bool is_first_day;

    //! \brief List of machines, indexed by MachineID
    QHash<MachineID,Machine *> machlist;

    //! \brief Add machine to this profiles machlist
    void AddMachine(Machine *m);

    //! \brief Remove machine from this profiles machlist
    void DelMachine(Machine *m);

    //! \brief Loads all machine (summary) data belonging to this profile
    void LoadMachineData();

    //! \brief Barf because data format has changed. This does a purge of CPAP data for machine *m
    void DataFormatError(Machine *m);

    /*! \brief Import Machine Data
        \param path containing import location
     */
    int Import(QString path);

    //! \brief Remove a session from day object, without deleting the Session object
    void RemoveSession(Session * sess);

    //! \brief Add Day record to Profile Day list
    void AddDay(QDate date,Day *day,MachineType mt);

    //! \brief Get Day record if data available for date and machine type, else return NULL
    Day * GetDay(QDate date,MachineType type=MT_UNKNOWN);

    //! \brief Get Day record if data available for date and machine type, and has enabled session data, else return NULL
    Day * GetGoodDay(QDate date,MachineType type);


    //! \brief Returns a list of all machines of type t
    QList<Machine *> GetMachines(MachineType t=MT_UNKNOWN);

    //! \brief Returns the machine of type t used on date, NULL if none..
    Machine * GetMachine(MachineType t,QDate date);

    //! \brief return the first machine of type t
    Machine * GetMachine(MachineType t);

    //! \brief Returns true if this profile stores this variable identified by key
    bool contains(QString key) { return p_preferences.contains(key); }

    int countDays(MachineType mt=MT_UNKNOWN, QDate start=QDate(), QDate end=QDate());
    EventDataType calcCount(ChannelID code, MachineType mt=MT_CPAP, QDate start=QDate(), QDate end=QDate());
    double calcSum(ChannelID code, MachineType mt=MT_CPAP, QDate start=QDate(), QDate end=QDate());
    EventDataType calcHours(MachineType mt=MT_CPAP, QDate start=QDate(), QDate end=QDate());
    EventDataType calcAvg(ChannelID code, MachineType mt=MT_CPAP, QDate start=QDate(), QDate end=QDate());
    EventDataType calcWavg(ChannelID code, MachineType mt=MT_CPAP, QDate start=QDate(), QDate end=QDate());
    EventDataType calcMin(ChannelID code, MachineType mt=MT_CPAP, QDate start=QDate(), QDate end=QDate());
    EventDataType calcMax(ChannelID code, MachineType mt=MT_CPAP, QDate start=QDate(), QDate end=QDate());
    EventDataType calcPercentile(ChannelID code, EventDataType percent, MachineType mt=MT_CPAP, QDate start=QDate(), QDate end=QDate());

    EventDataType calcSettingsMin(ChannelID code, MachineType mt=MT_CPAP, QDate start=QDate(), QDate end=QDate());
    EventDataType calcSettingsMax(ChannelID code, MachineType mt=MT_CPAP, QDate start=QDate(), QDate end=QDate());

    virtual void ExtraLoad(QDomElement & root);
    virtual QDomElement ExtraSave(QDomDocument & doc);

    QMap<QDate,QList<Day *> > daylist;
    QDate FirstDay(MachineType mt=MT_UNKNOWN);
    QDate LastDay(MachineType mt=MT_UNKNOWN);

    QDate FirstGoodDay(MachineType mt=MT_UNKNOWN);
    QDate LastGoodDay(MachineType mt=MT_UNKNOWN);

    QString dataFolder() { return (*this).Get("{DataFolder}"); }

    UserInfo *user;
    CPAPSettings *cpap;
    OxiSettings *oxi;
    DoctorInfo *doctor;
    AppearanceSettings *appearance;
    UserSettings *general;
    SessionSettings *session;


protected:
    QDate m_first,m_last;

};

class MachineLoader;
extern MachineLoader * GetLoader(QString name);

extern Preferences *p_pref;
extern Preferences *p_layout;
extern Profile * p_profile;

// these are bad and must change
#define PREF (*p_pref)
#define LAYOUT (*p_layout)
#define PROFILE (*p_profile)

extern const char * DI_STR_Name;
extern const char * DI_STR_Phone;
extern const char * DI_STR_Email;
extern const char * DI_STR_Practice;
extern const char * DI_STR_Address;
extern const char * DI_STR_PatientID;

class DoctorInfo
{
public:
    DoctorInfo(Profile *p) : m_profile(p)
    {
        if (!m_profile->contains(DI_STR_Name)) (*m_profile)[DI_STR_Name]=QString();
        if (!m_profile->contains(DI_STR_Phone)) (*m_profile)[DI_STR_Phone]=QString();
        if (!m_profile->contains(DI_STR_Email)) (*m_profile)[DI_STR_Email]=QString();
        if (!m_profile->contains(DI_STR_Practice)) (*m_profile)[DI_STR_Practice]=QString();
        if (!m_profile->contains(DI_STR_Address)) (*m_profile)[DI_STR_Address]=QString();
        if (!m_profile->contains(DI_STR_PatientID)) (*m_profile)[DI_STR_PatientID]=QString();
    }
    ~DoctorInfo() {}
    void setProfile(Profile *p) { m_profile=p; }

    const QString name() { return (*m_profile)[DI_STR_Name].toString(); }
    const QString phone() { return (*m_profile)[DI_STR_Phone].toString(); }
    const QString email() { return (*m_profile)[DI_STR_Email].toString(); }
    const QString practiceName() { return (*m_profile)[DI_STR_Practice].toString(); }
    const QString address() { return (*m_profile)[DI_STR_Address].toString(); }
    const QString patiendID() { return (*m_profile)[DI_STR_PatientID].toString(); }

    void setName(QString name) { (*m_profile)[DI_STR_Name]=name; }
    void setPhone(QString phone) { (*m_profile)[DI_STR_Phone]=phone; }
    void setEmail(QString phone) { (*m_profile)[DI_STR_Email]=phone; }
    void setPracticeName(QString practice) { (*m_profile)[DI_STR_Practice]=practice; }
    void setAddress(QString address) { (*m_profile)[DI_STR_Address]=address; }
    void setPatiendID(QString pid) { (*m_profile)[DI_STR_PatientID]=pid; }

    Profile *m_profile;
};

extern const char * UI_STR_DOB;
extern const char * UI_STR_FirstName;
extern const char * UI_STR_LastName;
extern const char * UI_STR_UserName;
extern const char * UI_STR_Password;
extern const char * UI_STR_Address;
extern const char * UI_STR_Phone;
extern const char * UI_STR_EmailAddress;
extern const char * UI_STR_Country;
extern const char * UI_STR_Height;
extern const char * UI_STR_Gender;
extern const char * UI_STR_TimeZone;
extern const char * UI_STR_Language;
extern const char * UI_STR_DST;

/*! \class UserInfo
    \brief Profile Options relating to the User Information
    */
class UserInfo
{
public:
    //! \brief Create UserInfo object given Profile *p, and initialize the defaults
    UserInfo(Profile *p) : m_profile(p)
    {
        if (!m_profile->contains(UI_STR_DOB)) (*m_profile)[UI_STR_DOB]=QDate(1970,1,1);
        if (!m_profile->contains(UI_STR_FirstName)) (*m_profile)[UI_STR_FirstName]=QString();
        if (!m_profile->contains(UI_STR_LastName)) (*m_profile)[UI_STR_LastName]=QString();
        if (!m_profile->contains(UI_STR_UserName)) (*m_profile)[UI_STR_UserName]=QString();
        if (!m_profile->contains(UI_STR_Password)) (*m_profile)[UI_STR_Password]=QString();
        if (!m_profile->contains(UI_STR_Address)) (*m_profile)[UI_STR_Address]=QString();
        if (!m_profile->contains(UI_STR_Phone)) (*m_profile)[UI_STR_Phone]=QString();
        if (!m_profile->contains(UI_STR_EmailAddress)) (*m_profile)[UI_STR_EmailAddress]=QString();
        if (!m_profile->contains(UI_STR_Country)) (*m_profile)[UI_STR_Country]=QString();
        if (!m_profile->contains(UI_STR_Height)) (*m_profile)[UI_STR_Height]=0.0;
        if (!m_profile->contains(UI_STR_Gender)) (*m_profile)[UI_STR_Gender]=(int)GenderNotSpecified;
        if (!m_profile->contains(UI_STR_TimeZone)) (*m_profile)[UI_STR_TimeZone]=QString();
        if (!m_profile->contains(UI_STR_Language)) (*m_profile)[UI_STR_Language]="English";
        if (!m_profile->contains(UI_STR_DST)) (*m_profile)[UI_STR_DST]=false;

    }
    ~UserInfo() {}

    void setProfile(Profile *p) { m_profile=p; }

    QDate DOB() { return (*m_profile)[UI_STR_DOB].toDate(); }
    const QString firstName() { return (*m_profile)[UI_STR_FirstName].toString(); }
    const QString lastName() { return (*m_profile)[UI_STR_LastName].toString(); }
    const QString userName() { return (*m_profile)[UI_STR_UserName].toString(); }
    const QString address() { return (*m_profile)[UI_STR_Address].toString(); }
    const QString phone() { return (*m_profile)[UI_STR_Phone].toString(); }
    const QString email() { return (*m_profile)[UI_STR_EmailAddress].toString(); }
    double height() { return (*m_profile)[UI_STR_Height].toDouble(); }
    const QString country() { return (*m_profile)[UI_STR_Country].toString(); }
    Gender gender() { return (Gender)(*m_profile)[UI_STR_Gender].toInt(); }
    const QString timeZone() { return (*m_profile)[UI_STR_TimeZone].toString(); }
    const QString language() { return (*m_profile)[UI_STR_Language].toString(); }
    bool daylightSaving() { return (*m_profile)[UI_STR_DST].toBool(); }

    void setDOB(QDate date) { (*m_profile)[UI_STR_DOB]=date; }
    void setFirstName(QString name) { (*m_profile)[UI_STR_FirstName]=name; }
    void setLastName(QString name) { (*m_profile)[UI_STR_LastName]=name; }
    void setUserName(QString username) { (*m_profile)[UI_STR_UserName]=username; }
    void setAddress(QString address) { (*m_profile)[UI_STR_Address]=address; }
    void setPhone(QString phone) { (*m_profile)[UI_STR_Phone]=phone; }
    void setEmail(QString email) { (*m_profile)[UI_STR_EmailAddress]=email; }
    void setHeight(double height) { (*m_profile)[UI_STR_Height]=height; }
    void setCountry(QString country) { (*m_profile)[UI_STR_Country]=country; }
    void setGender(Gender g) { (*m_profile)[UI_STR_Gender]=(int)g; }
    void setTimeZone(QString tz) { (*m_profile)[UI_STR_TimeZone]=tz; }
    void setLanguage(QString language) { (*m_profile)[UI_STR_Language]=language; }
    void setDaylightSaving(bool ds) { (*m_profile)[UI_STR_DST]=ds; }

    bool hasPassword() {
        return !((*m_profile)[UI_STR_Password].toString().isEmpty());
    }
    bool checkPassword(QString password) {
        QByteArray ba=password.toUtf8();
        return ((*m_profile)[UI_STR_Password].toString()==QString(QCryptographicHash::hash(ba,QCryptographicHash::Sha1).toHex()));
    }
    void setPassword(QString password) {
        QByteArray ba=password.toUtf8();
        // Hash me.
        (*m_profile)[UI_STR_Password]=QString(QCryptographicHash::hash(ba,QCryptographicHash::Sha1).toHex());
    }

    Profile *m_profile;
};

extern const char * OS_STR_EnableOximetry;
extern const char * OS_STR_SyncOximetry;
extern const char * OS_STR_OximeterType;
extern const char * OS_STR_OxiDiscardThreshold;
extern const char * OS_STR_SPO2DropDuration;
extern const char * OS_STR_SPO2DropPercentage;
extern const char * OS_STR_PulseChangeDuration;
extern const char * OS_STR_PulseChangeBPM;

/*! \class OxiSettings
    \brief Profile Options relating to the Oximetry settings
    */
class OxiSettings
{
public:
    //! \brief Create OxiSettings object given Profile *p, and initialize the defaults
    OxiSettings(Profile *p) :m_profile(p)
    {
        if (!m_profile->contains(OS_STR_EnableOximetry)) (*m_profile)[OS_STR_EnableOximetry]=false;
        if (!m_profile->contains(OS_STR_SyncOximetry)) (*m_profile)[OS_STR_SyncOximetry]=true;
        if (!m_profile->contains(OS_STR_OximeterType)) (*m_profile)[OS_STR_OximeterType]="CMS50";
        if (!m_profile->contains(OS_STR_OxiDiscardThreshold)) (*m_profile)[OS_STR_OxiDiscardThreshold]=0.0;
        if (!m_profile->contains(OS_STR_SPO2DropDuration)) (*m_profile)[OS_STR_SPO2DropDuration]=8.0;
        if (!m_profile->contains(OS_STR_SPO2DropPercentage)) (*m_profile)[OS_STR_SPO2DropPercentage]=3.0;
        if (!m_profile->contains(OS_STR_PulseChangeDuration)) (*m_profile)[OS_STR_PulseChangeDuration]=8.0;
        if (!m_profile->contains(OS_STR_PulseChangeBPM)) (*m_profile)[OS_STR_PulseChangeBPM]=5.0;
    }
    ~OxiSettings() {}

    void setProfile(Profile *p) { m_profile=p; }

    bool oximetryEnabled()  { return (*m_profile)[OS_STR_EnableOximetry].toBool(); }
    bool syncOximetry() { return (*m_profile)[OS_STR_SyncOximetry].toBool(); }
    QString oximeterType() { return (*m_profile)[OS_STR_OximeterType].toString(); }
    double oxiDiscardThreshold() { return (*m_profile)[OS_STR_OxiDiscardThreshold].toDouble(); }
    double spO2DropDuration() { return (*m_profile)[OS_STR_SPO2DropDuration].toDouble(); }
    double spO2DropPercentage() { return (*m_profile)[OS_STR_SPO2DropPercentage].toDouble(); }
    double pulseChangeDuration() { return (*m_profile)[OS_STR_PulseChangeDuration].toDouble(); }
    double pulseChangeBPM() { return (*m_profile)[OS_STR_PulseChangeBPM].toDouble(); }

    void setOximetryEnabled(bool enabled)  { (*m_profile)[OS_STR_EnableOximetry]=enabled; }
    void setSyncOximetry(bool synced) { (*m_profile)[OS_STR_SyncOximetry]=synced; }
    void setOximeterType(QString oxitype) { (*m_profile)[OS_STR_OximeterType]=oxitype; }
    void setOxiDiscardThreshold(double thresh) { (*m_profile)[OS_STR_OxiDiscardThreshold]=thresh; }
    void setSpO2DropDuration(double duration) { (*m_profile)[OS_STR_SPO2DropDuration]=duration; }
    void setSpO2DropPercentage(double percentage) { (*m_profile)[OS_STR_SPO2DropPercentage]=percentage; }
    void setPulseChangeDuration(double duration) { (*m_profile)[OS_STR_PulseChangeDuration]=duration; }
    void setPulseChangeBPM(double bpm) { (*m_profile)[OS_STR_PulseChangeBPM]=bpm; }

    Profile *m_profile;
};

extern const char * CS_STR_ComplianceHours;
extern const char * CS_STR_ShowCompliance;
extern const char * CS_STR_ShowLeaksMode;
extern const char * CS_STR_MaskStartDate;
extern const char * CS_STR_MaskDescription;
extern const char * CS_STR_MaskType;
extern const char * CS_STR_PrescribedMode;
extern const char * CS_STR_PrescribedMinPressure;
extern const char * CS_STR_PrescribedMaxPressure;
extern const char * CS_STR_UntreatedAHI;
extern const char * CS_STR_Notes;
extern const char * CS_STR_DateDiagnosed;

/*! \class CPAPSettings
    \brief Profile Options relating to the CPAP settings
    */
class CPAPSettings
{
public:
    //! \brief Create CPAPSettings object given Profile *p, and initialize the defaults
    CPAPSettings(Profile *p) :m_profile(p)
    {
        if (!m_profile->contains(CS_STR_ComplianceHours)) (*m_profile)[CS_STR_ComplianceHours]=4;
        if (!m_profile->contains(CS_STR_ShowCompliance)) (*m_profile)[CS_STR_ShowCompliance]=true;
        if (!m_profile->contains(CS_STR_ShowLeaksMode)) (*m_profile)[CS_STR_ShowLeaksMode]=0;
        // TODO: Check if this date is initiliazed yet
        if (!m_profile->contains(CS_STR_MaskStartDate)) (*m_profile)[CS_STR_MaskStartDate]=QDate();
        if (!m_profile->contains(CS_STR_MaskDescription)) (*m_profile)[CS_STR_MaskDescription]=QString();
        if (!m_profile->contains(CS_STR_MaskType)) (*m_profile)[CS_STR_MaskType]=Mask_Unknown;
        if (!m_profile->contains(CS_STR_PrescribedMode)) (*m_profile)[CS_STR_PrescribedMode]=MODE_UNKNOWN;
        if (!m_profile->contains(CS_STR_PrescribedMinPressure)) (*m_profile)[CS_STR_PrescribedMinPressure]=0.0;
        if (!m_profile->contains(CS_STR_PrescribedMaxPressure)) (*m_profile)[CS_STR_PrescribedMaxPressure]=0.0;
        if (!m_profile->contains(CS_STR_UntreatedAHI)) (*m_profile)[CS_STR_UntreatedAHI]=0.0;
        if (!m_profile->contains(CS_STR_Notes)) (*m_profile)[CS_STR_Notes]=QString();
        if (!m_profile->contains(CS_STR_DateDiagnosed)) (*m_profile)[CS_STR_DateDiagnosed]=QDate();
    }

    ~CPAPSettings() {}

    void setProfile(Profile *p) { m_profile=p; }

    //Getters
    double complianceHours() { return (*m_profile)[CS_STR_ComplianceHours].toDouble(); }
    bool showComplianceInfo() { return (*m_profile)[CS_STR_ShowCompliance].toBool(); }
    int leakMode() { return (*m_profile)[CS_STR_ShowLeaksMode].toInt(); }
    QDate maskStartDate() { return (*m_profile)[CS_STR_MaskStartDate].toDate(); }
    QString maskDescription() { return (*m_profile)[CS_STR_MaskDescription].toString(); }
    MaskType maskType() { return (MaskType)(*m_profile)[CS_STR_MaskType].toInt(); }
    CPAPMode mode() { return CPAPMode((*m_profile)[CS_STR_PrescribedMode].toInt()); }
    double minPressure() { return (*m_profile)[CS_STR_PrescribedMinPressure].toDouble(); }
    double maxPressure() { return (*m_profile)[CS_STR_PrescribedMaxPressure].toDouble(); }
    double untreatedAHI() { return (*m_profile)[CS_STR_UntreatedAHI].toDouble(); }
    const QString notes() { return (*m_profile)[CS_STR_Notes].toString(); }
    QDate dateDiagnosed() { return (*m_profile)[CS_STR_DateDiagnosed].toDate(); }

    //Setters
    void setMode(CPAPMode mode) { (*m_profile)[CS_STR_PrescribedMode]=(int)mode; }
    void setMinPressure(double pressure) { (*m_profile)[CS_STR_PrescribedMinPressure]=pressure; }
    void setMaxPressure(double pressure) { (*m_profile)[CS_STR_PrescribedMaxPressure]=pressure; }
    void setUntreatedAHI(double ahi) { (*m_profile)[CS_STR_UntreatedAHI]=ahi; }
    void setNotes(QString notes) { (*m_profile)[CS_STR_Notes]=notes; }
    void setDateDiagnosed(QDate date) { (*m_profile)[CS_STR_DateDiagnosed]=date; }
    void setComplianceHours(double hours) { (*m_profile)[CS_STR_ComplianceHours]=hours; }
    void setShowComplianceInfo(bool b) { (*m_profile)[CS_STR_ShowCompliance]=b; }
    void setLeakMode(int leakmode) { (*m_profile)[CS_STR_ShowLeaksMode]=(int)leakmode; }
    void setMaskStartDate(QDate date) { (*m_profile)[CS_STR_MaskStartDate]=date; }
    void setMaskDescription(QString description) { (*m_profile)[CS_STR_MaskDescription]=description; }
    void setMaskType(MaskType masktype) { (*m_profile)[CS_STR_MaskType]=(int)masktype; }

    Profile *m_profile;
};

extern const char * IS_STR_DaySplitTime;
extern const char * IS_STR_CacheSessions;
extern const char * IS_STR_CombineCloseSessions;
extern const char * IS_STR_IgnoreShorterSessions;
extern const char * IS_STR_Multithreading;
extern const char * IS_STR_TrashDayCache;
extern const char * IS_STR_ShowSerialNumbers;

/*! \class ImportSettings
    \brief Profile Options relating to the Import process
    */
class SessionSettings
{
public:
    //! \brief Create ImportSettings object given Profile *p, and initialize the defaults
    SessionSettings(Profile *p) :m_profile(p)
    {
        if (!m_profile->contains(IS_STR_DaySplitTime)) (*m_profile)[IS_STR_DaySplitTime]=QTime(12,0,0);
        if (!m_profile->contains(IS_STR_CacheSessions)) (*m_profile)[IS_STR_CacheSessions]=false;
        if (!m_profile->contains(IS_STR_CombineCloseSessions)) (*m_profile)[IS_STR_CombineCloseSessions]=240;
        if (!m_profile->contains(IS_STR_IgnoreShorterSessions)) (*m_profile)[IS_STR_IgnoreShorterSessions]=5;
        if (!m_profile->contains(IS_STR_Multithreading)) (*m_profile)[IS_STR_Multithreading]=QThread::idealThreadCount() > 1;
        if (!m_profile->contains(IS_STR_TrashDayCache)) (*m_profile)[IS_STR_TrashDayCache]=false; // can't remember..
        if (!m_profile->contains(IS_STR_ShowSerialNumbers)) (*m_profile)[IS_STR_ShowSerialNumbers]=false;
    }
    ~SessionSettings() {}

    void setProfile(Profile *p) { m_profile=p; }

    QTime daySplitTime() { return (*m_profile)[IS_STR_DaySplitTime].toTime(); }
    bool cacheSessions() { return (*m_profile)[IS_STR_CacheSessions].toBool(); }
    double combineCloseSessions() { return (*m_profile)[IS_STR_CombineCloseSessions].toDouble(); }
    double ignoreShortSessions() { return (*m_profile)[IS_STR_IgnoreShorterSessions].toDouble(); }
    bool multithreading() { return (*m_profile)[IS_STR_Multithreading].toBool(); }
    bool trashDayCache() { return (*m_profile)[IS_STR_TrashDayCache].toBool(); }
    bool showSerialNumbers() { return (*m_profile)[IS_STR_ShowSerialNumbers].toBool(); }

    void setDaySplitTime(QTime time) { (*m_profile)[IS_STR_DaySplitTime]=time; }
    void setCacheSessions(bool c) { (*m_profile)[IS_STR_CacheSessions]=c; }
    void setCombineCloseSessions(double val) { (*m_profile)[IS_STR_CombineCloseSessions]=val; }
    void setIgnoreShortSessions(double val) { (*m_profile)[IS_STR_IgnoreShorterSessions]=val; }
    void setMultithreading(bool enabled) { (*m_profile)[IS_STR_Multithreading]=enabled; }
    void setTrashDayCache(bool trash) { (*m_profile)[IS_STR_TrashDayCache]=trash; }
    void setShowSerialNumbers(bool trash) { (*m_profile)[IS_STR_ShowSerialNumbers]=trash; }

    Profile *m_profile;
};

extern const char * AS_STR_GraphHeight;
extern const char * AS_STR_AntiAliasing;
extern const char * AS_STR_HighResPrinting;
extern const char * AS_STR_GraphSnapshots;
extern const char * AS_STR_Animations;
extern const char * AS_STR_SquareWave;
extern const char * AS_STR_OverlayType;

/*! \class AppearanceSettings
    \brief Profile Options relating to Visual Appearance
    */
class AppearanceSettings
{
public:
    //! \brief Create AppearanceSettings object given Profile *p, and initialize the defaults
    AppearanceSettings(Profile *p) :m_profile(p)
    {
        if (!m_profile->contains(AS_STR_GraphHeight)) (*m_profile)[AS_STR_GraphHeight]=180.0;
        if (!m_profile->contains(AS_STR_AntiAliasing)) (*m_profile)[AS_STR_AntiAliasing]=false; // i think it's ugly
        if (!m_profile->contains(AS_STR_HighResPrinting)) (*m_profile)[AS_STR_HighResPrinting]=true;
        if (!m_profile->contains(AS_STR_GraphSnapshots)) (*m_profile)[AS_STR_GraphSnapshots]=true;
        if (!m_profile->contains(AS_STR_Animations)) (*m_profile)[AS_STR_Animations]=true;
        if (!m_profile->contains(AS_STR_SquareWave)) (*m_profile)[AS_STR_SquareWave]=false;
        if (!m_profile->contains(AS_STR_OverlayType)) (*m_profile)[AS_STR_OverlayType]=ODT_Bars;
    }
    ~AppearanceSettings() {}

    void setProfile(Profile *p) { m_profile=p; }

    //! \brief Returns the normal (unscaled) height of a graph
    int graphHeight() { return (*m_profile)[AS_STR_GraphHeight].toInt(); }
    //! \brief Returns true if AntiAliasing (the graphical smoothing method) is enabled
    bool antiAliasing() { return (*m_profile)[AS_STR_AntiAliasing].toBool(); }
    //! \brief Returns true if QPrinter object should use higher-quality High Resolution print mode
    bool highResPrinting() { return (*m_profile)[AS_STR_HighResPrinting].toBool(); }
    //! \brief Returns true if renderPixmap function is in use, which takes snapshots of graphs
    bool graphSnapshots() { return (*m_profile)[AS_STR_GraphSnapshots].toBool(); }
    //! \brief Returns true if Graphical animations & Transitions will be drawn
    bool animations() { return (*m_profile)[AS_STR_Animations].toBool(); }
    //! \brief Returns true if Square Wave plots are preferred (where possible)
    bool squareWavePlots() { return (*m_profile)[AS_STR_SquareWave].toBool(); }
    //! \brief Returns the type of overlay flags (which are displayed over the Flow Waveform)
    OverlayDisplayType overlayType() { return (OverlayDisplayType )(*m_profile)[AS_STR_OverlayType].toInt(); }

    //! \brief Set the normal (unscaled) height of a graph.
    void setGraphHeight(int height) { (*m_profile)[AS_STR_GraphHeight]=height; }
    //! \brief Set to true to turn on AntiAliasing (the graphical smoothing method)
    void setAntiAliasing(bool aa) { (*m_profile)[AS_STR_AntiAliasing]=aa; }
    //! \brief Set to true if QPrinter object should use higher-quality High Resolution print mode
    void setHighResPrinting(bool hires) { (*m_profile)[AS_STR_HighResPrinting]=hires; }
    //! \brief Set to true if renderPixmap functions are in use, which takes snapshots of graphs.
    void setGraphSnapshots(bool gs) { (*m_profile)[AS_STR_GraphSnapshots]=gs; }
    //! \brief Set to true if Graphical animations & Transitions will be drawn
    void setAnimations(bool anim) { (*m_profile)[AS_STR_Animations]=anim; }
    //! \brief Set whether or not to useSquare Wave plots (where possible)
    void setSquareWavePlots(bool sw) { (*m_profile)[AS_STR_SquareWave]=sw; }
    //! \brief Sets the type of overlay flags (which are displayed over the Flow Waveform)
    void setOverlayType(OverlayDisplayType od) { (*m_profile)[AS_STR_OverlayType]=(int)od; }

    Profile *m_profile;
};


extern const char * US_STR_UnitSystem;
extern const char * US_STR_EventWindowSize;
extern const char * US_STR_SkipEmptyDays;
extern const char * US_STR_RebuildCache;
extern const char * US_STR_ShowDebug;
extern const char * US_STR_LinkGroups;
extern const char * US_STR_CalculateRDI;

/*! \class UserSettings
    \brief Profile Options relating to General User Settings
    */
class UserSettings
{
public:
    //! \brief Create UserSettings object given Profile *p, and initialize the defaults
    UserSettings(Profile *p) :m_profile(p)
    {
        if (!m_profile->contains(US_STR_UnitSystem)) (*m_profile)[US_STR_UnitSystem]=US_Metric;
        if (!m_profile->contains(US_STR_EventWindowSize)) (*m_profile)[US_STR_EventWindowSize]=4.0;
        if (!m_profile->contains(US_STR_SkipEmptyDays)) (*m_profile)[US_STR_SkipEmptyDays]=true;
        if (!m_profile->contains(US_STR_RebuildCache)) (*m_profile)[US_STR_RebuildCache]=false; // can't remember..
        if (!m_profile->contains(US_STR_ShowDebug)) (*m_profile)[US_STR_ShowDebug]=false;
        if (!m_profile->contains(US_STR_LinkGroups)) (*m_profile)[US_STR_LinkGroups]=true; // can't remember..
        if (!m_profile->contains(US_STR_CalculateRDI)) (*m_profile)[US_STR_CalculateRDI]=false;
    }
    ~UserSettings() {}

    void setProfile(Profile *p) { m_profile=p; }

    UnitSystem unitSystem() { return (UnitSystem)(*m_profile)[US_STR_UnitSystem].toInt(); }
    double eventWindowSize() { return (*m_profile)[US_STR_EventWindowSize].toDouble(); }
    bool skipEmptyDays() { return (*m_profile)[US_STR_SkipEmptyDays].toBool(); }
    bool rebuildCache() { return (*m_profile)[US_STR_RebuildCache].toBool(); }
    bool showDebug() { return (*m_profile)[US_STR_ShowDebug].toBool(); }
    bool linkGroups() { return (*m_profile)[US_STR_LinkGroups].toBool(); }
    bool calculateRDI() { return (*m_profile)[US_STR_CalculateRDI].toBool(); }

    void setUnitSystem(UnitSystem us) { (*m_profile)[US_STR_UnitSystem]=(int)us; }
    void setEventWindowSize(double size) { (*m_profile)[US_STR_EventWindowSize]=size; }
    void setSkipEmptyDays(bool skip) { (*m_profile)[US_STR_SkipEmptyDays]=skip; }
    void setRebuildCache(bool rebuild) { (*m_profile)[US_STR_RebuildCache]=rebuild; }
    void setShowDebug(bool b) { (*m_profile)[US_STR_ShowDebug]=b; }
    void setLinkGroups(bool link) { (*m_profile)[US_STR_LinkGroups]=link; }
    void setCalculateRDI(bool rdi) { (*m_profile)[US_STR_CalculateRDI]=rdi; }


    Profile *m_profile;
};


namespace Profiles
{

extern QHash<QString,Profile *> profiles;
void Scan(); // Initialize and load Profile
void Done(); // Save all Profile objects and clear list

Profile *Create(QString name);
Profile *Get(QString name);
Profile *Get();

}

#endif //PROFILES_H

