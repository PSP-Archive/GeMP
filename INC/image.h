#ifndef IMAGE_H
#define IMAGE_H

extern char tmpBuf[];

struct jpeg_decompress_struct jpginfo;

int jpg_skip_x, jpg_skip_y;
int jpg_shrink;

void user_warning_fn(png_structp png_ptr, png_const_charp warning_msg);

int loadTGA(char *filename, int w, int h, unsigned short *imgData, char *imgAlphaData, int useAlpha);
int loadImageTGA(char *filename, int w, int h, unsigned short *imgData, char *imgAlphaData);
int loadImageNoAlphaTGA(char *filename, int w, int h, unsigned short *imgData);

Image *LoadGFX(const char* filename);

int write_png(char *path, const byte *in, unsigned int width, unsigned int height);
int read_png(char *path, unsigned short *out, size_t outlen);
int read_bitmap(char *path, unsigned short *out, size_t outlen);

int LoadJPEG(char *path, unsigned short *out);

int PackBits_encode( byte *in, byte *out, int size );
int PackBits_decode( byte *in, byte *out, int size );

#endif

