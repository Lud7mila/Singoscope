/****************************************************************************************
*
*   Определение класса MidiFile
*
*   Объект этого класса представляет собой MIDI-файл.
*
*   Автор: Людмила Огородникова и Александр Огородников, 2008-2010
*
****************************************************************************************/

#define _CRT_SECURE_NO_WARNINGS

#include <windows.h>
#include <math.h>

#include "Log.h"
#include "Song.h"
#include "MidiLibrary.h"
#include "MidiTrack.h"
#include "MidiPart.h"
#include "MidiLyric.h"
#include "MidiSong.h"
#include "MidiFile.h"

/****************************************************************************************
*
*   Константы
*
****************************************************************************************/

// если количество допустимых метасобытий типа TEXT_EVENT меньше этого порога,
// то считаем, что они не являются словами песни
#define MIN_TEXT_EVENTS_FOR_LYRIC_TRACK		5


// три флага, определяющих критерии, налагаемые на вокальную партию
// (смысл этих флагов объясняется в описании метода GetPartLyricDistance):

// критерий "одинаковое начало"
#define IDENTICAL_START						0x00000001

// критерий "одинаковый конец"
#define IDENTICAL_END						0x00000002

// критерий "ограниченное количество нот на одно метасобытие"
#define LIMIT_NOTES_PER_METAEVENT			0x00000004


// максимально допустимое количество нот на одно метасобытие
#define MAX_NOTES_PER_METAEVENT				10

// максимально допустимое значение меры сходства партии и слов песни в четвертных
// MIDI-нотах для вокальных партий
#define MAX_VOCAL_PART_LYRIC_DISTANCE		500

/****************************************************************************************
*
*   Определения типов
*
****************************************************************************************/

// структура, описывающая ограничения, налагаемые на вокальные партии при их поиске;
// используется в методе FindVocalParts
struct VOCAL_PART_LIMITATIONS {
	double OverlapsThreshold;	// максимально допустимая доля перекрытий нот по времени
								// по отношению к количеству одиночных нот в партии
	DWORD fCriteria;	// набор флагов, определяющих критерии поиска вокальных партий
};

/****************************************************************************************
*
*   Встроенные функции
*
****************************************************************************************/

inline DWORD GetBigEndianDword(PBYTE pBytes)
{
	return pBytes[0] << 24 | pBytes[1] << 16 | pBytes[2] << 8 | pBytes[3];
}

inline DWORD GetBigEndianWord(PBYTE pBytes)
{
	return pBytes[0] << 8 | pBytes[1];
}

inline DWORD Distance(DWORD Number1, DWORD Number2)
{
	if (Number1 > Number2) return Number1 - Number2;

	return Number2 - Number1;
}

/****************************************************************************************
*
*   Конструктор
*
*   Параметры
*       Нет
*
*   Возвращаемое значение
*       Нет
*
*   Инициализирует переменные объекта.
*
****************************************************************************************/

MidiFile::MidiFile()
{
	m_pFile = NULL;

	m_cTicksPerMidiQuarterNote = 0;

	m_MidiTracks = NULL;
	m_cTracks = 0;

	m_DefaultCodePage = CP_ACP;
	m_ConcordNoteChoice = CHOOSE_MIN_NOTE_NUMBER;
	m_LyricEventType = 0;
	m_pLyricTrack = NULL;

	m_pVocalPartList = NULL;
}

/****************************************************************************************
*
*   Деструктор
*
*   Параметры
*       Нет
*
*   Возвращаемое значение
*       Нет
*
*   Освобождает все выделенные ресурсы.
*
****************************************************************************************/

MidiFile::~MidiFile()
{
	Free();
}

/****************************************************************************************
*
*   Метод SetDefaultCodePage
*
*   Параметры
*       DefaultCodePage - кодовая страница по умолчанию для слов песни
*
*   Возвращаемое значение
*       MIDIFILE_SUCCESS - кодовая страница по умолчанию успешно установлена и найдены
*                          слова песни и хотя бы одна вокальная партия;
*       MIDIFILE_NO_LYRIC - слова песни не найдены;
*       MIDIFILE_NO_VOCAL_PARTS - не найдена ни одна вокальная партия (в том числе, когда
*                                 в файле нет ни одного корректного трека);
*       MIDIFILE_CANT_ALLOC_MEMORY - не удалось выделить память.
*
*   Устанавливает кодовую страницу по умолчанию для слов песни. После установки кодовой
*   страницы по умолчанию метод выполняет следующие действия:
*   1) находит слова песни;
*   2) создаёт список вокальных партий для данной песни.
*
****************************************************************************************/

MIDIFILERESULT MidiFile::SetDefaultCodePage(
	__in UINT DefaultCodePage)
{
	m_DefaultCodePage = DefaultCodePage;

	if (m_pFile == NULL) return MIDIFILE_SUCCESS;

	// ищем слова песни
	MIDIFILERESULT Res = FindLyric();
	if (Res != MIDIFILE_SUCCESS)
	{
		LOG("lyric not found\n");
		return Res;
	}

	// ищем вокальные партии
	Res = FindVocalParts();
	if (Res != MIDIFILE_SUCCESS)
	{
		LOG("vocal parts not found\n");
		return Res;
	}

	return MIDIFILE_SUCCESS;
}

/****************************************************************************************
*
*   Метод SetConcordNoteChoice
*
*   Параметры
*       ConcordNoteChoice - определяет, какую из нот созвучия нужно выбирать:
*                           CHOOSE_MIN_NOTE_NUMBER - ноту с минимальным номером,
*                           CHOOSE_MAX_NOTE_NUMBER - ноту с максимальным номером,
*                           CHOOSE_MAX_NOTE_DURATION - ноту с максимальной длительностью
*
*   Возвращаемое значение
*       MIDIFILE_SUCCESS - критерий выбора ноты из созвучия успешно установлен и найдена
*                          хотя бы одна вокальная партия;
*       MIDIFILE_NO_VOCAL_PARTS - не найдена ни одна вокальная партия (в том числе, когда
*                                 в файле нет ни одного корректного трека);
*       MIDIFILE_CANT_ALLOC_MEMORY - не удалось выделить память.
*
*   Устанавливает критерий выбора ноты из созвучия. Этот критерий используется, когда
*   вокальная партия содержит созвучия и необходимо выбрать только одну ноту. После
*   установки этого критерия метод заново создаёт список вокальных партий для данной
*   песни.
*
****************************************************************************************/

MIDIFILERESULT MidiFile::SetConcordNoteChoice(
	__in CONCORD_NOTE_CHOICE ConcordNoteChoice)
{
	m_ConcordNoteChoice = ConcordNoteChoice;

	if (m_pFile == NULL) return MIDIFILE_SUCCESS;

	// ищем вокальные партии
	MIDIFILERESULT Res = FindVocalParts();
	if (Res != MIDIFILE_SUCCESS)
	{
		LOG("vocal parts not found\n");
		return Res;
	}

	return MIDIFILE_SUCCESS;
}

/****************************************************************************************
*
*   Метод AssignFile
*
*   Параметры
*       pFile - указатель на буфер памяти, в который загружен файл; буфер должен быть
*               выделен функцией HeapAlloc
*		cbFile - размер файла в байтах
*
*   Возвращаемое значение
*       MIDIFILE_SUCCESS - файл успешно назначен объекту (найдены слова песни и хотя
*                          бы одна вокальная партия);
*       MIDIFILE_INVALID_FORMAT - файл не является MIDI-файлом;
*       MIDIFILE_UNSUPPORTED_FORMAT - неподдерживаемый формат MIDI-файла: либо формат
*                                     MIDI-файла равен 2 (последовательность идущих один
*                                     за другим треков), либо дельта-время представлено в
*                                     SMPTE-формате;
*       MIDIFILE_NO_LYRIC - не найдены слова песни;
*       MIDIFILE_NO_VOCAL_PARTS - не найдена ни одна вокальная партия (в том числе, когда
*                                 в файле нет ни одного корректного трека);
*       MIDIFILE_CANT_ALLOC_MEMORY - не удалось выделить память.
*
*   Назначает объекту загруженный в память файл. После вызова этого метода буфер памяти,
*   на который указывает параметр pFile, переходит во владение объекта, т.е. объект
*   становится ответственным за освобождение этого буфера. Перед тем, как назначить файл
*   объекту, метод выполняет следующие действия:
*   1) проверяет формат файла;
*   2) находит слова песни;
*   3) создаёт список вокальных партий для данной песни.
*
****************************************************************************************/

