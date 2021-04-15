#include "INC/main.h"
#include "INC/gz.h"
#include "INC/zlibInterface.h"


SceIoDirent files[MAX_ENTRY];
SceIoDirent *sortfiles[MAX_ENTRY];
int nfiles;
char path_files[MAX_PATH]={0};

SceIoDirent zip_files[MAX_ENTRY];
SceIoDirent *zip_sortfiles[MAX_ENTRY];
int zip_nfiles;
char path_inzip[MAX_PATH]={0};

char filecopyname[MAX_PATH];
char lastext[4];
int dir;
int device;

// 拡張子管理用
const struct {
	char *szExt;
	int nExtId;
} stExtentions[] = {
 "gb",EXT_GB,
 "gbc",EXT_GB,
 "sgb",EXT_GB,
 "gz",EXT_GZ,
 "zip",EXT_ZIP,
 "tch",EXT_TCH,
 "png", EXT_IMG,
 "jpg", EXT_IMG,
 "tga", EXT_IMG,
 "txt", EXT_TXT,
 "mp3", EXT_MP3,
 "ogg", EXT_OGG,
 NULL, EXT_UNKNOWN
};
int getExtId(const char *szFilePath) {
	char *pszExt;

	if ((pszExt = strrchr(szFilePath, '.'))) {
		pszExt++;
		int i;
		for (i = 0; stExtentions[i].nExtId != EXT_UNKNOWN; i++) {
			if (!stricmp(stExtentions[i].szExt,pszExt)) {
				return stExtentions[i].nExtId;
			}
		}
	}

	return EXT_UNKNOWN;
}

// Unzip 対応 by ruka

// コールバック受け渡し用
typedef struct {
	byte *p_rom_image;			// pointer to rom image
	long rom_size;				// rom size
	char szFileName[MAX_PATH];	// extracted file name
}ROM_INFO, *LPROM_INFO;

// せっかくなのでプログレスでも出してみます
void draw_load_rom_progress(unsigned long ulExtractSize, unsigned long ulCurrentPosition)
{
	int nPer = 100 * ulExtractSize / ulCurrentPosition;
	static int nOldPer = 0;
	if (nOldPer == nPer & 0xFFFFFFFE) {
		return ;
	}
	nOldPer = nPer;
	rin_frame("", "");
	// プログレス
	pgDrawFrame(89,121,391,141,setting2.textcolor[1]);
	pgFillBox(90,123, 90+nPer*3, 139,setting2.textcolor[1]);
	// ％
	char szPer[16];
	sprintf(szPer,"%d%%",nPer);
	pgPrint(28,16,setting2.textcolor[3],szPer);
	// pgScreenFlipV()を使うとpgWaitVが呼ばれてしまうのでこちらで。
	// プログレスだからちらついても良いよね～
	pgScreenFlip();
}

// Unzip コールバック
int funcUnzipCallback(int nCallbackId, unsigned long ulExtractSize, unsigned long ulCurrentPosition,
                      const void *pData, unsigned long ulDataSize, unsigned long ulUserData)
{
    const char *pszFileName;
    int nExtId;
    const unsigned char *pbData;
    LPROM_INFO pRomInfo = (LPROM_INFO)ulUserData;

    switch(nCallbackId) {
    case UZCB_FIND_FILE:
		pszFileName = (const char *)pData;
		
		if (path_inzip[0] && strcmp(pszFileName,path_inzip))
			break;
		
		nExtId = getExtId(pszFileName);
		// 拡張子がGB・GBC・SGBなら展開
		if (nExtId == EXT_GB) {
			// 展開する名前、rom sizeを覚えておく
			strcpy(pRomInfo->szFileName, pszFileName);
			pRomInfo->rom_size = ulExtractSize;
			return UZCBR_OK;
		}
        break;
    case UZCB_EXTRACT_PROGRESS:
		pbData = (const unsigned char *)pData;
		// 展開されたデータを格納しよう
		memcpy(pRomInfo->p_rom_image + ulCurrentPosition, pbData, ulDataSize);
		draw_load_rom_progress(ulCurrentPosition + ulDataSize, ulExtractSize);
		return UZCBR_OK;
        break;
    default: // unknown...
		pgFillvram(RGB(255,0,0));
		mh_print(0,0,"Unzip fatal error.",0xFFFF);
		pgScreenFlipV();
        break;
    }
    return UZCBR_PASS;
}

