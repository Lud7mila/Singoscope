/****************************************************************************************
*
*   ����������� ������ MidiSong
*
*   ������ ����� ������ ������������ ����� MIDI-�����. ����� �����������:
*   1) ������������������� �������� ������� - ��� (Singing Event Sequence - SES);
*   2) ������������������� ������ (Measure Sequence);
*   3) ������ ������.
*
*   �����: ������� ������������ � ��������� �����������, 2010
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
*   ��������� �������, ����������� ����
*
****************************************************************************************/

static bool IsStartOfGeneralizedSyllable(
	__in LPCWSTR pwsText,
	__in DWORD cchText);

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
*   ����������
*
*   ���������
*       ���
*
*   ������������ ��������
*       ���
*
*   ����������� ��� ���������� �������.
*
****************************************************************************************/

MidiSong::~MidiSong()
{
	Free();
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
*   ������������� �� ������ ����� ������� ������� ������������������ �������� �������,
*   ������������������ ������ � ����� ������.
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
*   ����� AddSingingEvent
*
*   ���������
*       NoteNumber - ����� ����
*       ctToNoteOn - ���������� ����� �� ������ ����� �� ������ ����
*       ctDuration - ������������ ���� � �����
*		pwsNoteText - ��������� ������, �� ����������� ����, �������������� �����
*                     �������� ���� �����, ����������� � ����; ���� �������� ����� ����
*                     ����� NULL
*       cchNoteText - ���������� �������� � ������ pwsNoteText
*
*   ������������ ��������
*       true, ���� �������� ������� ������� ��������� � ���; false, ���� �� �������
*       �������� ������.
*
*   ��������� � ��� �������� �������, ����������� ����������� NoteNumber, ctToNoteOn,
*   ctDuration, pwsNoteText � cchNoteText.
*
****************************************************************************************/

bool MidiSong::AddSingingEvent(
	__in DWORD NoteNumber,
	__in DWORD ctToNoteOn,
	__in DWORD ctDuration,
	__in_opt LPCWSTR pwsNoteText,
	__in DWORD cchNoteText)
{
	// �������� ������ ��� ������ �������� ������ �������� �������
	SINGING_EVENT *pNewSingingEvent = new SINGING_EVENT;

	if (pNewSingingEvent == NULL)
	{
		LOG("operator new failed\n");
		return false;
	}

	// ��������� �� �����, � ������� ����� ���������� ����� ������ ��������� �������
	LPWSTR pwsNewNoteText = NULL;

	if (pwsNoteText != NULL && cchNoteText > 0)
	{
		// �������� ������ ��� ������ ������ ��������� �������
		pwsNewNoteText = new WCHAR[cchNoteText];

		if (pwsNewNoteText == NULL)
		{
			LOG("operator new failed\n");
			delete pNewSingingEvent;
			return false;
		}

		// �������� ����� ������ ��������� ������� � ������ ��� ���������� �����
		wcsncpy(pwsNewNoteText, pwsNoteText, cchNoteText);
	}

	// �������������� ����� ������� ������ �������� �������
	pNewSingingEvent->NoteNumber = NoteNumber;
	pNewSingingEvent->ctToNoteOn = ctToNoteOn;
	pNewSingingEvent->ctDuration = ctDuration;
	pNewSingingEvent->pwsNoteText = pwsNewNoteText;
	pNewSingingEvent->cchNoteText = cchNoteText;
	pNewSingingEvent->pNext = NULL;

	if (m_pFirstSingingEvent == NULL)
	{
		// � ������ �������� ������� ��� ��� �� ������ ��������
		m_pFirstSingingEvent = pNewSingingEvent;
	}
	else
	{
		// � ������ �������� ������� ���� ��� ������� ���� �������
		m_pLastSingingEvent->pNext = pNewSingingEvent;
	}

	// ��������� ��������� �� ��������� ������� ������
	m_pLastSingingEvent = pNewSingingEvent;

	return true;
}

/****************************************************************************************
*
*   ����� GetNextSingingEvent
*
*   ���������
*       pNoteNumber - ��������� �� ����������, � ������� ����� ������� ����� ����; ����
*                     �������� ����� ���� ����� NULL
*       pctToNoteOn - ��������� �� ����������, � ������� ����� �������� ���������� �����
*                     �� ������ ����� �� ������ ����; ���� �������� ����� ���� ����� NULL
*       pctDuration - ��������� �� ����������, � ������� ����� �������� ������������ ����
*                     � �����; ���� �������� ����� ���� ����� NULL
*       ppwsNoteText - ��������� �� ����������, � ������� ����� ������� ��������� ������,
*                      �� ����������� ����, �������������� ����� �������� ���� �����,
*                      ����������� � ����; ���� �������� ����� ���� ����� NULL
*       pcchNoteText - ��������� �� ����������, � ������� ����� �������� ����������
*                      �������� �� ��������� ���� �����, ����������� � ����; ����
*                      �������� ����� ���� ����� NULL
*
*   ������������ ��������
*       true, ���� ���������� �� ��������� �������� ������� ������� ����������;
*       false, ���� �������� ������� ���������.
*
*   ���������� ���������� �� ��������� �������� ������� �� ���. ����� ������ ������
*   ResetCurrentPosition ���� ����� ���������� ���������� � ������ �������� ������� ��
*   ���.
*
****************************************************************************************/

bool MidiSong::GetNextSingingEvent(
	__out_opt DWORD *pNoteNumber,
	__out_opt DWORD *pctToNoteOn,
	__out_opt DWORD *pctDuration,
	__out_opt LPCWSTR *ppwsNoteText,
	__out_opt DWORD *pcchNoteText)
{
	// ���� �������� ������� ���������, ���������� false
	if (m_pCurSingingEvent == NULL) return false;

	if (pNoteNumber != NULL) *pNoteNumber = m_pCurSingingEvent->NoteNumber;
	if (pctToNoteOn != NULL) *pctToNoteOn = m_pCurSingingEvent->ctToNoteOn;
	if (pctDuration != NULL) *pctDuration = m_pCurSingingEvent->ctDuration;
	if (ppwsNoteText != NULL) *ppwsNoteText = m_pCurSingingEvent->pwsNoteText;
	if (pcchNoteText != NULL) *pcchNoteText = m_pCurSingingEvent->cchNoteText;

	// ��������� � ���������� ��������� �������
	m_pCurSingingEvent = m_pCurSingingEvent->pNext;

	return true;
}

/****************************************************************************************
*
*   ����� AddMeasure
*
*   ���������
*       Numerator - ��������� ������������ �������
*       Denominator - ����������� ������������ �������
*       cNotated32ndNotesPerMidiQuarterNote - ���������� �������������� ��� � ����������
*                                             MIDI-����
*
*   ������������ ��������
*       true, ���� ���� ������� ��������; false, ���� �� ������� �������� ������.
*
*   ��������� ����, ����������� ����������� Numerator, Denominator �
*   cNotated32ndNotesPerMidiQuarterNote, � ������������������ ������.
*
****************************************************************************************/

bool MidiSong::AddMeasure(
	__in DWORD Numerator,
	__in DWORD Denominator,
	__in DWORD cNotated32ndNotesPerMidiQuarterNote)
{
	// �������� ������ ��� ������ �������� ������ ������
	MEASURE *pNewMeasure = new MEASURE;

	if (pNewMeasure == NULL)
	{
		LOG("operator new failed\n");
		return false;
	}

	// �������������� ����� ������� ������ ������
	pNewMeasure->Numerator = Numerator;
	pNewMeasure->Denominator = Denominator;
	pNewMeasure->cNotated32ndNotesPerMidiQuarterNote = cNotated32ndNotesPerMidiQuarterNote;
	pNewMeasure->pNext = NULL;

	if (m_pFirstMeasure == NULL)
	{
		// � ������ ������ ��� ��� �� ������ ��������
		m_pFirstMeasure = pNewMeasure;
	}
	else
	{
		// � ������ ������ ���� ��� ������� ���� �������
		m_pLastMeasure->pNext = pNewMeasure;
	}

	// ��������� ��������� �� ��������� ������� ������
	m_pLastMeasure = pNewMeasure;

	return true;
}

/****************************************************************************************
*
*   ����� GetNextMeasure
*
*   ���������
*       pNumerator - ��������� �� ����������, � ������� ����� ������� ���������
*                    ������������ �������
*       pDenominator - ��������� �� ����������, � ������� ����� ������� �����������
*                      ������������ �������
*       pcNotated32ndNotesPerMidiQuarterNote - ��������� �� ����������, � ������� �����
*                                              �������� ���������� �������������� ���
*                                              � ���������� MIDI-����
*
*   ������������ ��������
*       true, ���� ���������� �� ��������� ����� ������� ����������;
*       false, ���� ����� ���������.
*
*   ���������� ���������� �� ��������� ����� �� ������������������ ������. ����� ������
*   ������ ResetCurrentPosition ���� ����� ���������� ���������� � ������ ����� ��
*   ������������������ ������.
*
****************************************************************************************/

bool MidiSong::GetNextMeasure(
	__out DWORD *pNumerator,
	__out DWORD *pDenominator,
	__out DWORD *pcNotated32ndNotesPerMidiQuarterNote)
{
	// ���� ����� ���������, ���������� false
	if (m_pCurMeasure == NULL) return false;

	*pNumerator = m_pCurMeasure->Numerator;
	*pDenominator = m_pCurMeasure->Denominator;
	*pcNotated32ndNotesPerMidiQuarterNote =
		m_pCurMeasure->cNotated32ndNotesPerMidiQuarterNote;

	// ��������� � ���������� �����
	m_pCurMeasure = m_pCurMeasure->pNext;

	return true;
}

/****************************************************************************************
*
*   ����� AddTempo
*
*   ���������
*       ctToTempoSet - ���������� ����� �� ������ ����� �� ������� ��������� �����
*       cMicrosecondsPerMidiQuarterNote - ������������ ���������� MIDI-����
*                                         � �������������
*
*   ������������ ��������
*       true, ���� ���� ������� ��������; false, ���� �� ������� �������� ������.
*
*   ��������� ����, ����������� ����������� ctToTempoSet �
*   cMicrosecondsPerMidiQuarterNote, � ����� ������.
*
****************************************************************************************/

bool MidiSong::AddTempo(
	__in DWORD ctToTempoSet,
	__in DWORD cMicrosecondsPerMidiQuarterNote)
{
	// �������� ������ ��� ������ �������� ������ ������
	TEMPO *pNewTempo = new TEMPO;

	if (pNewTempo == NULL)
	{
		LOG("operator new failed\n");
		return false;
	}

	// �������������� ����� ������� ������ ������
	pNewTempo->ctToTempoSet = ctToTempoSet;
	pNewTempo->cMicrosecondsPerMidiQuarterNote = cMicrosecondsPerMidiQuarterNote;
	pNewTempo->pNext = NULL;

	if (m_pFirstTempo == NULL)
	{
		// � ������ ������ ��� ��� �� ������ ��������
		m_pFirstTempo = pNewTempo;
	}
	else
	{
		// � ������ ������ ���� ��� ������� ���� �������
		m_pLastTempo->pNext = pNewTempo;
	}

	// ��������� ��������� �� ��������� ������� ������
	m_pLastTempo = pNewTempo;

	return true;
}

/****************************************************************************************
*
*   ����� GetNextTempo
*
*   ���������
*       pctToTempoSet - ��������� �� ����������, � ������� ����� �������� ����������
*                       ����� �� ������ ����� �� ������� ��������� �����
*       pcMicrosecondsPerMidiQuarterNote - ��������� �� ����������, � ������� �����
*                                          �������� ������������ ���������� MIDI-����
*                                          � �������������
*
*   ������������ ��������
*       true, ���� ���������� �� ��������� ����� ������� ����������;
*       false, ���� ����� ���������.
*
*   ���������� ���������� �� ��������� ����� �� ����� ������. ����� ������ ������
*   ResetCurrentPosition ���� ����� ���������� ���������� � ������ ����� �� ����� ������.
*
****************************************************************************************/

bool MidiSong::GetNextTempo(
	__out DWORD *pctToTempoSet,
	__out DWORD *pcMicrosecondsPerMidiQuarterNote)
{
	// ���� ����� ���������, ���������� false
	if (m_pCurTempo == NULL) return false;

	*pctToTempoSet = m_pCurTempo->ctToTempoSet;
	*pcMicrosecondsPerMidiQuarterNote = m_pCurTempo->cMicrosecondsPerMidiQuarterNote;

	// ��������� � ���������� �����
	m_pCurTempo = m_pCurTempo->pNext;

	return true;
}

/****************************************************************************************
*
*   ����� CorrectLyric
*
*   ���������
*       ���
*
*   ������������ ��������
*       true, ���� ����� ���� ����� ������� ��������������; false, ���� �� �������
*       �������� ������.
*
*   ������������ ����� ���� ����� � ���. ��������� ����������� � ������������ ���� �
*   �������� ���������� ��������.
*
****************************************************************************************/

bool MidiSong::CorrectLyric()
{
	// ����������� ����� � ���
	if (!AlignLyric())
	{
		LOG("MidiSong::AlignLyric failed\n");
		return false;
	}

	// ������� ���������� ������� �� ���� ����� � ���
	if (!RemoveNonPrintableSymbols())
	{
		LOG("MidiSong::RemoveNonPrintableSymbols failed\n");
		return false;
	}

	return true;
}

/****************************************************************************************
*
*   ����� CreateSong
*
*   ���������
*       cTicksPerMidiQuarterNote - ���������� �����, ������������ �� ���������� MIDI ����
*
*   ������������ ��������
*       ��������� �� ������ ������ Song, ��� NULL, ���� �� ������� �������� ������.
*
*   C����� ������ ������ Song, �������������� ����� �����.
*
****************************************************************************************/

Song *MidiSong::CreateSong(
	__in DWORD cTicksPerMidiQuarterNote)
{
	// ������ ������ ������ Song
	Song *pSong = new Song;

	if (pSong == NULL)
	{
		LOG("operator new failed\n");
		return NULL;
	}

	// ������ ��� � ������� ������ Song
	if (!CreateSes(pSong, cTicksPerMidiQuarterNote))
	{
		LOG("MidiSong::CreateSes failed\n");
		delete pSong;
		return NULL;
	}

	// ������ ������������������ ������ � ������� ������ Song
	if (!CreateMeasureSequence(pSong))
	{
		LOG("MidiSong::CreateMeasureSequence failed\n");
		delete pSong;
		return NULL;
	}

	// ������ ����� ������ � ������� ������ Song
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
*   ����� Free
*
*   ���������
*       ���
*
*   ������������ ��������
*       ���
*
*   ����������� ��� ���������� �������.
*
****************************************************************************************/

void MidiSong::Free()
{
	// ������� ������ �������� �������

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

	// ������� ������ ������

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

	// ������� ������ ������

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
*   ����� AlignLyric
*
*   ���������
*       ���
*
*   ������������ ��������
*       true, ���� ����� ����� ������� ���������; false, ���� �� ������� �������� ������.
*
*   ����������� ����� ����� � ���. � �������� ������������ ����� ��������� �����������
*   ����������� �������� �� ������ ������� ��������� ������� � ������� � �����
*   ����������� ��������� ������� � �������.
*
*   ��������� ������, �������������� ���� �������, ���������� �������� ������ ���������
*   ���� �����.
*
****************************************************************************************/

bool MidiSong::AlignLyric()
{
	// ��������� �� ������� ������� ������ �������� �������
	SINGING_EVENT *pCurSingingEvent = m_pFirstSingingEvent;

	// ��������� �� ������� ������ �������� �������, �� ������� ������� ��������;
	// ���� ���� ��������� ����� NULL, �� �������� �� �������;
	// ���� ���� ��������� �� ����� NULL, �� ����������� ������� ������ �����������
	// � ����� ������ pwsAlignedText
	SINGING_EVENT *pBookmarkedSingingEvent = NULL;

	// �����, � ������� ����� ������������� ����� ��������� �������,
	// �� ������� ������� ��������
	WCHAR pwsAlignedText[MAX_SYMBOLS_PER_NOTE];

	// ����������, � ������� ����� ��������� ������� ���������� ��������
	// � ������ AlignedText
	DWORD cchAlignedText;

	// ���� �� �������� ��������;
	// ����������� ����� � ������� �������
	while (true)
	{
		if (pCurSingingEvent == NULL)
		{
			// ��� �������� ������� ���������

			if (pBookmarkedSingingEvent != NULL)
			{
				// ������������� ����� � �������, �� ������� ������� ��������
				if (!SetText(pBookmarkedSingingEvent, pwsAlignedText, cchAlignedText))
				{
					LOG("MidiSong::SetText failed\n");
					return false;
				}
			}

			return true;
		}

		// ��������� �� �������� ���� �����, ����������� � ����
		LPCWSTR pwsNoteText = pCurSingingEvent->pwsNoteText;

		// ���������� �������� �� ��������� ���� �����
		DWORD cchNoteText = pCurSingingEvent->cchNoteText;

		if (pwsNoteText == NULL)
		{
			// ���������� �������� ������� ��� ������
			pCurSingingEvent = pCurSingingEvent->pNext;
			continue;
		}

		// ������ ������� � ������ pwsNoteText
		DWORD i = 0;

		// ���� �� �������� ������ �������� ��������� �������
		while (true)
		{
			if (IsStartOfGeneralizedSyllable(pwsNoteText + i, cchNoteText - i))
			{
				// ��������� ������ ������ ����������� �����

				if (pBookmarkedSingingEvent != NULL)
				{
					// ���� �������� �������, �� ������� ���� ������� ��������

					// ������������� ����� � �������, �� ������� ������� ��������
					if (!SetText(pBookmarkedSingingEvent, pwsAlignedText, cchAlignedText))
					{
						LOG("MidiSong::SetText failed\n");
						return false;
					}
				}

				// ������� �� �����
				break;
			}

			// ���� �� �����-���� �������� ������� ������� ��������, �� ���������
			// ��������� ����������� ������ � ����� ������ pwsAlignedText
			if (pBookmarkedSingingEvent != NULL)
			{
				pwsAlignedText[cchAlignedText] = pwsNoteText[i];
				cchAlignedText++;
			}

			// ��������� � ���������� ������� ������ �������� ��������� �������
			i++;

			if (cchAlignedText == MAX_SYMBOLS_PER_NOTE)
			{
				// ����� �� ����� ������

				if (pBookmarkedSingingEvent != NULL)
				{
					// ������������� ����� � �������, �� ������� ������� ��������
					if (!SetText(pBookmarkedSingingEvent, pwsAlignedText, cchAlignedText))
					{
						LOG("MidiSong::SetText failed\n");
						return false;
					}
				}

				// ���� ��������� ���������� ����������� ������� � ������ ������
				// �������� ��������� �������, ���� �� ����� �� ������ ������ �����
				while (i < cchNoteText)
				{
					if (IsStartOfGeneralizedSyllable(pwsNoteText + i, cchNoteText - i))
					{
						break;
					}
				}

				// ������� �� �����
				break;
			}

			// ���� ����� �� ����� ������ �������� ��������� �������, �� ������� �� �����
			if (i == cchNoteText) break;
		}

		if (i == cchNoteText)
		{
			// � ������ �������� ��������� ������� ��� �� ����� �����, ������� ���� �����
			SetText(pCurSingingEvent, NULL, 0);

			if (cchAlignedText == MAX_SYMBOLS_PER_NOTE)
			{
				// �� ����� �� ����������� ����� �� ������������ ������; ��� ��������,
				// ��� ����� ��������� �������, �� ������� ������ ��������, ����������;
				// � ��������� � ������� �������� ������� �� �������� ��������,
				// �������� �� �� ������� ������
				pBookmarkedSingingEvent = NULL;
			}
		}
		else
		{
			// �������� ����� �������� ��������� ������� � ����� pwsAlignedText,
			// ������� � ������ �����
			cchAlignedText = cchNoteText - i;
			wcsncpy(pwsAlignedText, pwsNoteText + i, cchAlignedText);

			// ������ �������� �� ������� �������� �������
			pBookmarkedSingingEvent = pCurSingingEvent;
		}

		// ��������� � ���������� ��������� �������
		pCurSingingEvent = pCurSingingEvent->pNext;
	}
}

/****************************************************************************************
*
*   ����� RemoveNonPrintableSymbols
*
*   ���������
*       ���
*
*   ������������ ��������
*       true, ���� ���������� ������� ������� ������� �� ���� �����; false, ���� ��
*       ������� �������� ������.
*
*   ������� ���������� ������� �� ���� ����� � ���, ������������ ������� AlignLyric.
*
*   ��������� ������, �������������� ���� �������, ���������� ����� ������ ���������
*   ���� �����.
*
****************************************************************************************/

bool MidiSong::RemoveNonPrintableSymbols()
{
	// ��������� �� ������� ������� ������ �������� �������
	SINGING_EVENT *pCurSingingEvent = m_pFirstSingingEvent;

	// ���� �� �������� ��������;
	// ������������ ����� ������� �������
	while (true)
	{
		// ���� �������� ������� ���������, �� �������
		if (pCurSingingEvent == NULL) return true;

		// ��������� �� �������� ���� �����, ����������� � ����
		LPCWSTR pwsNoteText = pCurSingingEvent->pwsNoteText;

		// ���������� �������� �� ��������� ���� �����
		DWORD cchNoteText = pCurSingingEvent->cchNoteText;

		if (pwsNoteText == NULL)
		{
			// ���������� �������� ������� ��� ������
			pCurSingingEvent = pCurSingingEvent->pNext;
			continue;
		}

		// �����, � ������� ����� ����������� ����� ����� �������� ��������� �������
		WCHAR pwsNewNoteText[MAX_SYMBOLS_PER_NOTE];

		// ���������� �������� � ������ pwsNewNoteText
		DWORD cchNewNoteText = cchNoteText;

		// �������� � ����� pwsNewNoteText ����� �������� ��������� �������
		wcsncpy(pwsNewNoteText, pwsNoteText, cchNoteText);

		// �������� ������� '/', '\', 0x00-0x1F �� �������
		for (DWORD j = 0; j < cchNewNoteText; j++)
		{
			if (pwsNewNoteText[j] == L'/' || pwsNewNoteText[j] == L'\\' ||
				pwsNewNoteText[j] < L' ')
			{
				pwsNewNoteText[j] = L' ';
			}
		}

		// ������� ������ �������, ������� ����� ����������
		// (������ ������ - ������ �� ������, ������� ����� ���������� ���)
		DWORD i = 1;

		// �������� ������������������ �������� ����� ��������
		// (������ ������ - ������ �� ������, ������� ����� ���������� ���)
		for (DWORD j = 1; j < cchNewNoteText; j++)
		{
			// �� �������� ������, ���� �� ����� ���� ��� ����������� ���� ������
			if (pwsNewNoteText[i-1] == L' ' && pwsNewNoteText[j] == L' ') continue;

			pwsNewNoteText[i++] = pwsNewNoteText[j];
		}

		// ��������� ���������� �������� � ������ pwsNewNoteText
		cchNewNoteText = i;

		// ������������� �������������� ����� � �������� ��������� �������
		if (!SetText(pCurSingingEvent, pwsNewNoteText, cchNewNoteText))
		{
			LOG("MidiSong::SetText failed\n");
			return false;
		}

		// ��������� � ���������� ��������� �������
		pCurSingingEvent = pCurSingingEvent->pNext;
	}
}

/****************************************************************************************
*
*   ����� SetText
*
*   ���������
*       pSingingEvent - ��������� �� ������� ������ �������� �������
*		pwsNoteText - ��������� ������, �� ����������� ����, �������������� �����
*                     �������� ���� �����, ����������� � ����; ���� �������� ����� ����
*                     ����� NULL
*       cchNoteText - ���������� �������� � ������ pwsNoteText
*
*   ������������ ��������
*       true, ���� ����� ����� ��������� ������� ������� ����������; false, ���� ��
*       ������� �������� ������.
*
*   ������������� ����� ����� ��������� �������, ��������� ���������� pSingingEvent.
*
****************************************************************************************/

bool MidiSong::SetText(
	__inout SINGING_EVENT *pSingingEvent,
	__in_opt LPCWSTR pwsNoteText,
	__in DWORD cchNoteText)
{
	// ��������� �� �����, � ������� ����� ���������� ����� ����� ��������� �������
	LPWSTR pwsNewNoteText = NULL;

	if (pwsNoteText != NULL && cchNoteText > 0)
	{
		// �������� ������ ��� ������ ������ ��������� �������
		pwsNewNoteText = new WCHAR[cchNoteText];

		if (pwsNewNoteText == NULL)
		{
			LOG("operator new failed\n");
			return false;
		}

		// �������� ����� ����� ��������� ������� � ������ ��� ���������� �����
		wcsncpy(pwsNewNoteText, pwsNoteText, cchNoteText);
	}

	// ������� ������ ����� ��������� �������
	if (pSingingEvent->pwsNoteText != NULL) delete pSingingEvent->pwsNoteText;

	// ��������� ���� �������� ������ �������� �������
	pSingingEvent->pwsNoteText = pwsNewNoteText;
	pSingingEvent->cchNoteText = cchNoteText;

	return true;
}

/****************************************************************************************
*
*   ����� CreateSes
*
*   ���������
*       pSong - ��������� �� ������ ������ Song, � ������� ���� ������� ���
*       cTicksPerMidiQuarterNote - ���������� �����, ������������ �� ���������� MIDI ����
*
*   ������������ ��������
*       true � ������ ������; false, ���� �� ������� �������� ������.
*
*   C����� ��� � ������� ������ Song.
*
****************************************************************************************/

bool MidiSong::CreateSes(
	__inout Song *pSong,
	__in DWORD cTicksPerMidiQuarterNote)
{
	// ��������� �� ������� ������� ������ �������� �������
	SINGING_EVENT *pCurSingingEvent = m_pFirstSingingEvent;

	// ��������� �� ������� ������� ������ ������
	MEASURE *pCurMeasure = m_pFirstMeasure;

	// ����������, ���������� ������������� �����;
	// ���������� ����� �� ������ ����� �� �������� �����
	double ctTotal = 0;

	// ����������, ���������� ������������� ����� ���;
    // ���������� ����� ��� �� ������ ����� �� �������� �����
	double cwnTotal = 0;

	// ���������� ����� ��� �� ������ ����� �� ���������� ���� ����������� ���������
    // �������
	double cwnToPrevNoteOff = 0;

	// ���� �� �������� ��������;
	// ������ ��� � ������� ������ Song
	while (true)
	{
		// ���� ��� �������� ������� ���������, �� �������� ��� ��������
		if (pCurSingingEvent == NULL) return true;

		// ������������ �������� ����� � �����
		double ctPerCurMeasure;

		// ������������ �������� ����� � ����� �����
		double cwnPerCurMeasure;

		// ����� �� ������, ���� �� ����� ����, �� ������� ���������� ��������� ����
        // �������� ��������� �������
		while (true)
		{
			// ��������� ������������ �������� ����� � �����
			ctPerCurMeasure = (double) 32 * pCurMeasure->Numerator *
				cTicksPerMidiQuarterNote / (pCurMeasure->Denominator *
				pCurMeasure->cNotated32ndNotesPerMidiQuarterNote);

			// ��������� ������������ �������� ����� � ����� �����
			cwnPerCurMeasure = (double) pCurMeasure->Numerator / pCurMeasure->Denominator;

			if (ctTotal + ctPerCurMeasure >= pCurSingingEvent->ctToNoteOn)
			{
				// ����, �� ������� ���������� ��������� ���� �������� ��������� �������,
                // ������, ������� ������� �� �����
				break;
			}

			ctTotal += ctPerCurMeasure;
			cwnTotal += cwnPerCurMeasure;

			// ��������� � ���������� �����
			pCurMeasure = pCurMeasure->pNext;
		}

		// ������� ���������� ����� ��� �� ������ ����� �� ��������� ���� ��������
		// ��������� �������
		double cwnToNoteOn = cwnTotal + (pCurSingingEvent->ctToNoteOn - ctTotal) *
			cwnPerCurMeasure / ctPerCurMeasure;

		// ����������, �������� ���������� ����� �� ������ ����� �� ���������� ����
        // �������� ��������� �������
        DWORD ctToNoteOff = pCurSingingEvent->ctToNoteOn + pCurSingingEvent->ctDuration;

		// ����� �� ������, ���� �� ����� ����, �� ������� ���������� ���������� ����
        // �������� ��������� �������
		while (true)
		{
			// ��������� ������������ �������� ����� � �����
			ctPerCurMeasure = (double) 32 * pCurMeasure->Numerator *
				cTicksPerMidiQuarterNote / (pCurMeasure->Denominator *
				pCurMeasure->cNotated32ndNotesPerMidiQuarterNote);

			// ��������� ������������ �������� ����� � ����� �����
			cwnPerCurMeasure = (double) pCurMeasure->Numerator / pCurMeasure->Denominator;

			if (ctTotal + ctPerCurMeasure >= ctToNoteOff)
			{
				// ����, �� ������� ���������� ���������� ���� �������� ��������� �������,
                // ������, ������� ������� �� �����
				break;
			}

			ctTotal += ctPerCurMeasure;
			cwnTotal += cwnPerCurMeasure;

			// ��������� � ���������� �����
			pCurMeasure = pCurMeasure->pNext;
		}

		// ������� ���������� ����� ��� �� ������ ����� �� ���������� ���� ��������
		// ��������� �������
		double cwnToNoteOff = cwnTotal + (ctToNoteOff - ctTotal) *
			cwnPerCurMeasure / ctPerCurMeasure;

		// ������� ������������ ���� �������� ��������� ������� � ����� �����
		double cwnPerNote = cwnToNoteOff - cwnToNoteOn;

		// ������������ ����� ����� ���������� ����� � ����� �������� ��������� �������
		// � ����� �����
		double cwnPerPause = cwnToNoteOn - cwnToPrevNoteOff;

		// ��������� � ��� ��������� �������� �������
		bool bEventAdded = pSong->AddSingingEvent(pCurSingingEvent->NoteNumber,
			cwnPerPause, cwnPerNote, pCurSingingEvent->pwsNoteText,
			pCurSingingEvent->cchNoteText);

		if (!bEventAdded)
		{
			LOG("Song::AddSingingEvent failed\n");
			return false;
		}

		cwnToPrevNoteOff = cwnToNoteOff;

		// ��������� � ���������� ��������� �������
		pCurSingingEvent = pCurSingingEvent->pNext;
	}
}

/****************************************************************************************
*
*   ����� CreateMeasureSequence
*
*   ���������
*       pSong - ��������� �� ������ ������ Song
*
*   ������������ ��������
*       true, ���� ������������������ ������ ������� �������; false, ���� �� �������
*       �������� ������.
*
*   ������ ������������������ ������ � ������� ������ Song.
*
****************************************************************************************/

bool MidiSong::CreateMeasureSequence(
	__inout Song *pSong)
{
	// ��������� �� ������� ������� ������ ������
	MEASURE *pCurMeasure = m_pFirstMeasure;

	// ���� �� ������;
	// ������ ������������������ ������ � ������� ������ Song
	while (true)
	{
		// ���� ��� ����� ���������, �� �������
		if (pCurMeasure == NULL) return true;

		if (!pSong->AddMeasure(pCurMeasure->Numerator, pCurMeasure->Denominator))
		{
			LOG("Song::AddMeasure failed\n");
			return false;
		}

		// ��������� � ���������� �����
		pCurMeasure = pCurMeasure->pNext;
	}
}

/****************************************************************************************
*
*   ����� CreateTempoMap
*
*   ���������
*       pSong - ��������� �� ������ ������ Song
*       cTicksPerMidiQuarterNote - ���������� �����, ������������ �� ���������� MIDI ����
*
*   ������������ ��������
*       true, ���� ����� ������ ������� �������; false, ���� �� ������� �������� ������.
*
*   ������ ����� ������ � ������� ������ Song.
*
****************************************************************************************/

bool MidiSong::CreateTempoMap(
	__inout Song *pSong,
	__in DWORD cTicksPerMidiQuarterNote)
{
	// ��������� �� ������� ������� ������ ������
	TEMPO *pCurTempo = m_pFirstTempo;

	// ��������� �� ������� ������� ������ ������
	MEASURE *pCurMeasure = m_pFirstMeasure;

	// ����������, ���������� ������������� �����;
	// ���������� ����� �� ������ ����� �� �������� �����
	double ctTotal = 0;

	// ����������, ���������� ������������� ����� ���;
    // ���������� ����� ��� �� ������ ����� �� �������� �����
	double cwnTotal = 0;

	// ���� �� ������;
	// ������ ����� ������ � ������� ������ Song
	while (true)
	{
		// ���� ��� ����� ���������, �� �������� ����� ������ ��������
		if (pCurTempo == NULL) return true;

		// ������������ �������� ����� � �����
		double ctPerCurMeasure;

		// ������������ �������� ����� � ����� �����
		double cwnPerCurMeasure;

		// ����� �� ������, ���� �� ����� ����, � ������� ���������� ��������� ��������
		// �����
		while (true)
		{
			// ��������� ������������ �������� ����� � �����
			ctPerCurMeasure = (double) 32 * pCurMeasure->Numerator *
				cTicksPerMidiQuarterNote / (pCurMeasure->Denominator *
				pCurMeasure->cNotated32ndNotesPerMidiQuarterNote);

			// ��������� ������������ �������� ����� � ����� �����
			cwnPerCurMeasure = (double) pCurMeasure->Numerator / pCurMeasure->Denominator;

			if (ctTotal + ctPerCurMeasure >= pCurTempo->ctToTempoSet)
			{
				// ����, � ������� ���������� ��������� �������� �����,
                // ������, ������� ������� �� �����
				break;
			}

			ctTotal += ctPerCurMeasure;
			cwnTotal += cwnPerCurMeasure;

			// ��������� � ���������� �����
			pCurMeasure = pCurMeasure->pNext;

			// ���� ����� ���������, �� �������
			if (pCurMeasure == NULL) return true;
		}

		// ������� ���������� ����� ��� �� ������ ����� �� ��������� �������� �����
		double cwnToTempoSet = cwnTotal + (pCurTempo->ctToTempoSet - ctTotal) *
			cwnPerCurMeasure / ctPerCurMeasure;

		double BPM = ((double) pCurMeasure->cNotated32ndNotesPerMidiQuarterNote / 8) *
			((double) 60000000 / pCurTempo->cMicrosecondsPerMidiQuarterNote);

		// ��������� � ����� ������ ��������� ����
		if (!pSong->AddTempo(cwnToTempoSet, BPM))
		{
			LOG("Song::AddTempo failed\n");
			return false;
		}

		// ��������� � ���������� �����
		pCurTempo = pCurTempo->pNext;
	}
}

/****************************************************************************************
*
*   ������� IsStartOfSyllable
*
*   ���������
*		pwsText - ��������� ������, �� ����������� ����, �������������� ����� ��������
*                 ������ ��������� �������
*       cchText - ���������� �������� � ������ pwsText; �������� ����� ��������� ������
*                 ���� ������ ����
*
*   ������������ ��������
*       true, ���� ����� ���������� � ������ ����������� �����; ����� false.
*
*   ���������, ���������� �� �����, �������� ����������� pwsText � cchText, � ������
*   ����������� �����.
*
****************************************************************************************/

static bool IsStartOfGeneralizedSyllable(
	__in LPCWSTR pwsText,
	__in DWORD cchText)
{
	// ���� ������ ������ ����� - �����, �� ��� ������ ����������� �����
	if (IsCharAlphaW(pwsText[0])) return true;

	// ���� ������ ������ - ������������� ������� ������, �� ��� ������ ����������� �����
	if (pwsText[0] == L'(') return true;

	// ���� ������ ������ - ��������, �� ������� ������� �����,
	// �� ��� ������ ����������� �����
	if (pwsText[0] == L'\'' && cchText > 1 && IsCharAlphaW(pwsText[1])) return true;

	return false;
}
