/****************************************************************************************
*
*   Определение класса SongFile
*
*	Объект этого класса представляет собой файл с песней.
*
*   Авторы: Людмила Огордникова и Александр Огородников, 2008-2010
*
****************************************************************************************/

#include <windows.h>
#include <tchar.h>

#include "Log.h"
#include "TextMessages.h"
#include "ShowError.h"
#include "Song.h"
#include "MidiLibrary.h"
#include "MidiTrack.h"
#include "MidiPart.h"
#include "MidiLyric.h"
#include "MidiSong.h"
#include "MidiFile.h"
#include "SongFile.h"

/****************************************************************************************
*
*   Прототипы функций, определённых ниже
*
****************************************************************************************/

static LPCTSTR GetFileNameExtension(
	__in LPCTSTR pszFileName);

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

SongFile::SongFile()
{
	m_pMidiFile = NULL;
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

SongFile::~SongFile()
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
*       true, если кодовая страница по умолчанию успешно установлена; иначе false.
*
*   Устанавливает кодовую страницу по умолчанию для слов песни.
*
****************************************************************************************/

bool SongFile::SetDefaultCodePage(
	__in UINT DefaultCodePage)
{
	if (m_pMidiFile == NULL) return false;

	MIDIFILERESULT Result = m_pMidiFile->SetDefaultCodePage(DefaultCodePage);

	// если метод SetDefaultCodePage успешно отработал, то выходим
	if (Result == MIDIFILE_SUCCESS) return true;

	switch (Result)
	{
		case MIDIFILE_NO_LYRIC:
			ShowError(MSGID_NO_LYRIC);
			break;

		case MIDIFILE_NO_VOCAL_PARTS:
			ShowError(MSGID_NO_VOCAL_PARTS);
			break;

		case MIDIFILE_CANT_ALLOC_MEMORY:
			ShowError(MSGID_CANT_ALLOC_MEMORY);
			break;
	}

	return false;
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
*       true, если критерий выбора ноты из созвучия успешно установлен; иначе false.
*
*   Устанавливает критерий выбора ноты из созвучия. Этот критерий используется, когда
*   вокальная партия содержит созвучия и необходимо выбрать только одну ноту.
*
****************************************************************************************/

bool SongFile::SetConcordNoteChoice(
	__in CONCORD_NOTE_CHOICE ConcordNoteChoice)
{
	if (m_pMidiFile == NULL) return false;

	MIDIFILERESULT Result = m_pMidiFile->SetConcordNoteChoice(ConcordNoteChoice);

	// если метод SetConcordNoteChoice успешно отработал, то выходим
	if (Result == MIDIFILE_SUCCESS) return true;

	switch (Result)
	{
		case MIDIFILE_NO_VOCAL_PARTS:
			ShowError(MSGID_NO_VOCAL_PARTS);
			break;

		case MIDIFILE_CANT_ALLOC_MEMORY:
			ShowError(MSGID_CANT_ALLOC_MEMORY);
			break;
	}

	return false;
}

/****************************************************************************************
*
*   Метод LoadFile
*
*   Параметры
*       pszFileName - указатель на строку, завершенную нулем, в которой указано имя
*					  файла с песней
*       DefaultCodePage - кодовая страница по умолчанию для слов песни
*       ConcordNoteChoice - определяет, какую из нот созвучия нужно выбирать:
*                           CHOOSE_MIN_NOTE_NUMBER - ноту с минимальным номером,
*                           CHOOSE_MAX_NOTE_NUMBER - ноту с максимальным номером,
*                           CHOOSE_MAX_NOTE_DURATION - ноту с максимальной длительностью
*
*   Возвращаемое значение
*       true, если файл с песней успешно загружен; или false, если произошла ошибка.
*
*   Загружает файл с песней.
*
****************************************************************************************/

bool SongFile::LoadFile(
	__in LPCTSTR pszFileName,
	__in UINT DefaultCodePage,
	__in CONCORD_NOTE_CHOICE ConcordNoteChoice)
{
	// если в этот объект уже был загружен какой-то файл, освобождаем все выделенные
	// для этого файла ресурсы
	Free();

	// открываем файл с песней
	HANDLE hFile = CreateFile(pszFileName, GENERIC_READ, FILE_SHARE_READ, NULL,
		OPEN_EXISTING, 0, NULL);

	if (hFile == INVALID_HANDLE_VALUE)
	{
		LOG("CreateFile failed (error %u)\n", GetLastError());
		ShowError(MSGID_CANT_OPEN_FILE, GetLastError());
		return false;
	}

	// получаем размер файла с песней в байтах
	DWORD cbFile = GetFileSize(hFile, NULL);

	if (cbFile == INVALID_FILE_SIZE)
	{
		LOG("GetFileSize failed (error %u)\n", GetLastError());
		ShowError(MSGID_CANT_LOAD_FILE, FUNCID_GET_FILE_SIZE, GetLastError());
		CloseHandle(hFile);
		return false;
	}

	// выделяем буфер, в который будет загружен файл с песней
	PBYTE pFile = (PBYTE) HeapAlloc(GetProcessHeap(), 0, cbFile);

	if (pFile == NULL)
	{
		LOG("HeapAlloc failed\n");
		ShowError(MSGID_CANT_ALLOC_MEMORY);
		CloseHandle(hFile);
		return false;
	}

	DWORD cbRead;

	// загружаем содержимое файла с песней в буфер
	if (!ReadFile(hFile, pFile, cbFile, &cbRead, NULL))
	{
		LOG("ReadFile failed (error %u)\n", GetLastError());
		ShowError(MSGID_CANT_LOAD_FILE, FUNCID_READ_FILE, GetLastError());
		HeapFree(GetProcessHeap(), 0, pFile);
		CloseHandle(hFile);
		return false;
	}

	// закрываем файл с песней
	CloseHandle(hFile);

	// создаём объект класса MidiFile
	m_pMidiFile = new MidiFile;

	if (m_pMidiFile == NULL)
	{
		LOG("operator new failed\n");
		ShowError(MSGID_CANT_ALLOC_MEMORY);
		HeapFree(GetProcessHeap(), 0, pFile);
		return false;
	}

	// устанавливаем кодовую страницу по умолчанию для слов песни
	m_pMidiFile->SetDefaultCodePage(DefaultCodePage);

	// устанавливаем критерий выбора ноты из созвучия
	m_pMidiFile->SetConcordNoteChoice(ConcordNoteChoice);

	// назначаем объекту загруженный в память файл с песней
	MIDIFILERESULT Result = m_pMidiFile->AssignFile(pFile, cbFile);

	// если метод AssignFile успешно отработал, то выходим
	if (Result == MIDIFILE_SUCCESS) return true;

	// если метод AssignFile завершился с ошибкой, то мы должны освободить буфер
	HeapFree(GetProcessHeap(), 0, pFile);

	switch (Result)
	{
		case MIDIFILE_INVALID_FORMAT:
		{
			LPCTSTR pszExtension = GetFileNameExtension(pszFileName);

			if (_tcsicmp(pszExtension, TEXT(".mid")) == 0 ||
				_tcsicmp(pszExtension, TEXT(".midi")) == 0 ||
				_tcsicmp(pszExtension, TEXT(".rmi")) == 0)
			{
				ShowError(MSGID_CORRUPTED_MIDI_FILE);
			}
			else if (_tcsicmp(pszExtension, TEXT(".kar")) == 0)
			{
				ShowError(MSGID_CORRUPTED_KAR_FILE);
			}
			else
			{
				ShowError(MSGID_UNSUPPORTED_FILE_FORMAT);
			}

			break;
		};

		case MIDIFILE_UNSUPPORTED_FORMAT:
			ShowError(MSGID_UNSUPPORTED_MIDI_FILE_FORMAT);
			break;

		case MIDIFILE_NO_LYRIC:
			ShowError(MSGID_NO_LYRIC);
			break;

		case MIDIFILE_NO_VOCAL_PARTS:
			ShowError(MSGID_NO_VOCAL_PARTS);
			break;

		case MIDIFILE_CANT_ALLOC_MEMORY:
			ShowError(MSGID_CANT_ALLOC_MEMORY);
			break;
	}

	return false;
}

/****************************************************************************************
*
*   Метод CreateSong
*
*   Параметры
*       QuantizeStepDenominator - знаменатель дроби, числителем которой является единица,
*                                 представляющей собой шаг сетки квантования
*
*   Возвращаемое значение
*       Указатель на созданный объект класса Song; или NULL, если не удалось выделить
*       память.
*
*   Cоздаёт объект класса Song, представляющий собой песню.
*
****************************************************************************************/

Song *SongFile::CreateSong(
	__in DWORD QuantizeStepDenominator)
{
	// знаменатель дроби, представляющей собой текущий шаг сетки квантования
	DWORD CurQuantizeStepDenominator = QuantizeStepDenominator;

	// указатель на объект класса Song
	Song *pSong = NULL;

	// цикл по размерам шага сетки квантования;
	// на каждой итерации уменьшаем шаг в два раза
	do
	{
		// удаляем объект класса Song, созданный на предыдущей итерации
		if (pSong != NULL) delete pSong;

		// создаём исходный объект класса Song
		pSong = m_pMidiFile->CreateSong();

		if (pSong == NULL)
		{
			LOG("MidiFile::CreateSong failed\n");
			ShowError(MSGID_CANT_ALLOC_MEMORY);
			return NULL;
		}

		// делаем квантование нот в ППС и выходим из цикла, если не образовалось
		// ни одной ноты с нулевой длительностью
		if (pSong->Quantize(CurQuantizeStepDenominator)) break;

		// в процессе квантования появились ноты с нулевой длительностью, растягиваем их;
		// если все ноты с нулевой длительностью удалось растянуть, то выходим из цикла
		if (pSong->ExtendEmptyNotes(CurQuantizeStepDenominator)) break;

		CurQuantizeStepDenominator *= 2;
	}
	while (CurQuantizeStepDenominator <= 256);

	// ставим дефисы в конце слогов слов песни в ППС
	if (!pSong->HyphenateLyric())
	{
		LOG("Song::HyphenateLyric failed\n");
		ShowError(MSGID_CANT_ALLOC_MEMORY);
		delete pSong;
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

void SongFile::Free()
{
	if (m_pMidiFile != NULL)
	{
		delete m_pMidiFile;
		m_pMidiFile = NULL;
	}
}

/****************************************************************************************
*
*   Функция GetFileNameExtension
*
*   Параметры
*       pszFileName - указатель на строку, завершенную нулем, в которой указано имя файла
*
*   Возвращаемое значение
*       Указатель на расширение имени файла или NULL, если его нет.
*
*   Возвращает указатель на расширение имени файла.
*
****************************************************************************************/

static LPCTSTR GetFileNameExtension(
	__in LPCTSTR pszFileName)
{
	size_t cchFileName = _tcslen(pszFileName);

	LPCTSTR pszSubstring = pszFileName + cchFileName - 1;

	while (pszSubstring > pszFileName)
	{
		if (*pszSubstring == TEXT('.')) return pszSubstring + 1;

		if (*pszSubstring == TEXT('\\')) return NULL;

		pszSubstring--;
	}

	return NULL;
}
