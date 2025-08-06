#include "script.h"
#include "../stb_image/stb_image.h"


enum return_codes
{
	IMAGE_OK,
	IMAGE_TOO_LARGE,
	INVALID_PATH,
	INVALID_FILE,
	PATH_EXISTS,
	FILE_EXISTS,
	BOOM_BOOM_ERROR
};


typedef enum background_color
{
	WHITE,
	BLACK
} BACKGROUND_COLOR;


typedef enum file_types
{
	NO_EXTENSION,

	// sRGB
	JPEG, 

	// ASSUMED sRGB
	BMP,
	TGA,
	PSD,
	PNM,
	PNG,

	// Linear RGB
	HDR,
	PIC

} FILE_EXTENSION;


typedef struct
{
	const char*     file_extension;
	enum file_types type;

} EXTENSION_TABLE;


void hang()
{
	int lebron_james;

	printf("Press ENTER to quit: ");

	// first while loop consumes all the '\n's (or basically leftover until EOF) in stdout because APPARENLTY GETCHAR() INSTANTLY
	// QUITS IF THERES A NEW LINE CHAR IN THE STDOUT STREAM AND FFLUSH DOESNT DO ANYTHINGGGGGGGGG
	while ((lebron_james = getchar()) != '\n'
		|| lebron_james != EOF)

	while ((lebron_james = getchar()) != '\n'
		|| lebron_james != EOF)

	return;
}


// FUCK this im wasting all my time doing cross platform shit and not doing
// actual shit what the FUCK and i dont REALLY need to do this but like idk
// if i wanna generate custom error responses then i GOTTA cuz
// stbi_load() only returns one and only one error reponse if it cant open an img
int check_valid(char* path, char* file_name)
{
#if _WIN32
	WIN32_FIND_DATAW file_data;
	HANDLE           file_handle;
	size_t           path_length;
	HRESULT          result;
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

	result = StringCchLengthW(
		w_path,
		MAXIMUM_PATH_LENGTH,
		&path_length);

	if (FAILED(result))
	{
		printf("StringCchLengthW failed.\n");
		hang();
		exit(-1);
	}

	if (path_length > 0
		&& w_path[path_length - 1] == L'\\')
	{
		w_path[path_length - 1] = L'\0';

		// path (string) should be passed by reference
		// since it's stack allocated, so ya.
		path[strlen(path) - 1] = '\0';
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


// now begin the MESS that is posix. ok but 2b fair 
// im just more familiar with the winapi.
#elif defined(__linux__) || defined (__APPLE__)
	struct dirent* directory_entry;
	DIR*           directory_pointer;
	char           entry_full_path[MAXIMUM_PATH_LENGTH];
	int            path_length;

	path_length = strnlen(
		path,
		MAXIMUM_PATH_LENGTH);

	if (path[path_length - 1] == '/')
		path[path_length - 1] = '\0';

	directory_pointer = opendir(path);
	if (!directory_pointer)
	{
		printf("An error occured while trying to open path: %s\n", path);
		return BOOM_BOOM_ERROR;
	}

	while ((directory_entry = readdir(directory_pointer)))
	{
		struct stat what_the_fuckkk;

		// yes i know the logic error here, of potentionally allowing a directory
		// to go thru as valid. but this allows me to 
		// have three separate errors: Path not found, File not found, and
		// Failed to open (from attempting to load it with stbi_load())
		if (strcmp(file_name, directory_entry->d_name) == 0)
		{
			closedir(directory_pointer);
			return FILE_EXISTS;
		}

		// yes, everything underneath this comment is redundant. but i wanted
		// to work with posix directory stuff, so like, thug it out. program'll
		// only run 0.00000000000000000000003 (guesstimate) seconds slower.

		snprintf(
			entry_full_path,
			MAXIMUM_PATH_LENGTH,
			"%s/%s",
			path,
			directory_entry->d_name);

		if (stat(entry_full_path, &what_the_fuckkk) == -1)
		{
			printf("Unable to stat file: %s\n", entry_full_path);
			continue;
		}

		if ((what_the_fuckkk.st_mode & S_IFMT) == S_IFDIR)
			continue;
	}

	closedir(directory_pointer);
	printf("File not found.\n");
	return BOOM_BOOM_ERROR;

#endif
}



int enforce_console_resolution(int img_width, int img_height)
{
#if _WIN32
	int                        cmd_width;
	int                        cmd_height;
	CONSOLE_SCREEN_BUFFER_INFO cmd_info;

	GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &cmd_info);

	cmd_width  = cmd_info.srWindow.Right  - cmd_info.srWindow.Left + 1;
	cmd_height = cmd_info.srWindow.Bottom - cmd_info.srWindow.Top  + 1;

	if (img_width > cmd_width
		|| img_height > cmd_height)
	{ 
		printf("The resolution of your image is %d x %d.\n",              img_width, img_height);
		printf("The textual resolution of this CMD window is %d x %d.\n", cmd_width, cmd_height);
		printf("Please lower your image resolution to fit your CMD window or increase the size of your CMD window.\n");
		return IMAGE_TOO_LARGE;
	}

	return IMAGE_OK;

#elif __defined(__linux__) || __defined(__APPLE__)

	int            console_width;
	int            console_height;
	struct winsize console_info;

	ioctl(
		STDOUT_FILENO,
		TIOCGWINSZ,
		&console_info);

	console_width  = console_info.ws_col;
	console_height = console_info.ws_row;

	if (img_width > console_width
		|| img_height > console_height)
	{
		printf("The resolution of your image is %d x %d.\n",              img_width, img_height);
		printf("The textual resolution of this CMD window is %d x %d.\n", cmd_width, cmd_height);
		printf("Please lower your image resolution to fit your CMD window or increase the size of your CMD window.\n");
		return IMAGE_TOO_LARGE;
	}

	return IMAGE_OK;

#endif
}


