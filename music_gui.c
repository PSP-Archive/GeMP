#include "INC/main.h"

int add_entry_pl(char *filepath, int type)
{
	strcpy(playlist[last_id].name, filepath);
	playlist[last_id].type = type;
	last_id++;
}

int remove_entry_pl(int num)
{
	int i;
	for(i=num;i<last_id;i++){
		if(i+1<last_id) strcpy(playlist[i].name, playlist[i+1].name);
		if(i+1<last_id) playlist[i].type = playlist[i+1].type;
	}
	last_id--;
	if(current_id>last_id-1) current_id=last_id-1;
}

void save_music_config(void)
{
	int i;

	char path[MAX_PATH];
	strcpy(path, RinPath);
	strcat(path, "CONFIG/Music_Player.CFG");
	
	sceIoRemove(path);
	
	int fd = sceIoOpen(path, SCE_O_WRONLY|SCE_O_TRUNC|SCE_O_CREAT, 644);
	if(fd<0)
		return;
	sceIoWrite(fd, &music_config, sizeof(music_config));
	sceIoClose(fd);
	return;
}

int check_music_config()
{
	int ret = 1;
	if(music_config.repeat>2 || music_config.repeat<0){
		music_config.repeat = 1;
		ret = 0;
	}
	if(music_config.shuffle>1 || music_config.shuffle<0){
		music_config.shuffle = 0;
		ret = 0;
	}
	return ret;
}

void load_music_config(void)
{
	FILE * file;
	int i;
	char CfgPath[MAX_PATH];
	char *p;
	strcpy(CfgPath, RinPath);
	strcat(CfgPath, "CONFIG/Music_Player.CFG");
	int fd = sceIoOpen(CfgPath, SCE_O_RDONLY, 644);
	if(fd<0){
		music_config.repeat = 2;
		music_config.shuffle = 0;
		sceIoClose(fd);
		sceIoRemove(CfgPath);
		save_music_config();
		return;
	}
	if(!check_music_config()) return;
	memset(&music_config, 0, sizeof(music_config));
	sceIoRead(fd, &music_config, sizeof(music_config));
	sceIoClose(fd);
	return;
}

