#ifndef MUSIC_GUI
#define MUSIC_GUI

#define MAX_PL 256

//char playlist[MAX_PL][MAX_PATH], playlist_tmp[MAX_PL][MAX_PATH];

unsigned short imgskin[480*272], imgbutton1[480*272], imgbutton2[480*272], imgbutton3[480*272], imgbutton4[480*272], imgbutton5[480*272], imgvolumeo[480*272], imgvolume[480*272], imgbalanceo[480*272], imgbalance[480*272], imgplaylistscr[480*272], imgplaylistscrt[480*272], imgplaylistscrb[480*272], imgpointer[480*272];

Image *skin_bg, *button1, *button2, *button3, *button4, *button5, *volumeo, *volume, *balanceo, *balance, *playlistscroll, *playlistscrollt, *playlistscrollb, *pointer;

int a_y, a_x;

char skin_path[MAX_PATH];

//Prev button
int button1_x;
int button1_y;

//Play button
int button2_x;
int button2_y;

//Pause button
int button3_x;
int button3_y;

//Stop button
int button4_x;
int button4_y;

//Next button
int button5_x;
int button5_y;

//Volume slider
int volume_x;
int volume_y;

//Volume Object
int volumeo_x;
int volumeo_y;

//Balance slider
int balance_x;
int balance_y;

//Balance object
int balanceo_x;
int balanceo_y;

//Playlist
int playlist_x;
int playlist_y;

//Playlist scrollbar
int playlists_x;
int playlists_y;

//Playlist cursor top
int playlistt_x;
int playlistt_y;

//Playlist cursor bottom
int playlistb_x;
int playlistb_y;

enum {
	PREV_B = 1,
	PLAY_B,
	PAUSE_B,
	STOP_B,
	NEXT_B,
	VOLUME,
	BALANCE,
	PLAYLIST,
	PLAYLISTT,
	PLAYLISTB,
};

typedef struct
{
	//0 = Song, 1 = Once playlist, 2 = Playlist
	int repeat;
	int shuffle;
} Music_Config;

typedef struct
{
	char name[MAX_PATH];
	int type;
} Playlist[MAX_PL];

Music_Config music_config;
Playlist playlist;

int last_id, current_id;

void load_music_config(void);
void save_music_config(void);
int music_main();
int add_entry_pl(char *filepath, int type);
int remove_entry_pl(int num);

#endif
