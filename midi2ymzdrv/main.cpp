#include <stdio.h>
#include <conio.h>

#include <windows.h> 
#include <mmsystem.h> 

#pragma comment(lib, "winmm.lib")

bool midi_closed = false;

LONG frame = 0;
LONG beginframe = -1;

FILE *output;

UINT evcount = 0;

void MidiDataRecieved(UINT stat, UINT data1, UINT data2)
{
	int ev = stat & 0xf0;
	int ch = stat & 0x0f;

	if (ev == 0x90 && ch < 3) {
		if (beginframe < 0) beginframe = frame;
		//printf("NOTE ON  : ch = %u, note = %u, velocity = %u : %d FRAME\n", stat & 0xf, data1, data2, frame);
		fprintf(output, "\t{ DRV_NOTE_ON     , %2d, %3d, %3d, %5d },\n", ch, data1, data2, frame - beginframe);
		evcount++;
	}

	if (ev == 0x80 && ch < 3) {
		//printf("NOTE OFF : ch = %u, note = %u, velocity = %u : %d FRAME\n", stat & 0xf, data1, data2, frame);
		fprintf(output, "\t{ DRV_NOTE_OFF    , %2d, %3d, %3d, %5d },\n", ch, data1, data2, frame - beginframe);
		evcount++;
	}
}

void CALLBACK MidiInProc(
	HMIDIIN midi_in_handle,
	UINT wMsg,
	DWORD dwInstance,
	DWORD dwParam1,
	DWORD dwParam2
)
{
	switch (wMsg)
	{
	case MIM_OPEN:
		printf("MIDI device was opened\n");
		break;
	case MIM_CLOSE:
		printf("MIDI device was closed\n");
		midi_closed = true;
		break;
	case MIM_DATA:
	{
		MidiDataRecieved(dwParam1 & 0xff, (dwParam1 >> 8) & 0xff, (dwParam1 >> 16) & 0xff);
	}
	break;
	case MIM_LONGDATA:
	case MIM_ERROR:
	case MIM_LONGERROR:
	case MIM_MOREDATA:
	default:
		break;
	}
}

#define FPS 60.0

void adjustFPS(void) {
	timeBeginPeriod(1);

	static unsigned long maetime = timeGetTime();
	static int frame = 0;
	long sleeptime;

	frame++;
	sleeptime = (frame < FPS) ?
		(maetime + (long)((double)frame*(1000.0 / FPS)) - timeGetTime()) :
		(maetime + 1000 - timeGetTime());
	if (sleeptime > 0) { Sleep(sleeptime); }
	if (frame >= FPS) {
		frame = 0;
		maetime = timeGetTime();
	}

	timeEndPeriod(1);
}


int main(int argc, char *argv[])
{
	HMIDIIN midi_in_handle;
	MMRESULT res;
	UINT device_id;
	WCHAR errmsg[MAXERRORLENGTH];
	char errmsg_buff[MAXERRORLENGTH];

	MIDIINCAPSA *device_caps;

	UINT devie_num = midiInGetNumDevs();

	device_caps = (MIDIINCAPSA *)malloc(devie_num * sizeof(MIDIINCAPSA));

	for (int i = 0; i < devie_num; i++) {
		midiInGetDevCapsA(i, device_caps + i, sizeof(MIDIINCAPSA));
	}

	for (int i = 0; i < devie_num; i++) {
		printf("[%02d]: %s\n", i, device_caps[i].szPname);
	}

	printf("Select Port(0-%d) >", devie_num - 1);
	scanf("%u", &device_id);

	char savename[256];
	printf("Select Savename >");
	scanf("%s", savename);

	char filename[256];

	sprintf(filename, "%s.h", savename);

	output = fopen(filename, "wt");

	FILE *head = fopen("ymzdrv_song.h", "wt");

	fprintf(head, "typedef struct {\n");
	fprintf(head, "\tunsigned char ev,ch,d1,d2;\n");
	fprintf(head, "\tunsigned short wf;\n");
	fprintf(head, "} ymzdrv_song_t;\n");
	fprintf(head, "\n");
	fprintf(head, "#define YMZDRVSONG(name) ymzdrvsng_ ## name\n");

	fclose(head);

	fprintf(output, "ymzdrv_song_t ymzdrvsng_%s[] = {\n", savename);

	res = midiInOpen(&midi_in_handle, device_id, (DWORD_PTR)MidiInProc, 0, CALLBACK_FUNCTION);
	if (res != MMSYSERR_NOERROR) {
		printf("Cannot open MIDI input device %u", device_id);
		return 1;
	}

	printf("Successfully opened a MIDI input device %u.\n", device_id);
	printf("PRESS Q KEY TO IMMIDIATE QUIT\n");

	midiInStart(midi_in_handle);

	while (!midi_closed) {
		adjustFPS();
		frame++;
		if (kbhit()) {
			int ch = getch();
			if (ch == 'q') break;
		}
	}

	fprintf(output, "};\n");
	fprintf(output, "// %d bytes(6 x %d)\n", evcount * 6, evcount);

	fclose(output);

	midiInStop(midi_in_handle);
	midiInReset(midi_in_handle);

	midiInClose(midi_in_handle);

	return 0;
}