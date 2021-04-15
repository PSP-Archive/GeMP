#include "INC/main.h"

int bSleep=0;
int bMenu=0;
int bMusic=0;

char RinPath[MAX_PATH];
char RomPath[MAX_PATH];
char RomName[MAX_NAME];
char SavePath[MAX_PATH];
char CheatPath[MAX_PATH];
char ScreenPath[MAX_PATH];
char PlaylistPath[MAX_PATH];

char dlog_buf[256] = "";

void dlog(char* msg, char* file)
{
	int fd = sceIoOpen(file, SCE_O_WRONLY|SCE_O_CREAT|SCE_O_APPEND, 0777);
	sceIoWrite(fd, msg, strlen(msg));
	sceIoClose(fd);
}

void dlog_clear(char* file)
{
	int fd = sceIoOpen(file, SCE_O_WRONLY|SCE_O_CREAT|SCE_O_TRUNC, 0777);
	sceIoClose(fd);
}

void set_cpu_clock(int n)
{
	switch(n) {
	case 0:
		scePowerSetClockFrequency(44, 44, 22);
		break;
	case 1:
		scePowerSetClockFrequency(111, 111, 55);
		break;
	case 2:
		scePowerSetClockFrequency(155, 155, 77);
		break;
  case 3:
  	scePowerSetClockFrequency(222, 222, 111);
		break;
	case 4:
		scePowerSetClockFrequency(266, 266, 133);
		break;
	case 5:
		scePowerSetClockFrequency(333, 333, 166);
		break;
	}
}

// -----------------------------------------------------------------------------

// ホームボタン終了時にコールバック
int exit_callback(void)
{
	bSleep=1;
	set_cpu_clock(3);
	save_config();
	save_config_glob();
	if (rom_get_loaded() && rom_has_battery())
		save_sram(get_sram(), rom_get_info()->ram_size);
	if(mp3_playing()) mp3_thread_stop();
	pspAudioEnd();
	sceKernelExitGame();
	return 0;
}

// スリープ時や不定期にコールバック
void power_callback(int unknown, int pwrflags)
{
	if(pwrflags & POWER_CB_POWER){
		bSleep=1;
		set_cpu_clock(0);
		save_config();
		save_config_glob();
		if (rom_get_loaded() && rom_has_battery())
			save_sram(get_sram(), rom_get_info()->ram_size);
	}
	if(pwrflags & POWER_CB_RESCOMP){
		bSleep=0;
	}
	if(pwrflags & POWER_CB_BATLOW){
		renderer_set_msg("PSP Battery is Low!");
	}

	// コールバック関数の再登録
	// （一度呼ばれたら再登録しとかないと次にコールバックされない）
	int cbid = sceKernelCreateCallback("Power Callback", power_callback);
	scePowerRegisterCallback(0, cbid);
}

// ポーリング用スレッド
int CallbackThread(int args, void *argp)
{
	int cbid;
	
	// コールバック関数の登録
	cbid = sceKernelCreateCallback("Exit Callback", exit_callback);
	sceKernelRegisterExitCallback(cbid);
	cbid = sceKernelCreateCallback("Power Callback", power_callback);
	scePowerRegisterCallback(0, cbid);
	
	// ポーリング
	sceKernelSleepThreadCB();
}

int SetupCallbacks(void)
{
	int thid = 0;
	
	// ポーリング用スレッドの生成
	thid = sceKernelCreateThread("update_thread", CallbackThread, 0x11, 0xFA0, 0, 0);
	if(thid >= 0)
		sceKernelStartThread(thid, 0, 0);
	
	return thid;
}

// -----------------------------------------------------------------------------  


