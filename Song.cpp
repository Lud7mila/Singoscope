/****************************************************************************************
*
*   ����������� ������ Song
*
*   ������ ����� ������ ������������ ����� �����. ����� �����������:
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
*   ����������
*
*   ���������
*       ���
*
*   ������������ ��������
*       ���
*
*   ����������� ����������� ��� ���������� �������.
*
****************************************************************************************/

Song::~Song()
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

void Song::ResetCurrentPosition()
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
*       PauseLength - ������������ ����� ����� ���������� ����� � ������ ����� � �����
*                     �����
*       NoteLength - ������������ ���� � ����� �����
*		pwsNoteText - ��������� ������, �� ����������� ����, �������������� �����
*                     �������� ���� �����, ����������� � ����; ���� �������� ����� ����
*                     ����� NULL
*       cchNoteText - ���������� �������� � ������ pwsNoteText
*
*   ������������ ��������
*       true, ���� �������� ������� ������� ��������� � ���; false, ���� �� �������
*       �������� ������.
*
*   ��������� � ��� �������� �������, ����������� ����������� NoteNumber, PauseLength,
*   NoteLength, pwsNoteText � cchNoteText.
*
****************************************************************************************/

bool Song::AddSingingEvent(
	__in DWORD NoteNumber,
	__in double PauseLength,
	__in double NoteLength,
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
	pNewSingingEvent->PauseLength = PauseLength;
	pNewSingingEvent->NoteLength = NoteLength;
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
*       pPauseLength - ��������� �� ����������, � ������� ����� �������� ������������
*                      ����� ����� ���������� ����� � ������ ����� � ����� �����; ����
*                      �������� ����� ���� ����� NULL
*       pNoteLength - ��������� �� ����������, � ������� ����� �������� ������������ ����
*                     � ����� �����; ���� �������� ����� ���� ����� NULL
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

bool Song::GetNextSingingEvent(
	__out_opt DWORD *pNoteNumber,
	__out_opt double *pPauseLength,
	__out_opt double *pNoteLength,
	__out_opt LPCWSTR *ppwsNoteText,
	__out_opt DWORD *pcchNoteText)
{
	// ���� �������� ������� ���������, ���������� false
	if (m_pCurSingingEvent == NULL) return false;

	if (pNoteNumber != NULL) *pNoteNumber = m_pCurSingingEvent->NoteNumber;
	if (pPauseLength != NULL) *pPauseLength = m_pCurSingingEvent->PauseLength;
	if (pNoteLength != NULL) *pNoteLength = m_pCurSingingEvent->NoteLength;
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
*
*   ������������ ��������
*       true, ���� ���� ������� ��������; false, ���� �� ������� �������� ������.
*
*   ��������� ����, ����������� ����������� Numerator � Denominator,
*   � ������������������ ������.
*
****************************************************************************************/

bool Song::AddMeasure(
	__in DWORD Numerator,
	__in DWORD Denominator)
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

bool Song::GetNextMeasure(
	__out DWORD *pNumerator,
	__out DWORD *pDenominator)
{
	// ���� ����� ���������, ���������� false
	if (m_pCurMeasure == NULL) return false;

	*pNumerator = m_pCurMeasure->Numerator;
	*pDenominator = m_pCurMeasure->Denominator;

	// ��������� � ���������� �����
	m_pCurMeasure = m_pCurMeasure->pNext;

	return true;
}

/****************************************************************************************
*
*   ����� AddTempo
*
*   ���������
*       Offset - ���������� ����� ��� �� ������ ����� �� ������� ��������� ����� �����
*       BPM - ���� (���������� ���������� ��� � ������)
*
*   ������������ ��������
*       true, ���� ���� ������� ��������; false, ���� �� ������� �������� ������.
*
*   ��������� ����, ����������� ����������� Offset � BPM, � ����� ������.
*
****************************************************************************************/

bool Song::AddTempo(
	__in double Offset,
	__in double BPM)
{
	// �������� ������ ��� ������ �������� ������ ������
	TEMPO *pNewTempo = new TEMPO;

	if (pNewTempo == NULL)
	{
		LOG("operator new failed\n");
		return false;
	}

	// �������������� ����� ������� ������ ������
	pNewTempo->Offset = Offset;
	pNewTempo->BPM = BPM;
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
*       pOffset - ��������� �� ����������, � ������� ����� �������� ���������� ����� ���
*                 �� ������ ����� �� ������� ��������� ����� �����
*       pBPM - ��������� �� ����������, � ������� ����� ������� ����
*              (���������� ���������� ��� � ������)
*
*   ������������ ��������
*       true, ���� ���������� �� ��������� ����� ������� ����������;
*       false, ���� ����� ���������.
*
*   ���������� ���������� �� ��������� ����� �� ����� ������. ����� ������ ������
*   ResetCurrentPosition ���� ����� ���������� ���������� � ������ ����� �� ����� ������.
*
****************************************************************************************/

bool Song::GetNextTempo(
	__out double *pOffset,
	__out double *pBPM)
{
	// ���� ����� ���������, ���������� false
	if (m_pCurTempo == NULL) return false;

	*pOffset = m_pCurTempo->Offset;
	*pBPM = m_pCurTempo->BPM;

	// ��������� � ���������� �����
	m_pCurTempo = m_pCurTempo->pNext;

	return true;
}

/****************************************************************************************
*
*   ����� Quantize
*
*   ���������
*       StepDenominator - ����������� �����, ���������� ������� �������� �������,
*                         �������������� ����� ��� ����� �����������
*
*   ������������ ��������
*       true, ���� � �������� ����������� �� ������������ �� ����� ���� � �������
*       �������������; ����� false.
*
*   �������� ���� � ���.
*
****************************************************************************************/

bool Song::Quantize(
	__in DWORD StepDenominator)
{
	// ������������, ��� �� ���������� �� ����� ���� � ������� �������������
	bool bNoEmptyNotes = true;

	// ��� ����� �����������
	double Step = (double) 1 / StepDenominator;

	// ��������� �� ������� ������� ������ �������� �������
	SINGING_EVENT *pCurSingingEvent = m_pFirstSingingEvent;

	// ���������� ����� ��� �� ������ ����� �� ������ ��� ����� ������� ����
	double CurOffset = 0;

	// ���������� ����� ��� �� ������ ����� �� ����� ���������� �������������� ����
	double cwnToPrevQuantizedNoteOff = 0;

	// ���� �� �������� ��������; �������� ����
	while (true)
	{
		// ���� ��� �������� ������� ���������, �� ������� �� �����
		if (pCurSingingEvent == NULL) break;

		// ���������� ����� ��� �� ������ ����� �� ������ ������� ����
		CurOffset += pCurSingingEvent->PauseLength;

		// ���������� ����� ��� �� ������ ����� �� ������ �������������� ������� ����
		double cwnToQuantizedNoteOn = Step * (int) (CurOffset / Step);

		// ���������� ����� ��� �� ������ ����� �� ����� ������� ����
		CurOffset += pCurSingingEvent->NoteLength;

		// ���������� ����� ��� �� ������ ����� �� ����� �������������� ������� ����
		double cwnToQuantizedNoteOff = Step * (int) (CurOffset / Step);

		// ��������� ����� ��������
		pCurSingingEvent->PauseLength = cwnToQuantizedNoteOn - cwnToPrevQuantizedNoteOff;
		pCurSingingEvent->NoteLength = cwnToQuantizedNoteOff - cwnToQuantizedNoteOn;

		// ���� ������������ ���� � ������� �������������, ���������� ���
		if (pCurSingingEvent->NoteLength == 0) bNoEmptyNotes = false;

		// ������� ���� ���������� ����������
		cwnToPrevQuantizedNoteOff = cwnToQuantizedNoteOff;

		// ��������� � ���������� ��������� �������
		pCurSingingEvent = pCurSingingEvent->pNext;
	}

	return bNoEmptyNotes;
}

/****************************************************************************************
*
*   ����� ExtendEmptyNotes
*
*   ���������
*       LengthDenominator - ����������� �����, ���������� ������� �������� �������,
*                           �������������� ����� ����� ������������ ��� � �������
*                           �������������
*
*   ������������ ��������
*       true, ���� ��� ���� � ������� ������������� ���� ������� ���������; ����� false.
*
*   ����������� ���� � ������� ������������� � ��� ��������� �������: ���� ����� �����
*   �����, ��������� �� ����� � ������� �������������, ������ ��� �����
*   1/LengthDenominator, �� ��� ����� ����������� �� 1/LengthDenominator, � ������������
*   ���� � ������� ������������� �������� ������ 1/LengthDenominator.
*
****************************************************************************************/

bool Song::ExtendEmptyNotes(
	__in DWORD LengthDenominator)
{
	// ������������, ��� �� ��������� �� ����� ���� � ������� �������������
	bool bNoEmptyNotes = true;

	// ����� ������������ ��� � ������� �������������
	double Length = (double) 1 / LengthDenominator;

	// ��������� �� ������� ������� ������ �������� �������
	SINGING_EVENT *pCurSingingEvent = m_pFirstSingingEvent;

	// ���� �� �������� ��������; ����������� ���� � ������� �������������
	while (true)
	{
		// ���� ��� �������� ������� ���������, �� ������� �� �����
		if (pCurSingingEvent == NULL) break;

		// ��������� �� ��������� ������� ������ �������� �������
		SINGING_EVENT *pNextSingingEvent = pCurSingingEvent->pNext;

		if (pCurSingingEvent->NoteLength == 0)
		{
			// ������������ ������� ���� - �������

			if (pNextSingingEvent == NULL)
			{
				// ������� ���� - ��������� � ���, � ������ ����� ���������
				pCurSingingEvent->NoteLength = Length;
			}
			else
			{
				// ������� ���� - �� ��������� � ���

				if (pNextSingingEvent->PauseLength < Length)
				{
					// ������� ���� ��������� ����������
					bNoEmptyNotes = false;
				}
				else
				{
					// ����� ����� ��������� ����� ������ ��� ����� Length,
					// ��������� ��� ����� � ����������� ������� ����
					pNextSingingEvent->PauseLength -= Length;
					pCurSingingEvent->NoteLength = Length;
				}
			}
		}

		// ��������� � ���������� ��������� �������
		pCurSingingEvent = pNextSingingEvent;
	}

	return bNoEmptyNotes;
}

/****************************************************************************************
*
*   ����� HyphenateLyric
*
*   ���������
*       ���
*
*   ������������ ��������
*       true, ���� ������ ������� ���������� � ����� ������ ���� �����; false, ���� ��
*       ������� �������� ������.
*
*   ������ ������ � ����� ������ ���� ����� � ���, ������������ �������
*   RemoveNonPrintableSymbols.
*
*   ��������� ������, �������������� ���� �������, ���������� ������ ������ ���������
*   ���� �����.
*
****************************************************************************************/

bool Song::HyphenateLyric()
{
	// ��������� �� ������� ������� ������ �������� �������
	SINGING_EVENT *pCurSingingEvent = m_pFirstSingingEvent;

	// ��������� �� ������� ������ �������� �������, �� ������� ������� ��������;
	// ���� ���� ��������� ����� NULL, �� �������� �� �������
	SINGING_EVENT *pBookmarkedSingingEvent = NULL;

	// �����, � ������� ����� ��������� ����� ��������� �������, �� ������� �������
	// ��������; ��������� ��������� �������������� ����� � ����� ������
	WCHAR pwsBookmarkedText[MAX_SYMBOLS_PER_NOTE + 1];

	// ���������� �������� � ������ pwsBookmarkedText
	DWORD cchBookmarkedText;

	// ��������� �� ����� �������� ��������� �������
	LPCWSTR pwsNoteText;

	// ���������� �������� � ������ �������� ��������� �������
	DWORD cchNoteText;

	// ���� �� �������� ��������; ���� ������ �������� ������� � �������
	while (true)
	{
		// ���� ��� �������� ������� ���������, �� �������
		if (pCurSingingEvent == NULL) return true;

		pwsNoteText = pCurSingingEvent->pwsNoteText;
		cchNoteText = pCurSingingEvent->cchNoteText;

		// ���� ����� �������� ������� � �������, �� ������� �� �����
		if (pwsNoteText != NULL) break;

		// ��������� � ���������� ��������� �������
		pCurSingingEvent = pCurSingingEvent->pNext;
	}

	// ������ �������� �� ������ �������� ������� � �������
	pBookmarkedSingingEvent = pCurSingingEvent;

	// ��������� ����� ��������� �������, �� ������� ������� ��������
	wcsncpy(pwsBookmarkedText, pwsNoteText, cchNoteText);
	cchBookmarkedText = cchNoteText;

	// ���� �� �������� ��������; ������ ������ � ����� ������
	while (true)
	{
		// ��������� � ���������� ��������� �������
		pCurSingingEvent = pCurSingingEvent->pNext;

		// ���� ��� �������� ������� ���������, �� �������
		if (pCurSingingEvent == NULL) return true;

		pwsNoteText = pCurSingingEvent->pwsNoteText;
		cchNoteText = pCurSingingEvent->cchNoteText;

		// ���������� �������� ������� ��� ������
		if (pwsNoteText == NULL) continue;

		if (IsCharAlphaW(pwsBookmarkedText[cchBookmarkedText-1]) &&
			IsCharAlphaW(pwsNoteText[0]))
		{
			// ��������� ������ ������ ��������� �������, �� ������� �������
			// ��������, - �����, � ������ ������ ������ ������������ ��������� �������
			// � ������� - �����, ������ ���� ��������� �����
			pwsBookmarkedText[cchBookmarkedText++] = L'-';

			// ������������� ����� � �������, �� ������� ������� ��������
			if (!SetText(pBookmarkedSingingEvent, pwsBookmarkedText, cchBookmarkedText))
			{
				LOG("Song::SetText failed\n");
				return false;
			}
		}

		// ������ �������� �� ������� �������� ������� � �������
		pBookmarkedSingingEvent = pCurSingingEvent;

		// ��������� ����� ��������� �������, �� ������� ������� ��������
		wcsncpy(pwsBookmarkedText, pwsNoteText, cchNoteText);
		cchBookmarkedText = cchNoteText;
	}
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

void Song::Free()
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

bool Song::SetText(
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
*   ������� IsStartOfGeneralizedSyllable
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
