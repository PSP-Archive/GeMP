#include "INC/main.h"
#include "INC/image.h"
#include "colbl.c"

SETTING setting, tmpsetting;
SETTING2 setting2;
int bTurbo=0, bBitmap;
word thumb_w[160*144];

byte *state_tmp = NULL;
word thumb_tmp[160*144];
ScePspDateTime state_tmp_time;

void init_config()
{
	int i;

	strcpy(setting.vercnf, VERCNF);
	
	setting.screensize = SCR_X2_FIT;
	setting.gb_type = 0;
	setting.gb_palette = PAL_DARK_GREEN;
	setting.frameskip = 2;
	setting.vsync = 1;
	setting.sound = 1;
	setting.sound_buffer = 0;
	setting.bgbright=100;
	
	for(i=0; i<32; i++){
		setting.skeys[i].buttons = 0;
		setting.skeys[i].n = -1;
	}
	for(i=0; i<=6; i++)
		setting.skeys[i].n = i;
	setting.skeys[0].buttons = CTRL_CROSS;
	setting.skeys[1].buttons = CTRL_CIRCLE;
	setting.skeys[2].buttons = CTRL_TRIANGLE;
	setting.skeys[3].buttons = CTRL_SQUARE;
	setting.skeys[4].buttons = CTRL_SELECT;
	setting.skeys[5].buttons = CTRL_START;
	setting.skeys[6].buttons = CTRL_LTRIGGER|CTRL_RTRIGGER;
	setting.skeys[7].buttons = CTRL_RTRIGGER|CTRL_SELECT;
	setting.skeys[7].n = 11;
	setting.skeys[8].buttons = CTRL_RTRIGGER|CTRL_START;
	setting.skeys[8].n = 12;
	setting.skeys[9].buttons = CTRL_LTRIGGER|CTRL_CROSS;
	setting.skeys[9].n = 16;
	
	setting.analog2dpad=1;
	setting.thumb = 1;
	setting.cpu_clock = 5;
	for(i=0; i<16; i++)
		setting.bScreenSizes[i] = 1;
	for(i=0; i<32; i++)
		setting.bGB_Pals[i] = 1;
	setting.compress = 1;
	
	setting.frame = 0;
}

void check_config()
{
	int i;

	if(strcmp(setting.vercnf, VERCNF)){
		init_config();
		return;
	}

	if(setting.screensize>=SCR_END)
		setting.screensize = SCR_X2_FIT;
	if(setting.gb_type>4)
		setting.gb_type = 0;
	if(setting.gb_palette<=PAL_MONOCHROME || setting.gb_palette>=PAL_SGB)
		setting.gb_palette = PAL_DARK_GREEN;
	if(setting.frameskip > 9)
		setting.frameskip=0;
	if(setting.sound_buffer>2)
		setting.sound_buffer = 0;
	if(setting.bgbright>100)
		setting.bgbright=100;
	if(setting.cpu_clock>5)
		setting.cpu_clock = 0;
	if(strncmp(setting2.lastpath, "ms0:/", 5))
		strcpy(setting2.lastpath,RinPath);
	for(i=1; i<PAL_SGB; i++)
		if(setting.bGB_Pals[i]) break;
	if(i>=PAL_SGB)
		setting.bGB_Pals[1] = 1;
	for(i=0; i<SCR_END; i++)
		if(setting.bScreenSizes[i]) break;
	if(i>=SCR_END)
		setting.bScreenSizes[0] = 1;
		
	lcd_set_mpal(setting.gb_palette);
	load_frame_bg();
}

void set_gb_type()
{
	if (rom_get_loaded()){
		switch(setting.gb_type){
		case 0:
			if(org_gbtype==1){
				rom_get_info()->gb_type = 2;
				lcd_set_mpal(setting.gb_palette);
			}else if(org_gbtype == 2){
				rom_get_info()->gb_type = 2;
				lcd_set_mpal(PAL_SGB);
			}else if(org_gbtype == 3){
				rom_get_info()->gb_type = 3;
				lcd_set_mpal(PAL_SGB);
			}
			break;
		case 1:
			rom_get_info()->gb_type = 1;
			lcd_set_mpal(PAL_MONOCHROME);
			break;
		case 2:
			rom_get_info()->gb_type = 2;
			if(sgb_mode)
				lcd_set_mpal(PAL_SGB);
			else
				lcd_set_mpal(setting.gb_palette);
			break;
		case 3:
			rom_get_info()->gb_type = 3;
			lcd_set_mpal(setting.gb_palette);
			break;
		case 4:
			rom_get_info()->gb_type = 4;
			lcd_set_mpal(setting.gb_palette);
			break;
		}
		
		if(rom_get_info()->gb_type>=3 && org_gbtype==3)
			now_gb_mode = 3;
		else if(rom_get_info()->gb_type==2 && sgb_mode)
			now_gb_mode = 2;
		else
			now_gb_mode = 1;
	}
}

int get_nShortcutKey(u32 buttons)
{
	int i;
	for(i=6; i<32; i++){
		if (setting.skeys[i].buttons==0)
			return -1;
		if ((buttons & setting.skeys[i].buttons)==setting.skeys[i].buttons)
			return setting.skeys[i].n;
	}
	return -1;
}

// îºìßñæèàóù
unsigned short rgbTransp(unsigned short fgRGB, unsigned short bgRGB, int alpha) {
    unsigned short fgR, fgG, fgB;
    unsigned short bgR, bgG, bgB;
	unsigned short R, G, B;
 	unsigned short rgb;

    fgB = (fgRGB >> 10) & 0x1F;
    fgG = (fgRGB >> 5) & 0x1F;
    fgR = fgRGB & 0x1F;

    bgB = (bgRGB >> 10) & 0x1F;
    bgG = (bgRGB >> 5) & 0x1F;
    bgR = bgRGB & 0x1F;

	R = coltbl[fgR][bgR][alpha/10];
	G = coltbl[fgG][bgG][alpha/10];
	B = coltbl[fgB][bgB][alpha/10];

	rgb = (((B & 0x1F)<<10)+((G & 0x1F)<<5)+((R & 0x1F)<<0)+0x8000);
    return rgb;
}

void bgbright_change()
{
	unsigned short *vptr=bgBitmap, rgb;
	int i;
	for(i=0; i<bg_height*bg_width; i++){
		rgb = *vptr;
		*vptr++ = rgbTransp(rgb, 0x0000, setting.bgbright);
	}
}

void rin_frame(const char *msg0, const char *msg1)
{
	char battery[32], tmp[256];
	int ret;
	
	if(scePowerIsBatteryExist()){
		sprintf(battery,"Battery[%d%%",scePowerGetBatteryLifePercent());
		if(!scePowerIsPowerOnline()){
			if((ret=scePowerGetBatteryLifeTime()) >= 0)
				sprintf(&battery[strlen(battery)],"(%d:%02d)",ret/60,ret%60);
		}
		strcat(battery,"]");
	}
	
	if(bBitmap)
		pgBitBlt(0,0,bg_width,bg_height,1,bgBitmap);
	else
		pgFillvram(0);
		
	//pgPrint(1, 1, setting2.textcolor[1], "Homer's RIN v2.0");
	if(msg0!=0) mh_print(17, 14, msg0, setting2.textcolor[1]);
	//pgDrawFrame(17,25,463,248,setting2.textcolor[1]);
	//pgDrawFrame(18,26,462,247,setting2.textcolor[1]);
	if(msg1!=0) mh_print(17, 252, msg1, setting2.textcolor[1]);
	mh_print(465-strlen(battery)*5, 252, battery, setting2.textcolor[2]);
}

int rin_MessageBox(const char *msg, int type){
	for(;;){
		readpad();
		if(new_pad & CTRL_CROSS){
			return 1;
		}else if(new_pad & CTRL_CIRCLE && type){
			return 0;
		}
		
		if(type)
			rin_frame(0,"Å~ÅFOK  ÅõÅFCancel");
		else
			rin_frame(0,"Å~ÅFOK");
		mh_print(28,32,msg,setting2.textcolor[3]);
		pgScreenFlipV();
	}
}

