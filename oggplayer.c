#include "INC/main.h"
#include "INC/oggplayer.h"


#define min(a,b) (((a)<(b))?(a):(b))
#define max(a,b) (((a)>(b))?(a):(b))
#define INPUT_BUFFER_SIZE	(5*8192)
#define OUTPUT_BUFFER_SIZE	4096	/* Must be an integer multiple of 4. */
/* Define //printf, just to make typing easier */
//#define //printf	pspDebugScreen//printf

OggVorbis_File vf;
int eof = 0;
int current_section;
char **oggComments;
vorbis_info *vi;
//int errno;			// __errno;
static int isPlaying;		// Set to true when a mod is being played
static int myChannel;
int fd = 0;

static int eos;

/*
#define PREBUFFER	2097152
static short outputBuffer[PREBUFFER];
u32 outputBufferLevel;
*/
/*void OGGsetStubs(codecStubs * stubs)
{
    stubs->init = OGG_Init;
    stubs->load = OGG_Load;
    stubs->play = OGG_Play;
    stubs->pause = OGG_Pause;
    stubs->stop = OGG_Stop;
    stubs->end = OGG_End;
    stubs->time = OGG_GetTimeString;
    stubs->tick = NULL;
    stubs->eos = OGG_EndOfStream;
    memcpy(stubs->extension, "ogg\0" "\0\0\0\0", 2 * 4);
}*/


static void OGGCallback(void *_buf2, unsigned int numSamples, void *pdata)
{
	char *tmp_ret;
	short *_buf = (short *)_buf2;
	static short tempmixbuf[PSP_NUM_AUDIO_SAMPLES * 2 * 2] __attribute__ ((aligned(64)));
	static unsigned long tempmixleft = 0;
	char *p;
	
	// Playing , so mix up a buffer
	if (isPlaying == TRUE)
	{
		//  Not enough in buffer, so we must mix more
		while (tempmixleft < numSamples)
		{
			unsigned long bytesRequired = (numSamples - tempmixleft) * 4;	// 2channels, 16bit = 4 bytes per sample
			unsigned long ret = ov_read(&vf, (char *) &tempmixbuf[tempmixleft * 2], bytesRequired, &current_section);
	    
			// EOF
	    if (ret == 0)
			{
				eos = 1;
					
				/*eos = 1;
				if (ov_pcm_seek_page(&vf, 0) != 0)
				{
					//pgPrint(0, 0, 0xffff, "Could not seek to start of file\n");
					OGG_End();
				}
				*/
				return;
			}
			else if (ret < 0)
			{
				//pgPrintf(0, 0, 0xffff, "Error occured during ov_read: %d\n", ret);
				sceKernelDelayThread(500000);
				
				return;
			}
			
			tempmixleft += ret / 4;	// back down to sample num
		}
		
		// Buffer has enough, so copy across
		if (tempmixleft >= numSamples)
		{
			//pgPrintf(0, 0, 0xffff, "Buffer has enough, so copy across");
			int count, count2;
			short *_buf2;
			
			for (count = 0; count < numSamples; count++)
			{
				count2 = count + count;
				_buf2 = _buf + count2;
				
				// Double up for stereo
				*(_buf2) = tempmixbuf[count2];
				*(_buf2 + 1) = tempmixbuf[count2 + 1];
	    }

			//  Move the pointers
			tempmixleft -= numSamples;
			
			//  Now shuffle the buffer along
			for (count = 0; count < tempmixleft; count++){
				//pgPrintf(0, 0, 0xffff, "Now shuffle the buffer along");
				tempmixbuf[count] = tempmixbuf[numSamples + count];
			}
		}

	}
	//  Not Playing , so clear buffer
	else
	{			
		int count;
		//pgPrintf(0, 0, 0xffff, "Clear buffer");
		for (count = 0; count < numSamples * 2; count++)
	    *(_buf + count) = 0;
	}
}

//////////////////////////////////////////////////////////////////////
// Functions - Local and not public
//////////////////////////////////////////////////////////////////////

