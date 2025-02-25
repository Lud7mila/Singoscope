/****************************************************************************************
*
*   Главный файл проекта
*
*   Осуществляет инициализацию и деинициализацию программы, создаёт рамочное окно и
*   содержит главный цикл программы.
*
*   Авторы: Александр Огородников и Людмила Огородникова, 2007-2010
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
*   Глобальные переменные
*
****************************************************************************************/

// описатель модуля exe-файла программы
HINSTANCE g_hInstance;

// описатель таблицы акселераторов
static HACCEL g_hAccel;

/****************************************************************************************
*
*   Прототипы функций, определённых ниже
*
****************************************************************************************/

static bool InitApp();
static void UninitApp();

/****************************************************************************************
*
*   Функция WinMain
*
*   См. описание функции WinMain в MSDN.
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
*   Функция InitApp
*
*   Параметры
*       Нет
*
*   Возвращаемое значение
*       true, если программа успешно инициализована; иначе false.
*
*   Инициализирует программу.
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
*   Функция UninitApp
*
*   Параметры
*       Нет
*
*   Возвращаемое значение
*       Нет
*
*   Деинициализирует программу.
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
