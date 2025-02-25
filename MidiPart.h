/****************************************************************************************
*
*   ���������� ������ MidiPart
*
*   ������ ����� ������ ������������ ����� ������ � ����� MIDI-�����. ������ - ���
*   ������������������ ���, ������������ MIDI-��������� �� ������ � ���� �� �����.
*   ������������ ������� ���������, ����� MIDI-������� ������ ���������� � ������:
*   1) MIDI-������� � �������� ������� ������ ���� � ����� ������� ������;
*   2) MIDI-������� � �������� ������� ����������� ���� � ����� ������� �����������.
*
*   ������: ������� ������������ � ��������� �����������, 2007-2010
*
****************************************************************************************/

/****************************************************************************************
*
*   ���������
*
****************************************************************************************/

// ��������� �������� ��������� Channel ������ InitSearch
#define ANY_CHANNEL			0xFFFFFFFF

// ��������� �������� ��������� Instrument ������ InitSearch
#define ANY_INSTRUMENT		0xFFFFFFFF

// �������� ������ ���� �� ��������
enum CONCORD_NOTE_CHOICE
{
	CHOOSE_MIN_NOTE_NUMBER,
	CHOOSE_MAX_NOTE_NUMBER,
	CHOOSE_MAX_NOTE_DURATION
};

// ���� �������� ��� ������ GetNextNote
enum MIDIPARTRESULT
{
	MIDIPART_SUCCESS,
	MIDIPART_PART_END,
	MIDIPART_CANT_ALLOC_MEMORY,
	MIDIPART_COMPLICATED_NOTE_COMBINATION,
	MIDIPART_OVERLAPS_THRESHOLD_EXCEEDED
};

/****************************************************************************************
*
*   ����������� �����
*
****************************************************************************************/

// ���������, ����������� ����; ������������ ������� GetNextNote
struct NOTEDESC {
	DWORD NoteNumber; // ����� ����
	DWORD ctToNoteOn; // ���������� ����� �� ������ ����� �� ������ ����
	DWORD ctDuration; // ������������ ���� � �����
};

/****************************************************************************************
*
*   ����� MidiPart
*
****************************************************************************************/

class MidiPart
{
	// ���� �������� ��� ������ ReadNextEvent
	enum READEVENTRESULT
	{
		READEVENT_NOTE_ON,
		READEVENT_NOTE_OFF,
		READEVENT_EMPTY_NOTE,
		READEVENT_UNRELEVANT_EVENT,
		READEVENT_PART_END,
		READEVENT_CANT_ALLOC_MEMORY
	};

	// ���������, ����������� ������� ������ ���;
	// ����� ReadNextEvent ������������ ���� ������ � ������������ � ��������� �����,
	// ����� GetNextNote ����� �������������� ���� ������ ��� ��������������� ���������
	// ���;
	// �������� ���� ctToNoteOn ������ �������� ������ ������ ������ ��� ����� ��������
	// ����� ���� � ����� ���������� ��������
	struct NOTELISTITEM {
		DWORD 		 NoteNumber; // ����� ����
		DWORD 		 ctToNoteOn; // ���������� ����� �� ������ ����� �� ������ ����
		DWORD 		 ctDuration; // ������������ ���� � �����
		DWORD		 cNotMatchedNoteOnEvents; // ���������� ������������� ��������� ����
		DWORD		 Channel;    // ����� ������
		DWORD 		 Instrument; // ����� �����������
		NOTELISTITEM *pNext;     // ��������� �� ��������� ������� ������
	};

	// ��������� �� ������ ������ MidiTrack, �������������� ����� ���� MIDI-�����,
	// � ������� ��������� ������
	MidiTrack *m_pMidiTrack;

	// ����� ������, � ������� ��������� ������
	DWORD m_PartChannel;

	// ����� �����������, ������� �������� ���� ������ ������
	DWORD m_PartInstrument;

	// ����������� ���������� ���� ���������� ��� �� �������
	// �� ��������� � ���������� ��������� ��� � ������
	double m_OverlapsThreshold;

	// ����������, ����� �� ��� �������� ������ ���������� ����� GetNextNote
	CONCORD_NOTE_CHOICE m_ConcordNoteChoice;

	// ������, ���������� ����� �������� ����������� ��� ������� ������
	DWORD m_CurInstrument[MAX_MIDI_CHANNELS];

	// ������� ����� (����� � �����, ��������� � ������ �����)
	DWORD m_ctCurTime;

	// ��������� �� ������ ���
	NOTELISTITEM *m_pNoteList;

	// ���������� ��������� ��� � ������ �� ������� ������
	DWORD m_cSingleNotes;

	// ���������� ���������� ��� �� ������� � ������ �� ������� ������
	DWORD m_cOverlaps;

public:

	MidiPart();
	~MidiPart();

	// �������������� ����� ��� � ��������� ������
	void InitSearch(
		__in MidiTrack *pMidiTrack,
		__in DWORD Channel,
		__in DWORD Instrument,
		__in double OverlapsThreshold,
		__in CONCORD_NOTE_CHOICE ConcordNoteChoice);

	// ���������� ��������� ��������� ����
	MIDIPARTRESULT GetNextNote(
		__out_opt NOTEDESC *pNoteDesc);

private:

	// ��������� ��������� ������� ����� � ��������� ��������� �������
	READEVENTRESULT ReadNextEvent();

	// ���������� ������ ���� �� ������ � ������� � �� ������
	void ReturnFirstNote(
		__out_opt NOTEDESC *pNoteDesc);

	// ���������� ���� ���� �������� �� ������ � ������� ��� ���� �������� �� ������
	void ReturnNoteFromConcord(
		__out_opt NOTEDESC *pNoteDesc);

	// ����������� ������, ���������� ��� ������ ���, �� ������� ��������� m_pNoteList
	void FreeNoteList();
};