void rin_colorconfig(void)
{
	enum
	{
		COLOR0_R,
		COLOR0_G,
		COLOR0_B,
		COLOR1_R,
		COLOR1_G,
		COLOR1_B,
		COLOR2_R,
		COLOR2_G,
		COLOR2_B,
		COLOR3_R,
		COLOR3_G,
		COLOR3_B,
		BG_BRIGHT,
		EXIT,
		INIT,
	};
	char tmp[4], msg[256];
	int color[4][3];
	int sel=0, x, y, i, crs_count, bLoop=1;

	memset(color, 0, sizeof(int)*4*3);
	for(i=0; i<4; i++){
		color[i][2] = setting2.textcolor[i]>>10 & 0x1F;
		color[i][1] = setting2.textcolor[i]>>5 & 0x1F;
		color[i][0] = setting2.textcolor[i] & 0x1F;
	}

	for(;;){
		readpad();
		if(now_pad & (CTRL_UP|CTRL_DOWN|CTRL_LEFT|CTRL_RIGHT))
			crs_count=0;
		if(new_pad & CTRL_CROSS){
			switch(sel){
			case EXIT:
				bLoop=0;
				break;
			case INIT:
				color[0][2] = DEF_COLOR0>>10 & 0x1F;
				color[0][1] = DEF_COLOR0>>5 & 0x1F;
				color[0][0] = DEF_COLOR0 & 0x1F;
				color[1][2] = DEF_COLOR1>>10 & 0x1F;
				color[1][1] = DEF_COLOR1>>5 & 0x1F;
				color[1][0] = DEF_COLOR1 & 0x1F;
				color[2][2] = DEF_COLOR2>>10 & 0x1F;
				color[2][1] = DEF_COLOR2>>5 & 0x1F;
				color[2][0] = DEF_COLOR2 & 0x1F;
				color[3][2] = DEF_COLOR3>>10 & 0x1F;
				color[3][1] = DEF_COLOR3>>5 & 0x1F;
				color[3][0] = DEF_COLOR3 & 0x1F;
				setting.bgbright = 100;
				if(bBitmap){
					load_menu_bg();
					bgbright_change();
				}
				break;
			case BG_BRIGHT:
				//ãPìxïœçX
				setting.bgbright += 10;
				if(setting.bgbright > 100) setting.bgbright=0;
				if(bBitmap){
					load_menu_bg();
					bgbright_change();
				}
				break;
			default:
				if(color[sel/3][sel%3]<31)
					color[sel/3][sel%3]++;
				break;
			}
		}else if(new_pad & CTRL_SQUARE){
			if(sel == BG_BRIGHT) {
				//ãPìxïœçX
				if(setting.bgbright > 10)
					setting.bgbright-=10;
				else
					setting.bgbright=100;
				if(bBitmap){
					load_menu_bg();
					bgbright_change();
				}
			}else if(sel>=COLOR0_R && sel<=COLOR3_B){
				if(color[sel/3][sel%3]>0)
					color[sel/3][sel%3]--;
			}
		}else if(new_pad & CTRL_CIRCLE){
			break;
		}else if(new_pad & CTRL_UP){
			if(sel!=0)	sel--;
			else		sel=INIT;
		}else if(new_pad & CTRL_DOWN){
			if(sel!=INIT)	sel++;
			else			sel=0;
		}else if(new_pad & CTRL_RIGHT){
			if(sel<COLOR1_R) 		sel=COLOR1_R;
			else if(sel<COLOR2_R)	sel=COLOR2_R;
			else if(sel<COLOR3_R)	sel=COLOR3_R;
			else if(sel<BG_BRIGHT)	sel=BG_BRIGHT;
			else if(sel<EXIT)		sel=EXIT;
		}else if(new_pad & CTRL_LEFT){
			if(sel>BG_BRIGHT)		sel=BG_BRIGHT;
			else if(sel>COLOR3_B)	sel=COLOR3_R;
			else if(sel>COLOR2_B)	sel=COLOR2_R;
			else if(sel>COLOR1_B)	sel=COLOR1_R;
			else					sel=COLOR0_R;
		}
		if(!bLoop) break;
		
		for(i=0; i<4; i++)
			setting2.textcolor[i]=color[i][2]<<10|color[i][1]<<5|color[i][0]|0x8000;
		
		if (crs_count++>=30) crs_count=0;
		
		x = 2;
		y = 5;
		
		if(sel>=COLOR0_R && sel<=BG_BRIGHT)
			strcpy(msg, "Å~ÅFAddÅ@Å†ÅFSub ÅõÅFReturn");
		else
			strcpy(msg, "Å~ÅFOK ÅõÅReturn");

		rin_frame(0, msg);

		pgPrint(x,y++,setting2.textcolor[3],"  COLOR0 R:");
		pgPrint(x,y++,setting2.textcolor[3],"  COLOR0 G:");
		pgPrint(x,y++,setting2.textcolor[3],"  COLOR0 B:");
		y++;
		pgPrint(x,y++,setting2.textcolor[3],"  COLOR1 R:");
		pgPrint(x,y++,setting2.textcolor[3],"  COLOR1 G:");
		pgPrint(x,y++,setting2.textcolor[3],"  COLOR1 B:");
		y++;
		pgPrint(x,y++,setting2.textcolor[3],"  COLOR2 R:");
		pgPrint(x,y++,setting2.textcolor[3],"  COLOR2 G:");
		pgPrint(x,y++,setting2.textcolor[3],"  COLOR2 B:");
		y++;
		pgPrint(x,y++,setting2.textcolor[3],"  COLOR3 R:");
		pgPrint(x,y++,setting2.textcolor[3],"  COLOR3 G:");
		pgPrint(x,y++,setting2.textcolor[3],"  COLOR3 B:");
		y++;
		if(setting.bgbright / 100 == 1)
			pgPrint(x,y++,setting2.textcolor[3],"  BG BRIGHT:100%");
		else
			pgPrint(x,y++,setting2.textcolor[3],"  BG BRIGHT:  0%");
		if(setting.bgbright % 100 != 0)			// 10%Å`90%
			pgPutChar((x+13)*8,(y-1)*8,setting2.textcolor[3],0,'0'+setting.bgbright/10,1,0,1);
		y++;
		pgPrint(x,y++,setting2.textcolor[3],"  Return to Main Menu");
		pgPrint(x,y++,setting2.textcolor[3],"  Default");

		x=14; y=5;
		for(i=0; i<12; i++){
			if(i!=0 && i%3==0) y++;
			sprintf(tmp, "%d", color[i/3][i%3]);
			pgPrint(x,y++,setting2.textcolor[3],tmp);
		}
		
		if (crs_count < 15){
			x = 2;
			y = sel + 5;
			if(sel>=COLOR1_R) y++;
			if(sel>=COLOR2_R) y++;
			if(sel>=COLOR3_R) y++;
			if(sel>=BG_BRIGHT) y++;
			if(sel>=EXIT) y++;
			pgPutChar((x+1)*8,y*8,setting2.textcolor[3],0,127,1,0,1);
		}
		
		pgScreenFlipV();
	}
}

int cmp_skey(S_BUTTON *a, S_BUTTON *b)
{
	int i, na=0, nb=0;
	
	for(i=0; i<32; i++){
		if ((a->buttons >> i) & 1) na++;
		if ((b->buttons >> i) & 1) nb++;
	}
    return nb-na;
}

void sort_skeys(S_BUTTON *a, int left, int right) {
	S_BUTTON tmp, pivot;
	int i, p;
	
	if (left < right) {
		pivot = a[left];
		p = left;
		for (i=left+1; i<=right; i++) {
			if (cmp_skey(&a[i],&pivot)<0){
				p=p+1;
				tmp=a[p];
				a[p]=a[i];
				a[i]=tmp;
			}
		}
		a[left] = a[p];
		a[p] = pivot;
		sort_skeys(a, left, p-1);
		sort_skeys(a, p+1, right);
	}
}

// by kwn
void rin_keyconfig(void)
{
	enum
	{
		CONFIG_A,
		CONFIG_B,
		CONFIG_RAPIDA,
		CONFIG_RAPIDB,
		CONFIG_SELECT,
		CONFIG_START,
		CONFIG_MENU,
		CONFIG_WAIT,
		CONFIG_VSYNC,
		CONFIG_SOUND,
		CONFIG_SCREENSIZE,
		CONFIG_QUICKSAVE,
		CONFIG_QUICKLOAD,
		CONFIG_STATE_SLOT,
		CONFIG_GB_COLOR,
		CONFIG_CPU_CLOCK,
		CONFIG_SCREENSHOT,
		CONFIG_ANALOG2DPAD,
		CONFIG_EXIT,
	};
	char msg[256];
	int key_config[32];
	int sel=0, x, y, i, bPad, crs_count;
	
	for(i=0; i<32; i++)
		key_config[i] = 0;
	for(i=0; i<32; i++){
		if(setting.skeys[i].n >= 0)
			key_config[setting.skeys[i].n] = setting.skeys[i].buttons;
	}
	
	for(;;){
		readpad();
		if(now_pad & (CTRL_UP|CTRL_DOWN|CTRL_LEFT|CTRL_RIGHT))
			crs_count=0;
		if(now_pad==CTRL_LEFT || now_pad==CTRL_RIGHT){
			if(sel!=CONFIG_EXIT && sel!=CONFIG_MENU && sel!=CONFIG_ANALOG2DPAD)
				key_config[sel] = 0;
		}else if(now_pad==CTRL_UP){
			if(bPad==0){
				if(sel!=0)	sel--;
				else		sel=CONFIG_EXIT;
				bPad++;
			}else if(bPad >= 25){
				if(sel!=0)	sel--;
				else		sel=CONFIG_EXIT;
				bPad=20;
			}else
				bPad++;
		}else if(now_pad==CTRL_DOWN){
			if(bPad==0){
				if(sel!=CONFIG_EXIT)sel++;
				else				sel=0;
				bPad++;
			}else if(bPad >= 25){
				if(sel!=CONFIG_EXIT)sel++;
				else				sel=0;
				bPad=20;
			}else
				bPad++;
		}else if(new_pad != 0){
			if(sel==CONFIG_EXIT && new_pad&CTRL_CROSS)
				break;
			else if(sel==CONFIG_ANALOG2DPAD && new_pad&CTRL_CROSS)
				setting.analog2dpad = !setting.analog2dpad;
			else
				key_config[sel] = now_pad;
		}else{
			bPad=0;
		}
		
		if (crs_count++>=30) crs_count=0;
		
		if(sel>=CONFIG_ANALOG2DPAD)
			strcpy(msg,"Å~ÅFOK");
		else
			strcpy(msg,"Å©Å®ÅFClear");
		
		rin_frame(0, msg);
		
		x=2; y=5;
		pgPrint(x,y++,setting2.textcolor[3],"  A BUTTON       :");
		pgPrint(x,y++,setting2.textcolor[3],"  B BUTTON       :");
		pgPrint(x,y++,setting2.textcolor[3],"  A BUTTON(RAPID):");
		pgPrint(x,y++,setting2.textcolor[3],"  B BUTTON(RAPID):");
		pgPrint(x,y++,setting2.textcolor[3],"  SELECT BUTTON  :");
		pgPrint(x,y++,setting2.textcolor[3],"  START BUTTON   :");
		pgPrint(x,y++,setting2.textcolor[3],"  MENU BUTTON    :");
		pgPrint(x,y++,setting2.textcolor[3],"  TURBO ON/OFF   :");
		pgPrint(x,y++,setting2.textcolor[3],"  VSYNC ON/OFF   :");
		pgPrint(x,y++,setting2.textcolor[3],"  SOUND ON/OFF   :");
		pgPrint(x,y++,setting2.textcolor[3],"  SCREEN SIZE    :");
		pgPrint(x,y++,setting2.textcolor[3],"  QUICK SAVE     :");
		pgPrint(x,y++,setting2.textcolor[3],"  QUICK LOAD     :");
		pgPrint(x,y++,setting2.textcolor[3],"  QUICK SLOT     :");
		pgPrint(x,y++,setting2.textcolor[3],"  GB PALETTE     :");
		pgPrint(x,y++,setting2.textcolor[3],"  CPU CLOCK      :");
		pgPrint(x,y++,setting2.textcolor[3],"  SCREENSHOT     :");
		y++;
		if(setting.analog2dpad)
			pgPrint(x,y++,setting2.textcolor[3],"  AnalogPad to D-Pad: ON");
		else
			pgPrint(x,y++,setting2.textcolor[3],"  AnalogPad to D-Pad: OFF");
		y++;
		pgPrint(x,y++,setting2.textcolor[3],"  Return to Main Menu");
		
		for (i=0; i<CONFIG_ANALOG2DPAD; i++){
			y = i + 5;
			int j = 0;
			msg[0]=0;
			if(key_config[i] == 0){
				strcpy(msg,"UNDEFINED");
			}else{
				if (key_config[i] & CTRL_LTRIGGER){
					msg[j++]='L'; msg[j++]='+'; msg[j]=0;
				}
				if (key_config[i] & CTRL_RTRIGGER){
					msg[j++]='R'; msg[j++]='+'; msg[j]=0;
				}
				if (key_config[i] & CTRL_CIRCLE){
					msg[j++]=1; msg[j++]='+'; msg[j]=0;
				}
				if (key_config[i] & CTRL_CROSS){
					msg[j++]=2; msg[j++]='+'; msg[j]=0;
				}
				if (key_config[i] & CTRL_SQUARE){
					msg[j++]=3; msg[j++]='+'; msg[j]=0;
				}
				if (key_config[i] & CTRL_TRIANGLE){
					msg[j++]=4; msg[j++]='+'; msg[j]=0;
				}
				if (key_config[i] & CTRL_START){
					strcat(msg,"START+"); j+=6;
				}
				if (key_config[i] & CTRL_SELECT){
					strcat(msg,"SELECT+"); j+=7;
				}
				if (key_config[i] & CTRL_UP){
					msg[j++]=5; msg[j++]='+'; msg[j]=0;
				}
				if (key_config[i] & CTRL_RIGHT){
					msg[j++]=6; msg[j++]='+'; msg[j]=0;
				}
				if (key_config[i] & CTRL_DOWN){
					msg[j++]=7; msg[j++]='+'; msg[j]=0;
				}
				if (key_config[i] & CTRL_LEFT){
					msg[j++]=8; msg[j++]='+'; msg[j]=0;
				}
				msg[strlen(msg)-1]=0;
			}
			pgPrint(21,y,setting2.textcolor[3],msg);
		}
		
		if (crs_count < 15){
			x = 2;
			y = sel + 5;
			if(sel >= CONFIG_ANALOG2DPAD) y++;
			if(sel >= CONFIG_EXIT)        y++;
			pgPutChar((x+1)*8,y*8,setting2.textcolor[3],0,127,1,0,1);
		}
		
		pgScreenFlipV();
	}
	
	for(i=0; i<32; i++){
		if (i!=6 && key_config[i] == key_config[6])
			key_config[i] = 0;
		if(key_config[i]){
			setting.skeys[i].buttons = key_config[i];
			setting.skeys[i].n = i;
		}else{
			setting.skeys[i].buttons = 0;
			setting.skeys[i].n = -1;
		}
	}
	sort_skeys(&setting.skeys[6],0,25);
}