MIDIFILERESULT MidiFile::AssignFile(
	__in BYTE *pFile,
	__in DWORD cbFile)
{
	// если этому объекту уже назначен какой-то файл, освобождаем все выделенные для
	// этого файла ресурсы
	Free();

	if (pFile == NULL) return MIDIFILE_INVALID_FORMAT;

	// анализируем заголовок файла
	// ---------------------------

	// проверяем, что размер файла не меньше размера стандартного заголовка MIDI-файла
	if (cbFile < 14)
	{
		LOG("file is too small\n");
		return MIDIFILE_INVALID_FORMAT;
	}

	// проверяем сигнатуру MIDI-файла
	DWORD dwFileSignature = GetBigEndianDword(pFile);
	if (dwFileSignature != 'MThd')
	{
		LOG("MIDI file signature not found\n");
        return MIDIFILE_INVALID_FORMAT;
	}

	// проверяем значение поля "длина заголовка"
	DWORD dwHeaderLength = GetBigEndianDword(pFile + 4);
	if (dwHeaderLength != 6)
	{
		LOG("invalid length of MIDI file header\n");
		return MIDIFILE_INVALID_FORMAT;
	}

	// проверяем формат MIDI-файла;
	// сейчас поддерживаются MIDI-файлы формата 0 (один многоканальный трек) и формата 1
	// (один и более одновременных треков);
	DWORD Format = GetBigEndianWord(pFile + 8);
	if (Format == 2)
	{
		// MIDI-файл формата 2 (последовательность идущих один за другим треков)
		LOG("unsupported MIDI file format\n");
		return MIDIFILE_UNSUPPORTED_FORMAT;
	}
	else if (Format > 2)
	{
		// недействительное значение поля "формат MIDI-файла"
		LOG("invalid MIDI file format\n");
		return MIDIFILE_INVALID_FORMAT;
	}

	// проверяем формат дельта-времени;
	// сейчас поддерживаются только MIDI-файлы, в которых дельта-время указано в тиках
	if (pFile[12] & 0x80)
	{
		// дельта-время указано в SMPTE-формате
		LOG("SMPTE format for delta times is not supported\n");
		return MIDIFILE_UNSUPPORTED_FORMAT;
	}

	// запоминаем количество тиков на четвертную MIDI-ноту
	m_cTicksPerMidiQuarterNote = GetBigEndianWord(pFile + 12);

	// анализируем треки
    // -----------------

	// подсчитываем количество треков (блоков c сигнатурой "MTrk") в MIDI-файле
	DWORD cTracks = 0;
	DWORD iCurByte = 14;
	while (iCurByte + 7 < cbFile)
	{
		// считываем сигнатуру текущего блока
		DWORD CurChunkSignature = GetBigEndianDword(pFile + iCurByte);
		iCurByte += 4;

        // считываем размер данных текущего блока в байтах
		DWORD cbChunk = GetBigEndianDword(pFile + iCurByte);
		iCurByte += 4;

		// если данные текущего блока выходят за пределы файла, то соответственно
		// уменьшаем размер данных блока
		if (iCurByte + cbChunk > cbFile) cbChunk = cbFile - iCurByte;

		// если текущий блок содержит сигнатуру, отличную от "MTrk", либо размер данных
        // блока равен нулю, то этот блок не считаем за трек
		if (CurChunkSignature == 'MTrk' && cbChunk > 0) cTracks ++;

		iCurByte += cbChunk;
	}

	// проверяем количество найденных треков в файле
	if (cTracks == 0)
	{
		LOG("no tracks in MIDI file\n");
		return MIDIFILE_NO_VOCAL_PARTS;
	}

	// выделяем память для массива объектов класса MidiTrack;
    // размер массива равен количеству найденных треков
    m_MidiTracks = new MidiTrack[cTracks];
    if (m_MidiTracks == NULL)
    {
    	LOG("operator new failed\n");
    	return MIDIFILE_CANT_ALLOC_MEMORY;
    }

	// подключаем объекты класса MidiTrack к непустым трекам MIDI-файла
   	iCurByte = 14;
   	DWORD iCurTrack = 0;
	while (iCurByte + 7 < cbFile)
	{
		// считываем сигнатуру текущего блока
		DWORD CurChunkSignature = GetBigEndianDword(pFile + iCurByte);
		iCurByte += 4;

		// считываем размер данных текущего блока в байтах
		DWORD cbChunk = GetBigEndianDword(pFile + iCurByte);
		iCurByte += 4;

		// если данные текущего блока выходят за пределы файла, то соответственно
		// уменьшаем размер данных блока
		if (iCurByte + cbChunk > cbFile) cbChunk = cbFile - iCurByte;

		if (CurChunkSignature == 'MTrk' && cbChunk > 0)
		{
			// подключаем текущий объект класса MidiTrack к данному треку MIDI-файла
			if (m_MidiTracks[iCurTrack].AttachToTrack(pFile + iCurByte, cbChunk))
			{
				// текущий объект успешно подключен, делаем текущим следующий объект
				iCurTrack++;
			}
		}

		iCurByte += cbChunk;
	}

	// проверяем количество объектов класса MidiTrack, которые удалось подключить
    // к трекам MIDI-файла
	if (iCurTrack == 0)
	{
		LOG("no valid tracks in MIDI file\n");
		return MIDIFILE_NO_VOCAL_PARTS;
	}

	// сохраняем количество действительных элеменов в массиве m_MidiTracks
	m_cTracks = iCurTrack;

	// ищем слова песни
	MIDIFILERESULT Res = FindLyric();
	if (Res != MIDIFILE_SUCCESS)
	{
		LOG("lyric not found\n");
		return Res;
	}

	// ищем вокальные партии
	Res = FindVocalParts();
	if (Res != MIDIFILE_SUCCESS)
	{
		LOG("vocal parts not found\n");
		return Res;
	}

	// запоминаем указатель на буфер памяти, в который загружен MIDI-файл
	m_pFile = pFile;

	return MIDIFILE_SUCCESS;
}

/****************************************************************************************
*
*   Метод CreateSong
*
*   Параметры
*       Нет
*
*   Возвращаемое значение
*       Указатель на объект класса Song, или NULL, если произошла ошибка. В норме,
*       единственно возможной ошибкой может быть ошибка при выделении памяти.
*
*   Создаёт объект класса Song, представляющий собой песню.
*
*   Реализация
*
*   В качестве вокальной партии метод берёт партию, описанную в первом элементе списка
*   вокальных партий m_pVocalPartList.
*
****************************************************************************************/

Song *MidiFile::CreateSong()
{
	if (m_pVocalPartList == NULL) return NULL;

	// указатель на первый элемент списка вокальных партий m_pVocalPartList
	VOCALPARTINFO *pVocalPart = m_pVocalPartList;

	// индекс трека в массиве m_MidiTracks, в котором располагается вокальная партия
	DWORD iTrack = pVocalPart->iTrack;

	// номер канала, в котором находится вокальная партия
	DWORD Channel = pVocalPart->Channel;

	// номер инструмента, которым играются ноты вокальной партии
	DWORD Instrument = pVocalPart->Instrument;

	// создаём объект класса MidiSong
	MidiSong MidiSong;

	// создаём ППС
	if (!CreateSes(iTrack, Channel, Instrument, &MidiSong))
	{
		LOG("MidiFile::CreateSes failed\n");
		return NULL;
	}

	// создаём последовательность тактов
	if (!CreateMeasureSequence(&MidiSong))
	{
		LOG("MidiFile::CreateMeasureSequence failed\n");
		return NULL;
	}

	// создаём карту темпов
	if (!CreateTempoMap(&MidiSong))
	{
		LOG("MidiFile::CreateTempoMap failed\n");
		return NULL;
	}

	// корректируем текст слов песни в ППС
	if (!MidiSong.CorrectLyric())
	{
		LOG("MidiSong::CorrectLyric failed\n");
		return NULL;
	}

	// создаём объект класса Song
	Song *pSong = MidiSong.CreateSong(m_cTicksPerMidiQuarterNote);
	if (pSong == NULL)
	{
		LOG("MidiSong::CreateSong failed\n");
		return NULL;
	}

	return pSong;
}

/****************************************************************************************
*
*   Метод Free
*
*   Параметры
*       Нет
*
*   Возвращаемое значение
*       Нет
*
*   Освобождает все выделенные ресурсы.
*
****************************************************************************************/

void MidiFile::Free()
{
	VOCALPARTINFO *pCurVocalPart = m_pVocalPartList;
	while (pCurVocalPart != NULL)
	{
		VOCALPARTINFO *pDelVocalPart = pCurVocalPart;
		pCurVocalPart = pCurVocalPart->pNext;
		delete pDelVocalPart;
	}
	m_pVocalPartList = NULL;

	if (m_pLyricTrack != NULL)
	{
		delete m_pLyricTrack;
		m_pLyricTrack = NULL;
	}

	if (m_MidiTracks != NULL)
	{
		delete m_MidiTracks;
		m_MidiTracks = NULL;
	}

	if (m_pFile != NULL)
	{
		HeapFree(GetProcessHeap(), 0, m_pFile);
		m_pFile = NULL;
	}
}

/****************************************************************************************
*
*   Метод FindLyric
*
*   Параметры
*       Нет
*
*   Возвращаемое значение
*       MIDIFILE_SUCCESS - слова песни найдены;
*       MIDIFILE_NO_LYRIC - слова песни не найдены;
*       MIDIFILE_CANT_ALLOC_MEMORY - не удалось выделить память.
*
*   Ищет слова песни. Слова песни могут находиться либо в метасобытиях LYRIC, либо
*   в метасобытиях TEXT_EVENT. В случае успеха метод создаёт объект класса MidiTrack,
*   подключает его к треку со словами и сохраняет указатель на этот объект в переменной
*   m_pLyricTrack. Кроме того, в переменной m_LyricEventType метод сохраняет тип
*   метасобытий, в которых были найдены слова песни.
*
****************************************************************************************/

MIDIFILERESULT MidiFile::FindLyric()
{
	if (m_pLyricTrack != NULL)
	{
		// удаляем старый объект класса MidiTrack, подключенный к треку со словами песни
		delete m_pLyricTrack;
		m_pLyricTrack = NULL;
	}

	if (m_MidiTracks == NULL) return MIDIFILE_NO_LYRIC;

	// текущий тип метасобытий, в которых ищутся слова песни;
	// начинаем поиск с метасобытий типа LYRIC
	DWORD CurLyricEventType = LYRIC;

	// в этом цикле перебираются типы метасобытий: LYRIC и TEXT_EVENT
	while(true)
	{
		// индекс трека в массиве m_MidiTracks, содержащего максимальное на данный
		// момент количество символов в допустимых метасобытиях типа CurLyricEventType
		DWORD iTrackWithMaxSymbols = 0;

		// количество символов в допустимых метасобытиях типа CurLyricEventType из трека
		// iTrackWithMaxSymbols, оно является максимальным на данный момент
		DWORD cMaxSymbols = 0;

		// количество допустимых метасобытий типа CurLyricEventType в треке
		// iTrackWithMaxSymbols
		DWORD cEventsInTrackWithMaxSymbols = 0;

		// цикл по всем трекам массива m_MidiTracks
		for (DWORD i = 0; i < m_cTracks; i++)
		{
			// количество символов в допустимых метасобытиях типа CurLyricEventType из
			// текущего трека
			DWORD cSymbolsInTrack = 0;

			// количество допустимых метасобытий типа CurLyricEventType в текущем треке
			DWORD cEventsInTrack = 0;

			// объект класса MidiLyric для поиска допустимых метасобытий типа
			// CurLyricEventType в текущем треке
			MidiLyric Lyric;

			// инициализируем поиск допустимых метасобытий типа CurLyricEventType
			// в текущем треке
			Lyric.InitSearch(&m_MidiTracks[i], CurLyricEventType, m_DefaultCodePage);

			// цикл по всем допустимым метасобытиям типа CurLyricEventType из текущего
			// трека
			while (true)
			{
				// количество символов в тексте очередного допустимого метасобытия
				DWORD cchEventText;

				// получаем очередное допустимое метасобытие типа CurLyricEventType
				MIDILYRICRESULT Res = Lyric.GetNextValidEvent(NULL, NULL, &cchEventText);

				// если метасобытия типа CurLyricEventType в текущем треке кончились,
				// выходим из цикла
				if (Res == MIDILYRIC_LYRIC_END) break;

				cSymbolsInTrack += cchEventText;
				cEventsInTrack++;
			}

			if (cSymbolsInTrack > cMaxSymbols)
			{
				// количество символов в допустимых метасобытиях типа CurLyricEventType
				// из текущего трека максимально среди всех просмотренных треков

				iTrackWithMaxSymbols = i;
				cMaxSymbols = cSymbolsInTrack;
				cEventsInTrackWithMaxSymbols = cEventsInTrack;
			}
		}

		if (CurLyricEventType == LYRIC)
		{
			if (cEventsInTrackWithMaxSymbols > 0)
			{
				// слова песни найдены;
				// создаём объект класса MidiTrack - копию объекта,
				// подключенного к треку со словами песни;
				m_pLyricTrack = m_MidiTracks[iTrackWithMaxSymbols].Duplicate();

				if (m_pLyricTrack == NULL)
				{
					LOG("MidiTrack::Duplicate failed\n");
					return MIDIFILE_CANT_ALLOC_MEMORY;
				}

				// слова песни находятся в метасобытиях типа LYRIC
				m_LyricEventType = LYRIC;

				return MIDIFILE_SUCCESS;
			}

			// слова песни в метасобытиях типа LYRIC не найдены,
			// поищем слова песни в метасобытиях типа TEXT_EVENT
			CurLyricEventType = TEXT_EVENT;
		}
		else // CurLyricEventType == TEXT_EVENT
		{
			if (cEventsInTrackWithMaxSymbols >= MIN_TEXT_EVENTS_FOR_LYRIC_TRACK)
			{
				// слова песни найдены;
				// создаём объект класса MidiTrack - копию объекта,
				// подключенного к треку со словами песни;
				m_pLyricTrack = m_MidiTracks[iTrackWithMaxSymbols].Duplicate();

				if (m_pLyricTrack == NULL)
				{
					LOG("MidiTrack::Duplicate failed\n");
					return MIDIFILE_CANT_ALLOC_MEMORY;
				}

                // слова песни находятся в метасобытиях типа TEXT_EVENT
				m_LyricEventType = TEXT_EVENT;

				return MIDIFILE_SUCCESS;
			}

			// слова песни не найдены
			return MIDIFILE_NO_LYRIC;
		}
	}
}

