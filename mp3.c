/***************************************************************************

	mp3.c

	PSP MP3 thread functions.

***************************************************************************/

#include "INC/main.h"
#include "INC/mad.h"

#define MP3_SAMPLES			PSP_AUDIO_SAMPLE_ALIGN(4096)
#define MP3_BUFFER_SIZE		(MP3_SAMPLES * 4)


/***************************************************************************
	Private variables
 **************************************************************************/

static char MP3_file[MAX_PATH];

static volatile int mp3_active;
static volatile int mp3_running;
static volatile int mp3_status;
static volatile int mp3_sleep;

static int mp3_handle;
static int mp3_thread;

static u8 mp3_out[2][MP3_BUFFER_SIZE];
static u8 mp3_in[(2 * MP3_BUFFER_SIZE) + MAD_BUFFER_GUARD];

static int mp3_newfile;
static int mp3_fsize;
static int mp3_filepos;
static int mp3_volume;

static char mp3_tag[128];

int mp3_frame;
int mp3_start_frame;
static int mp3_end_status;

static FILE *mp3_fp = NULL;

struct mad_stream Stream;
struct mad_frame Frame;
struct mad_synth Synth;
mad_timer_t Timer;


/*--------------------------------------------------------
	MP3 clipping sample data
 -------------------------------------------------------*/

s16 Limit(mad_fixed_t value)
{
	if (value >=  MAD_F_ONE) return 32767;
	if (value <= -MAD_F_ONE) return -32767;

	return (s16)(value >> MAD_F_FRACBITS - 15);
}

/*--------------------------------------------------------
	MP3 update audio stream
 -------------------------------------------------------*/

static void MP3Update(void)
{
	char *p;
	int flip;
	u8 *GuardPtr;
	s16 *OutputPtr, *OutputEnd;

	mad_stream_init(&Stream);
	mad_frame_init(&Frame);
	mad_synth_init(&Synth);
	mad_timer_reset(&Timer);

	OutputPtr = (s16 *)mp3_out[0];
	OutputEnd = (s16 *)(mp3_out[0] + MP3_BUFFER_SIZE);
	GuardPtr = NULL;

	mp3_filepos = 0;
	mp3_frame = 0;
	flip = 0;

	while (mp3_active && mp3_status != MP3_STOP && mp3_status != MP3_END)
	{
		if (Stream.buffer == NULL || Stream.error == MAD_ERROR_BUFLEN)
		{
			u32 ReadSize, Remaining;
			u8 *ReadStart;

			if (Stream.next_frame != NULL)
			{
				Remaining = Stream.bufend - Stream.next_frame;
				ReadStart = mp3_in + Remaining;
				ReadSize  = (2 * MP3_BUFFER_SIZE) - Remaining;
				memmove(mp3_in, Stream.next_frame, Remaining);
			}
			else
			{
				ReadSize  = 2 * MP3_BUFFER_SIZE;
				ReadStart = mp3_in;
				Remaining = 0;
			}


			ReadSize = fread(ReadStart, 1, ReadSize, mp3_fp);
			mp3_filepos += ReadSize;
			if (mp3_filepos == mp3_fsize)
			{
				mp3_status = MP3_END;
			}

			if (mp3_filepos == mp3_fsize)
			{
				GuardPtr = ReadStart + ReadSize;
				memset(GuardPtr, 0, MAD_BUFFER_GUARD);
				ReadSize += MAD_BUFFER_GUARD;
			}

			mad_stream_buffer(&Stream, mp3_in, ReadSize + Remaining);

			Stream.error = 0;
		}

		if (mad_frame_decode(&Frame, &Stream))
		{
			if (MAD_RECOVERABLE(Stream.error))
			{
				if (Stream.error != MAD_ERROR_LOSTSYNC || Stream.this_frame != GuardPtr)
				{
					sceDisplayWaitVblankStart();
				}
				continue;
			}
			else if (Stream.error == MAD_ERROR_BUFLEN)
			{
				continue;
			}
			else
			{
				break;
			}
	  	}

		mp3_frame++;
		mad_timer_add(&Timer, Frame.header.duration);
		mad_synth_frame(&Synth, &Frame);

		if (mp3_status == MP3_PLAY)
		{
			int i;

			for (i = 0; i < Synth.pcm.length; i++)
			{
				s16 lt, rt;

				lt = Limit(Synth.pcm.samples[0][i]);

				if (MAD_NCHANNELS(&Frame.header) == 2)
					rt = Limit(Synth.pcm.samples[1][i]);
				else
					rt = lt;

				*OutputPtr++ = lt;
				*OutputPtr++ = rt;

				if (OutputPtr == OutputEnd)
				{
					sceAudioOutputPannedBlocking(mp3_handle, mp3_volume, mp3_volume, mp3_out[flip]);
					flip ^= 1;
					OutputPtr = (s16 *)mp3_out[flip];
					OutputEnd = (s16 *)(mp3_out[flip] + MP3_BUFFER_SIZE);
				}
			}
		}

		if (mp3_status == MP3_SEEK && mp3_frame >= mp3_start_frame)
		{
			mp3_start_frame = 0;
			mp3_status = mp3_end_status;
		}
	}

	mad_synth_finish(&Synth);
	mad_frame_finish(&Frame);
	mad_stream_finish(&Stream);

	if (mp3_fp)
	{
		fclose(mp3_fp);
		mp3_fp = NULL;
	}
}