// since user input is ansi, keep this ansi as well.
// we're not interacting with a file system, so no need to do
// if windows / linux shit.
FILE_EXTENSION pull_image_extension(char* file_name)
{
	char* file_extension = NULL;

	EXTENSION_TABLE map[] = 
	{
		{".jpg",  JPEG},
		{".jpeg", JPEG},
		{".jpe",  JPEG},
		{".jfif", JPEG},
		{".bmp",  BMP},
		{".tga",  TGA},
		{".psd",  PSD},
		{".png",  PNG},
		{".hdr",  HDR},
		{".pic",  PIC},
		{".ppm",  PNM},
		{".pgm",  PNM},
		{".pnm",  PNM}
	};

	file_extension = strrchr(file_name, '.');
	if (!file_extension)
	{
		return NO_EXTENSION;
	}


	// for loop calculates the # of entries in the map by calculating
	// total size of map and divides it by the size of one entry
	for (int i = 0; i < sizeof(map) / sizeof(map[0]); i++)
	{
		if (strcmp(file_extension, map[i].file_extension) == 0)
		{
			// yes i know its confusing but .file_extension refers to the STRING
			// .type refers to the ENUM value corresponding to the string.
			return map[i].type;
		}
	}

	return NO_EXTENSION;
}


// if the file format is default sRGB, gamma decompress, linear luminance to grayscale, then recompress gamma back.
// else, just do linear luminance to grayscale.
//
// force all color channels to be 3 (always RGB)
int do_da_ascii_art(char* path_to_img, FILE_EXTENSION file_type, BACKGROUND_COLOR bg_color)
{
	unsigned char* img_data       = NULL;
	unsigned char* pixel_data     = NULL;
	char*          density        = NULL;
	int            density_length = 0;
	int            width          = 0;
	int            height         = 0;
	int            color_channels = 0;

	if (bg_color == BLACK)
		density = strdup(" `-.'_:,=^;><+!rc*/z?sLTv)J7(|Fi{C}fI31tlu[neoZ5Yxjya]2ESwqkP6h9d4VpOGbUAKXHm8RD#$Bg0MNWQ%&@$");
	else if (bg_color == WHITE)
		density = strdup("$@&%QWNM0gB$#DR8mHXKAUbGOpV4d9h6PkqwSE2]ayjxY5Zoen[ult13If}C{iF|(7J)vTLs?z/*cr!+<>;=^,:'_.-`");

	if (!density)
	{
		printf("strdup() for density in do_da_ascii_art() failed.\n");
		return BOOM_BOOM_ERROR;
	}
	density_length = strlen(density);

#if _WIN32

	wchar_t w_path_to_image[MAXIMUM_PATH_LENGTH];
	if (!MultiByteToWideChar(
		CP_UTF8,
		0,
		path_to_img,
		-1,
		w_path_to_image,
		MAXIMUM_FILE_LENGTH))
	{
		printf("Failed to convert path_to_img (ANSI) to Unicode\n");
		return BOOM_BOOM_ERROR;
	}

	FILE* file_pointer = _wfopen(w_path_to_image, L"rb");
	img_data = stbi_load_from_file(
		file_pointer,
		&width,
		&height,
		&color_channels,
		3);

	fclose(file_pointer);
	// unicode handling stops here, you can relax now.

#elif defined(__linux__) || defined(__APPLE__)

	img_data = stbi_load(
		path_to_img,
		&width,
		&height,
		&color_channels,
		3);
#endif

	if (!img_data)
	{
		printf("Failed to load image: %s\n", path_to_img);
		return BOOM_BOOM_ERROR;
	}

	if (enforce_console_resolution(width, height) == IMAGE_TOO_LARGE)
	{
		hang();
		exit(-1);
	}
	
	//finally, the algorithm actually starts here.

	// if file type is standardized or presumed to use sRGB,
	// gamma expansion -> calculuate grayscale linear luminance -> inverse gamma expansion
	if (file_type == JPEG
		|| file_type == BMP
		|| file_type == TGA
		|| file_type == PSD
		|| file_type == PNM
		|| file_type == PNG)
	{
		pixel_data = img_data;
		for (int i = 0; i < height; i++)
		{
			for (int j = 0; j < width; j++)
			{
				float normalized_sRGB[3];
				float linear_RGB     [3];
				float relative_linear_luminance = 0; // in the range of [0, 1]
				float luma                      = 0; // aka relative_sRGB_luminance; in the range of [0, 1]
				int   density_index             = 0;

				normalized_sRGB[0] = (float)*pixel_data++ / 255.0f;
				normalized_sRGB[1] = (float)*pixel_data++ / 255.0f;
				normalized_sRGB[2] = (float)*pixel_data++ / 255.0f;

				// gamma expansion 
				// https://en.wikipedia.org/wiki/Grayscale#Colorimetric_(perceptual_luminance-preserving)_conversion_to_grayscale
				for (int a = 0; a < 3; a++)
				{
					if (normalized_sRGB[a] <= 0.04045f)
					{
						linear_RGB[a] = normalized_sRGB[a] / 12.92;
					}
					else
					{
						linear_RGB[a] = powf(((normalized_sRGB[a] + 0.055f) / 1.055), 2.4f);
					}
				}

				relative_linear_luminance = 0.2126f*linear_RGB[0] + 0.7152f*linear_RGB[1] + 0.0722*linear_RGB[2]; 

				if (relative_linear_luminance <= 0.0031308f)
					luma = 12.92 * relative_linear_luminance;
				else
					luma = (1.055 * powf(relative_linear_luminance, 1.0f/2.4f) - 0.055);

				density_index = (int)(luma * (density_length - 1));

				// just in case for rounding errors
				if (density_index < 0)
					density_index = 0;
				if (density_index >= density_length)
					density_index = density_length - 1
				;
				putchar(density[density_index]);
				
			}
			putchar('\n');
		}
	}

	// default behavior; grayscale with linear RGB
	else
	{
		pixel_data = img_data;
		for (int i = 0; i < height; i++)
		{
			for (int j = 0; j < width; j++)
			{
				float relative_linear_luminance = 0;
				int   density_index             = 0;

				float R_linear = (float)*pixel_data++ / 255.0f;
				float G_linear = (float)*pixel_data++ / 255.0f;
				float B_linear = (float)*pixel_data++ / 255.0f;

				relative_linear_luminance = 0.2126*R_linear + 0.7152*G_linear + 0.0722*B_linear;

				density_index = relative_linear_luminance * (density_length - 1);

				if (density_index < 0)
					density_index = 0;
				if (density_index >= density_length)
					density_index = density_length - 1;

				putchar(density[density_index]);
			}
			putchar('\n');
		}
	}

	free((void*)density);
	return 0;
}



