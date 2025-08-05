#pragma once

#if   _WIN32 
	#define MAXIMUM_PATH_LENGTH 260
	#include <windows.h>
#elif __linux__
	#define MAXIMUM_PATH_LENGTH 4096
	#include <unistd.h>
	#include <dirent.h>
#elif __APPLE__
	#define MAXIMUM_PATH_LENGTH 1024
	#include <unistd.h>
	#include <dirent.h>
#endif

// True for Windows, Linux, and Mac OS X
#define MAXIMUM_FILE_LENGTH 255

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