const char *gbtype_names[] = {
	"AUTO",
	"GB",
	"SGB",
	"GBC",
	"GBA",
};
int rin_gbtype(int n)
{
	int x,y,i,sel=n;
	
	for(;;){
		readpad();
		if(new_pad & CTRL_CROSS)
			return sel;
		else if(new_pad & CTRL_CIRCLE)
			return -1;
		else if(new_pad & CTRL_DOWN){
			sel++;
			if(sel>4) sel=0;
		}else if(new_pad & CTRL_UP){
			sel--;
			if(sel<0) sel=4;
		}else if(new_pad & CTRL_RIGHT){
			sel+=2;
			if(sel>4) sel=4;
		}else if(new_pad & CTRL_LEFT){
			sel-=2;
			if(sel<0) sel=0;
		}
		
		rin_frame("Select GB Type", "Å~ÅFOK  ÅõÅFCancel");
		
		x=4, y=5;
		pgPrint(x++,y++,setting2.textcolor[3],"GB TYPE:");
		for(i=0; i<=4; i++)
			pgPrint(x,y++,setting2.textcolor[i==sel?2:3],gbtype_names[i]);
		
		pgScreenFlipV();
	}
}

int rin_screensize(int n)
{
	int x,y,i,sel=n;
	
	for(;;){
		readpad();
		if(new_pad & CTRL_CROSS)
			return sel;
		else if(new_pad & CTRL_CIRCLE)
			return -1;
		else if(new_pad & CTRL_SELECT){
			setting.bScreenSizes[sel] = !setting.bScreenSizes[sel];
			for(i=0; i<SCR_END; i++)
				if(setting.bScreenSizes[i]) break;
			if(i>=SCR_END)
				setting.bScreenSizes[sel] = 1;
		}else if(new_pad & CTRL_DOWN){
			sel++;
			if(sel>=SCR_END) sel=0;
		}else if(new_pad & CTRL_UP){
			sel--;
			if(sel<0) sel=SCR_END-1;
		}else if(new_pad & CTRL_RIGHT){
			sel+=SCR_END/2;
			if(sel>=SCR_END) sel=SCR_END-1;
		}else if(new_pad & CTRL_LEFT){
			sel-=SCR_END/2;
			if(sel<0) sel=0;
		}
		
		if(setting.bScreenSizes[sel])
			rin_frame("Select Screen Size", "Å~ÅFOK  ÅõÅFCancel   SELECTÅFDisable");
		else
			rin_frame("Select Screen Size", "Å~ÅFOK  ÅõÅFCancel   SELECTÅFEnable");
		
		x=4, y=5;
		pgPrint(x++,y++,setting2.textcolor[3],"SCREEN SIZE:");
		for(i=0; i<SCR_END; i++){
			if(setting.bScreenSizes[i])
				pgPrint(x-2,y,setting2.textcolor[1],"+");
			pgPrint(x,y++,setting2.textcolor[i==sel?2:3],scr_names[i]);
		}
		
		pgScreenFlipV();
	}
}

int rin_gbcolor(int n)
{
	int x,y,i,sel=n;
	
	for(;;){
		readpad();
		if(new_pad & CTRL_CROSS)
			return sel;
		else if(new_pad & CTRL_CIRCLE)
			return -1;
		else if(new_pad & CTRL_SELECT){
			setting.bGB_Pals[sel] = !setting.bGB_Pals[sel];
			for(i=1; i<PAL_SGB; i++)
				if(setting.bGB_Pals[i]) break;
			if(i>=PAL_SGB)
				setting.bGB_Pals[sel] = 1;
		}else if(new_pad & CTRL_DOWN){
			sel++;
			if(sel>=PAL_SGB) sel=1;
		}else if(new_pad & CTRL_UP){
			sel--;
			if(sel<1) sel=PAL_SGB-1;
		}else if(new_pad & CTRL_RIGHT){
			sel+=(PAL_SGB-1)/2;
			if(sel>=PAL_SGB) sel=PAL_SGB-1;
		}else if(new_pad & CTRL_LEFT){
			sel-=(PAL_SGB-1)/2;
			if(sel<1) sel=1;
		}
		
		if(setting.bGB_Pals[sel])
			rin_frame("Select GB Palette", "Å~ÅFOK  ÅõÅFCancel   SELECT:Disable");
		else
			rin_frame("Select GB Palette", "Å~ÅFOK  ÅõÅFCancel   SELECT:Enable");
		
		x=4, y=5;
		pgPrint(x++,y++,setting2.textcolor[3],"GB PALETTE:");
		for(i=1; i<PAL_SGB; i++){
			if(setting.bGB_Pals[i])
				pgPrint(x-2,y,setting2.textcolor[1],"+");
			pgPrint(x,y++,setting2.textcolor[i==sel?2:3],pal_names[i]);
		}
		
		pgScreenFlipV();
	}
}

int rin_findState(int nState[], int nThumb[])
{
	char tmp[MAX_PATH], *p;
	int i, j;
	
	strcpy(tmp,SavePath);
	p = strrchr(tmp,'/') + 1;
	*p = 0;
	
	nfiles = 0;
	strcpy(path_files, tmp);
	int fd = sceIoDopen(tmp);
	while(nfiles<MAX_ENTRY){
		memset(&files[nfiles], 0x00, sizeof(SceIoDirent));
		if(sceIoDread(fd, &files[nfiles])<=0) break;
		nfiles++;
	}
	sceIoDclose(fd);
	
	for(i=0; i<=STATE_SLOT_MAX; i++){
		get_state_path(i,tmp);
		nState[i]=-1;
		for(j=0; j<nfiles; j++){
			if(!stricmp(p,files[j].name)){
				nState[i] = j;
				break;
			}
		}

		if (nState[i]<0){
			strcat(p, ".gz");
			for(j=0; j<nfiles; j++){
				if(!stricmp(p,files[j].name)){
					nState[i] = j;
					break;
				}
			}
		}
		
		get_thumb_path(i,tmp);
		nThumb[i]=-1;
		for(j=0; j<nfiles; j++){
			if(!stricmp(p,files[j].name)){
				nThumb[i] = j;
				break;
			}
		}
		if(nThumb[i]>=0)
			continue;

		*strrchr(tmp, '.') = 0;
		for(j=0; j<nfiles; j++){
			if(!stricmp(p,files[j].name)){
				nThumb[i] = j;
				break;
			}
		}
	}
}

