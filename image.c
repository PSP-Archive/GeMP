#include <stdio.h>
#include <stdlib.h>
#include "INC/main.h"

#define TMPBUFSIZE 522284

void user_warning_fn(png_structp png_ptr, png_const_charp warning_msg)
{
}

/* can be used for anything, big enough to load a large tga */
char tmpBuf[TMPBUFSIZE];

/* load a targa image file */

/*****************************************************************************/
/* Image must be RGB, 24-bit, uncompressed, 480x272 TGA file.                */
/*                                                                           */
/* No alpha info - alpha processing currently removed.                       */
/*****************************************************************************/
int loadTGA(char *filename, int w, int h, unsigned short *imgData, char *imgAlphaData, int useAlpha)
{
  int i,j,k,pos;
  int loffset;

  int fd=sceIoOpen(filename,SCE_O_RDONLY, 644);
  if (fd<0) return 0; 
  sceIoRead(fd,tmpBuf,TMPBUFSIZE);
  sceIoClose(fd);

  loffset = tmpBuf[0] + 18;

  for (i=0,k=0; i < h; i++)
    for (j=0; j < w; j++, k++)
      {
      	pos = ((h - i-1)*w+j)*3 + loffset;
      	imgData[k] = RGB(tmpBuf[pos+2], tmpBuf[pos+1], tmpBuf[pos]);
      }
 	return 1;
}
int loadImageTGA(char *filename, int w, int h, unsigned short *imgData, char *imgAlphaData)
{
  return loadTGA(filename,w,h,imgData,imgAlphaData,1);
}

int loadImageNoAlphaTGA(char *filename, int w, int h, unsigned short *imgData)
{
  return loadTGA(filename,w,h,imgData,0,0);
}


// Next Power (Math)
int getNextPower2(int width)
{
	int b = width;
	int n;
	for (n = 0; b != 0; n++) b >>= 1;
	b = 1 << n;
	if (b == 2 * width) b >>= 1;
	return b;
}

// Load a Graphic into Memory
Image *LoadGFX(const char* filename)
{
	png_structp png_ptr;
	png_infop info_ptr;
	unsigned int sig_read = 0;
	png_uint_32 width, height;
	int bit_depth, color_type, interlace_type, x, y;
	u32* line;
	FILE *fp;

	Image* image = (Image*) malloc(sizeof(Image));
	memset(image, 0, sizeof(Image));
	
	if (!image) return NULL;
		
	if ((fp = fopen(filename, "rb")) == NULL){
		fclose(fp);
		return NULL;
	}
	
	png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	if (png_ptr == NULL) {
		free(image);
		fclose(fp);
		return NULL;;
	}
	
	png_set_error_fn(png_ptr, (png_voidp) NULL, (png_error_ptr) NULL, user_warning_fn);
	
	info_ptr = png_create_info_struct(png_ptr);
	if (info_ptr == NULL) {
		free(image);
		fclose(fp);
		png_destroy_read_struct(&png_ptr, png_infopp_NULL, png_infopp_NULL);
		return NULL;
	}
	png_init_io(png_ptr, fp);	
	png_set_sig_bytes(png_ptr, sig_read);
	png_read_info(png_ptr, info_ptr);
	png_get_IHDR(png_ptr, info_ptr, &width, &height, &bit_depth, &color_type, &interlace_type, int_p_NULL, int_p_NULL);
	image->realimageWidth = width;
	image->realimageHeight = height;
	if(image->imageWidth>479 || image->imageHeight >271){
		image->imageWidth = 480;
		image->imageHeight = 272;
		image->textureWidth = getNextPower2(480);
		image->textureHeight = getNextPower2(272);
	}else{
		image->imageWidth = width;
		image->imageHeight = height;
		image->textureWidth = getNextPower2(width);
		image->textureHeight = getNextPower2(height);
	}
	/*png_set_strip_16(png_ptr);
	png_set_packing(png_ptr);
	if (color_type == PNG_COLOR_TYPE_PALETTE) png_set_palette_to_rgb(png_ptr);
	if (color_type == PNG_COLOR_TYPE_GRAY && bit_depth < 8) png_set_gray_1_2_4_to_8(png_ptr);
	if (png_get_valid(png_ptr, info_ptr, PNG_INFO_tRNS)) png_set_tRNS_to_alpha(png_ptr);
	png_set_filler(png_ptr, 0xff, PNG_FILLER_AFTER);
	if (!image->data) {
		free(image);
		fclose(fp);
		png_destroy_read_struct(&png_ptr, png_infopp_NULL, png_infopp_NULL);
		return NULL;
	}
	line = (u32*) malloc(width * 4);
	if (!line) {
		free(image->data);
		free(image);
		fclose(fp);
		png_destroy_read_struct(&png_ptr, png_infopp_NULL, png_infopp_NULL);
		return NULL;
	}
	for (y = 0; y < height; y++) {
		png_read_row(png_ptr, (u8*) line, png_bytep_NULL);
		for (x = 0; x < width; x++) {
			u32 color = line[x];
			image->data[x + y * image->textureWidth] =  color;
		}
	}
	free(line);*/
	fclose(fp);
	png_destroy_read_struct(&png_ptr, png_infopp_NULL, png_infopp_NULL);
	return image;
}


