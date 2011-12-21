#ifndef COMMON_H
#define COMMON_H

#include <QString>
#include <QObject>

enum UnitSystem { US_Undefined, US_Metric, US_Archiac };

const float ounce_convert=28.3495231; // grams
const float pound_convert=ounce_convert*16;

QString weightString(float kg, UnitSystem us);

const QString STR_UNIT_CM=QObject::tr("cm");
const QString STR_UNIT_INCH=QObject::tr("\"");
const QString STR_UNIT_FOOT=QObject::tr("ft");
const QString STR_UNIT_POUND=QObject::tr("lb");
const QString STR_UNIT_OUNCE=QObject::tr("oz");
const QString STR_UNIT_KG=QObject::tr("Kg");

const QString STR_MESSAGE_ERROR=QObject::tr("Error");
const QString STR_MESSAGE_WARNING=QObject::tr("Warning");



#endif // COMMON_H