// load rom image by ruka
long load_rom(const char *szRomPath)
{
	char name[MAX_NAME], *p, tmp[MAX_PATH];
	long lReadSize=0;
	ROM_INFO stRomInfo;
	int nRet;
	int nExtId = getExtId(szRomPath);

	switch(nExtId) {
	case EXT_GB:{	// "gb" "gbc" "sgb"
		p = strrchr(szRomPath, '/');
		if (!p)
			return 0;
		strcpy(name, p+1);

		int fd = sceIoOpen(szRomPath, SCE_O_RDONLY, 644);
		lReadSize = sceIoRead(fd, rom_image, MAX_ROM_SIZE);
		sceIoClose(fd);
		break;

	}case EXT_GZ:{	// "gz"
		gzFile fd = gzopen(szRomPath, "r");
		if (!gzGetOrigName(szRomPath, name, fd)){
			gzclose(fd);
			return 0;
		}else if (getExtId(name)!=EXT_GB){
			gzclose(fd);
			return 0;
		}
		lReadSize = gzread(fd, rom_image, MAX_ROM_SIZE);
		gzclose(fd);
		break;

	}case EXT_ZIP:	// "zip"
		if (path_inzip[0]){
			if (getExtId(path_inzip)!=EXT_GB)
				return 0;
			p = strrchr(path_inzip, '/');
			if (!p)
				p = path_inzip;
			else
				p++;
			strcpy(name, p);
		}else{
			p = strrchr(szRomPath, '/');
			if (!p)
				return 0;
			strcpy(name, p+1);
		}
			

		stRomInfo.p_rom_image = rom_image;
		stRomInfo.rom_size = 0;
		memset(stRomInfo.szFileName, 0x00, sizeof(stRomInfo.szFileName));
		// Unzipコールバックセット
		Unzip_setCallback(funcUnzipCallback);
		// Unzip展開する
	    nRet = Unzip_execExtract(szRomPath, (unsigned long)&stRomInfo);
		if (nRet != UZEXR_OK) {
			// 読み込み失敗！ - このコードでは、UZEXR_CANCELもここに来て
			// しまうがコールバックでキャンセルしてないので無視
			lReadSize = 0;
			pgFillvram(RGB(255,0,0));
			mh_print(0,0,"Unzip fatal error.",0xFFFF);
			pgScreenFlipV();
		}
		lReadSize = stRomInfo.rom_size;
		break;
	default:
		return 0;
	}
	
	if(lReadSize){
		strcpy(RomName, name);
		p = strrchr(RomName, '.');
		if(p)
			*p = 0;
	}
	
	return lReadSize;
}

////////////////////////////////////////////////////////////////////////
// クイックソート
// AC add start
void SJISCopy(SceIoDirent *a, unsigned char *file)
{
	unsigned char ca;
	int i;
	int len=strlen(a->name);
	
	for(i=0;i<=len;i++){
		ca = a->name[i];
		if (((0x81 <= ca)&&(ca <= 0x9f))
		|| ((0xe0 <= ca)&&(ca <= 0xef))){
			file[i++] = ca;
			file[i] = a->name[i];
		}
		else{
			if(ca>='a' && ca<='z') ca-=0x20;
			file[i] = ca;
		}
	}

}
int cmpFile(SceIoDirent *a, SceIoDirent *b)
{
    unsigned char file1[0x108];
    unsigned char file2[0x108];
	unsigned char ca, cb;
	int i, n, ret;

	if(a->type==b->type){
		SJISCopy(a, file1);
		SJISCopy(b, file2);
		n=strlen((char*)file1);
		for(i=0; i<=n; i++){
			ca=file1[i]; cb=file2[i];
			ret = ca-cb;
			if(ret!=0) return ret;
		}
		return 0;
	}
	
	if(a->type & TYPE_DIR)	return -1;
	else					return 1;
}
// AC add end

