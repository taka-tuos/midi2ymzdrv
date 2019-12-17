#include <stdio.h>

#include <windows.h> 
#include <mmsystem.h> 

#pragma comment(lib, "winmm.lib")

void MidiDataRecieved(UINT stat, UINT data1, UINT data2)
{
	if ((stat & 0xf0) == 0x90) {
		printf("NOTE ON  : ch = %u, note = %u, velocity = %u\n", stat & 0xf, data1, data2);
	}

	if ((stat & 0xf0) == 0x80) {
		printf("NOTE OFF : ch = %u, note = %u, velocity = %u\n", stat & 0xf, data1, data2);
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

	res = midiInOpen(&midi_in_handle, device_id, (DWORD_PTR)MidiInProc, 0, CALLBACK_FUNCTION);
	if (res != MMSYSERR_NOERROR) {
		printf("Cannot open MIDI input device %u", device_id);
		return 1;
	}

	printf("Successfully opened a MIDI input device %u.\n", device_id);

	midiInStart(midi_in_handle);

	while (true) {
		Sleep(10);
	}

	midiInStop(midi_in_handle);
	midiInReset(midi_in_handle);

	midiInClose(midi_in_handle);

	return 0;
}