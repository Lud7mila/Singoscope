/****************************************************************************************
*
*   ����������� ������ TextMessages
*
*   ������������ ��������� ��������� ��������� �� ��������� ������������� �����.
*
*   ������: ������� ������������ � ��������� �����������, 2009-2010
*
****************************************************************************************/

#include <windows.h>

#include "TextMessages.h"

/****************************************************************************************
*
*   ������� TextMessages_Init
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

bool TextMessages_Init()
{
	return true;
}

/****************************************************************************************
*
*   ������� TextMessages_Uninit
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

void TextMessages_Uninit()
{
}

/****************************************************************************************
*
*   ������� TextMessages_GetMessage
*
*   ���������
*       MsgId - ������������� ���������� ���������, ��������� �� ������� ����� �������
*
*   ������������ ��������
*       ��������� �� ��������� ��������� ��� NULL, ���� ��������� ������.
*
*   ���������� ��������� �� ��������� ���������, �������� ���������� MsgId.
*
****************************************************************************************/

LPCTSTR TextMessages_GetMessage(
	__in DWORD MsgId)
{
	switch(MsgId)
	{
		// ��������� �� �������
		case MSGID_CANT_ALLOC_MEMORY:
			return TEXT("� ������� �� ������� ������. �� ������� ��������� ����������� ��������.");
		case MSGID_UNSUPPORTED_SCREEN_PIXEL_FORMAT:
			return TEXT("������ �������� ������ ���������� �� RGB. ������ ������ �������� �� ��������������.");
		case MSGID_CORRUPTED_MIDI_FILE:
			return TEXT("���� MIDI-���� ���������.");
		case MSGID_CORRUPTED_KAR_FILE:
			return TEXT("���� �������-���� ���������.");
		case MSGID_UNSUPPORTED_FILE_FORMAT:
			return TEXT("����� ������� ������� �� ��������������.");
		case MSGID_UNSUPPORTED_MIDI_FILE_FORMAT:
			return TEXT("������ ������� MIDI-����� ���� �� ��������������.");
		case MSGID_NO_VOCAL_PARTS:
			return TEXT("� ������ ����� ��� ��������� ������.");
		case MSGID_NO_LYRIC:
			return TEXT("� ������ ����� ��� ����� �� ������� �����.");

		// ���� ������ ��� ������� GetOpenFileName
		case MSGID_ALL_FILES:
			return TEXT("��� ����� (*.*)");
		case MSGID_ALL_SUPPORTED_FILES:
			return TEXT("��� �������������� �����");
		case MSGID_MIDI_FILES:
			return TEXT("MIDI-����� (*.mid;*.midi;*.rmi)");
		case MSGID_KARAOKE_FILES:
			return TEXT("�������-����� (*.kar)");

		// ��������� �� �������
		case MSGID_CANT_CREATE_PRIMARY_SURFACE:
			return TEXT("�� ������� ������� ����������� DirectDraw ��� ������ (������ %1)");
		case MSGID_CANT_SET_CLIPPER:
			return TEXT("�� ������� ������ ������ ������� ��� ������ �� ����� (������ %1)");
		case MSGID_CANT_GET_SCREEN_RESOLUTION:
			return TEXT("�� ������� �������� ���������� ������ (������ %1)");
		case MSGID_CANT_GET_SCREEN_PIXEL_FORMAT:
			return TEXT("�� ������� �������� �������� ���������� ������ (������ %1)");
		case MSGID_CANT_CREATE_SURFACE:
			return TEXT("�� ������� ������� ����������� DirectDraw (������ %1)");
		case MSGID_CANT_GET_DC:
			return TEXT("�� ������� �������� �������� ���������� ��� ��������� (������ %1)");
		case MSGID_CANT_SET_HWND_FOR_CLIPPING:
			return TEXT("�� ������� ������ ������� ������� ��� ������ �� ����� (������ %1)");
		case MSGID_CANT_BLT_TO_SCREEN:
			return TEXT("�� ������� ����������� ����������� �� ����� (������ %1)");
		case MSGID_CANT_BLT_FROM_SCREEN:
			return TEXT("�� ������� ����������� ����������� � ������ (������ %1)");
		case MSGID_CANT_RESTORE_PRIMARY_SURFACE:
			return TEXT("�� ������� ������������ ����������� DirectDraw ��� ������ (������ %1)");
		case MSGID_GRADIENT_FILL_FAILED:
			return TEXT("�� ������� ������� ����������� ������� (������ %1)");
		case MSGID_CANT_OPEN_FILE:
			return TEXT("�� ������� ������� ���� (������ %1)");

		// ��������� �� �������
		case MSGID_CANT_INIT_PROGRAM:
			return TEXT("�� ������� ���������������� ��������� (������ %1:%2)");
		case MSGID_CANT_LOAD_FILE:
			return TEXT("�� ������� ��������� ���� (������ %1:%2)");
	}

	return NULL;
}