size_t ogg_callback_read(void *ptr, size_t size, size_t nmemb, void *datasource)
{
	return sceIoRead(*(int *) datasource, ptr, size * nmemb);
}

int ogg_callback_seek(void *datasource, ogg_int64_t offset, int whence)
{
	return sceIoLseek32(*(int *) datasource, (unsigned int) offset, whence);
}

long ogg_callback_tell(void *datasource)
{
	return sceIoLseek32(*(int *) datasource, 0, SEEK_CUR);
}

int ogg_callback_close(void *datasource)
{
	return sceIoClose(*(int *) datasource);
}

int OGG_Load(char *filename)
{
	int size = 0;
	eos = 0;
	isPlaying = 0;	
	ov_callbacks ogg_callbacks;

	ogg_callbacks.read_func = ogg_callback_read;
	ogg_callbacks.seek_func = ogg_callback_seek;
	ogg_callbacks.close_func = ogg_callback_close;
	ogg_callbacks.tell_func = ogg_callback_tell;

	if ((fd = sceIoOpen(filename, SCE_O_RDONLY, 0777)) <= 0)
	{
		////printf("could not open file %s\n", filename);
		sceDisplayWaitVblankStart();
		sceDisplayWaitVblankStart();
		sceKernelDelayThread(500000);

		return 0;
	}

	if (ov_open_callbacks(&fd, &vf, NULL, 0, ogg_callbacks) < 0)
	{
		//printf("Input does not appear to be an Ogg bitstream.\n");
		sceKernelDelayThread(500000);
		
		return 0;
	}
	else
	{
		oggComments = ov_comment(&vf, -1)->user_comments;
		vi = ov_info(&vf, -1);
		
		//pspDebugScreenSetXY(0, 29);
		//printf("%d channel, %lu kb/s %s OGG Vorbis audio stream at %ldHz\n", vi->channels, vi->bitrate_nominal / 1000,
		//       vi->bitrate_upper == vi->bitrate_nominal ? "CBR" : "VBR", vi->rate);
		//printf("Encoded by: %s\n", ov_comment(&vf, -1)->vendor);
	}
	
	return 1;
}

void OGG_Init(int channel)
{
	myChannel = 1;
	isPlaying = FALSE;

	pspAudioSetChannelCallback(myChannel, OGGCallback,0);
}

// This function initialises for playing, and starts
int OGG_Play()
{
	// See if I'm already playing
	isPlaying = !isPlaying;
}

int OGG_Playing()
{
	return isPlaying;
}

int OGG_Stop()
{
	//stop playing
	isPlaying = FALSE;
	return TRUE;
}

void OGG_Pause(int mode)
{
	if(mode!=0 && mode!=1)
		isPlaying = isPlaying;
	else
		isPlaying = !mode;
}

void OGG_FreeTune()
{
	ov_clear(&vf);
	
	if (fd)
		sceIoClose(fd);
}

void OGG_End()
{
	OGG_Stop();
	pspAudioSetChannelCallback(myChannel, 0, 0);
	OGG_FreeTune();
}

void OGG_GetTimeString(char *dest)
{
	//extern ogg_int64_t ov_time_tell(OggVorbis_File *vf);
	unsigned int time = (unsigned int) ov_time_tell(&vf);
#define F_MULT 1000
#define S_MULT 60
#define M_MULT 60
#define H_MULT 60
    unsigned int timeS, timeM, timeH, timeF;
    timeF = time % F_MULT;
    time -= timeF;
    time /= F_MULT;
    timeS = time % S_MULT;
    time -= timeS;
    time /= S_MULT;
    timeM = time % M_MULT;
    time -= timeM;
    time /= M_MULT;
    timeH = time;
    sprintf(dest, "%02d:%02d", timeM, timeS);
}

int OGG_EndOfStream()
{
	if (eos == 1)
		return 1;
	
	return 0;
}

int OGG_SetVolume(int newvolume)
{
	//sceAudioChangeChannelVolume(myChannel, PSP_AUDIO_VOLUME_MAX * (newvolume * 10) / 100, PSP_AUDIO_VOLUME_MAX * (newvolume * 10) / 100);
}