/****************************************************************************************
*
*   Метод FindVocalParts
*
*   Параметры
*       Нет
*
*   Возвращаемое значение
*       MIDIFILE_SUCCESS - найдена как минимум одна вокальная партия;
*       MIDIFILE_NO_VOCAL_PARTS - не найдена ни одна вокальная партия;
*       MIDIFILE_CANT_ALLOC_MEMORY - не удалось выделить память.
*
*   Ищет вокальные партии.
*
*   Реализация
*
*   Поиск вокальных партий осуществляется в несколько этапов. Переход на следующий этап
*   происходит, если на текущем этапе не найдено ни одной вокальной партии. Для каждой
*   партии - претендента на звание вокальной - вычисляется мера сходства партии и слов
*   песни. Если найденное значение меры сходства превышает MAX_VOCAL_PART_LYRIC_DISTANCE,
*   то партия отбрасывается, а иначе добавляется в список вокальных партий
*   m_pVocalPartList.
*
****************************************************************************************/

MIDIFILERESULT MidiFile::FindVocalParts()
{
	// очищаем список вокальных партий
	VOCALPARTINFO *pCurVocalPart = m_pVocalPartList;
	while (pCurVocalPart != NULL)
	{
		VOCALPARTINFO *pDelVocalPart = pCurVocalPart;
		pCurVocalPart = pCurVocalPart->pNext;
		delete pDelVocalPart;
	}
	m_pVocalPartList = NULL;

	// массив, каждый элемент которого описывает ограничения очередного этапа поиска
	// вокальных партий:
	// 1) первое поле элемента задаёт максимально допустимую долю перекрытий нот
	//    по времени по отношению к количеству одиночных нот в партии;
	// 2) второе поле элемента задаёт набор флагов, определяющих критерии поиска
	//    вокальных партий
	VOCAL_PART_LIMITATIONS Limitations[] =
	{
		{ 0.0, IDENTICAL_START | IDENTICAL_END | LIMIT_NOTES_PER_METAEVENT },
		{ 0.5, IDENTICAL_START | IDENTICAL_END | LIMIT_NOTES_PER_METAEVENT },
		{ 0.5, IDENTICAL_START | LIMIT_NOTES_PER_METAEVENT },
		{ 0.5, IDENTICAL_END | LIMIT_NOTES_PER_METAEVENT },
		{ 0.5, LIMIT_NOTES_PER_METAEVENT },
		{ 0.5, 0 },
	};

	const DWORD cStages = sizeof(Limitations) / sizeof(Limitations[0]);

	// цикл по этапам поиска вокальных партий
	for (DWORD iStage = 0; iStage < cStages; iStage++)
	{
		// цикл по всем трекам массива m_MidiTracks
		for (DWORD iCurTrack = 0; iCurTrack < m_cTracks; iCurTrack++)
		{
			// матрица, в которой будут отмечены партии, существующие в текущем треке;
			// если значение элемента матрицы с индексом [i][j] равно true, то в треке
			// существует партия, играемая на канале i инструментом j
			bool DoesPartExist[MAX_MIDI_CHANNELS][MAX_MIDI_INSTRUMENTS];

			// инициализируем элементы матрицы DoesPartExist значениями false
			for (DWORD i = 0; i < MAX_MIDI_CHANNELS; i++)
				for (DWORD j = 0; j < MAX_MIDI_INSTRUMENTS; j++)
					DoesPartExist[i][j] = false;

			// количество партий в текущем треке;
			// оно равно количеству элементов в матрице DoesPartExist со значением true
			DWORD cPartsInTrack = 0;

			// устанавливает текущую позицию на начало трека
			m_MidiTracks[iCurTrack].ResetCurrentPosition();

			// цикл по всем событиям текущего трека, в этом цикле ищутся партии
			while (true)
			{
				// указатель на первый байт данных события
				PBYTE pEventData;

				// номер канала события
				DWORD EventChannel;

				// считываем очередное событие трека
				DWORD EventType = m_MidiTracks[iCurTrack].GetNextEvent(NULL,
					&pEventData, &EventChannel);

				// если в треке больше не осталось событий, выходим из цикла
				if (EventType == REAL_TRACK_END) break;

				// отбрасываем любые события, кроме событий PROGRAM_CHANGE
				if (EventType != PROGRAM_CHANGE) continue;

				// находим номер инструмента
				DWORD EventInstrument = *pEventData;

				if (!DoesPartExist[EventChannel][EventInstrument])
				{
					// в текущем треке найдена новая партия, играемая на канале
					// EventChannel инструментом EventInstrument
					DoesPartExist[EventChannel][EventInstrument] = true;
					cPartsInTrack++;
				}
			}

			// если не найдено ни одной партии, то переходим к следующему треку
			if (cPartsInTrack == 0) continue;

            // количество пустых партий (т.е. партий, не содержащих ноты) среди партий
            // из матрицы DoesPartExist
			DWORD cEmptyParts = 0;

            // цикл по всем элементам матрицы DoesPartExist: ищем вокальные партии

			for (DWORD CurChannel = 0; CurChannel < MAX_MIDI_CHANNELS; CurChannel++)
			{
				for (DWORD CurInstr = 0; CurInstr < MAX_MIDI_INSTRUMENTS; CurInstr++)
				{
					// если в текущем треке нет партии, играемой на канале CurChannel
                    // инструментом CurInstr, то переходим к следующему элементу матрицы
					if (!DoesPartExist[CurChannel][CurInstr]) continue;

					// переменная, в которую метод GetPartLyricDistance запишет меру
					// сходства текущей партии и слов песни в четвертных MIDI-нотах
					double PartLyricDistance;

					// находим меру сходства текущей партии и слов песни
					DISTANCERESULT DistanceRes = GetPartLyricDistance(iCurTrack,
						CurChannel, CurInstr, Limitations[iStage].OverlapsThreshold,
						Limitations[iStage].fCriteria, &PartLyricDistance);

					if (DistanceRes == DISTANCE_CANT_ALLOC_MEMORY)
					{
						LOG("MidiFile::GetPartLyricDistance failed\n");
						return MIDIFILE_CANT_ALLOC_MEMORY;
					}
					else if (DistanceRes == DISTANCE_NO_NOTES)
					{
						cEmptyParts++;
					}
					else if (DistanceRes == DISTANCE_SUCCESS)
					{
						// если найденное значение меры сходства текущей партии и слов
						// песни не превышает MAX_VOCAL_PART_LYRIC_DISTANCE, то добавляем
						// партию в список вокальных партий m_pVocalPartList
						if (PartLyricDistance <= MAX_VOCAL_PART_LYRIC_DISTANCE)
						{
							bool bIsPartAdded = AddVocalPart(iCurTrack, CurChannel,
								CurInstr, PartLyricDistance);

							if (!bIsPartAdded)
							{
								LOG("MidiFile::AddVocalPart failed\n");
								return MIDIFILE_CANT_ALLOC_MEMORY;
							}
						}
					}
				}
			}

			// если текущий трек содержит как минимум 2 непустые партии,
			// то проверяем, не является ли целый трек вокальной партией
			if (cPartsInTrack - cEmptyParts > 1)
			{
				// переменная, в которую метод GetPartLyricDistance запишет меру
				// сходства партии, соответствующей целому треку, и слов песни
				// в четвертных MIDI-нотах
				double PartLyricDistance;

				// находим меру сходства партии, соответствующей целому треку,
				// и слов песни
				DISTANCERESULT DistanceRes = GetPartLyricDistance(iCurTrack,
					ANY_CHANNEL, ANY_INSTRUMENT, Limitations[iStage].OverlapsThreshold,
					Limitations[iStage].fCriteria, &PartLyricDistance);

				if (DistanceRes == DISTANCE_CANT_ALLOC_MEMORY)
				{
					LOG("MidiFile::GetPartLyricDistance failed\n");
					return MIDIFILE_CANT_ALLOC_MEMORY;
				}
				else if (DistanceRes == DISTANCE_SUCCESS)
				{
					// если найденное значение меры сходства партии, соответствующей
					// целому треку, и слов песни не превышает
					// MAX_VOCAL_PART_LYRIC_DISTANCE, то добавляем партию в список
					// вокальных партий m_pVocalPartList
					if (PartLyricDistance <= MAX_VOCAL_PART_LYRIC_DISTANCE)
					{
						bool bIsPartAdded = AddVocalPart(iCurTrack, ANY_CHANNEL,
							ANY_INSTRUMENT, PartLyricDistance);

						if (!bIsPartAdded)
						{
							LOG("MidiFile::AddVocalPart failed\n");
							return MIDIFILE_CANT_ALLOC_MEMORY;
						}
					}
				}
			}
		}

		if (m_pVocalPartList != NULL)
		{
			// как минимум одна вокальная партия найдена, поэтому поиск прекращаем
			return MIDIFILE_SUCCESS;
		}
	}

	// не найдено ни одной вокальной партии
	return MIDIFILE_NO_VOCAL_PARTS;
}