enum
{
	RIN_STATE_SAVE,
	RIN_STATE_LOAD,
	RIN_QUICK_SLOT,
};
int rin_stateslot(int type)
{
	const int MAX_ITEM = STATE_SLOT_MAX+1;
	char msg[256], *p;
	static int ex_sel=0;
	int nState[STATE_SLOT_MAX+1], nThumb[STATE_SLOT_MAX+1], fd;
	int x, y, i, ret, sel=ex_sel, sel_bak=-1;

	rin_findState(nState, nThumb);

	for(;;){
		readpad();
		if(new_pad & CTRL_CROSS){
			if (type != RIN_STATE_LOAD)
				break;
			else{
				if(sel > STATE_SLOT_MAX){
					if(state_tmp)
						break;
				}else if (nState[sel]>=0)
					break;
			}
		}else if(new_pad & CTRL_CIRCLE){
			return -1;
		}else if((new_pad & CTRL_SELECT)){
			if (sel > STATE_SLOT_MAX){
				free(state_tmp);
				state_tmp = NULL;
			}
			else if (nState[sel]>=0){
				if (delete_state(sel)>=0)
					nState[sel] = nThumb[sel] = -1;
			}
		}else if(new_pad & CTRL_DOWN){
			sel++;
			if(sel>MAX_ITEM) sel=0;
		}else if(new_pad & CTRL_UP){
			sel--;
			if(sel<0) sel=MAX_ITEM;
		}else if(new_pad & CTRL_RIGHT){
			sel+=(MAX_ITEM+1)/2;
			if(sel>MAX_ITEM) sel=MAX_ITEM;
		}else if(new_pad & CTRL_LEFT){
			sel-=(MAX_ITEM+1)/2;
			if(sel<0) sel=0;
		}
		
		if (sel!=sel_bak){
			sel_bak = sel;
			if (sel > STATE_SLOT_MAX){
				if (state_tmp)
					memcpy(thumb_w,thumb_tmp,sizeof(thumb_w));
			}
			else if (nState[sel]>=0 && nThumb[sel]>=0){
				p = strrchr(files[nThumb[sel]].name, '.');
				if(!stricmp(p, ".png"))
					ret = load_thumb(sel,thumb_w,sizeof(thumb_w));
				else
					ret = load_thumb_old(sel,thumb_w,sizeof(thumb_w));
				if(!ret)
					nThumb[sel] = -1;
			}
		}
		
		switch(type)
		{
		case RIN_STATE_LOAD:
			p = "Select State Load Slot";
			break;
		case RIN_STATE_SAVE:
			p = "Select State Save Slot";
			break;
		case RIN_QUICK_SLOT:
			p = "Select Quick Slot";
			break;
		}
		rin_frame(p,"Å~ÅFOK  ÅõÅFCancel   SELECTÅFRemove");
		
		if ((sel>STATE_SLOT_MAX && state_tmp) ||
			(sel<=STATE_SLOT_MAX && nState[sel]>=0 && nThumb[sel]>=0)){
			pgBitBlt(272,50,160,144,1,thumb_w);
			pgDrawFrame(270,48,433,195,setting2.textcolor[1]);
			pgDrawFrame(271,49,432,194,setting2.textcolor[1]);
		}
		
		switch(type)
		{
		case RIN_STATE_LOAD:
			p = "STATE LOAD:";
			break;
		case RIN_STATE_SAVE:
			p = "STATE SAVE:";
			break;
		case RIN_QUICK_SLOT:
			p = "QUICK SLOT:";
			break;
		}
		x=4, y=5;
		pgPrint(x++,y++,setting2.textcolor[3],p);

		for(i=0; i<=STATE_SLOT_MAX; i++){
			if(nState[i] < 0){
				sprintf(msg,"%d - None", i);
			}else{
				sprintf(msg, "%d - %04d/%02d/%02d %02d:%02d:%02d", i,
					files[nState[i]].mtime.year,
					files[nState[i]].mtime.mon,
					files[nState[i]].mtime.mday,
					files[nState[i]].mtime.hour,
					files[nState[i]].mtime.min,
					files[nState[i]].mtime.sec);
			}
			pgPrint(x,y++,setting2.textcolor[i==sel?2:3],msg);
		}
		y++;
		if (state_tmp){
			sprintf(msg, "TMP:%04d/%02d/%02d %02d:%02d:%02d",
				state_tmp_time.year,
				state_tmp_time.mon,
				state_tmp_time.mday,
				state_tmp_time.hour,
				state_tmp_time.min,
				state_tmp_time.sec);
		}else
			strcpy(msg,"TMP:None");
		pgPrint(x,y++,setting2.textcolor[i==sel?2:3],msg);
		
		pgScreenFlipV();
	}

	ex_sel = sel;
	return sel;
}

void select_cheat(void);

