/****************************************************************************************
*
*   ���������� ������ MidiFile
*
*   ������ ����� ������ ������������ ����� MIDI-����.
*
*   �����: ������� ������������ � ��������� �����������, 2008-2010
*
****************************************************************************************/

/****************************************************************************************
*
*   ���������
*
****************************************************************************************/

// ����������� ���������� ���������� ��������, ������������ �� ���� ����
#define MAX_SYMBOLS_PER_NOTE				15

// ���� �������� ��� ��������� �������
enum MIDIFILERESULT
{
	MIDIFILE_SUCCESS,
	MIDIFILE_INVALID_FORMAT,
	MIDIFILE_UNSUPPORTED_FORMAT,
	MIDIFILE_NO_LYRIC,
	MIDIFILE_NO_VOCAL_PARTS,
	MIDIFILE_CANT_ALLOC_MEMORY
};

/****************************************************************************************
*
*   ����� MidiFile
*
****************************************************************************************/

class MidiFile
{
	// ���� �������� ��� ������ GetPartLyricDistance
	enum DISTANCERESULT
	{
		DISTANCE_SUCCESS,
		DISTANCE_NOT_VOCAL_PART,
		DISTANCE_NO_NOTES,
		DISTANCE_CANT_ALLOC_MEMORY
	};

	// ���������, ����������� ������� ������ ��������� ������
	struct VOCALPARTINFO {
		DWORD iTrack; // ������ ����� � ������� m_MidiTracks
		DWORD Channel; // ����� ������
		DWORD Instrument; // ����� �����������
		double PartLyricDistance; // ���� �������� ������ � ���� ����� � ����������
								  // MIDI-�����
		VOCALPARTINFO *pNext; // ��������� �� ��������� ������� ������
	};

	// ��������� �� ����������� � ������ MIDI-����
	BYTE *m_pFile;

	// ���������� �����, ������������ �� ���������� MIDI ����
	DWORD m_cTicksPerMidiQuarterNote;

	// ��������� �� ������ �������� ������ MidiTrack, ��������������� ������ MIDI-�����
	MidiTrack *m_MidiTracks;

	// ���������� �������������� �������� � ������� m_MidiTracks
	DWORD m_cTracks;

	// ������� �������� �� ��������� ��� ���� �����
	UINT m_DefaultCodePage;

	// �������� ������ ���� �� ��������
	CONCORD_NOTE_CHOICE m_ConcordNoteChoice;

	// ��� ����������� �� ������� ����� (LYRIC ��� TEXT_EVENT)
	DWORD m_LyricEventType;

	// ��������� �� ������ ������ MidiTrack, ������������ � ����� �� ������� �����
	MidiTrack *m_pLyricTrack;

	// ��������� �� ������ ��������� ������
	VOCALPARTINFO *m_pVocalPartList;

public:

	MidiFile();
	~MidiFile();

	// ������������� ������� �������� �� ��������� ��� ���� �����
	MIDIFILERESULT SetDefaultCodePage(
		__in UINT DefaultCodePage);

	// ������������� �������� ������ ���� �� ��������
	MIDIFILERESULT SetConcordNoteChoice(
		__in CONCORD_NOTE_CHOICE ConcordNoteChoice);

	// ��������� ������� ����������� � ������ ����
	MIDIFILERESULT AssignFile(
		__in BYTE *pFile,
		__in DWORD cbFile);

	// ������ ������ ������ Song, �������������� ����� �����
	Song *CreateSong();

	// ����������� ��� ���������� �������
	void Free();

private:

	// ���� ����� �����
	MIDIFILERESULT FindLyric();

	// ���� ��������� ������
	MIDIFILERESULT FindVocalParts();

	// ��������� ���� �������� ������ � ���� ����� � ���������� MIDI-�����
	DISTANCERESULT GetPartLyricDistance(
		__in DWORD iTrack,
		__in DWORD Channel,
		__in DWORD Instrument,
		__in double OverlapsThreshold,
		__in DWORD fCriteria,
		__out double *pPartLyricDistance);

	// ���������, ����� �� ���� ��� �������� ��������� � �����������
	bool IsFirstNoteNearer(
		__in DWORD ctToLyricEvent,
		__in NOTEDESC *pFirstNote,
		__in NOTEDESC *pSecondNote);

	// ��������� ������ � ������ ��������� ������ m_pVocalPartList
	bool AddVocalPart(
		__in DWORD iTrack,
		__in DWORD Channel,
		__in DWORD Instrument,
		__in double PartLyricDistance);

	// ������ ��� � ������� ������ MidiSong
	bool CreateSes(
		__in DWORD iTrack,
		__in DWORD Channel,
		__in DWORD Instrument,
		__in MidiSong *pMidiSong);

	// ������ ������������������ ������ � ������� ������ MidiSong
	bool CreateMeasureSequence(
		__inout MidiSong *pMidiSong);

	// ������ ����� ������ � ������� ������ MidiSong
	bool CreateTempoMap(
		__inout MidiSong *pMidiSong);
};
