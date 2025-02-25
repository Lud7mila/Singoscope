/****************************************************************************************
*
*   ������� ���� �������
*
*   ������������ ������������� � ��������������� ���������, ������ �������� ���� �
*   �������� ������� ���� ���������.
*
*   ������: ��������� ����������� � ������� ������������, 2007-2010
*
****************************************************************************************/

#include <windows.h>

#include "Log.h"
#include "TextMessages.h"
#include "ShowError.h"
#include "Video.h"
#include "FrameWnd.h"
#include "ToolbarWnd.h"
#include "StaveWnd.h"
#include "ScrollbarWnd.h"
#include "Resources.h"

/****************************************************************************************
*
*   ���������� ����������
*
****************************************************************************************/

// ��������� ������ exe-����� ���������
HINSTANCE g_hInstance;

// ��������� ������� �������������
static HACCEL g_hAccel;

/****************************************************************************************
*
*   ��������� �������, ����������� ����
*
****************************************************************************************/

static bool InitApp();
static void UninitApp();

/****************************************************************************************
*
*   ������� WinMain
*
*   ��. �������� ������� WinMain � MSDN.
*
****************************************************************************************/

int WINAPI WinMain(
	HINSTANCE hInstance,
	HINSTANCE hPrevInstance,
	LPSTR lpCmdLine,
	int nCmdShow)
{
	INITLOG(TEXT("log.txt"));

	g_hInstance = hInstance;

	if (!InitApp())
	{
		LOG("InitApp failed\n");
		goto exit;
	}

	if (!FrameWnd_Create())
	{
		LOG("FrameWnd_Create failed\n");
		goto exit;
	}

	MSG msg;

	while (GetMessage(&msg, NULL, 0, 0))
	{
		if (TranslateAccelerator(g_hwndFrame, g_hAccel, &msg))
		{
			continue;
		}

		DispatchMessage(&msg);
	}

exit:

	UninitApp();

	UNINITLOG();

	return 0;
}

/****************************************************************************************
*
*   ������� InitApp
*
*   ���������
*       ���
*
*   ������������ ��������
*       true, ���� ��������� ������� ��������������; ����� false.
*
*   �������������� ���������.
*
****************************************************************************************/

static bool InitApp()
{
	if (!TextMessages_Init())
	{
		LOG("TextMessages_Init failed\n");
		return false;
	}

	if (!ShowError_Init())
	{
		LOG("ShowError_Init failed\n");
		return false;
	}

	if (!Video_Init())
	{
		LOG("Video_Init failed\n");
		return false;
	}

	if (!FrameWnd_Init())
	{
		LOG("FrameWnd_Init failed\n");
		return false;
	}

	if (!ToolbarWnd_Init())
	{
		LOG("ToolbarWnd_Init failed\n");
		return false;
	}

	if (!StaveWnd_Init())
	{
		LOG("ToolbarWnd_Init failed\n");
		return false;
	}

	if (!ScrollbarWnd_Init())
	{
		LOG("ScrollbarWnd_Init failed\n");
		return false;
	}

	g_hAccel = LoadAccelerators(g_hInstance, MAKEINTRESOURCE(IDA_ACCELERATORS));

	if (g_hAccel == NULL)
	{
		LOG("LoadAccelerators failed\n");
		return false;
	}

	return true;
}

/****************************************************************************************
*
*   ������� UninitApp
*
*   ���������
*       ���
*
*   ������������ ��������
*       ���
*
*   ���������������� ���������.
*
****************************************************************************************/

static void UninitApp()
{
	ScrollbarWnd_Uninit();
	StaveWnd_Uninit();
	ToolbarWnd_Uninit();
	FrameWnd_Uninit();
	Video_Uninit();
	ShowError_Uninit();
	TextMessages_Uninit();
}
