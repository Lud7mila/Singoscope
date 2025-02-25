/****************************************************************************************
*
*   ����������� ������ StaveWnd
*
*   �������� ���� ��������� ����, ������������ ������ ����.
*
*   �����: ������� ������������ � ��������� �����������, 2007-2010
*
****************************************************************************************/

#define _CRT_SECURE_NO_DEPRECATE

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
#include "Main.h"
#include "FrameWnd.h"
#include "ScrollbarWnd.h"
#include "SimpleStaveFast.h"
#include "Resources.h"

/****************************************************************************************
*
*   ���������� ����������
*
****************************************************************************************/

// ��� ������ ���� ������� �����
static const TCHAR g_pszStaveWndClassName[] = TEXT("Singoscope Stave Window Class");

// ��������� �� ������ ��� ������� GetOpenFileName
static LPTSTR g_pszOpenFileFilter = NULL;

// ��������� ���� ������� �����
static HWND g_hwndStave;

// ������ ���� ������� �����
static int g_StaveWndWidth;

// ������ ���� ������� �����
static int g_StaveWndHeight;

// ������ ������ ��������� � ��������
static int g_ScrollbarHeight;

// ��������� ���� ������ ���������
static HWND g_hwndScrollbar;

// ������� �������� �� ��������� ��� ���� �����
static UINT g_DefaultCodePage = CP_ACP;

// �������� ������ ���� �� ��������
static CONCORD_NOTE_CHOICE g_ConcordNoteChoice = CHOOSE_MIN_NOTE_NUMBER;

// ����������� �����, �������������� ����� ��� ����� �����������
static DWORD g_QuantizeStepDenominator = 32;

// ��������� �� ������� ������ ������ SongFile
static SongFile *g_pSongFile = NULL;

// ��������� �� ������� ������ ������ Song
static Song *g_pSong = NULL;

/****************************************************************************************
*
*   ��������� �������, ����������� ����
*
****************************************************************************************/

static LRESULT CALLBACK StaveWndProc(
	HWND hwnd,
	UINT uMsg,
	WPARAM wParam,
	LPARAM lParam);

static bool ActivateCurrentStaveDrawer();

static void DeactivateCurrentStaveDrawer();

static void OpenFile();

static bool GetSongFileName(
	__out LPTSTR pszFileName,
	__in DWORD cchFileName);

/****************************************************************************************
*
*   ������� StaveWnd_Init
*
*   ���������
*       ���
*
*   ������������ ��������
*       true, ���� ������ ������� �������������; ����� false.
*
*   �������������� ������.
*
****************************************************************************************/

bool StaveWnd_Init()
{
	LPCTSTR pszAllSupportedFiles = TextMessages_GetMessage(MSGID_ALL_SUPPORTED_FILES);
	LPCTSTR pszMidiFiles = TextMessages_GetMessage(MSGID_MIDI_FILES);
	LPCTSTR pszKaraokeFiles = TextMessages_GetMessage(MSGID_KARAOKE_FILES);
	LPCTSTR pszAllFiles = TextMessages_GetMessage(MSGID_ALL_FILES);

	DWORD cchAllSupportedFiles = _tcslen(pszAllSupportedFiles) + 1;
	DWORD cchMidiFiles = _tcslen(pszMidiFiles) + 1;
	DWORD cchKaraokeFiles = _tcslen(pszKaraokeFiles) + 1;
	DWORD cchAllFiles = _tcslen(pszAllFiles) + 1;

	const TCHAR pszAllSupportedFilesMask[] = TEXT("*.mid;*.midi;*.rmi;*.kar");
	const TCHAR pszMidiFilesMask[] = TEXT("*.mid;*.midi;*.rmi");
	const TCHAR pszKaraokeFilesMask[] = TEXT("*.kar");
	const TCHAR pszAllFilesMask[] = TEXT("*.*");

	const DWORD cchAllSupportedFilesMask = sizeof(pszAllSupportedFilesMask) / sizeof(TCHAR);
	const DWORD cchMidiFilesMask = sizeof(pszMidiFilesMask) / sizeof(TCHAR);
	const DWORD cchKaraokeFilesMask = sizeof(pszKaraokeFilesMask) / sizeof(TCHAR);
	const DWORD cchAllFilesMask = sizeof(pszAllFilesMask) / sizeof(TCHAR);

	DWORD cbOpenFileFilter = (cchAllSupportedFiles + cchMidiFiles + cchKaraokeFiles +
		cchAllFiles + cchAllSupportedFilesMask + cchMidiFilesMask + cchKaraokeFilesMask +
		cchAllFilesMask + 1) * sizeof(TCHAR);

	g_pszOpenFileFilter = (LPTSTR) HeapAlloc(GetProcessHeap(), 0, cbOpenFileFilter);

	if (g_pszOpenFileFilter == NULL)
	{
		LOG("HeapAlloc failed\n");
		ShowFatalError(MSGID_CANT_ALLOC_MEMORY);
		return false;
	}

	LPTSTR pszCurrentString = g_pszOpenFileFilter;

	_tcscpy(pszCurrentString, pszAllSupportedFiles);
	pszCurrentString += cchAllSupportedFiles;
	_tcscpy(pszCurrentString, pszAllSupportedFilesMask);
	pszCurrentString += cchAllSupportedFilesMask;
	_tcscpy(pszCurrentString, pszMidiFiles);
	pszCurrentString += cchMidiFiles;
	_tcscpy(pszCurrentString, pszMidiFilesMask);
	pszCurrentString += cchMidiFilesMask;
	_tcscpy(pszCurrentString, pszKaraokeFiles);
	pszCurrentString += cchKaraokeFiles;
	_tcscpy(pszCurrentString, pszKaraokeFilesMask);
	pszCurrentString += cchKaraokeFilesMask;
	_tcscpy(pszCurrentString, pszAllFiles);
	pszCurrentString += cchAllFiles;
	_tcscpy(pszCurrentString, pszAllFilesMask);
	pszCurrentString[cchAllFilesMask] = 0;

	WNDCLASS wc;

	ZeroMemory(&wc, sizeof(wc));

	wc.lpfnWndProc = StaveWndProc;
	wc.hInstance = g_hInstance;
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.lpszClassName = g_pszStaveWndClassName;

	if (RegisterClass(&wc) == 0)
	{
		LOG("RegisterClass failed (error %u)\n", GetLastError());
		ShowFatalError(MSGID_CANT_INIT_PROGRAM, FUNCID_REGISTER_CLASS, GetLastError());
		return false;
	}

	return true;
}