int write_png(char *path, const byte *in, unsigned int width, unsigned int height)
{
    png_struct *pPngStruct = png_create_write_struct( PNG_LIBPNG_VER_STRING,
													  NULL, NULL, NULL );
	if (!pPngStruct){
		return 0;
	}

    png_info *pPngInfo = png_create_info_struct( pPngStruct );
	if (!pPngInfo){
		png_destroy_write_struct( &pPngStruct, NULL );
		return 0;
	}

	png_byte **buf = (png_byte**)malloc(height*sizeof(png_byte*));
	if (!buf){
        png_destroy_write_struct( &pPngStruct, &pPngInfo );
		return 0;
	}

	unsigned int y;
	for(y=0; y<height; y++)
		buf[y] = (byte*)&in[y*width*3];

    if (setjmp( pPngStruct->jmpbuf )){
		free(buf);
        png_destroy_write_struct( &pPngStruct, &pPngInfo );
        return 0;
    }

    FILE *fp = fopen( path, "wb" );
	if (!fp){
		free(buf);
        png_destroy_write_struct( &pPngStruct, &pPngInfo );
		return 0;
	}

    png_init_io( pPngStruct, fp );
    png_set_IHDR( pPngStruct, pPngInfo, width, height, 8, 
        PNG_COLOR_TYPE_RGB,
        PNG_INTERLACE_NONE,
        PNG_COMPRESSION_TYPE_DEFAULT,
        PNG_FILTER_TYPE_DEFAULT);
    png_write_info( pPngStruct, pPngInfo );
    png_write_image( pPngStruct, buf );
    png_write_end( pPngStruct, pPngInfo );

    png_destroy_write_struct( &pPngStruct, &pPngInfo );
    fclose(fp);
	free(buf);

    return 1;
}

int read_png(char *path, unsigned short *out, size_t outlen)
{
	FILE *fp = fopen(path,"rb");
	long img_size;
	int row;
	img_size=0;
	if(!fp){
		fclose(fp);
		return 0;
	}
	Image *image = LoadGFX(path);

	const size_t nSigSize = 8;
	byte signature[nSigSize];
	if (sceIoRead(fileno(fp), signature, sizeof(byte)*nSigSize) != nSigSize){
		fclose(fp);
		return 0;
	}

	if (!png_check_sig( signature, nSigSize )){
		fclose(fp);
		return 0;
	}

	png_struct *pPngStruct = png_create_read_struct( PNG_LIBPNG_VER_STRING,
													 NULL, NULL, NULL );
	if(!pPngStruct){
		fclose(fp);
		return 0;
	}

	png_info *pPngInfo = png_create_info_struct(pPngStruct);
	if(!pPngInfo){
		png_destroy_read_struct( &pPngStruct, NULL, NULL );
		fclose(fp);
		return 0;
	}

	if (setjmp( pPngStruct->jmpbuf )){
		png_destroy_read_struct( &pPngStruct, NULL, NULL );
		fclose(fp);
		return 0;
	}

	png_init_io( pPngStruct, fp );
	png_set_sig_bytes( pPngStruct, nSigSize );
	png_read_png( pPngStruct, pPngInfo,
			PNG_TRANSFORM_STRIP_16 | PNG_TRANSFORM_PACKING |
			PNG_TRANSFORM_EXPAND | PNG_TRANSFORM_BGR , NULL);

	int color_type = pPngInfo->color_type;

	/*if (outlen != width * height * sizeof(unsigned short)){
		png_destroy_read_struct( &pPngStruct, &pPngInfo, NULL );
		fclose(fp);
		return 0;
	}*/
	
	png_byte **pRowTable = pPngInfo->row_pointers;
	unsigned int x, y;
	byte r, g, b;
	for (y=0; y<image->imageHeight; y++){
		png_byte *pRow = pRowTable[y];
		for (x=0; x<image->imageWidth; x++){
			switch(color_type){
				case PNG_COLOR_TYPE_GRAY:
					r = g = b = *pRow++;
					break;
				case PNG_COLOR_TYPE_GRAY_ALPHA:
					r = g = b = *pRow++;
					pRow++;
					break;
				case PNG_COLOR_TYPE_RGB:
					b = *pRow++;
					g = *pRow++;
					r = *pRow++;
					break;
				case PNG_COLOR_TYPE_RGB_ALPHA:
					b = *pRow++;
					g = *pRow++;
					r = *pRow++;
					pRow++;
					break;
			}
			*out++ = RGB(r,g,b);
		}
	}
	/*for(;;){
		if (outlen != img_size * 2){
			*out++ = 0x0000;
			img_size++;
		}else
			break;
	}*/
	
	png_destroy_read_struct( &pPngStruct, &pPngInfo, NULL );
	fclose(fp);
	
	return 1;
}

