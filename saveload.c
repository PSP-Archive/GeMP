#include <stdio.h>
#include <stdlib.h>
#include "INC/main.h"
#include "INC/gz.h"
#include "INC/image.h"
#include "INC/zlib.h"

int remove_file(const char *fullpath)
{
	sceIoRemove(fullpath);
	return 0;

	/*
	char path[MAX_PATH], name[MAX_NAME];
	strcpy(path, fullpath);
	char *p = strrchr(path, '/');
	if(!p)
		return 0;
	strcpy(name , p+1);
	*p = 0;

	if (stricmp(path, path_files)){
		nfiles = 0;
		strcpy(path_files, path);
		int fd = sceIoDopen(path);
		if (fd < 0)
			return 0;
		while (nfiles < MAX_ENTRY){
			memset(&files[nfiles], 0x00, sizeof(SceIoDirent));
			if(sceIoDread(fd, &files[nfiles])<=0) break;
			nfiles++;
		}
		sceIoDclose(fd);
	}

	int i;
	for (i=0; i<nfiles; i++){
		if (!stricmp(name, files[i].name)){
			sceIoRemove(fullpath);
			return 1;
		}
	}

	return 0;
	*/
}

// by kwn
void save_config_glob(void)
{
	int i;

	char path[MAX_PATH];
	strcpy(path, RinPath);
	strcat(path, "CONFIG/Main.CFG");
	
	sceIoRemove(path);
	
	int fd = sceIoOpen(path, SCE_O_WRONLY|SCE_O_TRUNC|SCE_O_CREAT, 644);
	if(fd<0)
		return;
	sceIoWrite(fd, &setting2, sizeof(setting2));
	sceIoClose(fd);
	return;
}

int check_config_glob(){
	int file;
	int ret = 1;
	
	if(setting2.textcolor[0]>31 || setting2.textcolor[0]<0)
		ret = 0;
	if(setting2.textcolor[1]>31 || setting2.textcolor[1]<0)
		ret = 0;
	if(setting2.textcolor[2]>31 || setting2.textcolor[2]<0)
		ret = 0;
	if(setting2.textcolor[3]>31 || setting2.textcolor[3]<0)
		ret = 0;
	if(!setting2.pngpath)
		ret = 0;
	if(!setting2.lastpath)
		ret = 0;
	if(!setting2.framepath)
		ret = 0;
	if(setting2.jpg_shrink>1||setting2.jpg_shrink<0)
		ret = 0;
	if(setting2.jpg_shrink_fr>1||setting2.jpg_shrink_fr<0)
		ret = 0;
	if(setting2.mp3_volume>19 || setting2.mp3_volume<0)
		ret = 0;
	return ret;
}

void init_config_glob(void)
{
	
}

// by kwn
void load_config_glob(void)
{
	FILE * file;
	int i;
	char CfgPath[MAX_PATH];
	char *p;
	strcpy(CfgPath, RinPath);
	strcat(CfgPath, "CONFIG/Main.CFG");
	int fd = sceIoOpen(CfgPath, SCE_O_RDONLY, 644);
	if(fd<0 || !check_config_glob()){
		setting2.textcolor[0] = DEF_COLOR0;
		setting2.textcolor[1] = DEF_COLOR1;
		setting2.textcolor[2] = DEF_COLOR2;
		setting2.textcolor[3] = DEF_COLOR3;	
		setting2.jpg_shrink = 0;
		setting2.jpg_shrink_fr = 0;
		strcpy(setting2.framepath, "");
		strcpy(setting2.pngpath, RinPath);
		strcat(setting2.pngpath, "BACKGROUND/Default.png");
		strcpy(setting2.lastpath,RinPath);
		setting2.mp3_volume = 10;
		sceIoClose(fd);
		sceIoRemove(CfgPath);
		save_config_glob();
		return;
	}
	memset(&setting2, 0, sizeof(setting2));
	sceIoRead(fd, &setting2, sizeof(setting2));
	sceIoClose(fd);
	return;
}

// by kwn
void save_config(void)
{
	int i;
	for(i=0; i<sizeof(SETTING); i++){
		if ( *((byte*)&setting+i) != *((byte*)&tmpsetting+i) )
			break;
	}
	if (i>=sizeof(SETTING))
		return;

	char path[MAX_PATH];
	strcpy(path, RinPath);
	strcat(path, "CONFIG/");
	strcat(path, RomName);
	strcat(path, ".CFG");

	sceIoRemove(path);

	int fd = sceIoOpen(path, SCE_O_WRONLY|SCE_O_TRUNC|SCE_O_CREAT, 644);
	if(fd<0)
		return;
	sceIoWrite(fd, &setting, sizeof(setting));
	sceIoClose(fd);

	tmpsetting = setting;
}

