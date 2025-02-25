/****************************************************************************************
*
*   Объявление класса MidiPart
*
*   Объект этого класса представляет собой партию в треке MIDI-файла. Партия - это
*   последовательность нот, образованных MIDI-событиями из одного и того же трека.
*   Пользователь объекта указывает, какие MIDI-события должны относиться к партии:
*   1) MIDI-события с заданным номером канала либо с любым номером канала;
*   2) MIDI-события с заданным номером инструмента либо с любым номером инструмента.
*
*   Авторы: Людмила Огородникова и Александр Огородников, 2007-2010
*
****************************************************************************************/

/****************************************************************************************
*
*   Константы
*
****************************************************************************************/

// возможное значение параметра Channel метода InitSearch
#define ANY_CHANNEL			0xFFFFFFFF

// возможное значение параметра Instrument метода InitSearch
#define ANY_INSTRUMENT		0xFFFFFFFF

// критерии выбора ноты из созвучия
enum CONCORD_NOTE_CHOICE
{
	CHOOSE_MIN_NOTE_NUMBER,
	CHOOSE_MAX_NOTE_NUMBER,
	CHOOSE_MAX_NOTE_DURATION
};

// коды возврата для метода GetNextNote
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
*   Определения типов
*
****************************************************************************************/

// структура, описывающая ноту; возвращается методом GetNextNote
struct NOTEDESC {
	DWORD NoteNumber; // номер ноты
	DWORD ctToNoteOn; // количество тиков от начала трека до данной ноты
	DWORD ctDuration; // длительность ноты в тиках
};

/****************************************************************************************
*
*   Класс MidiPart
*
****************************************************************************************/

class MidiPart
{
	// коды возврата для метода ReadNextEvent
	enum READEVENTRESULT
	{
		READEVENT_NOTE_ON,
		READEVENT_NOTE_OFF,
		READEVENT_EMPTY_NOTE,
		READEVENT_UNRELEVANT_EVENT,
		READEVENT_PART_END,
		READEVENT_CANT_ALLOC_MEMORY
	};

	// структура, описывающая элемент списка нот;
	// метод ReadNextEvent модифицирует этот список в соответствии с событиями трека,
	// метод GetNextNote может модифицировать этот список при конструировании одиночных
	// нот;
	// значение поля ctToNoteOn любого элемента списка всегда больше или равно значению
	// этого поля в любом предыдущем элементе
	struct NOTELISTITEM {
		DWORD 		 NoteNumber; // номер ноты
		DWORD 		 ctToNoteOn; // количество тиков от начала трека до данной ноты
		DWORD 		 ctDuration; // длительность ноты в тиках
		DWORD		 cNotMatchedNoteOnEvents; // количество невыключенных включений ноты
		DWORD		 Channel;    // номер канала
		DWORD 		 Instrument; // номер инструмента
		NOTELISTITEM *pNext;     // указатель на следующий элемент списка
	};

	// указатель на объект класса MidiTrack, представляющий собой трек MIDI-файла,
	// в котором находится партия
	MidiTrack *m_pMidiTrack;

	// номер канала, в котором находится партия
	DWORD m_PartChannel;

	// номер инструмента, которым играются ноты данной партии
	DWORD m_PartInstrument;

	// максимально допустимая доля перекрытий нот по времени
	// по отношению к количеству одиночных нот в партии
	double m_OverlapsThreshold;

	// определяет, какую из нот созвучия должен возвращать метод GetNextNote
	CONCORD_NOTE_CHOICE m_ConcordNoteChoice;

	// массив, содержащий номер текущего инструмента для каждого канала
	DWORD m_CurInstrument[MAX_MIDI_CHANNELS];

	// текущее время (время в тиках, прошедшее с начала трека)
	DWORD m_ctCurTime;

	// указатель на список нот
	NOTELISTITEM *m_pNoteList;

	// количество одиночных нот в партии на текущий момент
	DWORD m_cSingleNotes;

	// количество перекрытий нот по времени в партии на текущий момент
	DWORD m_cOverlaps;

public:

	MidiPart();
	~MidiPart();

	// инициализирует поиск нот в указанной партии
	void InitSearch(
		__in MidiTrack *pMidiTrack,
		__in DWORD Channel,
		__in DWORD Instrument,
		__in double OverlapsThreshold,
		__in CONCORD_NOTE_CHOICE ConcordNoteChoice);

	// возвращает очередную одиночную ноту
	MIDIPARTRESULT GetNextNote(
		__out_opt NOTEDESC *pNoteDesc);

private:

	// считывает очередное событие трека и обновляет состояние объекта
	READEVENTRESULT ReadNextEvent();

	// возвращает первую ноту из списка и удаляет её из списка
	void ReturnFirstNote(
		__out_opt NOTEDESC *pNoteDesc);

	// возвращает одну ноту созвучия из списка и удаляет все ноты созвучия из списка
	void ReturnNoteFromConcord(
		__out_opt NOTEDESC *pNoteDesc);

	// освобождает память, выделенную для списка нот, на который указывает m_pNoteList
	void FreeNoteList();
};
