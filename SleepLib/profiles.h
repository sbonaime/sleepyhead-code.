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

extern const char * STR_DI_Name;
extern const char * STR_DI_Phone;
extern const char * STR_DI_Email;
extern const char * STR_DI_Practice;
extern const char * STR_DI_Address;
extern const char * STR_DI_PatientID;

class DoctorInfo
{
public:
    DoctorInfo(Profile *p) : m_profile(p)
    {
        if (!m_profile->contains(STR_DI_Name)) (*m_profile)[STR_DI_Name]=QString();
        if (!m_profile->contains(STR_DI_Phone)) (*m_profile)[STR_DI_Phone]=QString();
        if (!m_profile->contains(STR_DI_Email)) (*m_profile)[STR_DI_Email]=QString();
        if (!m_profile->contains(STR_DI_Practice)) (*m_profile)[STR_DI_Practice]=QString();
        if (!m_profile->contains(STR_DI_Address)) (*m_profile)[STR_DI_Address]=QString();
        if (!m_profile->contains(STR_DI_PatientID)) (*m_profile)[STR_DI_PatientID]=QString();
    }
    ~DoctorInfo() {}
    void setProfile(Profile *p) { m_profile=p; }

    const QString name() { return (*m_profile)[STR_DI_Name].toString(); }
    const QString phone() { return (*m_profile)[STR_DI_Phone].toString(); }
    const QString email() { return (*m_profile)[STR_DI_Email].toString(); }
    const QString practiceName() { return (*m_profile)[STR_DI_Practice].toString(); }
    const QString address() { return (*m_profile)[STR_DI_Address].toString(); }
    const QString patientID() { return (*m_profile)[STR_DI_PatientID].toString(); }

    void setName(QString name) { (*m_profile)[STR_DI_Name]=name; }
    void setPhone(QString phone) { (*m_profile)[STR_DI_Phone]=phone; }
    void setEmail(QString phone) { (*m_profile)[STR_DI_Email]=phone; }
    void setPracticeName(QString practice) { (*m_profile)[STR_DI_Practice]=practice; }
    void setAddress(QString address) { (*m_profile)[STR_DI_Address]=address; }
    void setPatientID(QString pid) { (*m_profile)[STR_DI_PatientID]=pid; }

    Profile *m_profile;
};

extern const char * STR_UI_DOB;
extern const char * STR_UI_FirstName;
extern const char * STR_UI_LastName;
extern const char * STR_UI_UserName;
extern const char * STR_UI_Password;
extern const char * STR_UI_Address;
extern const char * STR_UI_Phone;
extern const char * STR_UI_EmailAddress;
extern const char * STR_UI_Country;
extern const char * STR_UI_Height;
extern const char * STR_UI_Gender;
extern const char * STR_UI_TimeZone;
extern const char * STR_UI_Language;
extern const char * STR_UI_DST;

/*! \class UserInfo
    \brief Profile Options relating to the User Information
    */
class UserInfo
{
public:
    //! \brief Create UserInfo object given Profile *p, and initialize the defaults
    UserInfo(Profile *p) : m_profile(p)
    {
        if (!m_profile->contains(STR_UI_DOB)) (*m_profile)[STR_UI_DOB]=QDate(1970,1,1);
        if (!m_profile->contains(STR_UI_FirstName)) (*m_profile)[STR_UI_FirstName]=QString();
        if (!m_profile->contains(STR_UI_LastName)) (*m_profile)[STR_UI_LastName]=QString();
        if (!m_profile->contains(STR_UI_UserName)) (*m_profile)[STR_UI_UserName]=QString();
        if (!m_profile->contains(STR_UI_Password)) (*m_profile)[STR_UI_Password]=QString();
        if (!m_profile->contains(STR_UI_Address)) (*m_profile)[STR_UI_Address]=QString();
        if (!m_profile->contains(STR_UI_Phone)) (*m_profile)[STR_UI_Phone]=QString();
        if (!m_profile->contains(STR_UI_EmailAddress)) (*m_profile)[STR_UI_EmailAddress]=QString();
        if (!m_profile->contains(STR_UI_Country)) (*m_profile)[STR_UI_Country]=QString();
        if (!m_profile->contains(STR_UI_Height)) (*m_profile)[STR_UI_Height]=0.0;
        if (!m_profile->contains(STR_UI_Gender)) (*m_profile)[STR_UI_Gender]=(int)GenderNotSpecified;
        if (!m_profile->contains(STR_UI_TimeZone)) (*m_profile)[STR_UI_TimeZone]=QString();
        if (!m_profile->contains(STR_UI_Language)) (*m_profile)[STR_UI_Language]="English";
        if (!m_profile->contains(STR_UI_DST)) (*m_profile)[STR_UI_DST]=false;

    }
    ~UserInfo() {}

