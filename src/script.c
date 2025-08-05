#include "script.h"
#include "../stb_image/stb_image.h"
#include <string.h>


enum return_codes
{
	INVALID_PATH,
	INVALID_FILE,
	PATH_EXISTS,
	FILE_EXISTS,
	BOOM_BOOM_ERROR
};


void hang()
{
	printf("Press ENTER to quit: ");
	getchar();
	return;
}


// FUCK this im wasting all my time doing cross platform shit and not doing
// actual shit what the FUCK
int check_valid(char* path, char* file_name)
{
#if _WIN32
	WIN32_FIND_DATAW file_data;
	HANDLE           file_handle;
	wchar_t          w_path     [MAXIMUM_PATH_LENGTH];
	wchar_t          search_path[MAXIMUM_PATH_LENGTH];
	wchar_t          w_file_name[MAXIMUM_FILE_LENGTH];

	if (!MultiByteToWideChar(
		CP_UTF8,    
		0,          // Must be 0 for UTF8 (i dont know why, i skimmed the documentations)
		path,
		-1,         // String 2b converted is null terminated
		w_path,
		MAXIMUM_PATH_LENGTH))
	{
		printf("Failed conversion of path from ANSI to Unicode\n");
		return BOOM_BOOM_ERROR;
	}

	if(!MultiByteToWideChar(
		CP_UTF8,
		0,          // must be zewo
		file_name,
		-1,         // string 2b converted is null terminated da
		w_file_name,
		MAXIMUM_FILE_LENGTH))
	{
		printf("Failed conversion of file_name from ANSI to Unicode\n");
		return BOOM_BOOM_ERROR;
	}

	if(_snwprintf(
		search_path,
		MAXIMUM_PATH_LENGTH,
		L"%s\\*",
		w_path) == -1)
	{
		printf("Failed creation of search path\n");
		return BOOM_BOOM_ERROR;
	}


	file_handle = FindFirstFileW(search_path, &file_data);
	if (file_handle == INVALID_HANDLE_VALUE)
	{
		DWORD error = GetLastError();

		if (error == ERROR_PATH_NOT_FOUND)
		{
			printf("Path not found.\n");
			return BOOM_BOOM_ERROR;
		}

		if (error == ERROR_ACCESS_DENIED)
		{
			printf("Access denied to path: %s", path);
			return BOOM_BOOM_ERROR;
		}
	}

	do
	{
		wchar_t* file_name = file_data.cFileName;
		if (wcscmp(file_name, w_file_name) == 0)
		{
			FindClose(file_handle);
			return FILE_EXISTS;
		}
	} while (FindNextFileW(file_handle, &file_data));

	printf("File not found.\n");
	FindClose(file_handle);
	return BOOM_BOOM_ERROR;

#endif
}


int main(int argc, char* argv[])
{
	if (argc != 1)
	{
		printf("This script doesn't take any CLI arguments.\n");
		hang();
		exit(-1);
	}

	// Get path (  ≧ᗜ≦)
	printf("Enter the absolute / relative path of image: ");
	char path[MAXIMUM_PATH_LENGTH];

	if(!(fgets(
		path,
		MAXIMUM_PATH_LENGTH,
		stdin)))
	{
		printf("_fgetts() failed in _tmain() for path\n");
		exit(-1);
	}

	// Get file name ᓚ₍ ^. .^₎
	printf("Enter the file name: ");
	char file_name[MAXIMUM_FILE_LENGTH];

	if(!(fgets(
		file_name,
		MAXIMUM_FILE_LENGTH,
		stdin)))
	{
		printf("_fgetts() failed in _tmain() for file_name\n");
		exit(-1);
	}


	// haha get truncated dumb new line character from fgets I HATE YOUUUUUUUUUUUUU
	path     [strcspn(path, "\n")]      = '\0';
	file_name[strcspn(file_name, "\n")] = '\0';


	// Check if path and file is valid
	int return_value = 0;
	return_value = check_valid(path, file_name);
	// Do nothing if an error occured because the function
	// would've already printed out the reason for the error
	if (return_value == BOOM_BOOM_ERROR)
	{
		hang();
		exit(-1);
	}

	printf("Testing path: %s\n", path);
	printf("Testing file: %s\n", file_name);

	const char* density;
	density = strdup("$@B%8&WM#*oahkbdpqwmZO0QLCJUYXzcvunxrjft/\\|()1{}[]?-_+~<>i!lI;:,\"^`'. ");


	free((void*)density);
	hang();
	return 0;
}