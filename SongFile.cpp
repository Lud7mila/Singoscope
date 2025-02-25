/****************************************************************************************
*
*   ����������� ������ SongFile
*
*	������ ����� ������ ������������ ����� ���� � ������.
*
*   ������: ������� ����������� � ��������� �����������, 2008-2010
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
*   ��������� �������, ����������� ����
*
****************************************************************************************/

static LPCTSTR GetFileNameExtension(
	__in LPCTSTR pszFileName);

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

SongFile::SongFile()
{
	m_pMidiFile = NULL;
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

SongFile::~SongFile()
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
*       true, ���� ������� �������� �� ��������� ������� �����������; ����� false.
*
*   ������������� ������� �������� �� ��������� ��� ���� �����.
*
****************************************************************************************/

bool SongFile::SetDefaultCodePage(
	__in UINT DefaultCodePage)
{
	if (m_pMidiFile == NULL) return false;

	MIDIFILERESULT Result = m_pMidiFile->SetDefaultCodePage(DefaultCodePage);

	// ���� ����� SetDefaultCodePage ������� ���������, �� �������
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
*   ����� SetConcordNoteChoice
*
*   ���������
*       ConcordNoteChoice - ����������, ����� �� ��� �������� ����� ��������:
*                           CHOOSE_MIN_NOTE_NUMBER - ���� � ����������� �������,
*                           CHOOSE_MAX_NOTE_NUMBER - ���� � ������������ �������,
*                           CHOOSE_MAX_NOTE_DURATION - ���� � ������������ �������������
*
*   ������������ ��������
*       true, ���� �������� ������ ���� �� �������� ������� ����������; ����� false.
*
*   ������������� �������� ������ ���� �� ��������. ���� �������� ������������, �����
*   ��������� ������ �������� �������� � ���������� ������� ������ ���� ����.
*
****************************************************************************************/

bool SongFile::SetConcordNoteChoice(
	__in CONCORD_NOTE_CHOICE ConcordNoteChoice)
{
	if (m_pMidiFile == NULL) return false;

	MIDIFILERESULT Result = m_pMidiFile->SetConcordNoteChoice(ConcordNoteChoice);

	// ���� ����� SetConcordNoteChoice ������� ���������, �� �������
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
*   ����� LoadFile
*
*   ���������
*       pszFileName - ��������� �� ������, ����������� �����, � ������� ������� ���
*					  ����� � ������
*       DefaultCodePage - ������� �������� �� ��������� ��� ���� �����
*       ConcordNoteChoice - ����������, ����� �� ��� �������� ����� ��������:
*                           CHOOSE_MIN_NOTE_NUMBER - ���� � ����������� �������,
*                           CHOOSE_MAX_NOTE_NUMBER - ���� � ������������ �������,
*                           CHOOSE_MAX_NOTE_DURATION - ���� � ������������ �������������
*
*   ������������ ��������
*       true, ���� ���� � ������ ������� ��������; ��� false, ���� ��������� ������.
*
*   ��������� ���� � ������.
*
****************************************************************************************/

bool SongFile::LoadFile(
	__in LPCTSTR pszFileName,
	__in UINT DefaultCodePage,
	__in CONCORD_NOTE_CHOICE ConcordNoteChoice)
{
	// ���� � ���� ������ ��� ��� �������� �����-�� ����, ����������� ��� ����������
	// ��� ����� ����� �������
	Free();

	// ��������� ���� � ������
	HANDLE hFile = CreateFile(pszFileName, GENERIC_READ, FILE_SHARE_READ, NULL,
		OPEN_EXISTING, 0, NULL);

	if (hFile == INVALID_HANDLE_VALUE)
	{
		LOG("CreateFile failed (error %u)\n", GetLastError());
		ShowError(MSGID_CANT_OPEN_FILE, GetLastError());
		return false;
	}

	// �������� ������ ����� � ������ � ������
	DWORD cbFile = GetFileSize(hFile, NULL);

	if (cbFile == INVALID_FILE_SIZE)
	{
		LOG("GetFileSize failed (error %u)\n", GetLastError());
		ShowError(MSGID_CANT_LOAD_FILE, FUNCID_GET_FILE_SIZE, GetLastError());
		CloseHandle(hFile);
		return false;
	}

	// �������� �����, � ������� ����� �������� ���� � ������
	PBYTE pFile = (PBYTE) HeapAlloc(GetProcessHeap(), 0, cbFile);

	if (pFile == NULL)
	{
		LOG("HeapAlloc failed\n");
		ShowError(MSGID_CANT_ALLOC_MEMORY);
		CloseHandle(hFile);
		return false;
	}

	DWORD cbRead;

	// ��������� ���������� ����� � ������ � �����
	if (!ReadFile(hFile, pFile, cbFile, &cbRead, NULL))
	{
		LOG("ReadFile failed (error %u)\n", GetLastError());
		ShowError(MSGID_CANT_LOAD_FILE, FUNCID_READ_FILE, GetLastError());
		HeapFree(GetProcessHeap(), 0, pFile);
		CloseHandle(hFile);
		return false;
	}

	// ��������� ���� � ������
	CloseHandle(hFile);

	// ������ ������ ������ MidiFile
	m_pMidiFile = new MidiFile;

	if (m_pMidiFile == NULL)
	{
		LOG("operator new failed\n");
		ShowError(MSGID_CANT_ALLOC_MEMORY);
		HeapFree(GetProcessHeap(), 0, pFile);
		return false;
	}

	// ������������� ������� �������� �� ��������� ��� ���� �����
	m_pMidiFile->SetDefaultCodePage(DefaultCodePage);

	// ������������� �������� ������ ���� �� ��������
	m_pMidiFile->SetConcordNoteChoice(ConcordNoteChoice);

	// ��������� ������� ����������� � ������ ���� � ������
	MIDIFILERESULT Result = m_pMidiFile->AssignFile(pFile, cbFile);

	// ���� ����� AssignFile ������� ���������, �� �������
	if (Result == MIDIFILE_SUCCESS) return true;

	// ���� ����� AssignFile ���������� � �������, �� �� ������ ���������� �����
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
*   ����� CreateSong
*
*   ���������
*       QuantizeStepDenominator - ����������� �����, ���������� ������� �������� �������,
*                                 �������������� ����� ��� ����� �����������
*
*   ������������ ��������
*       ��������� �� ��������� ������ ������ Song; ��� NULL, ���� �� ������� ��������
*       ������.
*
*   C����� ������ ������ Song, �������������� ����� �����.
*
****************************************************************************************/

Song *SongFile::CreateSong(
	__in DWORD QuantizeStepDenominator)
{
	// ����������� �����, �������������� ����� ������� ��� ����� �����������
	DWORD CurQuantizeStepDenominator = QuantizeStepDenominator;

	// ��������� �� ������ ������ Song
	Song *pSong = NULL;

	// ���� �� �������� ���� ����� �����������;
	// �� ������ �������� ��������� ��� � ��� ����
	do
	{
		// ������� ������ ������ Song, ��������� �� ���������� ��������
		if (pSong != NULL) delete pSong;

		// ������ �������� ������ ������ Song
		pSong = m_pMidiFile->CreateSong();

		if (pSong == NULL)
		{
			LOG("MidiFile::CreateSong failed\n");
			ShowError(MSGID_CANT_ALLOC_MEMORY);
			return NULL;
		}

		// ������ ����������� ��� � ��� � ������� �� �����, ���� �� ������������
		// �� ����� ���� � ������� �������������
		if (pSong->Quantize(CurQuantizeStepDenominator)) break;

		// � �������� ����������� ��������� ���� � ������� �������������, ����������� ��;
		// ���� ��� ���� � ������� ������������� ������� ���������, �� ������� �� �����
		if (pSong->ExtendEmptyNotes(CurQuantizeStepDenominator)) break;

		CurQuantizeStepDenominator *= 2;
	}
	while (CurQuantizeStepDenominator <= 256);

	// ������ ������ � ����� ������ ���� ����� � ���
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
*   ������� GetFileNameExtension
*
*   ���������
*       pszFileName - ��������� �� ������, ����������� �����, � ������� ������� ��� �����
*
*   ������������ ��������
*       ��������� �� ���������� ����� ����� ��� NULL, ���� ��� ���.
*
*   ���������� ��������� �� ���������� ����� �����.
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
