/****************************************************************************************
*
*   ����������� ������ ScrollbarWnd
*
*   ������ ��������� ��� ������� ����� - �������� ���� ���� ������� �����.
*
*   �����: ��������� �����������, 2010
*
****************************************************************************************/

#include <windows.h>

#include "Log.h"
#include "TextMessages.h"
#include "ShowError.h"
#include "Main.h"

/****************************************************************************************
*
*   ���������� ����������
*
****************************************************************************************/

// ��� ������ ���� ������ ���������
static const TCHAR g_pszScrollbarWndClassName[] = TEXT("Singoscope Scrollbar Window Class");

// ��������� ���� ������ ���������
static HWND g_hwndScrollbar;

// ������ ���� ������ ���������
static int g_ScrollbarWidth;

// ������ ���� ������ ���������
static const int g_ScrollbarHeight = 16;

/****************************************************************************************
*
*   ��������� �������, ����������� ����
*
****************************************************************************************/

static LRESULT CALLBACK ScrollbarWndProc(
	HWND hwnd,
	UINT uMsg,
	WPARAM wParam,
	LPARAM lParam);

static void DrawScrollbar(
	__in HWND hwnd);

/****************************************************************************************
*
*   ������� ScrollbarWnd_Init
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

bool ScrollbarWnd_Init()
{
	WNDCLASS wc;

	ZeroMemory(&wc, sizeof(wc));

	wc.lpfnWndProc = ScrollbarWndProc;
	wc.hInstance = g_hInstance;
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.lpszClassName = g_pszScrollbarWndClassName;

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
*   ������� ScrollbarWnd_Uninit
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

void ScrollbarWnd_Uninit()
{
}

/****************************************************************************************
*
*   ������� ScrollbarWnd_GetHeight
*
*   ���������
*       ���
*
*   ������������ ��������
*       ������ ������ ��������� � ��������.
*
*   ���������� ������ ������ ��������� � ��������.
*
****************************************************************************************/

int ScrollbarWnd_GetHeight()
{
	return g_ScrollbarHeight;
}

/****************************************************************************************
*
*   ������� ScrollbarWnd_Create
*
*   ���������
*       x - x-���������� ������ �������� ���� ����
*       y - y-���������� ������ �������� ���� ����
*       nWidth - ������ ����
*       hwndParent - ��������� ������������� ����
*
*   ������������ ��������
*       ��������� ���������� ���� ������ ���������.
*
*   ������ ���� ������ ���������.
*
****************************************************************************************/

HWND ScrollbarWnd_Create(
	__in int x,
	__in int y,
	__in int nWidth,
	__in HWND hwndParent)
{
	g_ScrollbarWidth = nWidth;

	g_hwndScrollbar = CreateWindow(g_pszScrollbarWndClassName, NULL,
		WS_CHILD | WS_VISIBLE, x, y, nWidth, g_ScrollbarHeight,
		hwndParent, NULL, g_hInstance, NULL);

	if (g_hwndScrollbar == NULL)
	{
		LOG("CreateWindow failed (error %u)\n", GetLastError());
		ShowFatalError(MSGID_CANT_INIT_PROGRAM, FUNCID_CREATE_WINDOW, GetLastError());
		return NULL;
	}

	return g_hwndScrollbar;
}

/****************************************************************************************
*
*   ������� ScrollbarWndProc
*
*   ��. �������� ������� WindowProc � MSDN.
*
****************************************************************************************/

static LRESULT CALLBACK ScrollbarWndProc(
	HWND hwnd,
	UINT uMsg,
	WPARAM wParam,
	LPARAM lParam)
{
	switch(uMsg)
	{
		case WM_SIZE:
		{
			g_ScrollbarWidth = LOWORD(lParam);
			return 0;
		}

		case WM_PAINT:
		{
			PAINTSTRUCT ps;

			BeginPaint(hwnd, &ps);

			RECT rc; // ������!!!
			GetClientRect(hwnd, &rc); // ������!!!
			FillRect(ps.hdc, &rc, (HBRUSH) (COLOR_SCROLLBAR + 1)); // ������!!!
			Rectangle(ps.hdc, 2, 2, 52, 14); // ������!!!

			DrawScrollbar(hwnd);

			EndPaint(hwnd, &ps);

			return 0;
		}

		case WM_DESTROY:
		{
			return 0;
		}
	}

	return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

/****************************************************************************************
*
*   ������� DrawScrollbar
*
*   ���������
*       hwnd - ��������� ���� ������ ���������
*
*   ������������ ��������
*       ���
*
*   ��������� �������������� ������ ���������.
*
****************************************************************************************/

static void DrawScrollbar(
	__in HWND hwnd)
{
	POINT pt = {0, 0};

	ClientToScreen(hwnd, &pt);
}
