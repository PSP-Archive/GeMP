#ifndef MAIN_H
#define MAIN_H

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <limits.h>
#include <errno.h>
#include <pspaudio.h>
#include <pspaudiolib.h>

#include "zlib.h"
#include "id3tag.h"
#include "png.h"
#include "jpeglib.h"
#include "mad.h"
 
#include "syscall.h"
#include "pg.h"
#include "../GB/gb.h"
#include "image.h"
#include "sound.h"
#include "filer.h"
#include "menu.h"
#include "saveload.h"
#include "utils.h"
#include "mp3.h"

#include "music_gui.h"

extern char RinPath[], RomPath[], SavePath[], RomName[], CheatPath[], ScreenPath[], PlaylistPath[];
extern int bMenu, bSleep, bMusic;

void set_cpu_clock(int n);

#endif
