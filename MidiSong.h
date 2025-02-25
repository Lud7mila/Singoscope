/****************************************************************************************
*
*   Объявление класса MidiSong
*
*   Объект этого класса представляет собой MIDI-песню. Песня описывается:
*   1) последовательностью песенных событий - ППС (Singing Event Sequence - SES);
*   2) последовательностью тактов (Measure Sequence);
*   3) картой темпов.
*
*   Автор: Людмила Огородникова и Александр Огородников, 2010
*
****************************************************************************************/

class MidiSong
{
	// структура, описывающая элемент списка песенных событий
	struct SINGING_EVENT {
		DWORD NoteNumber; // номер ноты
		DWORD ctToNoteOn; // количество тиков от начала трека до данной ноты
		DWORD ctDuration; // длительность ноты в тиках
		LPWSTR pwsNoteText; // указатель на фрагмент слов песни, относящийся к ноте
		DWORD cchNoteText; // количество символов в строке pwsNoteText
		SINGING_EVENT *pNext; // указатель на следующий элемент списка
	};

	// структура, описывающая элемент списка тактов
	struct MEASURE {
		DWORD Numerator; // числитель музыкального размера
		DWORD Denominator; // знаменатель музыкального размера
		DWORD cNotated32ndNotesPerMidiQuarterNote; // количество тридцатьвторых нот
												   // в четвертной MIDI-ноте
		MEASURE *pNext; // указатель на следующий элемент списка
	};

	// структура, описывающая элемент списка темпов
	struct TEMPO {
		DWORD ctToTempoSet;	// количество тиков от начала трека до момента установки
							// этого темпа
		DWORD cMicrosecondsPerMidiQuarterNote; // длительность четвертной MIDI-ноты
											   // в микросекундах
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

	MidiSong();
	~MidiSong();

	// устанавливает текущую позицию на начало песни
	void ResetCurrentPosition();

	// добавляет песенное событие в ППС
	bool AddSingingEvent(
		__in DWORD NoteNumber,
		__in DWORD ctToNoteOn,
		__in DWORD ctDuration,
		__in_opt LPCWSTR pwsNoteText,
		__in DWORD cchNoteText);

	// возвращает информацию об очередном песенном событии из ППС
	bool GetNextSingingEvent(
		__out_opt DWORD *pNoteNumber,
		__out_opt DWORD *pctToNoteOn,
		__out_opt DWORD *pctDuration,
		__out_opt LPCWSTR *ppwsNoteText,
		__out_opt DWORD *pcchNoteText);

	// добавляет такт в последовательность тактов
	bool AddMeasure(
		__in DWORD Numerator,
		__in DWORD Denominator,
		__in DWORD cNotated32ndNotesPerMidiQuarterNote);

	// возвращает информацию об очередном такте из последовательности тактов
	bool GetNextMeasure(
		__out DWORD *pNumerator,
		__out DWORD *pDenominator,
		__out DWORD *pcNotated32ndNotesPerMidiQuarterNote);

	// добавляет темп в карту темпов
	bool AddTempo(
		__in DWORD ctToTempoSet,
		__in DWORD cMicrosecondsPerMidiQuarterNote);

	// возвращает информацию об очередном темпе из карты темпов
	bool GetNextTempo(
		__out DWORD *pctToTempoSet,
		__out DWORD *pcMicrosecondsPerMidiQuarterNote);

	// корректирует текст слов песни в ППС
	bool CorrectLyric();

	// создаёт объект класса Song, представляющий собой песню
	Song *CreateSong(
		__in DWORD cTicksPerMidiQuarterNote);

	// освобождает все выделенные ресурсы
	void Free();

private:

	// выравнивает слова песни в ППС
	bool AlignLyric();

	// удаляет непечатные символы из слов песни в ППС
	// после их выравнивания методом AlignLyric
	bool RemoveNonPrintableSymbols();

	// устанавливает новый текст песенного события
	bool SetText(
		__inout SINGING_EVENT *pSingingEvent,
		__in_opt LPCWSTR pwsNoteText,
		__in DWORD cchNoteText);

	// создаёт ППС в объекте класса Song
	bool CreateSes(
		__inout Song *pSong,
		__in DWORD cTicksPerMidiQuarterNote);

	// создаёт последовательность тактов в объекте класса Song
	bool CreateMeasureSequence(
		__inout Song *pSong);

	// создаёт карту темпов в объекте класса Song
	bool CreateTempoMap(
		__inout Song *pSong,
		__in DWORD cTicksPerMidiQuarterNote);
};
