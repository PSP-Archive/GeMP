#include <stdio.h>
#include <string.h>
#include "INC/main.h"

extern dword utils_dword2string(dword dw, char * dest, dword width)
{
	dest[width] = 0;
	if(dw == 0)
	{
		dest[--width] = '0';
		return width;
	}
	while(dw > 0 && width > 0)
	{
		dest[-- width] = '0' + (dw % 10);
		dw /= 10;
	}
	return width;
}

extern const char * utils_fileext(const char * filename)
{
	dword len = strlen(filename);
	const char * p = filename + len;
	while(p > filename && *p != '.' && *p != '/') p --;
	if(*p == '.')
		return p + 1;
	else
		return NULL;
}

extern int utils_del_dir(char * dir)
{
	char *p;
	char tmp[MAX_PATH];
	int dl = sceIoDopen(dir);
	if(dl < 0)
		return 0;
	SceIoDirent sid;
	while(sceIoDread(dl, &sid))
	{
		if(sid.name[0] == '.') continue; // hide file
		char compPath[260];
		strcpy(tmp, dir);
		strcat(tmp, sid.name);
		if(FIO_S_ISDIR(sid.stat.mode)) // dir
		{
			strcat(tmp, "/");
			utils_del_dir(tmp);
			continue;
		}
		sceIoRemove(tmp);
		memset(&sid, 0, sizeof(SceIoDirent));
	}
	sceIoDclose(dl);
	
	dir[strlen(dir)-1] = 0;
	sceIoRmdir(dir);
	return 1;
}
