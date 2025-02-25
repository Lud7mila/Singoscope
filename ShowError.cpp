/****************************************************************************************
*
*   Определение модуля ShowError
*
*   Обеспечивает вывод сообщений об ошибках.
*
*   Авторы: Александр Огородников и Людмила Огородникова, 2008-2010
*
****************************************************************************************/

#define _CRT_SECURE_NO_DEPRECATE

#include <windows.h>
#include <tchar.h>
#include <stdarg.h>

#include "Log.h"
#include "TextMessages.h"
#include "ShowError.h"
#include "FrameWnd.h"

/****************************************************************************************
*
*   Константы
*
****************************************************************************************/

// интервал времени с момента последнего показа пользователю сообщения о фатальной
// ошибке, в течение которого последующие сообщения о фатальных ошибках не выводятся;
// после истечения этого интервала вывод сообщений о фатальных ошибках возобновляется
#define SHOW_FATAL_ERROR_HYSTERESIS		60000 // миллисекунд

/****************************************************************************************
*
*   Глобальные переменные
*
****************************************************************************************/

// идентификатор последнего сообщения об ошибке, показанного пользователю
static DWORD g_LastMsgId = MSGID_UNDEFINED;

// время последнего вызова одной из функций ShowFatalError, измеряется в количестве
// миллисекунд с момента запуска операционной системы (возвращается функцией GetTickCount)
static DWORD g_LastFatalErrorTime;

// флаг, равный true, если в данный момент пользователю показывается сообщение о
// фатальной ошибке
static bool g_bFatalErrorIsBeingDisplayed = false;

/****************************************************************************************
*
*   Функция ShowError_Init
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

bool ShowError_Init()
{
	g_LastFatalErrorTime = GetTickCount() - SHOW_FATAL_ERROR_HYSTERESIS;
	return true;
}

/****************************************************************************************
*
*   Функция ShowError_Uninit
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

void ShowError_Uninit()
{
}

/****************************************************************************************
*
*   Функция ShowError
*
*   Параметры
*       MsgId - идентификатор сообщения об ошибке
*
*   Возвращаемое значение
*       Нет
*
*   Выводит сообщение об ошибке. Сообщение идентифицируется параметром MsgId.
*
****************************************************************************************/

void ShowError(
	__in TEXTMESSAGEID0 MsgId)
{
	LPCTSTR pszMsg = TextMessages_GetMessage(MsgId);

	if (pszMsg == NULL) return;

	g_LastMsgId = MsgId;

	MessageBox(g_hwndFrame, pszMsg, NULL, MB_OK | MB_HELP | MB_ICONEXCLAMATION);
}

/****************************************************************************************
*
*   Функция ShowError
*
*   Параметры
*       MsgId - идентификатор сообщения об ошибке
*       ErrorCode - код ошибки (например, возвращённый функцией GetLastError)
*
*   Возвращаемое значение
*       Нет
*
*   Выводит сообщение об ошибке с указанием кода ошибки. Сообщение идентифицируется
*   параметром MsgId.
*
****************************************************************************************/

void ShowError(
	__in TEXTMESSAGEID1 MsgId,
	__in DWORD ErrorCode)
{
	LPCTSTR pszMsg = TextMessages_GetMessage(MsgId);

	if (pszMsg == NULL) return;

	TCHAR pszHexNumber[9];
	_itot(ErrorCode, pszHexNumber, 16); 

	DWORD_PTR pArguments[1] = {(DWORD_PTR ) pszHexNumber};

	LPTSTR lpBuffer;

	DWORD cchReturned = FormatMessage(FORMAT_MESSAGE_FROM_STRING |
		FORMAT_MESSAGE_ARGUMENT_ARRAY | FORMAT_MESSAGE_ALLOCATE_BUFFER,
		pszMsg,	0, 0, (LPTSTR) &lpBuffer, 0, (va_list *) pArguments);

	if (cchReturned == 0)
	{
		LOG("FormatMessage failed (error %u)\n", GetLastError());
	}
	else
	{
		MessageBox(g_hwndFrame, lpBuffer, NULL, MB_OK | MB_HELP | MB_ICONEXCLAMATION);
		LocalFree(lpBuffer);
	}
}

/****************************************************************************************
*
*   Функция ShowError
*
*   Параметры
*       MsgId - идентификатор сообщения об ошибке
*       FunctionId - идентификатор функции,в которой произошла ошибка
*       ErrorCode - код ошибки, который вернула функция (в том числе, возвращённый
*                   функцией GetLastError)
*
*   Возвращаемое значение
*       Нет
*
*   Выводит сообщение об ошибке с указанием идентификатора функции,в которой произошла
*   ошибка, и кода ошибки. Сообщение идентифицируется параметром MsgId.
*
****************************************************************************************/

