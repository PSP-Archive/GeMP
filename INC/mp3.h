/***************************************************************************

	mp3.c

	PSP MP3 thread functions.

***************************************************************************/

#ifndef PSP_MP3_H
#define PSP_MP3_H

typedef signed short s16;

enum
{
	MP3_STOP = 0,
	MP3_SEEK,
	MP3_PAUSE,
	MP3_PLAY,
	MP3_END
};

int mp3_thread_start(void);
void mp3_thread_stop(void);
void mp3_set_volume(int newvolume);

int mp3_play(const char *name);
int mp3_stop();
void mp3_pause(int pause);
void mp3_seek_set(const char *fname, int frame, int pause);
void mp3_seek_start(void);
void mp3_file_reopen(void);

int mp3_get_current_frame();
int mp3_get_status(void);
void mp3_set_sleep_flag(void);
void mp3_update(void);

#endif /* PSP_MP3_H */
