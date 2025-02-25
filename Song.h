/****************************************************************************************
*
*   ���������� ������ Song
*
*   ������ ����� ������ ������������ ����� �����. ����� �����������:
*   1) ������������������� �������� ������� - ��� (Singing Event Sequence - SES);
*   2) ������������������� ������ (Measure Sequence);
*   3) ������ ������.
*
*   �����: ������� ������������ � ��������� �����������, 2010
*
****************************************************************************************/

class Song
{
	// ���������, ����������� ������� ������ �������� �������
	struct SINGING_EVENT {
		DWORD NoteNumber; // ����� ����
		double PauseLength; // ������������ ����� ����� ���������� ����� � ������ �����
							// � ����� �����
		double NoteLength; // ������������ ���� � ����� �����
		LPWSTR pwsNoteText; // ��������� �� �������� ���� �����, ����������� � ����
		DWORD cchNoteText; // ���������� �������� � ������ pwsNoteText
		SINGING_EVENT *pNext; // ��������� �� ��������� ������� ������
	};

	// ���������, ����������� ������� ������ ������
	struct MEASURE {
		DWORD Numerator; // ��������� ������������ �������
		DWORD Denominator; // ����������� ������������ �������
		MEASURE *pNext; // ��������� �� ��������� ������� ������
	};

	// ���������, ����������� ������� ������ ������
	struct TEMPO {
		double Offset;	// ���������� ����� ��� �� ������ ����� �� ������� ���������
						// ����� �����
		double BPM; // ���������� ���������� ��� � ������
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

	Song();
	~Song();

	// ������������� ������� ������� �� ������ �����
	void ResetCurrentPosition();

	// ��������� �������� ������� � ���
	bool AddSingingEvent(
		__in DWORD NoteNumber,
		__in double PauseLength,
		__in double NoteLength,
		__in_opt LPCWSTR pwsNoteText,
		__in DWORD cchNoteText);

	// ���������� ���������� �� ��������� �������� ������� �� ���
	bool GetNextSingingEvent(
		__out_opt DWORD *pNoteNumber,
		__out_opt double *pPauseLength,
		__out_opt double *pNoteLength,
		__out_opt LPCWSTR *ppwsNoteText,
		__out_opt DWORD *pcchNoteText);

	// ��������� ���� � ������������������ ������
	bool AddMeasure(
		__in DWORD Numerator,
		__in DWORD Denominator);

	// ���������� ���������� �� ��������� ����� �� ������������������ ������
	bool GetNextMeasure(
		__out DWORD *pNumerator,
		__out DWORD *pDenominator);

	// ��������� ���� � ����� ������
	bool AddTempo(
		__in double Offset,
		__in double BPM);

	// ���������� ���������� �� ��������� ����� �� ����� ������
	bool GetNextTempo(
		__out double *pOffset,
		__out double *pBPM);

	// �������� ���� � ���
	bool Quantize(
		__in DWORD StepDenominator);

	// ����������� ���� � ������� ������������� � ���
	bool ExtendEmptyNotes(
		__in DWORD LengthDenominator);

	// ������ ������ � ����� ������ ���� ����� � ���
	bool HyphenateLyric();

	// ����������� ��� ���������� �������
	void Free();

private:

	// ������������� ����� ����� ��������� �������
	bool SetText(
		__inout SINGING_EVENT *pSingingEvent,
		__in_opt LPCWSTR pwsNoteText,
		__in DWORD cchNoteText);
};
