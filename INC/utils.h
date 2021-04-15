#ifndef _UTILS_H_
#define _UTILS_H_

extern dword utils_dword2string(dword dw, char * dest, dword width);
extern const char * utils_fileext(const char * filename);
extern int utils_del_dir(char * dir);

#endif