void input_cheat(int input_type, char *last_name, long lPos)
{
	char name[66]={0};
	int pos=0,x,y,i;
	int mode=0;
	FILE *file;
	long lSize;
	
	strcpy(name, "");
	
	file = fopen(CheatPath, "rb");
	fseek (file , 0 , SEEK_END);
  lSize = ftell (file);
  rewind (file);
  fclose(file);
	
  //rewind (file);
  while(1)
  {
  	readpad();
		if(new_pad & CTRL_START){
			if(!input_type && strlen(name)!=0) {
				/*file = fopen(CheatPath, "wt+");
				fseek (file , lSize , SEEK_SET);
				strcat(name, "\n");
				fputs(name,file);
				fclose(file);*/
				input_cheat(1, name, 0);
				break;
			}else if(strlen(name)!=0){
				file = fopen(CheatPath, "wt+");
				if(!lPos)
					fseek (file , lSize , SEEK_SET);
				else
					fseek (file , lPos , SEEK_SET);
				strcat(last_name, "\n");
				fputs(last_name,file);
				if(!lPos)
					strcat(name, "\n");
				else
					strcat(name, "\n\n");
				fputs(name,file);
				fclose(file);
				select_cheat();
				break;
			}else if(strlen(name)==0){
				break;
			}
		}
		if(new_pad & CTRL_SELECT){
			mode++;
			if(mode>3)
				mode = 0;
		}
		if((old_pad & CTRL_LTRIGGER) && (old_pad & CTRL_RTRIGGER)){
			name[pos] = ' ';
		}
		if(mode==0){
		if(old_pad & CTRL_LTRIGGER){
			if(new_pad & CTRL_CROSS){
				if(strlen(name)==pos)
					strcat(name, "o");
				else
					name[pos] = 'o';
				pos++;
			}else if(new_pad & CTRL_CIRCLE){
				if(strlen(name)==pos)
					strcat(name, "p");
				else
					name[pos] = 'p';
				pos++;
			}else if(new_pad & CTRL_TRIANGLE){
				if(strlen(name)==pos)
					strcat(name, "q");
				else
					name[pos] = 'q';
				pos++;
			}else if(new_pad & CTRL_SQUARE){
				if(strlen(name)==pos)
					strcat(name, "r");
				else
					name[pos] = 'r';
				pos++;
			}else if(new_pad & CTRL_LEFT){
				if(strlen(name)==pos)
					strcat(name, "v");
				else
					name[pos] = 'v';
				pos++;
			}else if(new_pad & CTRL_RIGHT){
				if(strlen(name)==pos)
					strcat(name, "t");
				else
					name[pos] = 't';
				pos++;
			}else if(new_pad & CTRL_UP){
				if(strlen(name)==pos)
					strcat(name, "u");
				else
					name[pos] = 'u';
				pos++;
			}else if(new_pad & CTRL_DOWN){
				if(strlen(name)==pos)
					strcat(name, "s");
				else
					name[pos] = 's';
				pos++;
			}
		}else if(old_pad & CTRL_RTRIGGER){
			if(new_pad & CTRL_CROSS){
				if(strlen(name)==pos)
					strcat(name, "g");
				else
					name[pos] = 'g';
				pos++;
			}else if(new_pad & CTRL_CIRCLE){
				if(strlen(name)==pos)
					strcat(name, "h");
				else
					name[pos] = 'h';
				pos++;
			}else if(new_pad & CTRL_TRIANGLE){
				if(strlen(name)==pos)
					strcat(name, "i");
				else
					name[pos] = 'i';
				pos++;
			}else if(new_pad & CTRL_SQUARE){
				if(strlen(name)==pos)
					strcat(name, "j");
				else
					name[pos] = 'j';
				pos++;
			}else if(new_pad & CTRL_LEFT){
				if(strlen(name)==pos)
					strcat(name, "n");
				else
					name[pos] = 'n';
				pos++;
			}else if(new_pad & CTRL_RIGHT){
				if(strlen(name)==pos)
					strcat(name, "l");
				else
					name[pos] = 'l';
				pos++;
			}else if(new_pad & CTRL_UP){
				if(strlen(name)==pos)
					strcat(name, "m");
				else
					name[pos] = 'm';
				pos++;
			}else if(new_pad & CTRL_DOWN){
				if(strlen(name)==pos)
					strcat(name, "k");
				else
					name[pos] = 'k';
				pos++;
			}
		}else{
			if(new_pad & CTRL_CROSS){
				if(strlen(name)==pos)
					strcat(name, "a");
				else
					name[pos] = 'a';
				pos++;
			}else if(new_pad & CTRL_CIRCLE){
				if(strlen(name)==pos)
					strcat(name, "b");
				else
					name[pos] = 'b';
				pos++;
			}else if(new_pad & CTRL_TRIANGLE){
				if(strlen(name)==pos)
					strcat(name, "c");
				else
					name[pos] = 'c';
				pos++;
			}else if(new_pad & CTRL_SQUARE){
				if(strlen(name)==pos)
					strcat(name, "d");
				else
					name[pos] = 'd';
				pos++;
			}else if(new_pad & CTRL_LEFT){
				pos--;
			}else if(new_pad & CTRL_RIGHT){
				if(strlen(name)>0){
					if(strlen(name)==pos)
						strcat(name, " ");
					pos++;
				}
			}else if(new_pad & CTRL_UP){
				if(strlen(name)==pos)
					strcat(name, "f");
				else
					name[pos] = 'f';
				pos++;
			}else if(new_pad & CTRL_DOWN){
				if(strlen(name)==pos)
					strcat(name, "e");
				else
					name[pos] = 'e';
				pos++;
			}
		}
		}else if(mode==1){
		if(old_pad & CTRL_LTRIGGER){
			if(new_pad & CTRL_CROSS){
				if(strlen(name)==pos)
					strcat(name, "2");
				else
					name[pos] = '2';
				pos++;
			}else if(new_pad & CTRL_CIRCLE){
				if(strlen(name)==pos)
					strcat(name, "3");
				else
					name[pos] = '3';
				pos++;
			}else if(new_pad & CTRL_TRIANGLE){
				if(strlen(name)==pos)
					strcat(name, "4");
				else
					name[pos] = '4';
				pos++;
			}else if(new_pad & CTRL_SQUARE){
				if(strlen(name)==pos)
					strcat(name, "5");
				else
					name[pos] = '5';
				pos++;
			}else if(new_pad & CTRL_LEFT){
				if(strlen(name)==pos)
					strcat(name, "9");
				else
					name[pos] = '9';
				pos++;
			}else if(new_pad & CTRL_RIGHT){
				if(strlen(name)==pos)
					strcat(name, "7");
				else
					name[pos] = '7';
				pos++;
			}else if(new_pad & CTRL_UP){
				if(strlen(name)==pos)
					strcat(name, "8");
				else
					name[pos] = '8';
				pos++;
			}else if(new_pad & CTRL_DOWN){
				if(strlen(name)==pos)
					strcat(name, "6");
				else
					name[pos] = '6';
				pos++;
			}
			}else{
			if(new_pad & CTRL_CROSS){
				if(strlen(name)==pos)
					strcat(name, "w");
				else
					name[pos] = 'w';
				pos++;
			}else if(new_pad & CTRL_CIRCLE){
				if(strlen(name)==pos)
					strcat(name, "x");
				else
					name[pos] = 'x';
				pos++;
			}else if(new_pad & CTRL_TRIANGLE){
				if(strlen(name)==pos)
					strcat(name, "y");
				else
					name[pos] = 'y';
				pos++;
			}else if(new_pad & CTRL_SQUARE){
				if(strlen(name)==pos)
					strcat(name, "z");
				else
					name[pos] = 'z';
				pos++;
			}else if(new_pad & CTRL_LEFT){
				pos--;
			}else if(new_pad & CTRL_RIGHT){
				if(strlen(name)>0){
					if(strlen(name)==pos)
						strcat(name, " ");
					pos++;
				}
			}else if(new_pad & CTRL_UP){
				if(strlen(name)==pos)
					strcat(name, "1");
				else
					name[pos] = '1';
				pos++;
			}else if(new_pad & CTRL_DOWN){
				if(strlen(name)==pos)
					strcat(name, "0");
				else
					name[pos] = '0';
				pos++;
			}
			}
		}else if(mode==2){
		if(old_pad & CTRL_LTRIGGER){
			if(new_pad & CTRL_CROSS){
				if(strlen(name)==pos)
					strcat(name, "O");
				else
					name[pos] = 'O';
				pos++;
			}else if(new_pad & CTRL_CIRCLE){
				if(strlen(name)==pos)
					strcat(name, "P");
				else
					name[pos] = 'P';
				pos++;
			}else if(new_pad & CTRL_TRIANGLE){
				if(strlen(name)==pos)
					strcat(name, "Q");
				else
					name[pos] = 'Q';
				pos++;
			}else if(new_pad & CTRL_SQUARE){
				if(strlen(name)==pos)
					strcat(name, "R");
				else
					name[pos] = 'R';
				pos++;
			}else if(new_pad & CTRL_LEFT){
				if(strlen(name)==pos)
					strcat(name, "V");
				else
					name[pos] = 'V';
				pos++;
			}else if(new_pad & CTRL_RIGHT){
				if(strlen(name)==pos)
					strcat(name, "T");
				else
					name[pos] = 'T';
				pos++;
			}else if(new_pad & CTRL_UP){
				if(strlen(name)==pos)
					strcat(name, "U");
				else
					name[pos] = 'U';
				pos++;
			}else if(new_pad & CTRL_DOWN){
				if(strlen(name)==pos)
					strcat(name, "S");
				else
					name[pos] = 'S';
				pos++;
			}
		}else if(old_pad & CTRL_RTRIGGER){
			if(new_pad & CTRL_CROSS){
				if(strlen(name)==pos)
					strcat(name, "G");
				else
					name[pos] = 'G';
				pos++;
			}else if(new_pad & CTRL_CIRCLE){
				if(strlen(name)==pos)
					strcat(name, "H");
				else
					name[pos] = 'H';
				pos++;
			}else if(new_pad & CTRL_TRIANGLE){
				if(strlen(name)==pos)
					strcat(name, "I");
				else
					name[pos] = 'I';
				pos++;
			}else if(new_pad & CTRL_SQUARE){
				if(strlen(name)==pos)
					strcat(name, "J");
				else
					name[pos] = 'J';
				pos++;
			}else if(new_pad & CTRL_LEFT){
				if(strlen(name)==pos)
					strcat(name, "N");
				else
					name[pos] = 'N';
				pos++;
			}else if(new_pad & CTRL_RIGHT){
				if(strlen(name)==pos)
					strcat(name, "L");
				else
					name[pos] = 'L';
				pos++;
			}else if(new_pad & CTRL_UP){
				if(strlen(name)==pos)
					strcat(name, "M");
				else
					name[pos] = 'M';
				pos++;
			}else if(new_pad & CTRL_DOWN){
				if(strlen(name)==pos)
					strcat(name, "K");
				else
					name[pos] = 'K';
				pos++;
			}
		}else{
			if(new_pad & CTRL_CROSS){
				if(strlen(name)==pos)
					strcat(name, "A");
				else
					name[pos] = 'A';
				pos++;
			}else if(new_pad & CTRL_CIRCLE){
				if(strlen(name)==pos)
					strcat(name, "B");
				else
					name[pos] = 'B';
				pos++;
			}else if(new_pad & CTRL_TRIANGLE){
				if(strlen(name)==pos)
					strcat(name, "C");
				else
					name[pos] = 'C';
				pos++;
			}else if(new_pad & CTRL_SQUARE){
				if(strlen(name)==pos)
					strcat(name, "D");
				else
					name[pos] = 'D';
				pos++;
			}else if(new_pad & CTRL_LEFT){
				pos--;
			}else if(new_pad & CTRL_RIGHT){
				if(strlen(name)>0){
					if(strlen(name)==pos)
						strcat(name, " ");
					pos++;
				}
			}else if(new_pad & CTRL_UP){
				if(strlen(name)==pos)
					strcat(name, "F");
				else
					name[pos] = 'F';
				pos++;
			}else if(new_pad & CTRL_DOWN){
				if(strlen(name)==pos)
					strcat(name, "E");
				else
					name[pos] = 'E';
				pos++;
			}
		}
		}else if(mode==3){
		if(old_pad & CTRL_LTRIGGER){
			if(new_pad & CTRL_CROSS){
				if(strlen(name)==pos)
					strcat(name, ")");
				else
					name[pos] = ')';
				pos++;
			}else if(new_pad & CTRL_CIRCLE){
				if(strlen(name)==pos)
					strcat(name, "(");
				else
					name[pos] = '(';
				pos++;
			}else if(new_pad & CTRL_TRIANGLE){
				if(strlen(name)==pos)
					strcat(name, "]");
				else
					name[pos] = ']';
				pos++;
			}else if(new_pad & CTRL_SQUARE){
				if(strlen(name)==pos)
					strcat(name, "[");
				else
					name[pos] = '[';
				pos++;
			}else if(new_pad & CTRL_LEFT){
				if(strlen(name)==pos)
					strcat(name, "!");
				else
					name[pos] = '!';
				pos++;
			}else if(new_pad & CTRL_RIGHT){
				if(strlen(name)==pos)
					strcat(name, "-");
				else
					name[pos] = '-';
				pos++;
			}else if(new_pad & CTRL_UP){
				if(strlen(name)==pos)
					strcat(name, "?");
				else
					name[pos] = '?';
				pos++;
			}else if(new_pad & CTRL_DOWN){
				if(strlen(name)==pos)
					strcat(name, "_");
				else
					name[pos] = '_';
				pos++;
			}
			}else{
			if(new_pad & CTRL_CROSS){
				if(strlen(name)==pos)
					strcat(name, "W");
				else
					name[pos] = 'W';
				pos++;
			}else if(new_pad & CTRL_CIRCLE){
				if(strlen(name)==pos)
					strcat(name, "X");
				else
					name[pos] = 'X';
				pos++;
			}else if(new_pad & CTRL_TRIANGLE){
				if(strlen(name)==pos)
					strcat(name, "Y");
				else
					name[pos] = 'Y';
				pos++;
			}else if(new_pad & CTRL_SQUARE){
				if(strlen(name)==pos)
					strcat(name, "Z");
				else
					name[pos] = 'Z';
				pos++;
			}else if(new_pad & CTRL_LEFT){
				pos--;
			}else if(new_pad & CTRL_RIGHT){
				if(strlen(name)>0){
					if(strlen(name)==pos)
						strcat(name, " ");
					pos++;
				}
			}else if(new_pad & CTRL_UP){
				if(strlen(name)==pos)
					strcat(name, ",");
				else
					name[pos] = ',';
				pos++;
			}else if(new_pad & CTRL_DOWN){
				if(strlen(name)==pos)
					strcat(name, ".");
				else
					name[pos] = '.';
				pos++;
			}
			}
		}
		if(strlen(last_name)>0){
			if(pos>6)
				pos=7;
		}else{
			if(pos>45)
				pos=46;
		}
		if(pos<0)
			pos=0;
			
		rin_frame("","STARTÅFAccept  SELECTÅFMode");
		
		x=4;
		y=4;
		
		pgPrint(x-2, y-2, 0xffff, "D-Pad");
		if(mode==0){
		if(old_pad & CTRL_LTRIGGER) {
			//TRIANGLE
			pgPrint(x+10,y-1, 0xffff, "q");
			//SQUARE
			pgPrint(x-1+10,y, 0xffff, "r");
			//CROSS
			pgPrint(x+10,y+1, 0xffff, "o");
			//CIRCLE
			pgPrint(x+1+10,y, 0xffff, "p");
			
			//UP
			pgPrint(x,y-1, 0xffff, "u");
			//LEFT
			pgPrint(x-1,y, 0xffff, "v");
			//DOWN
			pgPrint(x,y+1, 0xffff, "s");
			//RIGHT
			pgPrint(x+1,y, 0xffff, "t");
		}else if(old_pad & CTRL_RTRIGGER) {
			//TRIANGLE
			pgPrint(x+10,y-1, 0xffff, "i");
			//SQUARE
			pgPrint(x-1+10,y, 0xffff, "j");
			//CROSS
			pgPrint(x+10,y+1, 0xffff, "g");
			//CIRCLE
			pgPrint(x+1+10,y, 0xffff, "h");
			
			//UP
			pgPrint(x,y-1, 0xffff, "m");
			//LEFT
			pgPrint(x-1,y, 0xffff, "n");
			//DOWN
			pgPrint(x,y+1, 0xffff, "k");
			//RIGHT
			pgPrint(x+1,y, 0xffff, "l");
		}else {
			//TRIANGLE
			pgPrint(x+10,y-1, 0xffff, "c");
			//SQUARE
			pgPrint(x-1+10,y, 0xffff, "d");
			//CROSS
			pgPrint(x+10,y+1, 0xffff, "a");
			//CIRCLE
			pgPrint(x+1+10,y, 0xffff, "b");
			
			//UP
			pgPrint(x,y-1, 0xffff, "f");
			//DOWN
			pgPrint(x,y+1, 0xffff, "e");
		}
		}else if(mode==1){
			if(old_pad & CTRL_LTRIGGER) {
			//TRIANGLE
			pgPrint(x+10,y-1, 0xffff, "4");
			//SQUARE
			pgPrint(x-1+10,y, 0xffff, "5");
			//CROSS
			pgPrint(x+10,y+1, 0xffff, "2");
			//CIRCLE
			pgPrint(x+1+10,y, 0xffff, "3");
			//UP
			pgPrint(x,y-1, 0xffff, "8");
			//LEFT
			pgPrint(x-1,y, 0xffff, "9");
			//DOWN
			pgPrint(x,y+1, 0xffff, "6");
			//RIGHT
			pgPrint(x+1,y, 0xffff, "7");
			}else{
			//TRIANGLE
			pgPrint(x+10,y-1, 0xffff, "y");
			//SQUARE
			pgPrint(x-1+10,y, 0xffff, "z");
			//CROSS
			pgPrint(x+10,y+1, 0xffff, "w");
			//CIRCLE
			pgPrint(x+1+10,y, 0xffff, "x");
			//UP
			pgPrint(x,y-1, 0xffff, "1");
			//DOWN
			pgPrint(x,y+1, 0xffff, "0");
			}
		}else if(mode==2){
		if(old_pad & CTRL_LTRIGGER) {
			//TRIANGLE
			pgPrint(x+10,y-1, 0xffff, "Q");
			//SQUARE
			pgPrint(x-1+10,y, 0xffff, "R");
			//CROSS
			pgPrint(x+10,y+1, 0xffff, "O");
			//CIRCLE
			pgPrint(x+1+10,y, 0xffff, "P");
			
			//UP
			pgPrint(x,y-1, 0xffff, "U");
			//LEFT
			pgPrint(x-1,y, 0xffff, "T");
			//DOWN
			pgPrint(x,y+1, 0xffff, "V");
			//RIGHT
			pgPrint(x+1,y, 0xffff, "S");
		}else if(old_pad & CTRL_RTRIGGER) {
			//TRIANGLE
			pgPrint(x+10,y-1, 0xffff, "I");
			//SQUARE
			pgPrint(x-1+10,y, 0xffff, "J");
			//CROSS
			pgPrint(x+10,y+1, 0xffff, "G");
			//CIRCLE
			pgPrint(x+1+10,y, 0xffff, "H");
			
			//UP
			pgPrint(x,y-1, 0xffff, "M");
			//LEFT
			pgPrint(x-1,y, 0xffff, "L");
			//DOWN
			pgPrint(x,y+1, 0xffff, "N");
			//RIGHT
			pgPrint(x+1,y, 0xffff, "K");
		}else {
			//TRIANGLE
			pgPrint(x+10,y-1, 0xffff, "C");
			//SQUARE
			pgPrint(x-1+10,y, 0xffff, "D");
			//CROSS
			pgPrint(x+10,y+1, 0xffff, "A");
			//CIRCLE
			pgPrint(x+1+10,y, 0xffff, "B");
			
			//UP
			pgPrint(x,y-1, 0xffff, "F");
			//DOWN
			pgPrint(x,y+1, 0xffff, "E");
		}
		}else if(mode==3){
			if(old_pad & CTRL_LTRIGGER) {
			//TRIANGLE
			pgPrint(x+10,y-1, 0xffff, "]");
			//SQUARE
			pgPrint(x-1+10,y, 0xffff, "[");
			//CROSS
			pgPrint(x+10,y+1, 0xffff, ")");
			//CIRCLE
			pgPrint(x+1+10,y, 0xffff, "(");
			//UP
			pgPrint(x,y-1, 0xffff, "?");
			//LEFT
			pgPrint(x-1,y, 0xffff, "!");
			//DOWN
			pgPrint(x,y+1, 0xffff, "_");
			//RIGHT
			pgPrint(x+1,y, 0xffff, "-");
			}else{
			//TRIANGLE
			pgPrint(x+10,y-1, 0xffff, "Y");
			//SQUARE
			pgPrint(x-1+10,y, 0xffff, "Z");
			//CROSS
			pgPrint(x+10,y+1, 0xffff, "W");
			//CIRCLE
			pgPrint(x+1+10,y, 0xffff, "X");
			//UP
			pgPrint(x,y-1, 0xffff, ",");
			//DOWN
			pgPrint(x,y+1, 0xffff, ".");
			}
		}
			
		if(strlen(last_name)!=0){
			pgPrint(x-1, y+3, 0xffff, "Cheat name:");
			pgPrint(x, y+4, 0xffff, last_name);
			pgPrint(x-1, y+6, 0xffff, "Cheat code:");
			pgPrint(x, y+7, 0xffff, name);
			pgPrint(pos+x, y+7, 0xffff, "_");
		}else{
			pgPrint(x-1, y+3, 0xffff, "Cheat name:");
			pgPrint(x, y+4, 0xffff, name);
			pgPrint(pos+x, y+4, 0xffff, "_");
			pgPrint(x-1, y+6, 0xffff, "Cheat code:");
		}
		
		pgScreenFlipV();
	}
	fclose(file);
}

