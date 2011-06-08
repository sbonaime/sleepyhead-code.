#ifndef VERSION_H
#define VERSION_H

namespace AutoVersion{
	
	//Date Version Types
	static const char DATE[] = "09";
	static const char MONTH[] = "06";
	static const char YEAR[] = "2011";
	static const char UBUNTU_VERSION_STYLE[] = "11.06";
	
	//Software Status
	static const char STATUS[] = "Alpha";
	static const char STATUS_SHORT[] = "a";
	
	//Standard Version Type
	static const long MAJOR = 0;
	static const long MINOR = 7;
	static const long BUILD = 3843;
	static const long REVISION = 4191;
	
	//Miscellaneous Version Types
	static const long BUILDS_COUNT = 1848;
	#define RC_FILEVERSION 0,7,3843,4191
	#define RC_FILEVERSION_STRING "0, 7, 3843, 4191\0"
	static const char FULLVERSION_STRING[] = "0.7.3843.4191";
	
	//These values are to keep track of your versioning state, don't modify them.
	static const long BUILD_HISTORY = 0;
	

}
#endif //VERSION_H
