/****************************************************************************************
*
*   ����������� ������ SimpleStaveFast
*
*   ������ ������ ���� � ���������� ������ (������� ������� ���������).
*
*   ������: ��������� ����������� � ������� ������������, 2010
*
****************************************************************************************/

#include <windows.h>

#include "Log.h"
#include "TextMessages.h"
#include "ShowError.h"
#include "Song.h"
#include "Video.h"
#include "SimpleStaveFast.h"

/****************************************************************************************
*
*   ���������� ����������
*
****************************************************************************************/

// ������ ������� �����
static DWORD g_StaveWidth = 0;

// ������ ������� �����
static DWORD g_StaveHeight = 0;

// ������ ������
static DWORD g_ScreenWidth = 0;

// ������ ������
static DWORD g_ScreenHeight = 0;

// ��������� ������� �����������
static HANDLE g_hBkgndSurface = NULL;

// ��������� ����������� ��� ���������������
static HANDLE g_hRectSurface = NULL;

// ���� ������� ����� ���� (������� ���� - ������� ����) [������� �������� �����]
static COLORREF g_crBkgndTop = 0x6496FF;

// ���� ������ ����� ���� (������� ���� - ������� ����) [������� �������� �����]
static COLORREF g_crBkgndBottom = 0x2D4474;

/*
// ���� ������� ����� ���� (������� ���� - ������� ����) [������ �������� �����]
static COLORREF g_crBkgndTop = 0xAFFF64;

// ���� ������ ����� ���� (������� ���� - ������� ����) [������ �������� �����]
static COLORREF g_crBkgndBottom = 0x4F742D;

// ���� ������� ����� ���� (������� ���� - ������� ����) [����� �������� �����]
static COLORREF g_crBkgndTop = 0xFFAF64;

// ���� ������ ����� ���� (������� ���� - ������� ����) [����� �������� �����]
static COLORREF g_crBkgndBottom = 0x744F2D;
*/

/****************************************************************************************
*
*   ��������� �������, ����������� ����
*
****************************************************************************************/

static bool CreateSurfaces();

static void DeleteSurfaces();

static bool DrawSurfaces();

static bool DrawBackground();

/****************************************************************************************
*
*   ������� SimpleStaveFast_Activate
*
*   ���������
*       StaveWidth - ������ ������� �����
*       StaveHeight - ������ ������� �����
*       pSong - ��������� �� ������ ������ Song, �������������� ����� �����; ����
*               �������� ����� ���� ����� NULL
*
*   ������������ ��������
*       true, ���� ������ ������� �����������; ����� false.
*
*   ���������� ������.
*
****************************************************************************************/

bool SimpleStaveFast_Activate(
	__in int StaveWidth,
	__in int StaveHeight,
	__in_opt Song *pSong)
{
	SimpleStaveFast_Deactivate();

	g_StaveWidth = StaveWidth;
	g_StaveHeight = StaveHeight;

	if (!CreateSurfaces())
	{
		LOG("CreateSurfaces failed\n");
		return false;
	}

	if (!DrawSurfaces())
	{
		LOG("DrawSurfaces failed\n");
		return false;
	}
/*
	if (pSong != NULL)
	{
		pSong->ResetCurrentPosition();

		while (true)
		{
			DWORD NoteNumber;
			double PauseLength;
			double NoteLength;
			LPCWSTR pwsNoteText;
			DWORD cchNoteText;

			bool bEndOfSes = !pSong->GetNextSingingEvent(&NoteNumber, &PauseLength,
				&NoteLength, &pwsNoteText, &cchNoteText);

			if (bEndOfSes) break;

			LOG("�%u [%lf, %lf]   \"", NoteNumber, PauseLength, NoteLength);
			if (pwsNoteText == NULL || cchNoteText == 0)
			{
				LOG("...");
			}
			else
			{
				for (DWORD i = 0; i < cchNoteText - 1; i++) LOG("%wc", pwsNoteText[i]);

				if (pwsNoteText[cchNoteText-1] != L' ') LOG("%wc", pwsNoteText[cchNoteText-1]);
			}
			if (NoteLength > 0) LOG("\"\n");
			else LOG("\" ������ ����!!!\n");
		}
	}
*/
	return true;
}

/****************************************************************************************
*
*   ������� SimpleStaveFast_Deactivate
*
*   ���������
*       ���
*
*   ������������ ��������
*       ���
*
*   ������������ ������.
*
****************************************************************************************/

void SimpleStaveFast_Deactivate()
{
	DeleteSurfaces();

	g_StaveWidth = 0;
	g_StaveHeight = 0;
	g_ScreenWidth = 0;
	g_ScreenHeight = 0;
}

/****************************************************************************************
*
*   ������� SimpleStaveFast_SetStaveSize
*
*   ���������
*       StaveWidth - ������ ������� �����
*       StaveHeight - ������ ������� �����
*
*   ������������ ��������
*       ���
*
*   ������������� ����� ������ � ������ ������� �����.
*
****************************************************************************************/

void SimpleStaveFast_SetStaveSize(
	__in int StaveWidth,
	__in int StaveHeight)
{
	g_StaveWidth = StaveWidth;
	g_StaveHeight = StaveHeight;

	if (g_StaveWidth > g_ScreenWidth || g_StaveHeight > g_ScreenHeight)
	{
		DeleteSurfaces();

		if (!CreateSurfaces())
		{
			LOG("CreateSurfaces failed\n");
			return;
		}
	}

	if (!DrawSurfaces())
	{
		LOG("DrawSurfaces failed\n");
	}
}