// todo: fix this stupid eyesore mess
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

	// Get background color
	BACKGROUND_COLOR background_color;
	printf("What is your console's background color?: 1) White 2) Black: ");
	char input = getchar();
	if (input == '1')
		background_color = WHITE;
	else if (input == '2')
		background_color = BLACK;
	else
	{
		printf("Not a valid option.\n");
		hang();
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

	// as much as i'd love to add auto extension detection,
	// since both filesystems (NTFS, ext.4) treat a file string
	// as including its extension, you really cant open a file
	// without including the extension. like, why check for jpg
	// if the file name is literally image.jpg. 
	//
	// ex. image.jpg && image.png are NOT the same thing, so you
	// cant just fopen() image.
	char real_path[MAXIMUM_PATH_LENGTH];

#if _WIN32
	if (_snprintf_s(
		real_path,
		MAXIMUM_PATH_LENGTH,
		_TRUNCATE,
		"%s\\%s",
		path,
		file_name) == -1)
	{
		printf("Failed to create real_path.\n");
		exit(-1);
	}
#elif defined(__linux) || defined(__APPLE__)
	int written_chars = snprintf(
		real_path,
		MAXIMUM_PATH_LENGTH,
		"%s/%s",
		path,
		file_name);

	if (written_chars < 0
		|| written_chars >= MAXIMUM_PATH_LENGTH)
	{
		printf("Failed to create real_path\n");
		exit(-1);
	}

#endif

	FILE_EXTENSION file_extension = pull_image_extension(file_name);
	if (file_extension == NO_EXTENSION)
	{
		printf("File has no extension?\n");
		hang();
		exit(-1);
	}

	if (do_da_ascii_art(real_path, file_extension, background_color) == BOOM_BOOM_ERROR)
	{
		hang();
		exit(-1);
	}


	hang();
	return 0;
}