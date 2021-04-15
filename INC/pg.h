// primitive graphics for Hello World PSP

#ifndef PG_H
#define PG_H

#define NUM2HEX(n) ((n) & 0xFF)

#define RGB(r,g,b) ((((b>>3) & 0x1F)<<10)|(((g>>3) & 0x1F)<<5)|(((r>>3) & 0x1F)<<0)|0x8000)
#define RGBA(r,g,b,a) (((b)<<16)|((g)<<8)|(r)|((a)<<24))
#define RGB_R(c) ((c) & 0xFF)
#define RGB_G(c) (((c) >> 8) & 0xFF)
#define RGB_B(c) (((c) >> 16) & 0xFF)
#define RGB_16to32(c) RGBA((((c)&0x1F)*255/31),((((c)>>5)&0x1F)*255/31),((((c)>>10)&0x1F)*255/31),((c&0x8000)?0xFF:0))

#define IS_ALPHA(color) (((color)&0xff000000)==0xff000000?0:1)

extern u32 new_pad, now_pad, old_pad;
extern ctrl_data_t paddata;

#define SCREEN_WIDTH  480
#define SCREEN_HEIGHT 272
#define		PIXELSIZE	1				//in short
#define		LINESIZE	512				//in short
#define		FRAMESIZE	0x44000			//in byte

//480*272 = 60*38
#define CMAX_X 60
#define CMAX_Y 34
#define CMAX2_X 30
#define CMAX2_Y 17
#define CMAX4_X 15
#define CMAX4_Y 8


typedef u32 Color;
#define A(color) ((u8)(color >> 24 & 0xFF))
#define B(color) ((u8)(color >> 16 & 0xFF))
#define G(color) ((u8)(color >> 8 & 0xFF))
#define R(color) ((u8)(color & 0xFF))

typedef struct
{
	int textureWidth;  // the real width of data, 2^n with n>=0
	int textureHeight;  // the real height of data, 2^n with n>=0
	int imageWidth;  // the image width
	int imageHeight;
	int realimageWidth;  // the real image width
	int realimageHeight;
	int colorData;
	Color *data;
} Image;

void pgInit();
void pgWaitV();
void pgWaitVn(unsigned long count);
void pgScreenFrame(long mode,long frame);
void pgScreenFlip();
void pgScreenFlipV();
void pgPrint_drawbg(unsigned long x,unsigned long y,unsigned long color,unsigned long bgcolor,const char *str);
void pgPrint(unsigned long x,unsigned long y,unsigned long color,const char *str);
void pgPrint2(unsigned long x,unsigned long y,unsigned long color,const char *str);
void pgPrint4(unsigned long x,unsigned long y,unsigned long color,const char *str);
void pgFillvram(unsigned long color);

void pgBitBlt(unsigned long x,unsigned long y,unsigned long w,unsigned long h,unsigned long mag,unsigned short *d);
void pgBitBltN1(unsigned long x,unsigned long y,unsigned long *d);
void pgBitBltN15(unsigned long x,unsigned long y,unsigned long *d);
void pgBitBltN2(unsigned long x,unsigned long y,unsigned long h,unsigned long *d);
void pgBitBltStScan(unsigned long x,unsigned long y,unsigned long h,unsigned long *d);
void pgBitBltSt2wotop(unsigned long x,unsigned long y,unsigned long h,unsigned long *d);
void pgBitBltSt2wobot(unsigned long x,unsigned long y,unsigned long h,unsigned long *d);
void pgBitBltSt2Fix(unsigned long x,unsigned long y,unsigned long h,unsigned long mag,const unsigned short *d);
void pgBitBltStFull(unsigned long x,unsigned long y,unsigned long h,unsigned long mag,const unsigned short *d);

void pgPutChar(unsigned long x,unsigned long y,unsigned long color,unsigned long bgcolor,unsigned char ch,char drawfg,char drawbg,char mag);
void pgDrawFrame(unsigned long x1, unsigned long y1, unsigned long x2, unsigned long y2, unsigned long color);
void pgFillBox(unsigned long x1, unsigned long y1, unsigned long x2, unsigned long y2, unsigned long color);
void mh_print(int x,int y,const char *str,int col);
char *pgGetVramAddr(unsigned long x,unsigned long y);

void readpad(void);

void pgcPuthex2(const unsigned long s);
void pgcPuthex8(const unsigned long s);


extern u32 now_tick;

//optimize

//long�z����R�s�[�B�z�񋫊E��4�o�C�g�A���C������Ă���K�v����
static inline void __memcpy4(void *d, void *s, unsigned long c)
{
	//for (; c>0; --c) *(((unsigned long *)d)++)=*(((unsigned long *)s)++);
	unsigned long *dd=d, *ss=s;
	for (; c>0; --c) *dd++ = *ss++;
}