/****************************************************************************************
*
*   ������� SimpleStaveFast_GlobalDraw
*
*   ���������
*       hwnd - ��������� ����, ������� �������� � ���� ������ ����
*
*   ������������ ��������
*       ���
*
*   ��������� �������������� ������ ���� � ������������ � ��� ������� ����������.
*
****************************************************************************************/

void SimpleStaveFast_GlobalDraw(
	__in HWND hwnd)
{
	VIDEORESULT vr = Video_CheckScreenStatus();

	if (vr == VIDEO_FAIL)
	{
		LOG("Video_CheckScreenStatus failed\n");
		return;
	}
	else if (vr == VIDEO_SCREEN_PIXEL_FORMAT_CHANGED)
	{
		DeleteSurfaces();

		if (!CreateSurfaces())
		{
			LOG("CreateSurfaces failed\n");
			return;
		}

		if (!DrawSurfaces())
		{
			LOG("DrawSurfaces failed\n");
			return;
		}
	}

	POINT pt = {0, 0};

	ClientToScreen(hwnd, &pt);

	RECT rc = {0, 0, g_StaveWidth, g_StaveHeight};

	vr = Video_BltRectToScreen(pt.x, pt.y, g_hBkgndSurface, &rc);

	if (vr != VIDEO_SUCCESS)
	{
		LOG("Video_BltRectToScreen failed (error %d)\n", vr);
		return;
	}
}

/****************************************************************************************
*
*   ������� SimpleStaveFast_LocalDraw
*
*   ���������
*       hwnd - ��������� ����, ������� �������� � ���� ������ ����
*
*   ������������ ��������
*       ���
*
*   �������������� ������ ���� �� ����� ����� � ������������ � ��� ������� ���������� �
*   ��������� ��� ���������.
*
****************************************************************************************/

void SimpleStaveFast_LocalDraw(
	__in HWND hwnd)
{
}

/****************************************************************************************
*
*   ������� CreateSurfaces
*
*   ���������
*       ���
*
*   ������������ ��������
*       true, ���� ��� ����������� ������� �������; ����� false.
*
*   ������ � ������������ � ����������� ������ ��������� �����������: �������, ���
*   ��������������� � ��� �������. �������� ���������� ���������� g_ScreenWidth �
*   g_ScreenHeight.
*
****************************************************************************************/

static bool CreateSurfaces()
{
	if (!Video_GetScreenResolution(&g_ScreenWidth, &g_ScreenHeight))
	{
		LOG("Video_GetScreenResolution failed\n");
		return false;
	}

	g_hBkgndSurface = Video_CreateSurface(g_ScreenWidth, g_ScreenHeight,
		MEMTYPE_SYSTEM_MEMORY);

	if (g_hBkgndSurface == NULL)
	{
		LOG("Video_CreateSurface for background failed\n");
		return false;
	}

	return true;
}

/****************************************************************************************
*
*   ������� DeleteSurfaces
*
*   ���������
*       ���
*
*   ������������ ��������
*       ���
*
*   ������� ��������� �����������: �������, ��� ��������������� � ��� �������.
*
****************************************************************************************/

static void DeleteSurfaces()
{
	if (g_hBkgndSurface != NULL)
	{
		Video_DeleteSurface(g_hBkgndSurface);
		g_hBkgndSurface = NULL;
	}
}

/****************************************************************************************
*
*   ������� DrawSurfaces
*
*   ���������
*       ���
*
*   ������������ ��������
*       true, ���� ��� ����������� ������� ����������; ����� false.
*
*   ������������ ��������� �����������: �������, ��� ��������������� � ��� �������.
*
****************************************************************************************/

static bool DrawSurfaces()
{
	if (!DrawBackground())
	{
		LOG("DrawBackground failed\n");
		return false;
	}

	return true;
}

/****************************************************************************************
*
*   ������� DrawBackground
*
*   ���������
*       ���
*
*   ������������ ��������
*       true, ���� ��� ������� ���������; ����� false.
*
*   ������ ��� �� ������� �����������.
*
****************************************************************************************/

static bool DrawBackground()
{
	if (g_hBkgndSurface == NULL) return false;

	HDC hdc = Video_GetDC(g_hBkgndSurface);

	if (hdc == NULL)
	{
		LOG("Video_GetDC failed\n");
		return false;
	}

    TRIVERTEX vertex[2];

	vertex[0].x = 0;
	vertex[0].y = 0;
	vertex[0].Red = GetRValue(g_crBkgndTop) << 8;
	vertex[0].Green = GetGValue(g_crBkgndTop) << 8;
	vertex[0].Blue = GetBValue(g_crBkgndTop) << 8;
	vertex[0].Alpha = 0;

	vertex[1].x = g_StaveWidth;
	vertex[1].y = g_StaveHeight;
	vertex[1].Red = GetRValue(g_crBkgndBottom) << 8;
	vertex[1].Green = GetGValue(g_crBkgndBottom) << 8;
	vertex[1].Blue = GetBValue(g_crBkgndBottom) << 8;
	vertex[1].Alpha = 0;

	GRADIENT_RECT grc = {0, 1};

	if (!GradientFill(hdc, vertex, 2, &grc, 1, GRADIENT_FILL_RECT_V))
	{
		LOG("GradientFill failed (error %u)\n", GetLastError());
		ShowFatalError(MSGID_GRADIENT_FILL_FAILED, GetLastError());
		Video_ReleaseDC(g_hBkgndSurface, hdc);
		return false;
	}

	Video_ReleaseDC(g_hBkgndSurface, hdc);

	return true;
}