void sort_files(SceIoDirent **a, int left, int right) {
	SceIoDirent *tmp, *pivot;
	int i, p;
	
	if (left < right) {
		pivot = a[left];
		p = left;
		for (i=left+1; i<=right; i++) {
			if (cmpFile(a[i],pivot)<0){
				p=p+1;
				tmp=a[p];
				a[p]=a[i];
				a[i]=tmp;
			}
		}
		a[left] = a[p];
		a[p] = pivot;
		sort_files(a, left, p-1);
		sort_files(a, p+1, right);
	}
}
////////////////////////////////////////////////////////////////////////

void getDir(const char *path, u32 ext) {
	int fd, b=0;
	
	nfiles = 0;
	
	if(device==0?strcmp(path,"ms0:/"):device==1?strcmp(path,"flash1:/"):device==2?strcmp(path,"flash0:/"):strcmp(path,"disc0:/")){
		strcpy(files[0].name,"..");
		files[0].type = TYPE_DIR;
		sortfiles[0] = files;
		nfiles = 1;
		b=1;
	}
	
	strcpy(path_files, path);
	fd = sceIoDopen(path);
	while(nfiles<MAX_ENTRY){
		memset(&files[nfiles], 0x00, sizeof(SceIoDirent));
		if(sceIoDread(fd, &files[nfiles])<=0) break;
		if(files[nfiles].name[0] == '.') continue;
		if(strrchr(files[nfiles].name, '.')==NULL){
			strcat(files[nfiles].name, "/");
			if(!stricmp(files[nfiles].name, files[nfiles-1].name)) continue;
			sortfiles[nfiles] = files + nfiles;
			files[nfiles].type = TYPE_DIR;
			nfiles++;
		}else{
			if(!stricmp(files[nfiles].name, files[nfiles-1].name)) continue;
			sortfiles[nfiles] = files + nfiles;
			files[nfiles].type = TYPE_FILE;
			nfiles++;
		}
	}
	sceIoDclose(fd);
	if(b)
		sort_files(sortfiles+1, 0, nfiles-2);
	else
		sort_files(sortfiles, 0, nfiles-1);
}

int getZipDirCallback(int nCallbackId, unsigned long ulExtractSize, unsigned long ulCurrentPosition,
                      const void *pData, unsigned long ulDataSize, unsigned long ulUserData)
{
	const char *pszFileName = (const char *)pData;
	
	switch(nCallbackId) {
	case UZCB_FIND_FILE:
		//if(getExtId(pszFileName)==EXT_GB){
			strcpy(zip_files[zip_nfiles].name, pszFileName);
			zip_sortfiles[zip_nfiles] = zip_files + zip_nfiles;
			zip_nfiles++;
		//}
		if(zip_nfiles >= MAX_ENTRY) return UZCBR_CANCEL;
		break;
	default: // unknown...
		pgFillvram(RGB(255,0,0));
		mh_print(0,0,"Unzip fatal error.",0xFFFF);
		pgScreenFlipV();
        break;
    }
	return UZCBR_PASS;
}

int getZipDirAll(const char *path)
{
	ROM_INFO stRomInfo;
	
	zip_nfiles = 0;
	path_files[0] = 0;
	path_inzip[0] = 0;

	Unzip_setCallback(getZipDirCallback);
	int ret = Unzip_execExtract(path, (unsigned long)&stRomInfo);
	if (ret != UZEXR_OK)
		zip_nfiles=0;

	sort_files(zip_sortfiles, 0, zip_nfiles-1);
	
	return zip_nfiles;
}

void getZipDir(const char *path)
{
	char *p;
	int i, len;
	
	strcpy(files[0].name,"..");
	files[0].type = TYPE_DIR;
	sortfiles[0] = files;
	nfiles = 1;
	
	len = strlen(path);
	for(i=0; i<zip_nfiles; i++){
		if(strncmp(zip_sortfiles[i]->name,path,len)) continue;
		strcpy(files[nfiles].name,zip_sortfiles[i]->name + len);
		p = strrchr(files[nfiles].name, '/');
		if(p){
			*(p+1) = 0;
			if(!strcmp(files[nfiles].name,files[nfiles-1].name)) continue;
			files[nfiles].type = TYPE_DIR;
		}else{
			files[nfiles].type = TYPE_FILE;
		}
		sortfiles[nfiles] = files + nfiles;
		nfiles++;
	}
	sort_files(sortfiles+1, 0, nfiles-2);
}


