#ifndef VERSION_H
#define VERSION_H

namespace AutoVersion{
	
	//Date Version Types
	static const char DATE[] = "28";
	static const char MONTH[] = "05";
	static const char YEAR[] = "2011";
	static const char UBUNTU_VERSION_STYLE[] = "11.05";
	
	//Software Status
	static const char STATUS[] = "Alpha";
	static const char STATUS_SHORT[] = "a";
	
	//Standard Version Type
	static const long MAJOR = 0;
	static const long MINOR = 7;
	static const long BUILD = 1406;
	static const long REVISION = 2152;
	
	//Miscellaneous Version Types
	static const long BUILDS_COUNT = 5159;
	#define RC_FILEVERSION 0,7,1406,2152
	#define RC_FILEVERSION_STRING "0, 7, 1406, 2152\0"
	static const char FULLVERSION_STRING[] = "0.7.1406.2152";
	
	//These values are to keep track of your versioning state, don't modify them.
	static const long BUILD_HISTORY = 62;
	

}
#endif //VERSION_H
