#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include <unistd.h>
#include <windows.h>
#include <freeimage.h>

#include "tim.h"

#define VERSION		"0.8"

namespace param {

	char	InputFile[MAX_PATH] = { 0 };
	char	OutputFile[MAX_PATH] = { 0 };

    int	BlackTrans		= false;	// Make black pixels transparent (STP bit off)
    int ColorTrans		= false;	// Make non-black pixels translucent (STP bit on)
	int UseAlphaMask	= false;	// Use alpha channel as transparency mask
	int AlphaThresh		= 127;		// Alpha mask threshold

	int TransColR		= -1;
	int TransColG		= -1;
	int TransColB		= -1;

	int TransCol		= -1;
	int OutputBpp		= -1;

	int Quantize        = false; // Quantize palette

	tim::PARAM	tim = { 0 };

}


typedef struct {

	int		fmt;		// 0 - 32-bit RGBA, 1 - 8-bit palletized, 2 - 4-bit palletized
    short	w,h;
    void	*pixels;

    short	numCols;
    void	*colors;

    int     sourceBpp;
    bool    from8to4;

} IMGPARAM;


void ConvertImageToTim(IMGPARAM image, tim::PARAM* tim);
int SimpleQuantize(tim::PARAM* tim, int bitDepth);


int LoadImagePixels(const char* fileName, IMGPARAM* image, bool makeRGBA, int bppMode);
void FreeImageStruct(IMGPARAM* image);