    void setProfile(Profile *p) { m_profile=p; }

    QDate DOB() { return (*m_profile)[STR_UI_DOB].toDate(); }
    const QString firstName() { return (*m_profile)[STR_UI_FirstName].toString(); }
    const QString lastName() { return (*m_profile)[STR_UI_LastName].toString(); }
    const QString userName() { return (*m_profile)[STR_UI_UserName].toString(); }
    const QString address() { return (*m_profile)[STR_UI_Address].toString(); }
    const QString phone() { return (*m_profile)[STR_UI_Phone].toString(); }
    const QString email() { return (*m_profile)[STR_UI_EmailAddress].toString(); }
    double height() { return (*m_profile)[STR_UI_Height].toDouble(); }
    const QString country() { return (*m_profile)[STR_UI_Country].toString(); }
    Gender gender() { return (Gender)(*m_profile)[STR_UI_Gender].toInt(); }
    const QString timeZone() { return (*m_profile)[STR_UI_TimeZone].toString(); }
    const QString language() { return (*m_profile)[STR_UI_Language].toString(); }
    bool daylightSaving() { return (*m_profile)[STR_UI_DST].toBool(); }

    void setDOB(QDate date) { (*m_profile)[STR_UI_DOB]=date; }
    void setFirstName(QString name) { (*m_profile)[STR_UI_FirstName]=name; }
    void setLastName(QString name) { (*m_profile)[STR_UI_LastName]=name; }
    void setUserName(QString username) { (*m_profile)[STR_UI_UserName]=username; }
    void setAddress(QString address) { (*m_profile)[STR_UI_Address]=address; }
    void setPhone(QString phone) { (*m_profile)[STR_UI_Phone]=phone; }
    void setEmail(QString email) { (*m_profile)[STR_UI_EmailAddress]=email; }
    void setHeight(double height) { (*m_profile)[STR_UI_Height]=height; }
    void setCountry(QString country) { (*m_profile)[STR_UI_Country]=country; }
    void setGender(Gender g) { (*m_profile)[STR_UI_Gender]=(int)g; }
    void setTimeZone(QString tz) { (*m_profile)[STR_UI_TimeZone]=tz; }
    void setLanguage(QString language) { (*m_profile)[STR_UI_Language]=language; }
    void setDaylightSaving(bool ds) { (*m_profile)[STR_UI_DST]=ds; }

    bool hasPassword() {
        return !((*m_profile)[STR_UI_Password].toString().isEmpty());
    }
    bool checkPassword(QString password) {
        QByteArray ba=password.toUtf8();
        return ((*m_profile)[STR_UI_Password].toString()==QString(QCryptographicHash::hash(ba,QCryptographicHash::Sha1).toHex()));
    }
    void setPassword(QString password) {
        QByteArray ba=password.toUtf8();
        // Hash me.
        (*m_profile)[STR_UI_Password]=QString(QCryptographicHash::hash(ba,QCryptographicHash::Sha1).toHex());
    }

    Profile *m_profile;
};