int getFileInfo(char *filepath)
{
	FILE * file;
	char ext[3], *p;
	long double filesize;
	int size;
	
	//File size
	file = fopen(filepath, "rb");
	fseek (file , 0 , SEEK_END);
  filesize = ftell (file);
  rewind (file);
  fclose(file);
  
  //File extension
	p = strrchr(filepath, '.');
	strcpy(ext, p+1);
	for(;;){
		readpad();
		if(new_pad){
			break;
		}
		rin_frame("File information", 0);
		mh_printf(4, 4, setting2.textcolor[3], filepath);
		if(filesize/1024>1){
			if((filesize/1024)/1024>1)
				mh_printf(5,5,setting2.textcolor[3],"Filesize    : %3.2lf MB",(filesize/1024)/1024);
			else
				mh_printf(5,5,setting2.textcolor[3],"Filesize    : %3.1lf KB",filesize/1024);
		}else{
			mh_printf(5,5,setting2.textcolor[3],"Filesize    : %d Byte",filesize);
		}
		if(!stricmp(ext, "zip"))
			mh_printf(5,6,setting2.textcolor[3],"Filetype    : Zip-Archive");
		else if(!stricmp(ext, "txt"))
			mh_printf(5,6,setting2.textcolor[3],"Filetype    : Text-Document");
		else if(!stricmp(ext, "tch"))
			mh_printf(5,6,setting2.textcolor[3],"Filetype    : RIN Cheat File");
		else if(!stricmp(ext, "gb"))
			mh_printf(5,6,setting2.textcolor[3],"Filetype    : Gameboy Rom");
		else if(!stricmp(ext, "gbc"))
			mh_printf(5,6,setting2.textcolor[3],"Filetype    : Gameboy Color Rom");
		else if(!stricmp(ext, "gba"))
			mh_printf(5,6,setting2.textcolor[3],"Filetype    : Gameboy Advance Rom");
		else if(!stricmp(ext, "png"))
			mh_printf(5,6,setting2.textcolor[3],"Filetype    : PNG Image");
		else if(!stricmp(ext, "bmp"))
			mh_printf(5,6,setting2.textcolor[3],"Filetype    : BMP Image");
		else if(!stricmp(ext, "tga"))
			mh_printf(5,6,setting2.textcolor[3],"Filetype    : TGA Image");
		else if(!stricmp(ext, "jpg"))
			mh_printf(5,6,setting2.textcolor[3],"Filetype    : JPEG Image");
		else if(!stricmp(ext, "gif"))
			mh_printf(5,6,setting2.textcolor[3],"Filetype    : GIF Image");
		else if(!stricmp(ext, "cfg"))
			mh_printf(5,6,setting2.textcolor[3],"Filetype    : Configuration File");
		else if(!stricmp(ext, "pbp"))
			mh_printf(5,6,setting2.textcolor[3],"Filetype    : EBOOT File");
		else if(!stricmp(ext, "mp3"))
			mh_printf(5,6,setting2.textcolor[3],"Filetype    : MP3 Audio File");
		else if(!stricmp(ext, "ogg"))
			mh_printf(5,6,setting2.textcolor[3],"Filetype    : OGG Audio File");
		else if(!stricmp(ext, "wav"))
			mh_printf(5,6,setting2.textcolor[3],"Filetype    : WAVE Audio File");
		else if(!stricmp(ext, "pbp"))
			mh_printf(5,6,setting2.textcolor[3],"Filetype    : EBOOT File");
		else
			mh_printf(5,6,setting2.textcolor[3],"Filetype    : Unknown");
			
		pgScreenFlipV();
	}
}


