# image2ascii

im making this so i can turn teto into ascii art. and because i hate myself and im addicted to C im doing it in C. ALSO THIS BECAME WAY TOOO LONG SO EVERYTHING ENDED UP BEING SHOVED WITHIN ONE ACTUAL .C FILE I UNDERESTIMATED MY VISION AND HOW MUCH CODE IT WOULD TAKE TO IMPLEMENT IT

# Design

- Convenience  
For this program, I opted to take user input as opposed to taking CLI arguments because I believe that taking user input allows the user to search asyncronously for the information they need to provide to the program while they're using the program as opposed to needing everything beforehand in order to use CLI arguments. 

- Convenience Season 2  
I also wanted to do some cross-compatibility, so I decided to learn some MACROS!!! on how to compile for Windows using the UCRT64 toolchain and POSIX systems with Linux (i use arch btw) so this little program works on either or operating systems. Though, since Mac OS X is POSIX compliant, it should work, but I don't have an Apple device to test it on. 

- Convenience Season 3  
This program also has the ability for the user to choose the color of the background so their ASCII art won't look weird due to the contrast.

- Quality  
I also wanted to ensure QUALITY outputs everytime, so even though it's a little pedantic!!!, I created checks to ensure that the input image doesn't exceed the textual resolution of the console / TTY session. This ensures that the output will FIT the window, and not generate unformatted slop everywhere.

- Quality Season 2  
If I were using a program, I'd want quality. So instead of averaging RGB values to create a gray grayscale (the ascii art maps to a grayscaled version of the input image), I used colorimetric conversion to preserve the original input image's luminance. 

However, some image formats store pixel data in sRGB, a gamma encoded color space, rather than linear light. (ex. 50% Red != 50% brightness).  

To do colorimetric conversion on sRGB, I needed to do gamma expansion -> calculate the relative grayscale luminance value from the three (now linear) channels (R, G, B) -> perform gamma compression.  
To calculate the relative grayscale luminance, I used the weights provided by CIE/Rec.709. The equations (including the weights by CIE/Rec.709) that I used for gamma expansion, relative grayscale luminance calculation, and gamma compression are from this [wikipedia article](https://en.wikipedia.org/wiki/Grayscale#Colorimetric_(perceptual_luminance-preserving)_conversion_to_grayscale). 


Anyways, besides all this sRGB conversion stuff, if the image file just straight up has linear RGB values then I just do the relative grayscale luminance calculation and call it a day.  


# Project specifications
- Supports multiple file types  
JPEG, JPG, JPE, JFIF, BMP, TGA, PSD, PNG, HDR, PIC, PPM, PGM, PNM should all be supported. Please submit a bug report / pull request for any bugs.  

- If input image uses sRGB,  
The program creates a grayscale image thru gamma expansion -> computing relative linear luminance -> then gamma compresses it to luma. 
Maps luma to an ASCII gradient, creating the ASCII art.  

Luminance = True linear-light brightness  
Luma      = Brightness in sRGB  

- If input image uses linear RGB  
Calculates luminance and maps the luminance to an ASCII gradient, creating the ASCII art.

- Portable -- Works on Windows / POSIX-compliant operating systems  
Please submit a bug report / pull request if you find any bugs.  

- O(n^2) algorithm   
yes!  


# How to run

cmake it using UCRT64 on Windows or thug it out with gcc 
or better yet! download from the [release page](https://github.com/cnnacat/image2ascii/releases)!

the program'll walk you thru on what you need to provide it upon exeuction.


# File Structure

Generated using [my own tree script](https://github.com/cnnacat/win-tree) (yes im glazing it).  
Edited to remove anything part of .gitignore and non-source-code related stuff.
```bash
├── src
│   ├── script.c
│   └── script.h
└── stb_image
    ├── stb_image.c
    └── stb_image.h
```


# Acknowledgements

Thanks to everyone who contributed to [stb_image.h](https://github.com/nothings/stb/blob/master/stb_image.h)  

Thanks to MagiHotline on their version of turning an [image into ascii](https://github.com/MagiHotline/IMGtoASCII) since I read their code instead of the documentation for std_image.h.  
For the record, I did not copy the name!!! I was honestly surprised I was using a similar name and same tools as MagiHotline when their implementation came up from a Google search.

Thanks SO87 for making this [REALLY good track](https://open.spotify.com/track/20tiIBe8xyNqovriJi6nH2?si=88eeeb009bc142c9) to listen to while I was programming this mess of a program.  


![MOVE TETO](https://kasanetetoplush.com/wp-content/uploads/2025/02/Kasane-Teto-Plush-4.png)