void ShowError(
	__in TEXTMESSAGEID2 MsgId,
	__in FUNCTIONID FunctionId,
	__in DWORD ErrorCode)
{
	LPCTSTR pszMsg = TextMessages_GetMessage(MsgId);

	if (pszMsg == NULL) return;

	TCHAR pszDecimalNumber[12];
	_itot(FunctionId, pszDecimalNumber, 10);

	TCHAR pszHexNumber[9];
	_itot(ErrorCode, pszHexNumber, 16); 

	DWORD_PTR pArguments[2] = {(DWORD_PTR) pszDecimalNumber, (DWORD_PTR) pszHexNumber};

	LPTSTR lpBuffer;

	DWORD cchReturned = FormatMessage(FORMAT_MESSAGE_FROM_STRING |
		FORMAT_MESSAGE_ARGUMENT_ARRAY | FORMAT_MESSAGE_ALLOCATE_BUFFER,
		pszMsg,	0, 0, (LPTSTR) &lpBuffer, 0, (va_list *) pArguments);

	if (cchReturned == 0)
	{
		LOG("FormatMessage failed (error %u)\n", GetLastError());
	}
	else
	{
		MessageBox(g_hwndFrame, lpBuffer, NULL, MB_OK | MB_HELP | MB_ICONEXCLAMATION);
		LocalFree(lpBuffer);
	}
}

/****************************************************************************************
*
*   Функция ShowFatalError
*
*   Параметры
*       MsgId - идентификатор сообщения об ошибке
*
*   Возвращаемое значение
*       Нет
*
*   Выводит сообщение о фатальной ошибке. Сообщение идентифицируется параметром MsgId.
*
****************************************************************************************/

void ShowFatalError(
	__in TEXTMESSAGEID0 MsgId)
{
	if (g_bFatalErrorIsBeingDisplayed) return;

	if (GetTickCount() - g_LastFatalErrorTime < SHOW_FATAL_ERROR_HYSTERESIS) return;

	g_bFatalErrorIsBeingDisplayed = true;

	ShowError(MsgId);

	g_bFatalErrorIsBeingDisplayed = false;

	g_LastFatalErrorTime = GetTickCount();
}

/****************************************************************************************
*
*   Функция ShowFatalError
*
*   Параметры
*       MsgId - идентификатор сообщения об ошибке
*       ErrorCode - код ошибки (например, возвращённый функцией GetLastError)
*
*   Возвращаемое значение
*       Нет
*
*   Выводит сообщение о фатальной ошибке с указанием кода ошибки. Сообщение
*   идентифицируется параметром MsgId.
*
****************************************************************************************/

void ShowFatalError(
	__in TEXTMESSAGEID1 MsgId,
	__in DWORD ErrorCode)
{
	if (g_bFatalErrorIsBeingDisplayed) return;

	if (GetTickCount() - g_LastFatalErrorTime < SHOW_FATAL_ERROR_HYSTERESIS) return;

	g_bFatalErrorIsBeingDisplayed = true;

	ShowError(MsgId, ErrorCode);

	g_bFatalErrorIsBeingDisplayed = false;

	g_LastFatalErrorTime = GetTickCount();
}

/****************************************************************************************
*
*   Функция ShowFatalError
*
*   Параметры
*       MsgId - идентификатор сообщения об ошибке
*       FunctionId - идентификатор функции,в которой произошла ошибка
*       ErrorCode - код ошибки, который вернула функция (в том числе, возвращённый
*                   функцией GetLastError)
*
*   Возвращаемое значение
*       Нет
*
*   Выводит сообщение о фатальной ошибке с указанием идентификатора функции, в которой
*   произошла ошибка, и кода ошибки. Сообщение идентифицируется параметром MsgId.
*
****************************************************************************************/

void ShowFatalError(
	__in TEXTMESSAGEID2 MsgId,
	__in FUNCTIONID FunctionId,
	__in DWORD ErrorCode)
{
	if (g_bFatalErrorIsBeingDisplayed) return;

	if (GetTickCount() - g_LastFatalErrorTime < SHOW_FATAL_ERROR_HYSTERESIS) return;

	g_bFatalErrorIsBeingDisplayed = true;

	ShowError(MsgId, FunctionId, ErrorCode);

	g_bFatalErrorIsBeingDisplayed = false;

	g_LastFatalErrorTime = GetTickCount();
}

/****************************************************************************************
*
*   Функция ShowError_GetLastMsgId
*
*   Параметры
*       Нет
*
*   Возвращаемое значение
*       Идентификатор последнего сообщения об ошибке.
*
*   Возвращает идентификатор последнего показанного пользователю сообщения об ошибке.
*
****************************************************************************************/

DWORD ShowError_GetLastMsgId()
{
	return g_LastMsgId;
}
