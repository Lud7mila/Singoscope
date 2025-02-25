/****************************************************************************************
*
*   ����������� ������ MidiFile
*
*   ������ ����� ������ ������������ ����� MIDI-����.
*
*   �����: ������� ������������ � ��������� �����������, 2008-2010
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
*   ���������
*
****************************************************************************************/

// ���� ���������� ���������� ����������� ���� TEXT_EVENT ������ ����� ������,
// �� �������, ��� ��� �� �������� ������� �����
#define MIN_TEXT_EVENTS_FOR_LYRIC_TRACK		5


// ��� �����, ������������ ��������, ���������� �� ��������� ������
// (����� ���� ������ ����������� � �������� ������ GetPartLyricDistance):

// �������� "���������� ������"
#define IDENTICAL_START						0x00000001

// �������� "���������� �����"
#define IDENTICAL_END						0x00000002

// �������� "������������ ���������� ��� �� ���� �����������"
#define LIMIT_NOTES_PER_METAEVENT			0x00000004


// ����������� ���������� ���������� ��� �� ���� �����������
#define MAX_NOTES_PER_METAEVENT				10

// ����������� ���������� �������� ���� �������� ������ � ���� ����� � ����������
// MIDI-����� ��� ��������� ������
#define MAX_VOCAL_PART_LYRIC_DISTANCE		500

/****************************************************************************************
*
*   ����������� �����
*
****************************************************************************************/

// ���������, ����������� �����������, ���������� �� ��������� ������ ��� �� ������;
// ������������ � ������ FindVocalParts
struct VOCAL_PART_LIMITATIONS {
	double OverlapsThreshold;	// ����������� ���������� ���� ���������� ��� �� �������
								// �� ��������� � ���������� ��������� ��� � ������
	DWORD fCriteria;	// ����� ������, ������������ �������� ������ ��������� ������
};

/****************************************************************************************
*
*   ���������� �������
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

MidiFile::~MidiFile()
{
	Free();
}

/****************************************************************************************
*
*   ����� SetDefaultCodePage
*
*   ���������
*       DefaultCodePage - ������� �������� �� ��������� ��� ���� �����
*
*   ������������ ��������
*       MIDIFILE_SUCCESS - ������� �������� �� ��������� ������� ����������� � �������
*                          ����� ����� � ���� �� ���� ��������� ������;
*       MIDIFILE_NO_LYRIC - ����� ����� �� �������;
*       MIDIFILE_NO_VOCAL_PARTS - �� ������� �� ���� ��������� ������ (� ��� �����, �����
*                                 � ����� ��� �� ������ ����������� �����);
*       MIDIFILE_CANT_ALLOC_MEMORY - �� ������� �������� ������.
*
*   ������������� ������� �������� �� ��������� ��� ���� �����. ����� ��������� �������
*   �������� �� ��������� ����� ��������� ��������� ��������:
*   1) ������� ����� �����;
*   2) ������ ������ ��������� ������ ��� ������ �����.
*
****************************************************************************************/

MIDIFILERESULT MidiFile::SetDefaultCodePage(
	__in UINT DefaultCodePage)
{
	m_DefaultCodePage = DefaultCodePage;

	if (m_pFile == NULL) return MIDIFILE_SUCCESS;

	// ���� ����� �����
	MIDIFILERESULT Res = FindLyric();
	if (Res != MIDIFILE_SUCCESS)
	{
		LOG("lyric not found\n");
		return Res;
	}

	// ���� ��������� ������
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
*   ����� SetConcordNoteChoice
*
*   ���������
*       ConcordNoteChoice - ����������, ����� �� ��� �������� ����� ��������:
*                           CHOOSE_MIN_NOTE_NUMBER - ���� � ����������� �������,
*                           CHOOSE_MAX_NOTE_NUMBER - ���� � ������������ �������,
*                           CHOOSE_MAX_NOTE_DURATION - ���� � ������������ �������������
*
*   ������������ ��������
*       MIDIFILE_SUCCESS - �������� ������ ���� �� �������� ������� ���������� � �������
*                          ���� �� ���� ��������� ������;
*       MIDIFILE_NO_VOCAL_PARTS - �� ������� �� ���� ��������� ������ (� ��� �����, �����
*                                 � ����� ��� �� ������ ����������� �����);
*       MIDIFILE_CANT_ALLOC_MEMORY - �� ������� �������� ������.
*
*   ������������� �������� ������ ���� �� ��������. ���� �������� ������������, �����
*   ��������� ������ �������� �������� � ���������� ������� ������ ���� ����. �����
*   ��������� ����� �������� ����� ������ ������ ������ ��������� ������ ��� ������
*   �����.
*
****************************************************************************************/

MIDIFILERESULT MidiFile::SetConcordNoteChoice(
	__in CONCORD_NOTE_CHOICE ConcordNoteChoice)
{
	m_ConcordNoteChoice = ConcordNoteChoice;

	if (m_pFile == NULL) return MIDIFILE_SUCCESS;

	// ���� ��������� ������
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
*   ����� AssignFile
*
*   ���������
*       pFile - ��������� �� ����� ������, � ������� �������� ����; ����� ������ ����
*               ������� �������� HeapAlloc
*		cbFile - ������ ����� � ������
*
*   ������������ ��������
*       MIDIFILE_SUCCESS - ���� ������� �������� ������� (������� ����� ����� � ����
*                          �� ���� ��������� ������);
*       MIDIFILE_INVALID_FORMAT - ���� �� �������� MIDI-������;
*       MIDIFILE_UNSUPPORTED_FORMAT - ���������������� ������ MIDI-�����: ���� ������
*                                     MIDI-����� ����� 2 (������������������ ������ ����
*                                     �� ������ ������), ���� ������-����� ������������ �
*                                     SMPTE-�������;
*       MIDIFILE_NO_LYRIC - �� ������� ����� �����;
*       MIDIFILE_NO_VOCAL_PARTS - �� ������� �� ���� ��������� ������ (� ��� �����, �����
*                                 � ����� ��� �� ������ ����������� �����);
*       MIDIFILE_CANT_ALLOC_MEMORY - �� ������� �������� ������.
*
*   ��������� ������� ����������� � ������ ����. ����� ������ ����� ������ ����� ������,
*   �� ������� ��������� �������� pFile, ��������� �� �������� �������, �.�. ������
*   ���������� ������������� �� ������������ ����� ������. ����� ���, ��� ��������� ����
*   �������, ����� ��������� ��������� ��������:
*   1) ��������� ������ �����;
*   2) ������� ����� �����;
*   3) ������ ������ ��������� ������ ��� ������ �����.
*
****************************************************************************************/

MIDIFILERESULT MidiFile::AssignFile(
	__in BYTE *pFile,
	__in DWORD cbFile)
{
	// ���� ����� ������� ��� �������� �����-�� ����, ����������� ��� ���������� ���
	// ����� ����� �������
	Free();

	if (pFile == NULL) return MIDIFILE_INVALID_FORMAT;

	// ����������� ��������� �����
	// ---------------------------

	// ���������, ��� ������ ����� �� ������ ������� ������������ ��������� MIDI-�����
	if (cbFile < 14)
	{
		LOG("file is too small\n");
		return MIDIFILE_INVALID_FORMAT;
	}

	// ��������� ��������� MIDI-�����
	DWORD dwFileSignature = GetBigEndianDword(pFile);
	if (dwFileSignature != 'MThd')
	{
		LOG("MIDI file signature not found\n");
        return MIDIFILE_INVALID_FORMAT;
	}

	// ��������� �������� ���� "����� ���������"
	DWORD dwHeaderLength = GetBigEndianDword(pFile + 4);
	if (dwHeaderLength != 6)
	{
		LOG("invalid length of MIDI file header\n");
		return MIDIFILE_INVALID_FORMAT;
	}

	// ��������� ������ MIDI-�����;
	// ������ �������������� MIDI-����� ������� 0 (���� �������������� ����) � ������� 1
	// (���� � ����� ������������� ������);
	DWORD Format = GetBigEndianWord(pFile + 8);
	if (Format == 2)
	{
		// MIDI-���� ������� 2 (������������������ ������ ���� �� ������ ������)
		LOG("unsupported MIDI file format\n");
		return MIDIFILE_UNSUPPORTED_FORMAT;
	}
	else if (Format > 2)
	{
		// ���������������� �������� ���� "������ MIDI-�����"
		LOG("invalid MIDI file format\n");
		return MIDIFILE_INVALID_FORMAT;
	}

	// ��������� ������ ������-�������;
	// ������ �������������� ������ MIDI-�����, � ������� ������-����� ������� � �����
	if (pFile[12] & 0x80)
	{
		// ������-����� ������� � SMPTE-�������
		LOG("SMPTE format for delta times is not supported\n");
		return MIDIFILE_UNSUPPORTED_FORMAT;
	}

	// ���������� ���������� ����� �� ���������� MIDI-����
	m_cTicksPerMidiQuarterNote = GetBigEndianWord(pFile + 12);

	// ����������� �����
    // -----------------

	// ������������ ���������� ������ (������ c ���������� "MTrk") � MIDI-�����
	DWORD cTracks = 0;
	DWORD iCurByte = 14;
	while (iCurByte + 7 < cbFile)
	{
		// ��������� ��������� �������� �����
		DWORD CurChunkSignature = GetBigEndianDword(pFile + iCurByte);
		iCurByte += 4;

        // ��������� ������ ������ �������� ����� � ������
		DWORD cbChunk = GetBigEndianDword(pFile + iCurByte);
		iCurByte += 4;

		// ���� ������ �������� ����� ������� �� ������� �����, �� ��������������
		// ��������� ������ ������ �����
		if (iCurByte + cbChunk > cbFile) cbChunk = cbFile - iCurByte;

		// ���� ������� ���� �������� ���������, �������� �� "MTrk", ���� ������ ������
        // ����� ����� ����, �� ���� ���� �� ������� �� ����
		if (CurChunkSignature == 'MTrk' && cbChunk > 0) cTracks ++;

		iCurByte += cbChunk;
	}

	// ��������� ���������� ��������� ������ � �����
	if (cTracks == 0)
	{
		LOG("no tracks in MIDI file\n");
		return MIDIFILE_NO_VOCAL_PARTS;
	}

	// �������� ������ ��� ������� �������� ������ MidiTrack;
    // ������ ������� ����� ���������� ��������� ������
    m_MidiTracks = new MidiTrack[cTracks];
    if (m_MidiTracks == NULL)
    {
    	LOG("operator new failed\n");
    	return MIDIFILE_CANT_ALLOC_MEMORY;
    }

	// ���������� ������� ������ MidiTrack � �������� ������ MIDI-�����
   	iCurByte = 14;
   	DWORD iCurTrack = 0;
	while (iCurByte + 7 < cbFile)
	{
		// ��������� ��������� �������� �����
		DWORD CurChunkSignature = GetBigEndianDword(pFile + iCurByte);
		iCurByte += 4;

		// ��������� ������ ������ �������� ����� � ������
		DWORD cbChunk = GetBigEndianDword(pFile + iCurByte);
		iCurByte += 4;

		// ���� ������ �������� ����� ������� �� ������� �����, �� ��������������
		// ��������� ������ ������ �����
		if (iCurByte + cbChunk > cbFile) cbChunk = cbFile - iCurByte;

		if (CurChunkSignature == 'MTrk' && cbChunk > 0)
		{
			// ���������� ������� ������ ������ MidiTrack � ������� ����� MIDI-�����
			if (m_MidiTracks[iCurTrack].AttachToTrack(pFile + iCurByte, cbChunk))
			{
				// ������� ������ ������� ���������, ������ ������� ��������� ������
				iCurTrack++;
			}
		}

		iCurByte += cbChunk;
	}

	// ��������� ���������� �������� ������ MidiTrack, ������� ������� ����������
    // � ������ MIDI-�����
	if (iCurTrack == 0)
	{
		LOG("no valid tracks in MIDI file\n");
		return MIDIFILE_NO_VOCAL_PARTS;
	}

	// ��������� ���������� �������������� �������� � ������� m_MidiTracks
	m_cTracks = iCurTrack;

	// ���� ����� �����
	MIDIFILERESULT Res = FindLyric();
	if (Res != MIDIFILE_SUCCESS)
	{
		LOG("lyric not found\n");
		return Res;
	}

	// ���� ��������� ������
	Res = FindVocalParts();
	if (Res != MIDIFILE_SUCCESS)
	{
		LOG("vocal parts not found\n");
		return Res;
	}

	// ���������� ��������� �� ����� ������, � ������� �������� MIDI-����
	m_pFile = pFile;

	return MIDIFILE_SUCCESS;
}