/*--------------------------------------------------------
	MP3 stream update thread
 -------------------------------------------------------*/

static int MP3Thread(int args, void *argp)
{
	while (mp3_active)
	{
		sceKernelSleepThread();

		if (mp3_newfile)
		{
			mp3_newfile = 0;

			mp3_running = 1;
			MP3Update();
			mp3_running = 0;
		}
	}

	return 0;
}



/*--------------------------------------------------------
	Start MP3 thread
 -------------------------------------------------------*/

int mp3_thread_start(void)
{
	mp3_handle  = 1;
	mp3_thread  = -1;
	mp3_active  = 0;
	mp3_status  = MP3_STOP;
	mp3_running = 0;
	mp3_sleep   = 0;

	mp3_newfile = 0;
	mp3_volume  = 0;

	memset(mp3_out[0], 0, MP3_BUFFER_SIZE);
	memset(mp3_out[1], 0, MP3_BUFFER_SIZE);

	mp3_handle = sceAudioChReserve(PSP_AUDIO_NEXT_CHANNEL, MP3_SAMPLES, PSP_AUDIO_FORMAT_STEREO);
	if (mp3_handle < 0)
	{
		return 0;
	}

	mp3_active = 1;

	mp3_thread = sceKernelCreateThread("MP3 thread", MP3Thread, 0x8, 0x40000, 0, NULL);
	if (mp3_thread < 0)
	{
		sceAudioChRelease(mp3_handle);
		return 0;
	}

	sceKernelStartThread(mp3_thread, 0, 0);

	return 1;
}


/*--------------------------------------------------------
	Stop sound thread
 -------------------------------------------------------*/

void mp3_thread_stop(void)
{
	if (mp3_thread >= 0)
	{
		mp3_active = 0;
		mp3_stop();

		sceKernelWakeupThread(mp3_thread);
		sceKernelWaitThreadEnd(mp3_thread, NULL);

		sceKernelDeleteThread(mp3_thread);
		mp3_thread = -1;

		sceAudioChRelease(mp3_handle);
		mp3_handle = -1;
	}
}


/*--------------------------------------------------------
	Set MP3 volume
 -------------------------------------------------------*/

void mp3_set_volume(int newvolume)
{
	mp3_volume = PSP_AUDIO_VOLUME_MAX * (newvolume * 10) / 100;
}

int mp3_playing()
{
	return mp3_active;
}

/*--------------------------------------------------------
	Play MP3
 -------------------------------------------------------*/

