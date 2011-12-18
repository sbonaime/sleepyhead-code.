#ifndef VERSION_H
#define VERSION_H

#include <qglobal.h>

const int major_version=0;
const int minor_version=8;
const int revision_number=8;

inline QString VersionString() { return QString().sprintf("%i.%i.%i",major_version,minor_version,revision_number); }

#ifdef Q_WS_MAC
    const QString PlatformString="MacOSX";
#elif defined(Q_WS_WIN32)
    const QString PlatformString="Win32";
#elif defined(Q_WS_X11)
    const QString PlatformString="Linux";
#endif

#endif // VERSION_H