extern const char * STR_OS_EnableOximetry;
extern const char * STR_OS_SyncOximetry;
extern const char * STR_OS_OximeterType;
extern const char * STR_OS_OxiDiscardThreshold;
extern const char * STR_OS_SPO2DropDuration;
extern const char * STR_OS_SPO2DropPercentage;
extern const char * STR_OS_PulseChangeDuration;
extern const char * STR_OS_PulseChangeBPM;

/*! \class OxiSettings
    \brief Profile Options relating to the Oximetry settings
    */
class OxiSettings
{
public:
    //! \brief Create OxiSettings object given Profile *p, and initialize the defaults
    OxiSettings(Profile *p) :m_profile(p)
    {
        if (!m_profile->contains(STR_OS_EnableOximetry)) (*m_profile)[STR_OS_EnableOximetry]=false;
        if (!m_profile->contains(STR_OS_SyncOximetry)) (*m_profile)[STR_OS_SyncOximetry]=true;
        if (!m_profile->contains(STR_OS_OximeterType)) (*m_profile)[STR_OS_OximeterType]="CMS50";
        if (!m_profile->contains(STR_OS_OxiDiscardThreshold)) (*m_profile)[STR_OS_OxiDiscardThreshold]=0.0;
        if (!m_profile->contains(STR_OS_SPO2DropDuration)) (*m_profile)[STR_OS_SPO2DropDuration]=8.0;
        if (!m_profile->contains(STR_OS_SPO2DropPercentage)) (*m_profile)[STR_OS_SPO2DropPercentage]=3.0;
        if (!m_profile->contains(STR_OS_PulseChangeDuration)) (*m_profile)[STR_OS_PulseChangeDuration]=8.0;
        if (!m_profile->contains(STR_OS_PulseChangeBPM)) (*m_profile)[STR_OS_PulseChangeBPM]=5.0;
    }
    ~OxiSettings() {}

    void setProfile(Profile *p) { m_profile=p; }

    bool oximetryEnabled()  { return (*m_profile)[STR_OS_EnableOximetry].toBool(); }
    bool syncOximetry() { return (*m_profile)[STR_OS_SyncOximetry].toBool(); }
    QString oximeterType() { return (*m_profile)[STR_OS_OximeterType].toString(); }
    double oxiDiscardThreshold() { return (*m_profile)[STR_OS_OxiDiscardThreshold].toDouble(); }
    double spO2DropDuration() { return (*m_profile)[STR_OS_SPO2DropDuration].toDouble(); }
    double spO2DropPercentage() { return (*m_profile)[STR_OS_SPO2DropPercentage].toDouble(); }
    double pulseChangeDuration() { return (*m_profile)[STR_OS_PulseChangeDuration].toDouble(); }
    double pulseChangeBPM() { return (*m_profile)[STR_OS_PulseChangeBPM].toDouble(); }

    void setOximetryEnabled(bool enabled)  { (*m_profile)[STR_OS_EnableOximetry]=enabled; }
    void setSyncOximetry(bool synced) { (*m_profile)[STR_OS_SyncOximetry]=synced; }
    void setOximeterType(QString oxitype) { (*m_profile)[STR_OS_OximeterType]=oxitype; }
    void setOxiDiscardThreshold(double thresh) { (*m_profile)[STR_OS_OxiDiscardThreshold]=thresh; }
    void setSpO2DropDuration(double duration) { (*m_profile)[STR_OS_SPO2DropDuration]=duration; }
    void setSpO2DropPercentage(double percentage) { (*m_profile)[STR_OS_SPO2DropPercentage]=percentage; }
    void setPulseChangeDuration(double duration) { (*m_profile)[STR_OS_PulseChangeDuration]=duration; }
    void setPulseChangeBPM(double bpm) { (*m_profile)[STR_OS_PulseChangeBPM]=bpm; }

    Profile *m_profile;
};

extern const char * STR_CS_ComplianceHours;
extern const char * STR_CS_ShowCompliance;
extern const char * STR_CS_ShowLeaksMode;
extern const char * STR_CS_MaskStartDate;
extern const char * STR_CS_MaskDescription;
extern const char * STR_CS_MaskType;
extern const char * STR_CS_PrescribedMode;
extern const char * STR_CS_PrescribedMinPressure;
extern const char * STR_CS_PrescribedMaxPressure;
extern const char * STR_CS_UntreatedAHI;
extern const char * STR_CS_Notes;
extern const char * STR_CS_DateDiagnosed;

