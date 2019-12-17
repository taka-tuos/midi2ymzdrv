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

UINT notefreq[] = {
  0,  9,  9, 10,  10, 11,   12,  12, 13, 14, 15, 15,
  16,  17, 18, 19,  21, 22,  23,  25, 26, 28, 29, 31,
  33,  35, 37,  39,  41, 44,  46, 49, 52, 55, 58, 62,
  65, 69, 73, 78,  82, 87,  93, 98, 104,  110,  117,  124,
  131, 139, 147, 156, 165, 175, 185,  196,  208,  220,  233,  247,
  262, 277, 294, 311, 330, 349, 370,  392,  415,  440,  466,  494,
  523, 554, 587, 622, 659, 699, 740,  784,  831,  880,  932,  988,
  1047, 1109, 1175, 1245, 1319, 1397, 1480, 1568, 1661, 1760, 1865, 1976,
  2093, 2218, 2349, 2489, 2637, 2794, 2960, 3136, 3322, 3520, 3729, 3951,
  4186, 4435, 4699, 4978, 5274, 5587, 5920, 6272, 6645, 7040, 7459, 7902,
  8372, 8870, 9397, 9956, 10548,11175,  11840,  12544
};

BYTE notelist[16] = {
  0xff, 0xff, 0xff, 0xff,
  0xff, 0xff, 0xff, 0xff,
  0xff, 0xff, 0xff, 0xff,
  0xff, 0xff, 0xff, 0xff,
};

BYTE cc_msb[16] = {
  0xff, 0xff, 0xff, 0xff,
  0xff, 0xff, 0xff, 0xff,
  0xff, 0xff, 0xff, 0xff,
  0xff, 0xff, 0xff, 0xff
};

BYTE cc_lsb[16] = {
  0xff, 0xff, 0xff, 0xff,
  0xff, 0xff, 0xff, 0xff,
  0xff, 0xff, 0xff, 0xff,
  0xff, 0xff, 0xff, 0xff
};

BYTE vollist[16] = {
  0x7f, 0x7f, 0x7f, 0x7f,
  0x7f, 0x7f, 0x7f, 0x7f,
  0x7f, 0x7f, 0x7f, 0x7f,
  0x7f, 0x7f, 0x7f, 0x7f
};

BYTE explist[16] = {
  0x7f, 0x7f, 0x7f, 0x7f,
  0x7f, 0x7f, 0x7f, 0x7f,
  0x7f, 0x7f, 0x7f, 0x7f,
  0x7f, 0x7f, 0x7f, 0x7f
};

BYTE bendrange[16] = {
  2, 2, 2, 2,
  2, 2, 2, 2,
  2, 2, 2, 2,
  2, 2, 2, 2
};

BYTE bvollist[16] = {
  0x7f, 0x7f, 0x7f, 0x7f,
  0x7f, 0x7f, 0x7f, 0x7f,
  0x7f, 0x7f, 0x7f, 0x7f,
  0x7f, 0x7f, 0x7f, 0x7f
};

//#define DEBUG_MIDI

#define MIDI_PITCHBEND_MAX 8191
#define MIDI_PITCHBEND_MIN -8192

void MidiDataRecieved(UINT stat, UINT data1, UINT data2)
{
	int ev = stat & 0xf0;
	int ch = stat & 0x0f;

	if (ev == 0x90 && ch < 3) {
		if (beginframe < 0) beginframe = frame;

		data2 = (data2 / 4 + 96) * vollist[ch] / 127;

		bvollist[ch] = data2;

		data2 = data2 * explist[ch] / 127;

		notelist[ch] = data1;

		fprintf(output, "\t{ DRV_NOTE_ON     , %2d, %3d, %3d, %5d },\n", ch, data1, data2, frame - beginframe);
		evcount++;
	}

	if (ev == 0x80 && ch < 3) {
		fprintf(output, "\t{ DRV_NOTE_OFF    , %2d, %3d, %3d, %5d },\n", ch, data1, data2, frame - beginframe);
		evcount++;
	}

	if (ev == 0xb0 && ch < 3) {
		switch (data1) {
		case 100:
			cc_lsb[ch] = data2;
			break;
		case 101:
			cc_msb[ch] = data2;
			break;
		case 6:
			if (cc_msb[ch] == 0 && cc_lsb[ch] == 0) bendrange[ch] = data2;
			break;
		case 7:
			vollist[ch] = data2;
			break;
		case 11:
			explist[ch] = data2;
			fprintf(output, "\t{ DRV_EXP         , %2d, %3d, %3d, %5d },\n", ch, (bvollist[ch] * data2 / 127), 0, frame - beginframe);
			evcount++;
			break;
		}
	}

	if (ev == 0xe0 && ch < 3) {
		union { unsigned a : 14; signed b : 14; } bits;
		bits.a = data1 | data2 << 7;
		int bend = bits.b;
		//printf("%d\n", bend);
		double freq = notefreq[notelist[ch]];
		if (bend >= 0) {
			int freqA = notefreq[notelist[ch] + bendrange[ch]];
			freq = (freq * (MIDI_PITCHBEND_MAX - bend) + freqA * bend) / MIDI_PITCHBEND_MAX;
		}
		else {
			int freqA = notefreq[notelist[ch] - bendrange[ch]];
			freq = (freq * (MIDI_PITCHBEND_MIN - bend) + freqA * bend) / MIDI_PITCHBEND_MIN;
		}

		int freqI = freq;

		printf("%d, %d\n", notefreq[notelist[ch]], freqI);

		fprintf(output, "\t{ DRV_BEND        , %2d, %3d, %3d, %5d },\n", ch, (freqI >> 8) & 0xff, freqI & 0xff, frame - beginframe);
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

#ifdef DEBUG_MIDI
	FILE *_out = output;
	output = stdout;
#endif

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

#ifdef DEBUG_MIDI
	output = _out;
#endif

	fclose(output);

	midiInStop(midi_in_handle);
	midiInReset(midi_in_handle);

	midiInClose(midi_in_handle);

	return 0;
}