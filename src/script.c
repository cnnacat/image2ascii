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
					relative_linear_luminance = 0.2126f*linear_RGB[0] + 0.7152f*linear_RGB[1] + 0.0722*linear_RGB[2]; 


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
					relative_linear_luminance = 0.2126*linear_RGB[0] + 0.7152*linear_RGB[1] + 0.0722*linear_RGB[2];

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


#if 0                                                              

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











#endif