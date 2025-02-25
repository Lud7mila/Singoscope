/****************************************************************************************
*
*   Определение класса MidiTrack
*
*   Объект этого класса представляет собой трек MIDI-файла.
*
*   Авторы: Людмила Огородникова и Александр Огородников, 2008-2010
*
****************************************************************************************/

#include <windows.h>

#include "Log.h"
#include "MidiLibrary.h"
#include "MidiTrack.h"

/****************************************************************************************
*
*   Константы
*
****************************************************************************************/

// пустой текущий статус
#define EMPTY_RUNNING_STATUS	0x00

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

MidiTrack::MidiTrack()
{
	m_pFirstTrackEvent = NULL;
	m_pLastByteOfTrack = NULL;
	m_pCurByte = NULL;
	m_RunningStatus = EMPTY_RUNNING_STATUS;
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
*   Ничего не делает.
*
****************************************************************************************/

MidiTrack::~MidiTrack()
{
}

/****************************************************************************************
*
*   Метод AttachToTrack
*
*   Параметры
*       pFirstByteOfTrack - указатель на первое событие трека
*       cbTrack - количество байт, занимаемое всеми событиями трека
*
*   Возвращаемое значение
*       true - объект успешно подключен к треку;
*       false - в первом MIDI-событии отсутствует статусный байт.
*
*   Подключает объект к указанному треку MIDI-файла.
*
*   Реализация
*
*   Если последнее событие трека обрывается на середине, метод корректирует длину трека
*   (устанавливает соответствующим образом переменную m_pLastByteOfTrack).
*
****************************************************************************************/

bool MidiTrack::AttachToTrack(
	__in BYTE *pFirstByteOfTrack,
	__in DWORD cbTrack)
{
	// указатель на последний байт текущего события
	BYTE *pLastByteOfCurEvent = pFirstByteOfTrack - 1;

	// указатель на последний байт трека
	BYTE *pLastByteOfTrack = pFirstByteOfTrack + cbTrack - 1;

	// указатель на текущий байт
	BYTE *pCurByte = pFirstByteOfTrack;

	// текущий статус
	BYTE RunningStatus = EMPTY_RUNNING_STATUS;

	// цикл по событиям
	while (pCurByte <= pLastByteOfTrack)
	{
		// проматываем байты дельта-времени
		while (pCurByte <= pLastByteOfTrack && *pCurByte & 0x80) pCurByte++;

		// переходим к первому байту, следующему за последним байтом дельта-времени
		pCurByte++;

		// если вышли за пределы трека, то выходим
		if (pCurByte > pLastByteOfTrack) goto exit;

		if (*pCurByte < 0xF0)
		{
			// это канальное MIDI-событие

			if (*pCurByte & 0x80)
			{
				// в событии есть статусный байт
				RunningStatus = *pCurByte;
				pCurByte++;
			}
			else
			{
				// в событии нет статусного байта;
				// проверяем, что текущий статус установлен
				if (RunningStatus == EMPTY_RUNNING_STATUS) return false;
			}

			// сохраняем код канального MIDI-события
			BYTE Event = RunningStatus & 0xF0;

			// большинство событий имеют два байта данных, проматываем их
			pCurByte += 2;

			if (Event == PROGRAM_CHANGE || Event == CHANNEL_AFTER_TOUCH)
			{
				// события PROGRAM_CHANGE и CHANNEL_AFTER_TOUCH имеют один байт данных
				pCurByte--;
			}
		}
		else
		{
			// это либо SysEx-событие, либо метасобытие

			if (*pCurByte == 0xFF)
			{
				// это метасобытие; проматываем статусный байт события и код метасобытия
				pCurByte += 2;
			}
			else
			{
				// это SysEx-событие; проматываем статусный байт события
				pCurByte++;
			}

			DWORD dwValue = 0;
			BYTE CurByte;
			DWORD cbVLQ = 0;

			// получаем размер данных метасобытия или SysEx-события в байтах
			do
			{
				// если вышли за пределы трека, то выходим
				if (pCurByte > pLastByteOfTrack) goto exit;

				CurByte = *pCurByte;

				dwValue = (dwValue << 7) + (CurByte & 0x7F);

				pCurByte++;
				cbVLQ++;
			}
			while (CurByte & 0x80);

			// если величина переменной длины занимает больше четырёх байтов,
			// считаем её равной нулю
			if (cbVLQ <= 4) pCurByte += dwValue;
		}

		if (pCurByte <= pLastByteOfTrack + 1)
		{
			// обновляем указатель на последний байт текущего события
			pLastByteOfCurEvent = pCurByte - 1;
		}
	}

exit:

	m_pFirstTrackEvent = pFirstByteOfTrack;
	m_pLastByteOfTrack = pLastByteOfCurEvent;
	m_RunningStatus = EMPTY_RUNNING_STATUS;

	return true;
}

/****************************************************************************************
*
*   Метод ResetCurrentPosition
*
*   Параметры
*       нет
*
*   Возвращаемое значение
*       нет
*
*   Устанавливает текущую позицию на начало трека.
*
****************************************************************************************/

void MidiTrack::ResetCurrentPosition()
{
	m_pCurByte = m_pFirstTrackEvent;
	m_RunningStatus = EMPTY_RUNNING_STATUS;
}

/****************************************************************************************
*
*   Метод GetNextEvent
*
*   Параметры
*       pctDeltaTime - указатель на переменную, в которую будет записано дельта-время
*                      очередного события; этот параметр может быть равен NULL
*       ppDataBytes - указатель на переменную, в которую будет записан указатель на
*                     первый байт данных очередного события; для MIDI-событий - это
*                     указатель на байт, следующий за статусным байтом события, для
*                     метасобытий - это указатель на байт, следующий за типом метасобытия
*       pChannel - указатель на переменную, в которую будет записан номер канала
*                  очередного события, если оно является канальным MIDI-событием; если
*                  оно не является канальным MIDI-событием, то содержимое этой переменной
*                  не определено; этот параметр может быть равен NULL; значение этого
*                  параметра по умолчанию равно NULL
*
*   Возвращаемое значение
*       Код события: код канального MIDI-события, код метасобытия (включая END_OF_TRACK),
*       код SysEx-события или код REAL_TRACK_END.
*
*   Возвращает очередное событие трека. После вызова методов AttachToTrack и
*   ResetCurrentPosition этот метод возвращает первое событие трека. Этот метод
*   возвращает код REAL_TRACK_END при достижении конца трека и никогда не возвращает
*   код метасобытия END_OF_TRACK. Если метод возвращает код REAL_TRACK_END, то
*   переменные, на которые указывают параметры pcTicks, ppDataBytes и pChannel, не
*   определены.
*
*   Реализация
*
*   Данный метод является обёрткой для метода GetNextRawEvent. Его назначением является
*   отбрасывание события о конце трека из середины трека.
*
****************************************************************************************/

DWORD MidiTrack::GetNextEvent(
	__out_opt DWORD *pctDeltaTime,
	__out BYTE **ppDataBytes,
	__out_opt DWORD *pChannel)
{
	if (m_pCurByte == NULL)	return REAL_TRACK_END;

	DWORD Event;
	DWORD ctDeltaTime, ctTotalDeltaTime = 0;

	do
	{
    	Event = GetNextRawEvent(&ctDeltaTime, ppDataBytes, pChannel);
    	ctTotalDeltaTime += ctDeltaTime;
    }
	while (Event == END_OF_TRACK);

	if (pctDeltaTime != NULL) *pctDeltaTime = ctTotalDeltaTime;

	return Event;
}

/****************************************************************************************
*
*   Метод GetNextRawEvent
*
*   Параметры
*       pctDeltaTime - указатель на переменную, в которую будет записано дельта-время
*                      очередного события
*       ppDataBytes - указатель на переменную, в которую будет записан указатель на
*                     первый байт данных очередного события; для MIDI-событий - это
*                     указатель на байт, следующий за статусным байтом события, для
*                     метасобытий - это указатель на байт, следующий за типом метасобытия
*       pChannel - указатель на переменную, в которую будет записан номер канала
*                  очередного события, если оно является канальным MIDI-событием; если
*                  оно не является канальным MIDI-событием, то содержимое этой переменной
*                  не определено; этот параметр может быть равен NULL; значение этого
*                  параметра по умолчанию равно NULL
*
*   Возвращаемое значение
*       Код события: код канального MIDI-события, код метасобытия (включая END_OF_TRACK),
*       код SysEx-события или код REAL_TRACK_END.
*
*   Возвращает очередное событие трека. После вызова методов AttachToTrack и
*   ResetCurrentPosition этот метод возвращает первое событие трека. Этот метод
*   возвращает код REAL_TRACK_END при достижении конца трека. Если метод возвращает код
*   REAL_TRACK_END, то переменные, на которые указывают параметры pcTicks, ppDataBytes и
*   pChannel, не определены.
*
****************************************************************************************/

DWORD MidiTrack::GetNextRawEvent(
	__out DWORD *pctDeltaTime,
	__out BYTE **ppDataBytes,
	__out_opt DWORD *pChannel)
{
	if (m_pCurByte > m_pLastByteOfTrack) return REAL_TRACK_END;

	// получаем значение дельта-времени
	*pctDeltaTime = GetNumberFromVLQ(&m_pCurByte);

	// либо код MIDI-события (канального или SysEx), либо код метасобытия
	BYTE Event;

	if (*m_pCurByte < 0xF0)
	{
		// это канальное MIDI-событие

		if (*m_pCurByte & 0x80)
		{
			// в событии есть статусный байт
			m_RunningStatus = *m_pCurByte;
			m_pCurByte ++;
		}

		// сохраняем указатель на первый байт данных события
		*ppDataBytes = m_pCurByte;

		// сохраняем код канального MIDI-события
		Event = m_RunningStatus & 0xF0;

		// сохраняем номер канала события
		if (pChannel != NULL) *pChannel = m_RunningStatus & 0x0F;

		if (Event == NOTE_ON)
		{
			// если скорость нажатия у события NOTE_ON равна нулю,
			// то на самом деле это событие NOTE_OFF
			if (m_pCurByte[1] == 0) Event = NOTE_OFF;
		}

		// проматываем указатель m_pCurByte на следующее событие трека

		m_pCurByte += 2;

		if (Event == PROGRAM_CHANGE || Event == CHANNEL_AFTER_TOUCH)
		{
			m_pCurByte --;
		}
	}
	else
	{
		// это либо SysEx-событие, либо метасобытие

		if (*m_pCurByte == 0xFF)
		{
			// это метасобытие

			// проматываем статусный байт события
			m_pCurByte ++;

			// сохраняем код метасобытия
			Event = *m_pCurByte;

			// проматываем код метасобытия
			m_pCurByte ++;

			// сохраняем указатель на первый байт данных события
			*ppDataBytes = m_pCurByte;

			// получаем размер данных события в байтах
			DWORD cbData = GetNumberFromVLQ(&m_pCurByte);

			// проматываем указатель m_pCurByte на следующее событие трека
			m_pCurByte += cbData;
		}
		else
		{
			// это SysEx-событие

			// сохраняем код SysEx-события
			Event = *m_pCurByte;

			// проматываем статусный байт события
			m_pCurByte ++;

			// сохраняем указатель на первый байт данных события
			*ppDataBytes = m_pCurByte;

			// получаем размер данных события в байтах
			DWORD cbData = GetNumberFromVLQ(&m_pCurByte);

			// проматываем указатель m_pCurByte на следующее событие трека
			m_pCurByte += cbData;
		}
	}

	return Event;
}

/****************************************************************************************
*
*   Метод Duplicate
*
*   Параметры
*       Нет
*
*   Возвращаемое значение
*       Указатель на созданный объект класса MidiTrack или NULL в случае ошибки.
*
*   Создаёт копию данного объекта.
*
****************************************************************************************/

MidiTrack *MidiTrack::Duplicate()
{
	MidiTrack *pNewMidiTrack = new MidiTrack;

	if (pNewMidiTrack == NULL)
	{
		LOG("operator new failed\n");
		return NULL;
	}

	pNewMidiTrack->AttachToTrack(m_pFirstTrackEvent,
		m_pLastByteOfTrack - m_pFirstTrackEvent + 1);

	return pNewMidiTrack;
}
