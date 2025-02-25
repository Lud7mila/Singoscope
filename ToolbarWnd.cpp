/****************************************************************************************
*
*   ����������� ������ ToolbarWnd
*
*   ������ ������������ - �������� ���� ��������� ����.
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

// ��� ������ ���� ������ ������������
static const TCHAR g_pszToolbarWndClassName[] = TEXT("Singoscope Toolbar Window Class");

// ��������� ���� ������ ������������
static HWND g_hwndToolbar;

// ������ ���� ������ ������������
static int g_ToolbarWidth;

// ������ ���� ������ ������������
static const int g_ToolbarHeight = 32;

/****************************************************************************************
*
*   ��������� �������, ����������� ����
*
****************************************************************************************/

static LRESULT CALLBACK ToolbarWndProc(
	HWND hwnd,
	UINT uMsg,
	WPARAM wParam,
	LPARAM lParam);

static void DrawToolbar(
	__in HWND hwnd);

/****************************************************************************************
*
*   ������� ToolbarWnd_Init
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

bool ToolbarWnd_Init()
{
	WNDCLASS wc;

	ZeroMemory(&wc, sizeof(wc));

	wc.lpfnWndProc = ToolbarWndProc;
	wc.hInstance = g_hInstance;
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.lpszClassName = g_pszToolbarWndClassName;

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
*   ������� ToolbarWnd_Uninit
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

void ToolbarWnd_Uninit()
{
}

/****************************************************************************************
*
*   ������� ToolbarWnd_GetHeight
*
*   ���������
*       ���
*
*   ������������ ��������
*       ������ ������ ������������ � ��������.
*
*   ���������� ������ ������ ������������ � ��������.
*
****************************************************************************************/

int ToolbarWnd_GetHeight()
{
	return g_ToolbarHeight;
}

/****************************************************************************************
*
*   ������� ToolbarWnd_Create
*
*   ���������
*       x - x-���������� ������ �������� ���� ����
*       y - y-���������� ������ �������� ���� ����
*       nWidth - ������ ����
*       hwndParent - ��������� ������������� ����
*
*   ������������ ��������
*       ��������� ���������� ���� ������ ������������.
*
*   ������ ���� ������ ������������.
*
****************************************************************************************/

HWND ToolbarWnd_Create(
	__in int x,
	__in int y,
	__in int nWidth,
	__in HWND hwndParent)
{
	g_ToolbarWidth = nWidth;

	g_hwndToolbar = CreateWindow(g_pszToolbarWndClassName, NULL,
		WS_CHILD | WS_VISIBLE, x, y, nWidth, g_ToolbarHeight,
		hwndParent, NULL, g_hInstance, NULL);

	if (g_hwndToolbar == NULL)
	{
		LOG("CreateWindow failed (error %u)\n", GetLastError());
		ShowFatalError(MSGID_CANT_INIT_PROGRAM, FUNCID_CREATE_WINDOW, GetLastError());
		return NULL;
	}

	return g_hwndToolbar;
}

/****************************************************************************************
*
*   ������� ToolbarWndProc
*
*   ��. �������� ������� WindowProc � MSDN.
*
****************************************************************************************/

static LRESULT CALLBACK ToolbarWndProc(
	HWND hwnd,
	UINT uMsg,
	WPARAM wParam,
	LPARAM lParam)
{
	switch(uMsg)
	{
		case WM_SIZE:
		{
			g_ToolbarWidth = LOWORD(lParam);
			return 0;
		}

		case WM_PAINT:
		{
			PAINTSTRUCT ps;

			BeginPaint(hwnd, &ps);

			RECT rc; // ������!!!
			GetClientRect(hwnd, &rc); // ������!!!
			FillRect(ps.hdc, &rc, (HBRUSH) (COLOR_BTNFACE + 1)); // ������!!!
			Rectangle(ps.hdc, 2, 2, 30, 30); // ������!!!

			DrawToolbar(hwnd);

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
*   ������� DrawToolbar
*
*   ���������
*       hwnd - ��������� ���� ������ ������������
*
*   ������������ ��������
*       ���
*
*   ��������� �������������� ������ ������������.
*
****************************************************************************************/

static void DrawToolbar(
	__in HWND hwnd)
{
	POINT pt = {0, 0};

	ClientToScreen(hwnd, &pt);
}
