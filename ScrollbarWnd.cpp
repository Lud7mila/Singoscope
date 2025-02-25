/****************************************************************************************
*
*   Определение модуля ScrollbarWnd
*
*   Полоса прокрутки для нотного стана - дочернее окно окна нотного стана.
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

// имя класса окна полосы прокрутки
static const TCHAR g_pszScrollbarWndClassName[] = TEXT("Singoscope Scrollbar Window Class");

// описатель окна полосы прокрутки
static HWND g_hwndScrollbar;

// ширина окна полосы прокрутки
static int g_ScrollbarWidth;

// высота окна полосы прокрутки
static const int g_ScrollbarHeight = 16;

/****************************************************************************************
*
*   Прототипы функций, определённых ниже
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
*   Функция ScrollbarWnd_Init
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
*   Функция ScrollbarWnd_Uninit
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

void ScrollbarWnd_Uninit()
{
}

/****************************************************************************************
*
*   Функция ScrollbarWnd_GetHeight
*
*   Параметры
*       Нет
*
*   Возвращаемое значение
*       Высота полосы прокрутки в пикселах.
*
*   Возвращает высоту полосы прокрутки в пикселах.
*
****************************************************************************************/

int ScrollbarWnd_GetHeight()
{
	return g_ScrollbarHeight;
}

/****************************************************************************************
*
*   Функция ScrollbarWnd_Create
*
*   Параметры
*       x - x-координата левого верхнего угла окна
*       y - y-координата левого верхнего угла окна
*       nWidth - ширина окна
*       hwndParent - описатель родительского окна
*
*   Возвращаемое значение
*       Описатель созданного окна полосы прокрутки.
*
*   Создаёт окно полосы прокрутки.
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
*   Функция ScrollbarWndProc
*
*   См. описание функции WindowProc в MSDN.
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

			RECT rc; // убрать!!!
			GetClientRect(hwnd, &rc); // убрать!!!
			FillRect(ps.hdc, &rc, (HBRUSH) (COLOR_SCROLLBAR + 1)); // убрать!!!
			Rectangle(ps.hdc, 2, 2, 52, 14); // убрать!!!

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
*   Функция DrawScrollbar
*
*   Параметры
*       hwnd - описатель окна полосы прокрутки
*
*   Возвращаемое значение
*       Нет
*
*   Полностью перерисовывает полосу прокрутки.
*
****************************************************************************************/

static void DrawScrollbar(
	__in HWND hwnd)
{
	POINT pt = {0, 0};

	ClientToScreen(hwnd, &pt);
}