/*! \class CPAPSettings
    \brief Profile Options relating to the CPAP settings
    */
class CPAPSettings
{
public:
    //! \brief Create CPAPSettings object given Profile *p, and initialize the defaults
    CPAPSettings(Profile *p) :m_profile(p)
    {
        if (!m_profile->contains(STR_CS_ComplianceHours)) (*m_profile)[STR_CS_ComplianceHours]=4;
        if (!m_profile->contains(STR_CS_ShowCompliance)) (*m_profile)[STR_CS_ShowCompliance]=true;
        if (!m_profile->contains(STR_CS_ShowLeaksMode)) (*m_profile)[STR_CS_ShowLeaksMode]=0;
        // TODO: Check if this date is initiliazed yet
        if (!m_profile->contains(STR_CS_MaskStartDate)) (*m_profile)[STR_CS_MaskStartDate]=QDate();
        if (!m_profile->contains(STR_CS_MaskDescription)) (*m_profile)[STR_CS_MaskDescription]=QString();
        if (!m_profile->contains(STR_CS_MaskType)) (*m_profile)[STR_CS_MaskType]=Mask_Unknown;
        if (!m_profile->contains(STR_CS_PrescribedMode)) (*m_profile)[STR_CS_PrescribedMode]=MODE_UNKNOWN;
        if (!m_profile->contains(STR_CS_PrescribedMinPressure)) (*m_profile)[STR_CS_PrescribedMinPressure]=0.0;
        if (!m_profile->contains(STR_CS_PrescribedMaxPressure)) (*m_profile)[STR_CS_PrescribedMaxPressure]=0.0;
        if (!m_profile->contains(STR_CS_UntreatedAHI)) (*m_profile)[STR_CS_UntreatedAHI]=0.0;
        if (!m_profile->contains(STR_CS_Notes)) (*m_profile)[STR_CS_Notes]=QString();
        if (!m_profile->contains(STR_CS_DateDiagnosed)) (*m_profile)[STR_CS_DateDiagnosed]=QDate();
    }

    ~CPAPSettings() {}

    void setProfile(Profile *p) { m_profile=p; }

    //Getters
    double complianceHours() { return (*m_profile)[STR_CS_ComplianceHours].toDouble(); }
    bool showComplianceInfo() { return (*m_profile)[STR_CS_ShowCompliance].toBool(); }
    int leakMode() { return (*m_profile)[STR_CS_ShowLeaksMode].toInt(); }
    QDate maskStartDate() { return (*m_profile)[STR_CS_MaskStartDate].toDate(); }
    QString maskDescription() { return (*m_profile)[STR_CS_MaskDescription].toString(); }
    MaskType maskType() { return (MaskType)(*m_profile)[STR_CS_MaskType].toInt(); }
    CPAPMode mode() { return CPAPMode((*m_profile)[STR_CS_PrescribedMode].toInt()); }
    double minPressure() { return (*m_profile)[STR_CS_PrescribedMinPressure].toDouble(); }
    double maxPressure() { return (*m_profile)[STR_CS_PrescribedMaxPressure].toDouble(); }
    double untreatedAHI() { return (*m_profile)[STR_CS_UntreatedAHI].toDouble(); }
    const QString notes() { return (*m_profile)[STR_CS_Notes].toString(); }
    QDate dateDiagnosed() { return (*m_profile)[STR_CS_DateDiagnosed].toDate(); }