int music_setup()
{
	char tmp[MAX_PATH];
	
	a_x = a_y = 0;
	
	
	sprintf(skin_path, "%s/Skins/Default/bg.png", RinPath);
	
	skin_bg = LoadGFX(skin_path);
	if(!skin_bg) return 0;
	read_png(skin_path,imgskin,sizeof(imgskin));
	
	strcpy(tmp, RinPath);
	strcat(tmp, "Skins/Default/button1_h.png");
	button1 = LoadGFX(tmp);
	if(!button1) return 0;
	read_png(tmp,imgbutton1,sizeof(imgbutton1));
	
	strcpy(tmp, RinPath);
	strcat(tmp, "Skins/Default/button2_h.png");
	button2 = LoadGFX(tmp);
	if(!button2) return 0;
	read_png(tmp,imgbutton2,sizeof(imgbutton2));
	
	strcpy(tmp, RinPath);
	strcat(tmp, "Skins/Default/button3_h.png");
	button3 = LoadGFX(tmp);
	if(!button3) return 0;
	read_png(tmp,imgbutton3,sizeof(imgbutton3));
	
	strcpy(tmp, RinPath);
	strcat(tmp, "Skins/Default/button4_h.png");
	button4 = LoadGFX(tmp);
	if(!button4) return 0;
	read_png(tmp,imgbutton4,sizeof(imgbutton4));
	
	strcpy(tmp, RinPath);
	strcat(tmp, "Skins/Default/button5_h.png");
	button5 = LoadGFX(tmp);
	if(!button5) return 0;
	read_png(tmp,imgbutton5,sizeof(imgbutton5));
	
	strcpy(tmp, RinPath);
	strcat(tmp, "Skins/Default/volume.png");
	volumeo = LoadGFX(tmp);
	if(!volumeo) return 0;
	read_png(tmp,imgvolumeo,sizeof(imgvolumeo));
	
	strcpy(tmp, RinPath);
	strcat(tmp, "Skins/Default/slider.png");
	volume = LoadGFX(tmp);
	if(!volume) return 0;
	read_png(tmp,imgvolume,sizeof(imgvolume));
	
	strcpy(tmp, RinPath);
	strcat(tmp, "Skins/Default/balance.png");
	balanceo = LoadGFX(tmp);
	if(!balanceo) return 0;
	read_png(tmp,imgbalanceo,sizeof(imgbalanceo));
	
	strcpy(tmp, RinPath);
	strcat(tmp, "Skins/Default/slider.png");
	balance = LoadGFX(tmp);
	if(!balance) return 0;
	read_png(tmp,imgbalance,sizeof(imgbalance));
	
	strcpy(tmp, RinPath);
	strcat(tmp, "Skins/Default/pointer.png");
	pointer = LoadGFX(tmp);
	if(!pointer) return 0;
	read_png(tmp,imgpointer,sizeof(imgpointer));
	
	strcpy(tmp, RinPath);
	strcat(tmp, "Skins/Default/playlist_scrollbar.png");
	playlistscroll = LoadGFX(tmp);
	if(!playlistscroll) return 0;
	read_png(tmp,imgplaylistscr,sizeof(imgplaylistscr));
	
	strcpy(tmp, RinPath);
	strcat(tmp, "Skins/Default/playlist_scrollbar_up.png");
	playlistscrollt = LoadGFX(tmp);
	if(!playlistscrollt) return 0;
	read_png(tmp,imgplaylistscrt,sizeof(imgplaylistscrt));
	
	strcpy(tmp, RinPath);
	strcat(tmp, "Skins/Default/playlist_scrollbar_down.png");
	playlistscrollb = LoadGFX(tmp);
	if(!playlistscrollb) return 0;
	read_png(tmp,imgplaylistscrb,sizeof(imgplaylistscrb));
	
	
	//Prev button
	button1_x = 16;
  button1_y = 215;

	//Play button
  button2_x = 64;
  button2_y = 215;

	//Pause button
  button3_x = 112;
  button3_y = 215;

	//Stop button
  button4_x = 160;
  button4_y = 215;

	//Next button
  button5_x = 208;
  button5_y = 215;

	//Volume slider
  volume_x = 269;
  volume_y = 215;

	//Volume object
  volumeo_x = 269;
  volumeo_y = 200;
  
  //Balance slider
  balance_x = 269;
  balance_y = 244;

	//Balance object
  balanceo_x = 269;
  balanceo_y = 229;
  
  //Playlist window
  playlist_x = 11;
  playlist_y = 40;
  
  //Playlist top cursor
  playlists_x = 247-playlistscroll->imageWidth;
  playlists_y = 205-playlistscroll->imageHeight;
  
  //Playlist top cursor
  playlistt_x = playlist_x-playlistscrollt->imageWidth;
  playlistt_y = playlist_y;
  
 	//Playlist bottom cursor
 	playlistb_x = playlist_x-playlistscrollb->imageWidth;
 	playlistb_y = playlist_y+(playlistscroll->imageHeight)-(playlistscrollb->imageHeight);
}

int collision_check(int x, int y)
{
	int i, a;
	for(i=0;i<button1->imageWidth;i++){
		for(a=0;a<button1->imageHeight;a++){
			if(x == button1_x+i && y == button1_y+a){
				return PREV_B;
			}
		}
	}
	for(i=0;i<button2->imageWidth;i++){
		for(a=0;a<button2->imageHeight;a++){
			if(x == button2_x+i && y == button2_y+a){
				return PLAY_B;
			}
		}
	}
	for(i=0;i<button3->imageWidth;i++){
		for(a=0;a<button3->imageHeight;a++){
			if(x == button3_x+i && y == button3_y+a){
				return PAUSE_B;
			}
		}
	}
	for(i=0;i<button4->imageWidth;i++){
		for(a=0;a<button4->imageHeight;a++){
			if(x == button4_x+i && y == button4_y+a){
				return STOP_B;
			}
		}
	}
	for(i=0;i<button5->imageWidth;i++){
		for(a=0;a<button5->imageHeight;a++){
			if(x == button5_x+i && y == button5_y+a){
				return NEXT_B;
			}
		}
	}
	for(i=0;i<volume->imageWidth;i++){
		for(a=0;a<volume->imageHeight;a++){
			if(x == (volume_x+(setting2.mp3_volume*10))+i && y == volume_y+a){
				return VOLUME;
			}
		}
	}
	for(i=0;i<238;i++){
		for(a=0;a<166;a++){
			if(x == playlist_x+i && y == playlist_y+a){
				return PLAYLIST;
			}
		}
	}
	return 0;
}