/****************************************************************************************
*
*   Метод GetPartLyricDistance
*
*   Параметры
*       iTrack - индекс трека в массиве m_MidiTracks, в котором располагается партия
*       Channel - номер канала, в котором находится партия; если значение этого параметра
*                 равно ANY_CHANNEL, то к партии относятся ноты со всех каналов трека
*       Instrument - номер инструмента, которым играются ноты партии; если значение этого
*                    параметра равно ANY_INSTRUMENT, то к партии относятся ноты, играемые
*                    любым инструментом
*       OverlapsThreshold - максимально допустимая доля перекрытий нот по времени
*                           по отношению к количеству одиночных нот в партии
*       fCriteria - набор флагов, определяющих критерии, налагаемые на партию:
*                   IDENTICAL_START - одновременное начало;
*                   IDENTICAL_END - одновременный конец;
*                   LIMIT_NOTES_PER_METAEVENT - ограниченное количество нот на одно
*                                               метасобытие
*       pPartLyricDistance - указатель на переменную, в которую будет записана мера
*                            сходства партии и слов песни в четвертных MIDI-нотах
*
*   Возвращаемое значение
*       DISTANCE_SUCCESS - мера сходства успешно вычислена;
*       DISTANCE_NOT_VOCAL_PART - партия не является вокальной;
*       DISTANCE_NO_NOTES - в партии нет ни одной ноты;
*		DISTANCE_CANT_ALLOC_MEMORY - не удалось выделить память.
*
*   Вычисляет меру сходства партии, заданной параметрами iTrack, Channel и Instrument, и
*   слов песни, определяемых переменными объекта m_pLyricTrack и m_LyricEventType, в
*   четвертных MIDI-нотах.
*
*   Чтобы партия могла претендовать на звание вокальной, она должна удовлетворяет всем
*   наложенным на неё ограничениям. Если партия не удовлетворяет всем наложенным на неё
*   ограничениям, метод возвращает код ошибки DISTANCE_NOT_VOCAL_PART. Возможные
*   ограничения, налагаемые на партию:
*   1) для вокальной партии не должен быть превышен порог OverlapsThreshold;
*   2) для вокальной партии количество символов во всех метасобытиях, приходящихся на
*      одну ноту, не должно превышать MAX_SYMBOLS_PER_NOTE; для последней ноты это
*      ограничение действует с одной оговоркой: если не задан критерий IDENTICAL_END, то
*      метасобытия, следующие строго за концом последней ноты, игнорируются;
*   3) если задан критерий LIMIT_NOTES_PER_METAEVENT, то для вокальной партии количество
*      нот на одно метасобытие не должно превышать MAX_NOTES_PER_METAEVENT;
*   4) если задан критерий IDENTICAL_END, то для вокальной партии количество нот на
*      последнее метасобытие не должно превышать MAX_NOTES_PER_METAEVENT;
*   5) если задан критерий IDENTICAL_START, то для вокальной партии нота, ближайшая к
*      первому метасобытию, должна быть первой в партии;
*   6) если задан критерий IDENTICAL_START, то для вокальной партии расстояние от начала
*      первой ноты до первого метасобытия должно быть меньше или равно расстоянию от
*      начала первой ноты до второго метасобытия.
*
****************************************************************************************/