void mainloop(void)
{
#ifdef DEBUG
	unsigned long lasttick;
	unsigned long lastclock=sceKernelLibcClock();
#endif
	unsigned long framecount;
	const unsigned int sync_time=16666;
	unsigned long cur_time = sceKernelLibcClock();
	unsigned long cur_time_bak = cur_time;
	unsigned long prev_time = cur_time;
	unsigned long next_time = cur_time + sync_time;
	unsigned long waitfc=0;
	int line, turbo_bak=0;
	
	char battery[32];
	int ret;
	
	for(;;) {
		for(line=0; line<154; line++)
			gb_run();
		
		cur_time = sceKernelLibcClock();
#ifdef DEBUG
		if (framecount>=60) {
			unsigned long l;

			//フレームレートの指標。60フレームでかかった時間(usec)を１６進で表示。
			//フルフレームで0x000f4240となり、大きいと遅いことになる。解像度が悪いのは勘弁。 - LCK
			framecount=0;
			
			pgcLocate(50,0);
			pgcPuthex8(cur_time-lasttick);
			lasttick=cur_time;
			
			pgcLocate(50,2);
			pgcPuthex8(c_regs_PC);
			l=(cpu_read(c_regs_PC)<<24)+(cpu_read(c_regs_PC+1)<<16)+(cpu_read(c_regs_PC+2)<<8)+(cpu_read(c_regs_PC+3));
			pgcLocate(50,3);
			pgcPuthex8(l);

			unsigned long curclock=cpu_get_clock();
			pgcLocate(50,5);
			pgcPuthex8(cur_time-lastclock);
			lastclock=cur_time;

			pgcLocate(50,25);
			pgcPuthex8(g_regs.IF);
			pgcLocate(50,26);
			pgcPuthex8(g_regs.IE);
			extern byte c_regs_I;
			pgcLocate(50,27);
			pgcPuthex8(c_regs_I);
			
			// kmg
			pgcLocate(2, 2);
			pgcPuthex8(paddata.analog[CTRL_ANALOG_X]);
			pgcLocate(2, 3);
			pgcPuthex8(paddata.analog[CTRL_ANALOG_Y]);
		}
		pgScreenFlip();
#else

		/*framecount++;
		if (framecount>=3600) {
			framecount=0;
			if (rom_get_loaded() && rom_has_battery())
				save_sram(get_sram(), rom_get_info()->ram_size);
			renderer_set_msg("Autosave SRAM");
		}*/
		
		if (bTurbo){
			turbo_bak = 1;
			skip++;
			if (skip > 9){
				skip = 0;
				prev_time = cur_time;
			}
		}else if (cur_time < cur_time_bak){
			prev_time = cur_time;
			skip=0;
		}else if (cur_time > next_time){
			skip++;
			if(skip > setting.frameskip){
				skip=0;
				if(setting.vsync){
					sceDisplayWaitVblank();
					cur_time = sceKernelLibcClock();
				}
				prev_time = cur_time;
			}
		}else{
			if(setting.vsync){
				sceDisplayWaitVblank();
				cur_time = sceKernelLibcClock();
				prev_time = cur_time;
			}else{
				waitfc++;
				while(cur_time < prev_time+10000*(skip+1))
					cur_time = sceKernelLibcClock();
				if (!(waitfc&3)){
					while(cur_time < next_time)
						cur_time = sceKernelLibcClock();
				}
				prev_time = next_time;
			}
			skip=0;
		}
		cur_time_bak = cur_time;
		next_time = prev_time + sync_time * (skip+1);
		if (setting.vsync && !bTurbo){
			if (turbo_bak)
				turbo_bak = 0;
			else if (skip==0) 
				pgScreenFlip();
		}else{
			if (now_frame==0) pgScreenFlip();
		}
#endif		

		// メニュー
		if(bMenu){
			
			save_config();
			save_config_glob();
			if(setting.sound==2){
				wavout_enable=0;
				rin_menu();
			}else{
			wavout_enable=0;
			set_cpu_clock(0);
			rin_menu();
			set_cpu_clock(setting.cpu_clock);
			if(setting.sound==1) 
			{
				wavout_enable=1;
			}
			else
				wavout_enable=0;
			cur_time = sceKernelLibcClock();
			prev_time = cur_time;
			next_time = cur_time + sync_time;
			skip=0;
			}
			bMenu = 0;
		}
		
		if(bMusic)
		{
			load_music_config();
			set_cpu_clock(4);
			
			wavout_enable=0;
			music_main();
			
			cur_time = sceKernelLibcClock();
			prev_time = cur_time;
			next_time = cur_time + sync_time;
			
			while(bMusic){};
			save_music_config();
			set_cpu_clock(setting.cpu_clock);
			if(setting.sound==1) 
			{
				wavout_enable=1;
			}
			else
				wavout_enable=0;
			skip=0;
		}
		
		// スリープ
		if(bSleep){
			int mp3_statu = mp3_get_status();
			int ogg_playing = OGG_Playing();
			mp3_pause(1);
			OGG_Pause(1);
			pgFillvram(0);
			pgScreenFlipV();
			pgFillvram(0);
			pgScreenFlipV();
			
			wavout_enable=0;
			scePowerSetClockFrequency(22, 22, 11);
			while(bSleep){
				readpad();
				if(old_pad & CTRL_HOLD){				
					pgPrint_drawbg(60-strlen("SLEEP"), 33, 0x0000, 0xffff, "SLEEP");
					pgScreenFlipV();		
				}else
					bSleep = 0;
			}
			set_cpu_clock(setting.cpu_clock);
			if(mp3_statu == MP3_PLAY) mp3_pause(0);
			if(ogg_playing) OGG_Pause(0);
			if(setting.sound==1) 
				wavout_enable=1;
			else
				wavout_enable=0;

			cur_time = sceKernelLibcClock();
			prev_time = cur_time;
			next_time = cur_time + sync_time;
			skip=0;
		}
	}
}