void select_cheat(void)
{
	char data[512], name[256];
	static int sel=0;
	int top, rows=21, x, y, h, i, fd;
	FILE *fp;
	long lSize;
	
	fp = fopen(CheatPath,"r");
	if (fp)
		cheat_load(fp);
		
	fclose(fp);
	
	fp = fopen(CheatPath, "rb");
	fseek (fp , 0 , SEEK_END);
  lSize = ftell (fp);
  rewind (fp);
  fclose(fp);
	
	cheat_decreate_cheat_map();

	for(;;){
		readpad();
		if(new_pad & CTRL_CROSS){
			strcpy(name, cheats[sel].name);
			for(i=0; i<nCheats; i++)
				if(!strcmp(name, cheats[i].name)) cheats[i].enable = !cheats[i].enable;
			
		}else if(new_pad & CTRL_SQUARE){
			cheats[0].enable = !cheats[0].enable;
			for (i=0; i<nCheats; i++)
				cheats[i].enable = cheats[0].enable;
		}else if(new_pad & CTRL_TRIANGLE){
			input_cheat(0, "", 0);
			break;
		}else if(new_pad & CTRL_CIRCLE){
			break;
		}else if(new_pad & CTRL_UP){
			sel--;
		}else if(new_pad & CTRL_DOWN){
			sel++;
		}else if(new_pad & CTRL_LEFT){
			sel-=rows/2;
		}else if(new_pad & CTRL_RIGHT){
			sel+=rows/2;
		}else if(new_pad & CTRL_SELECT){
			
			int i;
			
			for(i=sel;i<nCheats;i++){
				if(i+1<nCheats) {
					strcpy(cheats[i].name, cheats[i+1].name);
					cheats[i].enable = cheats[i+1].enable;
					cheats[i].code = cheats[i+1].code;
					cheats[i].adr = cheats[i+1].adr;
					cheats[i].dat = cheats[i+1].dat;
					cheats[i].dat_old = cheats[i+1].dat_old;
				}
			}
			nCheats--;
			sceIoRemove(CheatPath);
			fp=fopen(CheatPath, "wt+");
			fputs(&cheats[0], fp);
			fclose(fp);
		}
		
		if(top > nCheats-rows)	top=nCheats-rows;
		if(top < 0)				top=0;
		if(sel >= nCheats)		sel=nCheats-1;
		if(sel < 0)				sel=0;
		if(sel >= top+rows)		top=sel-rows+1;
		if(sel < top)			top=sel;
		
		rin_frame("","Å~ÅFOK  ÅõÅFReturn  Å†ÅFAll  SELECTÅFDelete");
		
		// ÉXÉNÉçÅ[ÉãÉoÅ[
		if(nCheats > rows){
			h = 219;
			pgDrawFrame(445,25,446,248,setting2.textcolor[1]);
			pgFillBox(448, h*top/nCheats + 27,
				460, h*(top+rows)/nCheats + 27,setting2.textcolor[1]);
		}
		
		x=30; y=32;
		for (i=0; i<rows; i++){
			if (top+i >= nCheats) break;
			mh_print(x, y, cheats[top+i].name, setting2.textcolor[top+i==sel?2:3]);
			mh_print(x-10, y, "Å†Å", setting2.textcolor[3]);
			if (cheats[top+i].enable)
				mh_print(x-11, y, "Å~Å", setting2.textcolor[3]);
			y+=10;
		}
		
		pgScreenFlipV();
	}

	cheat_create_cheat_map();
	return;
}

