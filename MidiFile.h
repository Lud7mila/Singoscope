/****************************************************************************************
*
*   Объявление класса MidiFile
*
*   Объект этого класса представляет собой MIDI-файл.
*
*   Автор: Людмила Огородникова и Александр Огородников, 2008-2010
*
****************************************************************************************/

/****************************************************************************************
*
*   Константы
*
****************************************************************************************/

// максимально допустимое количество символов, приходящихся на одну ноту
#define MAX_SYMBOLS_PER_NOTE				15

// коды возврата для некоторых методов
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
*   Класс MidiFile
*
****************************************************************************************/

class MidiFile
{
	// коды возврата для метода GetPartLyricDistance
	enum DISTANCERESULT
	{
		DISTANCE_SUCCESS,
		DISTANCE_NOT_VOCAL_PART,
		DISTANCE_NO_NOTES,
		DISTANCE_CANT_ALLOC_MEMORY
	};

	// структура, описывающая элемент списка вокальных партий
	struct VOCALPARTINFO {
		DWORD iTrack; // индекс трека в массиве m_MidiTracks
		DWORD Channel; // номер канала
		DWORD Instrument; // номер инструмента
		double PartLyricDistance; // мера сходства партии и слов песни в четвертных
								  // MIDI-нотах
		VOCALPARTINFO *pNext; // указатель на следующий элемент списка
	};

	// указатель на загруженный в память MIDI-файл
	BYTE *m_pFile;

	// количество тиков, приходящихся на четвертную MIDI ноту
	DWORD m_cTicksPerMidiQuarterNote;

	// указатель на массив объектов класса MidiTrack, соответствующих трекам MIDI-файла
	MidiTrack *m_MidiTracks;

	// количество действительных элеменов в массиве m_MidiTracks
	DWORD m_cTracks;

	// кодовая страница по умолчанию для слов песни
	UINT m_DefaultCodePage;

	// критерий выбора ноты из созвучия
	CONCORD_NOTE_CHOICE m_ConcordNoteChoice;

	// тип метасобытий со словами песни (LYRIC или TEXT_EVENT)
	DWORD m_LyricEventType;

	// указатель на объект класса MidiTrack, подключенный к треку со словами песни
	MidiTrack *m_pLyricTrack;

	// указатель на список вокальных партий
	VOCALPARTINFO *m_pVocalPartList;

public:

	MidiFile();
	~MidiFile();

	// устанавливает кодовую страницу по умолчанию для слов песни
	MIDIFILERESULT SetDefaultCodePage(
		__in UINT DefaultCodePage);

	// устанавливает критерий выбора ноты из созвучия
	MIDIFILERESULT SetConcordNoteChoice(
		__in CONCORD_NOTE_CHOICE ConcordNoteChoice);

	// назначает объекту загруженный в память файл
	MIDIFILERESULT AssignFile(
		__in BYTE *pFile,
		__in DWORD cbFile);

	// создаёт объект класса Song, представляющий собой песню
	Song *CreateSong();

	// освобождает все выделенные ресурсы
	void Free();

private:

	// ищет слова песни
	MIDIFILERESULT FindLyric();

	// ищет вокальные партии
	MIDIFILERESULT FindVocalParts();

	// вычисляет меру сходства партии и слов песни в четвертных MIDI-нотах
	DISTANCERESULT GetPartLyricDistance(
		__in DWORD iTrack,
		__in DWORD Channel,
		__in DWORD Instrument,
		__in double OverlapsThreshold,
		__in DWORD fCriteria,
		__out double *pPartLyricDistance);

	// проверяет, какая из двух нот является ближайшей к метасобытию
	bool IsFirstNoteNearer(
		__in DWORD ctToLyricEvent,
		__in NOTEDESC *pFirstNote,
		__in NOTEDESC *pSecondNote);

	// добавляет партию в список вокальных партий m_pVocalPartList
	bool AddVocalPart(
		__in DWORD iTrack,
		__in DWORD Channel,
		__in DWORD Instrument,
		__in double PartLyricDistance);

	// создаёт ППС в объекте класса MidiSong
	bool CreateSes(
		__in DWORD iTrack,
		__in DWORD Channel,
		__in DWORD Instrument,
		__in MidiSong *pMidiSong);

	// создаёт последовательность тактов в объекте класса MidiSong
	bool CreateMeasureSequence(
		__inout MidiSong *pMidiSong);

	// создаёт карту темпов в объекте класса MidiSong
	bool CreateTempoMap(
		__inout MidiSong *pMidiSong);
};