    //Setters
    void setMode(CPAPMode mode) { (*m_profile)[STR_CS_PrescribedMode]=(int)mode; }
    void setMinPressure(double pressure) { (*m_profile)[STR_CS_PrescribedMinPressure]=pressure; }
    void setMaxPressure(double pressure) { (*m_profile)[STR_CS_PrescribedMaxPressure]=pressure; }
    void setUntreatedAHI(double ahi) { (*m_profile)[STR_CS_UntreatedAHI]=ahi; }
    void setNotes(QString notes) { (*m_profile)[STR_CS_Notes]=notes; }
    void setDateDiagnosed(QDate date) { (*m_profile)[STR_CS_DateDiagnosed]=date; }
    void setComplianceHours(double hours) { (*m_profile)[STR_CS_ComplianceHours]=hours; }
    void setShowComplianceInfo(bool b) { (*m_profile)[STR_CS_ShowCompliance]=b; }
    void setLeakMode(int leakmode) { (*m_profile)[STR_CS_ShowLeaksMode]=(int)leakmode; }
    void setMaskStartDate(QDate date) { (*m_profile)[STR_CS_MaskStartDate]=date; }
    void setMaskDescription(QString description) { (*m_profile)[STR_CS_MaskDescription]=description; }
    void setMaskType(MaskType masktype) { (*m_profile)[STR_CS_MaskType]=(int)masktype; }

    Profile *m_profile;
};

extern const char * STR_IS_DaySplitTime;
extern const char * STR_IS_CacheSessions;
extern const char * STR_IS_CombineCloseSessions;
extern const char * STR_IS_IgnoreShorterSessions;
extern const char * STR_IS_Multithreading;
extern const char * STR_IS_BackupCardData;
extern const char * STR_IS_CompressBackupData;
extern const char * STR_IS_CompressSessionData;

/*! \class ImportSettings
    \brief Profile Options relating to the Import process
    */
class SessionSettings
{
public:
    //! \brief Create ImportSettings object given Profile *p, and initialize the defaults
    SessionSettings(Profile *p) :m_profile(p)
    {
        if (!m_profile->contains(STR_IS_DaySplitTime)) (*m_profile)[STR_IS_DaySplitTime]=QTime(12,0,0);
        if (!m_profile->contains(STR_IS_CacheSessions)) (*m_profile)[STR_IS_CacheSessions]=false;
        if (!m_profile->contains(STR_IS_CombineCloseSessions)) (*m_profile)[STR_IS_CombineCloseSessions]=240;
        if (!m_profile->contains(STR_IS_IgnoreShorterSessions)) (*m_profile)[STR_IS_IgnoreShorterSessions]=5;
        if (!m_profile->contains(STR_IS_Multithreading)) (*m_profile)[STR_IS_Multithreading]=QThread::idealThreadCount() > 1;
        if (!m_profile->contains(STR_IS_BackupCardData)) (*m_profile)[STR_IS_BackupCardData]=true;
        if (!m_profile->contains(STR_IS_CompressBackupData)) (*m_profile)[STR_IS_CompressBackupData]=false;
        if (!m_profile->contains(STR_IS_CompressSessionData)) (*m_profile)[STR_IS_CompressSessionData]=false;
    }
    ~SessionSettings() {}

    void setProfile(Profile *p) { m_profile=p; }

    QTime daySplitTime() { return (*m_profile)[STR_IS_DaySplitTime].toTime(); }
    bool cacheSessions() { return (*m_profile)[STR_IS_CacheSessions].toBool(); }
    double combineCloseSessions() { return (*m_profile)[STR_IS_CombineCloseSessions].toDouble(); }
    double ignoreShortSessions() { return (*m_profile)[STR_IS_IgnoreShorterSessions].toDouble(); }
    bool multithreading() { return (*m_profile)[STR_IS_Multithreading].toBool(); }
    bool compressSessionData() { return (*m_profile)[STR_IS_CompressSessionData].toBool(); }
    bool compressBackupData() { return (*m_profile)[STR_IS_CompressBackupData].toBool(); }
    bool backupCardData() { return (*m_profile)[STR_IS_BackupCardData].toBool(); }