/****************************************************************************************
*
*   ������� StaveWnd_Uninit
*
*   ���������
*       ���
*
*   ������������ ��������
*       ���
*
*   ���������������� ������.
*
****************************************************************************************/

void StaveWnd_Uninit()
{
	if (g_pszOpenFileFilter != NULL)
	{
		HeapFree(GetProcessHeap(), 0, g_pszOpenFileFilter);
	}
}

/****************************************************************************************
*
*   ������� StaveWnd_Create
*
*   ���������
*       x - x-���������� ������ �������� ���� ����
*       y - y-���������� ������ �������� ���� ����
*       nWidth - ������ ����
*       nHeight - ������ ����
*       hwndParent - ��������� ������������� ����
*
*   ������������ ��������
*       ��������� ���������� ���� ������� �����.
*
*   ������ ���� ������� �����.
*
****************************************************************************************/

HWND StaveWnd_Create(
	__in int x,
	__in int y,
	__in int nWidth,
	__in int nHeight,
	__in HWND hwndParent)
{
	g_StaveWndWidth = nWidth;
	g_StaveWndHeight = nHeight;
	g_ScrollbarHeight = ScrollbarWnd_GetHeight();

	g_hwndStave = CreateWindow(g_pszStaveWndClassName, NULL,
		WS_CHILD | WS_CLIPCHILDREN | WS_VISIBLE, x, y, nWidth, nHeight,
		hwndParent, NULL, g_hInstance, NULL);

	if (g_hwndStave == NULL)
	{
		LOG("CreateWindow failed (error %u)\n", GetLastError());
		ShowFatalError(MSGID_CANT_INIT_PROGRAM, FUNCID_CREATE_WINDOW, GetLastError());
		return NULL;
	}

	g_hwndScrollbar = ScrollbarWnd_Create(0, nHeight - g_ScrollbarHeight,
		nWidth, g_hwndStave);

	if (g_hwndScrollbar == NULL)
	{
		LOG("ScrollbarWnd_Create failed\n");
		DestroyWindow(g_hwndStave);
		return false;
	}

	return g_hwndStave;
}

/****************************************************************************************
*
*   ������� StaveWndProc
*
*   ��. �������� ������� WindowProc � MSDN.
*
****************************************************************************************/