MidiFile::DISTANCERESULT MidiFile::GetPartLyricDistance(
	__in DWORD iTrack,
	__in DWORD Channel,
	__in DWORD Instrument,
	__in double OverlapsThreshold,
	__in DWORD fCriteria,
	__out double *pPartLyricDistance)
{
	// мера сходства партии и слов песни в тиках
	DWORD PartLyricDistance = 0;

	// объект класса MidiLyric для получения допустимых метасобытий типа m_LyricEventType
	// из трека со словами песни
	MidiLyric Lyric;

	// инициализируем поиск допустимых метасобытий типа m_LyricEventType
	// в треке со словами песни
	Lyric.InitSearch(m_pLyricTrack, m_LyricEventType, m_DefaultCodePage);

	// переменная, в которую метод GetNextValidEvent будет записывать количество тиков,
	// прошедшее от начала трека до момента прихода очередного допустимого метасобытия
	DWORD ctToLyricEvent;

	// переменная, в которую метод GetNextValidEvent будет записывать количество символов
	// в тексте очередного допустимого метасобытия
	DWORD cchEventText;

	// переменная, в которой будет накапливаться количество символов
	// в тексте всех метасобытий, приходящихся на ноту PrevNoteDesc
	DWORD cchPerNote = 0;

	// объект класса MidiPart для получения одиночных нот из партии
	MidiPart Part;

	// инициализируем поиск одиночных нот в партии
	Part.InitSearch(&m_MidiTracks[iTrack], Channel, Instrument, OverlapsThreshold,
		m_ConcordNoteChoice);

	// переменная, описывающая ноту, предшествующую текущей ноте
	NOTEDESC PrevNoteDesc;

	// переменная, описывающая текущую ноту, т.е. ноту, считанную последней
	NOTEDESC CurNoteDesc;

	// считываем первое метасобытие, оно всегда есть, т.е. LyricRes == MIDILYRIC_SUCCESS
	MIDILYRICRESULT LyricRes = Lyric.GetNextValidEvent(&ctToLyricEvent, NULL,
		&cchEventText);

	// считываем первую ноту
	MIDIPARTRESULT PartRes = Part.GetNextNote(&CurNoteDesc);

	// обрабатываем ошибки GetNextNote
	if (PartRes == MIDIPART_CANT_ALLOC_MEMORY)
	{
		LOG("MidiPart::GetNextNote failed\n");
		return DISTANCE_CANT_ALLOC_MEMORY;
	}
	else if (PartRes == MIDIPART_PART_END)
	{
		return DISTANCE_NO_NOTES;
	}
	else if (PartRes != MIDIPART_SUCCESS)
	{
		// здесь возможна ошибка MIDIPART_COMPLICATED_NOTE_COMBINATION
		// или MIDIPART_OVERLAPS_THRESHOLD_EXCEEDED
		return DISTANCE_NOT_VOCAL_PART;
	}

	// проверяем, что пришло раньше: первое метасобытие или первая нота
	if (ctToLyricEvent <= CurNoteDesc.ctToNoteOn)
	{
		// первое метасобытие пришло раньше или одновременно с первой нотой

		// расстояние от первого метасобытия до начала первой ноты,
		// которая в данном случае является ближайшей для него
		PartLyricDistance = Distance(ctToLyricEvent, CurNoteDesc.ctToNoteOn);

		// сохраняем количество символов в первом метасобытии
		cchPerNote = cchEventText;

		// считываем второе метасобытие
		LyricRes = Lyric.GetNextValidEvent(&ctToLyricEvent, NULL, &cchEventText);

		if (LyricRes == MIDILYRIC_LYRIC_END)
		{
			// в песне только одно метасобытие

			// если превышено ограничение на количество символов на ноту,
			// возвращаем ошибку
			if (cchPerNote > MAX_SYMBOLS_PER_NOTE) return DISTANCE_NOT_VOCAL_PART;

			if (fCriteria & LIMIT_NOTES_PER_METAEVENT || fCriteria & IDENTICAL_END)
			{
				// необходимо проверить, что количество нот на первое метасобытие
				// не превышает MAX_NOTES_PER_METAEVENT

				DWORD cNotesPerMetaEvent = 1;

				// цикл по нотам
				while (true)
				{
					// считываем очередную ноту
					PartRes = Part.GetNextNote(&CurNoteDesc);

					// обрабатываем ошибки GetNextNote
					if (PartRes == MIDIPART_CANT_ALLOC_MEMORY)
					{
						LOG("MidiPart::GetNextNote failed\n");
						return DISTANCE_CANT_ALLOC_MEMORY;
					}
					else if (PartRes == MIDIPART_PART_END)
					{
						break;
					}
					else if (PartRes != MIDIPART_SUCCESS)
					{
						// здесь возможна ошибка MIDIPART_COMPLICATED_NOTE_COMBINATION
						// или MIDIPART_OVERLAPS_THRESHOLD_EXCEEDED
						return DISTANCE_NOT_VOCAL_PART;
					}

					cNotesPerMetaEvent++;

					if (cNotesPerMetaEvent > MAX_NOTES_PER_METAEVENT)
					{
						// превышено ограничение на количество нот на метасобытие,
						// возвращаем ошибку
						return DISTANCE_NOT_VOCAL_PART;
					}
				}
			}

			// возвращаем меру сходства партии и слов песни в четвертных MIDI-нотах
			*pPartLyricDistance = (double) PartLyricDistance / m_cTicksPerMidiQuarterNote;
			return DISTANCE_SUCCESS;
		}

		if (fCriteria & IDENTICAL_START)
		{
			// необходимо проверить, что расстояние от начала первой ноты до первого
			// метасобытия не больше расстояния от начала первой ноты до второго
			// метасобытия
			if (PartLyricDistance > Distance(ctToLyricEvent, CurNoteDesc.ctToNoteOn))
			{
				// критерий IDENTICAL_START нарушен, возвращаем ошибку
				return DISTANCE_NOT_VOCAL_PART;
			}
		}

		// текущая нота становится предыдущей
		PrevNoteDesc = CurNoteDesc;

		// считываем вторую ноту
		PartRes = Part.GetNextNote(&CurNoteDesc);

		// обрабатываем ошибки GetNextNote
		if (PartRes == MIDIPART_CANT_ALLOC_MEMORY)
		{
			LOG("MidiPart::GetNextNote failed\n");
			return DISTANCE_CANT_ALLOC_MEMORY;
		}
		else if (PartRes == MIDIPART_PART_END)
		{
			// в песне только одна нота

			DWORD cchPerFirstNote = cchPerNote;

			// момент окончания первой ноты
			DWORD ctToNoteOff = PrevNoteDesc.ctToNoteOn + PrevNoteDesc.ctDuration;

			// цикл по метасобытиям
			while (true)
			{
				if (fCriteria & IDENTICAL_END)
				{
					// если задан критерий IDENTICAL_END,
					// то учитываем все метасобытия
					cchPerFirstNote += cchEventText;
				}
				else
				{
					// если не задан критерий IDENTICAL_END, то метасобытия,
					// следующие строго за концом последней ноты, игнорируются
					if (ctToLyricEvent <= ctToNoteOff)
					{
						cchPerFirstNote += cchEventText;
					}
				}

				if (cchPerFirstNote > MAX_SYMBOLS_PER_NOTE)
				{
					// превышено ограничение на количество символов на ноту,
					// возвращаем ошибку
					return DISTANCE_NOT_VOCAL_PART;
				}

				// расстояние от очередного метасобытия до начала первой ноты,
				// которая в данном случае является ближайшей для него
				PartLyricDistance += Distance(ctToLyricEvent, PrevNoteDesc.ctToNoteOn);

				// считываем очередное метасобытие
				LyricRes = Lyric.GetNextValidEvent(&ctToLyricEvent, NULL, &cchEventText);

				if (LyricRes == MIDILYRIC_LYRIC_END) break;
			}

			// возвращаем меру сходства партии и слов песни в четвертных MIDI-нотах
			*pPartLyricDistance = (double) PartLyricDistance / m_cTicksPerMidiQuarterNote;
			return DISTANCE_SUCCESS;
		}
		else if (PartRes != MIDIPART_SUCCESS)
		{
			// здесь возможна ошибка MIDIPART_COMPLICATED_NOTE_COMBINATION
			// или MIDIPART_OVERLAPS_THRESHOLD_EXCEEDED
			return DISTANCE_NOT_VOCAL_PART;
		}
	}
	else
	{
		// первая нота пришла раньше первого метасобытия

		// текущая нота становится предыдущей
		PrevNoteDesc = CurNoteDesc;

		// считываем вторую ноту
		PartRes = Part.GetNextNote(&CurNoteDesc);

		// обрабатываем ошибки GetNextNote
		if (PartRes == MIDIPART_CANT_ALLOC_MEMORY)
		{
			LOG("MidiPart::GetNextNote failed\n");
			return DISTANCE_CANT_ALLOC_MEMORY;
		}
		else if (PartRes == MIDIPART_PART_END)
		{
			// в песне только одна нота

			DWORD cchPerFirstNote = 0;

			// момент окончания первой ноты
			DWORD ctToNoteOff = PrevNoteDesc.ctToNoteOn + PrevNoteDesc.ctDuration;

			// цикл по метасобытиям
			while (true)
			{
				if (fCriteria & IDENTICAL_END)
				{
					// если задан критерий IDENTICAL_END,
					// то учитываем все метасобытия
					cchPerFirstNote += cchEventText;
				}
				else
				{
					// если не задан критерий IDENTICAL_END, то метасобытия,
					// следующие строго за концом последней ноты, игнорируются
					if (ctToLyricEvent <= ctToNoteOff)
					{
						cchPerFirstNote += cchEventText;
					}
				}

				if (cchPerFirstNote > MAX_SYMBOLS_PER_NOTE)
				{
					// превышено ограничение на количество символов на ноту,
					// возвращаем ошибку
					return DISTANCE_NOT_VOCAL_PART;
				}

				// расстояние от очередного метасобытия до начала первой ноты,
				// которая в данном случае является ближайшей для него
				PartLyricDistance += Distance(ctToLyricEvent, PrevNoteDesc.ctToNoteOn);

				// считываем очередное метасобытие
				LyricRes = Lyric.GetNextValidEvent(&ctToLyricEvent, NULL, &cchEventText);

				if (LyricRes == MIDILYRIC_LYRIC_END) break;
			}

			// возвращаем меру сходства партии и слов песни в четвертных MIDI-нотах
			*pPartLyricDistance = (double) PartLyricDistance / m_cTicksPerMidiQuarterNote;
			return DISTANCE_SUCCESS;
		}
		else if (PartRes != MIDIPART_SUCCESS)
		{
			// здесь возможна ошибка MIDIPART_COMPLICATED_NOTE_COMBINATION
			// или MIDIPART_OVERLAPS_THRESHOLD_EXCEEDED
			return DISTANCE_NOT_VOCAL_PART;
		}

		if (fCriteria & IDENTICAL_START)
		{
			if (!IsFirstNoteNearer(ctToLyricEvent, &PrevNoteDesc, &CurNoteDesc))
			{
				// первая нота не является ближайшей к первому метасобытию
				return DISTANCE_NOT_VOCAL_PART;
			}
		}
	}

	// главный цикл метода

	// в этой точке считано одно или два метасобытия и две ноты

	// если считано два метасобытия, то расстояние от первого метасобытия до начала
	// ближайшей ему ноты сохранено в переменной PartLyricDistance, а количество символов
	// в нём сохранено в переменной cchPerNote

	// количество тиков от начала трека до момента прихода последнего считанного
	// метасобытия сохранено в переменной ctToLyricEvent, а количество символов
	// в этом метасобытии - в переменной cchEventText

	// описание первой ноты сохранено в переменной PrevNoteDesc,
	// а описание второй - в переменной CurNoteDesc

	// главное условие, которое должно выполняться в начале цикла:
	// нота, предшествующая ноте PrevNoteDesc, не должна быть ближайшей нотой
	// к последнему считанному метасобытию

	// цикл по метасобытиям
	while (true)
	{
		// количество нот на предыдущее метасобытие
		DWORD cNotesPerMetaEvent = 0;

		// цикл по нотам
		while (!IsFirstNoteNearer(ctToLyricEvent, &PrevNoteDesc, &CurNoteDesc))
		{
			// сбрасываем счётчик количества символов, приходящихся на ноту PrevNoteDesc
			cchPerNote = 0;

			cNotesPerMetaEvent++;

			if (fCriteria & LIMIT_NOTES_PER_METAEVENT)
			{
				if (cNotesPerMetaEvent > MAX_NOTES_PER_METAEVENT)
				{
					// превышено ограничение на количество нот на метасобытие,
					// возвращаем ошибку
					return DISTANCE_NOT_VOCAL_PART;
				}
			}

			// нота PrevNoteDesc не является ближайшей к текущему метасобытию

			// текущая нота становится предыдущей
			PrevNoteDesc = CurNoteDesc;

			// считываем очередную ноту
			PartRes = Part.GetNextNote(&CurNoteDesc);

			// обрабатываем ошибки GetNextNote
			if (PartRes == MIDIPART_CANT_ALLOC_MEMORY)
			{
				LOG("MidiPart::GetNextNote failed\n");
				return DISTANCE_CANT_ALLOC_MEMORY;
			}
			else if (PartRes == MIDIPART_PART_END)
			{
				// ноты партии кончились

				DWORD cchPerLastNote = 0;

				// момент окончания последней ноты
				DWORD ctToNoteOff = PrevNoteDesc.ctToNoteOn + PrevNoteDesc.ctDuration;

				// цикл по метасобытиям
				while (true)
				{
					if (fCriteria & IDENTICAL_END)
					{
						// если задан критерий IDENTICAL_END,
						// то учитываем все метасобытия
						cchPerLastNote += cchEventText;
					}
					else
					{
						// если не задан критерий IDENTICAL_END, то метасобытия,
						// следующие строго за концом последней ноты, игнорируются
						if (ctToLyricEvent <= ctToNoteOff)
						{
							cchPerLastNote += cchEventText;
						}
					}

					if (cchPerLastNote > MAX_SYMBOLS_PER_NOTE)
					{
						// превышено ограничение на количество символов на ноту,
						// возвращаем ошибку
						return DISTANCE_NOT_VOCAL_PART;
					}

					// расстояние от очередного метасобытия до начала последней ноты,
					// которая в данном случае является ближайшей для него
					PartLyricDistance += Distance(ctToLyricEvent, PrevNoteDesc.ctToNoteOn);

					// считываем очередное метасобытие
					LyricRes = Lyric.GetNextValidEvent(&ctToLyricEvent, NULL,
						&cchEventText);

					if (LyricRes == MIDILYRIC_LYRIC_END) break;
				}

				// возвращаем меру сходства партии и слов песни в четвертных MIDI-нотах
				*pPartLyricDistance =
					(double) PartLyricDistance / m_cTicksPerMidiQuarterNote;
				return DISTANCE_SUCCESS;
			}
			else if (PartRes != MIDIPART_SUCCESS)
			{
				// здесь возможна ошибка MIDIPART_COMPLICATED_NOTE_COMBINATION
				// или MIDIPART_OVERLAPS_THRESHOLD_EXCEEDED
				return DISTANCE_NOT_VOCAL_PART;
			}
		}

		// прибавляем расстояние от текущего метасобытия до начала ближайшей к нему ноты
		// к переменной, накапливающей меру сходства между партией и словами песни
		PartLyricDistance += Distance(ctToLyricEvent, PrevNoteDesc.ctToNoteOn);

		// обновляем количество символов, приходящихся на ноту PrevNoteDesc
		cchPerNote += cchEventText;

		if (cchPerNote > MAX_SYMBOLS_PER_NOTE)
		{
			// превышено ограничение на количество символов на ноту, возвращаем ошибку
			return DISTANCE_NOT_VOCAL_PART;
		}

		// считываем очередное метасобытие
		LyricRes = Lyric.GetNextValidEvent(&ctToLyricEvent, NULL, &cchEventText);

		if (LyricRes == MIDILYRIC_LYRIC_END)
		{
			// метасобытия кончились

			if (fCriteria & IDENTICAL_END || fCriteria & LIMIT_NOTES_PER_METAEVENT)
			{
				// необходимо проверить, что количество нот на последнее метасобытие
				// не превышает MAX_NOTES_PER_METAEVENT

				// к последнему метасобытию относятся ноты PrevNoteDesc и CurNoteDesc
				cNotesPerMetaEvent = 2;

				// цикл по нотам
				while (true)
				{
					// считываем очередную ноту
					PartRes = Part.GetNextNote(&CurNoteDesc);

					// обрабатываем ошибки GetNextNote
					if (PartRes == MIDIPART_CANT_ALLOC_MEMORY)
					{
						LOG("MidiPart::GetNextNote failed\n");
						return DISTANCE_CANT_ALLOC_MEMORY;
					}
					else if (PartRes == MIDIPART_PART_END)
					{
						break;
					}
					else if (PartRes != MIDIPART_SUCCESS)
					{
						// здесь возможна ошибка MIDIPART_COMPLICATED_NOTE_COMBINATION
						// или MIDIPART_OVERLAPS_THRESHOLD_EXCEEDED
						return DISTANCE_NOT_VOCAL_PART;
					}

					cNotesPerMetaEvent++;

					if (cNotesPerMetaEvent > MAX_NOTES_PER_METAEVENT)
					{
						// превышено ограничение на количество нот на метасобытие,
						// возвращаем ошибку
						return DISTANCE_NOT_VOCAL_PART;
					}
				}
			}

			// возвращаем меру сходства партии и слов песни в четвертных MIDI-нотах
			*pPartLyricDistance = (double) PartLyricDistance / m_cTicksPerMidiQuarterNote;
			return DISTANCE_SUCCESS;
		}
	}
}

