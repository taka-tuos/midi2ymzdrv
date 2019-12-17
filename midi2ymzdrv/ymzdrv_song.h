typedef struct {
	unsigned char ev,ch,d1,d2;
	unsigned short wf;
} ymzdrv_song_t;

#define YMZDRVSONG(name) ymzdrvsng_ ## name