int xmain(int argc, char *argv)
{
	int fd, romsize, ramsize;
	char *p, tmp[MAX_PATH];
	
	pgInit();
	pgScreenFrame(2,0);
	
	strcpy(RinPath, argv);
	p = strrchr(RinPath, '/');
	*++p = 0;
	sprintf(CheatPath, "%sCHEAT/", RinPath);
	
	SetupCallbacks();
	wavoutInit();
	pspAudioInit();
	
	last_id=0;
	current_id=MAX_PL+1;
	
	load_config();
	load_config_glob();
	bBitmap = load_menu_bg();
	if(bBitmap) bgbright_change();
		
	mp3_thread_start();
		
	/*if(setting.sound==1) 
		wavout_enable=1;
	else
		wavout_enable=0;*/
		
	strcpy(tmp,RinPath);
	strcat(tmp,"SAVE");
	sceIoMkdir(tmp,0777);
	strcpy(tmp,RinPath);
	strcat(tmp,"CHEAT");
	sceIoMkdir(tmp,0777);
	strcpy(tmp,RinPath);
	strcat(tmp,"CONFIG");
	sceIoMkdir(tmp,0777);
	strcpy(tmp,RinPath);
	strcat(tmp,"SCREENSHOTS");
	sceIoMkdir(tmp,0777);
	strcpy(tmp, RinPath);
	strcat(tmp, "PLAYLIST");
	sceIoMkdir(tmp, 0777);
	strcpy(ScreenPath, RinPath);
	strcat(ScreenPath, "SCREENSHOTS/");
	strcpy(PlaylistPath, RinPath);
	strcat(PlaylistPath, "PLAYLIST/");
	
	gb_init();
	
	strcpy(RomPath,setting2.lastpath);
	
	for(;;){
		if (!getFilePath(RomPath,EXT_GB|EXT_GZ|EXT_ZIP))
			continue;
		strcpy(tmp, RomPath);
		*(strrchr(tmp,'/')+1) = 0;
		strcpy(setting2.lastpath, tmp);

		// 指定したファイルをロードする。 by ruka
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

		load_frame_bg();
		break;
	}

	pgFillvram(0);
	pgScreenFlipV();
	pgFillvram(0);
	pgScreenFlipV();

	set_cpu_clock(setting.cpu_clock);

	mainloop();

	return 0;
}

