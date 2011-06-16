#ifndef VERSION_H
#define VERSION_H

namespace AutoVersion{
	
	//Date Version Types
	static const char  _DATE[] = "17";
	static const char  _MONTH[] = "06";
	static const char  _YEAR[] = "2011";
	static const char  _UBUNTU_VERSION_STYLE[] = "11.06";
	
	//Software Status
	static const char  _STATUS[] = "Alpha";
	static const char  _STATUS_SHORT[] = "a";
	
	//Standard Version Type
	static const long  _MAJOR = 0;
	static const long  _MINOR = 7;
	static const long  _BUILD = 6390;
	static const long  _REVISION = 18183;
	
	//Miscellaneous Version Types
	static const long  _BUILDS_COUNT = 7453;
	#define  _RC_FILEVERSION 0,7,6390,18183
	#define  _RC_FILEVERSION_STRING "0, 7, 6390, 18183\0"
	static const char  _FULLVERSION_STRING[] = "0.7.6390.18183";
	
	//These values are to keep track of your versioning state, don't modify them.
	static const long  _BUILD_HISTORY = 0;
	

}
#endif //VERSION_H
