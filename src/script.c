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



// depreciated for a better way of checking if a file
// uses sRGB (by stbi_is_hdr())
/*typedef enum file_types
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

} FILE_EXTENSION;*/

// depreciated (am i even using this word right?)
/*typedef struct
{
	const char*     file_extension;
	enum file_types type;

} EXTENSION_TABLE;
*/

void hang()
{
	int lebron_james;

	printf("Press ENTER to quit: ");

	// first while loop consumes all the '\n's (or basically leftover until EOF) in stdout because APPARENLTY GETCHAR() INSTANTLY
	// QUITS IF THERES A NEW LINE CHAR IN THE STDOUT STREAM AND FFLUSH DOESNT DO ANYTHINGGGGGGGGG
	while ((lebron_james = getchar()) != '\n'
		|| lebron_james != EOF)

	return;
}

// ok this is just me hotpatching the mess that i call my "code" but these two functions work in tandem
// with each other. (ex. if you remove get_char_and_flush(), you have to add another while loop in hang()) 

char get_char_and_flush()
{
	char input;
	char flush;
	input = getchar();

	while ((flush = getchar()) != '\n'
		&& flush != EOF)
	{
		;
	}

	return input;
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
			printf("\nPath not found.\n");
			return BOOM_BOOM_ERROR;
		}

		if (error == ERROR_ACCESS_DENIED)
		{
			printf("\nAccess denied to path: %s", path);
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

	printf("\nFile not found.\n");
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
	printf("\nFile not found.\n");
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

#elif defined(__linux__) || defined (__APPLE__)
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
		printf("The textual resolution of this CMD window is %d x %d.\n", console_width, console_height);
		printf("Please lower your image resolution to fit your CMD window or increase the size of your CMD window.\n");
		return IMAGE_TOO_LARGE;
	}

	return IMAGE_OK;

#endif
}


// since user input is ansi, keep this ansi as well.
// we're not interacting with a file system, so no need to do
// if windows / linux shit.
/*FILE_EXTENSION pull_image_extension(char* file_name)
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
		{".pgm",  PNM}
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
}*/



// if the file format is default sRGB, gamma decompress, linear luminance to grayscale, then recompress gamma back.
// else, just do linear luminance to grayscale.
//
// force all color channels to be 3 (always RGB)
int do_da_ascii_art(char* path_to_img, BACKGROUND_COLOR bg_color)
{
	bool   is_hdr         = false;
	float* img_data       = NULL;
	float* pixel_data     = NULL;
	char*  density        = NULL;
	int    density_length = 0;
	int    width          = 0;
	int    height         = 0;
	int    color_channels = 0;


	if (bg_color == BLACK)
		density = strdup(" `-.'_:,=^;><+!rc*/z?sLTv)J7(|Fi{C}fI31tlu[neoZ5Yxjya]2ESwqkP6h9d4VpOGbUAKXHm8RD#$Bg0MNWQ%&@$");
	else if (bg_color == WHITE)
		density = strdup("$@&%QWNM0gB$#DR8mHXKAUbGOpV4d9h6PkqwSE2]ayjxY5Zoen[ult13If}C{iF|(7J)vTLs?z/*cr!+<>;=^,:'_.-` ");

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

	// disable the LDR to HDR conversion when using stbi_loadf()
	stbi_ldr_to_hdr_gamma(1.0f);
	stbi_ldr_to_hdr_scale(1.0f);

	img_data = stbi_loadf_from_file(
		file_pointer,
		&width,
		&height,
		&color_channels,
		0);

	is_hdr = stbi_is_hdr_from_file(file_pointer);

	fclose(file_pointer);
	// unicode handling stops here, you can relax now.

#elif defined(__linux__) || defined(__APPLE__)

	stbi_ldr_to_hdr_scale(1.0f);
	stbi_ldr_to_hdr_gamma(1.0f);

	is_hdr = stbi_is_hdr(path_to_img);

	img_data = stbi_loadf(
		path_to_img,
		&width,
		&height,
		&color_channels,
		0);
#endif

	if (!img_data)
	{
		printf("Failed to load image: %s\n", path_to_img);
		return BOOM_BOOM_ERROR;
	}

/*	if (enforce_console_resolution(width, height) == IMAGE_TOO_LARGE)
	{
		hang();
		exit(-1);
	}
	*/
	//finally, the algorithm actually starts here.


	// HDR IS ALWAYS LINEAR LIGHT
	// LDR IS PRESUMED TO USE sRGB


	if (!is_hdr)
	{
		pixel_data = img_data;
		for (int i = 0; i < height; i++)
		{
			for (int j = 0; j < width; j++)
			{
				float normalized_sRGB[3];
				float linear_RGB     [3];
				float alpha_pixel               = 0;
				float relative_linear_luminance = 0; // in the range of [0, 1]
				float luma                      = 0; // aka relative_sRGB_luminance; in the range of [0, 1]
				int   density_index             = 0;

				normalized_sRGB[0] = *pixel_data++;
				normalized_sRGB[1] = *pixel_data++;
				normalized_sRGB[2] = *pixel_data++;

				if (color_channels == 4)
					alpha_pixel = *pixel_data++;

				// gamma expansion 
				// disabled stbi_image's gamma conversion for my own conversion because mine is better (true gamma expansion)
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

				// note that the values in linear_RGB are now alpha blended by now
				// account for alpha pixel
				if (color_channels == 4)
				{
					if (alpha_pixel <= 0.0f)
					{
						putchar(' ');
						putchar(' ');
						continue;
					}

					else
					{
						relative_linear_luminance = 0.2126f*linear_RGB[0] + 0.7152f*linear_RGB[1] + 0.0722*linear_RGB[2];
						relative_linear_luminance *= alpha_pixel;

						// because computers are bad at multiplication.
						if (relative_linear_luminance < 0.0f)
							relative_linear_luminance = 0.0f;

						else if (relative_linear_luminance > 1.0f)
							relative_linear_luminance = 1.0f;
					}
				}

				else
				{
					relative_linear_luminance = 0.2126f*linear_RGB[0] + 0.7152f*linear_RGB[1] + 0.0722*linear_RGB[2]; 

					if (relative_linear_luminance < 0.0f)
						relative_linear_luminance = 0.0f;

					else if (relative_linear_luminance > 1.0f)
						relative_linear_luminance = 1.0f;
				}


				// gamma compression
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
				putchar(density[density_index]);
				
			}
			putchar('\n');
		}
	}

	// default behavior; grayscale with linear RGB (HDR stuff)
	else
	{
		pixel_data = img_data;
		for (int i = 0; i < height; i++)
		{
			for (int j = 0; j < width; j++)
			{
				float relative_linear_luminance = 0;
				int   density_index             = 0;
				float linear_RGB[3];

				linear_RGB[0] = *pixel_data++;
				linear_RGB[1] = *pixel_data++;
				linear_RGB[2] = *pixel_data++;

				float alpha_pixel = 67.0f;

				if (color_channels == 4)
				{
					alpha_pixel = *pixel_data++;

					if (alpha_pixel <= 0.0f)
					{
						putchar(' ');
						putchar(' ');
						continue;
					}

					else
					{
						relative_linear_luminance = 0.2126f*linear_RGB[0] + 0.7152f*linear_RGB[1] + 0.0722*linear_RGB[2];
						relative_linear_luminance *= alpha_pixel;

						if (relative_linear_luminance < 0.0f)
							relative_linear_luminance = 0.0f;

						else if (relative_linear_luminance > 1.0f)
							relative_linear_luminance = 1.0f;
					}
				}	
				else
				{
					relative_linear_luminance = 0.2126*linear_RGB[0] + 0.7152*linear_RGB[1] + 0.0722*linear_RGB[2];

					if (relative_linear_luminance < 0.0f)
						relative_linear_luminance = 0.0f;
					else if (relative_linear_luminance > 1.0f)
						relative_linear_luminance = 1.0f;
				}


				density_index = relative_linear_luminance * (density_length - 1);

				if (density_index < 0)
					density_index = 0;
				if (density_index >= density_length)
					density_index = density_length - 1;

				putchar(density[density_index]);
				putchar(density[density_index]);
			}
			putchar('\n');
		}
	}

	stbi_image_free(img_data);
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
	printf("Enter the absolute / relative PATH of image: ");
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
	printf("Enter the file name (include the extension): ");
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
	printf("What is your console's background color? (default choice is black\n");
	printf("1) White\n");
	printf("2) Black\n");
	printf("Enter choice: ");
	char input = get_char_and_flush();

	if (input != '1')
		background_color = BLACK;
	else
		background_color = WHITE;	



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

/*	FILE_EXTENSION file_extension = pull_image_extension(file_name);
	if (file_extension == NO_EXTENSION)
	{
		printf("File has no extension?\n");
		hang();
		exit(-1);
	}
*/
	if (do_da_ascii_art(real_path, background_color) == BOOM_BOOM_ERROR)
	{
		hang();
		exit(-1);
	}


	hang();
	return 0;
}

// produced by the windows version of this program (different image is used for the testing of windows version and linux version, resulting in 
// resolution differences)
/*#if 0                                                              

                                                                                                                              vvrr
                                                                                                                            ooyyyynn
                                                                                                                          nnoo1111uu}}
                                                                                                                        ^^yy11}}}}}}ZZ^^
                                                                                                                        vvuu{{{{{{}}11}}
                                                                                                                        ccuu{{{{{{}}}}tt
                                                                                                                          3311{{{{{{{{uucc
                                                                                                                            [[11}}}}}}}}[[
                                                                                                                            ^^ZZ113333}}uurr
                                                                                                                              }}oouu11331133
                                                                                                                                EEZZuu33{{ZZ..
                                                                                                                                vv22uu1133uu}}
                                                                                                                                  EEZZuu33}}[[
                                                                                                                                  nnyyuu33{{uurr
                                                                                                                                  vvwwZZuu}}11}}
                                                                                                                                    99ZZuu}}}}[[
                                                                                                                                    ooyyuu33{{yy^^
                                                                                                                                    }}22ZZ33{{uu{{
                                                                                                                                    vvhhZZ33}}11oo
                                                                                                                                    rrwwZZuu3311yy
                                                                                                                                    ..99ZZZZuu11jj))
                                                                                                                                      PPyyZZZZuuZZYY
                                                                                                                                      2222yyxxZZZZyyvv^^..
                                                                                                                                      22PPqqqqSSSSPPUU88OOVV22}}cc
                                                                                                                                ..{{22OOGGppGGGGGGGGGGGGGGGGGGAAUU99tt
                                                                                                                            ^^22KKUUXXAAAAAAAAAAAAGGAAGGGGAAGGpppp44pp99{{
                                                                                                                          {{KK88AAAAHHHHHHAAAAAAAAAAAAAAAAAAOOppppVVpp99hh22
                                                                                                                        jjRR8888XXXXHHHHAAAAAAAAAAAAAAAAOOAAAAOOOOVVdddd99VVPP..
                                                                                                                      ttRR8888XXHHHHHHHHHHAAAAAAAAAAAAAAAAAAOOOOOOVVddhh666699PP
                                                                                                                    vvRR$$DDDD88DDDDDDRRHHHHAAAAKKKKAAAAAAUUUUOOOOVVddhhhh6666hh22
                                                                                                                  ..KK$$$$DDDD88BBBBBBBBDDHHHHHHKKKKKKKKUUUUOOOOVVdddd99hhPPkkwwPPnn
                                                                                                                  jj00$$DDDDDD00MMMMMM00BBDDHHHHHHKKKKKKKKUUOOVVVVdd9999PPqqSS22wwwwvv
                                                                                                                ^^88BB$$DDDD$$MMWWWWWWMMBBDDHHHHHHKKKKKKUUUUOOVVVVdd9999PPkkSS222222EE
                                                                                                                jj00BB$$$$$$00NNWWWWWW00BB##RRHHHHKKKKKKUUUUOOVVVV999999PPkkSS222222qq}}
                                                                                                                KK$$BB$$$$BBMMNNWWWWWW00BBDDHHHHHHHHHHKKUUUUOOVVVVdd9999PPkkSSEE22aa22EE
                                                                                                              {{ggBB$$$$DDBBMMWWQQWWWW00$$RRHHHHHHHHKKKKUUUUOOVVVVdd99PPkkkkqqEE]]aa22ww}}
                                                                                                              6600BB$$$$$$BBMMWWQQWWMM00$$RRHHHHHHKKKKUUUUOOVVVVVVddPPkkqqkkqqEE22]]2222EE
                                                                                                            ..8800BB$$$$$$BBMMQQQQWWMMBBDDRRHHHHHHHHKKUUUUOOVVVVddhhPPkkqqqqSSEE22]]2222hhvv
                                                                                                            {{ggBB$$##$$BB00WWQQWWWWMMBB##RRRRHHHHHHKKKKUUOOVVddddhhPPqqqqSSEE2222]]]]2222oo
                                                                                                            6600$$DD##$$BB00MMWWWWMMMMBB##RRRRHHKKHHHHUUUUOOVVdd66kkqqSSSSEEaaaaaa]]]]2222ww^^
                                                                                                            KKBB$$DD##$$BB00MMWWMMMM00BB##RRHHHHHHKKKKUUUUOOddhhPPqqSSSS2222aayyaaaa]]22SSqq))
                                                                                                          rrgg$$DDDDDDDD$$BBMMMMMMMM00$$DDHHHHHHHHKKAAUUUUVVhhPPkkkkSSEE22aayyjjyyaa]]22EE22oo
                                                                                                          nnMM$$DDDDRRDD$$BB00000000$$DDRRHHHHKKKKUUOOOOOOddhhPPkkqqSSEE22aayyyyyyyyyyaaaaaaww..
                                                                                                          VV00##DDDDDDDD$$BB0000BB$$DDRRHHHHHHKKKKUUOOVVdd99hhkkkkSSSSEE22aayyyyyyyyaaaaaaaa22vv
                                                                                                        ^^88BB##DDDDDD##$$BB00BB$$$$DDRRHHHHHHHHKKUUVVdd9999hhkkkkSSSSEE]]aayyyyyyyyaa]]aaaaww}}
                                                                                                        ttgg$$DDDDDDDD####$$BB$$$$$$DDRRDDHHHHKKUUOOVV99kkkkPPkkqqSSSS22aayyyyyyyyaaaaaaaaaa22YY
                                                                                                        6600$$##DDDDDD######BB$$DD##DDDDRRHHHHUUOOOOVV99kkkkkkkkqqSSSS]]aayyjjyyyyaaaaaa]]aa22ww
                                                                                                        KKBB$$####DDDDDD######DDDD##DDDDRRHHAAUUOOVVVV99PPkkPPkkqqSSEE]]yyyyyyyyyyaaaaaayyjjaahhcc
                                                                                                      vvggBB$$######DDDDDDRRDDDDDDDDDDRRHHAAAAOOOOOOVVdd99hhPPqqSSEEEEaayyyyyyaayyyyyyyyaajjaaww}}
                                                                                                      jjWWBB$$####DDDDDDRRRRDDDDDDDDRRHHHHAAAAUUUUOOVVVVddhhPPkkSSEEEEaaaayyyyaayyxxYYxxaaaaaawwYY
                                                                                                      KK00BB$$######DDDDRRRRDDDD##DDHHHHHHAAUUUUOOOOVVddddhhPPkkqqSS22aaaaaaaaaayyxxxxxxaaaaaa2299..
                                                                                                    vvgg00$$##DDDDDDDDDDRRDD88DDDDRRHHHHAAUUUUOOOOVVddddhhPPkkqqqqSS22222222aaaayyxxxxyyaaaaaa22hhcc
                                                                                                    jjWW00$$##DDRRRRDDDDDD888888RRHHHHAAUUOOOOOOVVddhhPPkkqqqqSSqqSSEE22222222aayyyyyyyyaaaaaawwdd}}
                                                                                                    KKMMBB$$DDRRRRRRDD8888888888AAHHHHAAOOOOOOVVddhhhhPPkkqqSSSSqqqqSSEEaaaaaayyyyyyjjyyyyaaaa22PP22
                                                                                                  vvgg00$$DDRRRRRRRR88888888XXXXAAAAAAAAOOUUOOVVddhhPPkkqqSSqqqqqqqqSS22aayyyyyyxxyyyyjjjjjjaa22ww99..
                                                                                                  66MM00DDRRRRRRRR88XXXXXXXXXXAAAAGGppOOOOOOOOddhhPPkkkkqqqqqqSSSSSSEE22aayyyyyyyyyyyyyyxxjjaa22wwhhvv
                                                                                                ^^8800$$DDRRRRHHHHXXXXAAAAbbGGGGGGppppOOOOVVVVddPPkkkkqqSSSSSSSS22222222aayyyyyyyyyyjjyyjjjjaa22wwPP22
                                                                                                ttNN00$$DDRRRRHHXXXXAAbbAAbbGG4444ppppVVVVVVdd6666kkkkqqSSSSSSEE2222aaaaaayyyyyyjjjjjjjjjjaaaa22wwPPOOrr
                                                                                                VVMM$$888888XXXXXXAAbbGGGGGG44444444ppVVVVVVdd6666qqqqqqSSSS2222222222aaaayyaaaayyxxYYxxyyaaaa22wwPPVVjj
                                                                                              vvgg$$888888XXXXAAXXbbOOOOOO44446644444444VVdddd6666kkqqSSSSSSEE22222222aaaaaayyyyyyjjYYxxyyyyaaSSSSkkPPOO^^
                                                                                              66MM$$888888XXXXbbbbOOOOOO44446666664444dd99dd66666666qqSSSSSSSS22222222aaaaaayyjjyyjjxxYYxxxxaa]]SSSSwwddnn
                                                                                            rrgg$$8888XXXXXXAAbbOOOOdddd666666666644PP44999966PP6666qqSSSSSSSS22aa2222aaaaaayyjjjjjjyyxxxxxxyyaa]]22EEqq99..
                                                                                            66$$$$88XXXXXXAAbbGGddddhhhh66666666666666PPPPPP666666qqqqSSSSEEEE2222aa2222aayyyyjjjjjjxxYYxxxxxxxxaaaa22wwhhtt
                                                                                          ttggBBDD88XXAAAAbbGGppddhhkkkkkkwwww66666666PPPPPPPPPPqqqqqqqqEEEEEE]]aaaaaaaaaayyjjjjjjYYYYxxyyxxxxxxyyaa22EEqqPP
                                                                                        rr88$$8888XXbbbbOOOOddddhhkkkkwwwwwwwwwwww66SSPPSSSSSSSSqqqqqqqqqqEEEE]]]]aa]]]]yyyyjjxxxxYYYYjjyyyyxxxxxxaa22aawwhh{{
                                                                                        VV$$$$XXXXbbbbOOOOddddhhkkkkkkwwEEEEEEEEwwwwwwwwSSSSSSSSwwPPPPPPqqqqEE]]]]]]]]]]yyxxYYYYooooYYxxjjyyyyxxxxyy22aa22wwPP
                                                                                      6600$$88XXbbbbppddddhhkkkkkkkkEEEEEEEEEEEEEEEEwwwwSSwwww666666PPPPqqqqqq]]]]]]]]]]yyYYYYooooooooYYjjyyxxyyxxaa222222wwhhtt
                                                                                    66MM$$88XXAAbbppddhhhhkkkkkkwwwwEEEEEEaaaaaaEEEEEE22wwSSwwww6666PPPPqqqqEE]]]]]]22]]yyYYYYooooooooYYxxxxyyxxxxaaEESSSSwwPP99
                                                                                  jj$$$$88XXAAbbOOddhhkkkkkkwwwwEEEEEEEEaaaaaaaaaaEEEE2222wwwwwwqqqqqqqqwwEEEEEE]]]]22]]yyjjYYYYooooooYYxxyyxxxxxxaa]]2222wwPPOO}}
                                                                                jjgg$$88XXAApppphhkkwwww2222EE22EE22EEaaaaaaaajjaaaaaaEE222222wwwwqqqqqqqqSSEEEE]]]]2222]]yyjjYYYYYYYYYYxxjjxxxxaaaa]]]]22SSwwdd99..
                                                                              jjRR88AAbbbbppddhhkkww22aaaaaa2222aaaaaaaaaaaajjjjjjaaaaaayy2222wwkkkkqqqqqqSSEEEE]]]]]]]]]]aajjxxYYYYYYxxjjxxyyyyyyaa]]]]]]SSwwhhOOjj
                                                                            ttRRXXbbOOddddhhqqqqqqwwaayyyyyyyyaayyaaaaaajjjjjjjjjjjjjjxxyy22wwkkqqhhhhqqqqSSSSEE]]]]yyaayyyyjjxxYYYYYYxxxxxxyyyyyyyy]]]]22]]kkhhddOO^^
                                                                          {{88AApphhkkkkkkqqqqPPPPwwaaxxxxyyxxjjjjjjjjjjjjjjjjjjjjjjaayyyy22wwqq99hhhhkkkkqqSSEE]]xxxxxxxxxxxxxxxxxxxxxxxxxxyyyyyyyyyy]]]]]]SSkkddOOjj
                                                                        ^^KKGGddqqwwkkkkqqPP999999qqaaxxxxxxyyjjjjjjjjjjjjjjZZjjjjjjyyxxaawwqq9944449999hhqqSSaaaaxxYYYYYYxxxxxxjjxxxxxxxxxxyyaayyaaaaaa]]aa22kkPPOOOO^^
                                                                        66bbhh222222wwPP9999ppppVV9922xx55xxxxjjaajjjjjjZZZZZZZZjjyyyyyyaawwPP44pppp4499qqqq22aaxxYYYYYYYYxxxxxxjjjjxxxxyyyyaaaayyaayyaaaa]]22SSqqddAAnn
                                                                      nnAAPPaaxxxxaawwPP99ppbbAAbb44wwaa55xxxxjjjjjjjjZZZZZZZZZZxxxxyyyy22qq44bbAAbb44PPqqqq22yyZZZZZZZZYYYYxxxxjjjjjjjjjjjjyyaaaaaayyyyaaaa22SSwwddOO99
                                                                    ..VV99225555xxaawwPP99ppAAHHAAGGPP22xx55xxjjjjjjjjZZZZZZZZZZjjxxxxyywwPPppKKKKbb99PPqq99qq22ZZeeeeeeZZZZYYYYxxYYxxjjxxjjyyaayyyyyyxxyy]]22SSkkhhVVUUtt
                                                                    ttGGwwyyZZZZyy22qqqqPP44bbKKKKAAppPP22yyxxxxjjjjZZZZZZZZeeZZxxxxxxaaww44bbmmHHbb99PP9999PP2255eetteeeeZZZZYYYYYYxxxxxxxxyyyyyyyyyyxxyyaa]]SSqqPPddOOVV
                                                                    PP99EEZZeeZZyy22qqqqqqPP99AAHHHHHHpp9922yyxxxxZZZZZZeeeeeeZZZZjjaa22PPbbmmDDRRKKVV9999PP2255ttffffffeeeeZZZZYYYYxxxxxxjjjjyyaaaayyxxyyyy]]EESSkkddOOUUvv
                                                                  rrVVPPaaZZeeZZyy2222aaoooojj99KKmmmmHHbb9922xxZZZZZZZZeeeeeeeeZZjjjj2299AARR##RRKK99wwjjoollFF))))))iitteeeeZZYYYYYYxxxxjjjjyyaa]]]]yyyyyy]]22SSkk99VVUU22
                                                                  {{GGwwjjee[[55xxoollCC))FFCCoo99AARRDDmmbb9922xxZZZZeeeeeeeeeeZZjjjj22ppmm$$$$KKwwooCC))))zzcccccczziifftteeZZZZYYxxxxxxjjyyaaaa]]]]]]aayy]]22SSkkhhddVVOO^^
                                                                  jjppwwxx[[[[llllCCCC))))zzzzFFooPPHHDDDDmmbbwwyy55ZZeeeeeeeeeeeejjxxwwbbRRBBRRPPoollCCFFzzcc!!!!!!!!TTiiff[[eeZZYYxxxxxxyyaaaayyyy]]22aayyaa]]SSkkhh99VVUUtt
                                                                  PP9922ZZ[[3333CCCCCCCC))zzcczz))llppDD$$$$mm44wwxx55eeeeeeeeeeeeZZxxwwKK####bbjjoooojjooFFzz!!!!cc!!zzTTiittZZeeYYxxxxxxyyyyaayyjjyyaaaaaaaa]]SSkkhh99VVUUPP
                                                                  VVPP2255[[33CCCClljjjjll))zzcczzFF22HH$$gg$$KK992255ZZeeeeeeeeeeZZyyqqHHBBRRPPoooo2299PPxxiicc**!!!!cczz))ff[[eeZZxxxxjjyyyyyyyyjjjjyyyyaaaa]]SSkkhhhh99VVOO^^
                                                                ..VVPPyyZZttCCCClljjwwPP22ttTTzzzz))ll44RRggggRRbbPPaa55eeeetteeeeZZaa99RRBBHHwwjjjj22aa22yyffzz****!!!!ccTTii[[eeZZYYYYjjyyyyyyyyjjjjyyaaaayyaa]]SSkkhhhhVVUUtt
                                                                ^^VVPPyy[[ffFF33oojjjjaaxxffTT****zzFF22HHggMMggRRGGwwxxZZeett[[ZZxxwwppRR00HH99wwjj33ffffiiTT**********cczz))ff[[ZZoooojjyyyyyyyyjjyy]]]]aayyaa]]SSkkhhhhddUU22
                                                                rrVVPPyy[[ffFFlljjoo33FFiiTTzz****cc))ooVV##MMMMggmmpp22ZZee[[[[[[xxqqbb##00RRVVPPooFFzzzzzz**!!********cccc))FF[[ZZooooxxyyyyyyaaaaaa22]]yyxxaa]]SShhhh99VVOO99
                                                                ^^VVPPyyttFFFFoo22ll))zzzz!!!!****!!))33PPRR00NNMM$$HHPPyyZZ[[[[ZZxxPPKKBBMMggmm44oo))cc!!!!!!!!!!******cccc))FF33uuooooxxyyxxxx]]22]]22]]aayyyy]]SS66hh99VVOOOOrr
                                                                ^^VVPPxx33FFCCjj22ll))zz**!!!!****!!TTCCwwRRMMWWWWMMDDbbqqxx[[[[ZZyyPPmm00MMMMDDppoo))cc!!!!!!!!!!******cczz))FF33uuooYYxxyyxxxxyyaa]]EEEE2222]]]]SS66hh99VVOOUU{{
                                                                ..VVqqxxll33lljjwwooFFTT************TT33PPRRNNWWQQWWMMRRVV2255[[ZZyy99RRMMNNNN$$bboo))cc!!!!!!**!!!!**zzzz))CCCC11uuYYxxxxyyaaaayyaaaa]]22aaEEEE22SS66hhhhVVOOXX}}
                                                                  VVqqxxoooooowwPPjjffTTzz********zzTTll99##WWQQQQQQWWMMRR99aa[[ZZ22ppDDNNWWWW00HHjj))zzzz****!!!!!!**TT))CCllllZZZZYYxxjjaa]]aaaaaaaaaaaa22SSSSEESSkkkkhhVVOOUUjj
                                                                  PPqqaajj2222wwPP22[[iiTT******zzTTiixxGGBBQQQQQQQQQQWWMMHHPP55xx22bb##NNQQQQMMRRwwCCFFTTTT**!!****zzTTFFlloojjyyaaYYxxjjaa]]aaaaaaaaaa2222SSSSEESSkkkkhhVVOOUU22
                                                                  66PP22qqPPPPPP99qqaa[[iiTTzzTTTTiillqqHHMM%%QQQQQQWWWWNN##bbwwaawwKKBBQQQQQQWWBBVVoo33ffiiTTzzzzTTTTiilloojjwwqqSS]]xxjjaa]]aaaaEEEE22aa22SSSSSSSSkkkkhhVVOOUU22
                                                                  jj9922PPpppp449999qqaa[[iiiiiiFF3322AABBNN%%%%%%WWNNMMMMggRRppPP99HHMMQQQQ%%QQMMRR99xxttffiiiiiiiiffttoo22PPPPhh66EEyyjjjjaa2222EEEE22aaaaSSqqSSSSkkkkhhVVOOUU22
                                                                  }}GG22PPbbKKpp999999PPwwxx55lloojjVV##MMQQ%%QQQQWWMMMMMMMM$$HHbbbbRRMMQQQQQQWWNNBBKK99aa55[[[[tt[[ZZyy22ww99VVpphhqq22aaxxaa22aa22EEaaaa22EEEEEE22SSkk99GGUUUUPP
                                                                  {{GG2299KKmmAApp4444pppp44PP99VVKKRRMMQQ%%%%WWWWNNMMggggMMMM##RR##BBWWQQQQWWWWWWNNBBHHbb99PPqqqqqqqqPP99VVbbAAGG44PPSS22yyaaaayyaaEE22EEEE2222]]22SSkk99GGUUUUVV
                                                                  rrOO2299KKRRmmKKbbbbAAHHRR####BB00MMQQQQQQQQNNNNMMggggMMNNNNMMNNNNNNQQQQWWQQQQWWWWWWMMBBDDHHHHHHHHKKKKKKHH88HHAAVVhhkkSS]]2222aaaaaaEEEE2222SSSSSSkkhhVVGGOOUUVV
                                                                  ..9922qqGGRRDDmmHHHHDD$$MMNNNNWWWWQQQQQQQQNNMMgg$$DD##00MMMMNNNNNNWWQQQQQQQQQQQQQQQQQQQQQQNNMM00ggBB##DDDDDD88AAVVhhkkSS2222EEEEaaaaEESSaaEEqqkkkkkkhhVVVVUUUUPP
                                                                    EE222244mmDD888888DD$$MMNNWWWWWWQQ%%QQWWMMgg$$RRmmDDBB00MMNNWWWWWWQQQQQQQQQQ%%QQQQQQQQ%%%%QQWWMMMMgg$$$$DD88AAVVhhkkkkSSSSSSEE22222222aaEEqqkk66kkhh99VVUUXX22
                                                                    jjqqyyPPHHRRRRRRRRRRDDggMMNNNNNNQQWWWWMMMM$$DDmmHHRR##00MMNNNNWWWWWWWWQQQQQQ%%%%%%QQQQ%%QQQQWWNNMM00gggg##88AAGG4466kkkkqqqqSS2222aayy22SSqqkkkkkkhhVVVVUUXXjj
                                                                    {{hhyyPPHHRRRRRRRRRRRR##BBgg00MMNNNNMMMM00$$RRHHHHmmDDggMMMMNNNNNNWWWWQQQQQQ%%%%%%QQQQ%%%%QQQQWWNNMMggggDDHHAAGG4499kkkkqqSS22aa22EESSqqqqkkkkkkkkhhVVGGUUXXtt
                                                                    ^^9922qqAARRRRRRRRRRRRDD####$$gg000000gg$$DDmmKKKKHHRR$$gg00MMMMNNNNWWQQQQQQ%%%%%%%%QQ%%%%%%QQWWWWMMgg$$DDHHAAGG449966kkqqSSEEEESSkkkkkkqqkk66666699VVOOUURRcc
                                                                      PPPPqqGGDDDDRRRRRRRRRRDDDDDD$$##$$$$$$DDRRHHKKAAKKmmDD$$gg00MMMMWWWWWWQQ%%%%QQ%%%%%%%%%%%%QQWWWWMMgg$$DDHHbbpp449966PPPPqqSSkkPPPPkkkkqqPPdd9999VVGGUUXXKK..
                                                                      jjGGhhGG88DDDDRRRRRRmmRRRRRRDDDDDDDDRRmmHHKKAAAAAAHHRR##ggggggMMMMWWQQQQQQQQQQ%%%%%%%%%%QQQQWWWWNN00##88AAGG449999kk66PPqqSSkkPPhhhhPPqqPPdd99VVVVGGUUXXPP
                                                                      {{UUppGGHHDDDDDDRRmmmmmmmmmmmmRRRRmmmmHHAAbbbbbbbbKKHHRR$$gggg00MMWWWWNNWWQQ%%%%%%%%%%QQ%%QQWWWWNN00##HHbbVV44996666kkkkqqqqqqkkPPhhPPPPhhdd9999VVGGUUXXnn
                                                                      ^^88AAAAHH88DDDDRRRR888888mmmmmmHHmmHHKKAAbbGGGGbbKKHHmm##$$ggMMMMMM0000NNWW%%%%%%%%QQQQQQQQWWNNMMggDDHHGG44996666kkkkqqqqqqkkkkPPhhhhhhhh66dd99VVGGUU88rr
                                                                        VV88XXHH8888DDRRRR88HH88HHHHHHHHHHKKAAGGppppppbbKKHHRR$$$$gggg##HHKKHHBBNNQQ%%%%%%QQQQQQQQWWMM00BBDDAAGG449966kkkkqqkkkkkkPPPPPPPPhhhhPP66ddVVppGGOOVV
                                                                        jj$$88HHHH8888DDDDRRHHHHmmHHHHHHXXAAAA44PP99ppppbbKKmmDDDD$$DDKK99PP99RRMMQQQQQQQQQQWWWWWWNNMMgg##88AAVV449966kkqqqqkkPPPPhhhhPPPP99hhPPkk6699VVGGOOnn
                                                                        vvgg88XXHH88mmDD8888HHHHHHXXXXXXXXAAbbPP2222qqPP44ppbbAAAAKKKKVV99VVKKBBNNQQQQQQQQWWWWWWNNMMgg$$DDHHbbVV449966kkkkqqkkPPhhddddhhhhddhhkkkk6699VVGGOO^^
                                                                          KK8888HHHHHH888888HHHHHHHHXXHHXXAAAA44qq2222qqPP9944ppppbbAAKKKKRRBBMMNNWWWWWWQQWWNNNNMM00gg##mmAApp4444996666PPkkkkkkPPhhhhddddhhPPPP66669944VV22
                                                                          nn$$8888HHHHHHHH88HHHHHHHHHHXXXXAAAAGG444499994444ppbbAAKKRRDD##BB00MMNNNNNNNNNNNNMMMM00gg$$RRKKppVV4499666666PPPPkkkkPPPPPPhh99PPPPPP666699hhUUvv
                                                                          ..KK88XXXXAAAAAAHHHHHHHHHHHHXXAAAAAAAAAAAAbbbbAAAAHHmmRRRRDD$$BB00gg00MMNNMMMMMMMMMM0000$$mmKKGGppVV44996666PPPPPPkkkkPPkkkkPPPPkkPPhhPP66hhPPPP
                                                                            nn$$88XXAAbbbbAAXXHHXXHHXXXXAAXXXXXXXXXXXXXXXXHHHHKKHHmmRRDD##BBgg00MMMMMMMMMMMM00BB$$RRAAVVVVppVV449966PPPPhhhh9999PPPP66PPkkPPhhPPhh66hhhh{{
                                                                              KK88XXAAGGVVppGGXXHHHHHHXXXXXXXXXXAAXXXXXXXXAAKKKKKKHHmmDD##BBBB00MM0000MMMM00BB##HHAAVVVVppVVVV449966PPPPhhddddddhhhhdd66PPhh99PP66hhPPPP
                                                                              vvRRXXAAGGVVVVppGGAAKKHHXXXXXXAAAAAAAAAAAAXXXXXXHHRRmmDDDDDD####BB0000000000BBDDHHAAVVddddVVVV44999966PPhhhhddVV99ddddhh66hhhh6666kkkkhhvv
                                                                                jjXXXXOOppppVVppGGGGHHHHHHHHXXAAAAAAAAAAXXXXHHHH888888DDDD####BB$$BBggggBBDDHHAAOOddddVVVVppVV99dd66hhhh99dd9999ddhhPPhhhh66kkkkwwPP22
                                                                                  66AAAAGGpp44hhddVVGGAAHHXXXXAAAAAAAAAAAAXXHH8888mmRRRRDDDD##$$$$$$####DDHHAAOOVVVVppppppVVVV99ddddddhh999999PPPPhhPPkkkkkkkkwwwwww^^
                                                                                    VVAAAAGGpp9999hhddppGGAAAAXXAAAAAAAAXXXXXXHHHHHHmmRRRRRRDDDD##DDRRHHHHAAAAOOVVppppVVVVVVVVdddddd6699999999PPkkPPhhkkqqkkwwwwPP}}
                                                                                    ..VVUUAAGGVV99PPPPhhddppbbAAAAAAAAAAXXXXXXHHHHHHmmmmmmmmRRRRRRRRHHHHAAAAAAGGppppppVVddVVVVdddd66PPPPPPkkPPPPPPPPPPqqkkkkwwqqYY
                                                                                      ..VVXXAAGGVVhhhhhhhhhh99ppGGbbAAXXKKKKHHHHHHHHHHmmHHmmmmHHHHHHHHAAAAAAbbGGGGppOOVVVVVVVVddddddhhPPPPPPkkPPPPkkkkkkkkwwwwww^^
                                                                                        ..VVXXGGpp4444hhhhhhqqhh9944bbbbAAAAKKHHHHHHHHHHHHHHHHAAHHAAAAAAAAAAAAGGppppOOVVddddVVddVVddhhPPPPkkkkkkqqqqqqkkkkwwwwvv
                                                                                            VVAAGGpp44pp44hhqqSS22qqPP99VVGGAAAAAAAAAAAAAAGGAAAAAAAAbbbbGGAAGGppppVVVVdddddddddddd66hhhhPPkkkkkkSSEESSSSwwqq}}
                                                                                            ..PPbbGGppppVV99kkaaxxyyyy22PPddppGGppGGppGGppGGbbbbGGGGGGGGGGGGppVVVVVVVVddhhhhddddhhPPhhhhPPkkkkkkSS222222ww33
                                                                                                22OOGGppVV66qqaaxxZZZZyy22qqqqhhddddddppppOOGGGGGGGGGGppppppVVVVVVVVddddhhddhhhhhhhhhhPPkkkkkkqqSS222222oo
                                                                                                  jjUUGGVV99kkEEaaxxxxxxaaaaSSqqPPhhhhddVVVVppppGGpp44ppppVVVVVVdddddd66hhhhPPPPhhPPPPkkqqqqkkSS222222YY
                                                                                                    ttOOGGVVkkqqEEEEaaaaaaaaSSSSqqPPhhhh4444444444pppp44VVVVddddddddhh6666PPPPkkPPkkkkqqSSEE22222222YY..
                                                                                                      rrPPVVhh66kkSSEEEE]]EEEEqqqqPPPP99999999ddddVVVVVVVVddddddVVdd6666PPkkPPkkSSqqqqEEEE22aaaa22YY
                                                                                                          nnhhhh66kkkkqqEEEEqqqqkkkkkk6666ddddhh66hhhhddddddddddddPPkkkkqqqqqqSSEE2222aaaaaaxxyy33
                                                                                                            vvEE4466kkkkkkkkkkkkkk66kkkkkkkkkkPPPPkkkkPPhh6666hh66kkqqSSSSEEEE2222aajjjjxxxxZZ))
                                                                                                                }}EEhh66kkkkkkkkkkqqqqSSqqqqSSSSSSSSSSqqqqqqSSqqSSSSEEEEEE]]aaaajjjjYYYYxxttcc
                                                                                                                    cc33ooZZEESSSS2222]]]]EE]]aaaaaaaaaa]]]]]]]]aajjjjjjjjjjxxYYYYoollff))
                                                                                                                          ^^>>ssttooZZjjjjjjjjxxxxxxxxYYYYYYYYjjooooooooooooooll1177>>
                                                                                                                                    ::>>ss77ff111111llllllllllllllllll11ff77ss::
                                                                                                                                                ^^::::::>>>>>>>>>>>>::^^










produced by the the linux version of this program
#endif*/


