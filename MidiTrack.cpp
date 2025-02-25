/****************************************************************************************
*
*   ����������� ������ MidiTrack
*
*   ������ ����� ������ ������������ ����� ���� MIDI-�����.
*
*   ������: ������� ������������ � ��������� �����������, 2008-2010
*
****************************************************************************************/

#include <windows.h>

#include "Log.h"
#include "MidiLibrary.h"
#include "MidiTrack.h"

/****************************************************************************************
*
*   ���������
*
****************************************************************************************/

// ������ ������� ������
#define EMPTY_RUNNING_STATUS	0x00

/****************************************************************************************
*
*   �����������
*
*   ���������
*       ���
*
*   ������������ ��������
*       ���
*
*   �������������� ���������� �������.
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
*   ����������
*
*   ���������
*       ���
*
*   ������������ ��������
*       ���
*
*   ������ �� ������.
*
****************************************************************************************/

MidiTrack::~MidiTrack()
{
}

/****************************************************************************************
*
*   ����� AttachToTrack
*
*   ���������
*       pFirstByteOfTrack - ��������� �� ������ ������� �����
*       cbTrack - ���������� ����, ���������� ����� ��������� �����
*
*   ������������ ��������
*       true - ������ ������� ��������� � �����;
*       false - � ������ MIDI-������� ����������� ��������� ����.
*
*   ���������� ������ � ���������� ����� MIDI-�����.
*
*   ����������
*
*   ���� ��������� ������� ����� ���������� �� ��������, ����� ������������ ����� �����
*   (������������� ��������������� ������� ���������� m_pLastByteOfTrack).
*
****************************************************************************************/

