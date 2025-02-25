/****************************************************************************************
*
*   Объявление класса MidiTrack
*
*   Объект этого класса представляет собой трек MIDI-файла.
*
*   Авторы: Людмила Огородникова и Александр Огородников, 2008-2010
*
****************************************************************************************/

/****************************************************************************************
*
*   Константы
*
****************************************************************************************/

// коды MIDI-событий
#define NOTE_OFF					0X80
#define NOTE_ON						0x90
#define KEY_AFTER_TOUCH				0xA0
#define CONTROL_CHANGE				0xB0
#define PROGRAM_CHANGE				0xC0
#define CHANNEL_AFTER_TOUCH			0xD0
#define PITCH_WHEEL_CHANGE			0xE0

// коды метасобытий
#define SEQUENCE_NUMBER				0x00
#define TEXT_EVENT 					0x01
#define COPYRIGHT_NOTICE 			0x02
#define SEQUENCE_OR_TRACK_NAME 		0x03
#define INSTRUMENT_NAME 			0x04
#define LYRIC						0x05
#define MARKER						0x06
#define CUE_POINT 					0x07
#define MIDI_CHANNEL_PREFIX			0x20
#define END_OF_TRACK 				0x2F
#define SET_TEMPO 					0x51
#define SMPTE_OFFSET				0x54
#define TIME_SIGNATURE 				0x58
#define KEY_SIGNATURE 				0x59
#define SEQUENCER_SPECIFIC			0x7F

// коды SysEx-событий
#define MULTIPACK					0xF0
#define PACKET 						0xF7

// код, возвращаемый методами GetNextEvent и GetNextRawEvent,
// после последнего события трека
#define REAL_TRACK_END				0xFFFFFFFF

/****************************************************************************************
*
*   Класс MidiTrack
*
****************************************************************************************/

class MidiTrack
{
	// указатель на первое событие трека
	BYTE *m_pFirstTrackEvent;

	// указатель на последний байт трека
	BYTE *m_pLastByteOfTrack;

	// указатель на текущий байт трека
	BYTE *m_pCurByte;

	// текущий статус для MIDI-событий
	BYTE m_RunningStatus;

public:

	MidiTrack();
	~MidiTrack();

	// подключает объект к указанному треку MIDI-файла
	bool AttachToTrack(
		__in BYTE *pFirstByteOfTrack,
		__in DWORD cbTrack);

	// устанавливает текущую позицию на начало трека
	void ResetCurrentPosition();

	// возвращает очередное событие, игнорируя события END_OF_TRACK
	DWORD GetNextEvent(
		__out_opt DWORD *pctDeltaTime,
		__out BYTE **ppDataBytes,
		__out_opt DWORD *pChannel = NULL);

	// создаёт копию данного объекта
	MidiTrack *Duplicate();

private:

	// возвращает очередное событие, включая события END_OF_TRACK
	DWORD GetNextRawEvent(
		__out DWORD *pctDeltaTime,
		__out BYTE **ppDataBytes,
		__out_opt DWORD *pChannel);
};
