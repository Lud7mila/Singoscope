/****************************************************************************************
*
*   ����������� ������ ShowError
*
*   ������������ ����� ��������� �� �������.
*
*   ������: ��������� ����������� � ������� ������������, 2008-2010
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
*   ���������
*
****************************************************************************************/

// �������� ������� � ������� ���������� ������ ������������ ��������� � ���������
// ������, � ������� �������� ����������� ��������� � ��������� ������� �� ���������;
// ����� ��������� ����� ��������� ����� ��������� � ��������� ������� ��������������
#define SHOW_FATAL_ERROR_HYSTERESIS		60000 // �����������

/****************************************************************************************
*
*   ���������� ����������
*
****************************************************************************************/

// ������������� ���������� ��������� �� ������, ����������� ������������
static DWORD g_LastMsgId = MSGID_UNDEFINED;

// ����� ���������� ������ ����� �� ������� ShowFatalError, ���������� � ����������
// ����������� � ������� ������� ������������ ������� (������������ �������� GetTickCount)
static DWORD g_LastFatalErrorTime;

// ����, ������ true, ���� � ������ ������ ������������ ������������ ��������� �
// ��������� ������
static bool g_bFatalErrorIsBeingDisplayed = false;

/****************************************************************************************
*
*   ������� ShowError_Init
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

bool ShowError_Init()
{
	g_LastFatalErrorTime = GetTickCount() - SHOW_FATAL_ERROR_HYSTERESIS;
	return true;
}

/****************************************************************************************
*
*   ������� ShowError_Uninit
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

void ShowError_Uninit()
{
}

/****************************************************************************************
*
*   ������� ShowError
*
*   ���������
*       MsgId - ������������� ��������� �� ������
*
*   ������������ ��������
*       ���
*
*   ������� ��������� �� ������. ��������� ���������������� ���������� MsgId.
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
*   ������� ShowError
*
*   ���������
*       MsgId - ������������� ��������� �� ������
*       ErrorCode - ��� ������ (��������, ������������ �������� GetLastError)
*
*   ������������ ��������
*       ���
*
*   ������� ��������� �� ������ � ��������� ���� ������. ��������� ����������������
*   ���������� MsgId.
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
*   ������� ShowError
*
*   ���������
*       MsgId - ������������� ��������� �� ������
*       FunctionId - ������������� �������,� ������� ��������� ������
*       ErrorCode - ��� ������, ������� ������� ������� (� ��� �����, ������������
*                   �������� GetLastError)
*
*   ������������ ��������
*       ���
*
*   ������� ��������� �� ������ � ��������� �������������� �������,� ������� ���������
*   ������, � ���� ������. ��������� ���������������� ���������� MsgId.
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
*   ������� ShowFatalError
*
*   ���������
*       MsgId - ������������� ��������� �� ������
*
*   ������������ ��������
*       ���
*
*   ������� ��������� � ��������� ������. ��������� ���������������� ���������� MsgId.
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
*   ������� ShowFatalError
*
*   ���������
*       MsgId - ������������� ��������� �� ������
*       ErrorCode - ��� ������ (��������, ������������ �������� GetLastError)
*
*   ������������ ��������
*       ���
*
*   ������� ��������� � ��������� ������ � ��������� ���� ������. ���������
*   ���������������� ���������� MsgId.
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
*   ������� ShowFatalError
*
*   ���������
*       MsgId - ������������� ��������� �� ������
*       FunctionId - ������������� �������,� ������� ��������� ������
*       ErrorCode - ��� ������, ������� ������� ������� (� ��� �����, ������������
*                   �������� GetLastError)
*
*   ������������ ��������
*       ���
*
*   ������� ��������� � ��������� ������ � ��������� �������������� �������, � �������
*   ��������� ������, � ���� ������. ��������� ���������������� ���������� MsgId.
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
*   ������� ShowError_GetLastMsgId
*
*   ���������
*       ���
*
*   ������������ ��������
*       ������������� ���������� ��������� �� ������.
*
*   ���������� ������������� ���������� ����������� ������������ ��������� �� ������.
*
****************************************************************************************/

DWORD ShowError_GetLastMsgId()
{
	return g_LastMsgId;
}
