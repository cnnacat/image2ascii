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
	float* img_data       = NULL;
	float* pixel_data     = NULL;
	bool is_hdr = false;
	char*          density        = NULL;
	int            density_length = 0;
	int            width          = 0;
	int            height         = 0;
	int            color_channels = 0;


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
                                                              oyyn
                                                             no11u}
                                                            ^y1}}}Z^
                                                            vu{{{}1}
                                                            cu{{{}}t
                                                             31{{{{uc
                                                              [1}}}}[
                                                              ^Z133}ur
                                                               }ou1313
                                                                EZu3{Z.
                                                                v2u13u}
                                                                 EZu3}[
                                                                 nyu3{ur
                                                                 vwZu}1}
                                                                  9Zu}}[
                                                                  oyu3{y^
                                                                  }2Z3{u{
                                                                  vhZ3}1o
                                                                  rwZu31y
                                                                  .9ZZu1j)
                                                                   PyZZuZY
                                                                   22yxZZyv^.
                                                                   2PqqSSPU8OV2}c
                                                                .{2OGpGGGGGGGGGAU9t
                                                              ^2KUXAAAAAAGAGGAGpp4p9{
                                                             {K8AAHHHAAAAAAAAAOppVp9h2
                                                            jR88XXHHAAAAAAAAOAAOOVdd9VP.
                                                           tR88XHHHHHAAAAAAAAAOOOVdh669P
                                                          vR$DD8DDDRHHAAKKAAAUUOOVdhh66h2
                                                         .K$$DD8BBBBDHHHKKKKUUOOVdd9hPkwPn
                                                         j0$DDD0MMM0BDHHHKKKKUOVVd99PqS2wwv
                                                        ^8B$DD$MWWWMBDHHHKKKUUOVVd99PkS222E
                                                        j0B$$$0NWWW0B#RHHKKKUUOVV999PkS222q}
                                                        K$B$$BMNWWW0BDHHHHHKUUOVVd99PkSE2a2E
                                                       {gB$$DBMWQWW0$RHHHHKKUUOVVd9PkkqE]a2w}
                                                       60B$$$BMWQWM0$RHHHKKUUOVVVdPkqkqE2]22E
                                                      .80B$$$BMQQWMBDRHHHHKUUOVVdhPkqqSE2]22hv
                                                      {gB$#$B0WQWWMB#RRHHHKKUOVddhPqqSE22]]22o
                                                      60$D#$B0MWWMMB#RRHKHHUUOVd6kqSSEaaa]]22w^
                                                      KB$D#$B0MWMM0B#RHHHKKUUOdhPqSS22ayaa]2Sq)
                                                     rg$DDDD$BMMMM0$DHHHHKAUUVhPkkSE2ayjya]2E2o
                                                     nM$DDRD$B0000$DRHHKKUOOOdhPkqSE2ayyyyyaaaw.
                                                     V0#DDDD$B00B$DRHHHKKUOVd9hkkSSE2ayyyyaaaa2v
                                                    ^8B#DDD#$B0B$$DRHHHHKUVd99hkkSSE]ayyyya]aaw}
                                                    tg$DDDD##$B$$$DRDHHKUOV9kkPkqSS2ayyyyaaaaa2Y
                                                    60$#DDD###B$D#DDRHHUOOV9kkkkqSS]ayjyyaaa]a2w
                                                    KB$##DDD###DD#DDRHAUOVV9PkPkqSE]yyyyyaaayjahc
                                                   vgB$###DDDRDDDDDRHAAOOOVd9hPqSEEayyyayyyyajaw}
                                                   jWB$##DDDRRDDDDRHHAAUUOVVdhPkSEEaayyayxYxaaawY
                                                   K0B$###DDRRDD#DHHHAUUOOVddhPkqS2aaaaayxxxaaa29.
                                                  vg0$#DDDDDRD8DDRHHAUUOOVddhPkqqS2222aayxxyaaa2hc
                                                  jW0$#DRRDDD888RHHAUOOOVdhPkqqSqSE2222ayyyyaaawd}
                                                  KMB$DRRRD88888AHHAOOOVdhhPkqSSqqSEaaayyyjyyaa2P2
                                                 vg0$DRRRR8888XXAAAAOUOVdhPkqSqqqqS2ayyyxyyjjja2w9.
                                                 6M0DRRRR8XXXXXAAGpOOOOdhPkkqqqSSSE2ayyyyyyyxja2whv
                                                ^80$DRRHHXXAAbGGGppOOVVdPkkqSSSS2222ayyyyyjyjja2wP2
                                                tN0$DRRHXXAbAbG44ppVVVd66kkqSSSE22aaayyyjjjjjaa2wPOr
                                                VM$888XXXAbGGG4444pVVVd66qqqSS22222aayaayxYxyaa2wPVj
                                               vg$888XXAXbOOO4464444Vdd66kqSSSE2222aaayyyjYxyyaSSkPO^
                                               6M$888XXbbOOO4466644d9d6666qSSSS2222aaayjyjxYxxa]SSwdn
                                              rg$88XXXAbOOdd666664P4996P66qSSSS2a22aaayjjjyxxxya]2Eq9.
                                              6$$8XXXAbGddhh6666666PPP666qqSSEE22a22ayyjjjxYxxxxaa2wht
                                             tgBD8XAAbGpdhkkkww6666PPPPPqqqqEEE]aaaaayjjjYYxyxxxya2EqP
                                            r8$88XbbOOddhkkwwwwww6SPSSSSqqqqqEE]]a]]yyjxxYYjyyxxxa2awh{
                                            V$$XXbbOOddhkkkwEEEEwwwwSSSSwPPPqqE]]]]]yxYYooYxjyyxxy2a2wP
                                           60$8XbbpddhkkkkEEEEEEEEwwSww666PPqqq]]]]]yYYooooYjyxyxa222wht
                                          6M$8XAbpdhhkkkwwEEEaaaEEE2wSww66PPqqE]]]2]yYYooooYxxyxxaESSwP9
                                         j$$8XAbOdhkkkwwEEEEaaaaaEE22wwwqqqqwEEE]]2]yjYYoooYxyxxxa]22wPO}
                                        jg$8XApphkww22E2E2EaaaajaaaE222wwqqqqSEE]]22]yjYYYYYxjxxaa]]2Swd9.
                                       jR8Abbpdhkw2aaa22aaaaaajjjaaay22wkkqqqSEE]]]]]ajxYYYxjxyyya]]]SwhOj
                                      tRXbOddhqqqwayyyyayaaajjjjjjjxy2wkqhhqqSSE]]yayyjxYYYxxxyyyy]]2]khdO^
                                     {8AphkkkqqPPwaxxyxjjjjjjjjjjjayy2wq9hhkkqSE]xxxxxxxxxxxxxyyyyy]]]SkdOj
                                    ^KGdqwkkqP999qaxxxyjjjjjjjZjjjyxawq94499hqSaaxYYYxxxjxxxxxyayaaa]a2kPOO^
                                    6bh222wP99ppV92x5xxjajjjZZZZjyyyawP4pp49qq2axYYYYxxxjjxxyyaayayaa]2SqdAn
                                   nAPaxxawP9pbAb4wa5xxjjjjZZZZZxxyy2q4bAb4Pqq2yZZZZYYxxjjjjjjyaaayyaa2SwdO9
                                  .V9255xawP9pAHAGP2x5xjjjjZZZZZjxxywPpKKb9Pq9q2ZeeeZZYYxYxjxjyayyyxy]2SkhVUt
                                  tGwyZZy2qqP4bKKApP2yxxjjZZZZeZxxxaw4bmHb9P99P25eteeZZYYYxxxxyyyyyxya]SqPdOV
                                  P9EZeZy2qqqP9AHHHp92yxxZZZeeeZZja2PbmDRKV99P25tfffeeZZYYxxxjjyaayxyy]ESkdOUv
                                 rVPaZeZy22aooj9KmmHb92xZZZZeeeeZjj29AR#RK9wjolF)))iteeZYYYxxjjya]]yyy]2Sk9VU2
                                 {Gwje[5xolC)FCo9ARDmb92xZZeeeeeZjj2pm$$KwoC))zccczifteZZYxxxjyaa]]]ay]2SkhdVO^
                                 jpwx[[llCC))zzFoPHDDmbwy5ZeeeeeejxwbRBRPolCFzc!!!!Tif[eZYxxxyaayy]2aya]Skh9VUt
                                 P92Z[33CCCC)zcz)lpD$$m4wx5eeeeeeZxwK##bjoojoFz!!c!zTitZeYxxxyyayjyaaaa]Skh9VUP
                                 VP25[3CCljjl)zczF2H$g$K925ZeeeeeZyqHBRPoo29Pxic*!!cz)f[eZxxjyyyyjjyyaa]Skhh9VO^
                                .VPyZtCCljwP2tTzz)l4RggRbPa5eeteeZa9RBHwjj2a2yfz**!!cTi[eZYYjyyyyjjyaaya]SkhhVUt
                                ^VPy[fF3ojjaxfT**zF2HgMgRGwxZet[ZxwpR0H9wj3ffiT*****cz)f[Zoojyyyyjy]]aya]SkhhdU2
                                rVPy[fFljo3FiTz**c)oV#MMgmp2Ze[[[xqb#0RVPoFzzz*!****cc)F[Zooxyyyaaa2]yxa]Shh9VO9
                                ^VPytFFo2l)zz!!**!)3PR0NM$HPyZ[[ZxPKBMgm4o)c!!!!!***cc)F3uooxyxx]2]2]ayy]S6h9VOOr
                                ^VPx3FCj2l)z*!!**!TCwRMWWMDbqx[[ZyPm0MMDpo)c!!!!!***cz)F3uoYxyxxya]EE22]]S6h9VOU{
                                .Vqxl3ljwoFT******T3PRNWQWMRV25[Zy9RMNN$bo)c!!!*!!*zz)CC1uYxxyaayaa]2aEE2S6hhVOX}
                                 VqxooowPjfTz****zTl9#WQQQWMR9a[Z2pDNWW0Hj)zz**!!!*T)CllZZYxja]aaaaaa2SSESkkhVOUj
                                 Pqaj22wP2[iT***zTixGBQQQQQWMHP5x2b#NQQMRwCFTT*!**zTFlojyaYxja]aaaaa22SSESkkhVOU2
                                 6P2qPPP9qa[iTzTTilqHM%QQQWWN#bwawKBQQQWBVo3fiTzzTTilojwqS]xja]aaEE2a2SSSSkkhVOU2
                                 j92Ppp499qa[iiiF32ABN%%%WNMMgRpP9HMQQ%QMR9xtfiiiifto2PPh6Eyjja22EE2aaSqSSkkhVOU2
                                 }G2PbKp999Pwx5lojV#MQ%QQWMMMM$HbbRMQQQWNBK9a5[[t[Zy2w9Vphq2axa2a2Eaa2EEE2Sk9GUUP
                                 {G29KmAp44pp4P9VKRMQ%%WWNMggMM#R#BWQQWWWNBHb9PqqqqP9VbAG4PS2yaayaE2EE22]2Sk9GUUV
                                 rO29KRmKbbAHR##B0MQQQQNNMggMNNMNNNQQWQQWWWMBDHHHHKKKH8HAVhkS]22aaaEE22SSSkhVGOUV
                                 .92qGRDmHHD$MNNWWQQQQNMg$D#0MMNNNWQQQQQQQQQQQNM0gB#DDD8AVhkS22EEaaESaEqkkkhVVUUP
                                  E224mD888D$MNWWWQ%QWMg$RmDB0MNWWWQQQQQ%QQQQ%%QWMMg$$D8AVhkkSSSE2222aEqk6kh9VUX2
                                  jqyPHRRRRRDgMNNNQWWMM$DmHR#0MNNWWWWQQQ%%%QQ%QQWNM0gg#8AG46kkqqS22ay2SqkkkhVVUXj
                                  {hyPHRRRRRR#Bg0MNNMM0$RHHmDgMMNNNWWQQQ%%%QQ%%QQWNMggDHAG49kkqS2a2ESqqkkkkhVGUXt
                                  ^92qARRRRRRD##$g000g$DmKKHR$g0MMNNWQQQ%%%%Q%%%QWWMg$DHAG496kqSEESkkkqk6669VOURc
                                   PPqGDDRRRRRDDD$#$$$DRHKAKmD$g0MMWWWQ%%Q%%%%%%QWWMg$DHbp496PPqSkPPkkqPd99VGUXK.
                                   jGhG8DDRRRmRRRDDDDRmHKAAAHR#gggMMWQQQQQ%%%%%QQWWN0#8AG499k6PqSkPhhPqPd9VVGUXP
                                   {UpGHDDDRmmmmmmRRmmHAbbbbKHR$gg0MWWNWQ%%%%%Q%QWWN0#HbV4966kkqqqkPhPPhd99VGUXn
                                   ^8AAH8DDRR888mmmHmHKAbGGbKHm#$gMMM00NW%%%%QQQQWNMgDHG4966kkqqqkkPhhhh6d9VGU8r
                                    V8XH88DRR8H8HHHHHKAGpppbKHR$$gg#HKHBNQ%%%QQQQWM0BDAG496kkqkkkPPPPhhP6dVpGOV
                                    j$8HH88DDRHHmHHHXAA4P9ppbKmDD$DK9P9RMQQQQQWWWNMg#8AV496kqqkPPhhPP9hPk69VGOn
                                    vg8XH8mD88HHHXXXXAbP22qP4pbAAKKV9VKBNQQQQWWWNMg$DHbV496kkqkPhddhhdhkk69VGO^
                                     K88HHH888HHHHXHXAA4q22qP94ppbAKKRBMNWWWQWNNM0g#mAp44966PkkkPhhddhPP6694V2
                                     n$88HHHH8HHHHHXXAAG449944pbAKRD#B0MNNNNNNMM0g$RKpV49666PPkkPPPh9PPP669hUv
                                     .K8XXAAAHHHHHHXAAAAAAbbAAHmRRD$B0g0MNMMMMM00$mKGpV4966PPPkkPkkPPkPhP6hPP
                                      n$8XAbbAXHXHXXAXXXXXXXXHHKHmRD#Bg0MMMMMM0B$RAVVpV496PPhh99PP6PkPhPh6hh{
                                       K8XAGVpGXHHHXXXXXAXXXXAKKKHmD#BB0M00MM0B#HAVVpVV496PPhdddhhd6Ph9P6hPP
                                       vRXAGVVpGAKHXXXAAAAAAXXXHRmDDD##B00000BDHAVddVV4996PhhdV9ddh6hh66kkhv
                                        jXXOppVpGGHHHHXAAAAAXXHH888DD##B$BggBDHAOddVVpV9d6hh9d99dhPhh6kkwP2
                                         6AAGp4hdVGAHXXAAAAAAXH88mRRDD#$$$##DHAOVVpppVV9dddh999PPhPkkkkwww^
                                          VAAGp99hdpGAAXAAAAXXXHHHmRRRDD#DRHHAAOVppVVVVddd69999PkPhkqkwwP}
                                          .VUAGV9PPhdpbAAAAAXXXHHHmmmmRRRRHHAAAGpppVdVVdd6PPPkPPPPPqkkwqY
                                           .VXAGVhhhhh9pGbAXKKHHHHHmHmmHHHHAAAbGGpOVVVVdddhPPPkPPkkkkwww^
                                            .VXGp44hhhqh94bbAAKHHHHHHHHAHAAAAAAGppOVddVdVdhPPkkkqqqkkwwv
                                              VAGp4p4hqS2qP9VGAAAAAAAGAAAAbbGAGppVVdddddd6hhPkkkSESSwq}
                                              .PbGppV9kaxyy2PdpGpGpGpGbbGGGGGGpVVVVdhhddhPhhPkkkS222w3
                                                2OGpV6qaxZZy2qqhdddppOGGGGGpppVVVVddhdhhhhhPkkkqS222o
                                                 jUGV9kEaxxxaaSqPhhdVVppGp4ppVVVddd6hhPPhPPkqqkS222Y
                                                  tOGVkqEEaaaaSSqPhh44444pp4VVddddh66PPkPkkqSE2222Y.
                                                   rPVh6kSEE]EEqqPP9999ddVVVVdddVd66PkPkSqqEE2aa2Y
                                                     nhh6kkqEEqqkkk66ddh6hhddddddPkkqqqSE22aaaxy3
                                                      vE46kkkkkkk6kkkkkPPkkPh66h6kqSSEE22ajjxxZ)
                                                        }Eh6kkkkkqqSqqSSSSSqqqSqSSEEE]aajjYYxtc
                                                          c3oZESS22]]E]aaaaa]]]]ajjjjjxYYolf)
                                                             ^>stoZjjjjxxxxYYYYjoooooool17>
                                                                  :>s7f111lllllllll1f7s:
                                                                        ^:::>>>>>>:^









#endif