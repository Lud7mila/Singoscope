/****************************************************************************************
*
*   Определение класса MidiPart
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

#include <windows.h>

#include "Log.h"
#include "MidiLibrary.h"
#include "MidiTrack.h"
#include "MidiPart.h"

/****************************************************************************************
*
*   Константы
*
****************************************************************************************/

// значение, которым инициализируются элементы массива m_CurInstrument
#define UNDEFINED_INSTRUMENT	0xFFFFFFFF

// возможное значение переменных, хранящих время в тиках;
// используется внутри метода GetNextNote
#define UNDEFINED_TICK_COUNT	0xFFFFFFFF

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

MidiPart::MidiPart()
{
	m_pMidiTrack = NULL;
	m_pNoteList = NULL;
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
*   Освобождает выделенную во время поиска память. Не трогает объект класса MidiTrack,
*   заданный при вызове метода InitSearch.
*
****************************************************************************************/

MidiPart::~MidiPart()
{
	FreeNoteList();
}

/****************************************************************************************
*
*   Метод InitSearch
*
*   Параметры
*       pMidiTrack - указатель на объект класса MidiTrack, представляющий собой трек
*                    MIDI-файла, в котором находится партия
*       Channel - номер канала, в котором находится партия; если значение этого параметра
*                 равно ANY_CHANNEL, то к партии относятся ноты со всех каналов трека
*       Instrument - номер инструмента, которым играются ноты данной партии; если
*                    значение этого параметра равно ANY_INSTRUMENT, то к партии относятся
*                    ноты, играемые любым инструментом
*       OverlapsThreshold - максимально допустимая доля перекрытий нот по времени
*                           по отношению к количеству одиночных нот в партии. Этот порог
*                           используется методом GetNextNote: если он превышен, метод
*                           GetNextNote возвращает код ошибки
*                           MIDIPART_OVERLAPS_THRESHOLD_EXCEEDED
*       ConcordNoteChoice - определяет, какую из нот созвучия должен возвращать метод
*                           GetNextNote:
*                           CHOOSE_MIN_NOTE_NUMBER - ноту с минимальным номером,
*                           CHOOSE_MAX_NOTE_NUMBER - ноту с максимальным номером,
*                           CHOOSE_MAX_NOTE_DURATION - ноту с максимальной длительностью
*
*   Возвращаемое значение
*       Нет
*
*   Инициализирует поиск нот в партии, заданной параметрами pMidiTrack, Channel и
*   Instrument.
*
****************************************************************************************/

void MidiPart::InitSearch(
	__in MidiTrack *pMidiTrack,
	__in DWORD Channel,
	__in DWORD Instrument,
	__in double OverlapsThreshold,
	__in CONCORD_NOTE_CHOICE ConcordNoteChoice)
{
	// если этот объект уже использовался для поиска нот, освобождаем выделенную во
	// время прошлого поиска память
	FreeNoteList();

	m_pMidiTrack = pMidiTrack;

	m_pMidiTrack->ResetCurrentPosition();

	m_PartChannel = Channel;
	m_PartInstrument = Instrument;
	m_OverlapsThreshold = OverlapsThreshold;
	m_ConcordNoteChoice = ConcordNoteChoice;

	for (DWORD i = 0; i < MAX_MIDI_CHANNELS; i++)
	{
    	m_CurInstrument[i] = UNDEFINED_INSTRUMENT;
    }

	m_ctCurTime = 0;
	m_pNoteList = NULL;

	m_cSingleNotes = 0;
	m_cOverlaps = 0;
}

/****************************************************************************************
*
*   Метод GetNextNote
*
*   Параметры
*       pNoteDesc - указатель на структуру типа NOTEDESC, в которую будет записана
*                   информация об очередной одиночной ноте партии; этот параметр может
*                   быть равен NULL
*
*   Возвращаемое значение
*		MIDIPART_SUCCESS - очередная одиночная нота сконструирована и возвращена;
*		MIDIPART_PART_END - конец партии;
*		MIDIPART_CANT_ALLOC_MEMORY - не удалось выделить память;
*       MIDIPART_COMPLICATED_NOTE_COMBINATION - встретилась слишком сложная комбинация
*                                               нот, метод не может сконструировать
*                                               одиночную ноту;
*		MIDIPART_OVERLAPS_THRESHOLD_EXCEEDED - превышен порог максимально допустимой доли
*                                              перекрытий нот по времени по отношению к
*                                              количеству одиночных нот в партии
*
*   Конструирует и возвращает очередную одиночную ноту партии. После вызова метода
*   InitSearch этот метод возвращает первую ноту партии. Если метод возвращает любой код,
*   кроме MIDIPART_SUCCESS, то поля структуры, на которую указывает параметр pNoteDesc,
*   не определены.
*
****************************************************************************************/

MIDIPARTRESULT MidiPart::GetNextNote(
	__out_opt NOTEDESC *pNoteDesc)
{
	if (m_pNoteList != NULL && m_pNoteList->ctDuration != UNDEFINED_TICK_COUNT)
	{
		// в списке есть одна завершённая нота, которую нужно вернуть
		m_cSingleNotes++;
		ReturnFirstNote(pNoteDesc);
		return MIDIPART_SUCCESS;
	}

	// в этот момент в списке может находиться одна или несколько незавершённых нот

	// количество нот в списке
	DWORD cNotes = 0;

	// количество завершённых нот в списке; пустые ноты никогда не учитываются в этой
	// переменной
	DWORD cFinishedNotes = 0;

	// максимальное количество уровней (одновременно звучащих нот) до текущего момента,
	// исключая этот момент; в этой переменной никогда не учитываются пустые ноты
	DWORD cMaxLevels = 0;

	// текущее количество уровней (одновременно звучащих нот);
	// в этой переменной могут учитываться пустые ноты
	DWORD cCurLevels = 0;

	// момент времени, в который пришло предпоследнее событие трека
	DWORD ctLastTime = m_ctCurTime;

	// если в списке есть незавершённые ноты, корректируем переменные,
	// инициализированные выше
	NOTELISTITEM *pCurNote = m_pNoteList;
	while (pCurNote != NULL)
	{
		cNotes++;
		cCurLevels++;

		if (pCurNote->ctToNoteOn < m_ctCurTime) cMaxLevels++;

		pCurNote = pCurNote->pNext;
	}

	// цикл по всем событиям трека; выходим из цикла тогда,
	// когда можно сконструировать хотя бы одну одиночную ноту
	while (true)
	{
		// считываем очередное событие трека

		READEVENTRESULT ReadEventResult = ReadNextEvent();

		if (ReadEventResult == READEVENT_UNRELEVANT_EVENT) continue;

		if (ReadEventResult == READEVENT_CANT_ALLOC_MEMORY)
		{
			LOG("ReadNextEvent failed\n");
			return MIDIPART_CANT_ALLOC_MEMORY;
		}

		if (ReadEventResult == READEVENT_EMPTY_NOTE)
		{
			// ReadNextEvent удалил пустую ноту из списка
			cNotes--;
			cCurLevels--;
			continue;
		}

		if (ctLastTime < m_ctCurTime)
		{
			if (cMaxLevels < cCurLevels) cMaxLevels = cCurLevels;
			ctLastTime = m_ctCurTime;
		}

		if (ReadEventResult == READEVENT_NOTE_ON)
		{
			// ReadNextEvent добавил новую ноту в список
			cNotes++;
			cCurLevels++;
		}
		else if (ReadEventResult == READEVENT_NOTE_OFF)
		{
			// ReadNextEvent установил длительность соответствующей ноты в списке
			cFinishedNotes++;
			cCurLevels--;
		}
		else if (ReadEventResult == READEVENT_PART_END)
		{
			// ReadNextEvent установил длительности у всех незавершённых нот в списке

			if (cNotes == 0)
			{
				// в списке не осталось нот

				if (m_cSingleNotes > 0)
				{
					double OverlapsShare = (double) m_cOverlaps / m_cSingleNotes;

					if (OverlapsShare > m_OverlapsThreshold)
					{
						return MIDIPART_OVERLAPS_THRESHOLD_EXCEEDED;
					}
				}

				return MIDIPART_PART_END;
			}

			cFinishedNotes = cNotes;
			cCurLevels = 0;
		}

		// сейчас в списке имеется хотя бы одна нота;
		// проверяем, можно ли сконструировать хотя бы одну одиночную ноту

		// в списке нет ни одной завершённой ноты, конструировать нечего
		if (cFinishedNotes == 0) continue;

		if (cNotes == 1)
		{
			// в списке ровно одна завершённая нота, возвращаем её
			m_cSingleNotes++;
			ReturnFirstNote(pNoteDesc);
			return MIDIPART_SUCCESS;
		}
		else if (cNotes == 2)
		{
			// в списке ровно две ноты и как минимум одна из них завершена

			// завершена только одна нота, конструировать нечего
			if (cFinishedNotes == 1) continue;

			// обе ноты завершены; это может быть либо пересечение, либо созвучие

			NOTELISTITEM *pFirstNote = m_pNoteList;
			NOTELISTITEM *pSecondNote = pFirstNote->pNext;

			if (pSecondNote->ctToNoteOn >
				pFirstNote->ctToNoteOn + pFirstNote->ctDuration / 3 &&
				pSecondNote->ctToNoteOn + pSecondNote->ctDuration >
				pFirstNote->ctToNoteOn + pFirstNote->ctDuration)
			{
				// это пересечение нот

				m_cOverlaps++;

				// если порог максимально допустимой доли перекрытий нот
				// по времени по отношению к количеству одиночных нот в партии
				// равен нулю, то он уже превышен и нужно вернуть ошибку
				if (m_OverlapsThreshold == 0) return MIDIPART_OVERLAPS_THRESHOLD_EXCEEDED;

				// уменьшаем длительность первой ноты так, чтобы ноты не перекрывались
				pFirstNote->ctDuration = pSecondNote->ctToNoteOn - pFirstNote->ctToNoteOn;

				// возвращаем первую ноту пересечения после её усечения;
				// вторая нота будет возвращена при следующем вызове GetNextNote
				m_cSingleNotes++;
				ReturnFirstNote(pNoteDesc);
				return MIDIPART_SUCCESS;
			}

			// это созвучие

			m_cOverlaps++;

			// если порог максимально допустимой доли перекрытий нот
			// по времени по отношению к количеству одиночных нот в партии
			// равен нулю, то он уже превышен и нужно вернуть ошибку
			if (m_OverlapsThreshold == 0) return MIDIPART_OVERLAPS_THRESHOLD_EXCEEDED;

			m_cSingleNotes++;
			ReturnNoteFromConcord(pNoteDesc);
			return MIDIPART_SUCCESS;
		}
		else if (cNotes == 3)
		{
			// в списке ровно три ноты и как минимум одна из них завершена

			// завершена только одна нота, конструировать нечего
			if (cFinishedNotes == 1) continue;

			if (cMaxLevels == 3)
			{
				// если завершены только две ноты, это может оказаться созвучием из трёх
				// нот
				if (cFinishedNotes == 2) continue;

				// завершены все три ноты, это созвучие из трёх нот

				m_cOverlaps++;

				// если порог максимально допустимой доли перекрытий нот
				// по времени по отношению к количеству одиночных нот в партии
				// равен нулю, то он уже превышен и нужно вернуть ошибку
				if (m_OverlapsThreshold == 0) return MIDIPART_OVERLAPS_THRESHOLD_EXCEEDED;

				m_cSingleNotes++;
				ReturnNoteFromConcord(pNoteDesc);
				return MIDIPART_SUCCESS;
			}

			// количество уровней равно двум

			if (cFinishedNotes == 2)
			{
				// завершены две ноты

				NOTELISTITEM *pFirstNote = m_pNoteList;
				NOTELISTITEM *pSecondNote = pFirstNote->pNext;
				NOTELISTITEM *pThirdNote = pSecondNote->pNext;

				if (pThirdNote->ctDuration != UNDEFINED_TICK_COUNT)
				{
					// третья нота, начавшаяся позже всех, завершена;
					// из этих трёх нот может получиться двухуровневое многозвучие
					// (а может и не получиться)
					continue;
				}

				// третья нота не завершена

				if (pFirstNote->ctToNoteOn + pFirstNote->ctDuration >
					pSecondNote->ctToNoteOn + pSecondNote->ctDuration)
				{
					// первая нота заканчивается строго позже второй,
					// а третья начинается до окончания первой;
					// уменьшаем длительность первой ноты так, чтобы она заканчивалась
					// одновременно с началом третьей ноты;
					// после этого усечения первые две ноты образуют созвучие

					pFirstNote->ctDuration = pThirdNote->ctToNoteOn -
						pFirstNote->ctToNoteOn;
				}
				else
				{
					// вторая нота заканчивается строго позже первой,
					// а третья начинается до окончания второй;
					// уменьшаем длительность второй ноты так, чтобы она заканчивалась
					// одновременно с началом третьей ноты;
					// после этого усечения рассматриваем первые две ноты либо как
					// пересечение, либо как созвучие

					pSecondNote->ctDuration = pThirdNote->ctToNoteOn -
						pSecondNote->ctToNoteOn;

					if (pSecondNote->ctToNoteOn >
						pFirstNote->ctToNoteOn + pFirstNote->ctDuration / 3)
					{
						// это пересечение нот

						m_cOverlaps++;

						// если порог максимально допустимой доли перекрытий нот
						// по времени по отношению к количеству одиночных нот в партии
						// равен нулю, то он уже превышен и нужно вернуть ошибку
						if (m_OverlapsThreshold == 0)
						{
							return MIDIPART_OVERLAPS_THRESHOLD_EXCEEDED;
						}

						// уменьшаем длительность первой ноты так, чтобы ноты не
						// перекрывались
						pFirstNote->ctDuration = pSecondNote->ctToNoteOn -
							pFirstNote->ctToNoteOn;

						// возвращаем первую ноту пересечения после её усечения;
						// вторая нота будет возвращена при следующем вызове GetNextNote;
						// третья нота не будет возвращена при следующем вызове
						// GetNextNote
						m_cSingleNotes++;
						ReturnFirstNote(pNoteDesc);
						return MIDIPART_SUCCESS;
					}
				}

				// это созвучие

				m_cOverlaps++;

				// если порог максимально допустимой доли перекрытий нот
				// по времени по отношению к количеству одиночных нот в партии
				// равен нулю, то он уже превышен и нужно вернуть ошибку
				if (m_OverlapsThreshold == 0) return MIDIPART_OVERLAPS_THRESHOLD_EXCEEDED;

				m_cSingleNotes++;
				ReturnNoteFromConcord(pNoteDesc);
				return MIDIPART_SUCCESS;
			}
			else
			{
				// это двухуровневое многозвучие из трёх нот;

				m_cOverlaps += 2;

				// если порог максимально допустимой доли перекрытий нот
				// по времени по отношению к количеству одиночных нот в партии
				// равен нулю, то он уже превышен и нужно вернуть ошибку
				if (m_OverlapsThreshold == 0) return MIDIPART_OVERLAPS_THRESHOLD_EXCEEDED;

				// удаляем базовую ноту многозвучия и возвращаем оставшиеся две ноты

				// базовая нота может быть либо первой, либо второй

				NOTELISTITEM *pFirstNote = m_pNoteList;
				NOTELISTITEM *pSecondNote = pFirstNote->pNext;
				NOTELISTITEM *pThirdNote = pSecondNote->pNext;

				if (pFirstNote->ctToNoteOn + pFirstNote->ctDuration >=
					pThirdNote->ctToNoteOn + pThirdNote->ctDuration)
				{
					// базовая нота - первая, удаляем её
					m_pNoteList = pSecondNote;
					delete pFirstNote;
				}
				else
				{
					// базовая нота - вторая, удаляем её
					pFirstNote->pNext = pThirdNote;
					delete pSecondNote;
				}

				// в списке остались две неперекрывающиеся ноты, возвращаем первую;
				// вторая нота будет возвращена при следующем вызове GetNextNote
				m_cSingleNotes++;
				ReturnFirstNote(pNoteDesc);
				return MIDIPART_SUCCESS;
			}
		}
		else
		{
			// в списке четыре и более нот и как минимум одна из них завершена

			if (cNotes == cFinishedNotes)
			{
				// все ноты в списке завершены

				if (cNotes == cMaxLevels)
				{
					// это созвучие, возвращаем его

					m_cOverlaps++;

					// если порог максимально допустимой доли перекрытий нот
					// по времени по отношению к количеству одиночных нот в партии
					// равен нулю, то он уже превышен и нужно вернуть ошибку
					if (m_OverlapsThreshold == 0)
					{
						return MIDIPART_OVERLAPS_THRESHOLD_EXCEEDED;
					}

					m_cSingleNotes++;
					ReturnNoteFromConcord(pNoteDesc);
					return MIDIPART_SUCCESS;
				}
				else
				{
					// в списке нот находится слишком сложная комбинация нот,
					// из которой мы не умеем конструировать одиночные ноты
					return MIDIPART_COMPLICATED_NOTE_COMBINATION;
				}
			}

			// проверим особый случай (встречается в песне "Снежинки"):
			// если в списке первые N нот завершены, а последние M нот не завершены, то
			// 1) находим время начала самой ранней незавершённой ноты;
			// 2) вычисляем долю перекрытия каждой завершенной ноты с самой ранней
			//    незавершённой нотой (доля перекрытия вычисляется как отношение
			//    длительности перекрытия к длительности завершённой ноты);
			// 3) если доли перекрытия не превышают порог, то усекаем завершённые
			//    ноты так, чтобы из них получилось созвучие, не перекрывающееся
			//    с незавершёнными нотами, и возвращаем это созвучие.
			// сейчас порог установлен в 33% (в песне Снежинки максимальная доля
			// перекрытия составляет 22,89%)

			NOTELISTITEM *pCurNote = m_pNoteList;

			// находим первую незавершённую ноту, она гарантированно есть в списке
			while (pCurNote->ctDuration != UNDEFINED_TICK_COUNT)
			{
				pCurNote = pCurNote->pNext;
			}

			// время начала самой ранней незавершённой ноты
			DWORD ctCutTime = pCurNote->ctToNoteOn;

			// проверяем, что после первой незавершённой ноты нет завершённых;
			// если такие есть, то это не наш случай
			while (pCurNote != NULL && pCurNote->ctDuration == UNDEFINED_TICK_COUNT)
			{
				pCurNote = pCurNote->pNext;
			}

			// после незавершённой ноты встретилась завершённая, это не наш случай
			if (pCurNote != NULL) continue;

			pCurNote = m_pNoteList;

			// проверяем, что доля перекрытия каждой завершённой ноты с самой ранней
			// незавершённой не превышает порог
			while (pCurNote->ctDuration != UNDEFINED_TICK_COUNT)
			{
				// доля перекрытия текущей ноты с самой ранней незавершённой
				double Overlap = ((double) pCurNote->ctToNoteOn + pCurNote->ctDuration -
					ctCutTime) / pCurNote->ctDuration;

				// если доля перекрытия текущей ноты больше порога 33%, выходим из цикла
				if (Overlap > 0.33) break;

				pCurNote = pCurNote->pNext;
			}

			// если доля перекрытия у одной из завершённых нот оказалась больше порога,
			// то это не наш случай
			if (pCurNote->ctDuration != UNDEFINED_TICK_COUNT) continue;

			pCurNote = m_pNoteList;

			// усекаем завершённые ноты
			while (pCurNote->ctDuration != UNDEFINED_TICK_COUNT)
			{
				if (pCurNote->ctToNoteOn + pCurNote->ctDuration > ctCutTime)
				{
					// текущая нота завершилась после начала самой ранней незавершённой
					// ноты, усекаем её
					pCurNote->ctDuration = ctCutTime - pCurNote->ctToNoteOn;
				}

				pCurNote = pCurNote->pNext;
			}

			// после усечения завершённых нот, они стали образовывать созвучие,
			// возвращаем его;
			// внимание: если это созвучие состоит из двух нот, то оно может оказаться
			// пересечением, но мы не будем рассматривать этот случай

			m_cOverlaps++;

			// если порог максимально допустимой доли перекрытий нот
			// по времени по отношению к количеству одиночных нот в партии
			// равен нулю, то он уже превышен и нужно вернуть ошибку
			if (m_OverlapsThreshold == 0) return MIDIPART_OVERLAPS_THRESHOLD_EXCEEDED;

			m_cSingleNotes++;
			ReturnNoteFromConcord(pNoteDesc);
			return MIDIPART_SUCCESS;
		}
	}
}

/****************************************************************************************
*
*   Метод ReadNextEvent
*
*   Параметры
*       Нет
*
*   Возвращаемое значение
*       READEVENT_NOTE_ON - считано событие NOTE_ON, относящееся к партии; в случае
*                           составной ноты этот код возвращается только для первого
*                           события;
*       READEVENT_NOTE_OFF - считано событие NOTE_OFF, относящееся к партии; в случае
*                            составной ноты этот код возвращается только для последнего
*                            события;
*       READEVENT_EMPTY_NOTE - считано событие NOTE_OFF, относящееся к пустой ноте
*                              партии;
*       READEVENT_UNRELEVANT_EVENT - считано событие либо не относящееся к партии, либо
*                                    относящееся, но отличное от NOTE_ON, NOTE_OFF и
*                                    REAL_TRACK_END, либо промежуточное событие NOTE_ON
*                                    или NOTE_OFF, относящееся к составной ноте;
*       READEVENT_PART_END - конец партии; если в конце партии обрываются пустые ноты,
*                            то каждая из них будет удалена из списка и при каждом таком
*                            удалении будет возвращаться код READEVENT_EMPTY_NOTE;
*       READEVENT_CANT_ALLOC_MEMORY - не удалось выделить память для элемента списка.
*
*   Считывает очередное событие трека и обновляет состояние объекта, а именно:
*   1) переменную m_ctCurTime,
*   2) массив m_CurInstrument,
*   3) список m_pNoteList.
*
*   Список нот модифицируется следующим образом:
*   1) если считано событие NOTE_ON, относящееся к партии и не являющееся промежуточным
*      событием составной ноты, то в конец списка добавляется новый элемент; поле
*      ctDuration этого элемента инициализируется значением UNDEFINED_TICK_COUNT, а поле
*      cNotMatchedNoteOnEvents - единицей.
*   2) если считано событие NOTE_ON, относящееся к партии и являющееся промежуточным
*      событием составной ноты, то в соответствующем элементе списка увеличивается
*      значение поля cNotMatchedNoteOnEvents;
*   3) если считано событие NOTE_OFF, которое относится к партии, не является
*      промежуточным событием составной ноты и не относится к пустой ноте, то в
*      соответствующем элементе списка устанавливается значение поля ctDuration;
*   4) если считано событие NOTE_OFF, относящееся к партии и являющееся промежуточным
*      событием составной ноты, то в соответствующем элементе списка уменьшается значение
*      поля cNotMatchedNoteOnEvents;
*   5) если считано событие NOTE_OFF, относящееся к пустой ноте партии, то
*      соответствующий элемент списка удаляется.
*
****************************************************************************************/

MidiPart::READEVENTRESULT MidiPart::ReadNextEvent()
{
	// дельта-время события в тиках
	DWORD ctEventDeltaTime;

	// указатель на первый байт данных события
	BYTE *pEventData;

	// номер канала события
	DWORD EventChannel;

	// считываем очередное событие трека
	DWORD Event = m_pMidiTrack->GetNextEvent(&ctEventDeltaTime, &pEventData,
		&EventChannel);

	if (Event == REAL_TRACK_END)
	{
		NOTELISTITEM *pCurNote = m_pNoteList;
		NOTELISTITEM *pPrevNote = NULL;

		// завершаем незавершённые ноты

		while (pCurNote != NULL)
		{
			if (pCurNote->ctDuration == UNDEFINED_TICK_COUNT)
			{
				// это незавершённая нота

				if (pCurNote->ctToNoteOn == m_ctCurTime)
				{
					// это пустая нота, удаляем её

					if (pPrevNote == NULL) m_pNoteList = pCurNote->pNext;
					else pPrevNote->pNext = pCurNote->pNext;

					delete pCurNote;

					return READEVENT_EMPTY_NOTE;
				}

				pCurNote->ctDuration = m_ctCurTime - pCurNote->ctToNoteOn;
			}

			pPrevNote = pCurNote;
			pCurNote = pCurNote->pNext;
		}

		return READEVENT_PART_END;
	}

	// обновляем текущее время (время в тиках, прошедшее с начала трека)
	m_ctCurTime += ctEventDeltaTime;

	// отбрасываем метасобытия и SysEx-события
	if (Event <= 0x7F || Event >= 0xF0)
	{
		return READEVENT_UNRELEVANT_EVENT;
	}

	// отбрасываем события из каналов, отличных от m_PartChannel
	if (m_PartChannel != ANY_CHANNEL && EventChannel != m_PartChannel)
	{
		return READEVENT_UNRELEVANT_EVENT;
	}

	if (Event == PROGRAM_CHANGE)
	{
		// меняем текущий инструмент на указанном канале
		m_CurInstrument[EventChannel] = *pEventData;
		return READEVENT_UNRELEVANT_EVENT;
	}

	// отбрасываем события для инструментов, отличных от m_PartInstrument
	if (m_CurInstrument[EventChannel] == UNDEFINED_INSTRUMENT ||
		(m_PartInstrument != ANY_INSTRUMENT &&
		m_PartInstrument != m_CurInstrument[EventChannel]))
	{
		return READEVENT_UNRELEVANT_EVENT;
	}

	if (Event == NOTE_ON)
	{
		NOTELISTITEM **ppCurNote = &m_pNoteList;

		// проверка на составную ноту
		while (*ppCurNote != NULL)
		{
			if ((*ppCurNote)->NoteNumber == *pEventData &&
				(*ppCurNote)->cNotMatchedNoteOnEvents > 0 &&
				(*ppCurNote)->Channel == EventChannel &&
				(*ppCurNote)->Instrument == m_CurInstrument[EventChannel])
			{
				(*ppCurNote)->cNotMatchedNoteOnEvents ++;
				return READEVENT_UNRELEVANT_EVENT;
			}

			ppCurNote = &((*ppCurNote)->pNext);
		}

		// добавляем новую ноту в список m_pNoteList
		NOTELISTITEM *pNewItem = new NOTELISTITEM;
		if (pNewItem == NULL)
		{
			LOG("operator new failed\n");
			return READEVENT_CANT_ALLOC_MEMORY;
		}

		pNewItem->NoteNumber = *pEventData;
		pNewItem->ctToNoteOn = m_ctCurTime;
		pNewItem->ctDuration = UNDEFINED_TICK_COUNT;
		pNewItem->cNotMatchedNoteOnEvents = 1;
		pNewItem->Channel = EventChannel;
		pNewItem->Instrument = m_CurInstrument[EventChannel];
		pNewItem->pNext = NULL;

		*ppCurNote = pNewItem;

		return READEVENT_NOTE_ON;
	}
	else if (Event == NOTE_OFF)
	{
		NOTELISTITEM *pCurNote = m_pNoteList;
		NOTELISTITEM *pPrevNote = NULL;

		// ищем ноту в списке m_pNoteList, к которой относится это событие NOTE_OFF
		while (pCurNote != NULL)
		{
			if (pCurNote->NoteNumber == *pEventData &&
				pCurNote->Channel == EventChannel &&
				pCurNote->Instrument == m_CurInstrument[EventChannel] &&
				pCurNote->ctDuration == UNDEFINED_TICK_COUNT)
			{
				// нота найдена

				pCurNote->cNotMatchedNoteOnEvents --;

				if (pCurNote->cNotMatchedNoteOnEvents > 0)
				{
					// незаконченная составная нота
					return READEVENT_UNRELEVANT_EVENT;
				}

				pCurNote->ctDuration = m_ctCurTime - pCurNote->ctToNoteOn;

				if (pCurNote->ctDuration == 0)
				{
					// это пустая нота, удаляем её

					if (pCurNote == m_pNoteList) m_pNoteList = pCurNote->pNext;
					else pPrevNote->pNext = pCurNote->pNext;

					delete pCurNote;

					return READEVENT_EMPTY_NOTE;
				}

				return READEVENT_NOTE_OFF;
			}

			pPrevNote = pCurNote;
			pCurNote = pCurNote->pNext;
		}

		// пришло событие NOTE_OFF, для которого не приходило соответствующего NOTE_ON
		return READEVENT_UNRELEVANT_EVENT;
	}

	return READEVENT_UNRELEVANT_EVENT;
}

/****************************************************************************************
*
*   Метод ReturnFirstNote
*
*   Параметры
*       pNoteDesc - указатель на структуру типа NOTEDESC, в которую нужно записать
*                   информацию о первой ноте из списка; этот параметр может быть равен
*                   NULL
*
*   Возвращаемое значение
*       Нет
*
*   Возвращает первую ноту из списка и удаляет её из списка.
*
****************************************************************************************/

void MidiPart::ReturnFirstNote(
	__out_opt NOTEDESC *pNoteDesc)
{
	if (pNoteDesc != NULL)
	{
		pNoteDesc->NoteNumber = m_pNoteList->NoteNumber;
		pNoteDesc->ctToNoteOn = m_pNoteList->ctToNoteOn;
		pNoteDesc->ctDuration = m_pNoteList->ctDuration;
	}

	NOTELISTITEM *pDelNote = m_pNoteList;

	m_pNoteList = m_pNoteList->pNext;

	delete pDelNote;
}

/****************************************************************************************
*
*   Метод ReturnNoteFromConcord
*
*   Параметры
*       pNoteDesc - указатель на структуру типа NOTEDESC, в которую нужно записать
*                   информацию о первой ноте из списка; этот параметр может быть равен
*                   NULL
*
*   Возвращаемое значение
*       Нет
*
*   Возвращает одну ноту созвучия из списка и удаляет все ноты созвучия из списка.
*   Возвращается нота созвучия либо с минимальным номером, либо с максимальным номером
*   в зависимости от значения переменной m_bChooseNoteWithMinNumber. В конце списка может
*   находиться одна или несколько незавершённых нот, не относящихся к созвучию.
*
****************************************************************************************/

void MidiPart::ReturnNoteFromConcord(
	__out_opt NOTEDESC *pNoteDesc)
{
	if (pNoteDesc != NULL)
	{
		pNoteDesc->NoteNumber = 0;
		pNoteDesc->ctDuration = 0;

		if (m_ConcordNoteChoice == CHOOSE_MIN_NOTE_NUMBER)
		{
			pNoteDesc->NoteNumber = MAXDWORD;
		}
	}

	NOTELISTITEM *pCurNote = m_pNoteList;

	while (pCurNote != NULL)
	{
		if (pCurNote->ctDuration == UNDEFINED_TICK_COUNT)
		{
			// достигнута незавершённая нота; если за ней следуют ноты,
			// то они тоже не завершены
			break;
		}

		if (pNoteDesc != NULL)
		{
			if ((m_ConcordNoteChoice == CHOOSE_MIN_NOTE_NUMBER &&
				pNoteDesc->NoteNumber > pCurNote->NoteNumber) ||
				(m_ConcordNoteChoice == CHOOSE_MAX_NOTE_NUMBER &&
				pNoteDesc->NoteNumber < pCurNote->NoteNumber) ||
				(m_ConcordNoteChoice == CHOOSE_MAX_NOTE_DURATION &&
				pNoteDesc->ctDuration < pCurNote->ctDuration))
			{
				pNoteDesc->NoteNumber = pCurNote->NoteNumber;
				pNoteDesc->ctToNoteOn = pCurNote->ctToNoteOn;
				pNoteDesc->ctDuration = pCurNote->ctDuration;
			}
		}

		NOTELISTITEM *pDelNote = pCurNote;

		pCurNote = pCurNote->pNext;

		delete pDelNote;
	}

	m_pNoteList = pCurNote;
}

/****************************************************************************************
*
*   Метод FreeNoteList
*
*   Параметры
*       Нет
*
*   Возвращаемое значение
*       Нет
*
*   Освобождает память, выделенную для списка нот, на который указывает m_pNoteList.
*
****************************************************************************************/

void MidiPart::FreeNoteList()
{
	NOTELISTITEM *pListHead = m_pNoteList;

	while (pListHead != NULL)
	{
		NOTELISTITEM *pDelNote = pListHead;
		pListHead = pListHead->pNext;
		delete pDelNote;
	}

	m_pNoteList = NULL;
}