/*
	Global variables
*/

/*
	This is basic error-handling stuff for the JPEG library.
*/
struct my_error_mgr {
	struct jpeg_error_mgr pub;
	jmp_buf setjmp_buffer;
};
typedef struct my_error_mgr *my_error_ptr;
METHODDEF(void) my_error_exit (j_common_ptr jpginfo)
{
	my_error_ptr myerr = (my_error_ptr) jpginfo;
	(*jpginfo->err->output_message) (jpginfo);
	longjmp(myerr->setjmp_buffer, 1);
}

/*
	Load a JPEG image into the offscreen buffer
	Returns 0 OK, nonzero=error
*/
int LoadJPEG(char *path, unsigned short *out)
{
	struct my_error_mgr jerr;
	FILE *infile;
	int row_stride;
	int i, y, out_width, out_height, skip_x=0, skip_y=0;
	int height_numerator = 0;
	
	JSAMPARRAY buffer = (JSAMPARRAY) malloc(sizeof(JSAMPARRAY));
	memset(buffer, 0, sizeof(JSAMPARRAY));
	
	if ((infile = fopen(path, "rb")) == NULL) {
		return 0;
	}

	jpginfo.err = jpeg_std_error(&jerr.pub);
	jerr.pub.error_exit = my_error_exit;
	if (setjmp(jerr.setjmp_buffer)) {
		jpeg_destroy_decompress(&jpginfo);
		fclose(infile);
		return 0;
	}

	jpeg_create_decompress(&jpginfo);
	jpeg_stdio_src(&jpginfo,infile);
	jpeg_read_header(&jpginfo,TRUE);

	jpginfo.out_color_space = JCS_RGB;
	jpginfo.quantize_colors = FALSE;
	jpginfo.dct_method = JDCT_IFAST;

	jpeg_start_decompress(&jpginfo);
	row_stride = jpginfo.output_width * jpginfo.output_components;
	buffer = (*jpginfo.mem->alloc_sarray) ((j_common_ptr) &jpginfo, JPOOL_IMAGE, row_stride, 1);

	// Quick and dirty calculation - Use the smaller of file dimension or
	// screen physical size.
	if (jpginfo.output_width < 480)
		out_width = jpginfo.output_width;
	else
		out_width = 480;
	if (jpginfo.output_height < 272)
		out_height = jpginfo.output_height;
	else
		out_height = 272;

	if(jpg_shrink){
		jpginfo.output_width = out_width;
		jpginfo.output_height = out_height;
	}

	printf("input width %d, output width %d, input height %d, output height %d\n",jpginfo.output_width, out_width, jpginfo.output_height, out_height);

	// fetch first scanline
	jpeg_read_scanlines(&jpginfo, buffer, 1);

	// scanline loop
	for (y=0; y<out_height; y++) {
		int width_numerator;
		unsigned char *src;
		unsigned short r,g,b;
		unsigned short *dest;

		// increment src scanline fraction
		height_numerator += jpginfo.output_height;
		if (height_numerator > out_height) {
			while (height_numerator > out_height) {
				height_numerator -= out_height;
				jpeg_read_scanlines(&jpginfo, buffer, 1);
			}
		}
		
		// scanline loop
		width_numerator = 0;
		src = (unsigned char *) *buffer;
		r = (*src++);
		g = (*src++);
		b = (*src++);
		for (i=0; i<out_width; i++) {
			// increment src pointer fraction
			width_numerator += jpginfo.output_width;
			if (width_numerator > out_width) {
				while (width_numerator > out_width) {
					width_numerator -= out_width;
					r = (*src++);
					g = (*src++);
					b = (*src++);
				}
			}
			*out++ = RGB(r,g,b);
		}
	}

	jpginfo.output_width = out_width;
	jpginfo.output_height = out_height;

	jpeg_finish_decompress(&jpginfo);
	jpeg_destroy_decompress(&jpginfo);
	fclose(infile);
	return 1;
}

