#ifndef _BMPLIB_H
#define _BMPLIB_H

#define TRUE  1

#define FALSE 0

typedef unsigned int  uint;
typedef unsigned char uchar;
typedef unsigned short ushort;
typedef unsigned long  ulong;

typedef struct tagRGBQUAD {
    uchar    rgbBlue;
    uchar    rgbGreen;
    uchar    rgbRed;
    uchar    rgbReserved;
} RGBQUAD;

#define BI_RGB        0L
#define BI_RLE8       1L
#define BI_RLE4       2L
#define BI_BITFIELDS  3L

typedef struct tagBITMAPFILEHEADER {
    ushort    bfType;
    ulong   bfSize;
    ushort    bfReserved1;
    ushort    bfReserved2;
    ulong   bfOffBits;
} BITMAPFILEHEADER;

typedef struct tagBITMAPINFOHEADER{
    ulong      biSize;
    long       biWidth;
    long       biHeight;
    ushort       biPlanes;
    ushort       biBitCount;
    ulong      biCompression;
    ulong      biSizeImage;
    long       biXPelsPerMeter;
    long       biYPelsPerMeter;
    ulong      biClrUsed;
    ulong      biClrImportant;
} BITMAPINFOHEADER;

typedef uchar * DIB;

extern DIB bmp_read_dib_file( FILE *fp );
extern DIB bmp_read_dib_filename( char *filename );
extern int bmp_save_dib_file( FILE *fp, DIB dib );
extern int bmp_save_dib_filename( char *filename, DIB dib );
extern DIB bmp_expand_dib_rle( DIB dib );
extern DIB bmp_compress_dib_rle( DIB dib );

#endif