//long�z��ɃZ�b�g�B�z�񋫊E��4�o�C�g�A���C������Ă���K�v����
static inline void __memset4(void *d, unsigned long v, unsigned long c)
{
	//for (; c>0; --c) *(((unsigned long *)d)++)=v;
	unsigned long *dd=d;
	for (; c>0; --c) *dd++=v;
}


//long�z����R�s�[�B�z�񋫊E��4�o�C�g�A���C������Ă���K�v����
//�R���p�C���̍œK���ɂ���ė\�����Ȃ��R�[�h����������邽�߁A�\���ɒ��ӂ̂��ƁB
static inline void __memcpy4aa(void *d, void *s, unsigned long c)
{
	unsigned long wk0,wk1,wk2,wk3;

	asm volatile (
		"		.set	push"				"\n"
		"		.set	noreorder"			"\n"

		"		move	%1,%4 "				"\n"
		"		move	%2,%5 "				"\n"
		"		move	%3,%6 "				"\n"

		"1:		lw		%0,0(%2) "			"\n"
		"		addiu	%3,%3,-1 "			"\n"
		"		addiu	%2,%2,4 "			"\n"
		"		sw		%0,0(%1) "			"\n"
		"		bnez	%3,1b "				"\n"
		"		addiu	%1,%1,4 "			"\n"

		"		.set	pop"				"\n"

			:	"=&r" (wk0),	// %0
				"=&r" (wk1),	// %1
				"=&r" (wk2),	// %2
				"=&r" (wk3)		// %3
			:	"r" (d),		// %4
				"r" (s),		// %5
				"r" (c)			// %6
	);
}

//long�z����R�s�[�B�z�񋫊E��4�o�C�g�A���C������Ă���K�v����
//s,d�͎Q�Ɠn�������ɂȂ�̂ŁA���^�[����͕ύX����Ă���ƍl�����ق����ǂ�
//�R���p�C���̍œK���ɂ���ė\�����Ȃ��R�[�h����������邽�߁A�\���ɒ��ӂ̂��ƁB__memcpy4�̂ق������S�B
//�Ƃ����܂���c�ŏ����Ă��S�R�ς��Ȃ��悤�ȁB��ݶޯ��̂ɁB
static inline void __memcpy4a(unsigned long *d, unsigned long *s, unsigned long c)
{
	unsigned long wk,counter;

	asm volatile (
		"		.set	push"				"\n"
		"		.set	noreorder"			"\n"

		"		move	%1,%4 "				"\n"
		"1:		lw		%0,0(%3) "			"\n"
		"		addiu	%1,%1,-1 "			"\n"
		"		addiu	%3,%3,4 "			"\n"
		"		sw		%0,0(%2) "			"\n"
		"		bnez	%1,1b "				"\n"
		"		addiu	%2,%2,4 "			"\n"

		"		.set	pop"				"\n"

			:	"=&r" (wk),		// %0
				"=&r" (counter)	// %1
			:	"r" (d),		// %2
				"r" (s),		// %3
				"r" (c)			// %4
	);
}

#define __USE_MIPS32R2__

static inline void cpy2x(unsigned long *d, unsigned long cc)
{
#ifdef __USE_MIPS32R2__
	unsigned long wk0;
	asm volatile (
		"		.set	push"				"\n"
		"		.set	noreorder"			"\n"

		"		.set	mips32r2"			"\n"
		
		"		srl		%0,%2,16"			"\n"
		"		ins 	%2,%2,16,16"		"\n"
		"		sw		%2,0(%1)"			"\n"
		"		ins 	%0,%0,16,16"		"\n"
		"		sw		%0,4(%1)"			"\n"
		
		"		.set	pop"				"\n"
		
			:	"=&r" (wk0)		// %0
			:	"r" (d),		// %1
				"r" (cc)		// %2
	);
#else
	*d      = (cc&0x0000ffff)|(cc<<16);
	*(d+1)  = (cc>>16)|(cc&0xffff0000);
#endif
}

static inline void cpy3x(unsigned long *d, unsigned long cc)
{
#ifdef __USE_MIPS32R2__
	unsigned long wk0;
	asm volatile (
		"		.set	push"				"\n"
		"		.set	noreorder"			"\n"

		"		.set	mips32r2"			"\n"
		
		"		sw		%2,4(%1)"			"\n"
		"		srl		%0,%2,16"			"\n"
		"		ins 	%2,%2,16,16"		"\n"
		"		sw		%2,0(%1)"			"\n"
		"		ins 	%0,%0,16,16"		"\n"
		"		sw		%0,8(%1)"			"\n"
		
		"		.set	pop"				"\n"
		
			:	"=&r" (wk0)		// %0
			:	"r" (d),		// %1
				"r" (cc)		// %2
	);
#else
	*d      = (cc&0x0000ffff)|(cc<<16);
	*(d+1)  = cc;
	*(d+2)  = (cc>>16)|(cc&0xffff0000);
#endif
}

#endif
