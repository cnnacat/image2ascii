# image2ascii

im making this so i can turn teto into ascii art. and because i hate myself and im addicted to C im doing it in C.  

ALSO THIS BECAME WAY TOOO LONG SO EVERYTHING ENDED UP BEING SHOVED WITHIN ONE ACTUAL .C FILE I UNDERESTIMATED MY VISION AND HOW MUCH CODE IT WOULD TAKE TO IMPLEMENT IT


# Project specifications
- Supports multiple file types (i think)
	- JPEG, JPG, JPE, JFIF
	- BMP
	- TGA
	- PSD
	- PNG
	- HDR
	- PIC
	- PPM, PGM


Please submit a bug report / pull request for any bugs.  


# How to run

cmake it using UCRT64 on Windows or thug it out with gcc 
or better yet! download from the [release page](https://github.com/cnnacat/image2ascii/releases)!

the program'll walk you thru on what you need to provide it upon exeuction.


# Design choices

### Easy to use

I wanted it to be as easy to use as possible so instead of forcing the user to learn how to interact with the program thru CLI arguments, I opted to just not use CLI arguments at all and take user input during run-time. 

### Cross-compatibility

Wanted it to work on Windows and POSIX-compliant systems. Because I use Arch Linux btw sometimes.

### Quality ASCII output

So the way ASCII art is produced is taking an image, grayscaling it, and then mapping it to an ASCII gradient. But usually, the quality of the ASCII output is determined by the quality of the grayscale. 

Instead of taking the easy approach of just averaging the RGB values, I did colorimetric conversion to make a cooler grayscale. More about it on the next section (nerd alert).



# Algorithm (boring stuff here nerd alert!!!)  

<pre>
Pre-requisite Definitions:  
Luminance   -> Percieved brightness of a color CALCULATED with linear RGB  
Luma        -> Percieved brightness of a color CALCULATED with sRGB  
LDR         -> Low Dynamic Range (typically stores R, G, B values from 0-255 for each color)  
HDR         -> High Dynamic Range (stores their RGB in multiple ways, 
                                   but definitely more than 1 byte per color)  
CIE/Rec.709 -> A standard for image encoding and other stuff.   
</pre>
----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

Instead of averaging RGB values to create a grayscale (the ascii art maps to a grayscaled version of the input image), I wanted to used colorimetric conversion to preserve the original input image's luma/luminance because it would create a better grayscale.   

If I wanted to do colorimetric conversion to grayscale, I would need the pixels to be in linear light.  

This is fairly easy if the image was a file format that used HDR, because they store RGB values in linear light. ALl I needed to do was use this linear luminance [formula](https://en.wikipedia.org/wiki/Grayscale#Colorimetric_(perceptual_luminance-preserving)_conversion_to_grayscale) whose weights was provided by [Rec.709](https://en.wikipedia.org/wiki/Rec._709) to make the pixel gray. 


However, some image formats (mainly file formats that use LDR) store pixel data in sRGB, a gamma encoded color space, rather than having their RGB values in linear light. (ex. 50% Red != 50% brightness).  

What this meant was that I needed to decode (expand) the gamma, perform the conversion to grayscale, and then recompress the gamma back.
The formula for gamma expansion and recompression (and the linear luminance formula to make a pixel gray) can be found on [this wikipedia page](https://en.wikipedia.org/wiki/Grayscale#Colorimetric_(perceptual_luminance-preserving)_conversion_to_grayscale).


----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

 ## For anyone reading this who also wants to use stb_image.h, do note.. 

stbi_load() and all derivatives of this function (ex. stbi_load_from_file()) **will effectively convert** all HDR images to LDR images by clamping the HDR image's pixels. Naturally, you can't convert it back to HDR.  

stbi_load() will also apply some kind of gamma conversion during the conversion from HDR to LDR. To disable this, you gotta use stbi_hdr_to_ldr_gamma(1.0f) and stbi_hdr_to_ldr_gamma(1.0f).  

<br>

stbi_loadf() and all derivatives of this function (ex. stbi_loadf_from_file()) was made for the sole purpose of loading HDR files, and is able to preserve HDR's dynamic range. 

Do note that instead of returning a range of integers from [0-255] for each color channel, stbi_loadf() will instead return a float from [0, 1] for each color channel.  

<br>

stbi_loadf() will **ALSO** convert all LDR files loaded with this function to HDR (kinda? it really just applies a gamma conversion to bring it from LDR to HDR). 

If you want to load HDR files, but don't want stbi_loadf() to convert LDR to HDR, just disable the gamma conversion with stbi_ldr_to_hdr_gamma(1.0f) and stbi_ldr_to_hdr_scale(1.0f).  


<br>

When stbi_loadf() does LDR to HDR promotion, it also does gamma expansion, bringing the pixels from the sRGB color space to linear color space. You don't need to manually bring any LDR files using the sRGB color space to linear color space. **BUT the gamma expansion that stbi_loadf() uses isn't true gamma expansion**, but rather an estimate of the formula. 

If you want true gamma expansion, you must disable LDR to HDR promotion, then implement the gamma conversion yourself. If this is your intent, I recommend using the provided formulas from the wikipedia page which is hyperlinked above.  

<br>


**If anyone is confused about the technicalities of color space and all that stuff, it's OK to just abstract it to "The values of R, G, B (and A) are just numbers. sRGB values are non-linear, while linear RGB values are linear".**

**Ignore all this LDR and HDR stuff. If your operation requires the pixels to be in the linear colorspace, and it is in the linear colorspace, just operate on it. Else, if it's in the sRGB color space, convert it to the linear color space using gamma expansion.**


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


Thanks to MagiHotline on their version of turning an [image into ascii](https://github.com/MagiHotline/IMGtoASCII) I used their implementation to get a foothold when I was just beginning to learn stb_image.h.   

For the record, I did not copy the name!!! I was honestly surprised I was using a similar name and same tools as MagiHotline when their implementation came up from a Google search.


Thanks SO87 for making this [REALLY good track](https://open.spotify.com/track/20tiIBe8xyNqovriJi6nH2?si=88eeeb009bc142c9) to listen to while I was programming this mess of a program.  


![MOVE TETO](https://kasanetetoplush.com/wp-content/uploads/2025/02/Kasane-Teto-Plush-4.png)