int main(int argc, char *argv[]) {

    FreeImage_Initialise(false);

    printf("IMG2TIM-EX v%s - Converts image files into PlayStation TIM files\n", VERSION);
    printf("2020 Arthur C. S. Lima, 2016 Meido-Tek Productions (Lameguy64/TheCodingBrony)\n");
    printf("Powered by FreeImage v%s\n\n", FreeImage_GetVersion());

	// Print banner if no arguments were passed
	if (argc <= 1) {

		printf("Syntax:\n");
		printf("   img2tim [options] [-o <outName>] <inFile>\n\n");
		printf("Options:\n");
		printf("  -b            - Sets semi-transparent bit on fully black pixels\n");
		printf("                  (ignored when -usealpha is specified).\n");
		printf("  -t            - Set semi-transparent bit on non fully black pixels.\n");
		printf("  -org <x y>    - Specifies the VRAM offset of the image (Default: 640 0).\n");
		printf("  -plt <x y>    - Specifies the VRAM offset of the CLUT (Default: 0 480).\n");
		printf("  -o <outFile>  - Sets the name of the output file (Default: <inFile>.tim).\n");
		printf("\nExtra options:\n");
		printf("  -usealpha     - Use alpha channel (if available) as transparency mask.\n");
        printf("  -alpt <value> - Threshold value when alpha channel is used as transparency\n");
        printf("                  mask (Default: 127).\n");
        printf("  -tindex <col> - Specify color index to be treated as transparent (ignored on\n");
        printf("                  non palletized images).\n");
		printf("  -tcol <r g b> - Specify RGB color value to be treated as transparent.\n");
		printf("  -bpp <bpp>    - Specify color depth for the output TIM file\n");
		printf("                  (Default: Color depth of source image, 24 is never default).\n");
        printf("  -quantize     - Enable Palette quantization.\n");
        printf("                  (Enabling this will modify the original palette,\n"
               "                   not recommended for pixel art)\n");
		/*
        printf("   -tc <r g b>   - Specify color to be treated as transparent.\n");
        printf("   -bc <r g b>   - Specify blending color when converting alpha-blended pixels.\n");
		printf("   -bg <r g b>   - Specifies a background color for images with an alpha channel.\n");
		printf("   -tc <r g b>   - Sepecifies the color to be treated as transparent.\n");
		printf("   -qm <method>  - Sets the quantization method used for generating a CLUT when\n");
		printf("                   converting true-color images to 4-bit/8-bit.\n");
		printf("                    0 - Simple 'color search' quantization (default)\n");
		printf("                    1 - Median-cut quantization\n");
		printf("                    2 - Median-cut quantization with dithering\n");
		*/

		printf("\nFor more info, please read img2tim.txt.\n");
		return(0);

	}


	param::tim.imgXoffs		= 640;
	param::tim.imgYoffs		= 0;
	param::tim.clutXoffs	= 0;
	param::tim.clutYoffs	= 480;


	// Parse arguments
	for(int i=1; i<argc; i++) {

        char* arg = strdup(argv[i]);
        for(int c=0; arg[c] != 0x00; c++)
			arg[c] = tolower(arg[c]);


        if (strcmp(arg, "-b") == 0) {

            param::BlackTrans = true;

        } else if (strcmp(arg, "-t") == 0) {

        	param::ColorTrans = true;

		} else if (strcmp(arg, "-o") == 0) {

            strcpy(param::OutputFile, argv[i+1]);
            i++;

		} else if (strcmp(arg, "-org") == 0) {

            param::tim.imgXoffs	= atoi(argv[i+1]);
            param::tim.imgYoffs	= atoi(argv[i+2]);
            i += 2;

		} else if (strcmp(arg, "-plt") == 0) {

            param::tim.clutXoffs	= atoi(argv[i+1]);
            param::tim.clutYoffs	= atoi(argv[i+2]);
            i += 2;

		} else if (strcmp(arg, "-usealpha") == 0) {

            param::UseAlphaMask = true;

		} else if (strcmp(arg, "-alpt") == 0) {

            param::AlphaThresh = atoi(argv[i+1]);
			i++;

		} else if (strcmp(arg, "-tcol") == 0) {

			param::TransColR = atoi(argv[i+1]);
			i++;
			param::TransColG = atoi(argv[i+1]);
			i++;
			param::TransColB = atoi(argv[i+1]);
			i++;

		} else if (strcmp(arg, "-tindex") == 0) {

			param::TransCol = atoi(argv[i+1]);
			i++;

		} else if (strcmp(arg, "-bpp") == 0) {

            param::OutputBpp = atoi(argv[i+1]);

            if (!((param::OutputBpp == 4) || (param::OutputBpp == 8) || (param::OutputBpp == 16) || (param::OutputBpp == 24))) {
                printf("Invalid color depth value specified.\n");
                exit(1);
            }

			i++;

        } else if (strcmp(arg, "-quantize") == 0) { // Make RGBA and quantize palette
            param::Quantize = true;
            i++;
        } else if (arg[0] == '-') {

            printf("Unknown parameter: %s\n", argv[i]);
            free(arg);
            exit(1);

        } else {

            strcpy(param::InputFile, argv[i]);

        }


        free(arg);

	}


	// Check if an input file name is present
	if (strlen(param::InputFile) == 0) {
		printf("No input file specified.\n");
		exit(1);
	}


	// Check and generate an output filename when necessary
    if (strlen(param::OutputFile) == 0) {

        if (strrchr(param::InputFile, '.') == NULL) {

            printf("Cannot generate output filename, input has no extension.\n");
            exit(1);

        }

		strcpy(param::OutputFile, param::InputFile);
        strcpy(strrchr(param::OutputFile, '.'), ".tim");

    }

	printf("Input File  : %s\n", param::InputFile);
	printf("Output File : %s\n", param::OutputFile);


	IMGPARAM	image;

	printf("Output Bpp  : ");
	if (param::OutputBpp == -1) {

		if (!LoadImagePixels(param::InputFile, &image, false, param::OutputBpp)) {
			return(EXIT_FAILURE);
		}

		printf("Input Bpp (");
		if (image.fmt == 0)
			printf("24");
		else if (image.fmt == 1)
			printf("8");
		else if (image.fmt == 2)
			printf("4");
		printf(")\n\n");

        if (param::Quantize == true) {
          if (image.fmt != 1 || image.fmt != 2) {
            printf("Warning: Quantizing a paletted image will destroy the original palette.\n\n");
          }
        }

		ConvertImageToTim(image, &param::tim);

	} else {

		if (param::Quantize == true) {
          if (image.fmt != 1 || image.fmt != 2) {
            printf("Warning: Quantizing a paletted image will destroy the original palette.\n\n");
          }
        }

		if (!LoadImagePixels(param::InputFile, &image, param::Quantize, param::OutputBpp)) {
			return(EXIT_FAILURE);
		}

		ConvertImageToTim(image, &param::tim);

		if (param::OutputBpp <= 8) {

            if(param::Quantize){
                if (!SimpleQuantize(&param::tim, param::OutputBpp)) {

                    tim::FreeParam(&param::tim);
                    FreeImageStruct(&image);

                    exit(1);

                }
            }

            tim::FreeParam(&param::tim);
            FreeImageStruct(&image);

            exit(1);

		}

	}


	tim::ExportFile(param::OutputFile, &param::tim);


	tim::FreeParam(&param::tim);
	FreeImageStruct(&image);

	printf("Converted successfully...\n\n");

	return(0);

}