    void setDaySplitTime(QTime time) { (*m_profile)[STR_IS_DaySplitTime]=time; }
    void setCacheSessions(bool c) { (*m_profile)[STR_IS_CacheSessions]=c; }
    void setCombineCloseSessions(double val) { (*m_profile)[STR_IS_CombineCloseSessions]=val; }
    void setIgnoreShortSessions(double val) { (*m_profile)[STR_IS_IgnoreShorterSessions]=val; }
    void setMultithreading(bool enabled) { (*m_profile)[STR_IS_Multithreading]=enabled; }
    void setBackupCardData(bool enabled) { (*m_profile)[STR_IS_BackupCardData]=enabled; }
    void setCompressBackupData(bool enabled) { (*m_profile)[STR_IS_CompressBackupData]=enabled; }
    void setCompressSessionData(bool enabled) { (*m_profile)[STR_IS_CompressSessionData]=enabled; }

    Profile *m_profile;
};

extern const char * STR_AS_GraphHeight;
extern const char * STR_AS_AntiAliasing;
extern const char * STR_AS_GraphSnapshots;
extern const char * STR_AS_Animations;
extern const char * STR_AS_SquareWave;
extern const char * STR_AS_OverlayType;

/*! \class AppearanceSettings
    \brief Profile Options relating to Visual Appearance
    */
class AppearanceSettings
{
public:
    //! \brief Create AppearanceSettings object given Profile *p, and initialize the defaults
    AppearanceSettings(Profile *p) :m_profile(p)
    {
        if (!m_profile->contains(STR_AS_GraphHeight)) (*m_profile)[STR_AS_GraphHeight]=180.0;
        if (!m_profile->contains(STR_AS_AntiAliasing)) (*m_profile)[STR_AS_AntiAliasing]=false; // i think it's ugly
        if (!m_profile->contains(STR_AS_GraphSnapshots)) (*m_profile)[STR_AS_GraphSnapshots]=true;
        if (!m_profile->contains(STR_AS_Animations)) (*m_profile)[STR_AS_Animations]=true;
        if (!m_profile->contains(STR_AS_SquareWave)) (*m_profile)[STR_AS_SquareWave]=false;
        if (!m_profile->contains(STR_AS_OverlayType)) (*m_profile)[STR_AS_OverlayType]=ODT_Bars;
    }
    ~AppearanceSettings() {}

    void setProfile(Profile *p) { m_profile=p; }

    //! \brief Returns the normal (unscaled) height of a graph
    int graphHeight() { return (*m_profile)[STR_AS_GraphHeight].toInt(); }
    //! \brief Returns true if AntiAliasing (the graphical smoothing method) is enabled
    bool antiAliasing() { return (*m_profile)[STR_AS_AntiAliasing].toBool(); }
    //! \brief Returns true if renderPixmap function is in use, which takes snapshots of graphs
    bool graphSnapshots() { return (*m_profile)[STR_AS_GraphSnapshots].toBool(); }
    //! \brief Returns true if Graphical animations & Transitions will be drawn
    bool animations() { return (*m_profile)[STR_AS_Animations].toBool(); }
    //! \brief Returns true if Square Wave plots are preferred (where possible)
    bool squareWavePlots() { return (*m_profile)[STR_AS_SquareWave].toBool(); }
    //! \brief Returns the type of overlay flags (which are displayed over the Flow Waveform)
    OverlayDisplayType overlayType() { return (OverlayDisplayType )(*m_profile)[STR_AS_OverlayType].toInt(); }

    //! \brief Set the normal (unscaled) height of a graph.
    void setGraphHeight(int height) { (*m_profile)[STR_AS_GraphHeight]=height; }
    //! \brief Set to true to turn on AntiAliasing (the graphical smoothing method)
    void setAntiAliasing(bool aa) { (*m_profile)[STR_AS_AntiAliasing]=aa; }
    //! \brief Set to true if renderPixmap functions are in use, which takes snapshots of graphs.
    void setGraphSnapshots(bool gs) { (*m_profile)[STR_AS_GraphSnapshots]=gs; }
    //! \brief Set to true if Graphical animations & Transitions will be drawn
    void setAnimations(bool anim) { (*m_profile)[STR_AS_Animations]=anim; }
    //! \brief Set whether or not to useSquare Wave plots (where possible)
    void setSquareWavePlots(bool sw) { (*m_profile)[STR_AS_SquareWave]=sw; }
    //! \brief Sets the type of overlay flags (which are displayed over the Flow Waveform)
    void setOverlayType(OverlayDisplayType od) { (*m_profile)[STR_AS_OverlayType]=(int)od; }

