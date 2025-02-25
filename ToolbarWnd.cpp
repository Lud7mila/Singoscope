/****************************************************************************************
*
*   Определение модуля ToolbarWnd
*
*   Панель инструментов - дочернее окно рамочного окна.
*
*   Автор: Александр Огородников, 2010
*
****************************************************************************************/

#include <windows.h>

#include "Log.h"
#include "TextMessages.h"
#include "ShowError.h"
#include "Main.h"

/****************************************************************************************
*
*   Глобальные переменные
*
****************************************************************************************/

// имя класса окна панели инструментов
static const TCHAR g_pszToolbarWndClassName[] = TEXT("Singoscope Toolbar Window Class");

// описатель окна панели инструментов
static HWND g_hwndToolbar;

// ширина окна панели инструментов
static int g_ToolbarWidth;

// высота окна панели инструментов
static const int g_ToolbarHeight = 32;

/****************************************************************************************
*
*   Прототипы функций, определённых ниже
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
*   Функция ToolbarWnd_Init
*
*   Параметры
*       Нет
*
*   Возвращаемое значение
*       true, если модуль успешно инициализован; иначе false.
*
*   Инициализирует модуль.
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
*   Функция ToolbarWnd_Uninit
*
*   Параметры
*       Нет
*
*   Возвращаемое значение
*       Нет
*
*   Деинициализирует модуль.
*
****************************************************************************************/

void ToolbarWnd_Uninit()
{
}

/****************************************************************************************
*
*   Функция ToolbarWnd_GetHeight
*
*   Параметры
*       Нет
*
*   Возвращаемое значение
*       Высота панели инструментов в пикселах.
*
*   Возвращает высоту панели инструментов в пикселах.
*
****************************************************************************************/

int ToolbarWnd_GetHeight()
{
	return g_ToolbarHeight;
}

/****************************************************************************************
*
*   Функция ToolbarWnd_Create
*
*   Параметры
*       x - x-координата левого верхнего угла окна
*       y - y-координата левого верхнего угла окна
*       nWidth - ширина окна
*       hwndParent - описатель родительского окна
*
*   Возвращаемое значение
*       Описатель созданного окна панели инструментов.
*
*   Создаёт окно панели инструментов.
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
*   Функция ToolbarWndProc
*
*   См. описание функции WindowProc в MSDN.
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

			RECT rc; // убрать!!!
			GetClientRect(hwnd, &rc); // убрать!!!
			FillRect(ps.hdc, &rc, (HBRUSH) (COLOR_BTNFACE + 1)); // убрать!!!
			Rectangle(ps.hdc, 2, 2, 30, 30); // убрать!!!

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
*   Функция DrawToolbar
*
*   Параметры
*       hwnd - описатель окна панели инструментов
*
*   Возвращаемое значение
*       Нет
*
*   Полностью перерисовывает панель инструментов.
*
****************************************************************************************/

static void DrawToolbar(
	__in HWND hwnd)
{
	POINT pt = {0, 0};

	ClientToScreen(hwnd, &pt);
}