void ConvertImageToTim(IMGPARAM image, tim::PARAM* tim) {

	if (image.fmt == 0) {

		if (param::OutputBpp == 24) {	// Convert image from 32-bit RGBA to 24-bit RGB

			tim->imgData = malloc(3*(image.w*image.h));

			for(short py=0; py<image.h; py++) {
				for(short px=0; px<image.w; px++) {

					u_char r = ((u_char*)image.pixels)[4*(px+(image.w*py))];
					u_char g = ((u_char*)image.pixels)[(4*(px+(image.w*py)))+1];
					u_char b = ((u_char*)image.pixels)[(4*(px+(image.w*py)))+2];

					((tim::PIX_RGB24*)tim->imgData)[px+(image.w*py)].r = r;
					((tim::PIX_RGB24*)tim->imgData)[px+(image.w*py)].g = g;
					((tim::PIX_RGB24*)tim->imgData)[px+(image.w*py)].b = b;

				}

			}

			tim->format		= 3;
			tim->imgWidth	= image.w;
			tim->imgHeight	= image.h;

		} else {	// Convert image from 32-bit RGBA to 16-bit RGBi

			tim->imgData = malloc(2*(image.w*image.h));

			for(short py=0; py<image.h; py++) {
				for(short px=0; px<image.w; px++) {

					u_char r = ((u_char*)image.pixels)[4*(px+(image.w*py))];
					u_char g = ((u_char*)image.pixels)[(4*(px+(image.w*py)))+1];
					u_char b = ((u_char*)image.pixels)[(4*(px+(image.w*py)))+2];
					u_char a = ((u_char*)image.pixels)[(4*(px+(image.w*py)))+3];

					((tim::PIX_RGB5*)tim->imgData)[px+(image.w*py)].r = r/8;
					((tim::PIX_RGB5*)tim->imgData)[px+(image.w*py)].g = g/8;
					((tim::PIX_RGB5*)tim->imgData)[px+(image.w*py)].b = b/8;

					if (param::UseAlphaMask == false) {

						// For fully-black pixels
						if ((r == 0) && (g == 0) && (b == 0)) {

							if (param::BlackTrans)
								((tim::PIX_RGB5*)tim->imgData)[px+(image.w*py)].i = 1;
							else
								((tim::PIX_RGB5*)tim->imgData)[px+(image.w*py)].i = 0;

						// For non-fully black pixels
						} else {

							if (param::ColorTrans)
								((tim::PIX_RGB5*)tim->imgData)[px+(image.w*py)].i = 1;
							else
								((tim::PIX_RGB5*)tim->imgData)[px+(image.w*py)].i = 0;

						}

					} else {

						if ((r == 0) && (g == 0) && (b == 0)) {

							if (a >= param::AlphaThresh)
								((tim::PIX_RGB5*)tim->imgData)[px+(image.w*py)].i = 1;
							else
								((tim::PIX_RGB5*)tim->imgData)[px+(image.w*py)].i = 0;

						} else {

							if (a < param::AlphaThresh) {

								((tim::PIX_RGB5*)tim->imgData)[px+(image.w*py)].r = 0;
								((tim::PIX_RGB5*)tim->imgData)[px+(image.w*py)].g = 0;
								((tim::PIX_RGB5*)tim->imgData)[px+(image.w*py)].b = 0;
								((tim::PIX_RGB5*)tim->imgData)[px+(image.w*py)].i = 0;

							} else {

								if (param::ColorTrans)
									((tim::PIX_RGB5*)tim->imgData)[px+(image.w*py)].i = 1;
								else
									((tim::PIX_RGB5*)tim->imgData)[px+(image.w*py)].i = 0;

							}

						}

					}

					if ((param::TransColR == r) && (param::TransColG == g) && (param::TransColB == b)) {

							((tim::PIX_RGB5*)tim->imgData)[px+(image.w*py)].r = 0;
							((tim::PIX_RGB5*)tim->imgData)[px+(image.w*py)].g = 0;
							((tim::PIX_RGB5*)tim->imgData)[px+(image.w*py)].b = 0;
							((tim::PIX_RGB5*)tim->imgData)[px+(image.w*py)].i = 0;

					}

				}

			}

			tim->format		 = 2;
			tim->imgWidth	 = image.w;
			tim->imgHeight	 = image.h;

		}

	} else if (image.fmt == 1 && !image.from8to4) {	// Convert 8-bit palletized

		if ((image.w%2) != 0)
			printf("WARNING: Image width is not a multiple of 2, output may be broken.\n");

		tim->clutData	= malloc(2*256);
        tim->clutWidth	= 256;
        tim->clutHeight	= 1;

        for(short c=0; c<256; c++) {

			u_char r = ((RGBQUAD*)image.colors)[c].rgbRed;
			u_char g = ((RGBQUAD*)image.colors)[c].rgbGreen;
			u_char b = ((RGBQUAD*)image.colors)[c].rgbBlue;

			if (param::TransCol == -1) {
				if ((param::TransColR == r) && (param::TransColG == g) && (param::TransColB == b)) {
                    param::TransCol = c;
				}
			}

			if (c != param::TransCol) {

				((tim::PIX_RGB5*)tim->clutData)[c].r	= r/8;
				((tim::PIX_RGB5*)tim->clutData)[c].g	= g/8;
				((tim::PIX_RGB5*)tim->clutData)[c].b	= b/8;

				// For fully-black pixels
				if ((r == 0) && (g == 0) && (b == 0)) {

					if (param::BlackTrans)
						((tim::PIX_RGB5*)tim->clutData)[c].i = 1;
					else
						((tim::PIX_RGB5*)tim->clutData)[c].i = 0;

				// For non-fully black pixels
				} else {

					if (param::ColorTrans)
						((tim::PIX_RGB5*)tim->clutData)[c].i = 1;
					else
						((tim::PIX_RGB5*)tim->clutData)[c].i = 0;

				}

			} else {

				((tim::PIX_RGB5*)tim->clutData)[c].r	= 0;
				((tim::PIX_RGB5*)tim->clutData)[c].g	= 0;
				((tim::PIX_RGB5*)tim->clutData)[c].b	= 0;
				((tim::PIX_RGB5*)tim->clutData)[c].i	= 0;

			}

        }

		tim->imgData = malloc(image.w*image.h);

		for(short py=0; py<image.h; py++) {

			memcpy(&((u_char*)tim->imgData)[image.w*py], &((u_char*)image.pixels)[image.w*py], image.w);

		}

		tim->format		= 1;
		tim->imgWidth	= image.w;
		tim->imgHeight	= image.h;

	} else if (image.fmt == 2 || image.from8to4) {	// Convert image for 4-bit TIM

		if ((image.w%4) != 0)
			printf("WARNING: Image width is not a multiple of 4, output may be broken.\n");

		tim->clutData	= malloc(2*16);
        tim->clutWidth	= 16;
        tim->clutHeight	= 1;

        for(short c=0; c<16; c++) {

			u_char r = ((RGBQUAD*)image.colors)[c].rgbRed;
			u_char g = ((RGBQUAD*)image.colors)[c].rgbGreen;
			u_char b = ((RGBQUAD*)image.colors)[c].rgbBlue;

			if (param::TransCol == -1) {
				if ((param::TransColR == r) && (param::TransColG == g) && (param::TransColB == b)) {
                    param::TransCol = c;
				}
			}

			if (c != param::TransCol) {

				((tim::PIX_RGB5*)tim->clutData)[c].r	= r/8;
				((tim::PIX_RGB5*)tim->clutData)[c].g	= g/8;
				((tim::PIX_RGB5*)tim->clutData)[c].b	= b/8;

				// For fully-black pixels
				if ((r == 0) && (g == 0) && (b == 0)) {

					if (param::BlackTrans)
						((tim::PIX_RGB5*)tim->clutData)[c].i = 1;
					else
						((tim::PIX_RGB5*)tim->clutData)[c].i = 0;

				// For non-fully black pixels
				} else {

					if (param::ColorTrans)
						((tim::PIX_RGB5*)tim->clutData)[c].i = 1;
					else
						((tim::PIX_RGB5*)tim->clutData)[c].i = 0;

				}

			} else {

				((tim::PIX_RGB5*)tim->clutData)[c].r	= 0;
				((tim::PIX_RGB5*)tim->clutData)[c].g	= 0;
				((tim::PIX_RGB5*)tim->clutData)[c].b	= 0;
				((tim::PIX_RGB5*)tim->clutData)[c].i	= 0;

			}

        }

		tim->imgData = malloc((image.w/2)*image.h);

		if(!image.from8to4){
            for(short py=0; py<image.h; py++) {

                for(short px=0; px<image.w/2; px++) {

                    u_char pix = ((u_char*)image.pixels)[px+((image.w/2)*py)];
                    ((u_char*)tim->imgData)[px+((image.w/2)*py)] = ((pix&0xf)<<4)|((pix>>4)&0xf);

                }

            }
		} else {
		    for(short py=0; py<image.h; py++) {
                for(short px=0; px<image.w/2; px++) {
                    u_char pixA = ((u_char*)image.pixels)[(px*2)+(image.w)*py];
                    u_char pixB = ((u_char*)image.pixels)[(px*2)+1+(image.w)*py];
                    ((u_char*)tim->imgData)[px+((image.w/2)*py)] = ((pixB&0xf)<<4)|((pixA)&0xf);
                }
		    }

		}

		tim->format		= 0;
		tim->imgWidth	= image.w;
		tim->imgHeight	= image.h;

	}


}