#if 0

                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                             
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                              
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                              
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                              
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                              
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                              
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                              
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                              
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                              
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                              
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                              
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                              
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                              
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                              
                                                                                                                                                                                                      --..--''..                                                                                                                                                                                                                                                                                                              
                                                                                                                                                                                                    ..//FFii((zz__``                                                                                                                                                                                                                                                                                                          
                                                                                                                                                                                                  ''zz[[aaaayyll((**''                                                                                                                                                                                                                                                                                                        
                                                                                                                                                                                                ''vvnnxxnnuuZZ[[nnffcc--                                                                                                                                                                                                                                                                                                      
                                                                                                                                                                                              --//IInneenn[[oo[[eeuuCC<<                                                                                                                                                                                                                                                                                                      
                                                                                                                                                                                            ``<<nnxxeett3333uuuuuunnnn||::                                                                                                                                                                                                                                                                                                    
                                                                                                                                                                                            ..vv11ee[[33CC{{CC{{33uunnII**``                                                                                                                                                                                                                                                                                                  
                                                                                                                                                                                            >>llYY[[IICCCCCCCCCCCC}}IItt{{;;                                                                                                                                                                                                                                                                                                  
                                                                                                                                                                                            ^^}}eeff{{}}}}CCCC}}}}ii}}llllLL--                                                                                                                                                                                                                                                                                                
                                                                                                                                                                                          ,,<<ff[[IIffCCiiCC}}{{CCCCII3311ii==                                                                                                                                                                                                                                                                                                
                                                                                                                                                                                          ''rrIIttCC{{CCiiCCff{{}}{{CCCC1133++                                                                                                                                                                                                                                                                                                
                                                                                                                                                                                          ''>>IIII{{CCffCC{{CCCCCCCCCCCC33uuvv''                                                                                                                                                                                                                                                                                              
                                                                                                                                                                                            ,,CCttffCC}}}}CC{{CCCC}}CCiiCCtt{{;;``                                                                                                                                                                                                                                                                                            
                                                                                                                                                                                            --ssttll}}{{CC}}CCCCCCCCCCCC{{ttuuLL``                                                                                                                                                                                                                                                                                            
                                                                                                                                                                                              ==77ttttCC{{CCCCCCCCCC{{}}{{33nnii,,                                                                                                                                                                                                                                                                                            
                                                                                                                                                                                              ``^^||uutt}}{{{{}}}}CC{{CC{{}}33}}++                                                                                                                                                                                                                                                                                            
                                                                                                                                                                                                ``>>FFlltt}}{{CC{{CCCCCCffIIIIttLL''                                                                                                                                                                                                                                                                                          
                                                                                                                                                                                                  ``<<CC[[11}}CC{{ff}}FF}}ffii11ffrr``                                                                                                                                                                                                                                                                                        
                                                                                                                                                                                                    ``??uunnttII}}ff}}}}iiffIIIIllvv``                                                                                                                                                                                                                                                                                        
                                                                                                                                                                                                      ::77uuoonnnn33CC331111ff}}[[ff>>..                                                                                                                                                                                                                                                                                      
                                                                                                                                                                                                      ``<<iieenn11II33llffCC}}11uullLL``                                                                                                                                                                                                                                                                                      
                                                                                                                                                                                                        --//uuoooouu113311tt33{{llooII;;''                                                                                                                                                                                                                                                                                    
                                                                                                                                                                                                          ^^FF[[nneeoouu333333}}fflltt??``                                                                                                                                                                                                                                                                                    
                                                                                                                                                                                                          ``**11nnuu[[uullIIffCCCC}}}}77,,                                                                                                                                                                                                                                                                                    
                                                                                                                                                                                                            ,,ttaaooeennnnIIffCC}}3333II!!``                                                                                                                                                                                                                                                                                  
                                                                                                                                                                                                            ``JJ22YYoonnnntttt11{{ff{{IILL--                                                                                                                                                                                                                                                                                  
                                                                                                                                                                                                            ,,^^11YYeeeeeeuuuu3333ffii11CC^^                                                                                                                                                                                                                                                                                  
                                                                                                                                                                                                              ``!!11eennoo[[uuttII33}}ttIIcc``                                                                                                                                                                                                                                                                                
                                                                                                                                                                                                              ''^^nn]]oonnllttttCC}}{{}}ll{{::                                                                                                                                                                                                                                                                                
                                                                                                                                                                                                                --CC22YYeennnn[[33ttttff33CC^^                                                                                                                                                                                                                                                                                
                                                                                                                                                                                                                  LLyy]]ee[[nntt1133}}ii33llzz``                                                                                                                                                                                                                                                                              
                                                                                                                                                                                                                  ==uuEEYYeeee[[331133}}}}ff77,,                                                                                                                                                                                                                                                                              
                                                                                                                                                                                                                  --JJ2222ZZ[[tttt11}}CC}}ffff!!,,                                                                                                                                                                                                                                                                            
                                                                                                                                                                                                                  ..>>ooEExxeeee11ttII}}{{{{ttss``                                                                                                                                                                                                                                                                            
                                                                                                                                                                                                                    ^^llwwjj[[ee[[IIIICC{{CC11ii::                                                                                                                                                                                                                                                                            
                                                                                                                                                                                                                    ..iiSSyyoonnttllnn11}}CC11II!!''                                                                                                                                                                                                                                                                          
                                                                                                                                                                                                                      zzaaSSyy[[eenntt{{iiCCttuuLL--                                                                                                                                                                                                                                                                          
                                                                                                                                                                                                                    ''>>YYPPaannuuZZnn33}}CC33nnii__                                                                                                                                                                                                                                                                          
                                                                                                                                                                                                                      ::nn6655eeoooo33ii{{}}CC33}}>>                                                                                                                                                                                                                                                                          
                                                                                                                                                                                                                      --{{kkYYeeuuoo[[ttffCCCCtt33??``                                                                                                                                                                                                                                                                        
                                                                                                                                                                                                                        **yyEEZZoonnII}}CCCCCCllooCC::                                                                                                                                                                                                                                                                        
                                                                                                                                                                                                                        ==uuwwyyoonnttttII}}{{33uuII;;``                                                                                                                                                                                                                                                                      
                                                                                                                                                                                                                        --CCwwaaeeoouu11ff{{CC}}33ll??``                                                                                                                                                                                                                                                                      
                                                                                                                                                                                                                        ``((SSEEyyZZll33II}}}}{{ttYYFF..                                                                                                                                                                                                                                                                      
                                                                                                                                                                                                                          //jj22ZZ[[uuttffCC}}CC{{nnCC==                                                                                                                                                                                                                                                                      
                                                                                                                                                                                                                          <<ZZSSxxooZZoo33{{{{}}33uuII!!``                                                                                                                                                                                                                                                                    
                                                                                                                                                                                                                          ==eePPjj[[nnnnlltt3333lluu[[77--                                                                                                                                                                                                                                                                    
                                                                                                                                                                                                                          ''ll66aannnn[[uull}}CCIIttooCC::                                                                                                                                                                                                                                                                    
                                                                                                                                                                                                                          --ffkkYYnnooeeeenn1111ttuu[[ff!!                                                                                                                                                                                                                                                                    
                                                                                                                                                                                                                          ``FFwwaa55ZZnnnnnn11uu33ttuuoo{{__                                                                                                                                                                                                                                                                  
                                                                                                                                                                                                                          --))]]wwxxeeooooeennnn11lleeuutt!!''                                                                                                                                                                                                                                                                
                                                                                                                                                                                                                          ``??]]qqyyoo[[[[nneeeeuulleeee55||--                                                                                                                                                                                                                                                                
                                                                                                                                                                                                                            !!jjSS]]jj55ooooee[[ttttnneeaa33==                                                                                                                                                                                                                                                                
                                                                                                                                                                                                                            ^^5566qq22jjoonneeee[[eeeennjj11==                                                                                                                                                                                                                                                                
                                                                                                                                                                                                                            ,,eekkaa5555YYYYZZnnZZooeeeexx[[//--                                                                                                                                                                                                                                                              
                                                                                                                                                                                                                            ''11qqkkkkwwyyZZZZZZ55eeooZZZZYYll>>``                                                                                                                                                                                                                                                            
                                                                                                                                                                                                                            ..FFwwEEyyyy]]SSEEaa]]aajjxxxxyy]]IIzz<<::,,::....``,,                                                                                                                                                                                                                                            
                                                                                                                                                                                                                            --))SSPP66qqSSwwqqEESSqqaa22PPEEkkqqaajjjjjjoottCC||zz++::..''                                                                                                                                                                                                                                    
                                                                                                                                                                                                                          ''::}}PP4499kkPPPPqqqqqqqqkkkkSSkkddppbbXXKKAAXXmmmmGGkkjj[[ii??!!^^--''                                                                                                                                                                                                                            
                                                                                                                                                                                                                          ,,JJEEOOOO44ddppOOppOO4444ppOOVVVVppGGVVAAHHKKUUUUKKAAKKbbUUGG99EEZZ33??;;--                                                                                                                                                                                                                        
                                                                                                                                                                                                                  ``..;;))uu44AAKKGG44VVGGbbOOGGOOOOOObbbbOOppOOOOOOUUbbOOGGbbOOKKAAUUbbppVVppOO66aaII//::                                                                                                                                                                                                                    
                                                                                                                                                                                                            ``::;;TT}}5544GG88mmUUAAAAUUGGppGGAAUUbbOOGGUUGGOOUUbbVVppOOOOUUUUGGGGbbbbGGOO44VVppGGOO992211zz::                                                                                                                                                                                                                
                                                                                                                                                                                                        '';;LLllaaVVKKHHHHAAUUXXHHAAbbUUXXmmHHUUAAAAbbbbUUGGOOAAKKGGUUUUppOOppppAAUUppddVVOOppVVVVddddppdd22IIzz::                                                                                                                                                                                                            
                                                                                                                                                                                                      ::77jj66mm88XXHHmmUUHHXXKKKKXXHHKKbbUUXXXXbbbbUUAAAAGGGGUUOOVVUUUUGGbbbbAAbbUUbbppVVppOOVVOOppVVVV44pphhEE}}!!                                                                                                                                                                                                          
                                                                                                                                                                                                  ''ccllVVRRmmKKXXUUXXmmAAHHmmHH88HHXXXXUUUUKKKKbbbbAAKKUUUUUUUUKKUUKKbbGGbbVVppOObbUUGGddddVVVV4444ppppppppppddhhYYvv::                                                                                                                                                                                                      
                                                                                                                                                                                                ,,77kkXXRRDD88KKKKHHHHXXmm88mmmmmmAAAAXXKKKKXXXXAAUUAAUUOOGGUUbbAAbbAAGGOOUUGGKKbbppbbbbppppOO44VVVVpppp4444ppppdd99ww33>>                                                                                                                                                                                                    
                                                                                                                                                                                              >>tt99DD$$RRmmHHmmXXXXKKXX8888mmmmHHHHXXGGHH88GGXXHHUUbbbbAAAAbbUUAAUUAAUUUUbbppAAbbGGUUOO44pp4444ppVVOOVV4444ddVV4466OO44nncc``                                                                                                                                                                                                
                                                                                                                                                                                          ''!!jjXXggBB88HHXXHH88mmmmHHHH88mmXXHHUUmmKKUUAAbbUUHHUUGGAAKKUUbbbbUUUUbbUUUUUUGGOOUUppOOUUOOVVppppppVV44VV449999hh99VV44ppVVhhaaTT__                                                                                                                                                                                              
                                                                                                                                                                                        ``zzaaHHBB$$RR8888mmHHmmmm88mmmmmmHHXXXXmmmmUUUUAAUUUUAAmmUUUUUUbbAAAAAAUUUUUUKKbbOOGGUUGGbbKKbbVVVVVVVVppVVpp449999hh9966dd44444444aa77__                                                                                                                                                                                            
                                                                                                                                                                                      ,,ssPP88gg##DD888888mmmm88HHmmmmmmmmmmHHmmRRXX88KKbbUUbbHHbbAAmmHHAAUUUUXXKKAAUUAAbbOOAAbbppppbbGGVVVVppOOppVVVVddhhhhPP6699ddhh9999dddd]]))::                                                                                                                                                                                          
                                                                                                                                                                                      <<EEDDBB$$$$##DD88mmmmmm88HH88mmmmmm8888RR88GGUUbbbbKKGGGGbbUUbbbbmmmmUUKKXXAAGGUUUUUUAAOOAAUUUUUUOOVVVV44pp444499hhhhPPPPhhdd66666699OOdd22))''                                                                                                                                                                                        
                                                                                                                                                                                    ++55XX####88RR8888mmmm8888HHHH88mmHHmmmmmmmmmmRRRR88KKUUXXXXAAXXKKbbUUUUUU88UUAAAAAAUUbbUUGGppVV44ppppVVppVVpp44VVddhhhhPP66PP6666PPPPPPddVVppaa??``                                                                                                                                                                                      
                                                                                                                                                                                  ::[[HHggggggDD$$RR8888HHmmRR8888RRRR888888mmmm8888HHXXKKKKKKbbXXKKKKKKKKXXUUUUGGAAKKAAUUUUbbUUAAbbOObbGGppOOppVVddddhh6666kkPP99PPPP6699hh66dddd66eecc''                                                                                                                                                                                    
                                                                                                                                                                                ..((99BBBBBBRRRR88mmRR##88mmRRRRDD$$##DD##DDRR88mmHH88AAKK88AAKKHHUUHHXXbbAAHHKKAAUUUUGGKKmmUUUUGGOO44OOpp44pp44pp444499hh996666PPPPhhPPPP66PP99hhhh66nn>>                                                                                                                                                                                    
                                                                                                                                                                                ssdd88BB00##RR8888RRRRRRRRDDDDDDBB$$$$BBBBBB##DD88mmmm88mmmmmmmmAAKKmm88mmAAAAHHUUKKAAUUUUAAAAppOOGGGGppVVVVppVVppdd9999hhPPPP66666666666666PPPPkkPP66SSll==                                                                                                                                                                                  
                                                                                                                                                                              <<aamm$$BB##RRDDRR88mmmmRR$$BBBB$$ggBBBB$$$$$$gggg$$RRmmmm88888888HHmmmmmmmmmmXXAAAAXXKKUUbbbbAAUUOOOOppVVVVppppVVpp4499hhhhPPPP6666666666PP66PPkkkkqqkkhh]]((''                                                                                                                                                                                
                                                                                                                                                                            ..ttKKBBBBBBRRDD##mm888888##$$BBBB$$##$$BBggBB$$$$$$gg##88mmmm8888mmKKKKHHHHAAKKKKUUUUXXKKKKKKUUUUUUOOVV44VVVVppVVVV44ddhh666666PPPPPPPP66PPkkkkPPSSSS2222PPwwZZ??                                                                                                                                                                                
                                                                                                                                                                            zzhh####$$$$DD##RRHHRRDD##BB$$##$$ggggggggBB$$$$BBBB$$$$##RRmmHHmmmm88KKmmRRHHKKHHHHXXHHbbGGAAUUUUKKOOppVVVVppVVVVpp999966PPPPPP66PPPPPP66kkSSqqqq22EEEE22qqqqqq[[;;                                                                                                                                                                              
                                                                                                                                                                          ::YY##BBBBBBRRRRDD8888RRDD$$BBBBBB00NNNNMMMMMM0000gggg$$BB$$DD88mmmmmm88UUAAHHKKKKKKUUUUKKUUAAmmAAbbbbOOppVVppppVVVVpp999966PPPP6666PP66PP66kkEEwwqq22EEwwEESSwwqqEE{{==                                                                                                                                                                            
                                                                                                                                                                          zz99DD$$ggBBmmRRRR88RRDDDDDD$$gg00MMNN00MMMMNNWWMMBB##BBBB##RR888888mmmmHHmmmmHHmmmmKKAAXXUUUUAAUUAAKKOOppVVVVVVVV44VVddddhh666666666666PPPPkkwwqqqqEE22SS]]22EE22EEYYss``                                                                                                                                                                          
                                                                                                                                                                        __nnmmgg##$$$$RRDD88RRRRDD##$$$$ggMMMM00WWNN00ggggggBBBBBB$$##RR888888mmHH88RRRRmmmmmmHHAAKKAAKKKKAAUUGGOOOOppVVVVVV44ddddhhhhhhhh66PP666666PPPPkkqqwwEESSSSEEEEEE2222]]11==                                                                                                                                                                          
                                                                                                                                                                        zz99DD$$gg$$$$DDDD88$$RRDDBB00ggggNNWWMMWWWWNNNNNNMMgg$$##BBBB##88mm888888mmHHmm88XXAAAAmmXXUUUUAAAAUUppbbGGppVVVVVV4499hhPPPP6666PPPP66666666PPPPqqSSEE2222EEEEEEEE22EExxvv--                                                                                                                                                                        
                                                                                                                                                                      __nnmmBBBBBBBB##DD##DDRR##BB$$BB00NNWWWWNNNNNNNNNN00ggBB$$$$BB$$##DD88HHmm88mmmmmmmmmm88mmUUUUXXKKAAbbAAGGOOOOVVVVVV449999hh66PPPPPP666666PPPP66kkqqkkkkSSEE22]]22EEEE22EE2233>>                                                                                                                                                                        
                                                                                                                                                                      !!99DD$$ggBB$$######DDDDBBBB$$BB00NNNNNNNNWWWWWWNNMM00BBBBBBBB$$DDDDRR8888mmmmmmmmmm888888RRUUKKHHAAbbAAOObbbbOOppppVVdd99dd99666666666666666666PPkkkkqqEE2222222222EEEE2222aaii''                                                                                                                                                                      
                                                                                                                                                                    ``CCRR$$$$BBBB##DDDD##DDDDggBBBBBB00NNNNNNNNMMNNNNMM00gg$$$$BB$$RRmm8888mmmmmmmmmm88mmmmmmHHmmmmKKUUUUKKUUGGppOOVV44ppppVV44dd996666PP6666PP666666PPkkkkqqEEEESSSS22]]22EE2222EEZZ<<                                                                                                                                                                      
                                                                                                                                                                    ,,xxDD$$gg$$BBDDRRDDDDRRRRBBBBBBggMMNNNNNNNNWWWWWWWWNNMMggggBB$$DD888888mm88mmmmmm88mmmmmmHHRRUUUU88bbGGAAKKUUAAbbppppppVV44dd9966PPPPPPPPPP66PPPPPPkkkkqqEEEESSEE]]aa]]]]aawwww]]JJ''                                                                                                                                                                    
                                                                                                                                                                    LLVV$$BB00##BB##RRRR##DDDDBBBBggggMMNNNNNNNNMMNNNNMM00gg$$$$$$$$DD8888mmmm8888mmmm88mm8888mmKKRRXXAAUUKKbbUU44OOpp4444ppppOO44dd99666666666666PPPP66PPkkqqEEEE2222]]aayyyyaaaa2222tt++                                                                                                                                                                    
                                                                                                                                                                  ::YY##BBBBBB$$BB##RRRR######BBBBBBggMMNNNNNNNNWWWWWWWWMM00ggBB####RRmmmmHHHHmm88mmmmmmHHHHHHXXmmUUUUHHAAbbbbbbGGUUbbOOppVVVVppVVdd996666PPPPPP66PP6666kkqqwwEEEE22]]22]]yyyy]]222222]]FF::                                                                                                                                                                  
                                                                                                                                                                  !!9900BB$$$$gg##$$DDRRDD####$$$$BB00MMNNNNNNWWMMNNNNMM00BB$$$$BB$$DD8888888888RRmmmmHHKKKKXXKKbbHHKKGGAAAAbbGGVVppVVVVVVVV44VV4499hhPPPPPPPPPP66PP6666kkqqwwSSEE222222]]aaaaaaEEEE22EEtt==                                                                                                                                                                  
                                                                                                                                                                ,,CCmmgggggg$$BB##BB$$DDDDDDDD$$BBBB00NNNNNNNNWWNNWWNNNNMMggBB$$BB##88HHmmmmmmmm88mmmmHHHHHHmmHHUUKKHHUUUUGGbbbbppppVVVVppVV99dddd9966PPPPPP6666PPkk6666kkqqqqwwEEEE22]]aaaaaajjaa22EESSEE((''                                                                                                                                                                
                                                                                                                                                                ::oo##BB##BB$$BBDDRRRRDDDDDD##BBBBggNNWWNNMMNNNNNNNNMM00ggBB$$$$ggDDmm888888mm8888HHKKKKHHKKAAmmHHUUUUUUOOOOpppppp44VVppppVVVVVV99hhPPPPPPPPPPkkwwqqqqwwqqkkqqEEEESS22]]22aajjyy]]EESSEEEEtt^^                                                                                                                                                                
                                                                                                                                                                **66BBBBBBBBggDD$$DDDD######$$BB$$$$00NNNNNNWWNNNNNNMM0000ggBBBBDDDDDDRRmmmmmm88RRHHHHmmmmXXXXmmUUAAXXUUOObbGGVV4444ppppppppVV9999hhPPPP6666PPkkwwqqqqqqkkPPkkwwEEEE22]]]]]]]]22EESSSSEEkkEEvv''                                                                                                                                                              
                                                                                                                                                              ''||HHgg$$gg$$00RR$$##DDDD####$$BBBB$$00NNNNNNWWNNNNNN00ggggBB$$BB88DDDD88HHmmmm88HHUUKKHHAAAAKKAAmmUUUUUUbbGGppVVVVppVV4444OOpphh9966PPPPPP66PPkkkkkkqqqqqqwwSS22SSEE2222]]]]22]]22EESSEE22kkuu,,                                                                                                                                                              
                                                                                                                                                              __ZZ$$BB$$ggRRBBDD##DDRRDD######$$BBBB00NNNNNNWWNNWWWWMMggggBB$$BBDDRR88HHHH8888mmRRmmmmRRmmXXHHHHRRAAbbUUUUOO44VVppppVVddddppppddhh66PPPPPPPPkkqqqqSSSSwwwwSSEEEESSEESSSS]]]]22aaaa22EESSaaqq]]ss''                                                                                                                                                            
                                                                                                                                                              rr66BBBB$$ggRRRR##DDDDDD##$$$$$$$$$$BB00NNNNNNWWWWWWWWMMggggBB$$BB$$DD88mm8888mmmmmm88mmHHmmXXAAXXUUAAXXAAbbGGVV44VVVVppVV4444449966666666PPPPkkkkwwSSEESSSSEEEESSEE22EESS22EEEE]]22EE]]2222SS]]{{..                                                                                                                                                            
                                                                                                                                                            ,,FFKKggBB##BB##mmDDRRDD##$$BBBB$$BBBBggMMWWNNMMNNWWMMMMggBBBBBB$$BB##DDRRRR88mmmmmmmmRRmmHHRR88XXmmUUAAXXAAUUbbVVVVVVVVppVVdddddd996666666666PPPPkkkkwwSSSSSSEE22EEEEEEEE2222EESSEE]]SS22aa22EE22nn;;                                                                                                                                                            
                                                                                                                                                            ''YY88BB$$DD####RRRR88DD##BB$$$$BB$$0000MMWWNNMMNNNN00MMggBBBBBB$$BBDDDDRR88mmHHmmmmmmmmHHmm8888mmmmmmKKUUUUAAbb44VVppVVVVdd9999dd9966666666PPPPkkqqEEEEEEEESSEEEEEE22EE]]yy]]222222yy2222]]EEwwww]]zz,,                                                                                                                                                          
                                                                                                                                                            ++PP##00BBDDRRDDDDmm88DD$$BBBBBBBBBBggBB00NNNNNNNNNNNNNN00ggggBB$$BB##$$DDHHHH8888mmRRmm8888mmmmmmAAAAHHHHUUUUAAppVVVVVVVV44dddd99PP6666PPPPkkqqwwSSEEEEEEEE2222aaaajj]]yyxxyy]]aa]]SS22]]]]EEEEEEwwCC..                                                                                                                                                          
                                                                                                                                                          ,,))dd$$BBggDD88DD8888##DD##BB$$$$BBBB$$BBggMMNNNNMM00gg00MMBB$$BBBBBBDD8888mmHHmm88mmmmRRmmHHmmHHmmHHXXAAUUAAUUOOVVVVVVVV44dd99hh66PPkkkkqqwwwwqqwwEEEEEE22EE22aayyyyaayyyyaaaajjjjaa]]EESSEEEE22EEqq11==                                                                                                                                                          
                                                                                                                                                          ::oo$$BBBBBBRRmmDDRRRR##DD##$$##$$BBBBBBBB00MMNNNNMM00ggggggggBB$$BBBB$$DDRR88mmmm88mm88XXHH88AAKK88AAHHKKAAAAUUOOppppVV44dd99hh6666PPkkqqwwSSEEEE2222EEEEEESSSS2222]]aayyyyyyyyjjjjyy]]22EEEEEE22EEqq]]ss''                                                                                                                                                        
                                                                                                                                                          ^^EEBBBBBB$$88mmRR8888DDDDDD####$$BB$$$$BB00MMMMMM000000ggggggBB$$$$BB$$88mmmmmmmm88mm88AAKK88HHmmRRKKKKAAUUUUbbOOppppVV4499hh66PPPP66PPqqwwwwSSEEEESSEEEE22]]]]aaaayyyyjjjjyyyyjjjjyy]]2222EEEEEEEEwwEEii''                                                                                                                                                        
                                                                                                                                                          //pp$$BB$$##DDRRRR88mmDDRRDDDDDD$$BB$$$$BBgg000000gggg00ggBBBBBBBB$$$$##mmHHmmHHmm888888mmXX8888KKAAUUUUUUUUUUbbOOppppVVddhh6666666666kkqqwwSSEE22EESSSSEE22aaaaaayyxxjjjjjjjjyyjjjjyyaa]]]]22SSEE22EE2211,,                                                                                                                                                        
                                                                                                                                                        ''ff88BBBB$$##DDDDRRRR88RRRRDDDDDD$$BB$$$$BB000000000000BBBBBBBBBBggBB$$##88mm88mmmm888888RR88RRmmAAAAKKAAUUUUUUbbGGOOOOVV9966PP666666PPPPPPqqSS2222EE22EEEE22aaaaaayyjjjjjjjjjjjjjjjjjjyyaa]]22EEEE22EESSxx//                                                                                                                                                        
                                                                                                                                                        ,,jjDD$$$$$$RRRRRR88RR88RR88RRDDRR##BB$$$$BBBBgggggggggggggggg$$$$BB$$DDRR888888mmmm88mm88KKXXRRHHXX88AAAAUUbbGGOOOOOOppVVhhPPPP6666PPPP6666kkSSEESSSSEE2222]]aayyyyjjjjjjjjjjjjjjjjjjjjaaaaaaaa222222SS22]]{{__                                                                                                                                                      
                                                                                                                                                        **ppBBgg##DD88RR88HH88mm88mmRRDDRRDD$$BB$$$$BBBBBBBBBBBBggggggBB$$##RR88mmmmmmmmmm8888XXmmAAAAHHAAAAKKVVbbOOppppppppVV44dd66PPPP66PPPP66kkqqSSEE22EEEEEESSSSEE22aayyjjjjjjjjjjjjjjjjjjjjyyaaaaaa]]aa]]EEEEaaII,,                                                                                                                                                      
                                                                                                                                                      --}}DDBB$$RRDD88DDRRmm88mmmmmm88DDRRRR##BBBBBBBBBBBBggBBBBBB$$$$BB$$DDRRRR888888mmmm88mmAAbbHHmmmmXXUUbbbbVVVVVVpppppp44dd9966PP6666PPPPhh66kkSSSSSSEEEEEEEEEEEE22]]jjjjyyjjjjxxxxxxjjjjjjjjyyaa]]]]aayy]]22SSyyzz                                                                                                                                                      
                                                                                                                                                      ,,22gg$$ggDD88RRRRHHHH88mmmmmm88DDDDRR##BB$$BBBB$$$$BBBBBB##BBggBBDD88mmmm88mmmm8888mmHHXXAAAAUUUUKKAAOOppppppppppppVV44996666PP66666666PPPPqqwwSSSSEEEEEEEE22]]]]aayyyyjjjjjjjjjjjjjjjjjjjjyyyyyyaaaaaaaa22EEyyFF..                                                                                                                                                    
                                                                                                                                                      ++PPggBB$$BBRR8888888888XX8888RRRRRRRR##BB$$BBBB$$BBBBBBBBggBB$$DDDDRR88mm88mmmmmmmmHHXXKKXXHHmm88mmbbppOOVVVV444444VV4499666666PPPPPPPPPPkkqqSSSSSSEEEEEESSEE22]]aaaayyjjjjjjjjjjjjjjjjjjjjxxjjyyaayyyyaaaa]]]]33::                                                                                                                                                    
                                                                                                                                                      LLddDDBBBBmm88RRRRmmmmRRDDRR88RRRRRRRR##BB$$BBBBBBBBBBBBBBBB$$##RRRR8888mm88mmmmmmmmmmHHmmKKXXAAGGbbKKbb44ppVV99hh6666PPqq666666666666PPPPqqwwSSEEEEEEEEEEEEEE]]aaaayyyyjjjjjjjjjjjjjjjjjjaaaaaaaaaaaaaaaaSSEE22YY!!                                                                                                                                                    
                                                                                                                                                    ``II88BB##$$##RRRR88888888mmRR88RRDDDDDD##BB$$BBBBBBBBBBBBBB######DD88HHHHmmmmmmmm88mmmm8888HHmmmm88mmUUppppppVVddhhhhhhhhhh66666666666666PPqqwwSSEEEEEEEEEEEE22aaaayyyyjjjjjjjjjjjjjjjjjjjjjjaaaajjjjyyyyxxxx]]EE]]??                                                                                                                                                    
                                                                                                                                                    ,,aagggg##DDmmmmmm88DD######RR88RR######$$BB$$$$BBBBBB$$$$$$DD####DD88mmmmmmmm88mmmmmmmmmmmmmmRRHHOOppbbGG44ddddhh66PPPPPP6666666666PPPPkkkkqqwwSSEEEEEEEEEEEE22aaaaaayyyyjjjjjjjjjjjjjjjjyyyyaa]]]]]]]]]]aaaaww22jj((__                                                                                                                                                  
                                                                                                                                                    //44DD$$BBRR88DDDDRR88mmmmRRRR88RRDD####$$BB$$$$BBBBBB$$####$$##DDRR888888mmmm88mmmmmm88mmmm88XXUUKKXXUUppVVppVVddhh66PPPP6666666666PPkkqqqqqqqqSSEEEEEEEEEE22]]yyyyyyyyyyjjjjjjjjjjjjjjjjjjyyjjyy]]]]jjjjaayyEESSSS33,,                                                                                                                                                  
                                                                                                                                                  ``llRRBBggBB888888HHHHRRDD8888RR8888DD########BBBB$$BBBB##DD######RRRRRR888888mmmmmmmm888888HHHH88mmbbVVVVVVpp99hhPPPP66666666PP666666PPkkqqqqqqqqwwEEEEEEEEEE]]aajjjjyyyyjjxxjjjjjjjjjjjjjjyy]]aa]]22]]yyyy]]22aa22PPjjrr                                                                                                                                                  
                                                                                                                                                  ==EEDD$$BBRRDDRR##RRmm88mmHHRRDD8888RRDDDDDD##BB$$$$BB$$##DDDDRRDD##RR88mmmm88mmmmmmmmmmmmHHAAAAAAUUbbGGVV44ppVV99PP666666PPPPPPPP666666PPkkkkwwwwSSEE22EEEEEE]]aayyjjyyyyjjjjjjjjjjjjjjjjjjyyYYaa]]yyyy]]22aa]]EESS22yyLL''                                                                                                                                                
                                                                                                                                                  rrhhgg$$BBRRRR88mmmm88RR88mmmmRRDDDD$$##DD##$$##$$$$####RRRRDDDDDDDDRR88mmmm8888mm88HH88mm88HHUUOOppOOppVVVVppdd66kkPP66PPPPPPqqPP6666PPqqSSEEEEEEEE2222EEEE22]]yyyyyyjjjjyyxxjjjjjjjjjjjjyyyyaayyaaaa22aa]]22aa]]qq]]]]ii--                                                                                                                                                
                                                                                                                                                  ((OOBB##BB####DDRR888888RRRRDD88RRRRDDRR88DDRRRR########DDRRRRDDRRRRDDRR88mmmmmmmm88mm88mmXXUUAAGGGGGGppppppOO44hh6666hh66PPPPqqkkPP6666PPkkqqSSSSSSEEEEEEEE2222yyyyaajjjjjjxxjjjjjjjjjjjjyyyyjjjjaayy]]aa]]]]aa]]SS]]22II..                                                                                                                                                
                                                                                                                                                ::jj$$00$$$$DDRRRRRR88RR8888mmmm88RRRRRRRRRRDDRRRR##$$$$##DD8888DDRRRRRR8888mmmmmm88mmmmHHmmXXAAGGOOOOpp4444VVVVddhhPPkkPPPPkkqq6666666666PPqqwwSSSSEE22EEEE22]]22yyyyyyjjjjjjjjjjjjjjjjjjjjyyyyyyyyaayyaayyaayyyyjjaa22qq55;;                                                                                                                                                
                                                                                                                                                >>hhggBB$$##DDRRRRRRDD##DDRRmmmmRRRR88DDDDRR##DDRR########RRmmmmDDDDRR8888mmmm8888RRmm88HH88XXKKbbGGGGppVVppppVVVV44996666hh66PPwwqqqqqqkkPPkkwwSSSS2222EEEE22]]aayyjjjjjjjjjjjjjjjjjjjjjjjjjjyyyyaa]]jjyyyyaajjyyjjyyEESSjjrr                                                                                                                                                
                                                                                                                                                zzOO$$$$BBDDDDDDDDDDRRRRRRRRRRRRRR88HHDDDD88RRDD88RR8888RRRR88RRRRDDDDRR88mmmm88XXmmHHmmKKXXbbppOOppppVV44VVVV9944ddhhPPkkPP66PPhhhh6666PPkkww22wwSSEEEEEESSEE]]yyjjjjjjjjjjjjyyjjjjjjjjjjjjjjyyjjyyaajjyyyyaayyjjyyaaEEaajjTT``                                                                                                                                              
                                                                                                                                              ''ll##ggBBggDDRRDDRRRRRRDDRRRR88RRRRmmHHRRRRmmmmRR88RR88mm888888RRmmRR##DD88mmmmmm88RR88mmXXHHKKUUOOppppppppOOpp44VVdd996666666666PP6666666666PPqqSSSSEE22EEEE22]]yyyyjjxxyyjjxxjjjjjjjjjjjjjjjjyyyyjjyyxxyyjjyyjjjjjjyyEEEE22CC..                                                                                                                                              
                                                                                                                                              ^^wwBBBBBB$$##DDDDRRDD##$$##DD88RRRRRRmm8888mm88mm88RR8888RR888888mmRRDDRR88mmmmmmmmmmXXUUbbUUAAUUppVV44VVVVVVVVdddd99hh66PPkkkkPPPP66PPkkqqwwSSEEEEEEEE222222]]aaaaaajjjjyyjjxxyyyyyyyyjjjjjjjjyyyyjjjjxxyyxxjjjjyyjjxxaaEESSll==                                                                                                                                              
                                                                                                                                              ccVVMMggBB$$##DDRRRRRRRRRR88mmmm88mmDD88mmHHmmRRmmmm888888RRRR8888RRRRRRmmmmmm88888888mmUUAAUUAAGGOOppppppVVVVppVVVVdddddd9966PPhh666666PPqqqqqqqqEEEEEEEE222222]]aa]]yyjjaayyjjaayyyyyyjjjjjjjjjjyyxxjjjjaajjyyaaxxyyjjyyEEww]]??''                                                                                                                                            
                                                                                                                                            ..((GGBBBBBBBBggRRDD$$88mm88RRRRRR##88mmHHmm8888mmmm88RR88RRRR88DDRRRR8888mmmmmm8888mm88AAKKbbUUAAppAAOOVVppOOVV44VV44dd99ddddhh66hh66666666qqqqwwEEEEEEEEEE2222]]]]aayyyyaayyjjjjjjjjyyyyjjxxjjjjjjjjjjjjjjjjjjjjjjxxyy]]22]]ww22ii--                                                                                                                                            
                                                                                                                                            ::jj$$MM$$$$####RR##DDRRRRRRRRRR8888RR88mmmm88mmmmmmmm8888RRRRRR##DD888888mmmmmmmmmmmmmmHHHHAAAAUUUUGGppOOGGGGppppppdd444499hh99hhPPPPPPPPPPkkkkkkSSEEEEEESSEE22]]]]]]aayyjjjjxxjjjjyyyyyyjjjjjjjjjjjjjjjjjjjjjjjjyyjjjjyyEE22wwEEtt,,                                                                                                                                            
                                                                                                                                            **GGMMgg$$BB$$##RR##RRDDDDRRRRDDRRmm88mmHHmm88888888mmRR88RRRRRRDDDD88888888mmmmmmmmmm88GGXXXXUUAAGGbbGGUUUUppVVVV44dd44449966hh66PP66PP6666PPPPkkSSEE22EEEEEE22aaaa]]aayyjjjjxxjjjjyyyyyyyyyyyyjjjjjjjjjjjjjjjjyyyyyyjjyyEEEESSwwjj<<                                                                                                                                            
                                                                                                                                          ''IImmgg####00BB$$##DD88DDDD8888RRRR88RR88mmmmmmmmmm8888RR88RRRR88DDRR888888mmmmmmmm88mmmm88XXbbUUppKKOOppGGGG44VVOOpp44dddddd99PPPP66hh666666kkkkkkwwSS2222EEEE]]aaaaaaaaaaaaaayyyyyyaayyyyyyjjjjjjjjjjjjjjjjjjjjyyyyyyyyyy22SSSSqqSSvv``                                                                                                                                          
                                                                                                                                          ,,EE0000$$BBBB##DD$$DD8888RRRR8888RRRRDDRR8888mmmmHHHHHH88mmRRRRRRDDDDRR8888mmmmmmmmmmXXRRbbAAAAUUUUppOOppOOOO44VVOOVV44ddddddhhPPPPhh66PP66PPqqkkkkkkqqSSEESSEE22]]]]aaaaaa]]]]]]aayyaayyjjjjjjjjjjjjjjjjjjjjjjjjyyyyjjyy]]]]EEEEqqkkCC''                                                                                                                                          
                                                                                                                                          **GGMMggBBgg$$DDDD##RR8888RRDDRR8888RRRRRRRR88mmmmmmmmmmRR88RRDDRRDDRRRR88mmmm88mmmmHHbbAAmmAAOOAAppbbGGOOGGGGppVVVVdd99dddd66PP666666666666kkwwwwqqqqkkSSEEEEEE22]]]]]]aaaa]]]]]]aaaaaayyjjjjjjjjjjjjjjjjjjjjjjjjyyyyyyyy]]]]22EEww66ll==                                                                                                                                          
                                                                                                                                        ..33mmgg$$$$gg####DD##RR888888RRRR88888888RRRR8888RRRRRRRRDDRRDDDD88RR88RRmmmm8888mmXXAAXXKKUUAAbbAAbbppVVVVVVpppppp44ddhh99hhPPPP6666PP66PPPPqqSSSSwwSSqqSSEEEEEE22]]aa22]]aa]]]]]]]]]]aajjjjjjjjjjjjjjjjjjjjjjjjjjyyyyyyjj]]2222EEwwhhaarr                                                                                                                                          
                                                                                                                                        ==22##NNgg$$$$DDDD88DDRRRRRR88mmmm88RRRRRRDDRRRR888888mmHH8888RRRRmmRR888888mm8888mmKKbbXXbbUUGG44pp44VVppVVVVVVVVddhhhhhhPPPP6666PPPP66PPqqqqqqSSwwwwSSwwwwSSEEEEEE22]]22]]]]]]2222]]]]yyjjjjjjyyjjjjjjjjjjjjjjjjjjjjyy]]xxaaEE22EEww99kkvv''                                                                                                                                        
                                                                                                                                        zzppNNBB$$BBgg$$DDRRRRRR88mmmmRRDDRR88RRmm88RRRR888888mmmm8888mmmm8888mm88mm88HHmmXXbbKKbbGGOOVV4444VVppVVVVpp4499ddhh66PP66PPPPPPPPPPPPSSSSqqwwSSSSEEwwwwSSEEEEEEEEEESS22aa22EEaayy]]yyaayyjjjjjjjjjjjjjjjjjjjjyyyyaaaayyyy]]EEEE22SS9966}}''                                                                                                                                        
                                                                                                                                      ''11##WWgg$$BBBB##DDDDRRRRRR88mmmmmmmm88RRRRDDDDRRRRRRRRRRmmmmmmmmmmmm8888mmXX88RRAAUUXXbbbbbbGGppppppppppVVVVVVddhh99PPPPPPPPPPPPPPkkqqqqqqwwwwEE22EEEEwwSSEE22EE22222222aayy]]EE]]22SS22aayyjjjjjjjjjjjjjjjjjjjjyyyyyyyyyyjjyyaa22EEww99hhuu::                                                                                                                                        
                                                                                                                                      ==]]gggg##$$BB$$##DDRRRRRRmmmmmm88mmmmmm88mm888888mm88mmmm88mmmm88mmHHmmmmRR88mmRRHHHHRRbbOOOOppVV44VVVV44ppVVppddhh99666666666666PPkkqqwwqqwwwwEEEEwwwwwwwwwwSSSSSSEEEESSSS]]22EE]]]]22]]yyyyjjjjjjjjjjjjjjjjjjjjyyyyjjjjaayyaaaaEESSww6699jj>>                                                                                                                                        
                                                                                                                                      **VVQQMM00$$BB$$##RRRRRR88HHmm888888mmmmmm88RRRRRRRRRR88mm88mmmmmmmmmmmmHHUUHHXXXXmmKKUUGGbbOOppppppVVVVppVV44dd66PP66PPPP66666666PPkkqqwwSSSSwwEESSkkwwSSwwSSSSSSSSEE222222yyyyyyjjyyaayyyyjjjjjjjjjjjjjjjjjjjjjjjjjjjjxxyyjjyyyy]]2222SShhww??''                                                                                                                                      
                                                                                                                                    ``IImmWWgg$$BB$$##DDRR88mmmm8888mmmmHHmmmm88mm8888888888mmHHHHmmHHXXHH88mmXXUUbbUUKK88mmOOppGGVV44pppp4444VVVVdd9966PP66kkPPPPPPPPPPPPkkwwSSSSSSwwSSSSqqSS22SSSSSSSSEEEE222222yyjjjjjjjjjjjjjjjjjjjjjjjjjjjjyyjjjjjjjjjjjjxxxxjjyyjj]]22EEwwhhPPFF--                                                                                                                                      
                                                                                                                                    ==wwMMWWgg$$BB$$DDRR8888mmmm88888888mmmmmmmmDDDDRR8888RRRRRRmmmmHHKKXXmmHHAAmmGGAAXXKKHHKKUUGGppppbbbbppVVOOpp44ddhh6666PP666666PPPPkkqqwwSSwwwwqqSSSSqqwwwwwwwwwwSSSSEEEE2222]]yyyyaayyjjxxjjjjjjjjjjjjjjjjyyyyjjjjjjjjjjjjjjjjyyjjaa22EEkk99ddnn==                                                                                                                                      
                                                                                                                                    ssOONNggBBBBBB##RR8888mmmmmmmm88RRRRRRRRmmHHmmmmmmHHHHmm88mm88HHXXXXXXKKUUUUUUGGKKUU44VVbbAAVVVVVVpppp4444444499hhPPPP66kkPP6666PPkkqqqqSSSSSSSSqqwwSSwwwwkkEESSSSEE222222]]jjjjxxxxyyjjjjyyjjjjjjjjjjjjjjjjyyjjjjjjjjjjjjjjjjxxjjjjaa]]]]SS9944EEss                                                                                                                                      
                                                                                                                                  __xxBBWWBB##BB$$DD8888mmmmmmmmmm8888mm88RRRRmmmmmmmmmm888888mmmmKKKKXXXXUUUUXXbbAAOOGGAAUUGG44VVpppp4444VVppppddhhhhPP66hhPP66PPPPkkqqwwwwSSSSSSSSwwwwEEEEEEwwwwwwwwSSEESSSSEE22]]yyjjjjjjxxyyjjjjjjjjjjjjjjyyjjjjjjjjjjjjjjjjjjxxyy]]EESSEEwwhhdd99CC--                                                                                                                                    
                                                                                                                                  ccppBBMM$$$$$$DDRR88mmmmmmmm88mmmm88mmmmmmmm88mmHHmmmmKKAAXXHHAAAAAAKKXXKKbbppUUGGVVVVppVVVVppppVVVVVVVVppppppdd66PPPPPP66PPhh66PPPPkkwwEESSwwqqwwSSSSSSSSSSEE22EEEEEE22EEEE22aaaaaayyjjyyjjjjjjjjjjjjxxjjjjyyjjjjjjjjjjjjjjjjjjjjaa]]22EEEEEEPPddOOZZ^^                                                                                                                                    
                                                                                                                                --}}DD00BB$$ggBB##DDRR88mmmmmmmm888888mmmmmmHHHHmmXXXXXXAAAAAAbbKKAAUUUUUUUUGGGGGGpp44VVppVVVVppppVVppppVVVVVVdd44hh66666666kk6666kkwwwwwwSSEEEEwwwwSSSSSSSSEEEE22EEEEEEEEEEEE22aaaayyjjjjjjjjxxjjjjjjjjjjjjyyyyjjjjjjjjjjjjjjjjxxxxyyaa]]222222qqkkVVPPLL--                                                                                                                                  
                                                                                                                                ,,aaWW00BBBBgg##DDRRmmmmmmmmmm888888mmmmmmmmHHXXKKAAUUAAAAXXKKUUbbGGOOVV44VV44VVppVV44VVppVVVVppVVVVppppVVpppp4466kkPPPP6666PP66PPkkqqqqwwSSSSSSSSSSSSEEEEEEEEEE22EEEEEE2222]]]]aaaayyjjxxjjjjjjjjjjjjjjjjjjjjyyjjjjjjjjjjjjjjjjjjyyaa]]EESSSSSSqqSS6644{{``                                                                                                                                  
                                                                                                                                rrVV0000ggBB$$DDRRmmmmmmmmmmmm88mmmmmmmm88mmHHKKXXKKKKKKUUUUAAbbAAUUbbGGOOOOOOOOppVVppppppVVVVppppVVVVVV44VVVV4466PP66PPPPPPwwqqqqqqwwwwSSEESSSSEEEESSEEEEEEEEEEEEEEEE22]]]]aayyaayyyyjjjjjjyyyyjjjjjjjjjjjjxxjjjjjjjjjjjjjjjjjjxxxxjjyy]]EE2222qqkkhhOO55;;                                                                                                                                  
                                                                                                                              --3388ggggggBB$$##DDRR88mmmmmmmmmm88mmmmmm88mmHHKKXXAAAAAAUUUUUUGGbbGGppVVVVVVVV44ppppppppVVVVVVppppVVVVVVdddd99hh66PP6666PPPPqqkk66qqSSSSwwqqSS2222EESSEEEE22EEEESSEE22]]]]aaaaaayyyyyyyyjjjjyyyyyyyyjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjyyyy]]EESSSSEEEEkk66ddqq77``                                                                                                                                
                                                                                                                              ==22MMMMggBBBBBBDDRR88mmmmmmmmmmmm88mmmmmmmmHHKKAAKKOOOObbAAAAUUbbbbOOVVVVVVVVVVVVVVVVVVVVVVVVVVppVVVVpppp44dd9966kkkk66PP66hhPPPP66kkwwSSwwwwSSEE22EEEEEEEEEEEEEEEE22]]]]]]aa]]]]aaaayyyyjjjjjjyyyyjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjxxxxjjyy]]222222kkhhhhddppoo^^                                                                                                                                
                                                                                                                            ''LLppNN00ggBB$$$$RR88mmmmmmmmmmmm88mmmmmmmmHHXXKKAAXXGGOObbUUGGOOOOGGOOVV444444VVVVVVVVVVVVVVVVppVVVVVVppppVV44ddhh6666hh66PPPPwwwwqqqqwwSS222222EE22EEEEEEEEEE2222EE22]]]]]]aa]]]]]]]]aayyjjjjjjyyyyjjjjjjjjjjjjjjjjjjjjjjjjjjyyyyyyyyyy]]EESSSSEESSkk66VVbbqqTT..                                                                                                                              
                                                                                                                            --uu$$NNBBBB$$$$BBDDDDRR888888mmmmmmHHHHHHHHHHXXKKKKUUppppbbbbppppbbppppppVV444444VVVVVVVVVVppppVVVVppVVppVVddddddhhPPPPhh666666qqqqkkkkwwSSwwqqwwSS2222EEEEEEEE222222]]]]]]]]aaaa]]22]]aaaayyjjyyaaaayyyyjjjjjjjjjjjjjjjjjjjjjjyyyyyyjjjjaa]]22EE22qqPPPPddppVVZZ==                                                                                                                              
                                                                                                                            rrppNNgg$$BBBB##RRDDRRmmmmmm8888mmmmAAKKKKAAAAUUbbGGppGGbbOOOOOOOOVVppppVVdddd44VV44ppVV44VVVVVVVVVVVVVVhh99dd99dd66PP666666PPkkqqqqkkwwSSwwwwSSEEEEEESSSS222222]]aa2222]]]]22]]aa]]yy]]yyjjyyjjjj]]yyyyyyjjjjjjjjjjjjjjjjjjjjjjjjjjjjyyaa]]22EEEE22wwPPhh44VVbbqqTT``                                                                                                                            
                                                                                                                          __II88MMBB$$BB$$DDRRRRmmmm8888HHXXHHmmAAXXHHXXUUUUAAAAGGGGOOGGGGOOppVVpp44ddhh669944VV4444ppppVVVVpppp44OOVVppVVdddd66666666PPPPkkqqqqPPqqSSSSSSEEEESSEEEEEE2222]]]]aa]]]]aa]]EE22]]22aa]]]]aa]]aayyaayyyyyyyyjjjjjjjjjjjjjjjjjjjjjjjjjjjjyyaa]]EESSSSPPPPPPhhhhppVV[[,,                                                                                                                            
                                                                                                                          <<66BBggBB$$$$DDRR888888mmHHmmmmHHKKKKAAKKAAUUUUbbbbOOGGppVVOOGGVV444444ddddddhhhhdd4444ddVVppVVVVppVVVVVV999966PP6666666666PPkkkkqqqqwwSSEEEEEE22222222EEEE222222]]]]]]]]aa]]EE22aaaayyaaaaaa]]yyxxjjyyjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjyy]]EESSSSPPqqkk66PPhhGGSSLL``                                                                                                                          
                                                                                                                        ``))bb00BBBBBBDD888888mmRRmmHHHHmmHHKKAAXXKKAAUUUUUUGGVVOOVVddVVVVdddddddd9999ddhh66hhhhVVddddVV44ppppddppVV44VVVVddhhPPPP6666PPPPPPkkqqwwwwwwwwSSEEEEEE22EEEEEEEEEEEE222222]]]]22]]aaaa]]aaaaaaaayyjjyyjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjaa]]EEEE22wwSSwwPPqqkk4499tt::                                                                                                                          
                                                                                                                        <<aagggg$$BBBBRRmmmmmmmm888888mmXXKKAAKKUUAAAAUUbbGGGGOOppVV4444dddd444444hh6666PPPP66hh449999dd44ppppdddd999999ddhhPPkkPP666666PPPPkkqqkkqqSSEEEEEEEESS22EEEEEEEESSEE22]]222222EE22]]22]]yyyyaaaayyyyyyjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjyy]]22EE22EEEEEESSEESS99OOSS!!                                                                                                                          
                                                                                                                      ''))OOMMBB$$BB##88mmmmmmmmmmHHHHHHXXAAAAAAbbUUbbGGGGOOppVV44VV44dd994444dd99hh6666PPPP6666hh99dd99dd44VVVVVVdd4499hh66PPhh666666PPPP66PPkkqqwwSSEEEEEEEEEEEEEEEE2222EE22]]]]2222]]]]aaaa22aajjyyaaaajjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjyyaa22SSEEEEEE22]]SS66VV44FF``                                                                                                                        
                                                                                                                      ^^aaBB00DDBB$$DD88mmmmmmmmmmXXKKXXHHXXAAUUKKAAbbGGGGGGppdddddddd9999ddddhh6666hhhhPP66hh66PPddddhhdd44dd449999VV44ddhhkkPP666666PPPP66PPkkqqwwwwwwwwwwSSEEEEEEEE2222EE22aa]]22]]]]]]aayyaa]]jjyy22aayyyyyyjjjjjjjjjjjjjjxxjjjjjjjjjjjjjjjjxxjjjjjjjjyy22EEEESSEEEEEEEEwwhhVVoo;;                                                                                                                        
                                                                                                                      TTOOgg$$BBBB##RRmmmmmmmmmmXXXXHHHHXXKKAAAAGGbbGGpp4444444499dd9999dddd996666hhhhkkwwkkhhhh66VV99664444hhddVV99dd999999PP66PP6666PP6666PPqqqqSSEE22EEEEEE22EEEEEE22EEEE22]]]]]]]]]]22]]aa]]aaxxjjaayyjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjxxjjyyyyjjjjaa22EE222222EE22EE66hhSSvv``                                                                                                                      
                                                                                                                    ^^aaDD$$00BBggDDHHmm8888mmmmHHmmHHKKAAKKAAUUbbGGOO4499hh999999dd9999hh66666666PPqqkkqqPPPPhh66hh9999999999999999ddddhhPPPPPPPP6666PP6666PPqqqqSSEEEESSEE22EEEEEESSSSEESSEE]]EE]]]]]]222222aaaa]]yyjjyyaayyYYjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjxxxxxxjjjjjjjjjjyy]]EE]]22EEEEwwqq99oo^^                                                                                                                      
                                                                                                                  --JJOOggBBBBDD##RRmmmmmmHHHHHHXXXXXXAAUUbbGGOOOOOOppdd99dddd999999hhPPkkkkkkPPqqPPPPPPwwwwkk6666hhhh99hhhhhh99hh66666666666666PP66666666PPkkwwwwSSEEEESSEE22EE222222EE22EEEE]]]]aaaa]]]]]]]]aa]]yyjjaaaajjxxyyjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjxxxxjjjjjjjjjjjjyyaa]]aaaaEEEEwwkk44wwvv``                                                                                                                    
                                                                                                                  ;;yy##BBBB$$DD8888mmmmHHXXXXHHXXKKAAAAbbGGOOOOVVppVV99hhddddhhhh66PPkkwwwwwwqq66kkqqwwkkPPPPkk66666666PPPP6666hh66PPPP6666PPPPPPPPPPPPPPPPqqwwSSSSEEEEEEEE22EESSEESSSSEESSSSEE]]aa]]]]]]]]22]]yyaaaayyjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjyyyyyyjjaa2222SSww9944nn^^                                                                                                                    
                                                                                                                __FFbb00$$$$##DDRR88mmmmXXKKXXHHXXAAbbbbbbGGOOGGVVpp44999999hhPPkkkkqqqqwwwwSSwwSSwwkkwwqqSSkkPP66PPPPkkkkqqkkkkhh66kkkkPPPPkkkkPPPPPPPPPPkkqqqqwwwwSSEEEE2222EEEE22EE22]]22EE22aaaa]]]]]]]]22]]yy]]]]yyjjjjyyjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjyyjjaaEEEESSEEkk44SSTT..                                                                                                                  
                                                                                                                ??hhgg00BBDDDDRRRR88mmHHXXKKKKXXKKbbOOOOGGOOOOOOVV44dddd9966PPPPqqqqqqwwwwwwSSSSSSqq66PPhhkkPPPPPPkkkkqqqqqqqqqqkkkkkkkkkkkkkkPPPPkkkkkkqqqqwwwwwwqqwwEEEEEE22EEEE22EE22]]]]22]]yyyyaa]]aaaa]]]]22yyyyaaaaxxxxyyjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjyyyy22SS22SSSSwwhhhh[[,,                                                                                                                  
                                                                                                              ^^55##00$$BBRRRR8888mmHHXXXXKKUUUUUUGGOOOOGGOOVV44VVdd99ddhhPPkkkkqqqqqqwwwwwwwwwwwwSSSSSSqqqqqqwwkkqqqqqqqqqqkkkkqqqqkkkkkkqqkkkkPPkkqqqqwwSSSSwwSSwwEE2222EE22EESSEESSEE2222EE22yyyyaaaayyaaaaaa22aayyyyyyjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjyyaa22EE22EEwwwwqq9922LL--                                                                                                                
                                                                                                            ''FFOOgg$$RR##DDRR88HHXXKKAAAAUUbbGGGGGGOOOOOOpp44dddd9999hh66PPqqqqqqqqwwwwwwwwwwwwSSqqwwww22EEww66qqqqwwqqqqqqqqqqqqqqqqqqwwSSSSwwwwwwSSwwSSEEEESSEESSEE22EEEE22222222EE22]]]]22]]aaaaaa]]aaaa]]aaaaaayyjjjjyyyyxxjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjyyyy]]22]]EEEESSwwPPkkll==                                                                                                                
                                                                                                          --77VV00ggBBDDRRRRmmHHHHXXAAbbbbbbbbGGOOGGGGppVV444499hh999966kkkkqqwwSSSSwwwwwwwwSSSSEESSSSqqwwqqSSwwqqqqwwwwqqqqwwwwSSEEEEEEEE222222EE22EESSSSEEEESSwwqqwwSSwwwwSSEEEEEESSSS2222EE22]]aaaa]]aaaa]]yyaajjxxyyyyYYxxyyjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjxxjjjjaa]]aaEE2222wwqq66]]ss--                                                                                                              
                                                                                                        ''cc22$$BBBB##RR88mmmmHHAAKKAAbbGGGGGGGGVVVV44dd9999dd99hh66PPPPPPwwSSSSwwwwwwwwSSEEEEEE22222222SSSSSSSSwwwwwwwwwwwwwwSSSSwwwwEE]]]]22EEEEEESSSSSSqqkkwwwwqqqqqqqqqqwwEEwwEEEEEEEE2222]]EE]]yyaa2222]]yyaayyjjjjjjxxjjjjjjjjjjjjxxxxjjxxjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjxxjjjjyyaaaaaa]]]]EEEEwwwwwwuu==                                                                                                              
                                                                                                        !!]]$$DDBBBB##88HHXXXXKKAAUUbbbbbbbbOOVV4444dd99999999hhPPkkkkPPkkqqwwqqwwwwwwwwEEEEEEEE22]]]]]]22EEEE2222SSqqwwSSEESSwwSSSSSSSSEESSSSSSwwSSwwqqqqqqkkkkqqkkkkkkkkPPkkqqqqwwEEEEEEEE2222EE2222]]]]aa]]]]aayyjjjjjjxxjjjjxxxxxxjjxxxxyyyyjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjaa]]222222]]EEEEqqqqhh]]LL``                                                                                                            
                                                                                                      ,,nnHHgggg$$##RRmmHHXXKKAAUUbbbbbbbbGGVVdd44dd99999999hh66PPkkqqkkkkqqwwwwSSSSSSEE222222]]22]]]]22EEEESSEE22EESSSSSSwwwwSSSSSSSSSSwwqqqqwwqqSSwwkkPPkkqqkkqqkkkkkkkkkkkkkkwwSS222222]]aa]]SS]]yyaa]]22]]yyyyjjjjjjjjjjxxjjyyjjjjxxYYYYYYYYxxxxjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjaa22SSEE22EEEE22SSSS66hhnn,,                                                                                                            
                                                                                                    ::iiOOBB##ggDDRR88HHXXKKKKAAGGbbbbGGppVV44ddddddhhhhhhhh66PPkkqqqqqqqqwwwwwwEEEEEE2222]]]]aayyjjyyyyaa]]]]]]22222222EEwwwwSSwwSSSSSSwwqqwwSSwwwwwwqqPPkkqqkkqqkkkkkkqqkkqqqqqqwwSSSSSSEEEESS]]]]]]]]aaaa]]]]jjjjxxjjjjjjxxjjjjxxYYxxYYYYxxYY55YYxxjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjaa22SSEEEESSEE22EEEEkk44kkLL--                                                                                                          
                                                                                                  ``77ppgggggg$$DDRRmmHHKKAAUUbbGGGGOOpp44dd999999hh6666PPkkkkkkwwwwwwwwwwwwwwwwSSEEEE2222]]]]aaaaaaaaaa]]]]]]]]22SSwwwwSSSSwwkkwwwwwwSSSSwwwwwwwwkkkkqqPPPPkkkkPPPPPPPPkkkkkkkkSSEE222222222222]]]]2222]]]]]]22yyjjjjjjjjjjjjxxjjYYYYYYYYYYYYZZZZYYxxjjxxxxxxjjjjjjjjjjjjjjjjjjjjjjyyaa2222EEEEEEEEEEEEEEqq4444nn::                                                                                                          
                                                                                                --LLqq00gg##$$##RRmmXXKKUUbbGGGGbbOO44dddddd99hhhh6666PPkkqqqqqqwwwwwwwwwwSSSSSSEEEE2222]]aaaayyjjjjjjjjyyyyyyyyjjaa222222222222SSwwwwwwSSSSwwwwwwPPkkqqPP66PPPPPPPPkkkkkkqqqqwwqqwwSSSSSSSSSSSS22]]aaaa22EE]]aayyjjjjjjjjjjjjxxyyxxYYYYYYYYZZeeZZ55YYxxxxxxjjjjjjjjjjjjjjjjjjjjjjjjyyaa]]22EEEEEEEEEESSEEwwhhVVwwss--                                                                                                        
                                                                                              --TTVV$$####BB$$88mmXXAAUUbbbbbbbbbbVVdd99dddd99hhhh66PPPPkkqqwwqqqqwwwwwwSSSSEEEE22]]]]aaaaaayyyyyyyyyyyyaaaaaa]]222222EESSwwSSEESSwwwwwwSSwwwwwwSSkkqqwwkkkkqqPPkkqqqqkkkkqqSSSSEEEEEEEEEE222222aa]]]]]]aaaa]]22yyjjjjjjjjjjjjjjxx55YYxxYYYYxx5555YYYYxxjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjaa]]22EEEEEESSEE22EEwwkkhhddnn^^                                                                                                        
                                                                                            ..zzSSggBB$$BB$$##HHHHKKUUbbbbbbGGbbOOVVdddd9999hhhh66PPkkkkPPqqwwSSSSSSwwSSwwSSSSEE22]]]]aaaaaaaayyyyaayyyyyyyyyyyyaa]]]]]]aa]]EESSSSwwwwwwwwwwwwSSSSqqwwSSqqwwSS66PPkkqqkkkkqqwwEEEE2222EEEEEEEESS22]]yyaa]]222222yyjjjjjjjjjjjjjjjjxxjjxxYY55YY55YYYYYYxxjjjjjjjjjjjjjjjjjjjjjjjjjjxxxxyy]]EEEEEE22EE22]]SSqqqqhhddqqTT                                                                                                        
                                                                                          --LL44ggggBBBB$$##RRXXXXKKUUGGGGUUbbOOddVV4499hh66kkkkqqwwwwwwSSSSwwwwwwqqwwEE22EE]]aa2222]]]]aaaayyyyyyyyyyyyyyyyyyyy]]]]]]]]]]]]222222wwwwwwwwwwSSSSwwwwwwwwwwwwwwwwkkqqqqkkqqwwSSEEEE2222EEEEEEEESS]]]]2222aa]]]]yyyyyyjjxxxxjjjjjjjjxxYYYYYYYYYYYYYYxxjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjxxjjyy22EEEEEEEEEE22EESSkkPPPP99ddnn,,                                                                                                      
                                                                                        ''rrSSBBBB$$$$$$DDRRmmAAUUUUbbbbbbGGppVVdd4499PPkkkkqqqqkkqqwwwwwwwwSSSSEESSSSEE222222aaaaaaaayyyyyyyyjjyyyyyyyyyyyyyyyyyyyyaaaa]]]]22EE]]EEEEEESSwwwwwwwwwwwwwwwwwwwwwwwwwwwwqqqqSSEEEEEE22EEEEEEEEEEEE22]]]]]]aa]]EE22]]aayyjjjjjjjjjjjjxxxxYYYYYYYYxxYYxxjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjxxjjaa22]]22SSEEEEEEEESSqqqqww66GGqq))``                                                                                                    
                                                                                      ,,rraaRRggBB$$$$##RRmmXXUUbbGGbbbbbbOO44dd66hhPPqqwwwwwwwwqqwwSSSSSSSSEE22EEEEEE22]]22]]aaaayyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyaaaa]]]]]]]]222222EESSSSwwwwwwwwwwwwwwwwwwwwwwwwqqqqwwSSSSEEEEEEEEEE2222EE22222222]]]]]]aa]]]]aayyjjjjjjjjjjxxxxxxxxYYxxxxYYxxxxjjjjjjjjjjjjjjjjjjjjjjjjjjjjxxyy]]]]aa]]SS22222222SSkkqqSShhppppoo;;                                                                                                    
                                                                                    ''!!22DD00##$$####RRHHXXAAbbUUbbGGOOppVVdd99qqPPkkwwwwwwSSSSEEEE2222222222]]22EE22]]aa]]aayyaaaaaayyyyyyyyyyyyyyyyyyyyyyyyyyyyyyaaaayyyyyyyy]]22]]]]EEEEEESSwwwwwwwwwwwwwwwwwwqqqqqqqqqqwwSSSSEEEEEEEEEE22EEaaaa]]22]]2222]]aaaaaaaayyjjjjjjxxjjjjxxxxxxxxjjxxxxjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjyyaaaaaa]]SS]]222222SSqqqqSS6644bbdd}}__                                                                                                  
                                                                                  ,,rr22mm$$gg$$88mmHHKKUUbbbbGGGGbbOO44dddd99PPwwqqwwSSSSSS2222222222]]]]]]]]222222]]yyyyaayyyyaayyyyyyjjjjyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyaaaa]]EESSSSwwwwwwwwwwwwwwwwqqwwwwwwwwwwwwSSSSSSSSSSEEEEEE2222SS2222]]]]22SSEEaaaaaa]]aajjjjjjjjjjjjjjxxxxxxjjxxjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjyyyyyyaa]]EEEE22EESSEEEESSSSSSkkddOOUU22cc                                                                                                  
                                                                                ,,ccyymmBBDDRRRRXXKKAAbbGGGGGGOOVVppVV99hhddhhwwwwwwEE2222]]aa]]]]]]]]aayyyyaa]]]]aayyyyyyyyyyyyyyyyyyyyjjyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyaaaa]]22EEEEwwwwwwwwwwwwwwqqqqqqqqwwwwwwwwwwSSEESSSSEEEEEE2222SSEEEE22]]22EE22]]aa]]]]aayyjjjjjjjjjjjjxxxxjjjjjjjjjjjjjjjjjjxxjjjjjjjjjjjjjjjjjjyyyyyyaa22EEEE22EEEEEE2222SSwwqq44VVVVkkff,,                                                                                                
                                                                                ;;yymm##BBDDXXAAUUbbbbGGOOOOOOVVdd44dd999999PPSSwwSS22]]]]aayyyyyyyyyyyyyyyyyyyyaayyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyjjjjjjyyaayyyyyyyyyyyyyyyyaaaa]]222222wwwwwwwwwwwwqqqqkkkkqqqqwwwwqqwwwwEESSSSEEEEEEEE22yyyy]]]]]]EESS22]]aaaaaaaajjjjjjjjjjxxxxjjjjjjjjjjjjjjjjjjjjjjxxjjjjjjjjjjjjjjjjjjjjyyyyyy]]22EE22]]22EEEE22SSkkqqhhppGGOO22zz                                                                                                
                                                                              ^^llpp####88KKUUUUOOppOOppVVVV44hhdd99hhhh99hhkkSSwwSSEE2222]]yyyyjjjjyyyyyyyyyyyyyyyyyyyyyyyyjjjjyyyyyyyyyyyyyyjjyyyyyyjjxxYYjjyyyyyyyyyyyyyyaaaajjyyaaaa]]22EEqqwwwwwwwwwwqqkkkkkkqqwwSSSSwwwwwwEESSSSEEEEEEEE22EE22]]aayyaa]]aa]]yyjjyyyyjjxxjjjjjjxxxxjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjxxjjyyyyyy]]EEEE]]]]EEEE22EEqqhhkk99GGGG44ll,,                                                                                              
                                                                            ''CCOO##mmAAUUbbOOOOVV999999ddddhhdd9966kkPPPPqqwwwwSS22yyyyaajjxxaayyyyyyyyyyyyyyaayyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyjjaayyjjjjjjyyyyyyyyyyyyyyyyyyyyyyyy]]]]22EESSwwqqSSSSSSPPkkPPkk66kkwwwwwwwwSSEEEEEEEESSSSSS22]]aaaaaaaaaayyaaaayyyyjjjjjjjjjjxxjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjxxjjyyyyaa]]22EE22EE]]]]SS2222kkPPdddd44ppbbww??``                                                                                            
                                                                          ''{{GGDD##KKbbbbOO444499hh99hhhh996666kkwwSSqqPPPPqqSSqqww22]]]]aayyaayyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyjjjjjjjjjj]]aaxx55jjaaaayyyyyyyyyyyyyyyyyyyyyyyyaa2222EESSSSwwkkkkPPkkPPPPPPPPPPqqwwSSSSwwSSSSEEEEEEEE2222]]aayyyyjjjjjjyyaayyyyyyyyyyyyjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjyyaa]]2222SS22]]SSEE22kkkkhh9944VVGGddll,,                                                                                            
                                                                        __FFhh##mmHHOOOOGG44hhhhqqkk66kkkkkkqqwwPPkkkkPPhh9966qqwwSS22aajjyyyyxxyyyyyyyyyyyyyyjjyyyyyyyyyyyyyyyyaayyyyyyyyaaaaaaxxyyjjxxjjaayyyyyyyyyyyyyyyyyyyyyyyyaaaa22EESSwwwwkkPPhh6666PP66PPPPPPkkwwSSwwqqwwwwSSEEEEEEEE2222]]yyjjjjjjjjjjyyyyjjjjyyyyjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjyyaa]]]]EE22]]EE2222wwkk6666ddVVppGGww??''                                                                                          
                                                                      ``JJ9988RRUUGG4444ddPPkkPPwwkkPPkkkkkkqqqqqqPPhh66666666PPkkwwSS22yyaaaajjjjyyyyyyjjyyyyyyyyyyyyyyyyyyyyyyyyjjjjjjyyyyyyjj55xxjjxxjjyyyyyyyyyyyyyyyyyyyyyyaayyaa22SSwwwwqqPP6666dd99dd66669966kkkkkkkkqqSSwwwwwwSSSSEE2222]]aayyjjjjjjjjxxxxxxjjxxxxxxxxxxjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjyyaaaaaaEE]]aa]]2222SSwwkkkkhh44ppGGVVuu::                                                                                          
                                                                    ,,rr]]AAKKUUGGOOdd66qqEEwwqqSSEEqqwwwwwwwwPPqq66hh6666hh99hhkkSSEE22yyjjjjxxxxyyaayyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyjjxxjjxx55ZZxxyyyyjjyyyyyyyyyyyyyyyyyyyyaa22SSwwwwqqhh99hhdd99ddhh66ddhhPP66hhhhkkwwSSwwSSEE2222]]aayyyyjjxxjjjjxxxxjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjyyjjjjjjjjjjjjjjjjjjjjjjaa]]]]aa]]22EEEEEEqqqq66ddppGGbb22//                                                                                          
                                                                    ;;jjAAAAbbOO9999kkqqSSSSqqSSkkwwPPPPPPPPPPdd99dd99hh9944ddhh66PPqqSS]]yyyyyyxxjjyyyyyyyyyyaayyyyyyyyyyyyyyyyjjjjjjjjyyyyjjxxYYxxYYZZYYjjjjyyyyyyyyyyyyyyyyyyyyyyaa22SSSSqqkkhhdd99ddhh99hh99999999999966kkwwwwwwSS22]]]]]]aajjjjxxjjjjjjjjxxjjjjjjjjjjjjjjyyjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjyyyyyyjjjjyyyyjjjjjjjjjjxxaaaa]]aa]]22EEEE22qqPPPPhh44GGAA6633__                                                                                        
                                                                  ''{{VVKKOOOOVVwwwwSSwwSSSSqqSSwwwwPPPPPP666699dddd99hh99dd99hhPP66qq]]yyyyjjjjxxYYxxyyaayyyyyyyyyyyyyyyyyyyyyy55YYYYYYYYYYYY55ooYYxx5555YYxxyyyyyyyyyyyyyyyyyyyyyy]]EESSwwPPhhhh99dd99dddddddd99999999hhPPqqSSwwqqww22]]]]aajjjjjjjjxxxxxxjjjjxxjjjjjjxxxxjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjaaaayyjjjjyyyyyyyyyyyyyyjjyyyyaaaaaa]]EEEEEEqqPPkkkkddOOUUKKqqzz                                                                                        
                                                                ..JJ66XXUU44hhwwqqwwSSSS22]]SSwwwwkkhhhhhhdd9999hh99dddddd99ddddhhddPP]]yyaayyyyjj5555jjaayyyyyyyyyyyyyyyyyyyyyyjjjjjjxxYY555555ZZ55ooeeooYYxxyyyyyyyyyyyyyyyyyyyyyyaaEESSww66dddd9999hhVVVVppdd999999999966qqSSSSwwwwEE]]aajjYYxxxxxxxxYYYYYYxxxxxxjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjaa]]yyjjjjyyjjjjjjyyyyyyyyyyjjyyaayyaa2222EEEEqqqqkkddOObbGGOO11__                                                                                      
                                                                !!]]AAbbVV66kkSSEEww22yyaa22EEwwwwkk669999dddd99ddddppOOddddpp44ddddkk222222yyjjYYYYxx55YYaayyyyyyyyyyyyxxjjaa55YYxxYYxxjjYY55eeooooZZ55YY55YYYYyyyyyyyyyyyyyyyyyyyyEESSEEPPVV99hhdddd44ppGGppdd999999hhPPqqwwwwSSqqww]]jjyyyyjjjjYYYY5555YYYYxxYYxxxxjjyyyyjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjxxjjjjyyaayyjjyyyyyyyyyyjjjjjjjjjjaaaaaaaa]]22EEEEEEwwSSqq99ppOOUUUUww!!                                                                                      
                                                              ^^YYUUUUddhhqqww22]]]]]]22EE2222SSwwkk66999999ddddOOOOUUAAOOOOOOddhh99PPSS22aaxxYYZZZZYYYYxxyyjjyyyyyyyyyyxxxxyyYYyyjj55ZZ55ZZ5555ooooooooZZZZYYxxyyyyyyyyyyyyyyyyjjyy22wwww66dd66999944AAXXUUGGGGVVdd9999hh66kkwwwwqqkkww]]jjYYYY55ZZZZZZ5555YYYYYYYYYYxxxxjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjyyyyyy]]aayyyyyyyyyyyyyyjjjjyyxxyyyyyyaaaa]]EE2222EEEEww66VVVVbbbbppff''                                                                                    
                                                            --7766bbdd66kk]]yyyyjjYYxxaaaayy]]SSwwqq66999999ddVVGGGGUUbbGGUUAAGGdddd99PPww]]yyyyxxYYxxxxjjjjjjyyyyaayyyyjjxxjjxx55YY55YYYYZZZZeeooooooZZZZZZYYjjyyyyyyyyyyyyyyyy]]]]EEqq66994444VVbbKKKKKKKKGGddddddhh66PPkkqqwwwwSSEE]]xxZZZZYYooooZZZZZZZZ5555YY55YYYYxxxxjjyyjjjjjjjjjjjjxxxxjjjjjjjjjjjjjjjjjjjjjjyyaayyjjyyyyyyyyyyjjjjjjjjjjyyyyyyaaaa]]EESSEESSwwkk6644ppOOGGUUaa>>                                                                                    
                                                            ^^xxUUUUdd66wwyyyyaajj55xxyyjjyyEEwwwwqq66dddd99ddOOGGAAHHHHXXXXKKOO4499hhPPww22]]aajjYYYY5555YYYYxxyyaayyyyyyxxxxxx55YYZZ55ZZooZZooeeeeooZZ5555YYYYjjyyyyyyyyyyyyyyyyaa22ww66hh99VVGGbbKKHHHHUUGGOOdddd9966kkqqqqkk66PPkkww22YYZZooeeooooooooooZZZZ5555YYxxxxxxjjjjjjjjjjjjjjjjxxxxjjjjjjjjjjjjjjjjjjjjjjjjyyyyyyyyyyyyyyjjjjjjjjjjyyyyyyyyaaaa]]EESSEESSwwwwqq99VVppGGUUkkTT--                                                                                  
                                                          ''((ddGGddww22aayyyyxxZZ55jjaayy]]EEEEwwqq6699dd9944OOUUKKXXKKAAKKXXAAppddhh66qqSS]]yyaajjjjYYxxyyjjyyyyaajjaaaaxxxxxx55YY5555ZZooZZZZeeeeeeZZ55555555jjjjyyyyyyyyyyyyjj22ww66dd99ddOOmmmmmm88HHUUppVV4499PPkkPP66PPkk6666PPwwEEaaxxZZeeooee[[uunneeooZZ55YYYYYYxxxxxxxxjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjyyjjjjaaaaaaaayyyyyyyyjjjjjjjjjjyyjjyyaaaa]]EEEE22SSwwSSww66VVppUUbbOOee==                                                                                  
                                                          !!jjOOpp99qq22aajjZZnnee55jjyyyy]]EE22wwkk6699dddd44ppppbbXXHHmmHHXXAAUUGGVVddhhkkEEaajjxxxx5555xxYY55yyyyjjaayyxxxxxxZZ55ZZ55ZZeeeenneeeennooZZ55YYxxjjjjyyyyyyyyyyyyyyEEqq6699hhddbbAAHHmmXXXXXXbb4499hh66PP6666hhhhdddd66wwEESS22jjeeeennuulluueeeeooZZZZZZZZ55YYxxxxjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjyyjjjjaaaaaayyyyyyyyjjjjjjjjjjjjjjjjyyaaaa]]SSSSEEwwkkwwqq99ppVVGGGGAAPPLL                                                                                  
                                                        --ii9944PPww22yyxx55ZZ55ZZjjyyyyaa22SSqqqqkk66hh99dddd44AAXXXXHHHHmmHHHHAAAAOOdd99PPEE22aayyyyxxYYYY55YYyyyyyyyyxxYYjjxxYYYYooeennnnooooooeeeeeeooZZYYjjyyyyyyyyyyyyyyyy22ww6699ddddOOHH88XXXX88mmbbdd9999dddddd9999dd44hhdd99kkSS22xx[[eeeennllttll[[[[eeooooooZZZZ55YYYYxxxxxxxxxxjjjjjjjjjjjjjjjjjjjjjjjjjjxxyyaayyjjjjjjjjjjjjjjjjjjjjjjjjyyaaaaaaEEEE22EEwwSSqqhhddVVVVVVUUpp[[==                                                                                
                                                        <<yyGGddPPSSaaaayyZZ[[ooeexxjjyyaa]]22wwqqkk666699ddddhhVVOObbKKmmmmHHHHAAXXGGddhh66qqqq]]jjyyjjYY5555aayyyyyyyy5555yyxx5555ZZ5555oooonneeeeeeeeeeeeZZxxyyyyyyyyyyyyyyyyyyEEPP99dd44GGKKmmmmHHHHmmXXGGdd99999999dddd99hh99dd99kkwwSS]]YYeeee[[tt11ttlltt[[eeooZZZZZZZZ55YYYYYYYYxxxxjjjjjjjjjjjjjjjjjjjjjjyyjjjjyyaayyjjjjjjyyjjjjjjjjjjjjjjxxyyaayyyy]]SS22EEEESSqq66664444OObbbbqqLL``                                                                              
                                                      ``||66VV99qqaayyjjooeeoonnZZxxyyaa]]EEwwwwww6666PPhhdd99dd44GGAAAAXXHHHHHHHHmmAAppVVdd6666EEaajjyyjjxxYYjjjjjjxxyyjjZZYYYYYYZZeenneeooeeeeeeeeeeeeoo55xxxxjjyyyyyyyyyyaa]]wwqq66hhppAAXXHHmmmmXX8888AAGGpp99dd994499ddhhdd999966PPSSyyjjjjZZnn3333ttffuuuu33uueeooooZZ55ZZ5555555555YYxxjjjjjjjjjjjjjjjjjjjjjjyyyyyyyyjjyyaaaaaayyjjjjjjjjjjjjyyyyyy]]2222EEEEEE22EEqqPP66hhppbbbbbbppoo..                                                                              
                                                      >>55ppddqq22yyjjxxeenneeeeeeYYyyyyaaEEwwwwqqPPkkqq6699hh99hh9944bbHHmmHHHHHHmmHHmmHHbbdd99PPww22aayyyyjjYYYYjjYYYYYY55YYZZZZooeenneeeeeeeeeeeeeennnneeooZZxxyyyyyyyyyyaa]]]]kkddddppUUKKHHXXRRmmXXXXXXAAppVVddPP669944ddhhddhhqqSS22xxZZ[[uu3311[[II}}1133[[uu[[eeeeeeooYY55YYYYYYYYxxxxjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjyyaayyjjjjjjjjjjjjjjxxxxjjjjjjyyaa]]EEEEEE22EEqqPP669944ppGGGGAAqq++                                                                              
                                                    ,,LLwwVVddkk]]yyxxZZeeeeeeooeeYYyyyyaaEEqqwwSSwwEEEEkk66hhhhhh66hhOOHHmmHHHHHHXXHHmmmmUUVV99hhPPww22yyyyxx55YYxxYYYYYY5555eeeeeeeeeeeeeeeeeennnneeooooooZZYYxxyyyyyyyyyyaaEEwwhhddVVKK88mmHHDD$$RRmmHHKKUUOOdd994444dd99ddhh6666EEyy55ZZZZnnII}}CCff{{{{{{11llttllee55555555ZZ55YYYYYYYYYYxxjjjjjjjjjjjjjjjjjjjjjjjjjjjjyyaaaaaayyyyyyyyjjjjyyjjyyjjyyyy]]222222EE22EEwwkk664444ppGGbbXXOOii--                                                                            
                                                    ''CCqq6699PP]]aaYYeeooooeeeennYYyyyyyyEEwwSS66PPwwSSSSwwwwSSPPhh9944UUXXHHmmmmmmmmXXXXXXAAOO9999PPww]]yyjjxxjjYYZZxxxxZZeenneeeeeeeeeeeeeeeeooeeeeooeenneeZZxxyyyyyyyyyyaaSSww6699ppAAXXHH88RRDD88mm88HHXXAAOOddddhhddhh66ww]]SSaa55[[llll33}}FFFFFFii}}{{fffflleeeenneeooeeooZZ5555YYYYYYxxjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjyy]]]]]]]]aayyjjxxjjxxjjjjjjjjyy]]22EEEEEEEEwwkkPPdd4444OOGGAAbboo^^                                                                            
                                                    ;;YY44hh66EEaayy55nneeoonneeooxxaaaa]]SSqqSSEE]]aaaaaa]]2222]]SSkk66ppUUXXmmHHmm88mmHH8888KKGGVV9966ww22aayyyyYY55YYYYZZeenneeeeeeeeeeeeeeeeooeeeeooeenneeZZxxjjjjyyyyjjaaSSPP99ddbbmmHH88DD##00##HHXXHHKKVVVVddddhh66wwaa55yyyyZZllffCCCCiiii??77JJTTiiffFF33llnnnneeZZ55ZZZZ5555YYYYxxxxjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjyyyyaa]]]]aayyjjxxxxxxjjjjjjyy]]22EESSEEEEwwkkPPhhVV44ppOOGGOOwwvv--                                                                          
                                                  ''zzEEVVddkkyyyyyyYYeeeeeennooeeYYjjjjaa]]22]]]]xxYY55ooooZZoo5555jjqq44OOUUHHHHHHHHHHmmHHHHHHHHbb449966ww22aayyjjxxoooo55ZZeeeeeeeeeeeeeeeeeeeenneeooooeeZZYYYYxxxxjjyyjjyySShh66hhUUmmHH88mm88##88HHmmmmKKOOPP6666ww2255[[llttttffCCFF((FFiiLLccLL////)){{CCCC{{}}tteeooeeee5555555555YYxxjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjyyaayyaa]]]]]]aayyjjjjjjjjyyyyjjyy2222EEEE22EESSqqkk66VVddVVGGGGGGGGnn__                                                                          
                                                  ``||qq9999wwaayyjj55ooeenneeooxxjjyyaaaa]]aajjZZuuuuuulllluuuullll[[jjqq6644bbXXmmHHHH88mmHH88mmKKGGVV9966ww22aayyyyZZZZYYooeeeeeeeeeeeeeeeeeeeeeeeeeenn[[nnnnYYYYYYjjyyjjyySS6644bb88mmHHBB##BB$$8888mmGGddkkww22eelluuttCC{{CC{{||((TT**rrrrrr//zz//TTTTLLii{{fftt[[[[[[nnZZooZZZZ5555YYYYxxjjjjjjjjjjjjjjjjjjjjjjjjjjjjyyyyaa]]2222]]aaaaaajjxxjjyyjjxxjjaa2222EE2222SSqqPP669999VV44OOGGbbSS**                                                                          
                                                  ::uudddd99SS]]yy55eenneeeennee55ZZ555555ZZee[[llff{{CCiiiiiiFF{{}}33uu552266VVOOXXmmHHDD##88mmXXmmHHbbVVdd66ww]]yyaayyxxYYeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeooooeeYY55YYjjaayyaaSS6644GGmmHH8800##gg00DDXXUU44wwYYuunnffCCCCFFLLTT||LLrr**//cc**//**zzcc**//**ccJJCC{{iiCCtteeoonneeooooZZ55YYYYjjjjjjjjjjjjjjjjjjjjjjjjyyyyjjjjjjyyaa2222]]]]22EEaaaaaa]]aajjyy]]22EESSEEEEwwPPhh996699OOddOObbOOVVii--                                                                        
                                                  <<jjVV99kk]]aayyZZeenneeoonnoonn55ZZnn[[[[ll33CC{{CCiivvvvssvv||||iiCCffZZkkhhddUUKK8888DDDD##DDHHHHXXbbOO44qqaaaajjYYZZ5555eeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeee55YYYYjjaayy]]kkdd44KKRRHHDDWWWWgg##HHGG44PPaannnntt}}CC{{vv//zz//rrcc**//********//****zzcc**TTiiCCCCCC33nneenneeooZZ55YYYYxxxxjjjjjjjjjjjjjjjjjjjjyyaaaa]]aayyyyaa]]]]22222222aaaaaayyyyyyyy]]]]22EEEEEEqqPP66qqdd44ddVVppVVGGOOoo^^                                                                        
                                                ''//qqpp99PP22yyaaYYZZeennee[[nneennuullllIICCCCCCff77vv))vv//rr**rrvvCCCClljjjjww44GGAAHHDD$$$$RRmmmmmmHHXXGG6622yyyyyyxx55oonneeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeZZ5555xxaayy]]kkddVVAAmm88ggNNBBRRmmUU44kkSSxx[[llff{{CCffCC((TTLLzz////cc////////zz////zzccccssvv||CC}}}}11[[eeeeeeooZZZZ55YYxxjjjjjjjjjjjjjjjjjjyyyyyyaaaayyyyyyyyyyaaaa]]]]]]aaaayyyyyyyyyyaaaa]]22EEEESSqqPPPP44dd9944VVppbbGGqqLL--                                                                      
                                                ..JJPPddhhkkaajjjjeennnneeoouuII[[uu33}}}}}}CCCC{{ffJJ))JJTTsszz//cc**TT||ffuunnjjPPOOAA88RRDDHH####RRmmHHXXGGhhPP22yyjjYYZZ5555nnnneeeeeeeeeeeeeeeeeeeeeeeeeeeeooZZ55xxyyyyaaqqddUUmmmm88ggMM##RRKKGG992255nntt33ffIIff{{CC{{JJ**rr******//**cccc******zz****ss))))FFII33IIlleeeeeeooooZZ55YYxxjjjjjjjjjjjjjjjjyyyyyyyyyyyyyyjjjjjjjjjjyyaaaa]]aaaayyyyyyyyyyjjaaaa]]EEEEEEwwPPPPdddd99VVVVVVGGGGOOtt::                                                                      
                                                ::ffPPdd44kkaa]]55eeZZZZ[[llllll1111}}ii{{CC}}}}}}CC{{{{TT**zz**//zzccccvvFFCCIIZZ22hhVVHH88$$##88##DDmmHHXXUUOO66SS22]]yyjjYYZZooooeeeeeenneeooeeeeeeeeeeeeeeeeeeZZ55xxyyyyaakk44KK88HHRR0000##88GGdd66aaee[[nnuueeZZ[[33ffff{{))zz**cc**//**//////**////**cc//TTssvvFF{{CC11eeeeeeooooZZ55YYxxxxjjjjjjjjjjjjjjyyyyyyyyyyyyyyyyjjjjjjjjyyyyaaaa]]aaaayyyyyyyyjjyyaa22EESSSSqqPPkkhhddddVVVVVVOOGGOOYY;;                                                                      
                                                ,,uu66hh99SSjjyyYYnneenn33}}ff11IICC{{}}}}iiiiffCC{{33CCJJ77TTcc****//**ccLLii}}11oo22hhAAXXDD##RRggggDDRRRRHHbbhhPPww]]yyaajjZZ55ZZooooeeeeeeeeeeeeeeeeeeeeeeeeeeZZ55jjyyjj]]PPVVAAXXHH$$NNgg88XXpp66SSYYnnnnooooYYYYZZZZnn11ff}}TT//**//zz//zz////**////********zzLLvv77CCllooeeeeooZZZZ55YYxxxxjjjjjjjjjjjjjjyyyyyyyyyyyyyyyyjjjjjjjjjjyyyyyyaa]]aaaaaaaaaajjyy]]EEEEEEwwqqkkhh999999ddVVppGGAAGGqqTT''                                                                    
                                                ^^jjOO4499ww]]yyaaooeeeeuu11}}}}}}CC{{CC}}}}fftt[[CC}}}}}}))**??//******cc//TT||FF3355kkbbAAHH88BB0000$$DD##88UUOO44PP22yyaayy55ooeenneeooeeeeeeeeeeeeeeeeeeeeeennZZ55jjyyyy]]PPOOKK88DD00MM##XXUUppwwYYeennnneejj]]]]]]EEyynntt}}vv//////zz////**cc**//////////**cczzJJ{{}}33nnnneeeeooZZZZ55YYxxjjjjjjjjjjjjjjyyyyyyyyyyjjjjjjjjjjjjjjjjjjjjjjyyaaaaaaaa]]aayyyy22SSEE22SSqqkkhh9999dd44VVppppbbbbbbee::                                                                    
                                                <<EEOO9966EEaaxxooeeoo[[11ff{{CC{{ffCCii}}llnnoouuooZZ33}}ff{{))cc//cc**??//rr**JJCCttaahhbb88$$####BB$$##$$$$mmUUVV9966ww22jjYYZZee[[nnooeeeeeeeeeeeeeeeeeeeeeeeeooZZxxaaaaEE66bbHH##gg00ggRRKK4466yynnnneeZZjjww66dd9999PP22xxnn}}JJzzcc**//zz////////********//ccrrTTCCCC{{11nneeeeeeooZZ5555xxxxjjjjjjjjjjjjyyyyyyyyjjjjjjjjjjjjjjjjjjjjjjjjjjyyaayyyyaa]]aayy]]EEEEEESSqqkk6666994444VVVV44VVOOUU22<<                                                                    
                                                zzww449999EE]]jjZZoonn11IIII}}CC{{CCCCIIll[[ooYYxxxxaaxxllCCII))//**////cc//zz**))CC11jjkkOOKKmm88##MMNN$$$$##HH88OO9999kk]]yy]]yyYYeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeooYYaa]]EE66AAHHDDBBMMMMDDUUVVPPYYnnee[[ee]]SSkk9999hh99PPaaooll}}vv**////******////**//zz//**??**cc))CCfftteeeeeeeeeeoo55YYxxxxjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjYYyyyyyyyyaa]]aayyaa22EEEESSqq6666PPhh999944VVVVGGUUbbqqss                                                                    
                                              ``LLkkddddPPaayyjjZZoonntt}}CC}}CCCCCCfftteeooYYSSwwwwkkwwxxoollCC))zzrr**//////**TT{{11oo]]hhKKHH$$88##00NNNNBBHHmmUUppddPPSS22yyaaYYooeeoonnnnoooonneeooeeeeooeeoonn55jjyy22qqddAAmm##NNMM$$RRbb99kkjjooZZ[[xx22SSEEkkhh6666kk]]YYoo33TTcc//////********////////**zz**rr77FFFF11nnnneeeeeeZZYYxxxxxxjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjxxjjjjyyaayyyyyyyyaayyxxyy22EEEEqqPPkkhhhhhhdd444444ppGGOOVV}}..                                                                  
                                              --vvPPdd99qqaajjYYoouuttttII{{{{}}CC}}ll[[nnaaqqSS66hhddPP22aaYYtt}}vv//******//****ssiitt55qqppAAHHDDBB$$$$MMNN$$mmXXKKbbVVhhqqaayyjjZZeeeeeeeeeennnnoooonnnneeeeeeoojjyyyy22kkppXXRRBBMMgg$$RRppwwkkkkww55eeyySSEE]]EESSEEEEEE]]xx55tt))********//******////////********TT))JJCC}}tteeeeeeooZZZZYYxxjjjjjjjjjjxxjjjjjjjjjjjjjjjjjjjjjjjjyyjjjjyyyyaaaaaayyjjjjjjjjxxyy]]2222EESSPP66PPPPhh999944ddOOUUAAoo,,                                                                  
                                              ''vvPPdd99qq]]yy55nnZZoo11}}}}}}{{CC33eeoonnYYSSSSPP66hhkk22yyZZ33CCTTcccc//////////ccJJ{{11]]44AAmmDDggNNNNWWMM$$DDmmHHAApp99kk22yyaaxxoonneeeeeenneeeeeeeeeeeeeeooooxxyy]]wwhhppUU8800NN$$$$##bb66qqwwkkxxjj]]xxYYYYxxjjxxxxxxxxoouuCCLL******////////////////////zz??**ccvvFF{{11nnoonn[[oo5555YYYYxxxxjjjjxxxxjjjjjjjjjjjjjjjjjjjjjjjjjjxxjjaaaa]]]]aayyjjjjjjyyjjyy]]EESSSSSSPP66PP66ddhhhh44VVppGGUUww**                                                                  
                                              ``))PPVV99qq22yy55[[[[ll33}}ii{{ff}}uueeeeeeeeYY22aayy22EE]]jjee11CCTT**//////****cc**JJCC33jjhh44KKKKmm$$MMWWWWMMMM##88mmAApp99PPaaaajj55eeeeeeeeoooo[[uueeooeeeeooZZxxjjEEPPddOO88RRBB00ggNN##44dd66PPqqxxjjjjnn[[uulllluulluunn11FFTT****zz////////////////////zzcccc//rrzz)){{CC11[[nneeZZZZZZ55YYxxxxjjjjxxjjjjjjjjjjjjjjjjjjjjyyjjjjjjxxjjaa]]]]]]aayyjjjjjjxxjjjjyy]]22EESSqqPPkkkkhh66PP9944OOUUGG66JJ                                                                  
                                              ''((PPVV99SSaajj55eell}}{{}}CC{{CC11eennnnZZ55YYZZjjxxjjjjxxoo11{{FFLL********////****TTiiffnnww66OO880000NNWWMM##gg##DDRRHHbb44PPEEaayyxxooeeeeeeooeeuulleeooeeeeee55yyaaEEPPddUUmm##NNNNBBggDDOO999944PP22jj55llffCC{{}}ffCCFFFFFFLL**cc////**//////////****////**??zz**ccTTvv))FF}}ttnneennnnoo5555xxxxjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjyy]]22]]aaaayyjjxxjjjjaaaaaa22SSwwkk6699hh66994444pp44ppOOGGbbee::                                                                
                                              --((kk44hh22yyjjYYooee33iiCCff}}CCuu[[oojj55oo55lltt[[uuttll[[33FF??**cc******//zzzz****JJ{{33aa6644KKRRDDBBMMWWQQMMWWMM##88mmUU99PP22yyyy55eeeeeeeeeennnneeeenneeooYYyy]]kk99VVKKmmRR00MMMMMMDDbb449999qqqqaaoo11ffFF((||FF77vvLLLL**cc**////cc//////////****//////**cc**//////vv{{fflleennuu[[ZZ5555YYjjjjjjjjyyjjjjjjjjjjjjyyyyxxjjjjjjyyyyyy22SS22]]aayyjjxxjjYYyyyyaa222222qqkkhh99hhdd44dd4444GGAAbbOOZZ==                                                                
                                              ''FFwwddhhEEaaxx55uu11CC}}ffii{{lleeeeZZaa55ll[[11}}1133{{CCffFFLL//****zz////******cc**TT{{33ooEEVVAA88$$NNNNMMNNNNWWNN$$mmHHAAppVVww]]aa55eeeeeeee[[nnooooeeeeeeZZYYjj]]6644VVKKHH##NNNNMMBBmmKKOOdd99PPSSyy[[33{{TTccrrrrcc******cc******//**//////////********////zzcc**zzLLJJ77ii11nnooeeeeeeYY55YYjjjjjjjjyyjjjjjjjjjjjjyyaayyyyjjyyyyyyaa]]SS22]]aaaajjjjyyjjaayy]]SSEEEEkkkk99ddddpppp4444OOGGGGGGGGSS**                                                                
                                              ..FFSSdd99ww]]55nnllttffCC}}{{{{11eejj55YY55nnuuIIFF{{ii((||((LL****cc********////**//****((11[[jjkkGGXXmm$$NNWWMMWWNNWWMM##mmHHHHbbPP22aa55eeoonneettllnneeeeeeeeooYYyyEE66dd44XXmmRRBB00QQWWRR88XXVVddddqq]]ll{{iiss**//**////cc**//zz**cczz//////////////******//rrzz//**rr??)){{CC}}11[[ooooee5555YYjjjjxxxxjjjjjjjjjjjjyyyyaa2222]]aa]]aaaa]]22aayyaayyjjxxjjjjyyxxyyEE2222kk66666666dd9999pp44OObbbbGGkkTT''                                                              
                                              ''((qqdd66SSaa55nnnn33CC{{}}}}CC33jj222222YYff{{CCvvLLsszz****//********//////////****//TT{{}}33xxkkppKKRRggNNNNWWNNNNWWMMBBDDmmHHKKppPP22jjYYZZnneeoonn[[oooonnee55jjaa]]6644KKHHmmBBMMNNNNMMBBDD88KKOOhhPPjjII}}TT****//******//**********////////////////******////**//cc//JJLL77{{CC33uuuunnoo55YYxxxxjjjjjjjjjjjjjjjjjjjjjjyy]]EEEE]]yyaa]]]]aa]]]]aajjjjjjjjyyxxjjjj]]EEEEkk66PPhhhh999966VVppVVbbbbbbppff''                                                              
                                              --((qqddhhww]]YYeenn33CC{{{{{{CCllyy22]]aa5511}}iizz//**cccc**//**//////////////////**cc**)){{11jjkkppXXRR00WWNNWWWWWWWWNNMMBBDD88mmGG66wwaaxxYY55eeooeeeeooeennooZZxxjjaaPPVVXX88RRggNNNNWWWWMMgg88mmKKddkkjjIICCvv//**//////////////********////////////********//****??//!!??77{{ff{{CCII11[[eeZZ55YYxxjjjjjjjjjjjjjjjjjjjjjjjjyy]]22]]]]]]]]]]22EEEE]]yyjjjjjjjjjjjjjjaa22EEqqkk669966hhdd9944VVOOGGbbbbOO[[,,                                                              
                                              --77SSdd99qqaa55ee11CC}}ff}}{{CC33jjEEEE225511iiLLss//****//zzzz**//////////////////zz//**TTii33ooEEddAARR00WWNNNNWWNNNNNNNNMMBB##HHUU44kkEEaajj55nneeeeeeeenneeZZxxyyaa]]PP44AAXX$$MMWWNNNNWWMM0088HHXX44qqyy11CCTT//**//////////////**********//////////**********zz**cc**//TTiiCCffii{{ff11uunnooZZYYjjjjjjjjjjjjjjjjjjjjjjjjjjyyaaaayyaa]]222222EESS22]]aaaayyaaaaaayy]]EEEEwwkkhhhhhh9999dd44VVbbOOGGbbGGjj;;                                                              
                                              ``))EE9999qqaaZZee11{{CC}}{{{{ffllyySSww22oo11ff77vv??//****////**////////////////**//**ccss(({{33yy66bb8800NNMMNNNNMMMMNNNNNN00BBmmXXUUddqqSSaa55ZZoonneeeenneeooYYyyyy22hhppXX88ggNNNNNNNNNNMM00DDXXKKVVkk]]uuCCvv//**////////zz////**********//////////********zzcccc//??////JJ{{CC{{CCIIlleeeeooZZYYjjjjjjjjjjjjjjjjjjjjjjjjjjyyyyyyyyyy]]22EE22EEEE22222222]]]]2222yy]]EEEESSPP66PPdd4466ddppVVGGOOGGbbGG22**                                                              
                                              ``vvEE9999qqaa55eett{{{{CC{{CC11nnyySSwwEEoo33ff||vv??********////////////////////zz//****LL||CCttjj66UURR00NNNNWWNNNNNNNNWWNNNNMM$$DDHHGG99kkEEjjyyZZeeeeeeeeeeee55jjyySS99bbmmDDggNNNNNNWWWWNNNNggmmHHOOPP]]uuCCvv//////////////////********//////////////////////cc**zzssvv((}}}}}}CCff33uuooooZZ55xxjjjjjjjjjjjjjjjjxxjjjjjjjjxxjjyyaa]]]]]]]]EEEEEE22]]]]]]]]]]2222aa]]EE22SSkkPPkk99dd66ddppVVppbbbbUUGGEE??''                                                            
                                              ..vvEEddhhwwaa55eett}}CCCCCCIIttooaawwqqwwxxttCC))vv??**////////////////**////////**cccc**ss((CCttaa99KKDD00NNNNWWNNNNWWNNNNNNWWWWNN##HHAA44kkwwEEaa55eeeeeeeeooeeYYaa]]qq99GGXXRR00NNNNNNWWWWNNWWgg88mmGGPPaauuCCLL**cc//**//****////********//////////////////////////cczz77CCiiCC{{CC11lluueeee55YYxxjjjjjjjjjjyyjjjjjjxxjjyyyyjjjjyyaa]]aaaa]]222222]]aa]]22222222EE]]22EEEEqqkkhhhh66hh9944VVOOppGGOObbGGkk||''                                                            
                                              --vvEE99PPEEyy55nneellIICC}}11nnxxSS66qqSSyynnttCCJJ??cc**//******////**********//////////ss((IInnEE44KKDD00NNMMNNNNNNNNNNNNNNNNNNNNBBRRHHbb4466qqaaYYZZoonneeooee55yyaakkddbbmm$$MMNNNNNNWWNNMMNNBBRR88GG66EE[[CCJJ//cc****////**********//////////******////////**////JJ}}CCCCCC}}CCff[[nn[[eeee55YYxxjjjjxxjjjjyyyyyyyyjjjjyyaayyjjjjyyyyyyyy]]]]]]aaaaaa]]22EEEEEEEE22EESSSSkkPPhh99PP66dddd44ppOOOOppbbGGpp33..                                                            
                                                LLSS99kk]]jjZZnneeee[[lluunneejjSS9966SSyynn11{{FFLL****////////////****************//cc//((ttYYwwVVXXDDMMWWNNNNNNNNNNNNWWWWNNNNMMWWMM##HHKKppkkEE]]yyYYeennoonn55aa]]PPddbb88BBNNWWNNNNWWNNMMNNNNggDDbb44PPooCC}}TT//****//****//******////////******************77))((}}{{CCffuu11tteenn[[ooZZ55YYxxjjjjjjjjjjyyaaaaaajjjjyyaajjjjyyaaaajjjjyy]]]]aaaaaa]]2222SSEEEE22EEEEEEkk66PPhhPPPPhh6644ddGGOOOOUUOOGG11,,                                                            
                                              --ssww99kkEEyyYYeeee[[eeeeeennnnyy669999PP]]ZZuuCCJJ**cczz//****//////////********cc//**cc77ffIIjjPPppAADD00MMWWWWNNNNNNNNNNNNNNNNNNQQ00##DDmmGGddPPEEyyZZeeooeeooxxjj22PPddKKHH$$NNNNNNNNNNNNNNNNNNggmmKKppqqxx33}}ss??//****////////**////****//cczz**rr////cc**//vvii}}}}CCffllnnnnnnnnnneeZZ5555YYxxjjjjjjjjjjyy22]]]]yy]]yyjjyyjjjjjjyyyyyyyyaayyyyaa]]222222qqSS2222EEEESSwwqq6666PP66hh9944ppppGGbbbbbbOOnn,,                                                            
                                              ''//wwddPPEE]]yyxx55ZZeeeeeeeeYYaaPP9999PPEEYYnn33ff77zzrr////cczz////******//////****////))CCttyyPPppXX$$NNNNNNNNNNNNNNNNNNNNNNNNMMWWNNNNMM##AApp99kkSSjjZZnneeYYyyyyEEhhppXX88$$NNNNNNNNNNNNNNNNNNMM$$mmGG66xx33}}77))vv??**//cc**//****//**//zz//**////cc****cc))JJ77FFfftt[[ooeennnnee55xx55ooYYxxxxjjjjxxjjjjxxaayyyyjj]]aayyjjjjyyaa]]]]aaaaaaaa]]]]22EEEEEESSEE22EEEEEEEESSqqPP66666666hh44ddppbbbbGGGGUU]];;                                                            
                                                !!SSVVkk22aajjxxxxyyxxyy2222wwww66999966wwaa55[[{{iiJJzz////cc//**//zzzz//******//!!??77}}uuooyy66GGmmggNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNWWggmmAAOOhhkk]]ZZnneexxaayyww99bbXX88$$NNNNNNNNNNNNNNNNWWNN00RRbb99jj11CCii77((ss//sszz//////////////zz//cc//zz**////??))iiCC}}ttooeeuu[[nnee55aaEE]]xxjjjjjjjjxxjjjjjjaa22]]aayyaayyjjyyyyaaaa]]]]aaaaaaaa2222EEEEEESSSSSSEEEEEE22EESSwwkkPP6666kk6644VVppOOOOGGGGbb22!!                                                            
                                                ^^yy44PPSSEE222222SSEEwwqqwwqqwwhh9999hhkkEEaaZZttIICCvvrrrr//////**////////////zz//JJCC}}1155wwddAARRggNNNNNNNNNNNNNNNNNNNNNNNNWWNN00MMNN00DDHHKKVVPP]]YY5555yyyyyyqq44KKHH##00NNNNNNNNNNNNNNNNNNNNWWDDKKVVaall}}CCii}}((JJLLcc**cc**//////****//////**//**cc))vv||{{}}11nneeooZZxxyyaa]]22aaxxjjjjjjjjjjjjjjjjyy]]2222]]]]aayyaaaaaaaaaaaaaaaaaa]]]]2222EEEEEEwwSSSSEE2222SSwwSSqqkkPP66PP66ddppppppOOUUGGbb22cc                                                            
                                                ,,55dd66SSwwEESSwwSSqqqqkkqqwwPP99dd9999PPwwEEjjoo11CCJJzz////cczz****//////**ccvvvv77CCll55]]PPppHH##00NNMMNNWWNNNNNNNNNNNNNNNNNNNNNNNNNNNNBBmmmmUU44SSaayyjjyyyy]]PPppKKXX$$WWNNNNNNNNNNNNNNNNMMNNQQBB88UUEEnntt}}{{CCCCffJJsszzcc******zz//////////cc****//||}}CC}}lleeooZZaa22SSwwwwEEEE]]aajjjjxxjjjjjjjjyyyyaa]]aaaaaaaayyyyaa]]]]]]22EE2222]]aa]]22EEEE22SSSSEEEE2222SSqqSSkkkkqqPP66ddVV44OOGGGGUUOOGGSS//                                                            
                                                ,,[[99hhSSEE2222wwww66hh9999PPdd99dd9999hhkkww]]YYoottffCCJJ**********//**cc//sszz((CC}}tt55SShhbb88BBNNWWNNNNNNNNNNNNNNNNNNNNNNMMNNWWNNNNWWMMDDmmKKbb6622aajjyyaaEEPPOOKKHHBBNNNNNNNNNNNNNNNNNNWWNNNN$$88KK99aaee11IICCiiff{{}}vvzzzz//cc//////**cc//zz//zzJJ{{{{IIll[[oo55YYyySSwwwwqqqqqqwwwwyyxxxxjjjjxxxxjjaaaa]]jjyyjjyyjjaa]]2222EEEEEE22EE]]aaaa]]EEEE22EE2222EEEEEESSSSwwkkqqSSqq66ddVVppppOOGGbbGGbbSS**                                                            
                                                ..}}6644kkqqwwqqqq66dddddd9966hhdddd99ddhhPPkkEEaaxxee11}}))zzss//zzzz//cczzJJCCCC}}ffttZZ22hhVVXX##ggNNWWNNNNNNNNNNNNNNNNNNNNNNWWNNNNNNMMNNNNggmmHHKKVVkkEEaa]]22wwPPOOXXRRggMMNNNNNNNNNNNNNNNNWWWWMMBBDDHHbb66yy55ooll33CC{{CC((ssLLzzrr**cccc**zzzzvv))JJ}}}}CClleeeeYYEEqqqqqqwwSSwwqqwwEEEE]]yyjjjjjjxxxxjjjjjjyyjjaaaa22]]]]22SSEEEEEEEE]]22]]aaaa]]22EEEESSEE22EEEEEEEEEESSqqqqqqqqPPhh44OOOOppOOGGbbUU22**                                                            
                                                --JJwwVVkkSS2222PP449999ddddVV999999hhdd99hh66wwEExxYYZZtt}}CC{{))))JJ7777JJ))JJCCii33jjSSkkppKKDD00MMNNNNMMNNNNNNNNNNNNNNNNNNNNWWMMNNWWNNNNNNgg##88HHbbddPPwwqqSSkkkkGGmmBBMMMMNNNNNNNNNNNNNNNNMMQQWWWW00RRXX446622ZZnnee1133{{{{JJ||))??ss????zzJJTT77}}{{ffff[[[[eexxSSqqwwww66kkkk6666kkwwwwEE]]jjyyjjjjxxjjjjjj]]yy]]aa]]aa]]EESS22EEEESSEE]]aaaaaa]]22EESSqqwwEEEEEE22EEEESSqqPPhhhhPP6644VVOOUUUUGGGGKKwwzz                                                            
                                                  TTkk99kkkkEEww66dd99ppOOppGGdd99dddd999999996622aayyxxnnllffJJ(({{}}CCCC{{CCCC}}}}uuyyww99UUmm$$ggMMNNWWNNNNNNWWNNNNNNNNNNNNNNNNWWNNNNMMNNNN$$BB##mmAAppddhh66qqPPddbbmm$$00NNNNNNNNNNNNNNNNNNMMNNWWMM$$DDmmbbVVSS55ZZeell1111ff{{{{}}CC{{{{CC}}JJ77}}ff}}llooeeeexxEEwwSSkkdd994444999944hhqqww22aayyxxjjyyjjjjjjyy]]22]]]]22]]22EE2222EEEE22yyyyyyyyaa]]SSwwSSSSSSEEEEEEEEEEqqwwkk66PP66ddVVOOGGOOAAbbbbUUEEzz                                                            
                                                ''//kkVVkkqqSSkk6699ddGGUUUUAAVV99hh9999hh99dd99PPSS22aaYY55oottffIIff}}ffffCCCCffttxxPPppUUHHRR00MMNNWWNNNNNNWWNNNNNNNNNNNNNNNNNNWWNNWWNNWWQQMMNN00$$RRHHbbVV999944OOAA88ggNNWWNNNNNNNNNNNNNNNNNNNNWWWWMM00$$HHpphhqqEEjjnnlluuttff}}ff}}CCCCCC{{}}ttuu[[eeZZeeZZYYyyEEwwqq66dddd9999ddddhhkkwwww22]]]]yyxxjjjjjjxxjj]]22]]aaaajjaa22EEEEEE22aaaaaaaaaaaa]]EESSSSSSSSEEEEEEEEEEwwSSqq66PP66ddVVVVppOObbOObbKKww//                                                            
                                                  !!22VVkkqqSSwwPP9944AAHHXXXXOOdd99dddd9999dddd99PPkkqq22yyxxooZZoo[[llll1111tteeZZaa66VVAADD00NNNNWWWWNNNNNNWWNNNNNNNNNNNNNNWWNNWWMMNNNNMMWWNNNN$$mmHHmmHHbb44ddGGKKHH##00NN00NNNNNNNNNNNNNNNNNNNNNNNNNNNNBBHHAAGGddPP22xxZZZZZZ[[llll1133IIffllll[[[[[[nnZZxxyy]]EEwwkk66hhddOOVVVVppppddhh66ww2222EE]]jjjjjjyyjjjjaaEE22aayyaaaa]]22EEEE22]]aaaaaaaa]]222222EEEEEESSEEEEEEEESSEEwwPP66hh44ppOOGGbbUUGGUUAAqq//                                                            
                                                  ^^YY44kkwwwwSSPP99ppKKmmmmmmUU449999dddd99dd99ddhh99dd66ww22yyaajjYYZZZZooZZYYxx]]hhbbHH88##$$NNNNNNNNNNNNNNWWNNNNNNNNNNNNNNWWWWWWMMWWQQNNNNNNNNNNMM$$88HHmmRRmmmmmmHHDDggNNNNNNNNNNNNNNNNNNNNWWNNMMNNNNMM$$mmmmAAppddPPSS]]aaYYooeeZZZZooooeeZZoooo5555eeZZaaaa22wwPPhhdd44VVGGGGppVVVVVVddhhkkSSEESSEEaayyjjyyjjjjaa]]]]aayyaa]]]]22EEEEEE22aayyaa]]22222222EEEEEEEE222222EEEE22SSkk669944VVppGGGGbbOObbUUqq??''                                                          
                                                  ,,ee44kkSSwwkk66ddGGHHmmmmmmKKppddhh999999dd99dd9999dd996666PPPPSS22EEkkkkkk66dd44OOAAmm##MMWWMMNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNggMMQQNNNNWWNNMMgg$$DDmmXXKKRRmmHH88$$MMWWQQNNNNNNNNNNNNNNNNWWNNNNWWWWWWMM$$RR88HHUUVV99PPqqSS]]aa]]]]]]aayyxxyyyyyyaa]]22EEwwqq669999hhddppbbbbOOVVVVVVddhhPPqqwwSSEEEE]]xxxxjjjjyyaayyjjjjjjyyaa22SSEE22]]aaaaaa22EEEEEEEEEE22222222]]22EEEEEESSkk66ddVVVVppbbUUAAUUXXmmpp77                                                            
                                                  ..}}66PPwwwwqqhh44UUHHHHHHHHHHKKOO44dd99dddd99dddd44VVVV44dd44hh66kk669999ddVVbbUUXXmmDD00NNNNNNNNWWWWWWNNNNNNNNNNNNNNNNNNNNNNNNNN$$BBNNMMMMWW00WWQQMM$$RRRR##88mmDDggWWWWNNMMNNNNNNNNNNNNNNNNNNNNWWWWNNWWWWMM00BBDDHHAAGGVV99dd66PPkkqqqqwwSSSSwwSS2222qqPPPPhhhhddVV44ddVVbbKKUUGGbbGGVVdd99hhPPqqSSEEEE22jjjjyyaaaaaayyjjjjxxjjyy]]22222222]]]]22EESS2222SSEE222222]]]]22EEwwwwqqPP6644ppVV44OOUUbbOObbKKGG}}..                                                          
                                                  ``))wwPPqqwwww9944UUHHHHHHHHmmmmXXUUOOVVVVVVddppOOGGbbAAUUGGGGGGbbUUbbbbGGbbKKmmRR$$gg00NNQQNNWWWWNNNNNNNNNNNNNNNNNNNNNNNNNNMMMMWWggggWWWWNNWWQQWWMMMMWWQQNNMMWWWWNNNNNNNNNNMMNNNNNNNNNNNNNNNNNNNNWWNNMMNNNNMMQQMM$$DD88HHAAbbOO44444444VVVV44VVdd44ppVVVVVV44ddVVOObbbbUUXXmmXXKKUUbbGGppdd99dd66kkwwSSEEEEaayy]]2222]]aayyjjyyyyyyaaaa]]22SSEE22EESSEE]]]]EEEE2222EE2222EEwwqqkkPPPPhh44ppVVOOGGbbbbGGGGbbVVff__                                                          
                                                  --LLEEkkSSSSkkhhddUUHHmmHHHHmmHHHHXXAAGGOOpp44ppOOGGUUXXHHmmmmmm8888mmRRDDRRDD##ggNNNNMMWWWWMMNNNNNNNNNNNNWWWWNNNNWWNNNNNNMMMMDDgg$$BBWWNNMMNNMMNNWWWWNNNNNNMMMMNNNNNNMMWWWWWWNNNNNNNNNNNNNNNNWWNNWWWWMMNNWWNNMMNNWWNNggDD888888HHmm88mmXXXXAAKKbbAAXXAAKKXXAAKKHHmmmmHHHHHHXXmm88HHbbGGbbOOdd44hhPPkkwwEE2222aa]]EESS22]]yyjjjjjjyy]]]]]]]]22EE22EESS22aa]]22EE22EESSSSEESSwwkkkkPP666644pp44OOVVppGGAAUUpp66ii''                                                          
                                                    cc]]PPSS22kk6644AAHHHH8888XXHHHHmmHHKKKKKKbbAAXX88mmXX88DDRRBBBBBBBBggMMNNWWNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNQQMMDD$$00MM00MMNNWWNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNMMBB##DDRRRRRRRRRR88mm8888mmHHHHmmHHmmHHXXAAHHmmXX88mmRRHHXXbbbbbbVVddhh99hhqqEEEEEE]]]]2222]]22EE22yy222222aayyaa22EEEEEE22EE22aaaaEEEEEEEESSSSwwwwSS66kkkkhh44VVppOOppAAOOGGUUOObbpp{{''                                                          
                                                    ;;ZZkkkkSSEE66hhVVUUmm##DDHHHHHHmmmmHHHHmmXXmmHHHH88DDggNNWWNNNNMMMMMMMMNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNWWWWMMgg00ggDD$$$$####ggNNWWNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNWWNNNNNNWWWWNNMMWWNNMMMMMM00ggBB$$######DDRR8888mmHH88RR88mm88RRDDmmHHAAbbbbVV44VVhhkkqqwwSSEESSSSEE22]]22EEEEEEEEEE22yyyyaa22EEEEEE2222]]jjyySSEESSwwwwwwwwqqkkPPPP669944pppppp44GG44ppUUGGUU44)),,                                                          
                                                    ^^ooqqSSqqqqPP66ddGGHH88mmmmmmHHHHmmHHHHmmHHHHXXHH##0000MMNNNNWWWWWWNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNWWWWNNMM00BBBB$$##RR##$$$$ggMMWWWWNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNWWNNNNNNNNNNNNWWWWNNNNNNNNNNNNMMMMBBggMMgg$$##DD##88$$######mmRRRRmmmmAAbbGGVVddVV66qqqqqqSSEEEESS22]]EEEE2222EEEE22aayyjjaa22EEEEEEEEEE]]yyyySSEESSwwwwwwqqkkPPPPPP66hh44ppppppppGGppbbAAAAKKhhss                                                            
                                                    ''uukkEEwwwwqqPP99GGmmRRDD$$RRHHHHHHHHHHHHHHHHmmHH88$$$$BBNNMMMMNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNWWNNBB####DDDDRRmmXXmmRR##$$ggMMMMNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNMMMMMMMMNNMMMMNNNNNNNNNNWW00MMWW00gg00BBgg##BBBB####888888mmmmAAGGGGVV4499hh66kkwwSSEEEESS2222SSSSEE22EEEE22aayyyyaa22EEEESSSSSS22aaaaSSEESSSSwwqqkkPPPPPP66PPPPhh44ppVVppOOOObbbbUUAAEE//                                                            
                                                    ..iiwwSSqqwwEEwwqq44KKHHHHmmRRHHHHmmHHHHmmmmHH888888$$ggMMWWNNWWWWWWNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNWWgg####RRmmmmHH888888DDggNNQQQQNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNMMNNWWWWWWNNNNWWWWWWWWWWNNNNNNNNWWMMNNNN00NNWW00$$gg0000##88DDDDRR88mmAAbbGGVVVV999966PPkkqqwwSSEEEE222222EEEEEEEEEE]]]]]]2222EEEEEE22222222]]EEEESSwwqqqqkkPPPPPP66PPkk66ddVVVVppOObbAAUUKKXX]]!!                                                            
                                                    --??yywwqqSSSSqq66ppKKmm88DD88HHHHmmHHHHmmHHXXmm88RRggWWNNMMNNWWNNNNNNNNNNWWNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNMMggBB$$88HHmmmmHHHHmmRR##ggMMNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNWWWWNNNNNNNNNNNNNNNNNNNNQQWWMMWWQQ00BB00BB00BBDDBB$$RR88mmAAGGGGVV44VVhhPPPPPPkkqqqqEESSwwEEEESSSS22EE22222222222222EE]]yyyy]]2222EE22wwkkkkwwqqPP66PP6666669944VVVV44ppGGUUGGKKAAjj<<                                                            
                                                      ;;55PPSSjjaaEE66ppbbXXDD$$mmHHmmHHXXHHHHXX88mmHHXX88ggWWNNMMMMNNNNNNNNNNWWNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNMMBB##DDRRHHXXHHmmHH88DDDD####ggNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNWWWWNNNNNNNNNNNNNNNNNNNNWWWWWWWWNNWWWWNNNNWWNNMMMMggBBggggggggDDRR88HHUUGGGGpp444499hh6666PPkkqqwwkkPPkkkkPPqqSSEE22]]222222]]aa]]aajjjj]]22EEEESSwwqqqqwwqqPP66PP66hh99dd44VVVVppbbUUUUbbHHXXjj<<                                                            
                                                      ==eekkEESSSSwwPP44OOKKHHHHXXHHmmHHXXmm88HHHHHH888888$$NNNNNNWWWWWWNNNNNNNNNNNNNNNNNNNNNNNNNNNNWWWWNNNNMMMMMMBB##RRmmmmmmHHmmHHXXHH88##00WWNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNMMNNNNWWWWWWWWNNNNWWWWNNNNNNNNNNNNNNNNNNNNNNMMNNNNggNN00gg00##RRDD88HHUUbbbbOOpp99pppp99PP66PPwwwwqqqqwwwwwwwwSSSSEE2222EE22aaaayyyyjjyy22SSEEEEkkqqwwwwkk6666PP66PP66994444VVppVVOOGGGGbbHHKKYY==                                                            
                                                      ''ll66qqjjyy22kkhhVVAA8888HHHHHHHHHHHHHHmmmmHH88DD####BBNNMMMMNNWWWWNNNNWWMMQQQQMM00NNWWNNNNNNWWWWNN00NN$$ggDDHHHHmmmmHHHHHHmmHH88$$####NNNNMMMMWWWWMMNNWWNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNWWNNNN00$$BBBB88mm88HHUUGGbbGGppVVVVVVddPPkkPPqqqqkkqqwwwwqqwwEE2222]]]]22EE22]]aayyaa22EESSEESSkkqqqqqqqqPP66PP66PPPP9944VVppppUUppbbbbGGmmUU}}..                                                            
                                                      --||qqPPSSYYaaqq99ppKKmmXXHHHHHHmmmmmmHHHHXXXXXXXXHHmmRR##$$BB00MMMMMMNNWWWW00ggNNQQQQWWWWQQWWWWMMNNNNQQgg$$DDmmHHHHHHHHHHHHmmXXXXmm88##QQQQWWWWWWWWQQWWMMNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNMMMM0000gggg$$RRRRDDmmKKbbbbGGOOVVVVVV99PPPP66kkkkPPkkwwwwwwwwEEEE]]yyaa2222EEEE22]]]]EEwwwwwwqqqqwwqqqqqqkkPPPP666666hhddVVppppVVGGbbKKAAKKOO}}--                                                            
                                                      ''**aaPPqqaa]]ww66VVKKmmHHmmHHHHHHmmmmHHHH888888mm88DD##$$$$BBMMggNN00NNNNMMWWWWWWNNMMMMNN00MMWWNNWWNNNNDDDDRR88mmHHHHmmmmXX888888DDDD##00ggMMNNMMMMWWWWWWNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNWWNN00gg00BBDDRRmmHHKKbbOOGGOOppVVppVV99PP6666kkqqqqwwEE22EEEE22EE]]]]22SSEE22EESSEEEEwwkkkkkkqqqqqqqqqqqqPP66PPPPhhhh66hh44ppVVGGppbbbbHHmmSSLL,,                                                            
                                                        ;;xxqqEEwwSSwwPP44AA88mmmmHHHHHHHHHHmmmmXXHHHHHHHHHH8888RRRR$$DDBB$$gg$$gggg00MMNNWWWWMMWWQQWWMM00ggNN0088RRRRmmHHmmmmHHHHHHXXKKmmRRDD##00WWQQWWMMMMNNWWNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNWWNNggBBgg$$8888mmmmKKUUbbbbGGOOVVVV44hhkk6666kkkkkkqqwwEEEESSSSEE]]22EESSEE22SSkkqqkkPP6666kkwwqqqqqqqqkkPPPP66PPPP6666hh44VVVV44UUbbXXmmUUEEcc                                                              
                                                        __llqqqqwwwwww66VVKKmmXXHHHHmmmmHHHHHHmmmmmmmm88mmHHmmDD##DD$$RRggBB00$$BB$$BBNNWW0000WWMM00gg$$BBDD$$$$mm88mmHHHHmmHHXXmmmmHHHH88DDDD##MM00MMNNNNNNMMNNNNNNWWWWWWWWWWNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNWWMMBB$$gg##88RRmmHHAAbbOOppppVVVVVV44hhkk6666kkqqqqwwSSEE222222EE2222222222SSkk666666PPPPPPkkqqqqkkqqqqPP666666hh66hh994444VVppbbppUUGGXXRR]]<<                                                              
                                                        ``))22hhwwSSwwPP44UUHHHHHHmm8888mmHHHHHHmmHHHHmmHHXX88$$##DD##RRBB##$$DD$$ggMMNNgg$$BB0000ggBBggNN$$DD88HHHHHHHHHHHHXXAAAAXXmmmmHHmmRRRRBB$$BB00MMMMMMWWMMMMMMNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNWWMMBBBB$$DD8888RRmmKKAAbbGGOOOOppVV4499PP6666kk66666666kkwwSSSSEEEESSwwwwwwkkPPPPPPPPqqqqqqqqqqkk66PPkkhh99hhdd44ddddVVppppppOOppUUUUKKXXOO55==                                                              
                                                        ``<<yy4466qqSSqqhhGGHH888888RR88mmHHHHHHHHHHHHmmmmmmRR$$########$$########$$$$##BBMM00##ggBBDDRR$$88RRRRmmHHHHHHHHXXKKAAHHHHmmmmXXHHDDBBBBggMMMM0000MMWWMMMMMMNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNWWMM00gg$$DDRRmmHHAAUUGGGGOOppppppppVVdd66PP66PP66PPPP66kkSSEE2222SSkkPPPPPPPPPPPPPPkkqqqqqqqqqqkkhh66PPdd44ddVVVVVVVVppppppOOOOUUOOUUXX##bbCC--                                                              
                                                          ==YYVV99PPqqkk66OOXXmm##DDRRmmmmHHmmmmHHmmmmHHHHmmHHHHHHHHHH88888888DD$$##########DDDDDDBB$$DD$$88RRRRmmHHHHmmmmXXKKKKKKAAXXmmmmHHmmmm##$$$$$$ggNNNNMMNNWWWWWWWWWWNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNMM0000gg$$####88mmKKUUbbbbOOppppppVVVV44hhPPPP6666PPPP66PPwwEEEESSwwqqqqPP6666kkPPPPPPPPPP66PPqqqq66PPPPdd44ddVVVVppppVV44ppOOOOGGbbUUmmRRSSLL''                                                              
                                                          --11ppVVhhkkPP66VVKKmm88mmRR##mmDDRRHHHHHHmmmmHHHHHHHHHHHHmmmmHHHHmm88DD$$$$DDDD##DD88$$$$$$DDDDRRmmHHHHHHmm88mmAAAAXXHHKKKKmm88XXHH##DD$$BBggMM000000WWMMMMNNNNWWNNMMWWMMNNWWNNNNWWNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNBB$$$$RRDDDDXXGGGGUUOOpppp44pp44ddhhPPPP66666666PPPPkkqqwwEEEEwwkk66PPPPPPhh66666666PPkkqqqqkkPP666699VVppVVVVVVVVVVVVOOGGGGbbbbHHmmHHww!!                                                                
                                                          --((hhbb44999999VVAAHHDDRRDD$$88DDRRHHHHHHHHHHHHHHHHHHmmmm88888888RRDDRRRRRRmm88RRDDDDRRRRRRmmmmmmmmHH88KKXXXXHHHHKKXXUUAAXXXXHHmmHHXXHHDDBBBBBBBB00QQMM00MMNNNNNNWWWWNNMMNNWWMMNNWWNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNMMWWMM00BB##DD88HHbbGGbbppVVOOppVV44VV4499hh66PP6666PPPPkkwwEE22SSqqPP666666666666666666PPkkkkqqPPPP6699dd99hh99OOppppVVVVVVppOObbOOKKmmXXYY==                                                                
                                                            //qqUUdd9999ddOObbAA88mmmmDDmmRRRRHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHmmmmXXHH88RRRR88mmHHmmmmmmHHHHHHmmmmHHHHUUAAKKAAXXUUAAKKUUUUAAmm88HHKKmm##ggMMMM0000WWNNNNWWMMMMNNNNWWNNWWWWMMNNWWNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNMMWWMMBB##RRRRHHAAGGbbbbppVVVV44pp44ddhhPPPP6666PP66PPPPkkqqSSSSqqkkPPPPPP6666PP66666666PPPPPPkkqqkkPP99dd99ddVV4444VVVVVVppOOGGbbGGKKmmGG33''                                                                
                                                            <<22AAVVppppppbbGGAAHHXXXX88HHRR88mmmmHHHHHHHHHHHHHHmmmmHHHHHHHHHHHH88mmHHmmmmHHXXHHHHmmmmmmHHmmmmHHXXUUXXbbGGKKGGbbUUGGGGAAHHHHHHHHRR88HH88BBMM0000MMggMMMMMMMMNNWWNNMMNNWWNNNNWWNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNWW00$$RR8888AAGGOOGGGGVV44444444ddddddhh66PPkkPPPPPPPPkkkkqqqqkkPPkkwwqqPP666666666666PPPPPPPP66hhhh66hh99dd99VVVVppVVVVVVppOOppOOKKAAqqTT``                                                                
                                                            ::oobbbbAAbbOOOOGGKK88mmHH88mmRRmmmmmmHHHHHHHHHHHHHHmmHHHHHHmmmmmmmmmmHHXXHHHHHHHHmmHHHHmmHHHHHHHHHHUUXXGGbbbbGGAAOOUUppppUUXXXXXXmmXXHHmmDDgggg$$$$WWMMNNWWNNNNNNNNNNMMMMNNNNWWWWMMNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNWWMMgg$$##RRbbGGGGOOVVdd44VVppdd999999666666kkPPPPkkqqqqqqqqqqkkPPqqSSwwPP666666666666PPPP666666hh66PP6699dd9944VVVVppVVVVppOObbUUmmHH22!!                                                                  
                                                            --iiddUUGGOObbbbbbKK8888mmRR88RRmm88mmmmHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH8888HHHHHHHHmmHHHHHHmmmmHHmmAAUUOOGGAAOOOOUUbbGGGGAAXXHHmmmmmmmm88DD##BBMMNNMMNNWWWWNNNNMMWWNNNNWWNNWWWWNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNMM00BB####RRUUbbbbGGVVVVVVVVVVddhh66PPkkPP6666PPPPkkqqwwSSwwqqqqkkkkqqkkPP66PP66666666PPPP6666PPkkkkhhdddddd449944VVppppOOGGGGbbOOXXmm55,,                                                                  
                                                            ``**PPXXAAbbXXAAAAKKHHmmHH8888RRmmRR88mmHHHHHHmmHHHHmmmmmmmmmmHHHHHH88mmHHXXXXHHHHmmmmmmHHHHmmmmHHXXHHXXOOGGGGGGGGVVOObbAAKKXXmmHHXXHHHHmmDDBBBB$$$$DDDDBBMMMMNNNNNNWWNNNNNN00MMNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNN00BB##8888RRAAGGbbOOVVpppp44ddhh669999hh6666kkPPPPPPkkqqwwwwqqqqqqkkkkPPPPPPPP66666666PPPPPPPPhh66669999hhhh99VVVVppppppppppppUUGGHHGG{{''                                                                  
                                                            ,,>>]]RRDDmmHHOObbXXHHmmXX88mm88HH8888mmHHHHHHmmHHHHXXHHHHHHHHHHHHmmXXHHHHmmmmmmHHHHmmHHHHHHmmmmXXAAbbGGGGOOOOOOppOOOOGGUUKKXXXXmm88HHHHHH88DDRRRR##BBggNNQQNNNNNNNNMM00MMgg##$$ggMMNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNN00ggBBRR8888bbbbGGVVdd44VV4444hhPPPPPPkkkkkkqqqqqqkkqqqqwwqqqqqqqqkkkkkkkkPP66PP666666PPPPPPPPPP6666PPPP99dddd4444VVppOOOOOOOObbUUXXSSrr                                                                    
                                                              ==5588##RRmmKKAAXXmmHHmm888888mmmm88mmXXHHmmmmHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHmmHHKKXXHHAAVVOOppVVVV44dd44ppGGAAXXXXHHHHHHHHHHHHmmRR##$$##00MMggMMWWMM00BBggDD####88mmRRBB00NNWWWWWWNNMMNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNWWNNNNWWNNMMNNWWggBB$$DD88mmAAGGGGbbppddVVVV99hh66PP66PPPPPPkkkkwwqqqqqqqqqqqqkkqqkk6666PPPPPP6666666666PPPPPP66PPPPPPPP6666hh9944ppVVpp44OOOOOObbGGbbjj==                                                                    
                                                              ``IIKKBB$$##HHUUAAHHHHHH888888mmmm88mmmmmmmmHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHmmHHmmXXAAKKHHKKOO996644OOVV44OOGGGGbbUUKKHHmmmm88mmHHmmDDBBBB$$##ggBBBBgggg$$HHHHXXmmHHXXmmXXKKDDggMMNNNNNNNNWWNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNWWWWNNWWWWNNWWWWMMMMgg$$DD88AAGGGGbbpp44VVpp444466PPPPkkqqkkwwqqwwqqPPPPPPPPkkkkPP666666PPPPPP6666666666PPPPPP666666PPPP66hhdd44VVppVVppVVOOOOOOGGUUddCC''                                                                    
                                                              ''!!dd$$DDRRHHUUAAHHHHHHmmmmmmmmmm88888888mmHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH8888mmXXbbdd4499hh4444hh99GGOOGGbbKKHHmmmmHHmmHHHHRR$$gg$$DD##gg0000##mmmmmmmmGGVVVVVVOOGGXX88BBNNNNNNNNMMNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNMM$$$$DD88RRmmUUbbGGGG44994444dd99hhPPPPqqwwqqwwqqwwkk66666666PPPP6666PPPP66666666666666PPPPPPPPPP666666PP66hhdd44VVppVVOOppGGOOOObbUUyyrr                                                                      
                                                                ^^wwgg####88KKXXHHHHHHmmmmmmmmmmmmmm8888mmHHHHmmHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHKKUUAAXXHH4444hh66hh999999VVVV44VVOOUUXXmmHHHHmm88RRDDRRmmBB$$DD##$$RRmmKKbbdd99ddddhhddAAmmggWWWWNNNNMMNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNMMNNNNNN00MM00##RRDDmmUUUUUUGGVV44VVppVVddhhPP66kkqqqqwwkkqqkkPPPPPPPP666666PPPPPPPP66PP66PPPPPPPPPP66PPPPPP666666666666hhdd4444ppppOOppppbbddCC..                                                                      
                                                                ::llKK$$gg88KKmmHHHHHHmmmmmmmmmmHHmm888888HHHHmmHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH88XXAAKK88mmGG66dd44hhqqkk9999hh66ddppGGGGbbAAKKHHmmmmHHXXHHHHHHKK####mmXXmmHHGGhhddddhhhhdd44bbHHBBNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNWWWWNNNNNNMMgg$$$$mmHH88KKOOGGOOpp4444VVppVVdd66PP66kkqqqqwwqqqqkkkkPPPPPP6666PPPP666666PPPP66PPPPPPPP66hh66PPPPPPPPPPPPPPPPPPhhdd44ppVVppppOOGGSS??''                                                                      
                                                                --LLVV####88mmmmmmHHHHmmmmmmHHmmmmmm8888mmHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHXXmm88HHXXKKbbbbhh66kkSSEESSkk99hh99dd99hh9944OObbAAHHmmHHmmmmmmmmRRHHXXmmmmAA99ddddhhPP99VVOOXXRRggNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNMMggBBDDDDmm88RRXXUUUUppdddd4444VVdd6666PP66PPqqqqwwqqqqkkPPPP66666666PP66hhhh6666PP666666PPPPhh9999666666PPPPPPPP6666dd4444pp44ppGGUUGGxx++                                                                        
                                                                  <<wwDDBBDD##88mmHHHHmmmmHHHHHHmmHHmm8888mmHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHmm88XXUUbbGG44dd99qq]]aa22]]22SSkkPPhhdddd9999hhddVVbbAAXXHHKKAAKKXXXXmmKKGGOOppdddd44VVUUUUUURRggNNWWNNWWWWNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNMMggBB$$BBDDRR88HHUUGGGGOOVV44ppVVVV446666PP66PPPPPPqqqqqqkkPP66666666PP666666hhhhhh66666666PPPPhh9999666666PPPPPPPP66hh444444ppVVppGGUUdd}}''                                                                        
                                                                  ``{{KK00DDDD88mmHHHHHHmmHHHHHHHHHHmm8888mmmmmmHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHmmHHXX88HHOO44kkww22]]aayyaaEE2222EEqqhhdddd99999999ddVVOOOOppppGGOOGGGGppOOVV99GGUUOOKKRRRRBBMMNNNNNNWWWWNNWWNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNWWNNWWWWNNgg$$$$$$88HHXXAAGGGGGGOOddddVVVVVV44hh66kk66PPPP66kkkkkkkkPPPP6666666666PPPP66hh99hh666666PPPP6699hhPPPP66666666PPPP66ddddddpp44ppppOOEEcc                                                                          
                                                                  ''//hh##DDDDDDmm88HHXXmmmmHHmmmmXXHH8888HHXXHHmmHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHmmmmmmHHAAUUbbbbddkkqqEE22EE22wwSSwwwwqq669999dd999944VVppGGOObbGGUUOOUUGGUUAAUUOOXX88XXmm88ggMMNNNNNNWWWWWWWWNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNMMWWNNNN$$##MMDDHHmm88XXUUbbGGOOVVppVVdd44ppddPP666666666666PPPPPP66666666PPPP66PPPP66666666PPPP996666PPPPPP666666PP6666PPkkPPhh99ddVVOOVVppUUddoo,,                                                                          
                                                                    ::eemm$$DD88RR88mmXXHHXXKKXXmmHHHH8888mmHHmmHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHKKXXKKbbUUHHXXbbGGddhhkkqqwwSSqqwwwwwwkk994499hhdd99dd9999VVVVppGGUU88mm88HHHHXXRRXXKKmmDD88$$%%NNNNNNNNWWWWNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNWWMMggWW00##BB88XXHHKKUUGGGGOOVVdddddd99dd449966666666666666PPPP66PPPPPPPPPPPPPP666666PPPP666666hh66PPPPPPPP666666PP6666PPPP6699VVVV44ppVVVVVV]]))``                                                                          
                                                                      TT44##$$##DDmmHHHHXXKKXXXXXXXXmmmmmmmmmmmmHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH88RRmmKKXX88HHbbAAppdd66kkkkqqPPPPhhhh99ddddhh9999ddVV44ppUUUUAAKKAA88KKHHHHmmDDDDRRDD$$NNgg00NNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNWWWWNNNNNNNNNNNNNNMMggBB####$$XXHHHHKKbbOOpppppp4444dddddd99hh66666666666666PPPP6666PPPPPP6666PP6666PPPPPPPP66666666PPPPPPPP666666PPPP66PPPPhhddhhOOppVV4444ddYY>>                                                                            
                                                                      >>aa88BB##DDHHHHmmXXXXHHXXAAXXHHHHHHmmHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHXXXXHHmmmmXXXXXXGGpp44999999994499dddd9999hhhhdd44VVGGGGUUKKAAAAKKXXmmmmXXRR88$$RRggMM$$ggMMWWMMNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNWWMMgg$$##$$mmmmmmHHUUpp4444ppppVVVV44ddhh66PP666666666666PPPP66hh66kkkk66hh6666666666666666666666PPPPPPPPPP6666PPPP66PP66hhdd99VV99hh99GG66ii''                                                                            
                                                                      ``ss99$$$$DDmmmmmmXXKKKKUUUUXXHHXXXXHHmmHHmmmmmmmmHHHHHHmmHHHHHHHHHHHHHHHH88HHmm88HHbbOOUUbbbbGGbbGGpp44dd44ddhh99VVppVV44bbGGKKHH8888HHmmHH88XX88mmRRDD$$##00NN00NNMMNNNNNNNNWWWWNNNNNNWWNNNNNNNNNNNNNNNNNNNNNNNNNNNNWWWWMM$$DDgg$$mmXXHHHHXXAAGGVVdd9944dddd4444ddhhPPPP666666666666PPPPPP6666kkqqPP66PP6666666666666666PPPPPPPPPPPPPP6666PP666666666699994499PPkk44jj<<                                                                              
                                                                        >>]]####RR88mmHHKKAAUUbbbbUUUUUUAAXXmm88HHHHmmmmHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHmmmmmmmmGGbbGGUUAAAAXXKKOOOOOOGGUUAAUUAAbbGGAAXXmmHHXX88mmmmHHmmDD88BB##$$ggMMMMQQMMMM00MMNNWWWWNNNNNNWWNNNNNNNNNNNNNNNNNNNNWWWWWWWWWWNNQQBBRRDD##88HHHHKKbbppVVVV449999dddd444444996666666666666666PPPPPP6666PPqqkk666666PPPPkkkkPPPP66PPPPPPPPPPPPPPPPPP66666666666666PPddVV99PPww{{''                                                                              
                                                                        ..77bb$$DDDD88HHXXKKUUbbGGGGGGGGbbAAXXXXXXHHHHmmHHHHHHHHHHHHHHHHHHHHHHHHmmmmmmXXXXmmmmXX8888KKAAUUbbAAUUAAXXHHXXHHXXXXmm88HHmmmmmmmmXXmmHHHH88DDgg##$$DD$$BBBBDDBBBB000000MMNNWWNNNNNNNNNNNNNNNNNNNNNNNNWWWWNNNNNNNNNNNN00BBRRKKXXmmKKUUUUpp4444VV44dddd44VVVV44ddddhh6666666666PPPPPPPPPP666666PPPPPP6666PPkkkkkkkkPP66PP666666PPPPPP666666666666666666hh99dd99ddxx!!                                                                                
                                                                          ^^aaHH$$gg##mmHHHHAAUUUUbbbbbbbbbbbbbbKKXXHHHHHHHHHHmmHHHHHHHHHHHHHHHHXXHHmmHHHHHHmmmmXXHHKKXXXXKKXXAAXXXXKKKKHHHHHHHHHHHHmmXXHHmmHHmm88HHHHmmmm$$DDggDD##gg00NNBBggNN00MMNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNMMMMMM00DDDDRRmmHHKKGGOOppVV44VV44ddddVVdd44dd999999hh6666666666PPPPPPPPPPPPPP6666PPPPPP666666666666666666666666PPPPPP66PP666666666666PP66dd44ddPP}}--                                                                                
                                                                            ccPP$$BB88DDHHAAAAAAbbbbbbbbbbGGbbAAAAUUXX88HHHH8888mmmmHHHHHHHHHHmmmmmmHHHHHHHHHHHHmmmm88KKHHmmAAKKmmHHXXKKXXHHHHHHHHHHHHHHHHXXHHmmHHHHHHmmDD##DDDD##$$$$BBggBBBBgg00NNWWNNWWWWNNWWWWMMWWNNNNNNQQMMWWNNMMMMgg$$$$BBDDHHXX88mmGGddVVpp99hh44VVVV4444dd44VVVV99hh66PP66PPPP6666PPPPPP6666PPPP666666PPPPPP66666666PPPP6666PPPPPPPPPPPPPP66PPPP6666PPPPhh9999ddxx++                                                                                  
                                                                            __11KKBB88mmmmKKUUKKUUbbbbbbbbGGGGUUGGGGKKmmHHHHmmXXmmmmmmmmmmmmmmmmHHHHHHHHmmmmmmmmHHXXHHAAXX88XXXXHHXXXXXXXXXXHHHHHHHHXXKKAAAAXXHHHHmmmmmmHHHHmm88RR$$BBggMMNNMMMM00MMNNNNWWNNMMWWNNMMWWNNNNNNQQNNMMNNMMgg00MMgg##mmHHXXHHXXGGVVppVVVVppppVV44ppppVVVVppVV99hhhh6666PPPP66PPPPPP66PP66hh66PPPPPP66PPPPPPPPPP6666666666PPPPPPPPPPPPPP6666PP6666PPPPhhhh44kk{{..                                                                                  
                                                                              //wwBBRR88mmUUGGbbUUbbGGGGGGOOOOOOOOGGAAKKKKHH88mmHHHHHHHHHHHHHHHHHHHHHHHHXXXXXXXX88HHHHKKKKmmHHKKXXKKXXHHXXXXXXmmmmHHXXXXKKKKXXHHmmHHHHHHmm8888RRRR####$$BB0000MMggNNWWMMMMWWWWMMNNMMWWNNNNMMWWMMNNNN00$$##$$88KKXXHHAAOO444444ppVVVVVVVV44VVVV4444VVVVVV99hh666666666666PPPP66hh6699dd9966PPPPPPPPPP6666666666666666PPPPPPPPPP66PP6666PP66PPPP6699hhddxx!!                                                                                    
                                                                              ..}}XX88##RRUUHHUUUUbbOOOOppVVVVppppGGbbUUAAXXmmmm88mmmmHHHHHHHHHHHHHHXXXXKKAAAAAAXXAAKKKKKKHHmmXXHHXXXXHHHHKKKKHHHHHHHHmmHHHHHHmmmmHHXXHH88DD######$$##$$BBBBggMM00WWWWNNNNWWNNNNWWNNNNNNNNNNNN0000ggBB##DDmmmm88HHHHUU4499dd44ppVVVVppOOppOOOOVV44VVppppddhh66PP66666666PPPPhh9999dddddd99hh66PPPPPPhhhh99hh66PP6666PPPPPP666666PP6666PPPPPP66999999wwFF''                                                                                    
                                                                                <<jjAABBmmAAKKOOUUbbGGppVV44VVpp44ppOObbAAKKKKKKHHHHHHHHHHHHHHHHXXKKKKXXKKKKKKKKHHXXHHHHKKXXmmXXmmHHHHmmHHKKKKKKXXHHHHmmmmmmmmHHXXHHmm88mmmmRR##RRDD##BB00BB$$ggMM0000NNWW0000QQMMNNNNNNWWWWMMgg$$####DD88HHHHmmbbGGVVdd999999444444OObbOOVVppOOVVVVppppdd9966PP66666666PPPP66999999dddddd99hh666666hh9999hh66PP6666666666666666PP6666PPPPkkPPhh9999oo<<                                                                                      
                                                                                  ccPP##8888KKXXUUbbGGpp4444ppOOVVppppOOUUAAUUAAAAAAKKHHmmHHXXKKXXKKKKXXXXKKKKXXAAKKAAKKUUbbAAUUXXXXXXmmmmHHXXHHHHHHHHHHHHHHHHHHHHHHmmmmmmmm8888RRRRDD$$BB####BB$$0000gg00MM0000gg00ggMMNNNNgggg##$$##88mmmmKKGGbbGGVV44VV4444OOVV44VVGGpp4444VVppVVVV4499hhhh6666666666PPkkPP66hh9944ddhhPP66hh99hhhhhhhhhhhh666666666666666666PP66666666kkkk66ddqq77--                                                                                      
                                                                                  __{{VVmm88GGUUUUbbGGppVVVVppOOOOpp44ppbbGGGGAAKKKKXXHHmmmmHHXXHHKKAAKKXXKKAAAAAAXXAAXXXXKKHHmmAAKKHHHHmmmmmmmmmmmmHHHHHHmmmmHHmmHHHHHHmm8888mm##$$##DD##$$$$BB##MMMM$$$$MMMMBBBBMMgg00gg00$$$$DD##RR888888XXGGOOpp44999999ddVVGGppppOOOOOOppVVVV4444ddhhhhhh6666PPPP66PPPPPPPPhh99dd99PPkkPPhh9966666666666666PPPP666666666666PP66PPPPPPkkkk66kknn<<                                                                                        
                                                                                    __11UURRHHAAAAUUbbOOOOOOOOppppVV44ppbbOOVVGGUUUUUUAAKKKKKKAAmmKKAAKKXXKKAAAAKKHHUUUUAAGGbbbbKKHHmmmmHHHHHHHHHHmmmmHHHHmmmmHHHHmmmmmmHHHHmm88mmDD##RRDD$$$$##00gggg0000BB$$0000WWMM00BBBBDD$$DD88HHHHHHAAbbGG44VVVV99dd444444ppppOOpp44VVppVVdddd4444dd99hhPP66PPPP6666666666666666666666hh9966PPPPPP666666PP666666666666666666PPkkqqkkkkPPhhyyLL--                                                                                        
                                                                                      ;;xxAAXXUUGGUUUU44GGOO44OOpp44444444VVppppbbbbUUAAKKAAKKXXAAHHHHHHKKKKHHUUKKKKHH88XXKKmmKKGGAAXXXXmmmmXX88HHHHHHHHHHHHHHHHHHHHmmHHmmmmRRDDDDHHDD####$$##00NN##$$##BB##DDgg$$0000ggggBBBBDDHHmmmmXXUUGGGGbbVVVV444444VVppOOOOOOVVVVppppVVVVpphh99dd44ddPPhh66PPPP66666666hhhhhh66666666PPPPPP6666PP666666PPPPPPPPPP66PPPP66PPkkSSaaSSqq66qq{{..                                                                                          
                                                                                        !!jjUUmmAAppbbAAGGVVGGpp44VVOOppddddVVppOOOObbAAAAAAAAKKmmXXAAXXXXXXmmmmmmOOAAAAbbbbppAAHHXXUUGGXX88HHHHHHHHHHHHHHHHHHHHmmmmHHHHHHHHHHHH88DDRR$$$$DD##mm$$DD$$88##BBgggg##BB##$$######RRmmmmXXAAbbGGGGGGVVVVVVVVppppppppppppVVVVppppppVV44dd994444dddddd66PPPP6666PP6666hhhh666666666666666666PP666666PP66666666666666PPqqSSEEEEwwqqPPYY++                                                                                            
                                                                                        ..LLwwXXXXbbbbGGGGOOppppVVpppp4499dd4444VVppOObbbbbbUUAAAAbbKKHHmmmmXXXXmmmm88OOGG88HHXXGGbbXXmm88mmXX88HHHHHHHHHHHHHHHHHHHHHHHHHHmmmmHHKK######mmmm$$##$$##gggg00gg$$####$$RRRRRR8888mmmmHHAAGGGGbbGGppVVppppppppppppppVVVVVVVVVVVVpppp44VV994444hh44ddhh66666666PPPP66hh66PPPP66666666666666PPPP66PPPPqqkkkkkkPP66PPkkqqSSwwwwwwwwxxTT--                                                                                            
                                                                                          ..iippKKAAAAppAAKK44GGOOVVVV4499994444VVVVVVppOOOOGGbbAAAAmmKKAAmmHHmmXXmmXXHH88KKbbbbXXHHmmHHHHHHXXHHHHHHHHHHHHHHHHHHHHHHHHmm888888mm88HHRRHHmmBB$$$$##DDDD$$BBBBgg00##DD888888HHmmHHHHKKUUOOGGUUbbppOOppppppppppppppVVVVVVVVVVVVVVppdd4499ddddhhdd9999hhhh666666666666PPPPPP66666666PP666666PPPPPPkkqqqqqqqqkkhh66kkSSSSqqwwqq22ii::                                                                                              
                                                                                            ,,}}hhbbAAKKXXAAOOGGVVVVpppp99hh44ppdddddd4444VVVVppppOOXXXXKKmmKKAARRXXGGAAXXGGbb88GGAAKKGGGGmm88HHHHHHHHHHHHHHHHHHmmHHHHHHmmmmmmHHXXHH$$DDmm88mmDD##BB$$DD88DDDDmmRR88mm8888mmmmmmAAAAbbGGbbUUbbppOOOOppppppVVVVppVVVVVVVVVVVVVVpp4499dd99dd4499dd9999hh6666666666PPPPPP666666PPPPPPPP66666666kkqqqqqqwwwwqqPPPPqqSS22wwqqkk55++,,                                                                                              
                                                                                              ^^llbbAAXXAAGGAAOOVVVVVVpp44dd444499hh99dddddddd44bbVVppGGUUAAXXHHKKmm88UUXXRRAAXXHHXX8888mmHHHHHHHHHHHHHHHHHHHHHHmmHHHHHHHHmmmmHH88mmXXRRDDRRDDHHXXRRRR$$####RRmmmm88mmmmHHXXHHXXbbbbbbbbbbbbGGOOppppppppppVVVVVVVVVVVVVVVVVVVVVV44hhdd99dd446699hhhh66PPPP666666PPPPPP6666PPPPkkPPPP66666666kkqqwwSSSSSSwwqqqqwwqq22wwPPyyvv``                                                                                                
                                                                                                ++ZZhhAAKKAAXXAAGGOOVV44VVppVVhh99hh6666hhhh9999pp44VVUUAAAAKKKKHHHHHHOObbHHbbbbRRGGGGXX88mmXXHHHHHHHHHHHHHHHHHHHHHHHHHHHHmmmmmmXXmmHH88mmHH##DD##DDHH##RRmmmmRRmmRRmmHHXXAAKKUUUUbbGGbbbbGGOOOOppppppppppVVVVVVVVVVVVVVppppVVVV449999dd9966PP66PP6666PPPP6666PP6666PPPP6666PPPPPP6666666666PPkkqqwwwwqqqqwwwwqqqqSSwwSSii,,                                                                                                  
                                                                                                  ,,llGGUUKKAAKKAAKKUUVVddpppp999966kkqqqqkk6699hh4444ppppppUUGGUUKKbbGGKKmmmmKKHHXX88mmXXHHHHXXHHHHHHHHHHHHHHHHHHHHmmmmHHHHHHHHmmXXmmXXXXmmHHHHHH88XXRRHHHHmmmmHH88mmXXAAAAKKUUKKUUGGbbbbGGOOOOppppVV4444VVVVppVVVV44VVppppVV449999hh44ddkk6666PPPP6666666666PP6666PPPPPP66666666666666666666PPPPqqqqkkqqEE22EE]]qqEExxrr                                                                                                    
                                                                                                    ++uuhhXXXXXXKKAAGGpp44VVpphhPPPPkkqqkkPPPP66hhhh99444444ppbbbbKKAAGGUUXXKKbbAAHHHHXXHHHHXXKKHHHHmmmmHHHHHHmmHHHHHHHHHHHHHHHHHHmmHHHHmmmmXXmmmmmmmm88mmmmRRRRmmmmHHHHHHAAGGbbAAUUOObbbbppOOppppppppVVVVVVVVVV9999dd4444ppVVdd9999hhhh99hhPP66PPPP666666666666kk6666PPPP666666PP66PPPPhhkkSSkkPPwwSSqqwwEEEEEESSqqxx))__                                                                                                    
                                                                                                      ^^uuUU88KKAAmmKKbbOO44VVVVVV99PPkkkkqqPPhh6699dddd99hh99ddVVdd44AAHHUUbbmmHH88HHKKKKHHHHHHHHHHHHHHHHHHHHmmmmmmmmmmmmmmmmmmmmmmHHHHmmHHHHHHHHHHmmmmHHmmmmXXXXmmHHAAKKKKUUbbUUUUUUGGGGOOppppVVppVVVVVVVVppppVV4444VVppppppdd9999hhhh99hh6666PPPP666666666666qqPP66hhhh66PPkk6666kkkkhhPPqqPPkk66PPqqSSEE22SSPP]]77::                                                                                                      
                                                                                                        ::11OORR88mmAAGGppVVppVV4499hhdd99PPkkPPqq66hhhhhhdddd9999VVVV44VVGGUUUUHHmmHHXXXXKKKKKKmmHHHHmmmmmmHHHHHHHHHHHHHHHHHHHHmmHHHHHHHHHHHHHHmmHHHHmmHHHHXXAAAAXXAAbbUUUUbbbbbbbbKKGGppGGVVppVVVVVVVVVVVVpppppp444444VVppppdddddddddddd99hhhhPP66666666666666hh66PPPPPPPP66hhkkPPqqqqPPPPqqkkkk66qqSSwwSS22SSaatt<<                                                                                                        
                                                                                                          >>uuddRRHHmmRRUUVVOOVVppdd99dd99hh9999hhhhhh6666hh666699dd99hh9944OOAAXXHHHHXXXXKKKKXXXXXXHHmmmmHHHHHHHHHHHHHHHHHHHHHHmmXXHHmmHHmmmmXX88HHHHHHXXXXXXKKXXAAUUUUKKUUbbAAUUUUKKGGppOO44VVVVVVVVVVVVVVVVVV4499dddd44VVVVdddd4444dddd99hhhh6666666666666666PPPPPP66hhPPkkPPwwqqwwSSqqqqqqqqPPqqSSqqqqSSEEEEoo**                                                                                                          
                                                                                                            ''33ppXXXXHHUU44dd44ppVV4444444444dddd99hh66PPqqqqqqhhhh99dd4499ddOObbUUbbGGbbbbUUXXAAKKXXHHHHHHmmmmmmmmmmmmmmmmmmmmmmHHmm88HHmmmmXXHHHHHHHHKKAAKKKKKKUUbbAAKKUUbbKKAAAAbbbbGGVVVVVVVVppppppVVVVVVVVddhhhh9999dddd99hhdddd9999hh66666666666666666666PPkkkk6666PPkkkkwwwwwwSSwwqqqqkkkkwwkkPPSSEEEE55TT''                                                                                                          
                                                                                                              >>33ppRRAAKKUUppOO44VVppVVVVpp4499ddhh66hh66kkwwqqqq99ddPP66ddddhhdd44VVppOOGGGGAAAAKKXXHHXXXXHHmmXXXXXXXXXXXXXXXXXXXXHHHHXXKKKKAAKKKKKKKKUUbbbbbbGGUUbbbbUUUUUUUUAAUUOObbGG44ppVVVVVVVVVVppVV4444hh6666hh6666hhhh66dddd999966PP66666666666666666666PPqqkkkkPPPPPPSSSSwwSSSSSSqqkkqqwwkkqq22SSYY77..                                                                                                            
                                                                                                                ::ffppXXAAGGpppppppp444444VVVVVVVV44ddhhPPqqSSSSSSwwSSwwkk66hhddhhdd44OOGGOOGGUUAAAAKKKKKKKKKKKKAAAAAAAAAAAAAAAAUUAAAAAAAAUUUUAAUUUUUUUUbbbbbbGGbbUUUUUUUUAAUUKKbbbbOOppppVVVVVVppVV44VVVVVV44dd66PP66hh6666hhhh66dd44999966PP666666666666666666qqkkkk66hhhhkkwwSSEEEESSEEEESSqqwwEEEEEESSaaFF__                                                                                                              
                                                                                                                  ::{{hhKKGGGGOOOOppppOOOOppVVVVVVVV4466qqqqqqwwwwEE22EEwwwwwwkk66hhhh999999ddOOGGOOGGGGbbUUUUUUUUUUUUUUUUUUUUUUbbAAUUbbUUUUUUKKAAbbbbbbGGbbUUbbGGGGGGbbbbOOOOAAGGOOGG4444ppVVppppVV4444VVVVddhh6666hh99hhhh99dd66dd4499hh66PP666666666666666666qqqqqqqqkk66kkwwww2222EEEEEESSwwSSSSSSwwSS[[<<                                                                                                                
                                                                                                                    ::||PPGGOOUUOOOOVVVVppOOVVpppp44ddPPqqkkwwSS]]jjxxyyyyaaEEwwwwSSkkhhdd9966VVOOppppOOGGbbGGOObbGGGGGGGGbbbbbbbbbbGGGGGGbbUUAAbbGGGGbbbbbbGGOOGGppOOUUAAGGVVVVGGOOVVVVVVVVVVVVVVppVV4499hhdd66PP66hhPPPPhh66PPhh99hh6666PPPP6666PPhh66PP66PP66hhqqkkhh66kkqqEE22EEEEEEEE2222EE22kkqqEEeecc--                                                                                                                
                                                                                                                      ,,{{kkbbOObbOOOObbbbpp4444ppVVVV99PPPPqqEEwwyy55ooeeoojj22EESSwwEEww9999hhhh99ddVVOOGGGGbbOOOOOOOOGGGGOOOObbbbbbbbbbGGGGGGbbbbGGGGGGGGOOOOGGppVVppOOppVVOObbGGpppppppppppp44VVVVVVdd99dd66PP6666PPPP66hh6666hhhh6666PP666666PP6666PP66666666kkPP66kkkkqqSSEEEEEEEE22EEEEEESS22wwjjvv''                                                                                                                  
                                                                                                                        ,,FFSSVVbbppppGGpp44VVOOVVVVpp99PP66ww]]jj55ZZZZeenneeZZjjjjEEqqSSqqPPhhdddd444444ddddddppppppppppppppppOOGGbbbbbbbbbbGGbbGGOOOOOOOOOOppOOppppppppppVVppppVV4444VVVVVVVVpppppppp44ddddhhPPPPPPPP666666hh66hhhh66PPPP66PP66PP66666666kkPP66PP66kkwwwwqqSSSS2222EE2222EE22SSEE55))''                                                                                                                    
                                                                                                                          ''TTxxddbbUUGGOOppVVVVVVVVpp99PP66ww]]yyYYYYxx55ZZ55YY55eeyyPPqq22EEPPhhhhhh99999999dd44444444VVppppppppOOOOOOOOGGGGbbGGOOOOOOppOOppppVVppOOOOOOpppp44VVVVVVVVppppVVVVVVVV44449966996666PPPP66666666hhPP666666PP6666PP66PP66PP6666qqqqPP6666kkwwwwwwEEEE22EEEE]]]]222222xx))--                                                                                                                      
                                                                                                                            ''zzooddbbVVOOGGVVVVppppOO99PP66qq22aaYY55ZZeeeeooooyyZZxx22EEwwqqwwwwwwwwkk669999dddddd4444VVVVVVVVppOOppppVVppOOppppOOppOOppppppVVVVppppVV44VVVVVVVVppppppVV44dd994444dd4499669966666666hhhhPPPP66PP666666PP6666666666PPkk66PPwwqqkk66PPqqwwqqSSEESS22SSEEaa]]SS22ZZvv''                                                                                                                        
                                                                                                                                ++[[99OOGGOOVVOOppVVpp9966PPwwEE22yyjjjjYY55ZZee55eeooZZYYaa2222wwwwSSwwkkPPPPPP6699dd444444dddd44VVppppppOOppppOOOOOOOOOOppVVVVVVVVVVVVVVVVVVVVVVVVVVVV44dd9999dd9999VVddhh99hh666666hhhh66PP6666666666666666666666PPkkPPPPwwkkqq66kkwwqqqqSSEESS22SSSS22wwwwYY77''                                                                                                                          
                                                                                                                                  ++nn99bbGGVVVVVV44ppdd66PPwwEEEEaayyjj55555555ZZZZxxjjjjyy]]wwEEEEEESSqqkkkkPPPP66hh999999ddddddVVppppppOOOOppOOOOOOOOppppppppVVVVVVppppppVVVVVVppppVVdddddd44666666dd99PPPPPP666666666666PPPP666666PP666666PPPP66PPqqkkkkSSkkqqPPqqSSwwwwEEEESS22EESSEEww5577,,                                                                                                                            
                                                                                                                                    ;;11qqGGAAOOpp44ppVV99PPwwSSEEEE2222yyjjjjxxZZ55jjxxxxjjxxaaaa]]EESSwwqqqqkkkkkkPP666699dd44ddVVVVdd44VVppVVVVVVVVVV4444VVVVppVV44VVVVVVVVppVVVV4499hh66hh9999hh66dd99PP6666666666PPPP66PPPP666666PPPP6666PPPP66PPwwqqww22qqwwqqSS22EEEE22SSSS]]2222]]YY((,,                                                                                                                              
                                                                                                                                      ::{{kkppOOppVVppVVhh66PPSSSSEEEESSEEaayyaajjxxaaYYYYyyYYjj22]]22wwqqwwwwqqqqkkPP66hh9944VV444444VVVVVVVVVVVVVVVVVVVVVVVVVVVVppppVVVVVVVVVVpp999999dd996699ddhhPP6666PPPPPPPPPP6666PPPP666666PP6666PP6666kkPPPPkkqqSSEEEESSSSSSEE2222EEEE2222EESSSS5577__                                                                                                                                
                                                                                                                                        __??ZZOObbbbGGVVhh6666qqqqqqSSEE2222EESS22aa22yyjj]]aa22EEEESSwwwwwwwwqqqqqqqqkk66hh99hhdd4444ppppVVVVppVVVVppppVVVVVVVVVVppppVV44VVVV44VVdd99hhhh99hh999966PPPP6666666666666666PPPPPPPP66666666PP66PPqqqqqqwwSSEEEEEEEE]]EESSSS22]]22EESSEE]]ZZ77::                                                                                                                                  
                                                                                                                                            ++11SShh44VVVV9966hh6666kkSSEEEEEEEE22aaaayyjjyyaa]]EESSwwEE22EESSSSPPkkkkPP6699999999hh994444dddd4444VVppVVVVVVVVVVVVVVVVVVVVppVV4444dddd666644dd9999hhPPkkkkPPPPkk66PPPPPP666666PPPPkkPPPPkkPPPPqqkkqqwwSSEEEEEEEEwwEE22]]aayyaaEE22SSYY77::                                                                                                                                    
                                                                                                                                              ::ssoo99ppOOOOdd666666PPkkwwSSEE]]EEEE22EE22aa2222]]22EESSwwqqkkPPkkkkkkkkPP66hhhh996666hh99hhhh99dddd4444VVVVVVVVVVVVVVVVVVVVVV44dddddd66hh44dd994444dd66666666PPPPPPPPPP66666666kkwwwwwwSSwwqqwwSSSSEE22EEEE2222SS22aaaaaayyyy]]]]oo77__                                                                                                                                      
                                                                                                                                                ''!!llEEdd44hhhhhhPPkkPPPPqqwwwwSSEE222222]]2222EE22EEwwqqqqkkPPPPPPPPPPPPPP66664499994444dddd44hhhhhh99dddd4444VV444444VVVV44dddd99dd9999dd99dd66hh66kkkkPPPPkk66666666PPkkkkPPqqSSEEEE22EEwwwwEEEEEE22EESSEE22]]aayyyyjjjj]]EEZZss..                                                                                                                                        
                                                                                                                                                    ,,ss55hhVV4444hhkkPPPPkkkkPPPPqqwwSSEEEEEEwwSSEEEEwwwwSSSSwwPPPP6666666666hhPPPP669999hh66PP6666PPPP66hhhh99hhhhhh99dddd44444499dddddd996699hhhhhhhh666666PPkkPPkkkkqqqqqqkkqqSSEEEE22EESSSSSSSSEE222222]]aa]]jjxxjjyyxxoo33//--                                                                                                                                          
                                                                                                                                                        rr[[wwhhVVddhh66PPkkPP66kkwwqqEE22SS22EEEE22EEwwqqkkPP66qqqqqqkkkkkkkkkkPP66hh9999dd9966PPPPPPPPPPPP6666PPPPPP66hhhh99ddddhh9999hhhhPPhh66666666PPkkqqqqqqwwwwSSSSSSwwqqwwEEEESSEE222222]]]]aajjjjjjxxYYYYYYjj]]22nnTT,,''                                                                                                                                            
                                                                                                                                                          ::))xxhhVVVVddhh6666PP66kkhhkkqqPPSSSSEESSwwSSSSwwkkkk66PPPPPPPPPPkkqqPP66PPPP66hhhhPPPPPP666666PP66666666666666PP66hhhh66hh66PP66PPPP66PPPP66PPqqwwwwqqSSEE22EEEEEEEEEE2222SSEE2222aaaaaayyjjxxjjyyjjxxxxjjYY[[))==                                                                                                                                                
                                                                                                                                                            ''ccttEEhhddppddkkhh66kk66PP6666kkPPqqqqPPhhPPPP6666PP66kkPPPPkkkk66kkPP66PPkkPP66PPPPPPPP666666PPPP66666666PP6666PP66hh9999hh66PPPP66PPPPPPPPqqSSEEEEEEEEEEEEEEEEEESSEE2222EEEE22yyyyYYaajjYYyyxxYYYY22ZZll))''                                                                                                                                                  
                                                                                                                                                              '',,vvoo2266ddhh66996666PPkkkkqqqqhhPPPPPPkkqqkkPPPP66qqkkkkwwwwqqqqqqqqwwSSqqkkPP66PPPPPPkkkkqqwwkkPP666666666666666666PPPPPP66hhPPkkqqqqqqSSEEEEEEEEEEEEEE2222222222]]aa]]]]aajj55jjjjxxyyyyxxxxZZ[[77++__                                                                                                                                                    
                                                                                                                                                                    __??11aaqqPPwwkk66hh66hhhhPP66PPkkkkkkqqkkhhwwqqwwqqqqSSwwqqkkkkkkkkqqwwwwqqqqwwwwwwSSSSSSEESSwwqqqqqqkkkkqqqqkkkkkkkkkkkkkkwwEE22EEEEEE2222EEEEEEEEEE22]]]]yyyyjjjjjjyyjjxxYYyyjjYY5555ZZ[[}}ss__''                                                                                                                                                      
                                                                                                                                                                        ==??IIee]]PPPPPPPPkk6666qqkkkkkkkkSSSSkkqqkkwwwwww222222wwwwqqqqwwEE2222EEEEEEEEEEEEEEEE22EESSEEEEEEEEEE22EESSSSSSEEEEEEEE2222SSSSEE2222EEEE222222]]aayyyyaayyyyyyjjjjxxyyYYYYjjZZeeuu))<<''                                                                                                                                                          
                                                                                                                                                                            __!!vvttooaaqqwwEEwwSSwwwwqqPPqqSSqqSSqqSSSSSS22EE22EE2222EEEE2222EEEEEE22222222222222EEEE2222EEEEEEEEEEEEEEEEEEEEEE22]]]]]]]]aaaayy]]aaaayyyyyyjjjjyyyyyyjjxxxxYYYY55eeoo55ttiizz__                                                                                                                                                              
                                                                                                                                                                                ''::<<TT33uunnxx]]222222SSwwSSSS22SSEEEEEEEEEEEE222222EESSEE2222aaaaaaaayyyyyyyyaa]]]]aaaa]]2222222222EEEE2222]]yyjjxxxxjjjjjjyyyyyyjjjjjjjjjjjjxxxxxxxxxxYY5555eeoo1177cc::--                                                                                                                                                                
                                                                                                                                                                                      ``''^^!!LLIIuueeoo55yy2222EEww22]]]]yyaayyyyyyaa]]EE22aaxxjjxxxxxxxxjjjjjjxxjjjjjjjjyyyyyyxxjjyyaaaayyjjxxjjjjxxxxjjyyyyaajjjjxxxxxxxxxxxxxxYYYYxxYY55ee[[}}((**::                                                                                                                                                                      
                                                                                                                                                                                              ''``,,++cc//JJ33eeYYyyjjxxjjxxjjjjjjjjxxjjaa22aajjyyyyjjjjjjjjjjxxjjyyjjjjxxjjjjxxyyyyjjjjjjjjjjjjxxYYxxxxxxYYYYxxxxxxxxxxYYYYYY55YY55ee[[ll}}((TT++::                                                                                                                                                                          
                                                                                                                                                                                                      ''``--==rr??||ff33uunneeYYxxjjjjyyjjjjjjjjjjxxxxjjyyjjjjjjjjjjjjjjxxxxxxjjYYxxxxxxxxjjxxYYYYZZ55555555YY5555xxYYZZZZooee[[llffFF))??++''--                                                                                                                                                                              
                                                                                                                                                                                                                --``''==++rr??FFff11[[oo55ZZZZZZxxjjxxxxxxxxxxxx55YYYYYY555555YYZZ55555555YY55ZZeeeeoooonneeZZooeeooeeuuuutt}}FFss++::..``                                                                                                                                                                                    
                                                                                                                                                                                                                            ..''==>>cc??TTTT))((CCII11ttuu[[nnnn[[uuuu[[eeooeennooeeeeeeeeeennllffII11IIff33tt33ff}}FFJJTT**==::``                                                                                                                                                                                            
                                                                                                                                                                                                                                      ``  --''::^^++cczz??LLvvvvvv77FFFFFFFF{{}}}}ffff}}}}CC{{FFii||(())sszz??//++^^::``                                                                                                                                                                                                      
                                                                                                                                                                                                                                                        ''  ``--``....''::::==;;>><<>>>>>><<>>>>;;,,::__..''                                                                                                                                                                                                                  
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                              
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                              
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                              
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                              
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                              
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                              
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                              
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                              
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                              
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                              
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                              
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                              
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                              
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                              
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                              
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                              
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                              
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                              
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                              
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                              
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                              
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                              
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                              
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                              
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                              
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                              
                                                                                                                                                                                                                                                                                                                                                                                                                                                

#endif