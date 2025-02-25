/****************************************************************************************
*
*   ����������� ������ MidiPart
*
*   ������ ����� ������ ������������ ����� ������ � ����� MIDI-�����. ������ - ���
*   ������������������ ���, ������������ MIDI-��������� �� ������ � ���� �� �����.
*   ������������ ������� ���������, ����� MIDI-������� ������ ���������� � ������:
*   1) MIDI-������� � �������� ������� ������ ���� � ����� ������� ������;
*   2) MIDI-������� � �������� ������� ����������� ���� � ����� ������� �����������.
*
*   ������: ������� ������������ � ��������� �����������, 2007-2010
*
****************************************************************************************/

#include <windows.h>

#include "Log.h"
#include "MidiLibrary.h"
#include "MidiTrack.h"
#include "MidiPart.h"

/****************************************************************************************
*
*   ���������
*
****************************************************************************************/

// ��������, ������� ���������������� �������� ������� m_CurInstrument
#define UNDEFINED_INSTRUMENT	0xFFFFFFFF

// ��������� �������� ����������, �������� ����� � �����;
// ������������ ������ ������ GetNextNote
#define UNDEFINED_TICK_COUNT	0xFFFFFFFF

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

MidiPart::MidiPart()
{
	m_pMidiTrack = NULL;
	m_pNoteList = NULL;
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
*   ����������� ���������� �� ����� ������ ������. �� ������� ������ ������ MidiTrack,
*   �������� ��� ������ ������ InitSearch.
*
****************************************************************************************/

MidiPart::~MidiPart()
{
	FreeNoteList();
}

/****************************************************************************************
*
*   ����� InitSearch
*
*   ���������
*       pMidiTrack - ��������� �� ������ ������ MidiTrack, �������������� ����� ����
*                    MIDI-�����, � ������� ��������� ������
*       Channel - ����� ������, � ������� ��������� ������; ���� �������� ����� ���������
*                 ����� ANY_CHANNEL, �� � ������ ��������� ���� �� ���� ������� �����
*       Instrument - ����� �����������, ������� �������� ���� ������ ������; ����
*                    �������� ����� ��������� ����� ANY_INSTRUMENT, �� � ������ ���������
*                    ����, �������� ����� ������������
*       OverlapsThreshold - ����������� ���������� ���� ���������� ��� �� �������
*                           �� ��������� � ���������� ��������� ��� � ������. ���� �����
*                           ������������ ������� GetNextNote: ���� �� ��������, �����
*                           GetNextNote ���������� ��� ������
*                           MIDIPART_OVERLAPS_THRESHOLD_EXCEEDED
*       ConcordNoteChoice - ����������, ����� �� ��� �������� ������ ���������� �����
*                           GetNextNote:
*                           CHOOSE_MIN_NOTE_NUMBER - ���� � ����������� �������,
*                           CHOOSE_MAX_NOTE_NUMBER - ���� � ������������ �������,
*                           CHOOSE_MAX_NOTE_DURATION - ���� � ������������ �������������
*
*   ������������ ��������
*       ���
*
*   �������������� ����� ��� � ������, �������� ����������� pMidiTrack, Channel �
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
	// ���� ���� ������ ��� ������������� ��� ������ ���, ����������� ���������� ��
	// ����� �������� ������ ������
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
*   ����� GetNextNote
*
*   ���������
*       pNoteDesc - ��������� �� ��������� ���� NOTEDESC, � ������� ����� ��������
*                   ���������� �� ��������� ��������� ���� ������; ���� �������� �����
*                   ���� ����� NULL
*
*   ������������ ��������
*		MIDIPART_SUCCESS - ��������� ��������� ���� ��������������� � ����������;
*		MIDIPART_PART_END - ����� ������;
*		MIDIPART_CANT_ALLOC_MEMORY - �� ������� �������� ������;
*       MIDIPART_COMPLICATED_NOTE_COMBINATION - ����������� ������� ������� ����������
*                                               ���, ����� �� ����� ���������������
*                                               ��������� ����;
*		MIDIPART_OVERLAPS_THRESHOLD_EXCEEDED - �������� ����� ����������� ���������� ����
*                                              ���������� ��� �� ������� �� ��������� �
*                                              ���������� ��������� ��� � ������
*
*   ������������ � ���������� ��������� ��������� ���� ������. ����� ������ ������
*   InitSearch ���� ����� ���������� ������ ���� ������. ���� ����� ���������� ����� ���,
*   ����� MIDIPART_SUCCESS, �� ���� ���������, �� ������� ��������� �������� pNoteDesc,
*   �� ����������.
*
****************************************************************************************/

MIDIPARTRESULT MidiPart::GetNextNote(
	__out_opt NOTEDESC *pNoteDesc)
{
	if (m_pNoteList != NULL && m_pNoteList->ctDuration != UNDEFINED_TICK_COUNT)
	{
		// � ������ ���� ���� ����������� ����, ������� ����� �������
		m_cSingleNotes++;
		ReturnFirstNote(pNoteDesc);
		return MIDIPART_SUCCESS;
	}

	// � ���� ������ � ������ ����� ���������� ���� ��� ��������� ������������� ���

	// ���������� ��� � ������
	DWORD cNotes = 0;

	// ���������� ����������� ��� � ������; ������ ���� ������� �� ����������� � ����
	// ����������
	DWORD cFinishedNotes = 0;

	// ������������ ���������� ������� (������������ �������� ���) �� �������� �������,
	// �������� ���� ������; � ���� ���������� ������� �� ����������� ������ ����
	DWORD cMaxLevels = 0;

	// ������� ���������� ������� (������������ �������� ���);
	// � ���� ���������� ����� ����������� ������ ����
	DWORD cCurLevels = 0;

	// ������ �������, � ������� ������ ������������� ������� �����
	DWORD ctLastTime = m_ctCurTime;

	// ���� � ������ ���� ������������� ����, ������������ ����������,
	// ������������������ ����
	NOTELISTITEM *pCurNote = m_pNoteList;
	while (pCurNote != NULL)
	{
		cNotes++;
		cCurLevels++;

		if (pCurNote->ctToNoteOn < m_ctCurTime) cMaxLevels++;

		pCurNote = pCurNote->pNext;
	}

	// ���� �� ���� �������� �����; ������� �� ����� �����,
	// ����� ����� ��������������� ���� �� ���� ��������� ����
	while (true)
	{
		// ��������� ��������� ������� �����

		READEVENTRESULT ReadEventResult = ReadNextEvent();

		if (ReadEventResult == READEVENT_UNRELEVANT_EVENT) continue;

		if (ReadEventResult == READEVENT_CANT_ALLOC_MEMORY)
		{
			LOG("ReadNextEvent failed\n");
			return MIDIPART_CANT_ALLOC_MEMORY;
		}

		if (ReadEventResult == READEVENT_EMPTY_NOTE)
		{
			// ReadNextEvent ������ ������ ���� �� ������
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
			// ReadNextEvent ������� ����� ���� � ������
			cNotes++;
			cCurLevels++;
		}
		else if (ReadEventResult == READEVENT_NOTE_OFF)
		{
			// ReadNextEvent ��������� ������������ ��������������� ���� � ������
			cFinishedNotes++;
			cCurLevels--;
		}
		else if (ReadEventResult == READEVENT_PART_END)
		{
			// ReadNextEvent ��������� ������������ � ���� ������������� ��� � ������

			if (cNotes == 0)
			{
				// � ������ �� �������� ���

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

		// ������ � ������ ������� ���� �� ���� ����;
		// ���������, ����� �� ��������������� ���� �� ���� ��������� ����

		// � ������ ��� �� ����� ����������� ����, �������������� ������
		if (cFinishedNotes == 0) continue;

		if (cNotes == 1)
		{
			// � ������ ����� ���� ����������� ����, ���������� �
			m_cSingleNotes++;
			ReturnFirstNote(pNoteDesc);
			return MIDIPART_SUCCESS;
		}
		else if (cNotes == 2)
		{
			// � ������ ����� ��� ���� � ��� ������� ���� �� ��� ���������

			// ��������� ������ ���� ����, �������������� ������
			if (cFinishedNotes == 1) continue;

			// ��� ���� ���������; ��� ����� ���� ���� �����������, ���� ��������

			NOTELISTITEM *pFirstNote = m_pNoteList;
			NOTELISTITEM *pSecondNote = pFirstNote->pNext;

			if (pSecondNote->ctToNoteOn >
				pFirstNote->ctToNoteOn + pFirstNote->ctDuration / 3 &&
				pSecondNote->ctToNoteOn + pSecondNote->ctDuration >
				pFirstNote->ctToNoteOn + pFirstNote->ctDuration)
			{
				// ��� ����������� ���

				m_cOverlaps++;

				// ���� ����� ����������� ���������� ���� ���������� ���
				// �� ������� �� ��������� � ���������� ��������� ��� � ������
				// ����� ����, �� �� ��� �������� � ����� ������� ������
				if (m_OverlapsThreshold == 0) return MIDIPART_OVERLAPS_THRESHOLD_EXCEEDED;

				// ��������� ������������ ������ ���� ���, ����� ���� �� �������������
				pFirstNote->ctDuration = pSecondNote->ctToNoteOn - pFirstNote->ctToNoteOn;

				// ���������� ������ ���� ����������� ����� � ��������;
				// ������ ���� ����� ���������� ��� ��������� ������ GetNextNote
				m_cSingleNotes++;
				ReturnFirstNote(pNoteDesc);
				return MIDIPART_SUCCESS;
			}

			// ��� ��������

			m_cOverlaps++;

			// ���� ����� ����������� ���������� ���� ���������� ���
			// �� ������� �� ��������� � ���������� ��������� ��� � ������
			// ����� ����, �� �� ��� �������� � ����� ������� ������
			if (m_OverlapsThreshold == 0) return MIDIPART_OVERLAPS_THRESHOLD_EXCEEDED;

			m_cSingleNotes++;
			ReturnNoteFromConcord(pNoteDesc);
			return MIDIPART_SUCCESS;
		}
		else if (cNotes == 3)
		{
			// � ������ ����� ��� ���� � ��� ������� ���� �� ��� ���������

			// ��������� ������ ���� ����, �������������� ������
			if (cFinishedNotes == 1) continue;

			if (cMaxLevels == 3)
			{
				// ���� ��������� ������ ��� ����, ��� ����� ��������� ��������� �� ���
				// ���
				if (cFinishedNotes == 2) continue;

				// ��������� ��� ��� ����, ��� �������� �� ��� ���

				m_cOverlaps++;

				// ���� ����� ����������� ���������� ���� ���������� ���
				// �� ������� �� ��������� � ���������� ��������� ��� � ������
				// ����� ����, �� �� ��� �������� � ����� ������� ������
				if (m_OverlapsThreshold == 0) return MIDIPART_OVERLAPS_THRESHOLD_EXCEEDED;

				m_cSingleNotes++;
				ReturnNoteFromConcord(pNoteDesc);
				return MIDIPART_SUCCESS;
			}

			// ���������� ������� ����� ����

			if (cFinishedNotes == 2)
			{
				// ��������� ��� ����

				NOTELISTITEM *pFirstNote = m_pNoteList;
				NOTELISTITEM *pSecondNote = pFirstNote->pNext;
				NOTELISTITEM *pThirdNote = pSecondNote->pNext;

				if (pThirdNote->ctDuration != UNDEFINED_TICK_COUNT)
				{
					// ������ ����, ���������� ����� ����, ���������;
					// �� ���� ��� ��� ����� ���������� ������������� �����������
					// (� ����� � �� ����������)
					continue;
				}

				// ������ ���� �� ���������

				if (pFirstNote->ctToNoteOn + pFirstNote->ctDuration >
					pSecondNote->ctToNoteOn + pSecondNote->ctDuration)
				{
					// ������ ���� ������������� ������ ����� ������,
					// � ������ ���������� �� ��������� ������;
					// ��������� ������������ ������ ���� ���, ����� ��� �������������
					// ������������ � ������� ������� ����;
					// ����� ����� �������� ������ ��� ���� �������� ��������

					pFirstNote->ctDuration = pThirdNote->ctToNoteOn -
						pFirstNote->ctToNoteOn;
				}
				else
				{
					// ������ ���� ������������� ������ ����� ������,
					// � ������ ���������� �� ��������� ������;
					// ��������� ������������ ������ ���� ���, ����� ��� �������������
					// ������������ � ������� ������� ����;
					// ����� ����� �������� ������������� ������ ��� ���� ���� ���
					// �����������, ���� ��� ��������

					pSecondNote->ctDuration = pThirdNote->ctToNoteOn -
						pSecondNote->ctToNoteOn;

					if (pSecondNote->ctToNoteOn >
						pFirstNote->ctToNoteOn + pFirstNote->ctDuration / 3)
					{
						// ��� ����������� ���

						m_cOverlaps++;

						// ���� ����� ����������� ���������� ���� ���������� ���
						// �� ������� �� ��������� � ���������� ��������� ��� � ������
						// ����� ����, �� �� ��� �������� � ����� ������� ������
						if (m_OverlapsThreshold == 0)
						{
							return MIDIPART_OVERLAPS_THRESHOLD_EXCEEDED;
						}

						// ��������� ������������ ������ ���� ���, ����� ���� ��
						// �������������
						pFirstNote->ctDuration = pSecondNote->ctToNoteOn -
							pFirstNote->ctToNoteOn;

						// ���������� ������ ���� ����������� ����� � ��������;
						// ������ ���� ����� ���������� ��� ��������� ������ GetNextNote;
						// ������ ���� �� ����� ���������� ��� ��������� ������
						// GetNextNote
						m_cSingleNotes++;
						ReturnFirstNote(pNoteDesc);
						return MIDIPART_SUCCESS;
					}
				}

				// ��� ��������

				m_cOverlaps++;

				// ���� ����� ����������� ���������� ���� ���������� ���
				// �� ������� �� ��������� � ���������� ��������� ��� � ������
				// ����� ����, �� �� ��� �������� � ����� ������� ������
				if (m_OverlapsThreshold == 0) return MIDIPART_OVERLAPS_THRESHOLD_EXCEEDED;

				m_cSingleNotes++;
				ReturnNoteFromConcord(pNoteDesc);
				return MIDIPART_SUCCESS;
			}
			else
			{
				// ��� ������������� ����������� �� ��� ���;

				m_cOverlaps += 2;

				// ���� ����� ����������� ���������� ���� ���������� ���
				// �� ������� �� ��������� � ���������� ��������� ��� � ������
				// ����� ����, �� �� ��� �������� � ����� ������� ������
				if (m_OverlapsThreshold == 0) return MIDIPART_OVERLAPS_THRESHOLD_EXCEEDED;

				// ������� ������� ���� ����������� � ���������� ���������� ��� ����

				// ������� ���� ����� ���� ���� ������, ���� ������

				NOTELISTITEM *pFirstNote = m_pNoteList;
				NOTELISTITEM *pSecondNote = pFirstNote->pNext;
				NOTELISTITEM *pThirdNote = pSecondNote->pNext;

				if (pFirstNote->ctToNoteOn + pFirstNote->ctDuration >=
					pThirdNote->ctToNoteOn + pThirdNote->ctDuration)
				{
					// ������� ���� - ������, ������� �
					m_pNoteList = pSecondNote;
					delete pFirstNote;
				}
				else
				{
					// ������� ���� - ������, ������� �
					pFirstNote->pNext = pThirdNote;
					delete pSecondNote;
				}

				// � ������ �������� ��� ����������������� ����, ���������� ������;
				// ������ ���� ����� ���������� ��� ��������� ������ GetNextNote
				m_cSingleNotes++;
				ReturnFirstNote(pNoteDesc);
				return MIDIPART_SUCCESS;
			}
		}
		else
		{
			// � ������ ������ � ����� ��� � ��� ������� ���� �� ��� ���������

			if (cNotes == cFinishedNotes)
			{
				// ��� ���� � ������ ���������

				if (cNotes == cMaxLevels)
				{
					// ��� ��������, ���������� ���

					m_cOverlaps++;

					// ���� ����� ����������� ���������� ���� ���������� ���
					// �� ������� �� ��������� � ���������� ��������� ��� � ������
					// ����� ����, �� �� ��� �������� � ����� ������� ������
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
					// � ������ ��� ��������� ������� ������� ���������� ���,
					// �� ������� �� �� ����� �������������� ��������� ����
					return MIDIPART_COMPLICATED_NOTE_COMBINATION;
				}
			}

			// �������� ������ ������ (����������� � ����� "��������"):
			// ���� � ������ ������ N ��� ���������, � ��������� M ��� �� ���������, ��
			// 1) ������� ����� ������ ����� ������ ������������� ����;
			// 2) ��������� ���� ���������� ������ ����������� ���� � ����� ������
			//    ������������� ����� (���� ���������� ����������� ��� ���������
			//    ������������ ���������� � ������������ ����������� ����);
			// 3) ���� ���� ���������� �� ��������� �����, �� ������� �����������
			//    ���� ���, ����� �� ��� ���������� ��������, �� ���������������
			//    � �������������� ������, � ���������� ��� ��������.
			// ������ ����� ���������� � 33% (� ����� �������� ������������ ����
			// ���������� ���������� 22,89%)

			NOTELISTITEM *pCurNote = m_pNoteList;

			// ������� ������ ������������� ����, ��� �������������� ���� � ������
			while (pCurNote->ctDuration != UNDEFINED_TICK_COUNT)
			{
				pCurNote = pCurNote->pNext;
			}

			// ����� ������ ����� ������ ������������� ����
			DWORD ctCutTime = pCurNote->ctToNoteOn;

			// ���������, ��� ����� ������ ������������� ���� ��� �����������;
			// ���� ����� ����, �� ��� �� ��� ������
			while (pCurNote != NULL && pCurNote->ctDuration == UNDEFINED_TICK_COUNT)
			{
				pCurNote = pCurNote->pNext;
			}

			// ����� ������������� ���� ����������� �����������, ��� �� ��� ������
			if (pCurNote != NULL) continue;

			pCurNote = m_pNoteList;

			// ���������, ��� ���� ���������� ������ ����������� ���� � ����� ������
			// ������������� �� ��������� �����
			while (pCurNote->ctDuration != UNDEFINED_TICK_COUNT)
			{
				// ���� ���������� ������� ���� � ����� ������ �������������
				double Overlap = ((double) pCurNote->ctToNoteOn + pCurNote->ctDuration -
					ctCutTime) / pCurNote->ctDuration;

				// ���� ���� ���������� ������� ���� ������ ������ 33%, ������� �� �����
				if (Overlap > 0.33) break;

				pCurNote = pCurNote->pNext;
			}

			// ���� ���� ���������� � ����� �� ����������� ��� ��������� ������ ������,
			// �� ��� �� ��� ������
			if (pCurNote->ctDuration != UNDEFINED_TICK_COUNT) continue;

			pCurNote = m_pNoteList;

			// ������� ����������� ����
			while (pCurNote->ctDuration != UNDEFINED_TICK_COUNT)
			{
				if (pCurNote->ctToNoteOn + pCurNote->ctDuration > ctCutTime)
				{
					// ������� ���� ����������� ����� ������ ����� ������ �������������
					// ����, ������� �
					pCurNote->ctDuration = ctCutTime - pCurNote->ctToNoteOn;
				}

				pCurNote = pCurNote->pNext;
			}

			// ����� �������� ����������� ���, ��� ����� ������������ ��������,
			// ���������� ���;
			// ��������: ���� ��� �������� ������� �� ���� ���, �� ��� ����� ���������
			// ������������, �� �� �� ����� ������������� ���� ������

			m_cOverlaps++;

			// ���� ����� ����������� ���������� ���� ���������� ���
			// �� ������� �� ��������� � ���������� ��������� ��� � ������
			// ����� ����, �� �� ��� �������� � ����� ������� ������
			if (m_OverlapsThreshold == 0) return MIDIPART_OVERLAPS_THRESHOLD_EXCEEDED;

			m_cSingleNotes++;
			ReturnNoteFromConcord(pNoteDesc);
			return MIDIPART_SUCCESS;
		}
	}
}

/****************************************************************************************
*
*   ����� ReadNextEvent
*
*   ���������
*       ���
*
*   ������������ ��������
*       READEVENT_NOTE_ON - ������� ������� NOTE_ON, ����������� � ������; � ������
*                           ��������� ���� ���� ��� ������������ ������ ��� �������
*                           �������;
*       READEVENT_NOTE_OFF - ������� ������� NOTE_OFF, ����������� � ������; � ������
*                            ��������� ���� ���� ��� ������������ ������ ��� ����������
*                            �������;
*       READEVENT_EMPTY_NOTE - ������� ������� NOTE_OFF, ����������� � ������ ����
*                              ������;
*       READEVENT_UNRELEVANT_EVENT - ������� ������� ���� �� ����������� � ������, ����
*                                    �����������, �� �������� �� NOTE_ON, NOTE_OFF �
*                                    REAL_TRACK_END, ���� ������������� ������� NOTE_ON
*                                    ��� NOTE_OFF, ����������� � ��������� ����;
*       READEVENT_PART_END - ����� ������; ���� � ����� ������ ���������� ������ ����,
*                            �� ������ �� ��� ����� ������� �� ������ � ��� ������ �����
*                            �������� ����� ������������ ��� READEVENT_EMPTY_NOTE;
*       READEVENT_CANT_ALLOC_MEMORY - �� ������� �������� ������ ��� �������� ������.
*
*   ��������� ��������� ������� ����� � ��������� ��������� �������, � ������:
*   1) ���������� m_ctCurTime,
*   2) ������ m_CurInstrument,
*   3) ������ m_pNoteList.
*
*   ������ ��� �������������� ��������� �������:
*   1) ���� ������� ������� NOTE_ON, ����������� � ������ � �� ���������� �������������
*      �������� ��������� ����, �� � ����� ������ ����������� ����� �������; ����
*      ctDuration ����� �������� ���������������� ��������� UNDEFINED_TICK_COUNT, � ����
*      cNotMatchedNoteOnEvents - ��������.
*   2) ���� ������� ������� NOTE_ON, ����������� � ������ � ���������� �������������
*      �������� ��������� ����, �� � ��������������� �������� ������ �������������
*      �������� ���� cNotMatchedNoteOnEvents;
*   3) ���� ������� ������� NOTE_OFF, ������� ��������� � ������, �� ��������
*      ������������� �������� ��������� ���� � �� ��������� � ������ ����, �� �
*      ��������������� �������� ������ ��������������� �������� ���� ctDuration;
*   4) ���� ������� ������� NOTE_OFF, ����������� � ������ � ���������� �������������
*      �������� ��������� ����, �� � ��������������� �������� ������ ����������� ��������
*      ���� cNotMatchedNoteOnEvents;
*   5) ���� ������� ������� NOTE_OFF, ����������� � ������ ���� ������, ��
*      ��������������� ������� ������ ���������.
*
****************************************************************************************/

MidiPart::READEVENTRESULT MidiPart::ReadNextEvent()
{
	// ������-����� ������� � �����
	DWORD ctEventDeltaTime;

	// ��������� �� ������ ���� ������ �������
	BYTE *pEventData;

	// ����� ������ �������
	DWORD EventChannel;

	// ��������� ��������� ������� �����
	DWORD Event = m_pMidiTrack->GetNextEvent(&ctEventDeltaTime, &pEventData,
		&EventChannel);

	if (Event == REAL_TRACK_END)
	{
		NOTELISTITEM *pCurNote = m_pNoteList;
		NOTELISTITEM *pPrevNote = NULL;

		// ��������� ������������� ����

		while (pCurNote != NULL)
		{
			if (pCurNote->ctDuration == UNDEFINED_TICK_COUNT)
			{
				// ��� ������������� ����

				if (pCurNote->ctToNoteOn == m_ctCurTime)
				{
					// ��� ������ ����, ������� �

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

	// ��������� ������� ����� (����� � �����, ��������� � ������ �����)
	m_ctCurTime += ctEventDeltaTime;

	// ����������� ����������� � SysEx-�������
	if (Event <= 0x7F || Event >= 0xF0)
	{
		return READEVENT_UNRELEVANT_EVENT;
	}

	// ����������� ������� �� �������, �������� �� m_PartChannel
	if (m_PartChannel != ANY_CHANNEL && EventChannel != m_PartChannel)
	{
		return READEVENT_UNRELEVANT_EVENT;
	}

	if (Event == PROGRAM_CHANGE)
	{
		// ������ ������� ���������� �� ��������� ������
		m_CurInstrument[EventChannel] = *pEventData;
		return READEVENT_UNRELEVANT_EVENT;
	}

	// ����������� ������� ��� ������������, �������� �� m_PartInstrument
	if (m_CurInstrument[EventChannel] == UNDEFINED_INSTRUMENT ||
		(m_PartInstrument != ANY_INSTRUMENT &&
		m_PartInstrument != m_CurInstrument[EventChannel]))
	{
		return READEVENT_UNRELEVANT_EVENT;
	}

	if (Event == NOTE_ON)
	{
		NOTELISTITEM **ppCurNote = &m_pNoteList;

		// �������� �� ��������� ����
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

		// ��������� ����� ���� � ������ m_pNoteList
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

		// ���� ���� � ������ m_pNoteList, � ������� ��������� ��� ������� NOTE_OFF
		while (pCurNote != NULL)
		{
			if (pCurNote->NoteNumber == *pEventData &&
				pCurNote->Channel == EventChannel &&
				pCurNote->Instrument == m_CurInstrument[EventChannel] &&
				pCurNote->ctDuration == UNDEFINED_TICK_COUNT)
			{
				// ���� �������

				pCurNote->cNotMatchedNoteOnEvents --;

				if (pCurNote->cNotMatchedNoteOnEvents > 0)
				{
					// ������������� ��������� ����
					return READEVENT_UNRELEVANT_EVENT;
				}

				pCurNote->ctDuration = m_ctCurTime - pCurNote->ctToNoteOn;

				if (pCurNote->ctDuration == 0)
				{
					// ��� ������ ����, ������� �

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

		// ������ ������� NOTE_OFF, ��� �������� �� ��������� ���������������� NOTE_ON
		return READEVENT_UNRELEVANT_EVENT;
	}

	return READEVENT_UNRELEVANT_EVENT;
}

/****************************************************************************************
*
*   ����� ReturnFirstNote
*
*   ���������
*       pNoteDesc - ��������� �� ��������� ���� NOTEDESC, � ������� ����� ��������
*                   ���������� � ������ ���� �� ������; ���� �������� ����� ���� �����
*                   NULL
*
*   ������������ ��������
*       ���
*
*   ���������� ������ ���� �� ������ � ������� � �� ������.
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
*   ����� ReturnNoteFromConcord
*
*   ���������
*       pNoteDesc - ��������� �� ��������� ���� NOTEDESC, � ������� ����� ��������
*                   ���������� � ������ ���� �� ������; ���� �������� ����� ���� �����
*                   NULL
*
*   ������������ ��������
*       ���
*
*   ���������� ���� ���� �������� �� ������ � ������� ��� ���� �������� �� ������.
*   ������������ ���� �������� ���� � ����������� �������, ���� � ������������ �������
*   � ����������� �� �������� ���������� m_bChooseNoteWithMinNumber. � ����� ������ �����
*   ���������� ���� ��� ��������� ������������� ���, �� ����������� � ��������.
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
			// ���������� ������������� ����; ���� �� ��� ������� ����,
			// �� ��� ���� �� ���������
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
*   ����� FreeNoteList
*
*   ���������
*       ���
*
*   ������������ ��������
*       ���
*
*   ����������� ������, ���������� ��� ������ ���, �� ������� ��������� m_pNoteList.
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