int SimpleQuantize(tim::PARAM* tim, int bitDepth) {

    u_short			colTable[256] = { 0 };
	int				diffColors = 0;
	bool			newCol = true;

	if (bitDepth == 8) {

		if ((tim->imgWidth%2) != 0)
			printf("WARNING: Image width is not a multiple of 2, output may be broken.\n");

	} else {

		if ((tim->imgWidth%4) != 0)
			printf("WARNING: Image width is not a multiple of 4, output may be broken.\n");

	}

    for(int py=0; py<tim->imgHeight; py++) {
		for(int px=0; px<tim->imgWidth; px++) {

			u_short	pix = ((u_short*)tim->imgData)[px+(tim->imgWidth*py)];

			for(short c=0; c<diffColors; c++) {

                if (colTable[c] == pix) {

					newCol = false;
					break;

                } else {

                	newCol = true;

                }

			}

			if (newCol) {

				if (bitDepth == 8) {

					if (diffColors >= 255) {
						printf("ERROR: More than 256 colors found in image.\n");
						return(0);
					}

				} else {

					if (diffColors >= 16) {
						printf("ERROR: More than 16 colors found in image.\n");
						return(0);
					}

				}

				colTable[diffColors] = pix;
                diffColors++;

				newCol = false;

			}

		}
    }

    u_short* srcImage = (u_short*)tim->imgData;

    if (bitDepth == 8) {

		tim->imgData = malloc(tim->imgWidth*tim->imgHeight);

		for(int py=0; py<tim->imgHeight; py++) {
			for(int px=0; px<tim->imgWidth; px++) {

				u_short	pix = srcImage[px+(tim->imgWidth*py)];
				short index;

				for(index=0; index<diffColors; index++) {

					if (pix == colTable[index])
						break;

				}

				if (index < diffColors) {
					((u_char*)tim->imgData)[px+(tim->imgWidth*py)] = index;
				}

			}
		}

    } else {

    	tim->imgData = malloc((tim->imgWidth/2)*tim->imgHeight);


		for(int py=0; py<tim->imgHeight; py++) {
			for(int px=0; px<tim->imgWidth; px++) {

				u_short	pix = srcImage[px+(tim->imgWidth*py)];
				short index;

				for(index=0; index<diffColors; index++) {

					if (pix == colTable[index])
						break;

				}

				if (index < diffColors) {

					if ((px%2) == 0) {
						((u_char*)tim->imgData)[(px/2)+((tim->imgWidth/2)*py)] = index;
					} else {
						((u_char*)tim->imgData)[(px/2)+((tim->imgWidth/2)*py)] |= index<<4;
					}


				}

			}
		}

    }

	free(srcImage);


	if (bitDepth == 8) {
		tim->clutData	= malloc(2*256);
		memcpy(tim->clutData, colTable, 2*256);
		tim->clutWidth	= 256;
	} else {
		tim->clutData	= malloc(2*16);
		memcpy(tim->clutData, colTable, 2*16);
		tim->clutWidth	= 16;
	}

    tim->clutHeight	= 1;

	if (bitDepth == 8) {
		tim->format = 1;
	} else {
		tim->format = 0;
	}

    return(1);

}