int music_main()
{
	FILE * playlist_fd;
	char timer[256], *p, tmp[MAX_PATH], disp_name[MAX_PATH], tmp_name[MAX_PATH], buf[MAX_PATH], *time_string;
	int i, a, sel=last_id, top=0, rows=20, highlight=last_id;
	//Mode 0=music, 1=playlist
	int mode=0;
	
	int sec, min, hour, dayl, dayh;
	
	music_setup();
	
	mp3_set_volume(setting2.mp3_volume);
	
	setting.sound = 2;
	
	ctrl_data_t pad;
	
	for(;;)
	{
		//readpad();
		//Music mode
		readpad2();
		
		if(mp3_get_status()==MP3_END || OGG_EndOfStream())
		{
			mp3_stop(); 
			OGG_End();
				
			music_config.shuffle?current_id=rand()%last_id-1:current_id++;
			if(current_id>=last_id-1) {
				current_id=0;
			}
					
			if(playlist[current_id].type==0){
				mp3_play(playlist[current_id].name);
				mp3_set_volume(setting2.mp3_volume);
			}else if(playlist[current_id].type==1){
				OGG_Init(0);
				OGG_Load(playlist[current_id].name);
				OGG_Play();
			}
		}
		
		sceCtrlReadBufferPositive(&pad, 1);
		
		//Move down
		if (pad.analog[CTRL_ANALOG_Y] == 0xFF) a_y+=4;
		//Move lower right
		if (pad.analog[CTRL_ANALOG_Y] == (0xFF | 0xFF) & pad.analog[CTRL_ANALOG_X] == (0xFF | 0xFF)) {
			a_y+=2;
			a_x+=2;
		}
		//Move lower left
		if (pad.analog[CTRL_ANALOG_Y] == (0xFF | 0xFF) & pad.analog[CTRL_ANALOG_X] == (0x00 | 0x00)) {
			a_y+=2;
			a_x+=2;
		}
		//Move up
		if (pad.analog[CTRL_ANALOG_Y] == 0x00) a_y-=4;
		//Move upper right
		if (pad.analog[CTRL_ANALOG_Y] == (0x00 | 0x00) & pad.analog[CTRL_ANALOG_X] == (0xFF | 0xFF)) {
			a_y+=2;
			a_x+=2;
		}
		//Move upper left
		if (pad.analog[CTRL_ANALOG_Y] == (0x00 | 0x00) & pad.analog[CTRL_ANALOG_X] == (0x00 | 0x00)) {
			a_y+=2;
			a_x+=2;
		}
		if(collision_check(a_x, a_y)==VOLUME){
			a_x = volume_x+(setting2.mp3_volume*10)+5;
			if (pad.analog[CTRL_ANALOG_X] == 0x00){
				if(setting2.mp3_volume>0) setting2.mp3_volume-=0.5;
				if(playlist[current_id].type==0) 
					mp3_set_volume(setting2.mp3_volume);
				else
					OGG_SetVolume(setting2.mp3_volume);
			}else if (pad.analog[CTRL_ANALOG_X] == 0xFF){
				if(setting2.mp3_volume<19) setting2.mp3_volume+=0.5;
				if(playlist[current_id].type==0) 
					mp3_set_volume(setting2.mp3_volume);
				else
					OGG_SetVolume(setting2.mp3_volume);
			}
		}else{
			//Move right
			if (pad.analog[CTRL_ANALOG_X] == 0xFF) a_x+=4;
			//Move left
			if (pad.analog[CTRL_ANALOG_X] == 0x00) a_x-=4;
		}
		if(a_x<0) a_x=0;
		if(a_y<0) a_y=0;
		if(a_x>480-pointer->imageWidth) a_x = 480-pointer->imageWidth;
		if(a_y>272-pointer->imageHeight) a_y = 272-pointer->imageHeight;
		
		if(mode==0){
			//Play
			if(top<0) top = 0;
			if(top>= last_id-rows && last_id>rows) top = last_id-rows;
				
			if(collision_check(a_x, a_y)==PLAYLIST){
				highlight = ((a_y-((playlist_y+4)))/8)+top;
			}
				
			if(new_pad & CTRL_CROSS){
				
				if(collision_check(a_x, a_y)==PLAYLISTB)
					if(last_id>rows && top<last_id-rows) top++;
				if(collision_check(a_x, a_y)==PLAYLISTT)
					if(last_id>rows && top>0) top--;
						
				if(collision_check(a_x, a_y)==PLAYLIST){
					if((a_y-((playlist_y+4)))/8<last_id){
						current_id = ((a_y-(playlist_y))/8)+top;
					
					mp3_stop();
					OGG_End();
			
					if(playlist[current_id].type==0){
						mp3_play(playlist[current_id].name);
						mp3_set_volume(setting2.mp3_volume);
					}else if(playlist[current_id].type==1){
						OGG_Init(0);
						OGG_Load(playlist[current_id].name);
						OGG_Play();
					}
				}
				}else if(collision_check(a_x, a_y)==PREV_B){
					mp3_stop();
					OGG_End();
					current_id--;
					if(current_id<0) current_id=last_id-1;
					
					if(playlist[current_id].type==0){
						mp3_play(playlist[current_id].name);
						mp3_set_volume(setting2.mp3_volume);
					}else if(playlist[current_id].type==1){
						OGG_Init(0);
						OGG_Load(playlist[current_id].name);
						OGG_Play();
					}
				//Play
				}else if(collision_check(a_x, a_y)==PLAY_B){
					mp3_stop();
					OGG_End();
			
					if(playlist[current_id].type==0){
						mp3_play(playlist[current_id].name);
						mp3_set_volume(setting2.mp3_volume);
					}else if(playlist[current_id].type==1){
						OGG_Init(0);
						OGG_Load(playlist[current_id].name);
						OGG_Play();
					}
				//Pause
				}else if(collision_check(a_x, a_y)==PAUSE_B){
					if(playlist[current_id].type==0){
						mp3_get_status()==MP3_PAUSE?mp3_pause(0):mp3_pause(1);
						mp3_set_volume(setting2.mp3_volume);
					}else if(playlist[current_id].type==1){
						OGG_Playing()?OGG_Pause(1):OGG_Pause(0);
				//Stop
				}else if(collision_check(a_x, a_y)==STOP_B){
					while(!mp3_stop() && !OGG_End())
					{
						sceKernelDelayThread(100);
					}
				}else if(collision_check(a_x, a_y)==NEXT_B){
					while(!mp3_stop() && !OGG_End())
					{
						sceKernelDelayThread(100);
					}
				
					current_id++;
					if(current_id>last_id) current_id=last_id;
					
					if(playlist[current_id].type==0){
						mp3_play(playlist[current_id].name);
						mp3_set_volume(setting2.mp3_volume);
					}else if(playlist[current_id].type==1){
						OGG_Init(0);
						OGG_Load(playlist[current_id].name);
						OGG_Play();
					}
				}
			}
			//Delete
			}else if(new_pad & CTRL_TRIANGLE){
				remove_entry_pl(highlight);
				current_id=last_id;
			}
		//Playlist mode
		}else if(mode==1){
			//Save playlist
			if(new_pad & CTRL_CROSS){
				strcpy(tmp, PlaylistPath);
				strcat(tmp, "playlist.rpl");
				playlist_fd = fopen(tmp, "wt+");
				if(playlist_fd==NULL){
					fclose(playlist_fd);
					continue;
				}
				fclose(playlist_fd);
				sceIoRemove(tmp);
				for(i=0;i<last_id;i++){
					strcpy(tmp_name, playlist[i].name);
					strcat(tmp_name, "\n");
					dlog(tmp_name, tmp);	
				}
			//Load playlist
			}else if(new_pad & CTRL_TRIANGLE){
				strcpy(tmp, PlaylistPath);
				strcat(tmp, "playlist.rpl");
				playlist_fd = fopen(tmp, "rt");
				if(playlist_fd == NULL){
					fclose(playlist_fd);
					continue;
				}
				for(i=0;i<last_id;i++)
					strcpy(playlist[i].name, "");
				i=0;
				while(!feof(playlist_fd)){
					/*if(fgets(buf, MAX_PATH, playlist_fd) && buf[0]!='\n' && buf[0]!='\r'){
						for(a=0;a<sizeof(buf);a++)
							if(buf[a]!='\n') playlist[i][a] = buf[a];
						i++;
					}*/
				}
				fclose(playlist_fd);
				last_id=i;
				if(sel>last_id-1) sel=last_id-1;
			}
		}
		//Return
		if(new_pad & CTRL_CIRCLE)
			break;
		else if(new_pad & CTRL_RTRIGGER){
			mode++;
			if(mode>1) mode=0;
		//Move cursor up
		}else if(new_pad & CTRL_UP){
			sel--;
			if(sel<0) {
				sel=last_id-1;
				if(last_id>rows)
					top=last_id-rows;
			}
			if(sel<top) top--;
		//Move cursor down
		}else if(new_pad & CTRL_DOWN){
			sel++;
			if(sel>last_id-1) sel=top=0;
			if(sel>rows-1 && top<last_id-rows) top++;
		}
		pgBitBlt(0, 0, skin_bg->imageWidth, skin_bg->imageHeight, 1, imgskin);
		if(last_id>0){
			for(i=0;i<rows;i++)
			{
				if(i>last_id-1) break;
				p=strrchr(playlist[i+top].name, '/');
				strcpy(tmp, p+1);
				sprintf(disp_name, "%d. %s", (top+i)+1, tmp);
				for(a=0;a<strlen(disp_name);a++)
				  if(a>27) disp_name[a]='\0';
				if(top+i==current_id) pgPrintf(5, 2, setting2.textcolor[3], disp_name);
				pgPrintp(2, i+5, setting2.textcolor[top+i==highlight?2:3], disp_name);
			}
		playlist[current_id].type==0?mp3_get_timer_string(timer):OGG_GetTimeString(timer);
		pgPrintf(5, 1, setting2.textcolor[3], "%s", timer);

		//pgPrintf(58-11, 0,setting2.textcolor[2], "Volume: %0.lf%", setting2.mp3_volume*10);
		}else
			pgPrint(2, 6, 0xffff, "-Empty-");
			
		char battery[32];
		int ret;
	
		if(scePowerIsBatteryExist()){
			sprintf(battery,"%d%%",scePowerGetBatteryLifePercent());
			if(!scePowerIsPowerOnline()){
				if((ret=scePowerGetBatteryLifeTime()) >= 0)
					sprintf(&battery[strlen(battery)]," %dh:%02dm",ret/60,ret%60);
			}
		}
		mh_print(360, 15, battery, setting2.textcolor[2]);
				
		//mh_printf(275, 46, 0xffff, "Collision: %d", collision_check(a_x, a_y));
				
		if(collision_check(a_x, a_y)==PREV_B)
			pgBitBltA(button1_x,button1_y,button1->imageWidth,button1->imageHeight,1,imgbutton1, 220, 31, 169);
		if(collision_check(a_x, a_y)==PLAY_B)
			pgBitBltA(button2_x,button2_y,button2->imageWidth,button2->imageHeight,1,imgbutton2, 220, 31, 169);
		if(collision_check(a_x, a_y)==PAUSE_B)
			pgBitBltA(button3_x,button3_y,button3->imageWidth,button3->imageHeight,1,imgbutton3, 220, 31, 169);
		if(collision_check(a_x, a_y)==STOP_B)
			pgBitBltA(button4_x,button4_y,button4->imageWidth,button4->imageHeight,1,imgbutton4, 220, 31, 169);
		if(collision_check(a_x, a_y)==NEXT_B)
			pgBitBltA(button5_x,button5_y,button5->imageWidth,button5->imageHeight,1,imgbutton5, 220, 31, 169);
		
		//Volume	
		pgBitBlt(volume_x+(setting2.mp3_volume*10),volume_y,volume->imageWidth,volume->imageHeight,1,imgvolume);
		
		//Balance
		pgBitBlt(balance_x+(setting2.mp3_volume*10),balance_y,balance->imageWidth,balance->imageHeight,1,imgbalance);

		pgBitBlt(playlists_x,playlists_y,playlistscroll->imageWidth,playlistscroll->imageHeight,1,imgplaylistscr);		
		
		pgBitBltA(a_x,a_y,pointer->imageWidth,pointer->imageHeight,1,imgpointer, 220, 31, 169);

		pgScreenFlipV();
	}
	bMusic=0;
	return 1;
}
