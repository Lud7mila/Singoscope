/****************************************************************************************
*
*   Определение класса MidiSong
*
*   Объект этого класса представляет собой MIDI-песню. Песня описывается:
*   1) последовательностью песенных событий - ППС (Singing Event Sequence - SES);
*   2) последовательностью тактов (Measure Sequence);
*   3) картой темпов.
*
*   Автор: Людмила Огородникова и Александр Огородников, 2010
*
****************************************************************************************/

#define _CRT_SECURE_NO_WARNINGS

#include <windows.h>

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
*   Прототипы функций, определённых ниже
*
****************************************************************************************/

static bool IsStartOfGeneralizedSyllable(
	__in LPCWSTR pwsText,
	__in DWORD cchText);

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

MidiSong::MidiSong()
{
	m_pFirstSingingEvent = NULL;
	m_pLastSingingEvent = NULL;
	m_pCurSingingEvent = NULL;

	m_pFirstMeasure = NULL;
	m_pLastMeasure = NULL;
	m_pCurMeasure = NULL;

	m_pFirstTempo = NULL;
	m_pLastTempo = NULL;
	m_pCurTempo = NULL;
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

MidiSong::~MidiSong()
{
	Free();
}

/****************************************************************************************
*
*   Метод ResetCurrentPosition
*
*   Параметры
*       Нет
*
*   Возвращаемое значение
*       Нет
*
*   Устанавливает на начало песни текущие позиции последовательности песенных событий,
*   последовательности тактов и карты темпов.
*
****************************************************************************************/

void MidiSong::ResetCurrentPosition()
{
	m_pCurSingingEvent = m_pFirstSingingEvent;
	m_pCurMeasure = m_pFirstMeasure;
	m_pCurTempo = m_pFirstTempo;
}

/****************************************************************************************
*
*   Метод AddSingingEvent
*
*   Параметры
*       NoteNumber - номер ноты
*       ctToNoteOn - количество тиков от начала трека до начала ноты
*       ctDuration - длительность ноты в тиках
*		pwsNoteText - указатель строку, не завершённую нулём, представляющую собой
*                     фрагмент слов песни, относящийся к ноте; этот параметр может быть
*                     равен NULL
*       cchNoteText - количество символов в строке pwsNoteText
*
*   Возвращаемое значение
*       true, если песенное событие успешно добавлено в ППС; false, если не удалось
*       выделить память.
*
*   Добавляет в ППС песенное событие, описываемое параметрами NoteNumber, ctToNoteOn,
*   ctDuration, pwsNoteText и cchNoteText.
*
****************************************************************************************/

bool MidiSong::AddSingingEvent(
	__in DWORD NoteNumber,
	__in DWORD ctToNoteOn,
	__in DWORD ctDuration,
	__in_opt LPCWSTR pwsNoteText,
	__in DWORD cchNoteText)
{
	// выделяем память для нового элемента списка песенных событий
	SINGING_EVENT *pNewSingingEvent = new SINGING_EVENT;

	if (pNewSingingEvent == NULL)
	{
		LOG("operator new failed\n");
		return false;
	}

	// указатель на буфер, в который будет скопирован текст нового песенного события
	LPWSTR pwsNewNoteText = NULL;

	if (pwsNoteText != NULL && cchNoteText > 0)
	{
		// выделяем память для текста нового песенного события
		pwsNewNoteText = new WCHAR[cchNoteText];

		if (pwsNewNoteText == NULL)
		{
			LOG("operator new failed\n");
			delete pNewSingingEvent;
			return false;
		}

		// копируем текст нового песенного события в только что выделенный буфер
		wcsncpy(pwsNewNoteText, pwsNoteText, cchNoteText);
	}

	// инициализируем новый элемент списка песенных событий
	pNewSingingEvent->NoteNumber = NoteNumber;
	pNewSingingEvent->ctToNoteOn = ctToNoteOn;
	pNewSingingEvent->ctDuration = ctDuration;
	pNewSingingEvent->pwsNoteText = pwsNewNoteText;
	pNewSingingEvent->cchNoteText = cchNoteText;
	pNewSingingEvent->pNext = NULL;

	if (m_pFirstSingingEvent == NULL)
	{
		// в списке песенных событий ещё нет ни одного элемента
		m_pFirstSingingEvent = pNewSingingEvent;
	}
	else
	{
		// в списке песенных событий есть как минимум один элемент
		m_pLastSingingEvent->pNext = pNewSingingEvent;
	}

	// обновляем указатель на последний элемент списка
	m_pLastSingingEvent = pNewSingingEvent;

	return true;
}

/****************************************************************************************
*
*   Метод GetNextSingingEvent
*
*   Параметры
*       pNoteNumber - указатель на переменную, в которую будет записан номер ноты; этот
*                     параметр может быть равен NULL
*       pctToNoteOn - указатель на переменную, в которую будет записано количество тиков
*                     от начала трека до начала ноты; этот параметр может быть равен NULL
*       pctDuration - указатель на переменную, в которую будет записана длительность ноты
*                     в тиках; этот параметр может быть равен NULL
*       ppwsNoteText - указатель на переменную, в которую будет записан указатель строку,
*                      не завершённую нулём, представляющую собой фрагмент слов песни,
*                      относящийся к ноте; этот параметр может быть равен NULL
*       pcchNoteText - указатель на переменную, в которую будет записано количество
*                      символов во фрагменте слов песни, относящимся к ноте; этот
*                      параметр может быть равен NULL
*
*   Возвращаемое значение
*       true, если информация об очередном песенном событии успешно возвращена;
*       false, если песенные события кончились.
*
*   Возвращает информацию об очередном песенном событии из ППС. После вызова метода
*   ResetCurrentPosition этот метод возвращает информацию о первом песенном событии из
*   ППС.
*
****************************************************************************************/

bool MidiSong::GetNextSingingEvent(
	__out_opt DWORD *pNoteNumber,
	__out_opt DWORD *pctToNoteOn,
	__out_opt DWORD *pctDuration,
	__out_opt LPCWSTR *ppwsNoteText,
	__out_opt DWORD *pcchNoteText)
{
	// если песенные события кончились, возвращаем false
	if (m_pCurSingingEvent == NULL) return false;

	if (pNoteNumber != NULL) *pNoteNumber = m_pCurSingingEvent->NoteNumber;
	if (pctToNoteOn != NULL) *pctToNoteOn = m_pCurSingingEvent->ctToNoteOn;
	if (pctDuration != NULL) *pctDuration = m_pCurSingingEvent->ctDuration;
	if (ppwsNoteText != NULL) *ppwsNoteText = m_pCurSingingEvent->pwsNoteText;
	if (pcchNoteText != NULL) *pcchNoteText = m_pCurSingingEvent->cchNoteText;

	// переходим к следующему песенному событию
	m_pCurSingingEvent = m_pCurSingingEvent->pNext;

	return true;
}

/****************************************************************************************
*
*   Метод AddMeasure
*
*   Параметры
*       Numerator - числитель музыкального размера
*       Denominator - знаменатель музыкального размера
*       cNotated32ndNotesPerMidiQuarterNote - количество тридцатьвторых нот в четвертной
*                                             MIDI-ноте
*
*   Возвращаемое значение
*       true, если такт успешно добавлен; false, если не удалось выделить память.
*
*   Добавляет такт, описываемый параметрами Numerator, Denominator и
*   cNotated32ndNotesPerMidiQuarterNote, в последовательность тактов.
*
****************************************************************************************/

bool MidiSong::AddMeasure(
	__in DWORD Numerator,
	__in DWORD Denominator,
	__in DWORD cNotated32ndNotesPerMidiQuarterNote)
{
	// выделяем память для нового элемента списка тактов
	MEASURE *pNewMeasure = new MEASURE;

	if (pNewMeasure == NULL)
	{
		LOG("operator new failed\n");
		return false;
	}

	// инициализируем новый элемент списка тактов
	pNewMeasure->Numerator = Numerator;
	pNewMeasure->Denominator = Denominator;
	pNewMeasure->cNotated32ndNotesPerMidiQuarterNote = cNotated32ndNotesPerMidiQuarterNote;
	pNewMeasure->pNext = NULL;

	if (m_pFirstMeasure == NULL)
	{
		// в списке тактов ещё нет ни одного элемента
		m_pFirstMeasure = pNewMeasure;
	}
	else
	{
		// в списке тактов есть как минимум один элемент
		m_pLastMeasure->pNext = pNewMeasure;
	}

	// обновляем указатель на последний элемент списка
	m_pLastMeasure = pNewMeasure;

	return true;
}

/****************************************************************************************
*
*   Метод GetNextMeasure
*
*   Параметры
*       pNumerator - указатель на переменную, в которую будет записан числитель
*                    музыкального размера
*       pDenominator - указатель на переменную, в которую будет записан знаменатель
*                      музыкального размера
*       pcNotated32ndNotesPerMidiQuarterNote - указатель на переменную, в которую будет
*                                              записано количество тридцатьвторых нот
*                                              в четвертной MIDI-ноте
*
*   Возвращаемое значение
*       true, если информация об очередном такте успешно возвращена;
*       false, если такты кончились.
*
*   Возвращает информацию об очередном такте из последовательности тактов. После вызова
*   метода ResetCurrentPosition этот метод возвращает информацию о первом такте из
*   последовательности тактов.
*
****************************************************************************************/

bool MidiSong::GetNextMeasure(
	__out DWORD *pNumerator,
	__out DWORD *pDenominator,
	__out DWORD *pcNotated32ndNotesPerMidiQuarterNote)
{
	// если такты кончились, возвращаем false
	if (m_pCurMeasure == NULL) return false;

	*pNumerator = m_pCurMeasure->Numerator;
	*pDenominator = m_pCurMeasure->Denominator;
	*pcNotated32ndNotesPerMidiQuarterNote =
		m_pCurMeasure->cNotated32ndNotesPerMidiQuarterNote;

	// переходим к следующему такту
	m_pCurMeasure = m_pCurMeasure->pNext;

	return true;
}

/****************************************************************************************
*
*   Метод AddTempo
*
*   Параметры
*       ctToTempoSet - количество тиков от начала трека до момента установки темпа
*       cMicrosecondsPerMidiQuarterNote - длительность четвертной MIDI-ноты
*                                         в микросекундах
*
*   Возвращаемое значение
*       true, если темп успешно добавлен; false, если не удалось выделить память.
*
*   Добавляет темп, описываемый параметрами ctToTempoSet и
*   cMicrosecondsPerMidiQuarterNote, в карту темпов.
*
****************************************************************************************/

bool MidiSong::AddTempo(
	__in DWORD ctToTempoSet,
	__in DWORD cMicrosecondsPerMidiQuarterNote)
{
	// выделяем память для нового элемента списка темпов
	TEMPO *pNewTempo = new TEMPO;

	if (pNewTempo == NULL)
	{
		LOG("operator new failed\n");
		return false;
	}

	// инициализируем новый элемент списка темпов
	pNewTempo->ctToTempoSet = ctToTempoSet;
	pNewTempo->cMicrosecondsPerMidiQuarterNote = cMicrosecondsPerMidiQuarterNote;
	pNewTempo->pNext = NULL;

	if (m_pFirstTempo == NULL)
	{
		// в списке темпов ещё нет ни одного элемента
		m_pFirstTempo = pNewTempo;
	}
	else
	{
		// в списке темпов есть как минимум один элемент
		m_pLastTempo->pNext = pNewTempo;
	}

	// обновляем указатель на последний элемент списка
	m_pLastTempo = pNewTempo;

	return true;
}

/****************************************************************************************
*
*   Метод GetNextTempo
*
*   Параметры
*       pctToTempoSet - указатель на переменную, в которую будет записано количество
*                       тиков от начала трека до момента установки темпа
*       pcMicrosecondsPerMidiQuarterNote - указатель на переменную, в которую будет
*                                          записана длительность четвертной MIDI-ноты
*                                          в микросекундах
*
*   Возвращаемое значение
*       true, если информация об очередном темпе успешно возвращена;
*       false, если такты кончились.
*
*   Возвращает информацию об очередном темпе из карты темпов. После вызова метода
*   ResetCurrentPosition этот метод возвращает информацию о первом темпе из карты темпов.
*
****************************************************************************************/

bool MidiSong::GetNextTempo(
	__out DWORD *pctToTempoSet,
	__out DWORD *pcMicrosecondsPerMidiQuarterNote)
{
	// если темпы кончились, возвращаем false
	if (m_pCurTempo == NULL) return false;

	*pctToTempoSet = m_pCurTempo->ctToTempoSet;
	*pcMicrosecondsPerMidiQuarterNote = m_pCurTempo->cMicrosecondsPerMidiQuarterNote;

	// переходим к следующему темпу
	m_pCurTempo = m_pCurTempo->pNext;

	return true;
}

/****************************************************************************************
*
*   Метод CorrectLyric
*
*   Параметры
*       Нет
*
*   Возвращаемое значение
*       true, если текст слов песни успешно скорректирован; false, если не удалось
*       выделить память.
*
*   Корректирует текст слов песни в ППС. Коррекция заключается в выравнивании слов и
*   удалении непечатных символов.
*
****************************************************************************************/

bool MidiSong::CorrectLyric()
{
	// выравниваем слова в ППС
	if (!AlignLyric())
	{
		LOG("MidiSong::AlignLyric failed\n");
		return false;
	}

	// удаляем непечатные символы из слов песни в ППС
	if (!RemoveNonPrintableSymbols())
	{
		LOG("MidiSong::RemoveNonPrintableSymbols failed\n");
		return false;
	}

	return true;
}

/****************************************************************************************
*
*   Метод CreateSong
*
*   Параметры
*       cTicksPerMidiQuarterNote - количество тиков, приходящихся на четвертную MIDI ноту
*
*   Возвращаемое значение
*       Указатель на объект класса Song, или NULL, если не удалось выделить память.
*
*   Cоздаёт объект класса Song, представляющий собой песню.
*
****************************************************************************************/

Song *MidiSong::CreateSong(
	__in DWORD cTicksPerMidiQuarterNote)
{
	// создаём объект класса Song
	Song *pSong = new Song;

	if (pSong == NULL)
	{
		LOG("operator new failed\n");
		return NULL;
	}

	// создаём ППС в объекте класса Song
	if (!CreateSes(pSong, cTicksPerMidiQuarterNote))
	{
		LOG("MidiSong::CreateSes failed\n");
		delete pSong;
		return NULL;
	}

	// создаём последовательность тактов в объекте класса Song
	if (!CreateMeasureSequence(pSong))
	{
		LOG("MidiSong::CreateMeasureSequence failed\n");
		delete pSong;
		return NULL;
	}

	// создаём карту темпов в объекте класса Song
	if (!CreateTempoMap(pSong, cTicksPerMidiQuarterNote))
	{
		LOG("MidiSong::CreateTempoMap failed\n");
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

void MidiSong::Free()
{
	// удаляем список песенных событий

	SINGING_EVENT *pCurSingingEvent = m_pFirstSingingEvent;

	while (pCurSingingEvent != NULL)
	{
		SINGING_EVENT *pDelSingingEvent = pCurSingingEvent;
		pCurSingingEvent = pCurSingingEvent->pNext;

		if (pDelSingingEvent->pwsNoteText != NULL) delete pDelSingingEvent->pwsNoteText;

		delete pDelSingingEvent;
	}

	m_pFirstSingingEvent = NULL;
	m_pLastSingingEvent = NULL;
	m_pCurSingingEvent = NULL;

	// удаляем список тактов

	MEASURE *pCurMeasure = m_pFirstMeasure;

	while (pCurMeasure != NULL)
	{
		MEASURE *pDelMeasure = pCurMeasure;
		pCurMeasure = pCurMeasure->pNext;
		delete pDelMeasure;
	}

	m_pFirstMeasure = NULL;
	m_pLastMeasure = NULL;
	m_pCurMeasure = NULL;

	// удаляем список темпов

	TEMPO *pCurTempo = m_pFirstTempo;

	while (pCurTempo != NULL)
	{
		TEMPO *pDelTempo = pCurTempo;
		pCurTempo = pCurTempo->pNext;
		delete pDelTempo;
	}

	m_pFirstTempo = NULL;
	m_pLastTempo = NULL;
	m_pCurTempo = NULL;
}

/****************************************************************************************
*
*   Метод AlignLyric
*
*   Параметры
*       Нет
*
*   Возвращаемое значение
*       true, если слова песни успешно выравнены; false, если не удалось выделить память.
*
*   Выравнивает слова песни в ППС. В процессе выравнивания метод переносит большинство
*   небуквенных символов из начала каждого песенного события с текстом в конец
*   предыдущего песенного события с текстом.
*
*   Обработка текста, осуществляемая этим методом, называется четвёртым этапом обработки
*   слов песни.
*
****************************************************************************************/

bool MidiSong::AlignLyric()
{
	// указатель на текущий элемент списка песенных событий
	SINGING_EVENT *pCurSingingEvent = m_pFirstSingingEvent;

	// указатель на элемент списка песенных событий, на котором сделана закладка;
	// если этот указатель равен NULL, то закладка не сделана;
	// если этот указатель не равен NULL, то небуквенные символы должны добавляться
	// в конец буфера pwsAlignedText
	SINGING_EVENT *pBookmarkedSingingEvent = NULL;

	// буфер, в котором будет формироваться текст песенного события,
	// на котором сделана закладка
	WCHAR pwsAlignedText[MAX_SYMBOLS_PER_NOTE];

	// переменная, в которой будет храниться текущее количество символов
	// в буфере AlignedText
	DWORD cchAlignedText;

	// цикл по песенным событиям;
	// выравниваем текст у каждого события
	while (true)
	{
		if (pCurSingingEvent == NULL)
		{
			// все песенные события кончились

			if (pBookmarkedSingingEvent != NULL)
			{
				// устанавливаем текст у события, на котором сделана закладка
				if (!SetText(pBookmarkedSingingEvent, pwsAlignedText, cchAlignedText))
				{
					LOG("MidiSong::SetText failed\n");
					return false;
				}
			}

			return true;
		}

		// указатель на фрагмент слов песни, относящийся к ноте
		LPCWSTR pwsNoteText = pCurSingingEvent->pwsNoteText;

		// количество символов во фрагменте слов песни
		DWORD cchNoteText = pCurSingingEvent->cchNoteText;

		if (pwsNoteText == NULL)
		{
			// пропускаем песенные события без текста
			pCurSingingEvent = pCurSingingEvent->pNext;
			continue;
		}

		// индекс символа в строке pwsNoteText
		DWORD i = 0;

		// цикл по символам текста текущего песенного события
		while (true)
		{
			if (IsStartOfGeneralizedSyllable(pwsNoteText + i, cchNoteText - i))
			{
				// встретили начало нового обобщённого слога

				if (pBookmarkedSingingEvent != NULL)
				{
					// есть песенное событие, на котором была сделана закладка

					// устанавливаем текст у события, на котором сделана закладка
					if (!SetText(pBookmarkedSingingEvent, pwsAlignedText, cchAlignedText))
					{
						LOG("MidiSong::SetText failed\n");
						return false;
					}
				}

				// выходим из цикла
				break;
			}

			// если на каком-либо песенном событии сделана закладка, то добавляем
			// очередной небуквенный символ в конец буфера pwsAlignedText
			if (pBookmarkedSingingEvent != NULL)
			{
				pwsAlignedText[cchAlignedText] = pwsNoteText[i];
				cchAlignedText++;
			}

			// переходим к следующему символу текста текущего песенного события
			i++;

			if (cchAlignedText == MAX_SYMBOLS_PER_NOTE)
			{
				// дошли до конца буфера

				if (pBookmarkedSingingEvent != NULL)
				{
					// устанавливаем текст у события, на котором сделана закладка
					if (!SetText(pBookmarkedSingingEvent, pwsAlignedText, cchAlignedText))
					{
						LOG("MidiSong::SetText failed\n");
						return false;
					}
				}

				// надо отбросить оставшиеся небуквенные символы в начале текста
				// текущего песенного события, пока не дойдём до начала нового слога
				while (i < cchNoteText)
				{
					if (IsStartOfGeneralizedSyllable(pwsNoteText + i, cchNoteText - i))
					{
						break;
					}
				}

				// выходим из цикла
				break;
			}

			// если дошли до конца текста текущего песенного события, то выходим из цикла
			if (i == cchNoteText) break;
		}

		if (i == cchNoteText)
		{
			// в тексте текущего песенного события нет ни одной буквы, удаляем этот текст
			SetText(pCurSingingEvent, NULL, 0);

			if (cchAlignedText == MAX_SYMBOLS_PER_NOTE)
			{
				// мы вышли из предыдущего цикла по переполнению буфера; это означает,
				// что текст песенного события, на котором стояла закладка, установлен;
				// а поскольку в текущем песенном событии не осталось символов,
				// закладку на нём сделать нельзя
				pBookmarkedSingingEvent = NULL;
			}
		}
		else
		{
			// копируем текст текущего песенного события в буфер pwsAlignedText,
			// начиная с первой буквы
			cchAlignedText = cchNoteText - i;
			wcsncpy(pwsAlignedText, pwsNoteText + i, cchAlignedText);

			// делаем закладку на текущем песенном событии
			pBookmarkedSingingEvent = pCurSingingEvent;
		}

		// переходим к следующему песенному событию
		pCurSingingEvent = pCurSingingEvent->pNext;
	}
}

/****************************************************************************************
*
*   Метод RemoveNonPrintableSymbols
*
*   Параметры
*       Нет
*
*   Возвращаемое значение
*       true, если непечатные символы успешно удалены из слов песни; false, если не
*       удалось выделить память.
*
*   Удаляет непечатные символы из слов песни в ППС, обработанной методом AlignLyric.
*
*   Обработка текста, осуществляемая этим методом, называется пятым этапом обработки
*   слов песни.
*
****************************************************************************************/

bool MidiSong::RemoveNonPrintableSymbols()
{
	// указатель на текущий элемент списка песенных событий
	SINGING_EVENT *pCurSingingEvent = m_pFirstSingingEvent;

	// цикл по песенным событиям;
	// обрабатываем текст каждого события
	while (true)
	{
		// если песенные события кончились, то выходим
		if (pCurSingingEvent == NULL) return true;

		// указатель на фрагмент слов песни, относящийся к ноте
		LPCWSTR pwsNoteText = pCurSingingEvent->pwsNoteText;

		// количество символов во фрагменте слов песни
		DWORD cchNoteText = pCurSingingEvent->cchNoteText;

		if (pwsNoteText == NULL)
		{
			// пропускаем песенные события без текста
			pCurSingingEvent = pCurSingingEvent->pNext;
			continue;
		}

		// буфер, в котором будет сформирован новый текст текущего песенного события
		WCHAR pwsNewNoteText[MAX_SYMBOLS_PER_NOTE];

		// количество символов в буфере pwsNewNoteText
		DWORD cchNewNoteText = cchNoteText;

		// копируем в буфер pwsNewNoteText текст текущего песенного события
		wcsncpy(pwsNewNoteText, pwsNoteText, cchNoteText);

		// заменяем символы '/', '\', 0x00-0x1F на пробелы
		for (DWORD j = 0; j < cchNewNoteText; j++)
		{
			if (pwsNewNoteText[j] == L'/' || pwsNewNoteText[j] == L'\\' ||
				pwsNewNoteText[j] < L' ')
			{
				pwsNewNoteText[j] = L' ';
			}
		}

		// текущий индекс символа, который нужно установить
		// (первый символ - всегда не пробел, поэтому сразу пропускаем его)
		DWORD i = 1;

		// заменяем последовательности пробелов одним пробелом
		// (первый символ - всегда не пробел, поэтому сразу пропускаем его)
		for (DWORD j = 1; j < cchNewNoteText; j++)
		{
			// не копируем пробел, если мы перед этим уже скопировали один пробел
			if (pwsNewNoteText[i-1] == L' ' && pwsNewNoteText[j] == L' ') continue;

			pwsNewNoteText[i++] = pwsNewNoteText[j];
		}

		// обновляем количество символов в буфере pwsNewNoteText
		cchNewNoteText = i;

		// устанавливаем сформированный текст у текущего песенного события
		if (!SetText(pCurSingingEvent, pwsNewNoteText, cchNewNoteText))
		{
			LOG("MidiSong::SetText failed\n");
			return false;
		}

		// переходим к следующему песенному событию
		pCurSingingEvent = pCurSingingEvent->pNext;
	}
}

/****************************************************************************************
*
*   Метод SetText
*
*   Параметры
*       pSingingEvent - указатель на элемент списка песенных событий
*		pwsNoteText - указатель строку, не завершённую нулём, представляющую собой
*                     фрагмент слов песни, относящийся к ноте; этот параметр может быть
*                     равен NULL
*       cchNoteText - количество символов в строке pwsNoteText
*
*   Возвращаемое значение
*       true, если новый текст песенного события успешно установлен; false, если не
*       удалось выделить память.
*
*   Устанавливает новый текст песенного события, заданного параметром pSingingEvent.
*
****************************************************************************************/

bool MidiSong::SetText(
	__inout SINGING_EVENT *pSingingEvent,
	__in_opt LPCWSTR pwsNoteText,
	__in DWORD cchNoteText)
{
	// указатель на буфер, в который будет скопирован новый текст песенного события
	LPWSTR pwsNewNoteText = NULL;

	if (pwsNoteText != NULL && cchNoteText > 0)
	{
		// выделяем память для нового текста песенного события
		pwsNewNoteText = new WCHAR[cchNoteText];

		if (pwsNewNoteText == NULL)
		{
			LOG("operator new failed\n");
			return false;
		}

		// копируем новый текст песенного события в только что выделенный буфер
		wcsncpy(pwsNewNoteText, pwsNoteText, cchNoteText);
	}

	// удаляем старый текст песенного события
	if (pSingingEvent->pwsNoteText != NULL) delete pSingingEvent->pwsNoteText;

	// обновляем поля элемента списка песенных событий
	pSingingEvent->pwsNoteText = pwsNewNoteText;
	pSingingEvent->cchNoteText = cchNoteText;

	return true;
}

/****************************************************************************************
*
*   Метод CreateSes
*
*   Параметры
*       pSong - указатель на объект класса Song, в котором надо создать ППС
*       cTicksPerMidiQuarterNote - количество тиков, приходящихся на четвертную MIDI ноту
*
*   Возвращаемое значение
*       true в случае успеха; false, если не удалось выделить память.
*
*   Cоздаёт ППС в объекте класса Song.
*
****************************************************************************************/

bool MidiSong::CreateSes(
	__inout Song *pSong,
	__in DWORD cTicksPerMidiQuarterNote)
{
	// указатель на текущий элемент списка песенных событий
	SINGING_EVENT *pCurSingingEvent = m_pFirstSingingEvent;

	// указатель на текущий элемент списка тактов
	MEASURE *pCurMeasure = m_pFirstMeasure;

	// переменная, являющаяся аккумулятором тиков;
	// количество тиков от начала песни до текущего такта
	double ctTotal = 0;

	// переменная, являющаяся аккумулятором целых нот;
    // количество целых нот от начала песни до текущего такта
	double cwnTotal = 0;

	// количество целых нот от начала песни до выключения ноты предыдущего песенного
    // события
	double cwnToPrevNoteOff = 0;

	// цикл по песенным событиям;
	// создаём ППС в объекте класса Song
	while (true)
	{
		// если все песенные события кончились, то создание ППС окончено
		if (pCurSingingEvent == NULL) return true;

		// длительность текущего такта в тиках
		double ctPerCurMeasure;

		// длительность текущего такта в целых нотах
		double cwnPerCurMeasure;

		// бежим по тактам, пока не найдём такт, на который приходится включение ноты
        // текущего песенного события
		while (true)
		{
			// вычисляем длительность текущего такта в тиках
			ctPerCurMeasure = (double) 32 * pCurMeasure->Numerator *
				cTicksPerMidiQuarterNote / (pCurMeasure->Denominator *
				pCurMeasure->cNotated32ndNotesPerMidiQuarterNote);

			// вычисляем длительность текущего такта в целых нотах
			cwnPerCurMeasure = (double) pCurMeasure->Numerator / pCurMeasure->Denominator;

			if (ctTotal + ctPerCurMeasure >= pCurSingingEvent->ctToNoteOn)
			{
				// такт, на который приходится включение ноты текущего песенного события,
                // найден, поэтому выходим из цикла
				break;
			}

			ctTotal += ctPerCurMeasure;
			cwnTotal += cwnPerCurMeasure;

			// переходим к следующему такту
			pCurMeasure = pCurMeasure->pNext;
		}

		// находим количество целых нот от начала песни до включения ноты текущего
		// песенного события
		double cwnToNoteOn = cwnTotal + (pCurSingingEvent->ctToNoteOn - ctTotal) *
			cwnPerCurMeasure / ctPerCurMeasure;

		// переменная, хранящая количество тиков от начала песни до выключения ноты
        // текущего песенного события
        DWORD ctToNoteOff = pCurSingingEvent->ctToNoteOn + pCurSingingEvent->ctDuration;

		// бежим по тактам, пока не найдём такт, на который приходится выключение ноты
        // текущего песенного события
		while (true)
		{
			// вычисляем длительность текущего такта в тиках
			ctPerCurMeasure = (double) 32 * pCurMeasure->Numerator *
				cTicksPerMidiQuarterNote / (pCurMeasure->Denominator *
				pCurMeasure->cNotated32ndNotesPerMidiQuarterNote);

			// вычисляем длительность текущего такта в целых нотах
			cwnPerCurMeasure = (double) pCurMeasure->Numerator / pCurMeasure->Denominator;

			if (ctTotal + ctPerCurMeasure >= ctToNoteOff)
			{
				// такт, на который приходится выключение ноты текущего песенного события,
                // найден, поэтому выходим из цикла
				break;
			}

			ctTotal += ctPerCurMeasure;
			cwnTotal += cwnPerCurMeasure;

			// переходим к следующему такту
			pCurMeasure = pCurMeasure->pNext;
		}

		// находим количество целых нот от начала песни до выключения ноты текущего
		// песенного события
		double cwnToNoteOff = cwnTotal + (ctToNoteOff - ctTotal) *
			cwnPerCurMeasure / ctPerCurMeasure;

		// находим длительность ноты текущего песенного события в целых нотах
		double cwnPerNote = cwnToNoteOff - cwnToNoteOn;

		// длительность паузы между предыдущей нотой и нотой текущего песенного события
		// в целых нотах
		double cwnPerPause = cwnToNoteOn - cwnToPrevNoteOff;

		// добавляем в ППС очередное песенное событие
		bool bEventAdded = pSong->AddSingingEvent(pCurSingingEvent->NoteNumber,
			cwnPerPause, cwnPerNote, pCurSingingEvent->pwsNoteText,
			pCurSingingEvent->cchNoteText);

		if (!bEventAdded)
		{
			LOG("Song::AddSingingEvent failed\n");
			return false;
		}

		cwnToPrevNoteOff = cwnToNoteOff;

		// переходим к следующему песенному событию
		pCurSingingEvent = pCurSingingEvent->pNext;
	}
}

/****************************************************************************************
*
*   Метод CreateMeasureSequence
*
*   Параметры
*       pSong - указатель на объект класса Song
*
*   Возвращаемое значение
*       true, если последовательность тактов успешно создана; false, если не удалось
*       выделить память.
*
*   Создаёт последовательность тактов в объекте класса Song.
*
****************************************************************************************/

bool MidiSong::CreateMeasureSequence(
	__inout Song *pSong)
{
	// указатель на текущий элемент списка тактов
	MEASURE *pCurMeasure = m_pFirstMeasure;

	// цикл по тактам;
	// создаём последовательность тактов в объекте класса Song
	while (true)
	{
		// если все такты кончились, то выходим
		if (pCurMeasure == NULL) return true;

		if (!pSong->AddMeasure(pCurMeasure->Numerator, pCurMeasure->Denominator))
		{
			LOG("Song::AddMeasure failed\n");
			return false;
		}

		// переходим к следующему такту
		pCurMeasure = pCurMeasure->pNext;
	}
}

/****************************************************************************************
*
*   Метод CreateTempoMap
*
*   Параметры
*       pSong - указатель на объект класса Song
*       cTicksPerMidiQuarterNote - количество тиков, приходящихся на четвертную MIDI ноту
*
*   Возвращаемое значение
*       true, если карта темпов успешно создана; false, если не удалось выделить память.
*
*   Создаёт карту темпов в объекте класса Song.
*
****************************************************************************************/

bool MidiSong::CreateTempoMap(
	__inout Song *pSong,
	__in DWORD cTicksPerMidiQuarterNote)
{
	// указатель на текущий элемент списка темпов
	TEMPO *pCurTempo = m_pFirstTempo;

	// указатель на текущий элемент списка тактов
	MEASURE *pCurMeasure = m_pFirstMeasure;

	// переменная, являющаяся аккумулятором тиков;
	// количество тиков от начала песни до текущего такта
	double ctTotal = 0;

	// переменная, являющаяся аккумулятором целых нот;
    // количество целых нот от начала песни до текущего такта
	double cwnTotal = 0;

	// цикл по темпам;
	// создаём карту темпов в объекте класса Song
	while (true)
	{
		// если все темпы кончились, то создание карты темпов окончено
		if (pCurTempo == NULL) return true;

		// длительность текущего такта в тиках
		double ctPerCurMeasure;

		// длительность текущего такта в целых нотах
		double cwnPerCurMeasure;

		// бежим по тактам, пока не найдём такт, в котором происходит установка текущего
		// темпа
		while (true)
		{
			// вычисляем длительность текущего такта в тиках
			ctPerCurMeasure = (double) 32 * pCurMeasure->Numerator *
				cTicksPerMidiQuarterNote / (pCurMeasure->Denominator *
				pCurMeasure->cNotated32ndNotesPerMidiQuarterNote);

			// вычисляем длительность текущего такта в целых нотах
			cwnPerCurMeasure = (double) pCurMeasure->Numerator / pCurMeasure->Denominator;

			if (ctTotal + ctPerCurMeasure >= pCurTempo->ctToTempoSet)
			{
				// такт, в котором происходит установка текущего темпа,
                // найден, поэтому выходим из цикла
				break;
			}

			ctTotal += ctPerCurMeasure;
			cwnTotal += cwnPerCurMeasure;

			// переходим к следующему такту
			pCurMeasure = pCurMeasure->pNext;

			// если такты кончились, то выходим
			if (pCurMeasure == NULL) return true;
		}

		// находим количество целых нот от начала песни до установки текущего темпа
		double cwnToTempoSet = cwnTotal + (pCurTempo->ctToTempoSet - ctTotal) *
			cwnPerCurMeasure / ctPerCurMeasure;

		double BPM = ((double) pCurMeasure->cNotated32ndNotesPerMidiQuarterNote / 8) *
			((double) 60000000 / pCurTempo->cMicrosecondsPerMidiQuarterNote);

		// добавляем в карту темпов очередной темп
		if (!pSong->AddTempo(cwnToTempoSet, BPM))
		{
			LOG("Song::AddTempo failed\n");
			return false;
		}

		// переходим к следующему темпу
		pCurTempo = pCurTempo->pNext;
	}
}

/****************************************************************************************
*
*   Функция IsStartOfSyllable
*
*   Параметры
*		pwsText - указатель строку, не завершённую нулём, представляющую собой фрагмент
*                 текста песенного события
*       cchText - количество символов в строке pwsText; значение этого параметра должно
*                 быть больше нуля
*
*   Возвращаемое значение
*       true, если текст начинается с нового обобщённого слога; иначе false.
*
*   Проверяет, начинается ли текст, заданный параметрами pwsText и cchText, с нового
*   обобщённого слога.
*
****************************************************************************************/

static bool IsStartOfGeneralizedSyllable(
	__in LPCWSTR pwsText,
	__in DWORD cchText)
{
	// если первый символ теста - буква, то это начало обобщённого слога
	if (IsCharAlphaW(pwsText[0])) return true;

	// если первый символ - открывающаяся круглая скобка, то это начало обобщённого слога
	if (pwsText[0] == L'(') return true;

	// если первый символ - апостроф, за которым следует буква,
	// то это начало обобщённого слога
	if (pwsText[0] == L'\'' && cchText > 1 && IsCharAlphaW(pwsText[1])) return true;

	return false;
}
