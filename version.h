#ifndef VERSION_H
#define VERSION_H

#include <qglobal.h>

const int major_version=0;
const int minor_version=9;
const int revision_number=3;
const int release_number=0;

const QString VersionString=QString().sprintf("%i.%i.%i",major_version,minor_version,revision_number);
const QString FullVersionString=QString().sprintf("%i.%i.%i-%i",major_version,minor_version,revision_number,release_number);

const QString ReleaseStatus="beta";

#ifdef Q_OS_MAC
    const QString PlatformString="MacOSX";
#elif defined(Q_WS_WIN32)
    const QString PlatformString="Win32";
#elif defined(Q_WS_X11)
    const QString PlatformString="Linux";
#endif

#endif // VERSION_H
