# IMG2TIM-EX
An Image to PlayStation TIM File Converter.

Converts image files supported by FreeImage into a PlayStation TIM for PlayStation homebrew development. Uses the FreeImage library.
This fork add automatic conversion of 8-bit images into 4-bit if the conversor detects that only the first 16 colors are used.

## Features
* Properly preserves the original palette as is when converting 4-bit or 8-bit images.
* Automatically detects and converts an image to 4-bit when the palette only contains only 16 colors.
* Uses the same arguments used in bmp2tim with special additional arguments.
* Supports alpha channel (if available) with threshold control as transparency mask.
* Color-key and index-key transparency masking.
* Basic RGB to color-index image conversion.

## Download
Precompiled Binary: [img2tim-ex_(v0.8).zip](https://github.com/ArthCarvalho/img2tim/releases/download/v0.8/img2tim-ex_.v0.8.zip) (Win32)

## Changelog
**Version 0.8**
* Forked from the original.
* Added functions to convert 8-bit images into 4-bit automatically.

**Version 0.75**
* Fixed a bug where a false error message is thrown when converting 4-bit images with -bpp 4.
* Fixed a pixel order bug when converting images from either RGB or 4-bit depth to 4-bit color depth.

**Version 0.60**
* Initial release.