    Profile *m_profile;
};

extern const char * STR_US_UnitSystem;
extern const char * STR_US_EventWindowSize;
extern const char * STR_US_SkipEmptyDays;
extern const char * STR_US_RebuildCache;
extern const char * STR_US_ShowDebug;
extern const char * STR_US_LinkGroups;
extern const char * STR_US_CalculateRDI;
extern const char * STR_US_ShowSerialNumbers;

/*! \class UserSettings
    \brief Profile Options relating to General User Settings
    */
class UserSettings
{
public:
    //! \brief Create UserSettings object given Profile *p, and initialize the defaults
    UserSettings(Profile *p) :m_profile(p)
    {
        if (!m_profile->contains(STR_US_UnitSystem)) (*m_profile)[STR_US_UnitSystem]=US_Metric;
        if (!m_profile->contains(STR_US_EventWindowSize)) (*m_profile)[STR_US_EventWindowSize]=4.0;
        if (!m_profile->contains(STR_US_SkipEmptyDays)) (*m_profile)[STR_US_SkipEmptyDays]=true;
        if (!m_profile->contains(STR_US_RebuildCache)) (*m_profile)[STR_US_RebuildCache]=false; // can't remember..
        if (!m_profile->contains(STR_US_ShowDebug)) (*m_profile)[STR_US_ShowDebug]=false;
        if (!m_profile->contains(STR_US_LinkGroups)) (*m_profile)[STR_US_LinkGroups]=true; // can't remember..
        if (!m_profile->contains(STR_US_CalculateRDI)) (*m_profile)[STR_US_CalculateRDI]=false;
        if (!m_profile->contains(STR_US_ShowSerialNumbers)) (*m_profile)[STR_US_ShowSerialNumbers]=false;
    }
    ~UserSettings() {}

    void setProfile(Profile *p) { m_profile=p; }

    UnitSystem unitSystem() { return (UnitSystem)(*m_profile)[STR_US_UnitSystem].toInt(); }
    double eventWindowSize() { return (*m_profile)[STR_US_EventWindowSize].toDouble(); }
    bool skipEmptyDays() { return (*m_profile)[STR_US_SkipEmptyDays].toBool(); }
    bool rebuildCache() { return (*m_profile)[STR_US_RebuildCache].toBool(); }
    bool showDebug() { return (*m_profile)[STR_US_ShowDebug].toBool(); }
    bool linkGroups() { return (*m_profile)[STR_US_LinkGroups].toBool(); }
    bool calculateRDI() { return (*m_profile)[STR_US_CalculateRDI].toBool(); }
    bool showSerialNumbers() { return (*m_profile)[STR_US_ShowSerialNumbers].toBool(); }

    void setUnitSystem(UnitSystem us) { (*m_profile)[STR_US_UnitSystem]=(int)us; }
    void setEventWindowSize(double size) { (*m_profile)[STR_US_EventWindowSize]=size; }
    void setSkipEmptyDays(bool skip) { (*m_profile)[STR_US_SkipEmptyDays]=skip; }
    void setRebuildCache(bool rebuild) { (*m_profile)[STR_US_RebuildCache]=rebuild; }
    void setShowDebug(bool b) { (*m_profile)[STR_US_ShowDebug]=b; }
    void setLinkGroups(bool link) { (*m_profile)[STR_US_LinkGroups]=link; }
    void setCalculateRDI(bool rdi) { (*m_profile)[STR_US_CalculateRDI]=rdi; }
    void setShowSerialNumbers(bool enabled) { (*m_profile)[STR_US_ShowSerialNumbers]=enabled; }

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