int rin_screenshot()
{
	char path[MAX_PATH], *p, ext[MAX_PATH], mess[512];
	int fd, i=1, a=0;
	while(1)
	{
		if(i>=9 && a>=9) {
			i=9;
			a=9;
			strcpy(ext, "99.png");
			strcpy(path, ScreenPath);
			strcat(path, RomName);
			strcat(path, ext);
			sceIoRemove(path);
			break;
		}
		strcpy(ext, "00.png");
		ext[0]=a+'0';
		ext[1]=i+'0';
			
		strcpy(path, ScreenPath);
		strcat(path, RomName);
		sceIoMkdir(path, 0777);
		strcat(path, "/");
		strcat(path, "Screen");
		strcat(path, ext);
		fd=sceIoOpen(path, SCE_O_RDONLY, 0777);
		if(fd<0){
			sceIoClose(fd);
			break;
		}
		sceIoClose(fd);
		i++;
		if(i>=10){
			a++;
			i=0;
		}
	}

	byte *buf = (byte*)malloc(144*160*3);
	if (!buf)
		return 0;

	byte r, g, b;
	unsigned short color;
	unsigned int x,y;
	for(y=0; y<144; y++){
		for(x=0; x<160; x++){
			color = vframe[y*SIZE_LINE+GUARD_LINE+x];
			r = (color      ) & 0x1F;
			g = (color >>  5) & 0x1F;
			b = (color >> 10) & 0x1F;
			buf[(y*160+x)*3  ] = r * 0xFF / 0x1F;
			buf[(y*160+x)*3+1] = g * 0xFF / 0x1F;
			buf[(y*160+x)*3+2] = b * 0xFF / 0x1F;
		}
	}

	int ret = write_png( path, buf, 160, 144 );
	free(buf);
	sceKernelDelayThread(1000000);
	strcpy(mess, "Saved as Screen");
	strcat(mess, ext);
	renderer_set_msg(mess);
	return ret;
}

unsigned short imgBitmap[1024*768];

int image_viewer(char *filepath)
{
	int i, disp_choices=0, width, height, mode=0;
	char msg[256], *p;
	
	p = strrchr(filepath, '.');
	if(!stricmp(p+1, "jpg") || !stricmp(p+1, "jpeg")){
		jpg_shrink=1;
		LoadJPEG(filepath, imgBitmap);
		width = jpginfo.output_width;
		height = jpginfo.output_height;
	}else if(!stricmp(p+1, "png")){
		Image *image = LoadGFX(filepath);
		width = image->imageWidth;
		height = image->imageHeight;
		read_png(filepath,imgBitmap,sizeof(imgBitmap));
	}else if(!stricmp(p+1, "tga")){
		width = 480;
		height = 272;
		loadImageNoAlphaTGA(filepath, width, height, imgBitmap);
	}else
		return 0;
	strcpy(msg, filepath);
	for(;;){
		readpad();
		if(new_pad & CTRL_CIRCLE){
			return 0;
		}else if(new_pad & CTRL_RTRIGGER){
			mode = !mode;
		}else if(new_pad & CTRL_TRIANGLE){
			disp_choices=!disp_choices;
		}
		if(!mode){
			if(new_pad & CTRL_CROSS){
				if(!stricmp(p+1, "jpg") || !stricmp(p+1, "jpeg")){
					jpg_shrink = !jpg_shrink;
					LoadJPEG(filepath, imgBitmap);
					width = jpginfo.output_width;
					height = jpginfo.output_height;
				}
			}
		}else if(mode){
			if(new_pad & CTRL_CROSS){
				strcpy(setting2.pngpath, filepath);
				setting2.jpg_shrink = jpg_shrink;
				load_menu_bg();
				save_config_glob();
				load_menu_bg();
				strcpy(msg,filepath);
				strcat(msg, " is set as wallpaper");
			}else if(new_pad & CTRL_SQUARE){
				setting.frame=1;
				strcpy(setting2.framepath, filepath);
				setting2.jpg_shrink_fr = jpg_shrink;
				save_config_glob();
				load_frame_bg();
				strcpy(msg,filepath);
				strcat(msg, " is set as frame");
			}
		}
		pgFillvram(0);
		if(imgBitmap)
			pgBitBlt(0,0,width,height,1,imgBitmap);
						
		if(disp_choices){
			if(msg!=0) mh_print(17, 14, msg, setting2.textcolor[1]);
			if(mode)
				mh_print(17, 252, "Å~ÅFBackground  ÅõÅFCancel Å@Å†ÅFFrame", setting2.textcolor[1]);
			else
				mh_print(17, 252, "Å~ÅFFit  ÅõÅFCancel", setting2.textcolor[1]);
		}
		
		pgScreenFlipV();
	 }
}

// by kwn
int load_frame_bg()
{
	char *p, tmp[MAX_PATH];
	if(!setting.frame || !rom_get_loaded()) return 0;
	if(!setting2.framepath) return 0;
	p = strrchr(setting2.framepath, '.');
	if(!stricmp(p+1, "jpg") || !stricmp(p+1, "jpeg")){
		jpg_shrink = setting2.jpg_shrink_fr;
		LoadJPEG(setting2.framepath, frBitmap);
		fr_width = jpginfo.output_width;
		fr_height = jpginfo.output_height;
	}else if(!stricmp(p+1, "png")){
		Image *image = LoadGFX(setting2.framepath);
		fr_width = image->imageWidth;
		fr_height = image->imageHeight;
		read_png(setting2.framepath,frBitmap,sizeof(frBitmap));
	}else if(!stricmp(p+1, "tga")){
		fr_width = 480;
		fr_height = 272;
		loadImageNoAlphaTGA(setting2.framepath, fr_width, fr_height, frBitmap);
	}
	return 0;
}

// by kwn
int load_menu_bg()
{
	char path[MAX_PATH], *p;

	p = strrchr(setting2.pngpath, '.');
	if(!stricmp(p+1, "jpg") || !stricmp(p+1, "jpeg")){
		jpg_shrink = setting2.jpg_shrink;
		LoadJPEG(setting2.pngpath, bgBitmap);
		bg_width = jpginfo.output_width;
		bg_height = jpginfo.output_height;
	}else if(!stricmp(p+1, "png")){
		Image *image = LoadGFX(setting2.pngpath);
		bg_width = image->imageWidth;
		bg_height = image->imageHeight;
		read_png(setting2.pngpath,bgBitmap,sizeof(bgBitmap));
	}else if(!stricmp(p+1, "tga")){
		bg_width = 480;
		bg_height = 272;
		loadImageNoAlphaTGA(setting2.pngpath, bg_width, bg_height, bgBitmap);
	}
	return 1;
}

#define TEXT_BYTES 4096
#define MAX_LINES 27

int text_reader(char *filepath)
{
	int fd, i, a=0, lines=0, current_buffer=0, old_buffer=0, tmp_buffer=0, data[999], cur_row=0;
	char text[TEXT_BYTES+1]="\0", text_tmp[TEXT_BYTES+1];
	fd = sceIoOpen(filepath, SCE_O_RDONLY, 0777);
}

const char *cpu_clocks[] = {
	"44MHz",
	"111MHz",
	"155MHz",
	"222MHz (default)",
	"266MHz",
	"333MHz",
};

const char *sound_buffers[] = {
	"FAST",
	"NORMAL",
	"SLOW",
};