bool MidiTrack::AttachToTrack(
	__in BYTE *pFirstByteOfTrack,
	__in DWORD cbTrack)
{
	// ��������� �� ��������� ���� �������� �������
	BYTE *pLastByteOfCurEvent = pFirstByteOfTrack - 1;

	// ��������� �� ��������� ���� �����
	BYTE *pLastByteOfTrack = pFirstByteOfTrack + cbTrack - 1;

	// ��������� �� ������� ����
	BYTE *pCurByte = pFirstByteOfTrack;

	// ������� ������
	BYTE RunningStatus = EMPTY_RUNNING_STATUS;

	// ���� �� ��������
	while (pCurByte <= pLastByteOfTrack)
	{
		// ����������� ����� ������-�������
		while (pCurByte <= pLastByteOfTrack && *pCurByte & 0x80) pCurByte++;

		// ��������� � ������� �����, ���������� �� ��������� ������ ������-�������
		pCurByte++;

		// ���� ����� �� ������� �����, �� �������
		if (pCurByte > pLastByteOfTrack) goto exit;

		if (*pCurByte < 0xF0)
		{
			// ��� ��������� MIDI-�������

			if (*pCurByte & 0x80)
			{
				// � ������� ���� ��������� ����
				RunningStatus = *pCurByte;
				pCurByte++;
			}
			else
			{
				// � ������� ��� ���������� �����;
				// ���������, ��� ������� ������ ����������
				if (RunningStatus == EMPTY_RUNNING_STATUS) return false;
			}

			// ��������� ��� ���������� MIDI-�������
			BYTE Event = RunningStatus & 0xF0;

			// ����������� ������� ����� ��� ����� ������, ����������� ��
			pCurByte += 2;

			if (Event == PROGRAM_CHANGE || Event == CHANNEL_AFTER_TOUCH)
			{
				// ������� PROGRAM_CHANGE � CHANNEL_AFTER_TOUCH ����� ���� ���� ������
				pCurByte--;
			}
		}
		else
		{
			// ��� ���� SysEx-�������, ���� �����������

			if (*pCurByte == 0xFF)
			{
				// ��� �����������; ����������� ��������� ���� ������� � ��� �����������
				pCurByte += 2;
			}
			else
			{
				// ��� SysEx-�������; ����������� ��������� ���� �������
				pCurByte++;
			}

			DWORD dwValue = 0;
			BYTE CurByte;
			DWORD cbVLQ = 0;

			// �������� ������ ������ ����������� ��� SysEx-������� � ������
			do
			{
				// ���� ����� �� ������� �����, �� �������
				if (pCurByte > pLastByteOfTrack) goto exit;

				CurByte = *pCurByte;

				dwValue = (dwValue << 7) + (CurByte & 0x7F);

				pCurByte++;
				cbVLQ++;
			}
			while (CurByte & 0x80);

			// ���� �������� ���������� ����� �������� ������ ������ ������,
			// ������� � ������ ����
			if (cbVLQ <= 4) pCurByte += dwValue;
		}

		if (pCurByte <= pLastByteOfTrack + 1)
		{
			// ��������� ��������� �� ��������� ���� �������� �������
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
*   ����� ResetCurrentPosition
*
*   ���������
*       ���
*
*   ������������ ��������
*       ���
*
*   ������������� ������� ������� �� ������ �����.
*
****************************************************************************************/

void MidiTrack::ResetCurrentPosition()
{
	m_pCurByte = m_pFirstTrackEvent;
	m_RunningStatus = EMPTY_RUNNING_STATUS;
}

/****************************************************************************************
*
*   ����� GetNextEvent
*
*   ���������
*       pctDeltaTime - ��������� �� ����������, � ������� ����� �������� ������-�����
*                      ���������� �������; ���� �������� ����� ���� ����� NULL
*       ppDataBytes - ��������� �� ����������, � ������� ����� ������� ��������� ��
*                     ������ ���� ������ ���������� �������; ��� MIDI-������� - ���
*                     ��������� �� ����, ��������� �� ��������� ������ �������, ���
*                     ����������� - ��� ��������� �� ����, ��������� �� ����� �����������
*       pChannel - ��������� �� ����������, � ������� ����� ������� ����� ������
*                  ���������� �������, ���� ��� �������� ��������� MIDI-��������; ����
*                  ��� �� �������� ��������� MIDI-��������, �� ���������� ���� ����������
*                  �� ����������; ���� �������� ����� ���� ����� NULL; �������� �����
*                  ��������� �� ��������� ����� NULL
*
*   ������������ ��������
*       ��� �������: ��� ���������� MIDI-�������, ��� ����������� (������� END_OF_TRACK),
*       ��� SysEx-������� ��� ��� REAL_TRACK_END.
*
*   ���������� ��������� ������� �����. ����� ������ ������� AttachToTrack �
*   ResetCurrentPosition ���� ����� ���������� ������ ������� �����. ���� �����
*   ���������� ��� REAL_TRACK_END ��� ���������� ����� ����� � ������� �� ����������
*   ��� ����������� END_OF_TRACK. ���� ����� ���������� ��� REAL_TRACK_END, ��
*   ����������, �� ������� ��������� ��������� pcTicks, ppDataBytes � pChannel, ��
*   ����������.
*
*   ����������
*
*   ������ ����� �������� ������� ��� ������ GetNextRawEvent. ��� ����������� ��������
*   ������������ ������� � ����� ����� �� �������� �����.
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
*   ����� GetNextRawEvent
*
*   ���������
*       pctDeltaTime - ��������� �� ����������, � ������� ����� �������� ������-�����
*                      ���������� �������
*       ppDataBytes - ��������� �� ����������, � ������� ����� ������� ��������� ��
*                     ������ ���� ������ ���������� �������; ��� MIDI-������� - ���
*                     ��������� �� ����, ��������� �� ��������� ������ �������, ���
*                     ����������� - ��� ��������� �� ����, ��������� �� ����� �����������
*       pChannel - ��������� �� ����������, � ������� ����� ������� ����� ������
*                  ���������� �������, ���� ��� �������� ��������� MIDI-��������; ����
*                  ��� �� �������� ��������� MIDI-��������, �� ���������� ���� ����������
*                  �� ����������; ���� �������� ����� ���� ����� NULL; �������� �����
*                  ��������� �� ��������� ����� NULL
*
*   ������������ ��������
*       ��� �������: ��� ���������� MIDI-�������, ��� ����������� (������� END_OF_TRACK),
*       ��� SysEx-������� ��� ��� REAL_TRACK_END.
*
*   ���������� ��������� ������� �����. ����� ������ ������� AttachToTrack �
*   ResetCurrentPosition ���� ����� ���������� ������ ������� �����. ���� �����
*   ���������� ��� REAL_TRACK_END ��� ���������� ����� �����. ���� ����� ���������� ���
*   REAL_TRACK_END, �� ����������, �� ������� ��������� ��������� pcTicks, ppDataBytes �
*   pChannel, �� ����������.
*
****************************************************************************************/

DWORD MidiTrack::GetNextRawEvent(
	__out DWORD *pctDeltaTime,
	__out BYTE **ppDataBytes,
	__out_opt DWORD *pChannel)
{
	if (m_pCurByte > m_pLastByteOfTrack) return REAL_TRACK_END;

	// �������� �������� ������-�������
	*pctDeltaTime = GetNumberFromVLQ(&m_pCurByte);

	// ���� ��� MIDI-������� (���������� ��� SysEx), ���� ��� �����������
	BYTE Event;

	if (*m_pCurByte < 0xF0)
	{
		// ��� ��������� MIDI-�������

		if (*m_pCurByte & 0x80)
		{
			// � ������� ���� ��������� ����
			m_RunningStatus = *m_pCurByte;
			m_pCurByte ++;
		}

		// ��������� ��������� �� ������ ���� ������ �������
		*ppDataBytes = m_pCurByte;

		// ��������� ��� ���������� MIDI-�������
		Event = m_RunningStatus & 0xF0;

		// ��������� ����� ������ �������
		if (pChannel != NULL) *pChannel = m_RunningStatus & 0x0F;

		if (Event == NOTE_ON)
		{
			// ���� �������� ������� � ������� NOTE_ON ����� ����,
			// �� �� ����� ���� ��� ������� NOTE_OFF
			if (m_pCurByte[1] == 0) Event = NOTE_OFF;
		}

		// ����������� ��������� m_pCurByte �� ��������� ������� �����

		m_pCurByte += 2;

		if (Event == PROGRAM_CHANGE || Event == CHANNEL_AFTER_TOUCH)
		{
			m_pCurByte --;
		}
	}
	else
	{
		// ��� ���� SysEx-�������, ���� �����������

		if (*m_pCurByte == 0xFF)
		{
			// ��� �����������

			// ����������� ��������� ���� �������
			m_pCurByte ++;

			// ��������� ��� �����������
			Event = *m_pCurByte;

			// ����������� ��� �����������
			m_pCurByte ++;

			// ��������� ��������� �� ������ ���� ������ �������
			*ppDataBytes = m_pCurByte;

			// �������� ������ ������ ������� � ������
			DWORD cbData = GetNumberFromVLQ(&m_pCurByte);

			// ����������� ��������� m_pCurByte �� ��������� ������� �����
			m_pCurByte += cbData;
		}
		else
		{
			// ��� SysEx-�������

			// ��������� ��� SysEx-�������
			Event = *m_pCurByte;

			// ����������� ��������� ���� �������
			m_pCurByte ++;

			// ��������� ��������� �� ������ ���� ������ �������
			*ppDataBytes = m_pCurByte;

			// �������� ������ ������ ������� � ������
			DWORD cbData = GetNumberFromVLQ(&m_pCurByte);

			// ����������� ��������� m_pCurByte �� ��������� ������� �����
			m_pCurByte += cbData;
		}
	}

	return Event;
}

/****************************************************************************************
*
*   ����� Duplicate
*
*   ���������
*       ���
*
*   ������������ ��������
*       ��������� �� ��������� ������ ������ MidiTrack ��� NULL � ������ ������.
*
*   ������ ����� ������� �������.
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
