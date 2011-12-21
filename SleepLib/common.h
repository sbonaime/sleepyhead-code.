#ifndef COMMON_H
#define COMMON_H

#include <QString>

enum UnitSystem { US_Undefined, US_Metric, US_Archiac };

UnitSystem unitSystem(bool reset=true);

const float ounce_convert=28.3495231; // grams
const float pound_convert=ounce_convert*16;

QString weightString(float kg, UnitSystem us);




#endif // COMMON_H
