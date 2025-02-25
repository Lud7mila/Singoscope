/****************************************************************************************
*
*   Определение класса Song
*
*   Объект этого класса представляет собой песню. Песня описывается:
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

Song::Song()
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
*   Освобождает освобождает все выделенные ресурсы.
*
****************************************************************************************/

Song::~Song()
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

void Song::ResetCurrentPosition()
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
*       PauseLength - длительность паузы между предыдущей нотой и данной нотой в целых
*                     нотах
*       NoteLength - длительность ноты в целых нотах
*		pwsNoteText - указатель строку, не завершённую нулём, представляющую собой
*                     фрагмент слов песни, относящийся к ноте; этот параметр может быть
*                     равен NULL
*       cchNoteText - количество символов в строке pwsNoteText
*
*   Возвращаемое значение
*       true, если песенное событие успешно добавлено в ППС; false, если не удалось
*       выделить память.
*
*   Добавляет в ППС песенное событие, описываемое параметрами NoteNumber, PauseLength,
*   NoteLength, pwsNoteText и cchNoteText.
*
****************************************************************************************/

bool Song::AddSingingEvent(
	__in DWORD NoteNumber,
	__in double PauseLength,
	__in double NoteLength,
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
	pNewSingingEvent->PauseLength = PauseLength;
	pNewSingingEvent->NoteLength = NoteLength;
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
*       pPauseLength - указатель на переменную, в которую будет записана длительность
*                      паузы между предыдущей нотой и данной нотой в целых нотах; этот
*                      параметр может быть равен NULL
*       pNoteLength - указатель на переменную, в которую будет записана длительность ноты
*                     в целых нотах; этот параметр может быть равен NULL
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

bool Song::GetNextSingingEvent(
	__out_opt DWORD *pNoteNumber,
	__out_opt double *pPauseLength,
	__out_opt double *pNoteLength,
	__out_opt LPCWSTR *ppwsNoteText,
	__out_opt DWORD *pcchNoteText)
{
	// если песенные события кончились, возвращаем false
	if (m_pCurSingingEvent == NULL) return false;

	if (pNoteNumber != NULL) *pNoteNumber = m_pCurSingingEvent->NoteNumber;
	if (pPauseLength != NULL) *pPauseLength = m_pCurSingingEvent->PauseLength;
	if (pNoteLength != NULL) *pNoteLength = m_pCurSingingEvent->NoteLength;
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
*
*   Возвращаемое значение
*       true, если такт успешно добавлен; false, если не удалось выделить память.
*
*   Добавляет такт, описываемый параметрами Numerator и Denominator,
*   в последовательность тактов.
*
****************************************************************************************/

bool Song::AddMeasure(
	__in DWORD Numerator,
	__in DWORD Denominator)
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

bool Song::GetNextMeasure(
	__out DWORD *pNumerator,
	__out DWORD *pDenominator)
{
	// если такты кончились, возвращаем false
	if (m_pCurMeasure == NULL) return false;

	*pNumerator = m_pCurMeasure->Numerator;
	*pDenominator = m_pCurMeasure->Denominator;

	// переходим к следующему такту
	m_pCurMeasure = m_pCurMeasure->pNext;

	return true;
}

/****************************************************************************************
*
*   Метод AddTempo
*
*   Параметры
*       Offset - количество целых нот от начала трека до момента установки этого темпа
*       BPM - темп (количество четвертных нот в минуту)
*
*   Возвращаемое значение
*       true, если темп успешно добавлен; false, если не удалось выделить память.
*
*   Добавляет темп, описываемый параметрами Offset и BPM, в карту темпов.
*
****************************************************************************************/

bool Song::AddTempo(
	__in double Offset,
	__in double BPM)
{
	// выделяем память для нового элемента списка темпов
	TEMPO *pNewTempo = new TEMPO;

	if (pNewTempo == NULL)
	{
		LOG("operator new failed\n");
		return false;
	}

	// инициализируем новый элемент списка темпов
	pNewTempo->Offset = Offset;
	pNewTempo->BPM = BPM;
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
*       pOffset - указатель на переменную, в которую будет записано количество целых нот
*                 от начала трека до момента установки этого темпа
*       pBPM - указатель на переменную, в которую будет записан темп
*              (количество четвертных нот в минуту)
*
*   Возвращаемое значение
*       true, если информация об очередном темпе успешно возвращена;
*       false, если такты кончились.
*
*   Возвращает информацию об очередном темпе из карты темпов. После вызова метода
*   ResetCurrentPosition этот метод возвращает информацию о первом темпе из карты темпов.
*
****************************************************************************************/

bool Song::GetNextTempo(
	__out double *pOffset,
	__out double *pBPM)
{
	// если темпы кончились, возвращаем false
	if (m_pCurTempo == NULL) return false;

	*pOffset = m_pCurTempo->Offset;
	*pBPM = m_pCurTempo->BPM;

	// переходим к следующему темпу
	m_pCurTempo = m_pCurTempo->pNext;

	return true;
}

/****************************************************************************************
*
*   Метод Quantize
*
*   Параметры
*       StepDenominator - знаменатель дроби, числителем которой является единица,
*                         представляющей собой шаг сетки квантования
*
*   Возвращаемое значение
*       true, если в процессе квантования не образовалось ни одной ноты с нулевой
*       длительностью; иначе false.
*
*   Квантует ноты в ППС.
*
****************************************************************************************/

bool Song::Quantize(
	__in DWORD StepDenominator)
{
	// предполагаем, что не образуется ни одной ноты с нулевой длительностью
	bool bNoEmptyNotes = true;

	// шаг сетки квантования
	double Step = (double) 1 / StepDenominator;

	// указатель на текущий элемент списка песенных событий
	SINGING_EVENT *pCurSingingEvent = m_pFirstSingingEvent;

	// количество целых нот от начала песни до начала или конца текущей ноты
	double CurOffset = 0;

	// количество целых нот от начала песни до конца предыдущей отквантованной ноты
	double cwnToPrevQuantizedNoteOff = 0;

	// цикл по песенным событиям; квантуем ноты
	while (true)
	{
		// если все песенные события кончились, то выходим из цикла
		if (pCurSingingEvent == NULL) break;

		// количество целых нот от начала песни до начала текущей ноты
		CurOffset += pCurSingingEvent->PauseLength;

		// количество целых нот от начала песни до начала отквантованной текущей ноты
		double cwnToQuantizedNoteOn = Step * (int) (CurOffset / Step);

		// количество целых нот от начала песни до конца текущей ноты
		CurOffset += pCurSingingEvent->NoteLength;

		// количество целых нот от начала песни до конца отквантованной текущей ноты
		double cwnToQuantizedNoteOff = Step * (int) (CurOffset / Step);

		// сохраняем новые значения
		pCurSingingEvent->PauseLength = cwnToQuantizedNoteOn - cwnToPrevQuantizedNoteOff;
		pCurSingingEvent->NoteLength = cwnToQuantizedNoteOff - cwnToQuantizedNoteOn;

		// если образовалась нота с нулевой длительностью, запоминаем это
		if (pCurSingingEvent->NoteLength == 0) bNoEmptyNotes = false;

		// текущая нота становится предыдущей
		cwnToPrevQuantizedNoteOff = cwnToQuantizedNoteOff;

		// переходим к следующему песенному событию
		pCurSingingEvent = pCurSingingEvent->pNext;
	}

	return bNoEmptyNotes;
}

/****************************************************************************************
*
*   Метод ExtendEmptyNotes
*
*   Параметры
*       LengthDenominator - знаменатель дроби, числителем которой является единица,
*                           представляющей собой новую длительность нот с нулевой
*                           длительностью
*
*   Возвращаемое значение
*       true, если все ноты с нулевой длительностью были успешно растянуты; иначе false.
*
*   Растягивает ноты с нулевой длительностью в ППС следующим образом: если пауза перед
*   нотой, следующей за нотой с нулевой длительностью, больше или равна
*   1/LengthDenominator, то эта пауза уменьшается на 1/LengthDenominator, а длительность
*   ноты с нулевой длительностью делается равной 1/LengthDenominator.
*
****************************************************************************************/

bool Song::ExtendEmptyNotes(
	__in DWORD LengthDenominator)
{
	// предполагаем, что не останется ни одной ноты с нулевой длительностью
	bool bNoEmptyNotes = true;

	// новая длительность нот с нулевой длительностью
	double Length = (double) 1 / LengthDenominator;

	// указатель на текущий элемент списка песенных событий
	SINGING_EVENT *pCurSingingEvent = m_pFirstSingingEvent;

	// цикл по песенным событиям; растягиваем ноты с нулевой длительностью
	while (true)
	{
		// если все песенные события кончились, то выходим из цикла
		if (pCurSingingEvent == NULL) break;

		// указатель на следующий элемент списка песенных событий
		SINGING_EVENT *pNextSingingEvent = pCurSingingEvent->pNext;

		if (pCurSingingEvent->NoteLength == 0)
		{
			// длительность текущей ноты - нулевая

			if (pNextSingingEvent == NULL)
			{
				// текущая нота - последняя в ППС, её всегда можно растянуть
				pCurSingingEvent->NoteLength = Length;
			}
			else
			{
				// текущая нота - не последняя в ППС

				if (pNextSingingEvent->PauseLength < Length)
				{
					// текущую ноту растянуть невозможно
					bNoEmptyNotes = false;
				}
				else
				{
					// пауза перед следующей нотой больше или равна Length,
					// уменьшаем эту паузу и растягиваем текущую ноту
					pNextSingingEvent->PauseLength -= Length;
					pCurSingingEvent->NoteLength = Length;
				}
			}
		}

		// переходим к следующему песенному событию
		pCurSingingEvent = pNextSingingEvent;
	}

	return bNoEmptyNotes;
}

/****************************************************************************************
*
*   Метод HyphenateLyric
*
*   Параметры
*       Нет
*
*   Возвращаемое значение
*       true, если дефисы успешно поставлены в конце слогов слов песни; false, если не
*       удалось выделить память.
*
*   Ставит дефисы в конце слогов слов песни в ППС, обработанной методом
*   RemoveNonPrintableSymbols.
*
*   Обработка текста, осуществляемая этим методом, называется шестым этапом обработки
*   слов песни.
*
****************************************************************************************/

bool Song::HyphenateLyric()
{
	// указатель на текущий элемент списка песенных событий
	SINGING_EVENT *pCurSingingEvent = m_pFirstSingingEvent;

	// указатель на элемент списка песенных событий, на котором сделана закладка;
	// если этот указатель равен NULL, то закладка не сделана
	SINGING_EVENT *pBookmarkedSingingEvent = NULL;

	// буфер, в котором будет храниться текст песенного события, на котором сделана
	// закладка; учитываем возможный дополнительный дефис в конце текста
	WCHAR pwsBookmarkedText[MAX_SYMBOLS_PER_NOTE + 1];

	// количество символов в буфере pwsBookmarkedText
	DWORD cchBookmarkedText;

	// указатель на текст текущего песенного события
	LPCWSTR pwsNoteText;

	// количество символов в тексте текущего песенного события
	DWORD cchNoteText;

	// цикл по песенным событиям; ищем первое песенное событие с текстом
	while (true)
	{
		// если все песенные события кончились, то выходим
		if (pCurSingingEvent == NULL) return true;

		pwsNoteText = pCurSingingEvent->pwsNoteText;
		cchNoteText = pCurSingingEvent->cchNoteText;

		// если нашли песенное событие с текстом, то выходим из цикла
		if (pwsNoteText != NULL) break;

		// переходим к следующему песенному событию
		pCurSingingEvent = pCurSingingEvent->pNext;
	}

	// делаем закладку на первом песенном событии с текстом
	pBookmarkedSingingEvent = pCurSingingEvent;

	// сохраняем текст песенного события, на котором сделана закладка
	wcsncpy(pwsBookmarkedText, pwsNoteText, cchNoteText);
	cchBookmarkedText = cchNoteText;

	// цикл по песенным событиям; ставим дефисы в конце слогов
	while (true)
	{
		// переходим к следующему песенному событию
		pCurSingingEvent = pCurSingingEvent->pNext;

		// если все песенные события кончились, то выходим
		if (pCurSingingEvent == NULL) return true;

		pwsNoteText = pCurSingingEvent->pwsNoteText;
		cchNoteText = pCurSingingEvent->cchNoteText;

		// пропускаем песенные события без текста
		if (pwsNoteText == NULL) continue;

		if (IsCharAlphaW(pwsBookmarkedText[cchBookmarkedText-1]) &&
			IsCharAlphaW(pwsNoteText[0]))
		{
			// последний символ текста песенного события, на котором сделана
			// закладка, - буква, и первый символ текста последующего песенного события
			// с текстом - буква, значит надо поставить дефис
			pwsBookmarkedText[cchBookmarkedText++] = L'-';

			// устанавливаем текст у события, на котором сделана закладка
			if (!SetText(pBookmarkedSingingEvent, pwsBookmarkedText, cchBookmarkedText))
			{
				LOG("Song::SetText failed\n");
				return false;
			}
		}

		// делаем закладку на текущем песенном событии с текстом
		pBookmarkedSingingEvent = pCurSingingEvent;

		// сохраняем текст песенного события, на котором сделана закладка
		wcsncpy(pwsBookmarkedText, pwsNoteText, cchNoteText);
		cchBookmarkedText = cchNoteText;
	}
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

void Song::Free()
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

bool Song::SetText(
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
*   Функция IsStartOfGeneralizedSyllable
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
