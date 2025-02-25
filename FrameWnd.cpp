/****************************************************************************************
*
*   ����������� ������ FrameWnd
*
*   �������� � ���������� �������� ����� (������� ����� ���������).
*
*   �����: ��������� �����������, 2010
*
****************************************************************************************/

#include <windows.h>

#include "Log.h"
#include "TextMessages.h"
#include "ShowError.h"
#include "Video.h"
#include "Main.h"
#include "ToolbarWnd.h"
#include "StaveWnd.h"
#include "Resources.h"
 
/****************************************************************************************
*
*   ���������� ����������
*
****************************************************************************************/

// ��� ������ ��������� ����
static const TCHAR g_pszFrameWndClassName[] = TEXT("Singoscope Frame Window Class");

// ��������� ��������� ����
HWND g_hwndFrame = NULL;

// ������ ������ ������������ � ��������
static int g_ToolbarHeight;

// ��������� ���� ������ ������������
static HWND g_hwndToolbar;

// ��������� ���� ������� �����
static HWND g_hwndStave;

/****************************************************************************************
*
*   ��������� �������, ����������� ����
*
****************************************************************************************/

static LRESULT CALLBACK FrameWndProc(
	HWND hwnd,
	UINT uMsg,
	WPARAM wParam,
	LPARAM lParam);

/****************************************************************************************
*
*   ������� FrameWnd_Init
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

bool FrameWnd_Init()
{
	WNDCLASS wc;

	ZeroMemory(&wc, sizeof(wc));

	wc.lpfnWndProc = FrameWndProc;
	wc.hInstance = g_hInstance;
	wc.hIcon = LoadIcon(g_hInstance, MAKEINTRESOURCE(IDI_FRAMEWND));
	wc.lpszClassName = g_pszFrameWndClassName;

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
*   ������� FrameWnd_Uninit
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

void FrameWnd_Uninit()
{
}

/****************************************************************************************
*
*   ������� FrameWnd_Create
*
*   ���������
*       ���
*
*   ������������ ��������
*       ���
*
*   ������ �������� ����.
*
****************************************************************************************/

bool FrameWnd_Create()
{
	g_hwndFrame = CreateWindow(g_pszFrameWndClassName, TEXT("Singoscope"),
		WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
		NULL, NULL, g_hInstance, NULL);

	if (g_hwndFrame == NULL)
	{
		LOG("CreateWindow failed (error %u)\n", GetLastError());
		ShowFatalError(MSGID_CANT_INIT_PROGRAM, FUNCID_CREATE_WINDOW, GetLastError());
		return false;
	}

	if (!Video_SetHwndForClipping(g_hwndFrame))
	{
		LOG("Video_SetHwndForClipping failed\n");
		DestroyWindow(g_hwndFrame);
		return false;
	}

	RECT cr;

	GetClientRect(g_hwndFrame, &cr);

	g_ToolbarHeight = ToolbarWnd_GetHeight();

	g_hwndToolbar = ToolbarWnd_Create(0, 0, cr.right, g_hwndFrame);

	if (g_hwndToolbar == NULL)
	{
		LOG("ToolbarWnd_Create failed\n");
		DestroyWindow(g_hwndFrame);
		return false;
	}

	g_hwndStave = StaveWnd_Create(0, g_ToolbarHeight,
		cr.right, cr.bottom - g_ToolbarHeight, g_hwndFrame);

	if (g_hwndStave == NULL)
	{
		LOG("StaveWnd_Create failed\n");
		DestroyWindow(g_hwndToolbar);
		DestroyWindow(g_hwndFrame);
		return false;
	}

	ShowWindow(g_hwndFrame, SW_SHOWDEFAULT);
	UpdateWindow(g_hwndFrame);

	return true;
}

/****************************************************************************************
*
*   ������� FrameWndProc
*
*   ��. �������� ������� WindowProc � MSDN.
*
****************************************************************************************/

static LRESULT CALLBACK FrameWndProc(
	HWND hwnd,
	UINT uMsg,
	WPARAM wParam,
	LPARAM lParam)
{
	switch(uMsg)
	{
		case WM_SIZE:
		{
			if (wParam != SIZE_MINIMIZED)
			{
				SetWindowPos(g_hwndToolbar, NULL, 0, 0, LOWORD(lParam),
					g_ToolbarHeight, SWP_NOMOVE | SWP_NOZORDER);

				SetWindowPos(g_hwndStave, NULL, 0, 0, LOWORD(lParam),
					HIWORD(lParam) - g_ToolbarHeight, SWP_NOMOVE | SWP_NOZORDER);
			}

			return 0;
		}

		case WM_COMMAND:
		{
			switch (LOWORD(wParam))
			{
				case IDC_OPENFILE:
				{
					SendMessage(g_hwndStave, WM_COMMAND, wParam, lParam);
					return 0;
				}
			}

			break;
		}

		case WM_DESTROY:
		{
			PostQuitMessage(0);
			return 0;
		}
	}

	return DefWindowProc(hwnd, uMsg, wParam, lParam);
}
