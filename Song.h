/****************************************************************************************
*
*   Объявление класса Song
*
*   Объект этого класса представляет собой песню. Песня описывается:
*   1) последовательностью песенных событий - ППС (Singing Event Sequence - SES);
*   2) последовательностью тактов (Measure Sequence);
*   3) картой темпов.
*
*   Автор: Людмила Огородникова и Александр Огородников, 2010
*
****************************************************************************************/

class Song
{
	// структура, описывающая элемент списка песенных событий
	struct SINGING_EVENT {
		DWORD NoteNumber; // номер ноты
		double PauseLength; // длительность паузы между предыдущей нотой и данной нотой
							// в целых нотах
		double NoteLength; // длительность ноты в целых нотах
		LPWSTR pwsNoteText; // указатель на фрагмент слов песни, относящийся к ноте
		DWORD cchNoteText; // количество символов в строке pwsNoteText
		SINGING_EVENT *pNext; // указатель на следующий элемент списка
	};

	// структура, описывающая элемент списка тактов
	struct MEASURE {
		DWORD Numerator; // числитель музыкального размера
		DWORD Denominator; // знаменатель музыкального размера
		MEASURE *pNext; // указатель на следующий элемент списка
	};

	// структура, описывающая элемент списка темпов
	struct TEMPO {
		double Offset;	// количество целых нот от начала трека до момента установки
						// этого темпа
		double BPM; // количество четвертных нот в минуту
		TEMPO *pNext; // указатель на следующий элемент списка
	};

	// указатель на первый элемент списка песенных событий
	SINGING_EVENT *m_pFirstSingingEvent;

	// указатель на последний элемент списка песенных событий
	SINGING_EVENT *m_pLastSingingEvent;

	// указатель на текущий элемент списка песенных событий
	SINGING_EVENT *m_pCurSingingEvent;

	// указатель на первый элемент списка тактов
	MEASURE *m_pFirstMeasure;

	// указатель на последний элемент списка тактов
	MEASURE *m_pLastMeasure;

	// указатель на текущий элемент списка тактов
	MEASURE *m_pCurMeasure;

	// указатель на первый элемент списка темпов
	TEMPO *m_pFirstTempo;

	// указатель на последний элемент списка темпов
	TEMPO *m_pLastTempo;

	// указатель на текущий элемент списка темпов
	TEMPO *m_pCurTempo;

public:

	Song();
	~Song();

	// устанавливает текущую позицию на начало песни
	void ResetCurrentPosition();

	// добавляет песенное событие в ППС
	bool AddSingingEvent(
		__in DWORD NoteNumber,
		__in double PauseLength,
		__in double NoteLength,
		__in_opt LPCWSTR pwsNoteText,
		__in DWORD cchNoteText);

	// возвращает информацию об очередном песенном событии из ППС
	bool GetNextSingingEvent(
		__out_opt DWORD *pNoteNumber,
		__out_opt double *pPauseLength,
		__out_opt double *pNoteLength,
		__out_opt LPCWSTR *ppwsNoteText,
		__out_opt DWORD *pcchNoteText);

	// добавляет такт в последовательность тактов
	bool AddMeasure(
		__in DWORD Numerator,
		__in DWORD Denominator);

	// возвращает информацию об очередном такте из последовательности тактов
	bool GetNextMeasure(
		__out DWORD *pNumerator,
		__out DWORD *pDenominator);

	// добавляет темп в карту темпов
	bool AddTempo(
		__in double Offset,
		__in double BPM);

	// возвращает информацию об очередном темпе из карты темпов
	bool GetNextTempo(
		__out double *pOffset,
		__out double *pBPM);

	// квантует ноты в ППС
	bool Quantize(
		__in DWORD StepDenominator);

	// растягивает ноты с нулевой длительностью в ППС
	bool ExtendEmptyNotes(
		__in DWORD LengthDenominator);

	// ставит дефисы в конце слогов слов песни в ППС
	bool HyphenateLyric();

	// освобождает все выделенные ресурсы
	void Free();

private:

	// устанавливает новый текст песенного события
	bool SetText(
		__inout SINGING_EVENT *pSingingEvent,
		__in_opt LPCWSTR pwsNoteText,
		__in DWORD cchNoteText);
};
