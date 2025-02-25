/****************************************************************************************
*
*   ���������� ������ MidiSong
*
*   ������ ����� ������ ������������ ����� MIDI-�����. ����� �����������:
*   1) ������������������� �������� ������� - ��� (Singing Event Sequence - SES);
*   2) ������������������� ������ (Measure Sequence);
*   3) ������ ������.
*
*   �����: ������� ������������ � ��������� �����������, 2010
*
****************************************************************************************/

class MidiSong
{
	// ���������, ����������� ������� ������ �������� �������
	struct SINGING_EVENT {
		DWORD NoteNumber; // ����� ����
		DWORD ctToNoteOn; // ���������� ����� �� ������ ����� �� ������ ����
		DWORD ctDuration; // ������������ ���� � �����
		LPWSTR pwsNoteText; // ��������� �� �������� ���� �����, ����������� � ����
		DWORD cchNoteText; // ���������� �������� � ������ pwsNoteText
		SINGING_EVENT *pNext; // ��������� �� ��������� ������� ������
	};

	// ���������, ����������� ������� ������ ������
	struct MEASURE {
		DWORD Numerator; // ��������� ������������ �������
		DWORD Denominator; // ����������� ������������ �������
		DWORD cNotated32ndNotesPerMidiQuarterNote; // ���������� �������������� ���
												   // � ���������� MIDI-����
		MEASURE *pNext; // ��������� �� ��������� ������� ������
	};

	// ���������, ����������� ������� ������ ������
	struct TEMPO {
		DWORD ctToTempoSet;	// ���������� ����� �� ������ ����� �� ������� ���������
							// ����� �����
		DWORD cMicrosecondsPerMidiQuarterNote; // ������������ ���������� MIDI-����
											   // � �������������
		TEMPO *pNext; // ��������� �� ��������� ������� ������
	};

	// ��������� �� ������ ������� ������ �������� �������
	SINGING_EVENT *m_pFirstSingingEvent;

	// ��������� �� ��������� ������� ������ �������� �������
	SINGING_EVENT *m_pLastSingingEvent;

	// ��������� �� ������� ������� ������ �������� �������
	SINGING_EVENT *m_pCurSingingEvent;

	// ��������� �� ������ ������� ������ ������
	MEASURE *m_pFirstMeasure;

	// ��������� �� ��������� ������� ������ ������
	MEASURE *m_pLastMeasure;

	// ��������� �� ������� ������� ������ ������
	MEASURE *m_pCurMeasure;

	// ��������� �� ������ ������� ������ ������
	TEMPO *m_pFirstTempo;

	// ��������� �� ��������� ������� ������ ������
	TEMPO *m_pLastTempo;

	// ��������� �� ������� ������� ������ ������
	TEMPO *m_pCurTempo;

public:

	MidiSong();
	~MidiSong();

	// ������������� ������� ������� �� ������ �����
	void ResetCurrentPosition();

	// ��������� �������� ������� � ���
	bool AddSingingEvent(
		__in DWORD NoteNumber,
		__in DWORD ctToNoteOn,
		__in DWORD ctDuration,
		__in_opt LPCWSTR pwsNoteText,
		__in DWORD cchNoteText);

	// ���������� ���������� �� ��������� �������� ������� �� ���
	bool GetNextSingingEvent(
		__out_opt DWORD *pNoteNumber,
		__out_opt DWORD *pctToNoteOn,
		__out_opt DWORD *pctDuration,
		__out_opt LPCWSTR *ppwsNoteText,
		__out_opt DWORD *pcchNoteText);

	// ��������� ���� � ������������������ ������
	bool AddMeasure(
		__in DWORD Numerator,
		__in DWORD Denominator,
		__in DWORD cNotated32ndNotesPerMidiQuarterNote);

	// ���������� ���������� �� ��������� ����� �� ������������������ ������
	bool GetNextMeasure(
		__out DWORD *pNumerator,
		__out DWORD *pDenominator,
		__out DWORD *pcNotated32ndNotesPerMidiQuarterNote);

	// ��������� ���� � ����� ������
	bool AddTempo(
		__in DWORD ctToTempoSet,
		__in DWORD cMicrosecondsPerMidiQuarterNote);

	// ���������� ���������� �� ��������� ����� �� ����� ������
	bool GetNextTempo(
		__out DWORD *pctToTempoSet,
		__out DWORD *pcMicrosecondsPerMidiQuarterNote);

	// ������������ ����� ���� ����� � ���
	bool CorrectLyric();

	// ������ ������ ������ Song, �������������� ����� �����
	Song *CreateSong(
		__in DWORD cTicksPerMidiQuarterNote);

	// ����������� ��� ���������� �������
	void Free();

private:

	// ����������� ����� ����� � ���
	bool AlignLyric();

	// ������� ���������� ������� �� ���� ����� � ���
	// ����� �� ������������ ������� AlignLyric
	bool RemoveNonPrintableSymbols();

	// ������������� ����� ����� ��������� �������
	bool SetText(
		__inout SINGING_EVENT *pSingingEvent,
		__in_opt LPCWSTR pwsNoteText,
		__in DWORD cchNoteText);

	// ������ ��� � ������� ������ Song
	bool CreateSes(
		__inout Song *pSong,
		__in DWORD cTicksPerMidiQuarterNote);

	// ������ ������������������ ������ � ������� ������ Song
	bool CreateMeasureSequence(
		__inout Song *pSong);

	// ������ ����� ������ � ������� ������ Song
	bool CreateTempoMap(
		__inout Song *pSong,
		__in DWORD cTicksPerMidiQuarterNote);
};