/****************************************************************************************
*
*   Метод IsFirstNoteNearer
*
*   Параметры
*       ctToLyricEvent - количество тиков, прошедшее от начала трека до момента прихода
*                        метасобытия
*       pFirstNote - указатель на структуру NOTEDESC, описывающую первую ноту
*       pSecondNote - указатель на структуру NOTEDESC, описывающую вторую ноту
*
*   Возвращаемое значение
*       true, если из двух нот первая нота является ближайшей к метасобытию; иначе false.
*
*   Проверяет, какая из двух нот является ближайшей к метасобытию.
*
****************************************************************************************/

bool MidiFile::IsFirstNoteNearer(
	__in DWORD ctToLyricEvent,
	__in NOTEDESC *pFirstNote,
	__in NOTEDESC *pSecondNote)
{
	// момент окончания первой ноты
	DWORD ctToFirstNoteOff = pFirstNote->ctToNoteOn + pFirstNote->ctDuration;

	if (ctToLyricEvent >= ctToFirstNoteOff)
	{
		// метасобытие пришло одновременно с концом или позже первой ноты

		if (ctToLyricEvent >= pSecondNote->ctToNoteOn)
		{
			// первая нота не является ближайшей к метасобытию
			return false;
		}

		// метасобытие пришло между первой и второй нотами

		if (ctToLyricEvent - ctToFirstNoteOff > pSecondNote->ctToNoteOn - ctToLyricEvent)
		{
			// первая нота не является ближайшей к метасобытию
			return false;
		}
	}

	return true;
}

/****************************************************************************************
*
*   Метод AddVocalPart
*
*   Параметры
*       iTrack - индекс трека в массиве m_MidiTracks, в котором располагается партия
*       Channel - номер канала, в котором находится партия; если значение этого параметра
*                 равно ANY_CHANNEL, то к партии относятся ноты со всех каналов трека
*       Instrument - номер инструмента, которым играются ноты данной партии; если
*                    значение этого параметра равно ANY_INSTRUMENT, то к партии относятся
*                    ноты, играемые любым инструментом
*       PartLyricDistance - мера сходства партии и слов песни в четвертных MIDI-нотах
*
*   Возвращаемое значение
*       true в случае успеха; false, если не удалось выделить память.
*
*   Добавляет партию, заданную параметрами iTrack, Channel и Instrument, в список
*   вокальных партий m_pVocalPartList. Партия вставляется перед первым элементом списка,
*   у которого мера сходства численно больше, чем PartLyricDistance. Таким образом,
*   партии в списке упорядочены по убыванию сходства со словами песни.
*
****************************************************************************************/

bool MidiFile::AddVocalPart(
	__in DWORD iTrack,
	__in DWORD Channel,
	__in DWORD Instrument,
	__in double PartLyricDistance)
{
	// выделяем память для новой вокальной партии
	VOCALPARTINFO *pNewVocalPart = new VOCALPARTINFO;
	if (pNewVocalPart == NULL)
	{
		LOG("operator new failed\n");
		return false;
	}

	// заполняем поля поля структуры для новой вокальной партии значениями параметров
	pNewVocalPart->iTrack = iTrack;
	pNewVocalPart->Channel = Channel;
	pNewVocalPart->Instrument = Instrument;
	pNewVocalPart->PartLyricDistance = PartLyricDistance;

	// добавляем новую вокальную партию в список m_pVocalPartList

	VOCALPARTINFO **ppCurVocalPart = &m_pVocalPartList;

    // ищем первый элемент списка, у которого мера сходства численно больше,
    // чем PartLyricDistance
	while (*ppCurVocalPart != NULL &&
		(*ppCurVocalPart)->PartLyricDistance <= PartLyricDistance)
	{
		ppCurVocalPart = &((*ppCurVocalPart)->pNext);
	}

	// вставляем новую вокальную партию перед элементом списка, у которого мера сходства
	// численно больше, чем PartLyricDistance
	pNewVocalPart->pNext = *ppCurVocalPart;
	*ppCurVocalPart = pNewVocalPart;

	return true;
}

/****************************************************************************************
*
*   Метод CreateSes
*
*   Параметры
*       iTrack - индекс трека в массиве m_MidiTracks, в котором располагается партия
*       Channel - номер канала, в котором находится партия; если значение этого параметра
*                 равно ANY_CHANNEL, то к партии относятся ноты со всех каналов трека
*       Instrument - номер инструмента, которым играются ноты партии; если значение этого
*                    параметра равно ANY_INSTRUMENT, то к партии относятся ноты, играемые
*                    любым инструментом
*       pMidiSong - указатель на объект класса MidiSong, в котором надо создать ППС
*
*   Возвращаемое значение
*       true в случае успеха; false, если произошла ошибка. В норме, единственно
*       возможной ошибкой может быть ошибка при выделении памяти.
*
*   Создаёт ППС в объекте класса MidiSong. В качестве вокальной партии метод берёт
*   партию, описанную параметрами iTrack, Channel и Instrument.
*
*   Обработка текста, осуществляемая этим методом, называется третьим этапом обработки
*   слов песни.
*
****************************************************************************************/

