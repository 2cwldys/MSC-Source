//When defined, DEV_BUILD indicates that this is
//a MSDev version of MS.  Certain features are allowed in this mode
//New: This is now defined in the project instead of a file
//#define DEV_BUILD

//When defined, the client and server dlls will maintain txt logfiles
#define KEEP_LOG

//Allows exceptions to be logged to both console and the logfile
#define LOG_EXCEPTIONS
//!!!EXTENSIVE_LOGGING has MOVED!! - Look at the top of Global.cpp!
//#define EXTENSIVE_LOGGING		//Causes EXTENSIVE logging of every dbg operation


#ifdef EXTENSIVE_LOGGING
	#define LOG_INPUTS				//Automatic, with extensive logging
#endif

//Causes DbgInputs() to be called for each DLL input function in the client and server dlls
#define TRACK_INPUTS
//#define LOG_INPUTS				//Extensive logging of inputs to file


//In the release build, whether certain errors treated as fatal (release) or not (debug)
#define RELEASE_LOCKDOWN

#ifdef RELEASE_LOCKDOWN
	#define SCRIPT_LOCKDOWN				//Automatic, with lockdown mode
#endif


//Encrypt memory
#define MEM_ENCRYPT