int read_bitmap(char *path, unsigned short *out, size_t outlen)
{
	/*
	FILE *fp = fopen(path,"rb");
	if(!fp)
		return 0;
	fseek(fp, 0, SEEK_END);
	size_t size = ftell(fp);
	fseek(fp, 0, SEEK_SET);
	byte *buf = (byte*)malloc(size);
	if(!buf){
		fclose(fp);
		return 0;
	}
	fread(buf, 1, size, fp);
	fclose(fp);
	*/
	int fd = sceIoOpen(path, SCE_O_RDONLY, 644);
	if(fd<0)
		return 0;
	size_t size = sceIoLseek(fd, 0, SEEK_END);
	sceIoLseek(fd, 0, SEEK_SET);
	byte *buf = (byte*)malloc(size);
	if(!buf){
		sceIoClose(fd);
		return 0;
	}
	sceIoRead(fd, buf, size);
	sceIoClose(fd);

	if ((size - 0x36)/3 != outlen/2){
		free(buf);
		return 0;
	}
	
	byte *menu_bg = buf + 0x36;
	byte r, g, b;
	unsigned int x, y, yy;
	for(y=0; y<272; y++){
		for(x=0; x<480; x++){
			yy = 271 - y;
			r = *(menu_bg + (yy*480 + x)*3 + 2);
			g = *(menu_bg + (yy*480 + x)*3 + 1);
			b = *(menu_bg + (yy*480 + x)*3);
			*out++=RGB(r,g,b);
		}
	}
	free(buf);
	return 1;
}

/*
// PackBitsˆ³k•„†‰»
int PackBits_encode( byte *in, byte *out, int size )
{
	byte ltr[128], bak, cur, *wp=out;
	int n=1, m=0, i=0, j;
	
	bak = in[i++];
	while(i<size){
		cur = in[i++];
		if(cur != bak){
			if(n>=2){
				*wp++ = -(n-1);
				*wp++ = bak;
			}else{
				ltr[m++] = bak;
				if(m >= 128){
					*wp++ = m-1;
					for(j=0; j<m; j++) *wp++ = ltr[j];
					m=0;
				}
			}
			bak = cur;
			n = 1;
		}else{
			if(m >= 1){
				*wp++ = m-1;
				for(j=0; j<m; j++) *wp++ = ltr[j];
				m = 0;
			}
			if(n < 128){
				n++;
			}else{
				*wp++ = -(n-1);
				*wp++ = bak;
				n = 1;
			}
		}
	}
	if(n>=2){
		if(m >= 1){
			*wp++ = m-1;
			for(j=0; j<m; j++) *wp++ = ltr[j];
		}
		*wp++ = -(n-1);
		*wp++ = bak;
	}else{
		ltr[m++] = bak;
		*wp++ = m-1;
		for(j=0; j<m; j++) *wp++ = ltr[j];
	}
	
	return wp - out;
}
*/

// PackBitsˆ³k•¡‡‰»
int PackBits_decode( byte *in, byte *out, int size )
{
	byte *limit=in+size, *wp=out;
	int i, c;
	
	while( in < limit ){
		c = (char)(*in++);
		if(c < 0){
			for(i=1-c; i>0; i--) *wp++ = *in;
			in++;
		}else{
			for(i=1+c; i>0; i--) *wp++ = *in++;
		}
	}
	
	return wp - out;
}