bool MidiFile::CreateSes(
	__in DWORD iTrack,
	__in DWORD Channel,
	__in DWORD Instrument,
	__in MidiSong *pMidiSong)
{
	// объект класса MidiLyric для получения допустимых метасобытий типа m_LyricEventType
	// из трека со словами песни
	MidiLyric Lyric;

	// инициализируем поиск допустимых метасобытий типа m_LyricEventType
	// в треке со словами песни
	Lyric.InitSearch(m_pLyricTrack, m_LyricEventType, m_DefaultCodePage);

	// переменная, в которую метод GetNextValidEvent будет записывать количество тиков,
	// прошедшее от начала трека до момента прихода очередного допустимого метасобытия
	DWORD ctToLyricEvent;

	// буфер, в который метод GetNextValidEvent будет записывать текст очередного
	// допустимого метасобытия
	WCHAR pwsEventText[MAX_SYMBOLS_PER_LYRIC_EVENT];

	// буфер, в котором будет накапливаться текст метасобытий, относящихся к одной ноте
	WCHAR pwsNoteText[MAX_SYMBOLS_PER_NOTE];

	// переменная, в которую метод GetNextValidEvent будет записывать количество символов
	// в тексте очередного допустимого метасобытия
	DWORD cchEventText;

	// объект класса MidiPart для получения одиночных нот из партии
	MidiPart Part;

	// инициализируем поиск одиночных нот в партии
	Part.InitSearch(&m_MidiTracks[iTrack], Channel, Instrument, 1.0, m_ConcordNoteChoice);

	// переменная, описывающая ноту, предшествующую текущей ноте
	NOTEDESC PrevNoteDesc;

	// переменная, описывающая текущую ноту, т.е. ноту, считанную последней
	NOTEDESC CurNoteDesc;

	// считываем первое метасобытие, оно всегда есть, т.е. LyricRes == MIDILYRIC_SUCCESS
	MIDILYRICRESULT LyricRes = Lyric.GetNextValidEvent(&ctToLyricEvent, pwsEventText,
		&cchEventText);

	// считываем первую ноту
	MIDIPARTRESULT PartRes = Part.GetNextNote(&PrevNoteDesc);

	// обрабатываем ошибки GetNextNote
	if (PartRes != MIDIPART_SUCCESS)
	{
		// здесь возможна только ошибка MIDIPART_CANT_ALLOC_MEMORY;
		// ошибки MIDIPART_PART_END, MIDIPART_COMPLICATED_NOTE_COMBINATION и
		// MIDIPART_OVERLAPS_THRESHOLD_EXCEEDED здесь невозможны
		LOG("MidiPart::GetNextNote failed\n");
		return false;
	}

	// считываем вторую ноту
	PartRes = Part.GetNextNote(&CurNoteDesc);

	// обрабатываем ошибки GetNextNote
	if (PartRes == MIDIPART_PART_END)
	{
		// в песне только одна нота

		// переменная, хранящая текущее количество символов на ноту
		DWORD cchPerNote = 0;

		// момент окончания первой (и последней) ноты
		DWORD ctToNoteOff = PrevNoteDesc.ctToNoteOn + PrevNoteDesc.ctDuration;

		// цикл по метасобытиям
		while (true)
		{
			// если превышено ограничение на количество символов на ноту,
			// отбрасываем это метасобытие и все последующие за ним
			if (cchPerNote + cchEventText > MAX_SYMBOLS_PER_NOTE) break;

			// копируем текст текущего метасобытия в накапливающий буфер
			wcsncpy(pwsNoteText + cchPerNote, pwsEventText, cchEventText);

			// обновляем количество символов в накапливающем буфере
			cchPerNote += cchEventText;

			// считываем очередное метасобытие
			LyricRes = Lyric.GetNextValidEvent(&ctToLyricEvent, pwsEventText,
				&cchEventText);

			if (LyricRes == MIDILYRIC_LYRIC_END) break;

			// если очередное метасобытие начинается спустя более чем восьмую
			// MIDI-ноту после окончания первой (и последней) ноты партии,
			// отбрасываем это метасобытие и все последующие за ним
			if (ctToLyricEvent > ctToNoteOff &&
				ctToLyricEvent - ctToNoteOff > m_cTicksPerMidiQuarterNote/2)
			{
				break;
			}
		}

		// добавляем в ППС ноту PrevNoteDesc с текстом в буфере pwsNoteText
		bool bEventAdded = pMidiSong->AddSingingEvent(PrevNoteDesc.NoteNumber,
			PrevNoteDesc.ctToNoteOn, PrevNoteDesc.ctDuration, pwsNoteText, cchPerNote);

		// если не удалось выделить память, возвращаем ошибку
		if (!bEventAdded)
		{
			LOG("MidiSong::AddSingingEvent failed\n");
			return false;
		}

		return true;
	}
	else if (PartRes != MIDIPART_SUCCESS)
	{
		// здесь возможна только ошибка MIDIPART_CANT_ALLOC_MEMORY;
		// ошибки MIDIPART_COMPLICATED_NOTE_COMBINATION и
		// MIDIPART_OVERLAPS_THRESHOLD_EXCEEDED здесь невозможны
		LOG("MidiPart::GetNextNote failed\n");
		return false;
	}

	if (ctToLyricEvent > PrevNoteDesc.ctToNoteOn)
	{
		// первая нота предшествует первому метасобытию,
		// необходимо найти ближайшую к первому метасобытию ноту

		// цикл по нотам
		while (!IsFirstNoteNearer(ctToLyricEvent, &PrevNoteDesc, &CurNoteDesc))
		{
			// текущая нота становится предыдущей
			PrevNoteDesc = CurNoteDesc;

			// считываем очередную ноту
			PartRes = Part.GetNextNote(&CurNoteDesc);

			// обрабатываем ошибки GetNextNote
			if (PartRes == MIDIPART_PART_END)
			{
				// ближайшей к первому метасобытию является последняя нота партии

				// переменная, хранящая текущее количество символов на ноту
				DWORD cchPerNote = 0;

				// момент окончания последней ноты
				DWORD ctToNoteOff = PrevNoteDesc.ctToNoteOn + PrevNoteDesc.ctDuration;

				// цикл по метасобытиям
				while (true)
				{
					// если превышено ограничение на количество символов на ноту,
					// отбрасываем это метасобытие и все последующие за ним
					if (cchPerNote + cchEventText > MAX_SYMBOLS_PER_NOTE) break;

					// копируем текст текущего метасобытия в накапливающий буфер
					wcsncpy(pwsNoteText + cchPerNote, pwsEventText, cchEventText);

					// обновляем количество символов в накапливающем буфере
					cchPerNote += cchEventText;

					// считываем очередное метасобытие
					LyricRes = Lyric.GetNextValidEvent(&ctToLyricEvent, pwsEventText,
						&cchEventText);

					if (LyricRes == MIDILYRIC_LYRIC_END) break;

					// если очередное метасобытие начинается спустя более чем восьмую
					// MIDI-ноту после окончания последней ноты партии,
					// отбрасываем это метасобытие и все последующие за ним
					if (ctToLyricEvent > ctToNoteOff &&
						ctToLyricEvent - ctToNoteOff > m_cTicksPerMidiQuarterNote/2)
					{
						break;
					}
				}

				// добавляем в ППС ноту PrevNoteDesc с текстом в буфере pwsNoteText
				bool bEventAdded = pMidiSong->AddSingingEvent(PrevNoteDesc.NoteNumber,
					PrevNoteDesc.ctToNoteOn, PrevNoteDesc.ctDuration, pwsNoteText,
					cchPerNote);

				// если не удалось выделить память, возвращаем ошибку
				if (!bEventAdded)
				{
					LOG("MidiSong::AddSingingEvent failed\n");
					return false;
				}

				return true;
			}
			else if (PartRes != MIDIPART_SUCCESS)
			{
				// здесь возможна только ошибка MIDIPART_CANT_ALLOC_MEMORY;
				// ошибки MIDIPART_COMPLICATED_NOTE_COMBINATION и
				// MIDIPART_OVERLAPS_THRESHOLD_EXCEEDED здесь невозможны
				LOG("MidiPart::GetNextNote failed\n");
				return false;
			}
		}
	}

	// главный цикл метода

	// в этой точке считано одно метасобытие и как минимум две ноты,
	// причём нота PrevNoteDesc является ближайшей к первому метасобытию

	// количество тиков от начала трека до момента прихода первого метасобытия
	// сохранено в переменной ctToLyricEvent, а количество символов
	// в этом метасобытии - в переменной cchEventText

	// главное условие, которое должно выполняться в начале цикла:
	// нота PrevNoteDesc должна быть ближайшей к последнему считанному метасобытию

	// на каждой итерации этого цикла в ППС добавляется ровно одна нота со словами
	// и, возможно, одна и более нот без слов
	while (true)
	{
		// переменная, хранящая текущее количество символов на ноту
		DWORD cchPerNote = 0;

		// цикл по метасобытиям; находим все метасобытия,
		// для которых нота PrevNoteDesc ближайшая
		do
		{
			// если превышено ограничение на количество символов на ноту,
			// возвращаем ошибку, хотя эта ситуация невозможна
			if (cchPerNote + cchEventText > MAX_SYMBOLS_PER_NOTE)
			{
				LOG("number of symbols per note exceeded, this cannot happen!\n");
				return false;
			}

			// копируем текст текущего метасобытия в накапливающий буфер
			wcsncpy(pwsNoteText + cchPerNote, pwsEventText, cchEventText);

			// обновляем количество символов в накапливающем буфере
			cchPerNote += cchEventText;

			// считываем очередное метасобытие
			LyricRes = Lyric.GetNextValidEvent(&ctToLyricEvent, pwsEventText,
				&cchEventText);

			if (LyricRes == MIDILYRIC_LYRIC_END)
			{
				// все метасобытия считаны

				// добавляем в ППС ноту PrevNoteDesc с текстом в буфере pwsNoteText
				bool bEventAdded = pMidiSong->AddSingingEvent(PrevNoteDesc.NoteNumber,
					PrevNoteDesc.ctToNoteOn, PrevNoteDesc.ctDuration, pwsNoteText,
					cchPerNote);

				// если не удалось выделить память, возвращаем ошибку
				if (!bEventAdded)
				{
					LOG("MidiSong::AddSingingEvent failed\n");
					return false;
				}

				// количество нот без слов песни, которое можно добавить в ППС
				DWORD cNotesToAdd = MAX_NOTES_PER_METAEVENT - 1;

				// момент окончания последней добавленной в ППС ноты
				DWORD ctToLastNoteOff = PrevNoteDesc.ctToNoteOn + PrevNoteDesc.ctDuration;

				// считываем ноты, относящиеся к последнему метасобытию,
				// пока не будет превышено ограничение на количество нот на метасобытие
				while (cNotesToAdd > 0)
				{
					if (CurNoteDesc.ctToNoteOn - ctToLastNoteOff >
						m_cTicksPerMidiQuarterNote/2)
					{
						// превышено ограничение на расстояние между концом и началом
						// двух смежных нот, относящихся к одному метасобытию
						break;
					}

					// добавляем в ППС ноту CurNoteDesc без текста
					bool bEventAdded = pMidiSong->AddSingingEvent(CurNoteDesc.NoteNumber,
						CurNoteDesc.ctToNoteOn, CurNoteDesc.ctDuration, NULL, 0);

					// если не удалось выделить память, возвращаем ошибку
					if (!bEventAdded)
					{
						LOG("MidiSong::AddSingingEvent failed\n");
						return false;
					}

					// момент окончания последней добавленной в ППС ноты
					ctToLastNoteOff = CurNoteDesc.ctToNoteOn + CurNoteDesc.ctDuration;

					// текущая нота становится предыдущей
					PrevNoteDesc = CurNoteDesc;

					// считываем очередную ноту
					PartRes = Part.GetNextNote(&CurNoteDesc);

					// обрабатываем ошибки GetNextNote
					if (PartRes == MIDIPART_PART_END)
					{
						// все ноты кончились, выходим из цикла
						break;
					}
					else if (PartRes != MIDIPART_SUCCESS)
					{
						// здесь возможна только ошибка MIDIPART_CANT_ALLOC_MEMORY;
						// ошибки MIDIPART_COMPLICATED_NOTE_COMBINATION и
						// MIDIPART_OVERLAPS_THRESHOLD_EXCEEDED здесь невозможны
						LOG("MidiPart::GetNextNote failed\n");
						return false;
					}

					// обновляем количество нот без слов песни,
					// которое можно добавить в ППС
					cNotesToAdd--;
				}

				// ППС сформирована
				return true;
			}
		}
		while (IsFirstNoteNearer(ctToLyricEvent, &PrevNoteDesc, &CurNoteDesc));

		// добавляем в ППС ноту PrevNoteDesc с текстом в буфере pwsNoteText
		bool bEventAdded = pMidiSong->AddSingingEvent(PrevNoteDesc.NoteNumber,
			PrevNoteDesc.ctToNoteOn, PrevNoteDesc.ctDuration, pwsNoteText, cchPerNote);

		// если не удалось выделить память, возвращаем ошибку
		if (!bEventAdded)
		{
			LOG("MidiSong::AddSingingEvent failed\n");
			return false;
		}

		// количество нот без слов песни, которое можно добавить в ППС
		DWORD cNotesToAdd = MAX_NOTES_PER_METAEVENT - 1;

		// момент окончания последней добавленной в ППС ноты
		DWORD ctToLastNoteOff = PrevNoteDesc.ctToNoteOn + PrevNoteDesc.ctDuration;

		// цикл по нотам; ищем ноту, ближайшую к последнему считанному метасобытию
		while (true)
		{
			// текущая нота становится предыдущей
			PrevNoteDesc = CurNoteDesc;

			// считываем очередную ноту
			PartRes = Part.GetNextNote(&CurNoteDesc);

			// обрабатываем ошибки GetNextNote
			if (PartRes == MIDIPART_PART_END)
			{
				// переменная, хранящая текущее количество символов на ноту
				DWORD cchPerNote = 0;

				// момент окончания последней ноты партии
				DWORD ctToLastNoteOff = PrevNoteDesc.ctToNoteOn + PrevNoteDesc.ctDuration;

				// считываем метасобытия, относящиеся к последней ноте,
				// пока не будет превышено ограничение на количество символов на ноту
				while (cchPerNote + cchEventText <= MAX_SYMBOLS_PER_NOTE)
				{
					// копируем текст текущего метасобытия в накапливающий буфер
					wcsncpy(pwsNoteText + cchPerNote, pwsEventText, cchEventText);

					// обновляем количество символов в накапливающем буфере
					cchPerNote += cchEventText;

					// считываем очередное метасобытие
					LyricRes = Lyric.GetNextValidEvent(&ctToLyricEvent, pwsEventText,
						&cchEventText);

					// если все метасобытия считаны, выходим из цикла
					if (LyricRes == MIDILYRIC_LYRIC_END) break;

					if (ctToLyricEvent > ctToLastNoteOff &&
						ctToLyricEvent - ctToLastNoteOff > m_cTicksPerMidiQuarterNote/2)
					{
						// превышено ограничение на расстояние между концом последней
						// ноты и моментом прихода метасобытия, относящегося к ней
						break;
					}
				}

				// добавляем в ППС ноту PrevNoteDesc с текстом в буфере pwsNoteText
				bool bEventAdded = pMidiSong->AddSingingEvent(PrevNoteDesc.NoteNumber,
					PrevNoteDesc.ctToNoteOn, PrevNoteDesc.ctDuration, pwsNoteText,
					cchPerNote);

				// если не удалось выделить память, возвращаем ошибку
				if (!bEventAdded)
				{
					LOG("MidiSong::AddSingingEvent failed\n");
					return false;
				}

				// ППС сформирована
				return true;
			}
			else if (PartRes != MIDIPART_SUCCESS)
			{
				// здесь возможна только ошибка MIDIPART_CANT_ALLOC_MEMORY;
				// ошибки MIDIPART_COMPLICATED_NOTE_COMBINATION и
				// MIDIPART_OVERLAPS_THRESHOLD_EXCEEDED здесь невозможны
				LOG("MidiPart::GetNextNote failed\n");
				return false;
			}

			// если ближайшая к текущему метасобытию нота найдена, выходим из цикла
			if (IsFirstNoteNearer(ctToLyricEvent, &PrevNoteDesc, &CurNoteDesc)) break;

			if (cNotesToAdd > 0)
			{
				// ограничение на количество нот на метасобытие не превышено

				if (PrevNoteDesc.ctToNoteOn - ctToLastNoteOff <=
					m_cTicksPerMidiQuarterNote/2)
				{
					// ограничение на расстояние между концом и началом двух смежных нот,
					// относящихся к одному метасобытию, не превышено

					// добавляем в ППС ноту PrevNoteDesc без текста
					bool bEventAdded = pMidiSong->AddSingingEvent(PrevNoteDesc.NoteNumber,
						PrevNoteDesc.ctToNoteOn, PrevNoteDesc.ctDuration, NULL, 0);

					// если не удалось выделить память, возвращаем ошибку
					if (!bEventAdded)
					{
						LOG("MidiSong::AddSingingEvent failed\n");
						return false;
					}

					// момент окончания последней добавленной в ППС ноты
					ctToLastNoteOff = PrevNoteDesc.ctToNoteOn + PrevNoteDesc.ctDuration;
				}

				// обновляем количество нот без слов песни, которое можно добавить в ППС
				cNotesToAdd--;
			}
		}
	}
}