int CompareRGBQUAD(RGBQUAD * a, RGBQUAD * b) {
    return (a->rgbRed == b->rgbRed && a->rgbGreen == b->rgbGreen && a->rgbBlue == b->rgbBlue);
}

int LoadImagePixels(const char* fileName, IMGPARAM* image, bool makeRGBA, int bppMode) {


	// Check if file exists
	if (access(fileName, F_OK) == -1) {

		printf("ERROR: File not found.\n");
		return(0);

	}


	// Determine format of input file
	FREE_IMAGE_FORMAT fif = FreeImage_GetFileType(fileName, 0);

	if (fif == FIF_UNKNOWN) {

		fif = FreeImage_GetFIFFromFilename(fileName);

		if (!FreeImage_FIFSupportsReading(fif)) {

			printf("ERROR: Unknown/unsupported image format.\n");
			return(0);

		}

	}


	// Load the input image
	FIBITMAP *srcImage = FreeImage_Load(fif, fileName, 0);

	if (srcImage == NULL) {

		printf("ERROR: Cannot load specified image file.\n");
		return(0);

	}

	// Some checks to make sure that the image is really valid
	if (!FreeImage_HasPixels(srcImage)) {

		printf("ERROR: Source image has no pixel data... Somehow.\n");
		return(0);

	}


	if ((makeRGBA) || (FreeImage_GetBPP(srcImage) == 24) || ((FreeImage_GetBPP(srcImage) > 32))) {

		// Convert to RGBA32 for 16-bit and 24-bit exports
		FIBITMAP *tempImage;

		// Just to make things simpler, convert pixels to RGB32 when it is not palletized
		tempImage = FreeImage_ConvertTo32Bits(srcImage);

		if (tempImage == NULL) {

			printf("ERROR: Could not convert image to RGBA32 for conversion.\n");
			FreeImage_Unload(srcImage);
			return(0);

		}

		FreeImage_Unload(srcImage);
		srcImage = tempImage;

	}

	// Convert 8-bit to 4-bit flag
	// Set it to false beforehand
	image->from8to4 = false;


	switch(FreeImage_GetBPP(srcImage)) {
	case 32:	// 32-bit images

		image->fmt	= 0;
		image->w	= FreeImage_GetWidth(srcImage);
		image->h	= FreeImage_GetHeight(srcImage);

		image->numCols	= 0;
		image->colors	= NULL;
		image->pixels	= malloc(4*(image->w*image->h));

		image->sourceBpp = 32;

        for(short py=0; py<image->h; py++) {

			void* pixels = FreeImage_GetScanLine(srcImage, (image->h-py)-1);
			memcpy(&((u_int*)image->pixels)[image->w*py], pixels, 4*image->w);

			for(short p=0; p<image->w; p++) {

				u_char* pix = (u_char*)&((u_int*)image->pixels)[ p+(image->w*py) ];
				u_char	t;

				t = pix[0];
				pix[0] = pix[2];
				pix[2] = t;

			}

        }

		break;

	case 8:		// 8-bit palletized
		image->fmt	= 1;
		image->w	= FreeImage_GetWidth(srcImage);
		image->h	= FreeImage_GetHeight(srcImage);

		image->numCols	= 256;
		image->colors	= malloc(4*256);

		memcpy(image->colors, FreeImage_GetPalette(srcImage), 4*256);

		image->sourceBpp = 8;

		if (bppMode != 8) {
            // Some file formats are always 8-bit even when
            // the artist meant them to be 4-bit, so auto-detect if
            // colors above the 16th color are: all the same,
            // if they are, this means that not only the rest of the palette
            // is unused.

            // Analyze palette colors
            // Check if all colors are the same
            RGBQUAD * palcolors = (RGBQUAD *)image->colors;
            bool paletteIsGrayScale = true;
            bool upperPalGrayScale = true;
            bool upperPalSameColor = true;

            // Check if this image contains only grayscale values, if so, export as 8 bit anyway.
            // Some image editors apparently do this to unused colors (i.e.: Aseprite)
            for(int i = 0; i < 256; i++) {
                if(palcolors[i].rgbRed != i) {
                    paletteIsGrayScale = false;
                    if(i >= 16){
                        upperPalGrayScale = false;
                    }
                }
            }

            if(!paletteIsGrayScale) {
                // Check if the colors above 16 are the same color
                RGBQUAD lastColor = palcolors[16];
                for(int i = 16; i < 256; i++) {
                    bool colorEqual = CompareRGBQUAD(&lastColor, &palcolors[i]);
                    lastColor = palcolors[i];
                    if(!colorEqual){
                        upperPalSameColor = false;
                    }
                }
            }

            if(!paletteIsGrayScale && (upperPalGrayScale || upperPalSameColor)) {
                image->from8to4 = true;
            }

            // Emit a warning to the user that the palette is going to clip.
            if(image->from8to4) {
                printf("Input Bpp (8) >> Output Bpp (4) (Automatic Coneversion)\n\nPalette appears to contain only 16 colors,"
                       "the image will be converted as 4bpp.\nIf this is not what you want, use -bpp 8 to force 8-bit mode. ");
            }
		}

		image->pixels	= malloc(image->w*image->h);

		for(short py=0; py<image->h; py++) {

			void* pixels = FreeImage_GetScanLine(srcImage, (image->h-py)-1);
			memcpy(&((u_char*)image->pixels)[image->w*py], pixels, image->w);

        }

        break;

	case 4:		// 4-bit palletized

		image->fmt	= 2;
		image->w	= FreeImage_GetWidth(srcImage);
		image->h	= FreeImage_GetHeight(srcImage);

		image->numCols	= 16;
		image->colors	= malloc(4*16);
		memcpy(image->colors, FreeImage_GetPalette(srcImage), 4*16);

		image->pixels	= malloc((image->w/2)*image->h);

		image->sourceBpp = 4;

		for(short py=0; py<image->h; py++) {

			void* pixels = FreeImage_GetScanLine(srcImage, (image->h-py)-1);
			memcpy(&((u_char*)image->pixels)[(image->w/2)*py], pixels, image->w/2);

        }

        break;

	default:

		printf("ERROR: Unsupported color depth: %dbpp", FreeImage_GetBPP(srcImage));
		FreeImage_Unload(srcImage);
		return(0);

	}


	// Unload source image
	FreeImage_Unload(srcImage);

	return(1);

}

void FreeImageStruct(IMGPARAM* image) {

    if (image->pixels != NULL) {
		free(image->pixels);
		image->pixels = NULL;
    }

	if (image->colors != NULL) {
		free(image->colors);
		image->colors = NULL;
	}

}
