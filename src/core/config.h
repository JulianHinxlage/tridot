//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

//platform
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
	#define TRI_WINDOWS 1
#elif __APPLE__
	#define TRI_MACOS 1
#elif __linux__
	#define TRI_LINUX 1
#elif __unix__
	#define TRI_UNIX 1
#elif defined(_POSIX_VERSION)
	#define TRI_POSIX 1
#endif

//build mode
#if not defined(TRI_DEBUG) && not defined(TRI_RELEASE) && not defined(TRI_DISTRIBUTION)
#define TRI_DEBUG 1
#endif

//tri api
#if TRI_WINDOWS
	#ifdef TRI_DLL_EXPORT
		#define TRI_API __declspec(dllexport)
	#else
		#define TRI_API __declspec(dllimport)
	#endif
#else
	#define TRI_API
#endif

//windows header
#if TRI_WINDOWS
	#include <windows.h>
	#undef ERROR
	#undef min
	#undef max
	#undef near
	#undef far
#endif

//version
#define TRI_VERSION_MAJOR 0
#define TRI_VERSION_MINOR 1
#define TRI_VERSION_PATH 0
#define TRI_VERSION "0.1.0-dev"