int mp3_play(const char *name)
{
	if (mp3_thread >= 0)
	{
		strcpy(MP3_file, name);

		mp3_stop();


		if ((mp3_fp = fopen(MP3_file, "rb")))
		{
			fseek(mp3_fp, 0, SEEK_END);
			mp3_fsize = ftell(mp3_fp);
			fseek(mp3_fp, 0, SEEK_SET);

			mp3_status = MP3_PLAY;
			mp3_newfile = 1;
			mp3_set_volume(10);

			sceKernelWakeupThread(mp3_thread);
			return 0;
		}
	}
	return 1;
}


/*--------------------------------------------------------
	Stop MP3
 -------------------------------------------------------*/

int mp3_stop()
{
	if (mp3_thread >= 0)
	{
		mp3_volume = 0;

		if (mp3_status == MP3_PAUSE)
			sceKernelResumeThread(mp3_thread);

		mp3_status = MP3_STOP;
		while (mp3_running) wait(100);

		memset(mp3_out[0], 0, MP3_BUFFER_SIZE);
		memset(mp3_out[1], 0, MP3_BUFFER_SIZE);
		sceAudioOutputPannedBlocking(mp3_handle, 0, 0, mp3_out[0]);
		return 1;
	}
}


/*--------------------------------------------------------
	Pause MP3
 -------------------------------------------------------*/

void mp3_pause(int pause)
{
	if (mp3_thread >= 0)
	{
		if (mp3_running)
		{
			if (pause)
			{
				mp3_status = MP3_PAUSE;
				sceKernelSuspendThread(mp3_thread);
				memset(mp3_out[0], 0, MP3_BUFFER_SIZE);
				memset(mp3_out[1], 0, MP3_BUFFER_SIZE);
				sceAudioOutputPannedBlocking(mp3_handle, 0, 0, mp3_out[0]);
			}
			else
			{
				mp3_file_reopen();
				mp3_status = MP3_PLAY;
				sceKernelResumeThread(mp3_thread);
			}
		}
	}
}


/*--------------------------------------------------------
	Seek set
 -------------------------------------------------------*/

void mp3_seek_set(const char *name, int frame, int pause)
{
	if (mp3_thread >= 0)
	{
		strcpy(MP3_file, name);

		mp3_stop();

		if ((mp3_fp = fopen(MP3_file, "rb")))
		{
			fseek(mp3_fp, 0, SEEK_END);
			mp3_fsize = ftell(mp3_fp);
			fseek(mp3_fp, 0, SEEK_SET);

			mp3_status = MP3_SEEK;
			mp3_start_frame = frame;
			mp3_end_status = pause ? MP3_PAUSE : MP3_PLAY;
			mp3_newfile = 1;
		}
	}
}


/*--------------------------------------------------------
	Seek start
 -------------------------------------------------------*/

void mp3_seek_start(void)
{
	if (mp3_thread >= 0)
	{
		mp3_set_volume(10);
		sceKernelWakeupThread(mp3_thread);
	}
}


/*--------------------------------------------------------
	MP3 file reopen
 -------------------------------------------------------*/

void mp3_file_reopen(void)
{
	if (mp3_sleep)
	{
		mp3_sleep = 0;

		if (mp3_thread >= 0 && mp3_running)
		{
			fclose(mp3_fp);

			if ((mp3_fp = fopen(MP3_file, "rb")) == NULL)
			{
				mp3_status = MP3_STOP;
				return;
			}

			fseek(mp3_fp, mp3_filepos, SEEK_SET);
		}
	}
}


/*--------------------------------------------------------
	Get current MP3 stream frame count
 -------------------------------------------------------*/

int mp3_get_current_frame(void)
{
	return mp3_status ? mp3_frame : 0;
}


/*--------------------------------------------------------
	Get current MP3 status
 -------------------------------------------------------*/

int mp3_get_status(void)
{
	return mp3_status;
}


/*--------------------------------------------------------
	Set sleep flag
 -------------------------------------------------------*/

void mp3_set_sleep_flag(void)
{
	mp3_sleep = 1;
}

void mp3_get_timer_string(char *dest)
{
  mad_timer_string(Timer, dest, "%02u:%02u", MAD_UNITS_MINUTES, MAD_UNITS_SECONDS, 0);
}