void rin_menu(void)
{
	enum
	{
		MUSIC_PLAYER,
		STATE_SAVE,
		STATE_LOAD,
		QUICK_SLOT,
		SAVE_THUMB,
		COMPRESS,
		SCREEN_SIZE,
		GB_TYPE,
		GB_PALETTE,
		GB_FRAME,
		TURBO,
		VSYNC,
		SOUND,
		SOUND_BUFFER,
		MAX_FRAME_SKIP,
		CPU_CLOCK,
		COLOR_CONFIG,
		KEY_CONFIG,
		LOAD_CHEAT,
		SELECT_CHEAT,
		LOAD_ROM,
		RESET,
		CONTINUE,
		EXIT,
	};
	char imgpath_tmp[MAX_PATH];
	char tmp[MAX_PATH], path[MAX_PATH], msg[256]={0}, battery[32]={0};
	static int sel=0;
	int x, y, ret, crs_count=0, bLoop=1;
	int fd, romsize, ramsize;
	char *p;
	
	old_pad = 0;
	readpad();
	old_pad = paddata.buttons;
	
	for(;;){
		readpad();
		if(now_pad & (CTRL_UP|CTRL_DOWN|CTRL_LEFT|CTRL_RIGHT))
			crs_count=0;
		if(new_pad)
			msg[0]=0;
		if(new_pad & CTRL_CROSS){
			switch(sel)
			{
			case MUSIC_PLAYER:
				setting.sound=2;
				bLoop=0;
				bMusic=1;
				break;
			case STATE_SAVE:
				ret = rin_stateslot(RIN_STATE_SAVE);
				if(ret>=0){
					strcpy(msg, "State Save Failed");
					if(ret>STATE_SLOT_MAX){
						free(state_tmp);
						state_tmp = save_state_tmp();
						if(state_tmp)
							strcpy(msg, "State Saved Successfully");
					}else{
						if(save_state(ret))
							strcpy(msg, "State Saved Successfully");
					}
				}
				crs_count=0;
				break;
			case STATE_LOAD:
				ret = rin_stateslot(RIN_STATE_LOAD);
				if(ret>=0){
					strcpy(msg, "State Load Failed");
					if(ret>STATE_SLOT_MAX){
						if(load_state_tmp(state_tmp))
//							strcpy(msg, "State Loaded Successfully");
							bLoop = 0;
					}else{
						if(load_state(ret))
//							strcpy(msg, "State Loaded Successfully");
							bLoop = 0;
					}
				}
				crs_count=0;
				break;
			case SAVE_THUMB:
				setting.thumb = !setting.thumb;
				break;
			case COMPRESS:
				setting.compress = !setting.compress;
				break;
			case QUICK_SLOT:
				if (++setting.quickslot>STATE_SLOT_MAX+1) setting.quickslot=0;
				break;
			case SCREEN_SIZE:
				setting.screensize++;
				if(setting.screensize>=SCR_END)
					setting.screensize=0;
				crs_count=0;
				break;
			case MAX_FRAME_SKIP:
				setting.frameskip++;
				if(setting.frameskip>9)
					setting.frameskip=0;
				crs_count=0;
				break;
			case TURBO:
				bTurbo = !bTurbo;
				break;
			case SOUND:
				setting.sound++;
				if(setting.sound>2) setting.sound=0;
				if(setting.sound==0) {
					mp3_pause(1);
					OGG_Pause(1);
				}
				break;
			case VSYNC:
				setting.vsync = !setting.vsync;
				break;
			case GB_TYPE:
				ret = rin_gbtype(setting.gb_type);
				if(ret>=0){
					setting.gb_type = ret;
					gb_reset();
					bTurbo = 0;
				}
				crs_count=0;
				break;
			case GB_PALETTE:
				ret = rin_gbcolor(setting.gb_palette);
				if(ret>=0){
					setting.gb_palette = ret;
					if(rom_get_info()->gb_type!=1 && now_gb_mode==1)
						lcd_set_mpal(setting.gb_palette);
				}
				crs_count=0;
				break;
			case GB_FRAME:
				setting.frame = !setting.frame;
				break;
			case SOUND_BUFFER:
				setting.sound_buffer++;
				if(setting.sound_buffer>2)
					setting.sound_buffer=0;
				break;
			case CPU_CLOCK:
				setting.cpu_clock++;
				if(setting.cpu_clock>5)
					setting.cpu_clock=0;
				break;
			case COLOR_CONFIG:
				rin_colorconfig();
				crs_count=0;
				break;
			case KEY_CONFIG:
				rin_keyconfig();
				crs_count=0;
				break;
			case LOAD_CHEAT: 
				if(getFilePath(CheatPath,EXT_TCH)){
					strcpy(msg, "Cheat Load Failed");
					FILE *fp = fopen(CheatPath,"r");
					if (fp){
						if (cheat_load(fp))
							strcpy(msg, "Cheat Loaded Successfully");
						fclose(fp);
					}
				}
				crs_count=0;
				break;
			case SELECT_CHEAT:
				if (nCheats>0){
					select_cheat();
					crs_count=0;
				}
				break;
			case LOAD_ROM:
				for (;;){
					if (!getFilePath(RomPath, EXT_GB|EXT_GZ|EXT_ZIP)){
						if (bLoop)
							break;
						else
							continue;
					}
					strcpy(tmp, RomPath);
					*(strrchr(tmp,'/')+1) = 0;
					strcpy(setting2.lastpath, tmp);
					
					bTurbo = 0;
					bLoop = 0;
					
					if (rom_get_loaded() && rom_has_battery())
						save_sram(get_sram(), rom_get_info()->ram_size);

					gb_init();
					
					// éwíËÇµÇΩÉtÉ@ÉCÉãÇÉçÅ[ÉhÇ∑ÇÈÅB by ruka
					romsize = load_rom(RomPath);
					if (!romsize){
						strcpy(filer_msg,"ROM Load Failed");
						continue;
					}

					ramsize = load_sram();
					if (!gb_load_rom(rom_image, romsize, sram_space, ramsize)){
						strcpy(filer_msg,"ROM Load Failed");
						continue;
					}
					load_config();
					if(org_gbtype==1)
						renderer_set_msg("ROM TYPE:GB");
					else if(org_gbtype==2)
						renderer_set_msg("ROM TYPE:SGB");
					else if(org_gbtype==3)
						renderer_set_msg("ROM TYPE:GBC");
					
					if(setting.sound==1) 
						wavout_enable=1;
					else
						wavout_enable=0;
					
					free(state_tmp);
					state_tmp = NULL;
					
					if(mp3_playing() | OGG_Playing()) setting.sound=2; 
					break;
				}
				crs_count=0;
				break;
			case RESET:
				gb_reset();
				bTurbo = 0;
				bLoop = 0;
				break;
			case CONTINUE:
				bLoop = 0;
				
				break;
			case EXIT:
				gb_reset();
				exit_callback();
				break;
			}
		}else if(new_pad & CTRL_SQUARE){
			if (sel==QUICK_SLOT){
				if (setting.quickslot>0)
					setting.quickslot--;
				else
					setting.quickslot=STATE_SLOT_MAX+1;
			}
		}else if(new_pad & CTRL_CIRCLE){
			bLoop = 0;
		}else if(new_pad & CTRL_UP){
			if(sel!=0) sel--;
			else       sel=EXIT;
			continue;
		}else if(new_pad & CTRL_DOWN){
			if(sel!=EXIT)	sel++;
			else				sel=0;
			continue;
		}else if(new_pad & CTRL_LEFT){
			if(sel>LOAD_CHEAT)
				sel=LOAD_CHEAT;
			else if(sel>KEY_CONFIG)
				sel=SCREEN_SIZE;
			else if(sel>0)
				sel=0;
			continue;
		}else if(new_pad & CTRL_RIGHT){
			if(sel<SCREEN_SIZE)
				sel=SCREEN_SIZE;
			else if(sel<LOAD_CHEAT)
				sel=LOAD_CHEAT;
			else if(sel<LOAD_ROM)
				sel=LOAD_ROM;
			continue;
		}else if(get_nShortcutKey(new_pad)==6){
			bLoop = 0;
			break;
		}
		
		if(!bLoop) break;
		if (crs_count++>=30) crs_count=0;
		
		rin_frame(msg, "Å~ÅFOK  ÅõÅFContinue  MenuBTNÅFContinue");
		x = 4;
		y = 5;
		
		pgPrintf(x,y++,setting2.textcolor[3],"Music Player");
		pgPrintf(x,y++,setting2.textcolor[3],"State Save");
		pgPrintf(x,y++,setting2.textcolor[3],"State Load");
		if (setting.quickslot > STATE_SLOT_MAX)
			pgPrintf(x,y++,setting2.textcolor[3],"Quick Slot    : Temp");
		else
			pgPrintf(x,y++,setting2.textcolor[3],"Quick Slot    : %d",setting.quickslot);
		pgPrintf(x,y++,setting2.textcolor[3],"Save Thumbnail: %s",setting.thumb?"On":"Off");
		pgPrintf(x,y++,setting2.textcolor[3],"Compress File : %s",setting.compress?"On":"Off");
		y++;
		pgPrintf(x,y++,setting2.textcolor[3],"Screen Size   : %s",scr_names[setting.screensize]);
		pgPrintf(x,y++,setting2.textcolor[3],"GB Type       : %s",gbtype_names[setting.gb_type]);
		pgPrintf(x,y++,setting2.textcolor[3],"GB Palette    : %s",pal_names[setting.gb_palette]);
		pgPrintf(x,y++,setting2.textcolor[3],"GB Frame      : %s",setting.frame?"On":"Off");
		pgPrintf(x,y++,setting2.textcolor[3],"Turbo         : %s",bTurbo?"On":"Off");
		pgPrintf(x,y++,setting2.textcolor[3],"VSYNC         : %s",setting.vsync?"On":"Off");
		pgPrintf(x,y++,setting2.textcolor[3],"Sound         : %s",setting.sound==0?"Off":setting.sound==1?"On":"Mp3");
			
		pgPrintf(x,y++,setting2.textcolor[3],"Sound Buffer  : %s",sound_buffers[setting.sound_buffer]);
		pgPrintf(x,y++,setting2.textcolor[3],"Max Frame Skip: %d",setting.frameskip);
		pgPrintf(x,y++,setting2.textcolor[3],"CPU Clock     : %s",cpu_clocks[setting.cpu_clock]);
		pgPrintf(x,y++,setting2.textcolor[3],"Menu Color Config");
		pgPrintf(x,y++,setting2.textcolor[3],"Key Config");
		y++;
		pgPrintf(x,y++,setting2.textcolor[3],"Load Cheat File");
		pgPrintf(x,y++,setting2.textcolor[nCheats>0?3:2],"Select Cheatcode");
		y++;
		pgPrintf(x,y++,setting2.textcolor[3],"File Browser");
		pgPrintf(x,y++,setting2.textcolor[3],"Reset");
		pgPrintf(x,y++,setting2.textcolor[3],"Continue");
		pgPrintf(x,y++,setting2.textcolor[3],"Exit");
		
		if(crs_count < 16){
			y = sel + 5;
			if(sel >= SCREEN_SIZE)	y++;
			if(sel >= LOAD_CHEAT)	y++;
			if(sel >= LOAD_ROM)		y++;
			tmp[0]=127; tmp[1]=0;
			pgPrintf(x-1,y,setting2.textcolor[3],tmp);
		}
		
		pgScreenFlipV();
	}
	
	save_config();
	save_config_glob();
	
	pgFillvram(0);
	pgScreenFlipV();
	pgFillvram(0);
	pgScreenFlipV();
	gb_fill_vframe(0);
	pgWaitVn(10);
	memset(&paddata, 0x00, sizeof(paddata));
	wavoutClear();
	
	if(render_msg_mode!=6)
		render_msg_mode = 0;
	border_uploaded = 2;
	bMenu=0;
}
