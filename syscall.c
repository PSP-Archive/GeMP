#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <string.h>
#include <assert.h>
#include <errno.h>
#include <fcntl.h>

#include "INC/syscall.h"

static int hex2num(char c)
{
	if (c >= '0' && c <= '9')
		return c - '0';
	if (c >= 'a' && c <= 'f')
		return c - 'a' + 10;
	if (c >= 'A' && c <= 'F')
		return c - 'A' + 10;
	return 0;
}

char cwd[MAX_PATH]={0};

char *getcwd(char *buf, size_t size)
{
	strcpy(buf,cwd);
	return buf;
}

int chdir(const char *path)
{
	int ret = sceIoChdir(path);
	if(ret==0) strcpy(cwd,path);
	return ret;
}

void _exit(int status)
{
	for(;;)
		sceKernelExitGame();
}

int unlink(const char *pathname)   
{
	return sceIoRemove(pathname);
}

static inline fdcnv(int fd)
{
	switch(fd) {
	case 0:
		return sceKernelStdin();
	case 1:
		return sceKernelStdout();
	case 2:
		return sceKernelStderr();
	default:
		return fd;
	}
}

int open(const char *pathname, int flags, ...)
{
	return sceIoOpen(pathname, flags, 644);
}

int close(int fd)
{
	fd = fdcnv(fd);
	if(fd<0) return -1;
	return sceIoClose(fd);
}

/*long read(int fd, void *buf, size_t count)
{
	fd = fdcnv(fd);
	if(fd<0) return -1;
	return sceIoRead(fd, buf, count);
}

long write(int fd, const void *buf, size_t count)
{
	fd = fdcnv(fd);
	if(fd<0) return -1;
	return sceIoWrite(fd, (void*)buf, count);
}*/

off_t lseek(int fd, off_t offset, int dir)
{
	fd = fdcnv(fd);
	if(fd<0) return -1;
	return sceIoLseek(fd, offset, dir);
}

int fstat(int fd, struct stat *st)
{
	int cur = sceIoLseek(fd,0,SEEK_CUR);
	int size = sceIoLseek(fd,0,SEEK_END);
	sceIoLseek(fd,cur,SEEK_SET);
	
	memset(st,0,sizeof(*st));
	st->st_mode = S_IFCHR;
	st->st_size = size;
	return 0;
}

int stat(const char *file_name, struct stat *buf)
{
	//struct dirent de;
	//sceIoGetStat(file_name, &de);
	return -1;
}

int isatty(int desc)
{
	return 0;
}

#define SBRK_SIZE 1*1024*1024
static char alloc_buffer[SBRK_SIZE];
void* sbrk(ptrdiff_t incr)
{
	extern char end;
	static char *heap_end = alloc_buffer;
	static int total;
	char *prev_heap_end;
	
	if(heap_end < alloc_buffer+SBRK_SIZE){
		prev_heap_end = heap_end;
		heap_end += incr;
		total += incr;
		return (caddr_t) prev_heap_end;
	}else{
		return (caddr_t) -1;
	}
}

int kill(int pid, int sig)
{
	return -1;
}

int getpid(void)
{
	return 1;
}

time_t time(time_t *tm)
{
	return sceKernelLibcTime((unsigned long*)tm);
}

double pow(double x, double y)
{
	long ret=1;
	while(y-->0)
		ret*=x;
	return ret;
}
