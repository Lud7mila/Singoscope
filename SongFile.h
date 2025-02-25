/****************************************************************************************
*
*   ���������� ������ SongFile
*
*	������ ����� ������ ������������ ����� ���� � ������.
*
*   ������: ������� ����������� � ��������� �����������, 2008-2010
*
****************************************************************************************/

class SongFile
{
	// ��������� �� ������ ������ MidiFile
	MidiFile *m_pMidiFile;

public:

	SongFile();
	~SongFile();

	// ������������� ������� �������� �� ��������� ��� ���� �����
	bool SetDefaultCodePage(
		__in UINT DefaultCodePage);

	// ������������� �������� ������ ���� �� ��������
	bool SetConcordNoteChoice(
		__in CONCORD_NOTE_CHOICE ConcordNoteChoice);

	// ��������� ���� � ������
	bool LoadFile(
		__in LPCTSTR pszFileName,
		__in UINT DefaultCodePage,
		__in CONCORD_NOTE_CHOICE ConcordNoteChoice);

	// ������ ������ ������ Song, �������������� ����� �����
	Song *CreateSong(
		__in DWORD QuantizeStepDenominator);

	// ����������� ��� ���������� �������
	void Free();
};