static LRESULT CALLBACK StaveWndProc(
	HWND hwnd,
	UINT uMsg,
	WPARAM wParam,
	LPARAM lParam)
{
	switch(uMsg)
	{
		case WM_CREATE:
		{
			if (!ActivateCurrentStaveDrawer())
			{
				LOG("ActivateCurrentStaveDrawer failed\n");
				return -1;
			}

			return 0;
		}

		case WM_SIZE:
		{
			g_StaveWndWidth = LOWORD(lParam);
			g_StaveWndHeight = HIWORD(lParam);

			int StaveHeight = g_StaveWndHeight - g_ScrollbarHeight;

			if (StaveHeight < 0) StaveHeight = 0;

			SimpleStaveFast_SetStaveSize(g_StaveWndWidth, StaveHeight);

			SetWindowPos(g_hwndScrollbar, NULL, 0, g_StaveWndHeight - g_ScrollbarHeight,
				g_StaveWndWidth, g_ScrollbarHeight, SWP_NOZORDER);

			InvalidateRect(hwnd, NULL, FALSE);

			return 0;
		}

		case WM_PAINT:
		{
			PAINTSTRUCT ps;

			BeginPaint(hwnd, &ps);

			SimpleStaveFast_GlobalDraw(hwnd);

			EndPaint(hwnd, &ps);

			return 0;
		}

		case WM_COMMAND:
		{
			switch (LOWORD(wParam))
			{
				case IDC_OPENFILE:
				{
					OpenFile();
					InvalidateRect(hwnd, NULL, FALSE);
					return 0;
				}
			}

			break;
		}

		case WM_DESTROY:
		{
			DeactivateCurrentStaveDrawer();
			return 0;
		}
	}

	return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

/****************************************************************************************
*
*   ������� ActivateCurrentStaveDrawer
*
*   ���������
*       ���
*
*   ������������ ��������
*       true, ���� ������� ���������� ������� ����� ������� �����������; ����� false.
*
*   ���������� ������� ���������� ������� �����.
*
****************************************************************************************/

static bool ActivateCurrentStaveDrawer()
{
	return SimpleStaveFast_Activate(g_StaveWndWidth,
		g_StaveWndHeight - g_ScrollbarHeight, g_pSong);
}

/****************************************************************************************
*
*   ������� DeactivateCurrentStaveDrawer
*
*   ���������
*       ���
*
*   ������������ ��������
*       ���
*
*   ������������ ������� ���������� ������� �����.
*
****************************************************************************************/

static void DeactivateCurrentStaveDrawer()
{
	SimpleStaveFast_Deactivate();
}

/****************************************************************************************
*
*   ������� OpenFile
*
*   ���������
*       ���
*
*   ������������ ��������
*       ���
*
*   ������������ ������� IDC_OPENFILE (������� ����).
*
****************************************************************************************/

static void OpenFile()
{
	TCHAR pszFileName[MAX_PATH];

	// ����������� � ������������ ��� ����� � ������
	if (!GetSongFileName(pszFileName, sizeof(pszFileName) / sizeof(TCHAR)))
	{
		return;
	}

	// ������ ������ ������ SongFile
	SongFile *pSongFile = new SongFile;

	if (pSongFile == NULL)
	{
		LOG("operator new failed\n");
		ShowError(MSGID_CANT_ALLOC_MEMORY);
		return;
	}

	// ��������� ���� � ������
	bool bIsFileLoaded = pSongFile->LoadFile(pszFileName,
		g_DefaultCodePage, g_ConcordNoteChoice);

	// ���� �� ������� ��������� ����, �������
	if (!bIsFileLoaded)
	{
		LOG("SongFile::LoadFile failed\n");
		delete pSongFile;
		return;
	}

	// ������ ������ ������ Song
	Song *pSong = pSongFile->CreateSong(g_QuantizeStepDenominator);

	if (pSong == NULL)
	{
		LOG("SongFile::CreateSong failed\n");
		delete pSongFile;
		return;
	}

	// ������� ������ ������ ������ SongFile
	if (g_pSongFile != NULL) delete g_pSongFile;

	// ������� ������ ������ ������ Song
	if (g_pSong != NULL) delete g_pSong;

	// ��������� ������ ������ SongFile ���������� �������
	g_pSongFile = pSongFile;

	// ��������� ������ ������ Song ���������� �������
	g_pSong = pSong;

	// ���������� ������� ���������� ������� ����� � ����� ������
	ActivateCurrentStaveDrawer();
}

/****************************************************************************************
*
*   ������� GetSongFileName
*
*   ���������
*       pszFileName - ��������� �� �����, � ������� ������������ ��� ����������
*                     ������������� �����
*       cchFileName - ������ ������ � ��������
*
*   ������������ ��������
*       true, ���� ������������ ������� ������ ��� ����� � ������; false, ����
*       ������������ ����� ������ ������ ���� ��������� �����-�� ������.
*
*   ���������� ������������ ���������� ����, ����������� ������� ��� ����� � ������.
*
****************************************************************************************/

static bool GetSongFileName(
	__out LPTSTR pszFileName,
	__in DWORD cchFileName)
{
	pszFileName[0] = 0;

	OPENFILENAME ofn;

	ZeroMemory(&ofn, sizeof(ofn));

	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = g_hwndFrame;
	ofn.lpstrFilter = g_pszOpenFileFilter;
	ofn.nFilterIndex = 1;
	ofn.lpstrFile = pszFileName;
	ofn.nMaxFile = cchFileName;
	ofn.Flags = OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;

	if (!GetOpenFileName(&ofn))
	{
		return false;
	}

	return true;
}