// by kwn
void load_config(void)
{
	int i;
	char CfgPath[MAX_PATH];
	char *p;
	
	strcpy(CfgPath, RinPath);
	strcat(CfgPath, "CONFIG/");
	strcat(CfgPath, RomName);
	strcat(CfgPath, ".CFG");

	int fd = sceIoOpen(CfgPath, SCE_O_RDONLY, 644);
	if(fd<0){
		init_config();
		return;
	}
	
	memset(&setting, 0, sizeof(setting));
	sceIoRead(fd, &setting, sizeof(setting));
	sceIoClose(fd);
	tmpsetting = setting;

	check_config();
}

int save_sram(byte *buf,int size)
{
	const int sram_tbl[]={1,1,1,4,16,8};

	char *p = strrchr(SavePath, '.');
	if (!strcmp(p,".gz")){
		if(!setting.compress)
			remove_file(SavePath);
	}else{
		if(setting.compress)
			remove_file(SavePath);
	}
	
	sprintf(SavePath, "%sSAVE/%s.sav", RinPath, RomName);
	if (setting.compress) {
		strcat(SavePath, ".gz");
		int fd = sceIoOpen(SavePath, SCE_O_WRONLY|SCE_O_TRUNC|SCE_O_CREAT, 644);
		if (fd<0)
			return 0;
	
		int sram_size = 0x2000 * sram_tbl[size];

		if ((rom_get_info()->cart_type>=0x0f)
		 && (rom_get_info()->cart_type<=0x13)){
			int tmp = renderer_get_timer_state();
			memcpy(buf+sram_size, &tmp, sizeof(int));
			sram_size += sizeof(int);
		}

		cheat_decreate_cheat_map();
		gzCreate(fd, buf, sram_size);
		sceIoClose(fd);
		cheat_create_cheat_map();
	}else{
		int fd = sceIoOpen(SavePath, SCE_O_WRONLY|SCE_O_TRUNC|SCE_O_CREAT, 644);
		if(fd<0)
			return 0;

		cheat_decreate_cheat_map();
		sceIoWrite(fd, buf, 0x2000*sram_tbl[size]);
		if ((rom_get_info()->cart_type>=0x0f)
		 && (rom_get_info()->cart_type<=0x13)){
			int tmp = renderer_get_timer_state();
			sceIoWrite(fd, &tmp, sizeof(int));
		}
		sceIoClose(fd);
		cheat_create_cheat_map();

	}
	return 1;
}

int load_sram(void)
{
	sprintf(SavePath, "%sSAVE/%s.sav", RinPath, RomName);
	gzFile fd = gzopen(SavePath, "rb");
	
	if(!fd){
		strcat(SavePath, ".gz");
		fd = gzopen(SavePath, "rb");
	}
	
	if(!fd)
		return 0;
	
	int ramsize = gzread(fd, sram_space, 16*0x2000+4);
	gzclose(fd);
	if(ramsize & 4)
		renderer_set_timer_state(*(int*)(sram_space+ramsize-4));
	return ramsize;
}

void get_state_path(int slot, char *out)
{
	char *p, ext[]=".sv0";
	
	ext[3]=slot+'0';
	strcpy(out, SavePath);
	p = strrchr(out, '.');
	if (!strcmp(p,".gz")){
		*p = 0;
		p = strrchr(out, '.');
	}
	strcpy(p, ext);
}

void get_thumb_path(int slot, char *out)
{
	char *p, ext[]=".tn0.png";
	
	ext[3]=slot+'0';
	strcpy(out, SavePath);
	p = strrchr(out, '.');
	if (!strcmp(p,".gz")){
		*p = 0;
		p = strrchr(out, '.');
	}
	strcpy(p, ext);
}

int delete_thumb(int slot)
{
	int ret;
	char path[MAX_PATH];
	
	get_thumb_path(slot,path);
	ret = remove_file(path);
	*strrchr(path, '.') = 0;
	ret = remove_file(path);
	
	return 1;
}

int delete_state(int slot)
{
	int ret;
	char path[MAX_PATH];
	
	get_state_path(slot,path);
	ret = remove_file(path);
	strcat(path, ".gz");
	ret = remove_file(path);
	
	delete_thumb(slot);
	
	return ret;
}

int save_thumb(int slot)
{
	char path[MAX_PATH];
	get_thumb_path(slot,path);

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
	return ret;
}

int load_thumb(int slot, unsigned short *out, size_t outlen)
{
	int fd, size;
	char path[MAX_PATH];
	
	get_thumb_path(slot,path);
	
	return read_png(path, out, outlen);
}