/****************************************************************************************
*
*   Метод CreateMeasureSequence
*
*   Параметры
*       pMidiSong - указатель на объект класса MidiSong, содержащий ППС
*
*   Возвращаемое значение
*       true, если последовательность тактов успешно создана; false, если не удалось
*       выделить память.
*
*   Создаёт последовательность тактов в объекте класса MidiSong.
*
****************************************************************************************/

bool MidiFile::CreateMeasureSequence(
	__inout MidiSong *pMidiSong)
{
	// устанавливаем текущую позицию на начало нулевого трека, в котором будем искать
    // метасобытия типа TIME_SIGNATURE
	m_MidiTracks[0].ResetCurrentPosition();

	// текущий числитель музыкального размера
	DWORD Numerator = 4;

	// текущий знаменатель музыкального размера
	DWORD Denominator = 4;

	// текущее количество тридцатьвторых в четвертной MIDI-ноте
	DWORD cNotated32ndNotesPerMidiQuarterNote = 8;

	// количество тиков от начала трека до конца последнего добавленного такта
	double ctToEndOfLastMeasure = 0;

	// текущее время в тиках, прошедшее с начала трека
	DWORD ctCurTime = 0;

	// цикл по событиям нулевого трека
    while (true)
	{
		// дельта-время события в тиках
		DWORD ctDeltaTime;

		// указатель на первый байт данных события
		BYTE *pEventData;

		// считываем очередное событие из нулевого трека
		DWORD Event = m_MidiTracks[0].GetNextEvent(&ctDeltaTime, &pEventData);

		if (Event == REAL_TRACK_END)
		{
			// в треке больше не осталось событий

			// устанавливаем текущую позицию на начало песни для пробега по ППС
			pMidiSong->ResetCurrentPosition();

			// количество тиков от начала трека до последней ноты
			DWORD ctToNoteOn;

			// длительность последней ноты в тиках
			DWORD ctDuration;

			// бежим до конца ППС
			while (pMidiSong->GetNextSingingEvent(NULL, &ctToNoteOn, &ctDuration,
				NULL, NULL));

			// количество тиков в такте
			double ctMeasure = (double) 32 * Numerator * m_cTicksPerMidiQuarterNote /
				(Denominator * cNotated32ndNotesPerMidiQuarterNote);

			// количество тактов, которое надо добавить в последовательность тактов
			DWORD cMeasures = (DWORD) ceil((ctToNoteOn + ctDuration -
				ctToEndOfLastMeasure) / ctMeasure);

			// добавляем такты в последовательность тактов
			for (DWORD i = 0; i < cMeasures; i++)
			{
				if (!pMidiSong->AddMeasure(Numerator, Denominator,
					cNotated32ndNotesPerMidiQuarterNote))
				{
					LOG("MidiSong::AddMeasure failed\n");
					return false;
				}
			}

			return true;
		}

		// обновляем текущее время (время в тиках, прошедшее с начала трека)
		ctCurTime += ctDeltaTime;

		// отбрасываем любые события, кроме метасобытий типа TIME_SIGNATURE
		if (Event != TIME_SIGNATURE) continue;

		// количество тиков в такте
		double ctMeasure = (double) 32 * Numerator * m_cTicksPerMidiQuarterNote /
			(Denominator * cNotated32ndNotesPerMidiQuarterNote);

		// количество тактов, которое надо добавить в последовательность тактов
		DWORD cMeasures = (DWORD) floor((ctCurTime - ctToEndOfLastMeasure) / ctMeasure);

		// добавляем такты в последовательность тактов
		for (DWORD i = 0; i < cMeasures; i++)
		{
			if (!pMidiSong->AddMeasure(Numerator, Denominator,
				cNotated32ndNotesPerMidiQuarterNote))
			{
				LOG("MidiSong::AddMeasure failed\n");
				return false;
			}
		}

		// проматываем pEventData до байта, содержащего числитель музыкального размера
		pEventData++;

		// сохраняем числитель музыкального размера
		Numerator = pEventData[0];

		// сохраняем знаменатель музыкального размера
		Denominator = (DWORD) pow(2.0, pEventData[1]);

		// сохраняем количество тридцатьвторых в четвертной MIDI-ноте
		cNotated32ndNotesPerMidiQuarterNote = pEventData[3];

		// обновляем количество тиков от начала трека до конца последнего добавленного
		// такта
		ctToEndOfLastMeasure += cMeasures * ctMeasure;
	}
}

/****************************************************************************************
*
*   Метод CreateTempoMap
*
*   Параметры
*       pMidiSong - указатель на объект класса MidiSong, содержащий ППС
*
*   Возвращаемое значение
*       true, если карта темпов успешно создана; false, если не удалось выделить память.
*
*   Создаёт карту темпов в объекте класса MidiSong.
*
****************************************************************************************/

bool MidiFile::CreateTempoMap(
	__inout MidiSong *pMidiSong)
{
	// устанавливаем текущую позицию на начало нулевого трека, в котором будем искать
    // метасобытия типа SET_TEMPO
	m_MidiTracks[0].ResetCurrentPosition();

	// дельта-время события в тиках
	DWORD ctDeltaTime;

	// указатель на первый байт данных события
	BYTE *pEventData;

	// текущее время в тиках, прошедшее с начала трека
	DWORD ctCurTime = 0;

	// цикл по событиям нулевого трека; ищем первое метасобытие SET_TEMPO
    while (true)
	{
		// считываем очередное событие из нулевого трека
		DWORD Event = m_MidiTracks[0].GetNextEvent(&ctDeltaTime, &pEventData);

		// если события кончились, то выходим
		if (Event == REAL_TRACK_END) return true;

		// обновляем текущее время (время в тиках, прошедшее с начала трека)
		ctCurTime += ctDeltaTime;

		// отбрасываем любые события, кроме метасобытий типа SET_TEMPO
		if (Event == SET_TEMPO) break;
	}

	// переменная, в которой будет храниться количество тиков от начала трека до момента
	// установки темпа, являющегося претендентом на добавление в карту темпов;
	// инициализируем количеством тиков от начала трека до момента первой установки темпа
	DWORD ctToLastTempoSet = ctCurTime;

	// проматываем pEventData до байта, содержащего старший байт количества микросекунд
	pEventData++;

	// переменная, в которой будет храниться длительность четвертной MIDI-ноты
	// в микросекундах для темпа, являющегося претендентом на добавление в карту темпов;
	// инициализируем количеством микросекунд на четвертную MIDI-ноту в первом темпе
	DWORD cLastMicroseconds = pEventData[0] << 16 | pEventData[1] << 8 | pEventData[2];

	// цикл по событиям нулевого трека
    while (true)
	{
		// считываем очередное событие из нулевого трека
		DWORD Event = m_MidiTracks[0].GetNextEvent(&ctDeltaTime, &pEventData);

		if (Event == REAL_TRACK_END)
		{
			// в треке больше не осталось событий

			// добавляем темп-претендент в карту темпов
			if (!pMidiSong->AddTempo(ctToLastTempoSet, cLastMicroseconds))
			{
				LOG("MidiSong::AddTempo failed\n");
				return false;
			}

			return true;
		}

		// обновляем текущее время (время в тиках, прошедшее с начала трека)
		ctCurTime += ctDeltaTime;

		// отбрасываем любые события, кроме метасобытий типа SET_TEMPO
		if (Event != SET_TEMPO) continue;

		// проматываем pEventData до байта, содержащего старший байт количества
		// микросекунд
		pEventData++;

		// считываем количество микросекунд на четвертную MIDI-ноту в только что
		// найденном темпе
		DWORD cMicroseconds = pEventData[0] << 16 | pEventData[1] << 8 | pEventData[2];

		if (ctToLastTempoSet == ctCurTime)
		{
			// время прихода темпа, являющегося претендентом на добавление в карту
			// темпов, и только что найденного темпа одинаково

			// претендентом на добавление становится только что найденный темп
			cLastMicroseconds = cMicroseconds;

			// ищем следующее метасобытие SET_TEMPO
			continue;
		}

		// если только что найденный темп совпадает с претендентом на добавление,
		// отбрасываем его
		if (cLastMicroseconds == cMicroseconds) continue;

		// добавляем темп-претендент в карту темпов
		if (!pMidiSong->AddTempo(ctToLastTempoSet, cLastMicroseconds))
		{
			LOG("MidiSong::AddTempo failed\n");
			return false;
		}

		// претендентом на добавление в карту темпов становится только что найденный темп
		ctToLastTempoSet = ctCurTime;
		cLastMicroseconds = cMicroseconds;
	}
}
