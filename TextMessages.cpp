/****************************************************************************************
*
*   Определение модуля TextMessages
*
*   Обеспечивает получение текстовых сообщений на выбранном пользователем языке.
*
*   Авторы: Людмила Огородникова и Александр Огородников, 2009-2010
*
****************************************************************************************/

#include <windows.h>

#include "TextMessages.h"

/****************************************************************************************
*
*   Функция TextMessages_Init
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

bool TextMessages_Init()
{
	return true;
}

/****************************************************************************************
*
*   Функция TextMessages_Uninit
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

void TextMessages_Uninit()
{
}

/****************************************************************************************
*
*   Функция TextMessages_GetMessage
*
*   Параметры
*       MsgId - идентификатор текстового сообщения, указатель на которое нужно вернуть
*
*   Возвращаемое значение
*       Указатель на текстовое сообщение или NULL, если произошла ошибка.
*
*   Возвращает указатель на текстовое сообщение, заданное параметром MsgId.
*
****************************************************************************************/

LPCTSTR TextMessages_GetMessage(
	__in DWORD MsgId)
{
	switch(MsgId)
	{
		// сообщения об ошибках
		case MSGID_CANT_ALLOC_MEMORY:
			return TEXT("В системе не хватает памяти. Не удалось выполнить запрошенную операцию.");
		case MSGID_UNSUPPORTED_SCREEN_PIXEL_FORMAT:
			return TEXT("Формат пикселов экрана отличается от RGB. Данный формат пикселов не поддерживается.");
		case MSGID_CORRUPTED_MIDI_FILE:
			return TEXT("Этот MIDI-файл поврежден.");
		case MSGID_CORRUPTED_KAR_FILE:
			return TEXT("Этот караоке-файл поврежден.");
		case MSGID_UNSUPPORTED_FILE_FORMAT:
			return TEXT("Файлы данного формата не поддерживаются.");
		case MSGID_UNSUPPORTED_MIDI_FILE_FORMAT:
			return TEXT("Формат данного MIDI-файла пока не поддерживается.");
		case MSGID_NO_VOCAL_PARTS:
			return TEXT("В данном файле нет вокальных партий.");
		case MSGID_NO_LYRIC:
			return TEXT("В данном файле нет трека со словами песни.");

		// типы файлов для диалога GetOpenFileName
		case MSGID_ALL_FILES:
			return TEXT("Все файлы (*.*)");
		case MSGID_ALL_SUPPORTED_FILES:
			return TEXT("Все поддерживаемые файлы");
		case MSGID_MIDI_FILES:
			return TEXT("MIDI-файлы (*.mid;*.midi;*.rmi)");
		case MSGID_KARAOKE_FILES:
			return TEXT("Караоке-файлы (*.kar)");

		// сообщения об ошибках
		case MSGID_CANT_CREATE_PRIMARY_SURFACE:
			return TEXT("Не удалось создать поверхность DirectDraw для экрана (ошибка %1)");
		case MSGID_CANT_SET_CLIPPER:
			return TEXT("Не удалось задать объект обрезки для вывода на экран (ошибка %1)");
		case MSGID_CANT_GET_SCREEN_RESOLUTION:
			return TEXT("Не удалось получить разрешение экрана (ошибка %1)");
		case MSGID_CANT_GET_SCREEN_PIXEL_FORMAT:
			return TEXT("Не удалось получить цветовое разрешение экрана (ошибка %1)");
		case MSGID_CANT_CREATE_SURFACE:
			return TEXT("Не удалось создать поверхность DirectDraw (ошибка %1)");
		case MSGID_CANT_GET_DC:
			return TEXT("Не удалось получить контекст устройства для рисования (ошибка %1)");
		case MSGID_CANT_SET_HWND_FOR_CLIPPING:
			return TEXT("Не удалось задать область обрезки для вывода на экран (ошибка %1)");
		case MSGID_CANT_BLT_TO_SCREEN:
			return TEXT("Не удалось скопировать изображение на экран (ошибка %1)");
		case MSGID_CANT_BLT_FROM_SCREEN:
			return TEXT("Не удалось скопировать изображение с экрана (ошибка %1)");
		case MSGID_CANT_RESTORE_PRIMARY_SURFACE:
			return TEXT("Не удалось восстановить поверхность DirectDraw для экрана (ошибка %1)");
		case MSGID_GRADIENT_FILL_FAILED:
			return TEXT("Не удалось сделать градиентную заливку (ошибка %1)");
		case MSGID_CANT_OPEN_FILE:
			return TEXT("Не удалось открыть файл (ошибка %1)");

		// сообщения об ошибках
		case MSGID_CANT_INIT_PROGRAM:
			return TEXT("Не удалось инициализировать программу (ошибка %1:%2)");
		case MSGID_CANT_LOAD_FILE:
			return TEXT("Не удалось загрузить файл (ошибка %1:%2)");
	}

	return NULL;
}