int input_name(int input_type, char *last_name, char *path, char *out)
{
	char name[66]={0}, tmp[MAX_PATH];
	int pos=0,x,y,i;
	int mode=0;

	strcpy(name, last_name);

  while(1)
  {
  	readpad();
		if(new_pad & CTRL_START){
			if(!input_type && strlen(name)!=0) {
				strcpy(tmp, path);
				strcat(tmp, name);
				strcpy(out, tmp);
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
		if(pos>45)
			pos=46;
			
		if(pos<0)
			pos=0;
			
		rin_frame("","START：Accept  SELECT：Mode");
		
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
			
			pgPrint(x-1, y+3, 0xffff, "Input name:");
			pgPrint(x, y+4, 0xffff, name);
			pgPrint(pos+x, y+4, 0xffff, "_");
		
		pgScreenFlipV();
	}
}


char            *data_ptr;       
unsigned long   data_size; 
   
char filer_msg[256]={0};
int getFilePath(char *fullpath, u32 ext)
{
	FILE * fd, * file;
	long filesize;
	int sel=0, top=0, rows=21, x, y, h, i, up=0, inzip=0, oldDirType, mode, move=0;
	char path[MAX_PATH], oldDir[MAX_NAME], tmp[MAX_PATH], tmp2[MAX_PATH], *p;
	
	path_inzip[0] = 0;
	
	dir=0;
	device=0;
	mode=0;
	strcpy(filecopyname, "");
	strcpy(path, fullpath);
	p = strrchr(path, '/');
	if (p){
		p++;
		strcpy(tmp, p);
		*p = 0;
	}
	else{
		strcpy(path, "ms0:/");
	}

	getDir(path, ext);

	if (tmp[0]){
		for(i=0; i<nfiles; i++){
			if (!stricmp(sortfiles[i]->name,tmp)){
				sel = i;
				top = i-3;
				break;
			}
		}
	}
	
	for(;;){
		readpad();
		if(new_pad)
			filer_msg[0]=0;
		if(mode==1){
			if(new_pad & CTRL_TRIANGLE){
				if(!inzip){
					input_name(0, "", path, tmp);
					sceIoMkdir(tmp,0777);
					getDir(path, ext);
					continue;
				}
			}else if(new_pad & CTRL_CIRCLE){
				if(sortfiles[sel]->type==TYPE_DIR){
					strcpy(tmp,"\"");
					strcat(tmp,sortfiles[sel]->name);
					strcat(tmp,"\"\n\nRemove?");
					if(rin_MessageBox(tmp,1)){
						strcpy(tmp, path);
						strcat(tmp, sortfiles[sel]->name);
						if(utils_del_dir(tmp)>0){
							strcpy(filer_msg,"Removed dir \"");
							strcat(filer_msg,sortfiles[sel]->name);
							strcat(filer_msg,"\"");
							getDir(path, ext);
							continue;
						}
					}
				}
				if(!inzip){
					strcpy(tmp,"\"");
					strcat(tmp,sortfiles[sel]->name);
					strcat(tmp,"\"\n\nRemove?");
					if(rin_MessageBox(tmp,1)){
						strcpy(tmp, path);
						strcat(tmp, sortfiles[sel]->name);
						if(sceIoRemove(tmp)>=0){
							strcpy(filer_msg,"Removed \"");
							strcat(filer_msg,sortfiles[sel]->name);
							strcat(filer_msg,"\"");
							getDir(path, ext);
						}
					}
				}
			}else if((new_pad & CTRL_CROSS)){
				input_name(0, sortfiles[sel]->name, path, tmp);
				strcpy(tmp2, path);
				strcat(tmp2, sortfiles[sel]->name);
				if(sceIoRename(tmp2, tmp)<0)
					strcpy(filer_msg, "Couldn't rename file");
				else{
					strcpy(filer_msg, "File has been successfully renamed");
				}
				getDir(path, ext);
			}
		}else if(mode==0){
			if(new_pad & CTRL_CROSS){
				if(getExtId(sortfiles[sel]->name) == EXT_TCH && fullpath != CheatPath && !inzip) {
					//strcat(filer_msg, "Please use Load Cheat File option from the menu");
					//continue;
				}
					
				if(getExtId(sortfiles[sel]->name) == EXT_IMG && !inzip)
				{
					strcpy(tmp, path);
					strcat(tmp, sortfiles[sel]->name);
					image_viewer(tmp);
				}else	if((getExtId(sortfiles[sel]->name) == EXT_TXT || (getExtId(sortfiles[sel]->name) == EXT_TCH & fullpath != CheatPath) && !inzip))
				{
					strcpy(tmp, path);
					strcat(tmp, sortfiles[sel]->name);
					text_reader(tmp);
				}else if(getExtId(sortfiles[sel]->name) == EXT_OGG && !inzip){
					strcpy(tmp,path);
					strcat(tmp,sortfiles[sel]->name);
					add_entry_pl(tmp, 1);
				}else if(getExtId(sortfiles[sel]->name) == EXT_MP3 && !inzip){
					strcpy(tmp, path);
					strcat(tmp, sortfiles[sel]->name);
					add_entry_pl(tmp, 0);
				}else if(sortfiles[sel]->type == TYPE_DIR){
					if(!strcmp(sortfiles[sel]->name,"..")){
						up=1;
					}else{
						if(inzip){
							strcat(path_inzip,sortfiles[sel]->name);
							getZipDir(path_inzip);
						}else{
							strcat(path,sortfiles[sel]->name);
							getDir(path, ext);
						}
					sel=0;
					}
				}else if(!inzip && getExtId(sortfiles[sel]->name) == EXT_GB || getExtId(sortfiles[sel]->name) == EXT_ZIP || getExtId(sortfiles[sel]->name) == EXT_TCH){
						strcpy(tmp,path);
						strcat(tmp,sortfiles[sel]->name);
						if (getExtId(tmp)==EXT_ZIP){
							getZipDirAll(tmp);
							if(zip_nfiles!=1){
								strcat(path,sortfiles[sel]->name);
								getZipDir(path_inzip);
								sel=0;
								inzip=1;
							}else
								break;
						}else
							break;
					}
		}else if(new_pad & CTRL_SQUARE){
			strcpy(tmp,path);
			strcat(tmp,sortfiles[sel]->name);
			getFileInfo(tmp);
		}else if(new_pad & CTRL_CIRCLE){
			return 0;
		}else if(new_pad & CTRL_TRIANGLE){
			up=1;
		}
	}else if(mode==2){
		if(new_pad & CTRL_CROSS){
			if(move){
				p=strrchr(filecopyname, '/');
				strcpy(tmp, path);
				strcat(tmp, p+1);
				sceIoRename(filecopyname, tmp);
				getDir(path, ext);
				dir=0;
				continue;
			}
			if(!inzip){
				if(!strcmp(filecopyname, "") || filecopyname==NULL){
					move=0;
					strcpy(filecopyname,path);
					if(sortfiles[sel]->type == TYPE_DIR)
						strncat(filecopyname,sortfiles[sel]->name, strlen(sortfiles[sel]->name)-1);
					else
						strcat(filecopyname,sortfiles[sel]->name);
					strcpy(filer_msg,"Copy ");
					strcat(filer_msg,filecopyname);
					continue;
				}else{
					p=strrchr(filecopyname, '/');
					strcpy(tmp, path);
					strcat(tmp, p+1);
					if(!strcmp(filecopyname, tmp)){ 
						strcpy(filer_msg,"Can't copy to the same directory!");
						continue;
					}
					file = fopen(tmp, "rb");
					if(file!=NULL){						
						if(rin_MessageBox("Overwrite file?",1)){
							sceIoRemove(tmp);
							pgCopyFile(filecopyname, tmp);
							strcpy(filecopyname, "");
						}
					}else{
						pgCopyFile(filecopyname, tmp);
						strcpy(filecopyname, "");
					}
					fclose(file);
					getDir(path, ext);
					dir=0;
					continue;
				}
			}
		}else if(new_pad & CTRL_CIRCLE){
			strcpy(filecopyname, "");
		}else if(new_pad & CTRL_TRIANGLE){
			if(!inzip){
				if(!strcmp(filecopyname, "") || filecopyname==NULL){
					move=1;
					strcpy(filecopyname,path);
					if(sortfiles[sel]->type == TYPE_DIR)
						strncat(filecopyname,sortfiles[sel]->name, strlen(sortfiles[sel]->name)-1);
					else
						strcat(filecopyname,sortfiles[sel]->name);
					strcpy(filer_msg,"Move ");
					strcat(filer_msg,filecopyname);
					continue;
				}
			}
		}
	}
		
		if(new_pad & CTRL_LTRIGGER){
			device++;
			if(device>3) device=0;
			switch(device){
				case 0:
					strcpy(path, "ms0:/");
					getDir("ms0:/", ext);
					break;
				case 1:
					strcpy(path, "flash1:/");
					getDir("flash1:/", ext);
					break;
				case 2:
					strcpy(path, "flash0:/");
					getDir("flash0:/", ext);
					break;
				case 3:
					strcpy(path, "disc0:/");
					getDir("disc0:/", ext);
					break;
			}
		}else if(new_pad & CTRL_RTRIGGER){
			mode++;
			if(mode>2) mode=0;
		}else if(new_pad & CTRL_UP){
			sel--;
		}else if(new_pad & CTRL_DOWN){
			sel++;
		}else if(new_pad & CTRL_LEFT){
			sel-=rows/2;
		}else if(new_pad & CTRL_RIGHT){
			sel+=rows/2;
		}
		
		if(up){
			oldDir[0]=0;
			oldDirType = TYPE_DIR;
			if(inzip){
				if(path_inzip[0]==0){
					oldDirType = TYPE_FILE;
					inzip=0;
				}else{
					path_inzip[strlen(path_inzip)-1]=0;
					p = strrchr(path_inzip,'/');
					if (p)
						p++;
					else
						p = path_inzip;
					sprintf(oldDir,"%s/", p);
					*p = 0;
					getZipDir(path_inzip);
					sel=0;
				}
			}
			if(device==0?strcmp(path,"ms0:/"):device==1?strcmp(path,"flash1:/"):device==2?strcmp(path,"flash0:/"):strcmp(path,"disc0:/") && !inzip){
				if(oldDirType==TYPE_DIR)
					path[strlen(path)-1]=0;
				p=strrchr(path,'/')+1;
				strcpy(oldDir,p);
				if(oldDirType==TYPE_DIR)
					strcat(oldDir,"/");
				*p=0;
				getDir(path, ext);
				sel=0;
			}
			for(i=0; i<nfiles; i++) {
				if(oldDirType==sortfiles[i]->type && !strcmp(oldDir, sortfiles[i]->name)) {
					sel=i;
					top=sel-3;
					break;
				}
			}
			up=0;
		}
		if(top > nfiles-rows)	top=nfiles-rows;
		if(top < 0)				top=0;
		if(sel >= nfiles)		sel=nfiles-1;
		if(sel < 0)				sel=0;
		if(sel >= top+rows)		top=sel-rows+1;
		if(sel < top)			top=sel;
		
		if(inzip){
			sprintf(tmp,"%s:/%s",strrchr(path,'/')+1,path_inzip);
			rin_frame(tmp,"×：OK  ○：Cancel  △：UP");
		}else{
			
			if(mode==0)
				rin_frame(filer_msg[0]?filer_msg:path,"×：OK  ○：Cancel  △：Up");
			else if(mode==1)
				rin_frame(filer_msg[0]?filer_msg:path,"×：Rename  ○：Delete  △：Make Dir");
			else if(mode==2){
				if(!strcmp(filecopyname, "") || filecopyname==NULL)
					rin_frame(filer_msg[0]?filer_msg:path,"×：Copy  △：Move");
				else
					rin_frame(filer_msg[0]?filer_msg:path,"×：Paste  ○：Cancel");
			}
		}
		// スクロールバー
		if(nfiles > rows){
			h = 219;
			pgDrawFrame(445,25,446,248,setting2.textcolor[1]);
			pgFillBox(448, h*top/nfiles + 27,
				460, h*(top+rows)/nfiles + 27,setting2.textcolor[1]);
		}
		
		x=28; y=32;
		for(i=0; i<rows; i++){
			if(top+i >= nfiles) break;
			mh_print(x, y, sortfiles[top+i]->name, setting2.textcolor[top+i==sel?2:3]);
			y+=10;
		}
		pgScreenFlipV();
	}
	
	strcpy(fullpath, path);
	strcat(inzip?path_inzip:fullpath, sortfiles[sel]->name);
	return 1;
}
