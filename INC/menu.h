#ifndef MENU_H
#define MENU_H

#define MAX_TEXT 1024

unsigned short bgBitmap[480*272];
unsigned short frBitmap[480*272];

int fr_width, fr_height, bg_width, bg_height;

enum{
	STATE_SLOT_MAX=9,
	DEF_COLOR0=0x9063,
	DEF_COLOR1=RGB(85,85,95),
	DEF_COLOR2=RGB(105,105,115),
	DEF_COLOR3=0xffff,
};

typedef struct
{
	unsigned long buttons;
	int n;
} S_BUTTON;

#define VERRIN "3.3"
#define VERCNF "GeMP3.3"

typedef struct
{
	char pngpath[MAX_PATH];
	char lastpath[MAX_PATH];
	char framepath[MAX_PATH];
	unsigned long textcolor[4];
	unsigned int jpg_shrink;
	unsigned int jpg_shrink_fr;
	float mp3_volume;
} SETTING2;

extern SETTING2 setting2;

typedef struct
{
	char vercnf[16];
	u8 compress;
	u8 thumb;
	u8 quickslot;
	u8 screensize;
	u8 bScreenSizes[16]; //余分に確保
	u8 gb_type;
	u8 gb_palette;
	u8 bGB_Pals[32]; //余分に確保
	u8 frameskip;
	u8 vsync;
	u8 cpu_clock;
	S_BUTTON skeys[32]; //余分に確保
	u8 analog2dpad;
	u8 bgbright;
	u8 sound_buffer;
	int sound;
	int frame;
} SETTING;

extern SETTING setting, tmpsetting;
extern int bTurbo, bBitmap;
extern unsigned short bgBitmap[];
extern unsigned short thumb_w[160*144];
extern const char *cpu_clocks[];

extern byte *state_tmp;
extern unsigned short thumb_tmp[160*144];
extern ScePspDateTime state_tmp_time;

extern char * buffer;

void init_config();
void check_config();
void set_gb_type();
int load_menu_bg();
void rin_frame(const char *msg0, const char *msg1);
void rin_menu(void);
int rin_MessageBox(const char *msg, int type);

#endif