/****************************************************************************************
*
*   ����� CreateSong
*
*   ���������
*       ���
*
*   ������������ ��������
*       ��������� �� ������ ������ Song, ��� NULL, ���� ��������� ������. � �����,
*       ����������� ��������� ������� ����� ���� ������ ��� ��������� ������.
*
*   ������ ������ ������ Song, �������������� ����� �����.
*
*   ����������
*
*   � �������� ��������� ������ ����� ���� ������, ��������� � ������ �������� ������
*   ��������� ������ m_pVocalPartList.
*
****************************************************************************************/

Song *MidiFile::CreateSong()
{
	if (m_pVocalPartList == NULL) return NULL;

	// ��������� �� ������ ������� ������ ��������� ������ m_pVocalPartList
	VOCALPARTINFO *pVocalPart = m_pVocalPartList;

	// ������ ����� � ������� m_MidiTracks, � ������� ������������� ��������� ������
	DWORD iTrack = pVocalPart->iTrack;

	// ����� ������, � ������� ��������� ��������� ������
	DWORD Channel = pVocalPart->Channel;

	// ����� �����������, ������� �������� ���� ��������� ������
	DWORD Instrument = pVocalPart->Instrument;

	// ������ ������ ������ MidiSong
	MidiSong MidiSong;

	// ������ ���
	if (!CreateSes(iTrack, Channel, Instrument, &MidiSong))
	{
		LOG("MidiFile::CreateSes failed\n");
		return NULL;
	}

	// ������ ������������������ ������
	if (!CreateMeasureSequence(&MidiSong))
	{
		LOG("MidiFile::CreateMeasureSequence failed\n");
		return NULL;
	}

	// ������ ����� ������
	if (!CreateTempoMap(&MidiSong))
	{
		LOG("MidiFile::CreateTempoMap failed\n");
		return NULL;
	}

	// ������������ ����� ���� ����� � ���
	if (!MidiSong.CorrectLyric())
	{
		LOG("MidiSong::CorrectLyric failed\n");
		return NULL;
	}

	// ������ ������ ������ Song
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
*   ����� FindLyric
*
*   ���������
*       ���
*
*   ������������ ��������
*       MIDIFILE_SUCCESS - ����� ����� �������;
*       MIDIFILE_NO_LYRIC - ����� ����� �� �������;
*       MIDIFILE_CANT_ALLOC_MEMORY - �� ������� �������� ������.
*
*   ���� ����� �����. ����� ����� ����� ���������� ���� � ������������ LYRIC, ����
*   � ������������ TEXT_EVENT. � ������ ������ ����� ������ ������ ������ MidiTrack,
*   ���������� ��� � ����� �� ������� � ��������� ��������� �� ���� ������ � ����������
*   m_pLyricTrack. ����� ����, � ���������� m_LyricEventType ����� ��������� ���
*   �����������, � ������� ���� ������� ����� �����.
*
****************************************************************************************/

MIDIFILERESULT MidiFile::FindLyric()
{
	if (m_pLyricTrack != NULL)
	{
		// ������� ������ ������ ������ MidiTrack, ������������ � ����� �� ������� �����
		delete m_pLyricTrack;
		m_pLyricTrack = NULL;
	}

	if (m_MidiTracks == NULL) return MIDIFILE_NO_LYRIC;

	// ������� ��� �����������, � ������� ������ ����� �����;
	// �������� ����� � ����������� ���� LYRIC
	DWORD CurLyricEventType = LYRIC;

	// � ���� ����� ������������ ���� �����������: LYRIC � TEXT_EVENT
	while(true)
	{
		// ������ ����� � ������� m_MidiTracks, ����������� ������������ �� ������
		// ������ ���������� �������� � ���������� ������������ ���� CurLyricEventType
		DWORD iTrackWithMaxSymbols = 0;

		// ���������� �������� � ���������� ������������ ���� CurLyricEventType �� �����
		// iTrackWithMaxSymbols, ��� �������� ������������ �� ������ ������
		DWORD cMaxSymbols = 0;

		// ���������� ���������� ����������� ���� CurLyricEventType � �����
		// iTrackWithMaxSymbols
		DWORD cEventsInTrackWithMaxSymbols = 0;

		// ���� �� ���� ������ ������� m_MidiTracks
		for (DWORD i = 0; i < m_cTracks; i++)
		{
			// ���������� �������� � ���������� ������������ ���� CurLyricEventType ��
			// �������� �����
			DWORD cSymbolsInTrack = 0;

			// ���������� ���������� ����������� ���� CurLyricEventType � ������� �����
			DWORD cEventsInTrack = 0;

			// ������ ������ MidiLyric ��� ������ ���������� ����������� ����
			// CurLyricEventType � ������� �����
			MidiLyric Lyric;

			// �������������� ����� ���������� ����������� ���� CurLyricEventType
			// � ������� �����
			Lyric.InitSearch(&m_MidiTracks[i], CurLyricEventType, m_DefaultCodePage);

			// ���� �� ���� ���������� ������������ ���� CurLyricEventType �� ��������
			// �����
			while (true)
			{
				// ���������� �������� � ������ ���������� ����������� �����������
				DWORD cchEventText;

				// �������� ��������� ���������� ����������� ���� CurLyricEventType
				MIDILYRICRESULT Res = Lyric.GetNextValidEvent(NULL, NULL, &cchEventText);

				// ���� ����������� ���� CurLyricEventType � ������� ����� ���������,
				// ������� �� �����
				if (Res == MIDILYRIC_LYRIC_END) break;

				cSymbolsInTrack += cchEventText;
				cEventsInTrack++;
			}

			if (cSymbolsInTrack > cMaxSymbols)
			{
				// ���������� �������� � ���������� ������������ ���� CurLyricEventType
				// �� �������� ����� ����������� ����� ���� ������������� ������

				iTrackWithMaxSymbols = i;
				cMaxSymbols = cSymbolsInTrack;
				cEventsInTrackWithMaxSymbols = cEventsInTrack;
			}
		}

		if (CurLyricEventType == LYRIC)
		{
			if (cEventsInTrackWithMaxSymbols > 0)
			{
				// ����� ����� �������;
				// ������ ������ ������ MidiTrack - ����� �������,
				// ������������� � ����� �� ������� �����;
				m_pLyricTrack = m_MidiTracks[iTrackWithMaxSymbols].Duplicate();

				if (m_pLyricTrack == NULL)
				{
					LOG("MidiTrack::Duplicate failed\n");
					return MIDIFILE_CANT_ALLOC_MEMORY;
				}

				// ����� ����� ��������� � ������������ ���� LYRIC
				m_LyricEventType = LYRIC;

				return MIDIFILE_SUCCESS;
			}

			// ����� ����� � ������������ ���� LYRIC �� �������,
			// ������ ����� ����� � ������������ ���� TEXT_EVENT
			CurLyricEventType = TEXT_EVENT;
		}
		else // CurLyricEventType == TEXT_EVENT
		{
			if (cEventsInTrackWithMaxSymbols >= MIN_TEXT_EVENTS_FOR_LYRIC_TRACK)
			{
				// ����� ����� �������;
				// ������ ������ ������ MidiTrack - ����� �������,
				// ������������� � ����� �� ������� �����;
				m_pLyricTrack = m_MidiTracks[iTrackWithMaxSymbols].Duplicate();

				if (m_pLyricTrack == NULL)
				{
					LOG("MidiTrack::Duplicate failed\n");
					return MIDIFILE_CANT_ALLOC_MEMORY;
				}

                // ����� ����� ��������� � ������������ ���� TEXT_EVENT
				m_LyricEventType = TEXT_EVENT;

				return MIDIFILE_SUCCESS;
			}

			// ����� ����� �� �������
			return MIDIFILE_NO_LYRIC;
		}
	}
}

/****************************************************************************************
*
*   ����� FindVocalParts
*
*   ���������
*       ���
*
*   ������������ ��������
*       MIDIFILE_SUCCESS - ������� ��� ������� ���� ��������� ������;
*       MIDIFILE_NO_VOCAL_PARTS - �� ������� �� ���� ��������� ������;
*       MIDIFILE_CANT_ALLOC_MEMORY - �� ������� �������� ������.
*
*   ���� ��������� ������.
*
*   ����������
*
*   ����� ��������� ������ �������������� � ��������� ������. ������� �� ��������� ����
*   ����������, ���� �� ������� ����� �� ������� �� ����� ��������� ������. ��� ������
*   ������ - ����������� �� ������ ��������� - ����������� ���� �������� ������ � ����
*   �����. ���� ��������� �������� ���� �������� ��������� MAX_VOCAL_PART_LYRIC_DISTANCE,
*   �� ������ �������������, � ����� ����������� � ������ ��������� ������
*   m_pVocalPartList.
*
****************************************************************************************/

MIDIFILERESULT MidiFile::FindVocalParts()
{
	// ������� ������ ��������� ������
	VOCALPARTINFO *pCurVocalPart = m_pVocalPartList;
	while (pCurVocalPart != NULL)
	{
		VOCALPARTINFO *pDelVocalPart = pCurVocalPart;
		pCurVocalPart = pCurVocalPart->pNext;
		delete pDelVocalPart;
	}
	m_pVocalPartList = NULL;

	// ������, ������ ������� �������� ��������� ����������� ���������� ����� ������
	// ��������� ������:
	// 1) ������ ���� �������� ����� ����������� ���������� ���� ���������� ���
	//    �� ������� �� ��������� � ���������� ��������� ��� � ������;
	// 2) ������ ���� �������� ����� ����� ������, ������������ �������� ������
	//    ��������� ������
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

	// ���� �� ������ ������ ��������� ������
	for (DWORD iStage = 0; iStage < cStages; iStage++)
	{
		// ���� �� ���� ������ ������� m_MidiTracks
		for (DWORD iCurTrack = 0; iCurTrack < m_cTracks; iCurTrack++)
		{
			// �������, � ������� ����� �������� ������, ������������ � ������� �����;
			// ���� �������� �������� ������� � �������� [i][j] ����� true, �� � �����
			// ���������� ������, �������� �� ������ i ������������ j
			bool DoesPartExist[MAX_MIDI_CHANNELS][MAX_MIDI_INSTRUMENTS];

			// �������������� �������� ������� DoesPartExist ���������� false
			for (DWORD i = 0; i < MAX_MIDI_CHANNELS; i++)
				for (DWORD j = 0; j < MAX_MIDI_INSTRUMENTS; j++)
					DoesPartExist[i][j] = false;

			// ���������� ������ � ������� �����;
			// ��� ����� ���������� ��������� � ������� DoesPartExist �� ��������� true
			DWORD cPartsInTrack = 0;

			// ������������� ������� ������� �� ������ �����
			m_MidiTracks[iCurTrack].ResetCurrentPosition();

			// ���� �� ���� �������� �������� �����, � ���� ����� ������ ������
			while (true)
			{
				// ��������� �� ������ ���� ������ �������
				PBYTE pEventData;

				// ����� ������ �������
				DWORD EventChannel;

				// ��������� ��������� ������� �����
				DWORD EventType = m_MidiTracks[iCurTrack].GetNextEvent(NULL,
					&pEventData, &EventChannel);

				// ���� � ����� ������ �� �������� �������, ������� �� �����
				if (EventType == REAL_TRACK_END) break;

				// ����������� ����� �������, ����� ������� PROGRAM_CHANGE
				if (EventType != PROGRAM_CHANGE) continue;

				// ������� ����� �����������
				DWORD EventInstrument = *pEventData;

				if (!DoesPartExist[EventChannel][EventInstrument])
				{
					// � ������� ����� ������� ����� ������, �������� �� ������
					// EventChannel ������������ EventInstrument
					DoesPartExist[EventChannel][EventInstrument] = true;
					cPartsInTrack++;
				}
			}

			// ���� �� ������� �� ����� ������, �� ��������� � ���������� �����
			if (cPartsInTrack == 0) continue;

            // ���������� ������ ������ (�.�. ������, �� ���������� ����) ����� ������
            // �� ������� DoesPartExist
			DWORD cEmptyParts = 0;

            // ���� �� ���� ��������� ������� DoesPartExist: ���� ��������� ������

			for (DWORD CurChannel = 0; CurChannel < MAX_MIDI_CHANNELS; CurChannel++)
			{
				for (DWORD CurInstr = 0; CurInstr < MAX_MIDI_INSTRUMENTS; CurInstr++)
				{
					// ���� � ������� ����� ��� ������, �������� �� ������ CurChannel
                    // ������������ CurInstr, �� ��������� � ���������� �������� �������
					if (!DoesPartExist[CurChannel][CurInstr]) continue;

					// ����������, � ������� ����� GetPartLyricDistance ������� ����
					// �������� ������� ������ � ���� ����� � ���������� MIDI-�����
					double PartLyricDistance;

					// ������� ���� �������� ������� ������ � ���� �����
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
						// ���� ��������� �������� ���� �������� ������� ������ � ����
						// ����� �� ��������� MAX_VOCAL_PART_LYRIC_DISTANCE, �� ���������
						// ������ � ������ ��������� ������ m_pVocalPartList
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

			// ���� ������� ���� �������� ��� ������� 2 �������� ������,
			// �� ���������, �� �������� �� ����� ���� ��������� �������
			if (cPartsInTrack - cEmptyParts > 1)
			{
				// ����������, � ������� ����� GetPartLyricDistance ������� ����
				// �������� ������, ��������������� ������ �����, � ���� �����
				// � ���������� MIDI-�����
				double PartLyricDistance;

				// ������� ���� �������� ������, ��������������� ������ �����,
				// � ���� �����
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
					// ���� ��������� �������� ���� �������� ������, ���������������
					// ������ �����, � ���� ����� �� ���������
					// MAX_VOCAL_PART_LYRIC_DISTANCE, �� ��������� ������ � ������
					// ��������� ������ m_pVocalPartList
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
			// ��� ������� ���� ��������� ������ �������, ������� ����� ����������
			return MIDIFILE_SUCCESS;
		}
	}

	// �� ������� �� ����� ��������� ������
	return MIDIFILE_NO_VOCAL_PARTS;
}

/****************************************************************************************
*
*   ����� GetPartLyricDistance
*
*   ���������
*       iTrack - ������ ����� � ������� m_MidiTracks, � ������� ������������� ������
*       Channel - ����� ������, � ������� ��������� ������; ���� �������� ����� ���������
*                 ����� ANY_CHANNEL, �� � ������ ��������� ���� �� ���� ������� �����
*       Instrument - ����� �����������, ������� �������� ���� ������; ���� �������� �����
*                    ��������� ����� ANY_INSTRUMENT, �� � ������ ��������� ����, ��������
*                    ����� ������������
*       OverlapsThreshold - ����������� ���������� ���� ���������� ��� �� �������
*                           �� ��������� � ���������� ��������� ��� � ������
*       fCriteria - ����� ������, ������������ ��������, ���������� �� ������:
*                   IDENTICAL_START - ������������� ������;
*                   IDENTICAL_END - ������������� �����;
*                   LIMIT_NOTES_PER_METAEVENT - ������������ ���������� ��� �� ����
*                                               �����������
*       pPartLyricDistance - ��������� �� ����������, � ������� ����� �������� ����
*                            �������� ������ � ���� ����� � ���������� MIDI-�����
*
*   ������������ ��������
*       DISTANCE_SUCCESS - ���� �������� ������� ���������;
*       DISTANCE_NOT_VOCAL_PART - ������ �� �������� ���������;
*       DISTANCE_NO_NOTES - � ������ ��� �� ����� ����;
*		DISTANCE_CANT_ALLOC_MEMORY - �� ������� �������� ������.
*
*   ��������� ���� �������� ������, �������� ����������� iTrack, Channel � Instrument, �
*   ���� �����, ������������ ����������� ������� m_pLyricTrack � m_LyricEventType, �
*   ���������� MIDI-�����.
*
*   ����� ������ ����� ������������ �� ������ ���������, ��� ������ ������������� ����
*   ���������� �� �� ������������. ���� ������ �� ������������� ���� ���������� �� ��
*   ������������, ����� ���������� ��� ������ DISTANCE_NOT_VOCAL_PART. ���������
*   �����������, ���������� �� ������:
*   1) ��� ��������� ������ �� ������ ���� �������� ����� OverlapsThreshold;
*   2) ��� ��������� ������ ���������� �������� �� ���� ������������, ������������ ��
*      ���� ����, �� ������ ��������� MAX_SYMBOLS_PER_NOTE; ��� ��������� ���� ���
*      ����������� ��������� � ����� ���������: ���� �� ����� �������� IDENTICAL_END, ��
*      �����������, ��������� ������ �� ������ ��������� ����, ������������;
*   3) ���� ����� �������� LIMIT_NOTES_PER_METAEVENT, �� ��� ��������� ������ ����������
*      ��� �� ���� ����������� �� ������ ��������� MAX_NOTES_PER_METAEVENT;
*   4) ���� ����� �������� IDENTICAL_END, �� ��� ��������� ������ ���������� ��� ��
*      ��������� ����������� �� ������ ��������� MAX_NOTES_PER_METAEVENT;
*   5) ���� ����� �������� IDENTICAL_START, �� ��� ��������� ������ ����, ��������� �
*      ������� �����������, ������ ���� ������ � ������;
*   6) ���� ����� �������� IDENTICAL_START, �� ��� ��������� ������ ���������� �� ������
*      ������ ���� �� ������� ����������� ������ ���� ������ ��� ����� ���������� ��
*      ������ ������ ���� �� ������� �����������.
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
	// ���� �������� ������ � ���� ����� � �����
	DWORD PartLyricDistance = 0;

	// ������ ������ MidiLyric ��� ��������� ���������� ����������� ���� m_LyricEventType
	// �� ����� �� ������� �����
	MidiLyric Lyric;

	// �������������� ����� ���������� ����������� ���� m_LyricEventType
	// � ����� �� ������� �����
	Lyric.InitSearch(m_pLyricTrack, m_LyricEventType, m_DefaultCodePage);

	// ����������, � ������� ����� GetNextValidEvent ����� ���������� ���������� �����,
	// ��������� �� ������ ����� �� ������� ������� ���������� ����������� �����������
	DWORD ctToLyricEvent;

	// ����������, � ������� ����� GetNextValidEvent ����� ���������� ���������� ��������
	// � ������ ���������� ����������� �����������
	DWORD cchEventText;

	// ����������, � ������� ����� ������������� ���������� ��������
	// � ������ ���� �����������, ������������ �� ���� PrevNoteDesc
	DWORD cchPerNote = 0;

	// ������ ������ MidiPart ��� ��������� ��������� ��� �� ������
	MidiPart Part;

	// �������������� ����� ��������� ��� � ������
	Part.InitSearch(&m_MidiTracks[iTrack], Channel, Instrument, OverlapsThreshold,
		m_ConcordNoteChoice);

	// ����������, ����������� ����, �������������� ������� ����
	NOTEDESC PrevNoteDesc;

	// ����������, ����������� ������� ����, �.�. ����, ��������� ���������
	NOTEDESC CurNoteDesc;

	// ��������� ������ �����������, ��� ������ ����, �.�. LyricRes == MIDILYRIC_SUCCESS
	MIDILYRICRESULT LyricRes = Lyric.GetNextValidEvent(&ctToLyricEvent, NULL,
		&cchEventText);

	// ��������� ������ ����
	MIDIPARTRESULT PartRes = Part.GetNextNote(&CurNoteDesc);

	// ������������ ������ GetNextNote
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
		// ����� �������� ������ MIDIPART_COMPLICATED_NOTE_COMBINATION
		// ��� MIDIPART_OVERLAPS_THRESHOLD_EXCEEDED
		return DISTANCE_NOT_VOCAL_PART;
	}

	// ���������, ��� ������ ������: ������ ����������� ��� ������ ����
	if (ctToLyricEvent <= CurNoteDesc.ctToNoteOn)
	{
		// ������ ����������� ������ ������ ��� ������������ � ������ �����

		// ���������� �� ������� ����������� �� ������ ������ ����,
		// ������� � ������ ������ �������� ��������� ��� ����
		PartLyricDistance = Distance(ctToLyricEvent, CurNoteDesc.ctToNoteOn);

		// ��������� ���������� �������� � ������ �����������
		cchPerNote = cchEventText;

		// ��������� ������ �����������
		LyricRes = Lyric.GetNextValidEvent(&ctToLyricEvent, NULL, &cchEventText);

		if (LyricRes == MIDILYRIC_LYRIC_END)
		{
			// � ����� ������ ���� �����������

			// ���� ��������� ����������� �� ���������� �������� �� ����,
			// ���������� ������
			if (cchPerNote > MAX_SYMBOLS_PER_NOTE) return DISTANCE_NOT_VOCAL_PART;

			if (fCriteria & LIMIT_NOTES_PER_METAEVENT || fCriteria & IDENTICAL_END)
			{
				// ���������� ���������, ��� ���������� ��� �� ������ �����������
				// �� ��������� MAX_NOTES_PER_METAEVENT

				DWORD cNotesPerMetaEvent = 1;

				// ���� �� �����
				while (true)
				{
					// ��������� ��������� ����
					PartRes = Part.GetNextNote(&CurNoteDesc);

					// ������������ ������ GetNextNote
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
						// ����� �������� ������ MIDIPART_COMPLICATED_NOTE_COMBINATION
						// ��� MIDIPART_OVERLAPS_THRESHOLD_EXCEEDED
						return DISTANCE_NOT_VOCAL_PART;
					}

					cNotesPerMetaEvent++;

					if (cNotesPerMetaEvent > MAX_NOTES_PER_METAEVENT)
					{
						// ��������� ����������� �� ���������� ��� �� �����������,
						// ���������� ������
						return DISTANCE_NOT_VOCAL_PART;
					}
				}
			}

			// ���������� ���� �������� ������ � ���� ����� � ���������� MIDI-�����
			*pPartLyricDistance = (double) PartLyricDistance / m_cTicksPerMidiQuarterNote;
			return DISTANCE_SUCCESS;
		}

		if (fCriteria & IDENTICAL_START)
		{
			// ���������� ���������, ��� ���������� �� ������ ������ ���� �� �������
			// ����������� �� ������ ���������� �� ������ ������ ���� �� �������
			// �����������
			if (PartLyricDistance > Distance(ctToLyricEvent, CurNoteDesc.ctToNoteOn))
			{
				// �������� IDENTICAL_START �������, ���������� ������
				return DISTANCE_NOT_VOCAL_PART;
			}
		}

		// ������� ���� ���������� ����������
		PrevNoteDesc = CurNoteDesc;

		// ��������� ������ ����
		PartRes = Part.GetNextNote(&CurNoteDesc);

		// ������������ ������ GetNextNote
		if (PartRes == MIDIPART_CANT_ALLOC_MEMORY)
		{
			LOG("MidiPart::GetNextNote failed\n");
			return DISTANCE_CANT_ALLOC_MEMORY;
		}
		else if (PartRes == MIDIPART_PART_END)
		{
			// � ����� ������ ���� ����

			DWORD cchPerFirstNote = cchPerNote;

			// ������ ��������� ������ ����
			DWORD ctToNoteOff = PrevNoteDesc.ctToNoteOn + PrevNoteDesc.ctDuration;

			// ���� �� ������������
			while (true)
			{
				if (fCriteria & IDENTICAL_END)
				{
					// ���� ����� �������� IDENTICAL_END,
					// �� ��������� ��� �����������
					cchPerFirstNote += cchEventText;
				}
				else
				{
					// ���� �� ����� �������� IDENTICAL_END, �� �����������,
					// ��������� ������ �� ������ ��������� ����, ������������
					if (ctToLyricEvent <= ctToNoteOff)
					{
						cchPerFirstNote += cchEventText;
					}
				}

				if (cchPerFirstNote > MAX_SYMBOLS_PER_NOTE)
				{
					// ��������� ����������� �� ���������� �������� �� ����,
					// ���������� ������
					return DISTANCE_NOT_VOCAL_PART;
				}

				// ���������� �� ���������� ����������� �� ������ ������ ����,
				// ������� � ������ ������ �������� ��������� ��� ����
				PartLyricDistance += Distance(ctToLyricEvent, PrevNoteDesc.ctToNoteOn);

				// ��������� ��������� �����������
				LyricRes = Lyric.GetNextValidEvent(&ctToLyricEvent, NULL, &cchEventText);

				if (LyricRes == MIDILYRIC_LYRIC_END) break;
			}

			// ���������� ���� �������� ������ � ���� ����� � ���������� MIDI-�����
			*pPartLyricDistance = (double) PartLyricDistance / m_cTicksPerMidiQuarterNote;
			return DISTANCE_SUCCESS;
		}
		else if (PartRes != MIDIPART_SUCCESS)
		{
			// ����� �������� ������ MIDIPART_COMPLICATED_NOTE_COMBINATION
			// ��� MIDIPART_OVERLAPS_THRESHOLD_EXCEEDED
			return DISTANCE_NOT_VOCAL_PART;
		}
	}
	else
	{
		// ������ ���� ������ ������ ������� �����������

		// ������� ���� ���������� ����������
		PrevNoteDesc = CurNoteDesc;

		// ��������� ������ ����
		PartRes = Part.GetNextNote(&CurNoteDesc);

		// ������������ ������ GetNextNote
		if (PartRes == MIDIPART_CANT_ALLOC_MEMORY)
		{
			LOG("MidiPart::GetNextNote failed\n");
			return DISTANCE_CANT_ALLOC_MEMORY;
		}
		else if (PartRes == MIDIPART_PART_END)
		{
			// � ����� ������ ���� ����

			DWORD cchPerFirstNote = 0;

			// ������ ��������� ������ ����
			DWORD ctToNoteOff = PrevNoteDesc.ctToNoteOn + PrevNoteDesc.ctDuration;

			// ���� �� ������������
			while (true)
			{
				if (fCriteria & IDENTICAL_END)
				{
					// ���� ����� �������� IDENTICAL_END,
					// �� ��������� ��� �����������
					cchPerFirstNote += cchEventText;
				}
				else
				{
					// ���� �� ����� �������� IDENTICAL_END, �� �����������,
					// ��������� ������ �� ������ ��������� ����, ������������
					if (ctToLyricEvent <= ctToNoteOff)
					{
						cchPerFirstNote += cchEventText;
					}
				}

				if (cchPerFirstNote > MAX_SYMBOLS_PER_NOTE)
				{
					// ��������� ����������� �� ���������� �������� �� ����,
					// ���������� ������
					return DISTANCE_NOT_VOCAL_PART;
				}

				// ���������� �� ���������� ����������� �� ������ ������ ����,
				// ������� � ������ ������ �������� ��������� ��� ����
				PartLyricDistance += Distance(ctToLyricEvent, PrevNoteDesc.ctToNoteOn);

				// ��������� ��������� �����������
				LyricRes = Lyric.GetNextValidEvent(&ctToLyricEvent, NULL, &cchEventText);

				if (LyricRes == MIDILYRIC_LYRIC_END) break;
			}

			// ���������� ���� �������� ������ � ���� ����� � ���������� MIDI-�����
			*pPartLyricDistance = (double) PartLyricDistance / m_cTicksPerMidiQuarterNote;
			return DISTANCE_SUCCESS;
		}
		else if (PartRes != MIDIPART_SUCCESS)
		{
			// ����� �������� ������ MIDIPART_COMPLICATED_NOTE_COMBINATION
			// ��� MIDIPART_OVERLAPS_THRESHOLD_EXCEEDED
			return DISTANCE_NOT_VOCAL_PART;
		}

		if (fCriteria & IDENTICAL_START)
		{
			if (!IsFirstNoteNearer(ctToLyricEvent, &PrevNoteDesc, &CurNoteDesc))
			{
				// ������ ���� �� �������� ��������� � ������� �����������
				return DISTANCE_NOT_VOCAL_PART;
			}
		}
	}

	// ������� ���� ������

	// � ���� ����� ������� ���� ��� ��� ����������� � ��� ����

	// ���� ������� ��� �����������, �� ���������� �� ������� ����������� �� ������
	// ��������� ��� ���� ��������� � ���������� PartLyricDistance, � ���������� ��������
	// � �� ��������� � ���������� cchPerNote

	// ���������� ����� �� ������ ����� �� ������� ������� ���������� ����������
	// ����������� ��������� � ���������� ctToLyricEvent, � ���������� ��������
	// � ���� ����������� - � ���������� cchEventText

	// �������� ������ ���� ��������� � ���������� PrevNoteDesc,
	// � �������� ������ - � ���������� CurNoteDesc

	// ������� �������, ������� ������ ����������� � ������ �����:
	// ����, �������������� ���� PrevNoteDesc, �� ������ ���� ��������� �����
	// � ���������� ���������� �����������

	// ���� �� ������������
	while (true)
	{
		// ���������� ��� �� ���������� �����������
		DWORD cNotesPerMetaEvent = 0;

		// ���� �� �����
		while (!IsFirstNoteNearer(ctToLyricEvent, &PrevNoteDesc, &CurNoteDesc))
		{
			// ���������� ������� ���������� ��������, ������������ �� ���� PrevNoteDesc
			cchPerNote = 0;

			cNotesPerMetaEvent++;

			if (fCriteria & LIMIT_NOTES_PER_METAEVENT)
			{
				if (cNotesPerMetaEvent > MAX_NOTES_PER_METAEVENT)
				{
					// ��������� ����������� �� ���������� ��� �� �����������,
					// ���������� ������
					return DISTANCE_NOT_VOCAL_PART;
				}
			}

			// ���� PrevNoteDesc �� �������� ��������� � �������� �����������

			// ������� ���� ���������� ����������
			PrevNoteDesc = CurNoteDesc;

			// ��������� ��������� ����
			PartRes = Part.GetNextNote(&CurNoteDesc);

			// ������������ ������ GetNextNote
			if (PartRes == MIDIPART_CANT_ALLOC_MEMORY)
			{
				LOG("MidiPart::GetNextNote failed\n");
				return DISTANCE_CANT_ALLOC_MEMORY;
			}
			else if (PartRes == MIDIPART_PART_END)
			{
				// ���� ������ ���������

				DWORD cchPerLastNote = 0;

				// ������ ��������� ��������� ����
				DWORD ctToNoteOff = PrevNoteDesc.ctToNoteOn + PrevNoteDesc.ctDuration;

				// ���� �� ������������
				while (true)
				{
					if (fCriteria & IDENTICAL_END)
					{
						// ���� ����� �������� IDENTICAL_END,
						// �� ��������� ��� �����������
						cchPerLastNote += cchEventText;
					}
					else
					{
						// ���� �� ����� �������� IDENTICAL_END, �� �����������,
						// ��������� ������ �� ������ ��������� ����, ������������
						if (ctToLyricEvent <= ctToNoteOff)
						{
							cchPerLastNote += cchEventText;
						}
					}

					if (cchPerLastNote > MAX_SYMBOLS_PER_NOTE)
					{
						// ��������� ����������� �� ���������� �������� �� ����,
						// ���������� ������
						return DISTANCE_NOT_VOCAL_PART;
					}

					// ���������� �� ���������� ����������� �� ������ ��������� ����,
					// ������� � ������ ������ �������� ��������� ��� ����
					PartLyricDistance += Distance(ctToLyricEvent, PrevNoteDesc.ctToNoteOn);

					// ��������� ��������� �����������
					LyricRes = Lyric.GetNextValidEvent(&ctToLyricEvent, NULL,
						&cchEventText);

					if (LyricRes == MIDILYRIC_LYRIC_END) break;
				}

				// ���������� ���� �������� ������ � ���� ����� � ���������� MIDI-�����
				*pPartLyricDistance =
					(double) PartLyricDistance / m_cTicksPerMidiQuarterNote;
				return DISTANCE_SUCCESS;
			}
			else if (PartRes != MIDIPART_SUCCESS)
			{
				// ����� �������� ������ MIDIPART_COMPLICATED_NOTE_COMBINATION
				// ��� MIDIPART_OVERLAPS_THRESHOLD_EXCEEDED
				return DISTANCE_NOT_VOCAL_PART;
			}
		}

		// ���������� ���������� �� �������� ����������� �� ������ ��������� � ���� ����
		// � ����������, ������������� ���� �������� ����� ������� � ������� �����
		PartLyricDistance += Distance(ctToLyricEvent, PrevNoteDesc.ctToNoteOn);

		// ��������� ���������� ��������, ������������ �� ���� PrevNoteDesc
		cchPerNote += cchEventText;

		if (cchPerNote > MAX_SYMBOLS_PER_NOTE)
		{
			// ��������� ����������� �� ���������� �������� �� ����, ���������� ������
			return DISTANCE_NOT_VOCAL_PART;
		}

		// ��������� ��������� �����������
		LyricRes = Lyric.GetNextValidEvent(&ctToLyricEvent, NULL, &cchEventText);

		if (LyricRes == MIDILYRIC_LYRIC_END)
		{
			// ����������� ���������

			if (fCriteria & IDENTICAL_END || fCriteria & LIMIT_NOTES_PER_METAEVENT)
			{
				// ���������� ���������, ��� ���������� ��� �� ��������� �����������
				// �� ��������� MAX_NOTES_PER_METAEVENT

				// � ���������� ����������� ��������� ���� PrevNoteDesc � CurNoteDesc
				cNotesPerMetaEvent = 2;

				// ���� �� �����
				while (true)
				{
					// ��������� ��������� ����
					PartRes = Part.GetNextNote(&CurNoteDesc);

					// ������������ ������ GetNextNote
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
						// ����� �������� ������ MIDIPART_COMPLICATED_NOTE_COMBINATION
						// ��� MIDIPART_OVERLAPS_THRESHOLD_EXCEEDED
						return DISTANCE_NOT_VOCAL_PART;
					}

					cNotesPerMetaEvent++;

					if (cNotesPerMetaEvent > MAX_NOTES_PER_METAEVENT)
					{
						// ��������� ����������� �� ���������� ��� �� �����������,
						// ���������� ������
						return DISTANCE_NOT_VOCAL_PART;
					}
				}
			}

			// ���������� ���� �������� ������ � ���� ����� � ���������� MIDI-�����
			*pPartLyricDistance = (double) PartLyricDistance / m_cTicksPerMidiQuarterNote;
			return DISTANCE_SUCCESS;
		}
	}
}

/****************************************************************************************
*
*   ����� IsFirstNoteNearer
*
*   ���������
*       ctToLyricEvent - ���������� �����, ��������� �� ������ ����� �� ������� �������
*                        �����������
*       pFirstNote - ��������� �� ��������� NOTEDESC, ����������� ������ ����
*       pSecondNote - ��������� �� ��������� NOTEDESC, ����������� ������ ����
*
*   ������������ ��������
*       true, ���� �� ���� ��� ������ ���� �������� ��������� � �����������; ����� false.
*
*   ���������, ����� �� ���� ��� �������� ��������� � �����������.
*
****************************************************************************************/

bool MidiFile::IsFirstNoteNearer(
	__in DWORD ctToLyricEvent,
	__in NOTEDESC *pFirstNote,
	__in NOTEDESC *pSecondNote)
{
	// ������ ��������� ������ ����
	DWORD ctToFirstNoteOff = pFirstNote->ctToNoteOn + pFirstNote->ctDuration;

	if (ctToLyricEvent >= ctToFirstNoteOff)
	{
		// ����������� ������ ������������ � ������ ��� ����� ������ ����

		if (ctToLyricEvent >= pSecondNote->ctToNoteOn)
		{
			// ������ ���� �� �������� ��������� � �����������
			return false;
		}

		// ����������� ������ ����� ������ � ������ ������

		if (ctToLyricEvent - ctToFirstNoteOff > pSecondNote->ctToNoteOn - ctToLyricEvent)
		{
			// ������ ���� �� �������� ��������� � �����������
			return false;
		}
	}

	return true;
}

/****************************************************************************************
*
*   ����� AddVocalPart
*
*   ���������
*       iTrack - ������ ����� � ������� m_MidiTracks, � ������� ������������� ������
*       Channel - ����� ������, � ������� ��������� ������; ���� �������� ����� ���������
*                 ����� ANY_CHANNEL, �� � ������ ��������� ���� �� ���� ������� �����
*       Instrument - ����� �����������, ������� �������� ���� ������ ������; ����
*                    �������� ����� ��������� ����� ANY_INSTRUMENT, �� � ������ ���������
*                    ����, �������� ����� ������������
*       PartLyricDistance - ���� �������� ������ � ���� ����� � ���������� MIDI-�����
*
*   ������������ ��������
*       true � ������ ������; false, ���� �� ������� �������� ������.
*
*   ��������� ������, �������� ����������� iTrack, Channel � Instrument, � ������
*   ��������� ������ m_pVocalPartList. ������ ����������� ����� ������ ��������� ������,
*   � �������� ���� �������� �������� ������, ��� PartLyricDistance. ����� �������,
*   ������ � ������ ����������� �� �������� �������� �� ������� �����.
*
****************************************************************************************/

bool MidiFile::AddVocalPart(
	__in DWORD iTrack,
	__in DWORD Channel,
	__in DWORD Instrument,
	__in double PartLyricDistance)
{
	// �������� ������ ��� ����� ��������� ������
	VOCALPARTINFO *pNewVocalPart = new VOCALPARTINFO;
	if (pNewVocalPart == NULL)
	{
		LOG("operator new failed\n");
		return false;
	}

	// ��������� ���� ���� ��������� ��� ����� ��������� ������ ���������� ����������
	pNewVocalPart->iTrack = iTrack;
	pNewVocalPart->Channel = Channel;
	pNewVocalPart->Instrument = Instrument;
	pNewVocalPart->PartLyricDistance = PartLyricDistance;

	// ��������� ����� ��������� ������ � ������ m_pVocalPartList

	VOCALPARTINFO **ppCurVocalPart = &m_pVocalPartList;

    // ���� ������ ������� ������, � �������� ���� �������� �������� ������,
    // ��� PartLyricDistance
	while (*ppCurVocalPart != NULL &&
		(*ppCurVocalPart)->PartLyricDistance <= PartLyricDistance)
	{
		ppCurVocalPart = &((*ppCurVocalPart)->pNext);
	}

	// ��������� ����� ��������� ������ ����� ��������� ������, � �������� ���� ��������
	// �������� ������, ��� PartLyricDistance
	pNewVocalPart->pNext = *ppCurVocalPart;
	*ppCurVocalPart = pNewVocalPart;

	return true;
}

/****************************************************************************************
*
*   ����� CreateSes
*
*   ���������
*       iTrack - ������ ����� � ������� m_MidiTracks, � ������� ������������� ������
*       Channel - ����� ������, � ������� ��������� ������; ���� �������� ����� ���������
*                 ����� ANY_CHANNEL, �� � ������ ��������� ���� �� ���� ������� �����
*       Instrument - ����� �����������, ������� �������� ���� ������; ���� �������� �����
*                    ��������� ����� ANY_INSTRUMENT, �� � ������ ��������� ����, ��������
*                    ����� ������������
*       pMidiSong - ��������� �� ������ ������ MidiSong, � ������� ���� ������� ���
*
*   ������������ ��������
*       true � ������ ������; false, ���� ��������� ������. � �����, �����������
*       ��������� ������� ����� ���� ������ ��� ��������� ������.
*
*   ������ ��� � ������� ������ MidiSong. � �������� ��������� ������ ����� ����
*   ������, ��������� ����������� iTrack, Channel � Instrument.
*
*   ��������� ������, �������������� ���� �������, ���������� ������� ������ ���������
*   ���� �����.
*
****************************************************************************************/

bool MidiFile::CreateSes(
	__in DWORD iTrack,
	__in DWORD Channel,
	__in DWORD Instrument,
	__in MidiSong *pMidiSong)
{
	// ������ ������ MidiLyric ��� ��������� ���������� ����������� ���� m_LyricEventType
	// �� ����� �� ������� �����
	MidiLyric Lyric;

	// �������������� ����� ���������� ����������� ���� m_LyricEventType
	// � ����� �� ������� �����
	Lyric.InitSearch(m_pLyricTrack, m_LyricEventType, m_DefaultCodePage);

	// ����������, � ������� ����� GetNextValidEvent ����� ���������� ���������� �����,
	// ��������� �� ������ ����� �� ������� ������� ���������� ����������� �����������
	DWORD ctToLyricEvent;

	// �����, � ������� ����� GetNextValidEvent ����� ���������� ����� ����������
	// ����������� �����������
	WCHAR pwsEventText[MAX_SYMBOLS_PER_LYRIC_EVENT];

	// �����, � ������� ����� ������������� ����� �����������, ����������� � ����� ����
	WCHAR pwsNoteText[MAX_SYMBOLS_PER_NOTE];

	// ����������, � ������� ����� GetNextValidEvent ����� ���������� ���������� ��������
	// � ������ ���������� ����������� �����������
	DWORD cchEventText;

	// ������ ������ MidiPart ��� ��������� ��������� ��� �� ������
	MidiPart Part;

	// �������������� ����� ��������� ��� � ������
	Part.InitSearch(&m_MidiTracks[iTrack], Channel, Instrument, 1.0, m_ConcordNoteChoice);

	// ����������, ����������� ����, �������������� ������� ����
	NOTEDESC PrevNoteDesc;

	// ����������, ����������� ������� ����, �.�. ����, ��������� ���������
	NOTEDESC CurNoteDesc;

	// ��������� ������ �����������, ��� ������ ����, �.�. LyricRes == MIDILYRIC_SUCCESS
	MIDILYRICRESULT LyricRes = Lyric.GetNextValidEvent(&ctToLyricEvent, pwsEventText,
		&cchEventText);

	// ��������� ������ ����
	MIDIPARTRESULT PartRes = Part.GetNextNote(&PrevNoteDesc);

	// ������������ ������ GetNextNote
	if (PartRes != MIDIPART_SUCCESS)
	{
		// ����� �������� ������ ������ MIDIPART_CANT_ALLOC_MEMORY;
		// ������ MIDIPART_PART_END, MIDIPART_COMPLICATED_NOTE_COMBINATION �
		// MIDIPART_OVERLAPS_THRESHOLD_EXCEEDED ����� ����������
		LOG("MidiPart::GetNextNote failed\n");
		return false;
	}

	// ��������� ������ ����
	PartRes = Part.GetNextNote(&CurNoteDesc);

	// ������������ ������ GetNextNote
	if (PartRes == MIDIPART_PART_END)
	{
		// � ����� ������ ���� ����

		// ����������, �������� ������� ���������� �������� �� ����
		DWORD cchPerNote = 0;

		// ������ ��������� ������ (� ���������) ����
		DWORD ctToNoteOff = PrevNoteDesc.ctToNoteOn + PrevNoteDesc.ctDuration;

		// ���� �� ������������
		while (true)
		{
			// ���� ��������� ����������� �� ���������� �������� �� ����,
			// ����������� ��� ����������� � ��� ����������� �� ���
			if (cchPerNote + cchEventText > MAX_SYMBOLS_PER_NOTE) break;

			// �������� ����� �������� ����������� � ������������� �����
			wcsncpy(pwsNoteText + cchPerNote, pwsEventText, cchEventText);

			// ��������� ���������� �������� � ������������� ������
			cchPerNote += cchEventText;

			// ��������� ��������� �����������
			LyricRes = Lyric.GetNextValidEvent(&ctToLyricEvent, pwsEventText,
				&cchEventText);

			if (LyricRes == MIDILYRIC_LYRIC_END) break;

			// ���� ��������� ����������� ���������� ������ ����� ��� �������
			// MIDI-���� ����� ��������� ������ (� ���������) ���� ������,
			// ����������� ��� ����������� � ��� ����������� �� ���
			if (ctToLyricEvent > ctToNoteOff &&
				ctToLyricEvent - ctToNoteOff > m_cTicksPerMidiQuarterNote/2)
			{
				break;
			}
		}

		// ��������� � ��� ���� PrevNoteDesc � ������� � ������ pwsNoteText
		bool bEventAdded = pMidiSong->AddSingingEvent(PrevNoteDesc.NoteNumber,
			PrevNoteDesc.ctToNoteOn, PrevNoteDesc.ctDuration, pwsNoteText, cchPerNote);

		// ���� �� ������� �������� ������, ���������� ������
		if (!bEventAdded)
		{
			LOG("MidiSong::AddSingingEvent failed\n");
			return false;
		}

		return true;
	}
	else if (PartRes != MIDIPART_SUCCESS)
	{
		// ����� �������� ������ ������ MIDIPART_CANT_ALLOC_MEMORY;
		// ������ MIDIPART_COMPLICATED_NOTE_COMBINATION �
		// MIDIPART_OVERLAPS_THRESHOLD_EXCEEDED ����� ����������
		LOG("MidiPart::GetNextNote failed\n");
		return false;
	}

	if (ctToLyricEvent > PrevNoteDesc.ctToNoteOn)
	{
		// ������ ���� ������������ ������� �����������,
		// ���������� ����� ��������� � ������� ����������� ����

		// ���� �� �����
		while (!IsFirstNoteNearer(ctToLyricEvent, &PrevNoteDesc, &CurNoteDesc))
		{
			// ������� ���� ���������� ����������
			PrevNoteDesc = CurNoteDesc;

			// ��������� ��������� ����
			PartRes = Part.GetNextNote(&CurNoteDesc);

			// ������������ ������ GetNextNote
			if (PartRes == MIDIPART_PART_END)
			{
				// ��������� � ������� ����������� �������� ��������� ���� ������

				// ����������, �������� ������� ���������� �������� �� ����
				DWORD cchPerNote = 0;

				// ������ ��������� ��������� ����
				DWORD ctToNoteOff = PrevNoteDesc.ctToNoteOn + PrevNoteDesc.ctDuration;

				// ���� �� ������������
				while (true)
				{
					// ���� ��������� ����������� �� ���������� �������� �� ����,
					// ����������� ��� ����������� � ��� ����������� �� ���
					if (cchPerNote + cchEventText > MAX_SYMBOLS_PER_NOTE) break;

					// �������� ����� �������� ����������� � ������������� �����
					wcsncpy(pwsNoteText + cchPerNote, pwsEventText, cchEventText);

					// ��������� ���������� �������� � ������������� ������
					cchPerNote += cchEventText;

					// ��������� ��������� �����������
					LyricRes = Lyric.GetNextValidEvent(&ctToLyricEvent, pwsEventText,
						&cchEventText);

					if (LyricRes == MIDILYRIC_LYRIC_END) break;

					// ���� ��������� ����������� ���������� ������ ����� ��� �������
					// MIDI-���� ����� ��������� ��������� ���� ������,
					// ����������� ��� ����������� � ��� ����������� �� ���
					if (ctToLyricEvent > ctToNoteOff &&
						ctToLyricEvent - ctToNoteOff > m_cTicksPerMidiQuarterNote/2)
					{
						break;
					}
				}

				// ��������� � ��� ���� PrevNoteDesc � ������� � ������ pwsNoteText
				bool bEventAdded = pMidiSong->AddSingingEvent(PrevNoteDesc.NoteNumber,
					PrevNoteDesc.ctToNoteOn, PrevNoteDesc.ctDuration, pwsNoteText,
					cchPerNote);

				// ���� �� ������� �������� ������, ���������� ������
				if (!bEventAdded)
				{
					LOG("MidiSong::AddSingingEvent failed\n");
					return false;
				}

				return true;
			}
			else if (PartRes != MIDIPART_SUCCESS)
			{
				// ����� �������� ������ ������ MIDIPART_CANT_ALLOC_MEMORY;
				// ������ MIDIPART_COMPLICATED_NOTE_COMBINATION �
				// MIDIPART_OVERLAPS_THRESHOLD_EXCEEDED ����� ����������
				LOG("MidiPart::GetNextNote failed\n");
				return false;
			}
		}
	}

	// ������� ���� ������

	// � ���� ����� ������� ���� ����������� � ��� ������� ��� ����,
	// ������ ���� PrevNoteDesc �������� ��������� � ������� �����������

	// ���������� ����� �� ������ ����� �� ������� ������� ������� �����������
	// ��������� � ���������� ctToLyricEvent, � ���������� ��������
	// � ���� ����������� - � ���������� cchEventText

	// ������� �������, ������� ������ ����������� � ������ �����:
	// ���� PrevNoteDesc ������ ���� ��������� � ���������� ���������� �����������

	// �� ������ �������� ����� ����� � ��� ����������� ����� ���� ���� �� �������
	// �, ��������, ���� � ����� ��� ��� ����
	while (true)
	{
		// ����������, �������� ������� ���������� �������� �� ����
		DWORD cchPerNote = 0;

		// ���� �� ������������; ������� ��� �����������,
		// ��� ������� ���� PrevNoteDesc ���������
		do
		{
			// ���� ��������� ����������� �� ���������� �������� �� ����,
			// ���������� ������, ���� ��� �������� ����������
			if (cchPerNote + cchEventText > MAX_SYMBOLS_PER_NOTE)
			{
				LOG("number of symbols per note exceeded, this cannot happen!\n");
				return false;
			}

			// �������� ����� �������� ����������� � ������������� �����
			wcsncpy(pwsNoteText + cchPerNote, pwsEventText, cchEventText);

			// ��������� ���������� �������� � ������������� ������
			cchPerNote += cchEventText;

			// ��������� ��������� �����������
			LyricRes = Lyric.GetNextValidEvent(&ctToLyricEvent, pwsEventText,
				&cchEventText);

			if (LyricRes == MIDILYRIC_LYRIC_END)
			{
				// ��� ����������� �������

				// ��������� � ��� ���� PrevNoteDesc � ������� � ������ pwsNoteText
				bool bEventAdded = pMidiSong->AddSingingEvent(PrevNoteDesc.NoteNumber,
					PrevNoteDesc.ctToNoteOn, PrevNoteDesc.ctDuration, pwsNoteText,
					cchPerNote);

				// ���� �� ������� �������� ������, ���������� ������
				if (!bEventAdded)
				{
					LOG("MidiSong::AddSingingEvent failed\n");
					return false;
				}

				// ���������� ��� ��� ���� �����, ������� ����� �������� � ���
				DWORD cNotesToAdd = MAX_NOTES_PER_METAEVENT - 1;

				// ������ ��������� ��������� ����������� � ��� ����
				DWORD ctToLastNoteOff = PrevNoteDesc.ctToNoteOn + PrevNoteDesc.ctDuration;

				// ��������� ����, ����������� � ���������� �����������,
				// ���� �� ����� ��������� ����������� �� ���������� ��� �� �����������
				while (cNotesToAdd > 0)
				{
					if (CurNoteDesc.ctToNoteOn - ctToLastNoteOff >
						m_cTicksPerMidiQuarterNote/2)
					{
						// ��������� ����������� �� ���������� ����� ������ � �������
						// ���� ������� ���, ����������� � ������ �����������
						break;
					}

					// ��������� � ��� ���� CurNoteDesc ��� ������
					bool bEventAdded = pMidiSong->AddSingingEvent(CurNoteDesc.NoteNumber,
						CurNoteDesc.ctToNoteOn, CurNoteDesc.ctDuration, NULL, 0);

					// ���� �� ������� �������� ������, ���������� ������
					if (!bEventAdded)
					{
						LOG("MidiSong::AddSingingEvent failed\n");
						return false;
					}

					// ������ ��������� ��������� ����������� � ��� ����
					ctToLastNoteOff = CurNoteDesc.ctToNoteOn + CurNoteDesc.ctDuration;

					// ������� ���� ���������� ����������
					PrevNoteDesc = CurNoteDesc;

					// ��������� ��������� ����
					PartRes = Part.GetNextNote(&CurNoteDesc);

					// ������������ ������ GetNextNote
					if (PartRes == MIDIPART_PART_END)
					{
						// ��� ���� ���������, ������� �� �����
						break;
					}
					else if (PartRes != MIDIPART_SUCCESS)
					{
						// ����� �������� ������ ������ MIDIPART_CANT_ALLOC_MEMORY;
						// ������ MIDIPART_COMPLICATED_NOTE_COMBINATION �
						// MIDIPART_OVERLAPS_THRESHOLD_EXCEEDED ����� ����������
						LOG("MidiPart::GetNextNote failed\n");
						return false;
					}

					// ��������� ���������� ��� ��� ���� �����,
					// ������� ����� �������� � ���
					cNotesToAdd--;
				}

				// ��� ������������
				return true;
			}
		}
		while (IsFirstNoteNearer(ctToLyricEvent, &PrevNoteDesc, &CurNoteDesc));

		// ��������� � ��� ���� PrevNoteDesc � ������� � ������ pwsNoteText
		bool bEventAdded = pMidiSong->AddSingingEvent(PrevNoteDesc.NoteNumber,
			PrevNoteDesc.ctToNoteOn, PrevNoteDesc.ctDuration, pwsNoteText, cchPerNote);

		// ���� �� ������� �������� ������, ���������� ������
		if (!bEventAdded)
		{
			LOG("MidiSong::AddSingingEvent failed\n");
			return false;
		}

		// ���������� ��� ��� ���� �����, ������� ����� �������� � ���
		DWORD cNotesToAdd = MAX_NOTES_PER_METAEVENT - 1;

		// ������ ��������� ��������� ����������� � ��� ����
		DWORD ctToLastNoteOff = PrevNoteDesc.ctToNoteOn + PrevNoteDesc.ctDuration;

		// ���� �� �����; ���� ����, ��������� � ���������� ���������� �����������
		while (true)
		{
			// ������� ���� ���������� ����������
			PrevNoteDesc = CurNoteDesc;

			// ��������� ��������� ����
			PartRes = Part.GetNextNote(&CurNoteDesc);

			// ������������ ������ GetNextNote
			if (PartRes == MIDIPART_PART_END)
			{
				// ����������, �������� ������� ���������� �������� �� ����
				DWORD cchPerNote = 0;

				// ������ ��������� ��������� ���� ������
				DWORD ctToLastNoteOff = PrevNoteDesc.ctToNoteOn + PrevNoteDesc.ctDuration;

				// ��������� �����������, ����������� � ��������� ����,
				// ���� �� ����� ��������� ����������� �� ���������� �������� �� ����
				while (cchPerNote + cchEventText <= MAX_SYMBOLS_PER_NOTE)
				{
					// �������� ����� �������� ����������� � ������������� �����
					wcsncpy(pwsNoteText + cchPerNote, pwsEventText, cchEventText);

					// ��������� ���������� �������� � ������������� ������
					cchPerNote += cchEventText;

					// ��������� ��������� �����������
					LyricRes = Lyric.GetNextValidEvent(&ctToLyricEvent, pwsEventText,
						&cchEventText);

					// ���� ��� ����������� �������, ������� �� �����
					if (LyricRes == MIDILYRIC_LYRIC_END) break;

					if (ctToLyricEvent > ctToLastNoteOff &&
						ctToLyricEvent - ctToLastNoteOff > m_cTicksPerMidiQuarterNote/2)
					{
						// ��������� ����������� �� ���������� ����� ������ ���������
						// ���� � �������� ������� �����������, ������������ � ���
						break;
					}
				}

				// ��������� � ��� ���� PrevNoteDesc � ������� � ������ pwsNoteText
				bool bEventAdded = pMidiSong->AddSingingEvent(PrevNoteDesc.NoteNumber,
					PrevNoteDesc.ctToNoteOn, PrevNoteDesc.ctDuration, pwsNoteText,
					cchPerNote);

				// ���� �� ������� �������� ������, ���������� ������
				if (!bEventAdded)
				{
					LOG("MidiSong::AddSingingEvent failed\n");
					return false;
				}

				// ��� ������������
				return true;
			}
			else if (PartRes != MIDIPART_SUCCESS)
			{
				// ����� �������� ������ ������ MIDIPART_CANT_ALLOC_MEMORY;
				// ������ MIDIPART_COMPLICATED_NOTE_COMBINATION �
				// MIDIPART_OVERLAPS_THRESHOLD_EXCEEDED ����� ����������
				LOG("MidiPart::GetNextNote failed\n");
				return false;
			}

			// ���� ��������� � �������� ����������� ���� �������, ������� �� �����
			if (IsFirstNoteNearer(ctToLyricEvent, &PrevNoteDesc, &CurNoteDesc)) break;

			if (cNotesToAdd > 0)
			{
				// ����������� �� ���������� ��� �� ����������� �� ���������

				if (PrevNoteDesc.ctToNoteOn - ctToLastNoteOff <=
					m_cTicksPerMidiQuarterNote/2)
				{
					// ����������� �� ���������� ����� ������ � ������� ���� ������� ���,
					// ����������� � ������ �����������, �� ���������

					// ��������� � ��� ���� PrevNoteDesc ��� ������
					bool bEventAdded = pMidiSong->AddSingingEvent(PrevNoteDesc.NoteNumber,
						PrevNoteDesc.ctToNoteOn, PrevNoteDesc.ctDuration, NULL, 0);

					// ���� �� ������� �������� ������, ���������� ������
					if (!bEventAdded)
					{
						LOG("MidiSong::AddSingingEvent failed\n");
						return false;
					}

					// ������ ��������� ��������� ����������� � ��� ����
					ctToLastNoteOff = PrevNoteDesc.ctToNoteOn + PrevNoteDesc.ctDuration;
				}

				// ��������� ���������� ��� ��� ���� �����, ������� ����� �������� � ���
				cNotesToAdd--;
			}
		}
	}
}

/****************************************************************************************
*
*   ����� CreateMeasureSequence
*
*   ���������
*       pMidiSong - ��������� �� ������ ������ MidiSong, ���������� ���
*
*   ������������ ��������
*       true, ���� ������������������ ������ ������� �������; false, ���� �� �������
*       �������� ������.
*
*   ������ ������������������ ������ � ������� ������ MidiSong.
*
****************************************************************************************/

bool MidiFile::CreateMeasureSequence(
	__inout MidiSong *pMidiSong)
{
	// ������������� ������� ������� �� ������ �������� �����, � ������� ����� ������
    // ����������� ���� TIME_SIGNATURE
	m_MidiTracks[0].ResetCurrentPosition();

	// ������� ��������� ������������ �������
	DWORD Numerator = 4;

	// ������� ����������� ������������ �������
	DWORD Denominator = 4;

	// ������� ���������� �������������� � ���������� MIDI-����
	DWORD cNotated32ndNotesPerMidiQuarterNote = 8;

	// ���������� ����� �� ������ ����� �� ����� ���������� ������������ �����
	double ctToEndOfLastMeasure = 0;

	// ������� ����� � �����, ��������� � ������ �����
	DWORD ctCurTime = 0;

	// ���� �� �������� �������� �����
    while (true)
	{
		// ������-����� ������� � �����
		DWORD ctDeltaTime;

		// ��������� �� ������ ���� ������ �������
		BYTE *pEventData;

		// ��������� ��������� ������� �� �������� �����
		DWORD Event = m_MidiTracks[0].GetNextEvent(&ctDeltaTime, &pEventData);

		if (Event == REAL_TRACK_END)
		{
			// � ����� ������ �� �������� �������

			// ������������� ������� ������� �� ������ ����� ��� ������� �� ���
			pMidiSong->ResetCurrentPosition();

			// ���������� ����� �� ������ ����� �� ��������� ����
			DWORD ctToNoteOn;

			// ������������ ��������� ���� � �����
			DWORD ctDuration;

			// ����� �� ����� ���
			while (pMidiSong->GetNextSingingEvent(NULL, &ctToNoteOn, &ctDuration,
				NULL, NULL));

			// ���������� ����� � �����
			double ctMeasure = (double) 32 * Numerator * m_cTicksPerMidiQuarterNote /
				(Denominator * cNotated32ndNotesPerMidiQuarterNote);

			// ���������� ������, ������� ���� �������� � ������������������ ������
			DWORD cMeasures = (DWORD) ceil((ctToNoteOn + ctDuration -
				ctToEndOfLastMeasure) / ctMeasure);

			// ��������� ����� � ������������������ ������
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

		// ��������� ������� ����� (����� � �����, ��������� � ������ �����)
		ctCurTime += ctDeltaTime;

		// ����������� ����� �������, ����� ����������� ���� TIME_SIGNATURE
		if (Event != TIME_SIGNATURE) continue;

		// ���������� ����� � �����
		double ctMeasure = (double) 32 * Numerator * m_cTicksPerMidiQuarterNote /
			(Denominator * cNotated32ndNotesPerMidiQuarterNote);

		// ���������� ������, ������� ���� �������� � ������������������ ������
		DWORD cMeasures = (DWORD) floor((ctCurTime - ctToEndOfLastMeasure) / ctMeasure);

		// ��������� ����� � ������������������ ������
		for (DWORD i = 0; i < cMeasures; i++)
		{
			if (!pMidiSong->AddMeasure(Numerator, Denominator,
				cNotated32ndNotesPerMidiQuarterNote))
			{
				LOG("MidiSong::AddMeasure failed\n");
				return false;
			}
		}

		// ����������� pEventData �� �����, ����������� ��������� ������������ �������
		pEventData++;

		// ��������� ��������� ������������ �������
		Numerator = pEventData[0];

		// ��������� ����������� ������������ �������
		Denominator = (DWORD) pow(2.0, pEventData[1]);

		// ��������� ���������� �������������� � ���������� MIDI-����
		cNotated32ndNotesPerMidiQuarterNote = pEventData[3];

		// ��������� ���������� ����� �� ������ ����� �� ����� ���������� ������������
		// �����
		ctToEndOfLastMeasure += cMeasures * ctMeasure;
	}
}

/****************************************************************************************
*
*   ����� CreateTempoMap
*
*   ���������
*       pMidiSong - ��������� �� ������ ������ MidiSong, ���������� ���
*
*   ������������ ��������
*       true, ���� ����� ������ ������� �������; false, ���� �� ������� �������� ������.
*
*   ������ ����� ������ � ������� ������ MidiSong.
*
****************************************************************************************/

bool MidiFile::CreateTempoMap(
	__inout MidiSong *pMidiSong)
{
	// ������������� ������� ������� �� ������ �������� �����, � ������� ����� ������
    // ����������� ���� SET_TEMPO
	m_MidiTracks[0].ResetCurrentPosition();

	// ������-����� ������� � �����
	DWORD ctDeltaTime;

	// ��������� �� ������ ���� ������ �������
	BYTE *pEventData;

	// ������� ����� � �����, ��������� � ������ �����
	DWORD ctCurTime = 0;

	// ���� �� �������� �������� �����; ���� ������ ����������� SET_TEMPO
    while (true)
	{
		// ��������� ��������� ������� �� �������� �����
		DWORD Event = m_MidiTracks[0].GetNextEvent(&ctDeltaTime, &pEventData);

		// ���� ������� ���������, �� �������
		if (Event == REAL_TRACK_END) return true;

		// ��������� ������� ����� (����� � �����, ��������� � ������ �����)
		ctCurTime += ctDeltaTime;

		// ����������� ����� �������, ����� ����������� ���� SET_TEMPO
		if (Event == SET_TEMPO) break;
	}

	// ����������, � ������� ����� ��������� ���������� ����� �� ������ ����� �� �������
	// ��������� �����, ����������� ������������ �� ���������� � ����� ������;
	// �������������� ����������� ����� �� ������ ����� �� ������� ������ ��������� �����
	DWORD ctToLastTempoSet = ctCurTime;

	// ����������� pEventData �� �����, ����������� ������� ���� ���������� �����������
	pEventData++;

	// ����������, � ������� ����� ��������� ������������ ���������� MIDI-����
	// � ������������� ��� �����, ����������� ������������ �� ���������� � ����� ������;
	// �������������� ����������� ����������� �� ���������� MIDI-���� � ������ �����
	DWORD cLastMicroseconds = pEventData[0] << 16 | pEventData[1] << 8 | pEventData[2];

	// ���� �� �������� �������� �����
    while (true)
	{
		// ��������� ��������� ������� �� �������� �����
		DWORD Event = m_MidiTracks[0].GetNextEvent(&ctDeltaTime, &pEventData);

		if (Event == REAL_TRACK_END)
		{
			// � ����� ������ �� �������� �������

			// ��������� ����-���������� � ����� ������
			if (!pMidiSong->AddTempo(ctToLastTempoSet, cLastMicroseconds))
			{
				LOG("MidiSong::AddTempo failed\n");
				return false;
			}

			return true;
		}

		// ��������� ������� ����� (����� � �����, ��������� � ������ �����)
		ctCurTime += ctDeltaTime;

		// ����������� ����� �������, ����� ����������� ���� SET_TEMPO
		if (Event != SET_TEMPO) continue;

		// ����������� pEventData �� �����, ����������� ������� ���� ����������
		// �����������
		pEventData++;

		// ��������� ���������� ����������� �� ���������� MIDI-���� � ������ ���
		// ��������� �����
		DWORD cMicroseconds = pEventData[0] << 16 | pEventData[1] << 8 | pEventData[2];

		if (ctToLastTempoSet == ctCurTime)
		{
			// ����� ������� �����, ����������� ������������ �� ���������� � �����
			// ������, � ������ ��� ���������� ����� ���������

			// ������������ �� ���������� ���������� ������ ��� ��������� ����
			cLastMicroseconds = cMicroseconds;

			// ���� ��������� ����������� SET_TEMPO
			continue;
		}

		// ���� ������ ��� ��������� ���� ��������� � ������������ �� ����������,
		// ����������� ���
		if (cLastMicroseconds == cMicroseconds) continue;

		// ��������� ����-���������� � ����� ������
		if (!pMidiSong->AddTempo(ctToLastTempoSet, cLastMicroseconds))
		{
			LOG("MidiSong::AddTempo failed\n");
			return false;
		}

		// ������������ �� ���������� � ����� ������ ���������� ������ ��� ��������� ����
		ctToLastTempoSet = ctCurTime;
		cLastMicroseconds = cMicroseconds;
	}
}
