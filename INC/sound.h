
extern int wavout_enable;
extern unsigned long cur_play;

//�T�E���h�o�b�t�@�P�o���N������̗e�ʁB�S�o���N�œK���Ƀ��E���h���r��
//PGA_SAMPLES�̔{���ɂ��邱�ƁBPGA_SAMPLES�Ɠ������Ƒ����_���Ȃ̂Œ��ӁB - LCK
#define MAX_SOUND_BANKLEN 2048

extern short sound_buf[MAX_SOUND_BANKLEN*4*2];

void wavoutClear();
int wavoutInit();