int load_thumb_old(int slot, unsigned short *out, size_t outlen)
{
	if (outlen != 160*144*sizeof(unsigned short))
		return 0;
	
	char path[MAX_PATH];
	get_thumb_path(slot,path);
	*strrchr(path, '.') = 0;
	int fd = sceIoOpen(path, SCE_O_RDONLY, 644);
	if (fd<0)
		return 0;
	size_t size = sceIoLseek(fd, 0, SEEK_END);
	sceIoLseek(fd, 0, SEEK_SET);
	byte *buf = (byte*)malloc(size);
	if (!buf){
		sceIoClose(fd);
		return 0;
	}
	sceIoRead(fd, buf, size);
	sceIoClose(fd);

	byte thumb_b[160*144];
	PackBits_decode(buf, thumb_b, size);
	free(buf);
	
	byte bgr;
	word r, g, b;
	int i;
	for(i=0; i<160*144; i++){
		bgr = thumb_b[i];
		r = ((bgr   )&0x3) * 0x1F / 0x03;
		g = ((bgr>>2)&0x3) * 0x1F / 0x03;
		b = ((bgr>>4)&0x3) * 0x1F / 0x03;
		out[i] = (b<<10) | (g<<5) | r;
	}
	return 1;
}

int save_state(int slot)
{
	size_t size = gb_save_state(NULL);
	byte *buf = (byte*)malloc(size);
	if (!buf)
		return 0;
	gb_save_state(buf);
	
	int ret;
	char path[MAX_PATH];
	get_state_path(slot,path);

	if (setting.compress){
		strcat(path,".gz");

		int fd = sceIoOpen(path, SCE_O_WRONLY|SCE_O_CREAT|SCE_O_TRUNC, 644);
		if (fd<0){
			free(buf);
			return 0;
		}

		ret = gzCreate(fd, buf, size);
		sceIoClose(fd);
		free(buf);
		
		if (!ret){
			sceIoRemove(path);
			return 0;
		}else{
			get_state_path(slot,path);
			remove_file(path);
		}
	}else{
		int fd = sceIoOpen(path, SCE_O_WRONLY|SCE_O_CREAT|SCE_O_TRUNC, 644);
		if (fd<0){
			free(buf);
			return 0;
		}

		ret = sceIoWrite(fd, buf, size);
		sceIoClose(fd);
		free(buf);

		if (size!=ret){
			sceIoRemove(path);
			return 0;
		}else{
			strcat(path, ".gz");
			remove_file(path);
		}
	}

	if(!setting.thumb)
		delete_thumb(slot);
	else
		return save_thumb(slot);
	
	return 1;
}


int load_state(int slot)
{
	char path[MAX_PATH];
	get_state_path(slot,path);

	gzFile fd = gzopen(path,"rb");
	if (!fd){
		strcat(path,".gz");
		fd = gzopen(path,"rb");
		if (!fd)
			return 0;
	}

	gb_restore_state(fd,NULL);
	gzclose(fd);
	
	if(rom_get_info()->gb_type == 1)
		lcd_set_mpal(0);
	else if(rom_get_info()->gb_type == 2 && sgb_mode)
		lcd_set_mpal(PAL_SGB);
	else
		lcd_set_mpal(setting.gb_palette);
		
	if(rom_get_info()->gb_type>=3 && org_gbtype==3)
		now_gb_mode = 3;
	else if(rom_get_info()->gb_type==2 && sgb_mode)
		now_gb_mode = 2;
	else
		now_gb_mode = 1;
	
	border_uploaded = 2;
	
	return 1;
}

byte *save_state_tmp()
{
	size_t size = gb_save_state(NULL);
	byte *buf = (byte*)malloc(size);
	if (!buf)
		return NULL;
	gb_save_state(buf);

	unsigned int x,y;
	for(y=0; y<144; y++){
		for(x=0; x<160; x++){
			thumb_tmp[y*160+x] = vframe[y*SIZE_LINE+GUARD_LINE+x];
		}
	}
	
	sceRtcGetCurrentClockLocalTime(&state_tmp_time);
	
	return buf;
}

int load_state_tmp(byte *buf)
{
	if (!buf)
		return 0;

	gb_restore_state(0,buf);

	if(rom_get_info()->gb_type == 1)
		lcd_set_mpal(0);
	else if(rom_get_info()->gb_type == 2 && sgb_mode)
		lcd_set_mpal(PAL_SGB);
	else
		lcd_set_mpal(setting.gb_palette);
		
	if(rom_get_info()->gb_type>=3 && org_gbtype==3)
		now_gb_mode = 3;
	else if(rom_get_info()->gb_type==2 && sgb_mode)
		now_gb_mode = 2;
	else
		now_gb_mode = 1;
	
	border_uploaded = 2;

	return 1;
}

