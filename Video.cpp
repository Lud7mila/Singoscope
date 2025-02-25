/****************************************************************************************
*
*   Определение модуля Video
*
*   Предоставляет функции для работы с экраном.
*
*   Автор: Людмила Огородникова, 2010
*
****************************************************************************************/

#include <windows.h>
#include <ddraw.h>

#include "Log.h"
#include "TextMessages.h"
#include "ShowError.h"
#include "Video.h"
#include "float_const.h"
#include "Statistics.h"

/****************************************************************************************
*
*   Глобальные переменные
*
****************************************************************************************/

// указатель на интерфейс IDirectDraw
static LPDIRECTDRAW g_pDDraw = NULL;

// указатель на интерфейс IDirectDraw4
static LPDIRECTDRAW4 g_pDDraw4 = NULL;

// указатель на интерфейс IDirectDrawSurface4 для первичной поверхности
static LPDIRECTDRAWSURFACE4 g_pDDSPrimary = NULL;

// указатель на интерфейс IDirectDrawClipper для объекта обрезки, используемого для
// первичной поверхности
static LPDIRECTDRAWCLIPPER g_pDDClipper = NULL;

// количество битов на пиксел экрана
static DWORD g_RGBBitCount;

// битовая маска для красного канала пиксела экрана
static DWORD g_RBitMask;

// битовая маска для зелёного канала пиксела экрана
static DWORD g_GBitMask;

// битовая маска для синего канала пиксела экрана
static DWORD g_BBitMask;

/****************************************************************************************
*
*   Прототипы функций, определённых ниже
*
****************************************************************************************/

static bool CreatePrimarySurface();

static void ReleasePrimarySurface();

static bool GetScreenPixelFormat(
	__out PDWORD pRGBBitCount,
	__out PDWORD pRBitMask,
	__out PDWORD pGBitMask,
	__out PDWORD pBBitMask);

static VIDEORESULT RecoverPrimarySurface();

/****************************************************************************************
*
*   Функция Video_Init
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

bool Video_Init()
{
	HRESULT ddrval = DirectDrawCreate(NULL, &g_pDDraw, NULL);
	if (ddrval != DD_OK)
	{
		LOG("DirectDrawCreate failed (error 0x%X)\n", ddrval);
		ShowFatalError(MSGID_CANT_INIT_PROGRAM, FUNCID_DIRECT_DRAW_CREATE, ddrval);
		return false;
	}

	ddrval = g_pDDraw->SetCooperativeLevel(NULL, DDSCL_NORMAL);
	if (ddrval != DD_OK)
	{
		LOG("IDirectDraw::SetCooperativeLevel failed (error 0x%X)\n", ddrval);
		ShowFatalError(MSGID_CANT_INIT_PROGRAM, FUNCID_SET_COOPERATIVE_LEVEL, ddrval);
		return false;
	}

	ddrval = g_pDDraw->QueryInterface(IID_IDirectDraw4, (LPVOID *)&g_pDDraw4);
	if (ddrval != DD_OK)
	{
		LOG("IDirectDraw::QueryInterface failed (error 0x%X)\n", ddrval);
		ShowFatalError(MSGID_CANT_INIT_PROGRAM, FUNCID_QUERY_INTERFACE, ddrval);
		return false;
	}

	ddrval = g_pDDraw4->CreateClipper(0, &g_pDDClipper, NULL);
	if (ddrval != DD_OK)
	{
		LOG("IDirectDraw4::CreateClipper failed (error 0x%X)\n", ddrval);
		ShowFatalError(MSGID_CANT_INIT_PROGRAM, FUNCID_CREATE_CLIPPER, ddrval);
		return false;
	}

	if (!CreatePrimarySurface())
	{
		LOG("CreatePrimarySurface failed\n");
		return false;
	}

	if (!GetScreenPixelFormat(&g_RGBBitCount, &g_RBitMask, &g_GBitMask, &g_BBitMask))
	{
		LOG("GetScreenPixelFormat failed\n");
		return false;
	}

	return true;
}

/****************************************************************************************
*
*   Функция Video_Uninit
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

void Video_Uninit()
{
	ReleasePrimarySurface();

	if (g_pDDClipper != NULL)
	{
		g_pDDClipper->Release();
		g_pDDClipper = NULL;
	}

	if (g_pDDraw4 != NULL)
	{
		g_pDDraw4->Release();
		g_pDDraw4 = NULL;
	}

	if (g_pDDraw != NULL)
	{
		g_pDDraw->Release();
		g_pDDraw = NULL;
	}
}

/****************************************************************************************
*
*   Функция Video_CheckScreenStatus
*
*   Параметры
*       Нет
*
*   Возвращаемое значение
*       VIDEO_SUCCESS, если формат пикселов экрана не изменился;
*       VIDEO_FAIL, если не удалось установить, изменился ли формат пикселов экрана;
*       VIDEO_SCREEN_PIXEL_FORMAT_CHANGED, формат пикселов экрана изменился.
*
*   Проверяет, изменился ли формат пикселов экрана. В случае, если первичная поверхность
*   была потеряна, функция восстанавливает её.
*
****************************************************************************************/

VIDEORESULT Video_CheckScreenStatus()
{
	HRESULT ddrval = g_pDDSPrimary->IsLost();

	if (ddrval == DD_OK) return VIDEO_SUCCESS;

	VIDEORESULT vr = RecoverPrimarySurface();

	if (vr == VIDEO_FAIL) LOG("RecoverPrimarySurface failed\n");

	return vr;
}

/****************************************************************************************
*
*   Функция Video_GetScreenResolution
*
*   Параметры
*       pScreenWidth - указатель на переменную, в которую будет записана ширина экрана
*       pScreenHeight - указатель на переменную, в которую будет записана высота экрана
*
*   Возвращаемое значение
*       true, если разрешение экрана успешно возвращено; иначе false.
*
*   Возвращает ширину и высоту экрана в пикселах.
*
****************************************************************************************/

bool Video_GetScreenResolution(
	__out PDWORD pScreenWidth,
	__out PDWORD pScreenHeight)
{
	if (pScreenWidth == NULL || pScreenHeight == NULL) return false;

	if (Video_CheckScreenStatus() == VIDEO_FAIL)
	{
		LOG("Video_GetScreenStatus failed\n");
		return false;
	}

	DDSURFACEDESC2 DDSDesc;

	ZeroMemory(&DDSDesc, sizeof(DDSDesc));
	DDSDesc.dwSize = sizeof(DDSDesc);
	DDSDesc.dwFlags = DDSD_WIDTH | DDSD_HEIGHT;

	HRESULT ddrval = g_pDDSPrimary->GetSurfaceDesc(&DDSDesc);
	if (ddrval != DD_OK)
	{
		LOG("IDirectDrawSurface4::GetSurfaceDesc failed (error 0x%X)\n", ddrval);
		ShowFatalError(MSGID_CANT_GET_SCREEN_RESOLUTION, ddrval);
		return false;
	}

	*pScreenWidth = DDSDesc.dwWidth;
	*pScreenHeight = DDSDesc.dwHeight;

	return true;
}

/****************************************************************************************
*
*   Функция Video_CreateSurface
*
*   Параметры
*       Width - ширина поверхности
*       Height - высота поверхности
*       MemoryType - тип памяти, в которой должна быть создана поверхность:
*                    MEMTYPE_VIDEO_MEMORY - в видеопамяти,
*                    MEMTYPE_SYSTEM_MEMORY - в системной памяти
*
*   Возвращаемое значение
*       Описатель созданной поверхности.
*
*   Создаёт поверхность в видеопамяти либо в системной памяти в зависимости от параметра
*   MemoryType.
*
****************************************************************************************/

HANDLE Video_CreateSurface(
	__in DWORD Width,
	__in DWORD Height,
	__in MEMORYTYPE MemoryType)
{
	DDSURFACEDESC2 DDSDesc;

	ZeroMemory(&DDSDesc, sizeof(DDSDesc));
	DDSDesc.dwSize = sizeof(DDSDesc);
	DDSDesc.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT;
	DDSDesc.dwWidth = Width;
	DDSDesc.dwHeight = Height;
	DDSDesc.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN;

	if (MemoryType == MEMTYPE_VIDEO_MEMORY)
		DDSDesc.ddsCaps.dwCaps |= DDSCAPS_VIDEOMEMORY;
	else
		DDSDesc.ddsCaps.dwCaps |= DDSCAPS_SYSTEMMEMORY;

	LPDIRECTDRAWSURFACE4 pDDSurface;

	HRESULT ddrval = g_pDDraw4->CreateSurface(&DDSDesc, &pDDSurface, NULL);
	if (ddrval != DD_OK)
	{
		LOG("IDirectDraw4::CreateSurface failed (error 0x%X)\n", ddrval);
		ShowFatalError(MSGID_CANT_CREATE_SURFACE, ddrval);
		return NULL;
	}

	return (HANDLE) pDDSurface;
}

/****************************************************************************************
*
*   Функция Video_DeleteSurface
*
*   Параметры
*       hSurface - описатель поверхности
*
*   Возвращаемое значение
*       Нет
*
*   Удаляет указанную поверхность.
*
****************************************************************************************/

void Video_DeleteSurface(
	__in HANDLE hSurface)
{
	if (hSurface != NULL)
	{
		((LPDIRECTDRAWSURFACE4) hSurface)->Release();
	}
}

/****************************************************************************************
*
*   Функция Video_GetDC
*
*   Параметры
*       hSurface - описатель поверхности
*
*   Возвращаемое значение
*       Описатель созданного GDI-совместимого контекста устройства.
*
*   Создаёт GDI-совместимый контекст устройства для указанной поверхности.
*
****************************************************************************************/

HDC Video_GetDC(
	__in HANDLE hSurface)
{
	if (hSurface == NULL) return NULL;

	HDC hdc;

	HRESULT ddrval = ((LPDIRECTDRAWSURFACE4) hSurface)->GetDC(&hdc);
	if (ddrval != DD_OK)
	{
		LOG("IDirectDrawSurface4::GetDC failed (error 0x%X)\n", ddrval);
		ShowFatalError(MSGID_CANT_GET_DC, ddrval);
		return NULL;
	}

	return hdc;
}

/****************************************************************************************
*
*   Функция Video_ReleaseDC
*
*   Параметры
*       hSurface - описатель поверхности
*       hdc - описатель GDI-совместимого контекста устройства для поверхности hSurface
*
*   Возвращаемое значение
*       Нет
*
*   Освобождает указанный GDI-совместимый контекст устройства для поверхности hSurface.
*
****************************************************************************************/

void Video_ReleaseDC(
	__in HANDLE hSurface,
	__in HDC hdc)
{
	if (hSurface == NULL || hdc == NULL) return;

	HRESULT ddrval = ((LPDIRECTDRAWSURFACE4) hSurface)->ReleaseDC(hdc);
	if (ddrval != DD_OK)
	{
		LOG("IDirectDrawSurface4::ReleaseDC failed (error 0x%X)\n", ddrval);
	}
}

/****************************************************************************************
*
*   Функция Video_SetHwndForClipping
*
*   Параметры
*       hwnd - описатель окна
*
*   Возвращаемое значение
*       true, если описатель окна успешно установлен; иначе false.
*
*   Устанавливает описатель окна, который будет использоваться для обрезки прямоугольных
*   областей, копируемых на экран с помощью функции Video_BltRectToScreen.
*
****************************************************************************************/

bool Video_SetHwndForClipping(
	__in HWND hwnd)
{
	HRESULT ddrval = g_pDDClipper->SetHWnd(0, hwnd);
	if (ddrval != DD_OK)
	{
		LOG("IDirectDrawClipper::SetHWnd failed (error 0x%X)\n", ddrval);
		ShowFatalError(MSGID_CANT_SET_HWND_FOR_CLIPPING, ddrval);
		return false;
	}

	return true;
}

/****************************************************************************************
*
*   Функция Video_BltRectToScreen
*
*   Параметры
*       x - х-координата левого верхнего угла прямоугольной области экрана, в которую
*           нужно скопировать прямоугольную область prcSrc поверхности hSrcSurface
*       y - y-координата левого верхнего угла прямоугольной области экрана, в которую
*           нужно скопировать прямоугольную область prcSrc поверхности hSrcSurface
*       hSrcSurface - описатель поверхности, с которой нужно cкопировать прямоугольную
*                     область prcSrc на экран
*       prcSrc - указатель на структуру RECT, задающую координаты прямоугольной области
*                поверхности hSrcSurface, которую нужно скопировать на экран
*
*   Возвращаемое значение
*       VIDEO_SUCCESS, если копирование произведено успешно;
*       VIDEO_FAIL, если произошла неисправимая ошибка;
*       VIDEO_SCREEN_PIXEL_FORMAT_CHANGED, если изменился формат пикселов экрана, в этом
*                                          случае необходимо пересоздать поверхность
*                                          hSrcSurface и вызвать эту функцию снова.
*
*   Копирует прямоугольную область, заданную параметром prcSrc, с поверхности hSrcSurface
*   на прямоугольную область экрана, левый верхний угол которой задан параметрами х и у.
*
****************************************************************************************/

VIDEORESULT Video_BltRectToScreen(
	__in int x,
	__in int y,
	__in HANDLE hSrcSurface,
	__in RECT *prcSrc)
{
	if (hSrcSurface == NULL || prcSrc == NULL) return VIDEO_FAIL;

	LONG SrcWidth = prcSrc->right - prcSrc->left;
	LONG SrcHeight = prcSrc->bottom - prcSrc->top;

	if (SrcWidth == 0 || SrcHeight == 0) return VIDEO_SUCCESS;

	RECT rcDst = {x, y, x + SrcWidth, y + SrcHeight};

	HRESULT ddrval = g_pDDSPrimary->Blt(&rcDst,
		(LPDIRECTDRAWSURFACE4) hSrcSurface, prcSrc, DDBLT_WAIT, NULL);

	if (ddrval == DDERR_SURFACELOST)
	{
		VIDEORESULT vr = RecoverPrimarySurface();

		if (vr != VIDEO_SUCCESS)
		{
			if (vr == VIDEO_FAIL) LOG("RecoverPrimarySurface failed\n");

			return vr;
		}

		ddrval = g_pDDSPrimary->Blt(&rcDst,
			(LPDIRECTDRAWSURFACE4) hSrcSurface, prcSrc, DDBLT_WAIT, NULL);
	}

	if (ddrval != DD_OK)
	{
		LOG("IDirectDrawSurface4::Blt to screen failed (error 0x%X)\n", ddrval);
		ShowFatalError(MSGID_CANT_BLT_TO_SCREEN, ddrval);
		return VIDEO_FAIL;
	}

	return VIDEO_SUCCESS;
}

/****************************************************************************************
*
*   Функция Video_BltRectFromScreen
*
*   Параметры
*       x - х-координата левого верхнего угла прямоугольной области экрана, которую нужно
*           скопировать в прямоугольную область prcDst поверхности hDstSurface
*       y - y-координата левого верхнего угла прямоугольной области экрана, которую нужно
*           скопировать в прямоугольную область prcDst поверхности hDstSurface
*       hDstSurface - описатель поверхности, на которую нужно скопировать прямоугольную
*                     область экрана
*       prcDst - указатель на структуру RECT, задающую координаты прямоугольной области
*                поверхности hDstSurface, на которую нужно скопировать прямоугольную
*                область экрана
*
*   Возвращаемое значение
*       true, если копирование произведено успешно; иначе false.
*
*   Копирует прямоугольную область экрана, левый верхний угол которой задан параметрами х
*   и у, на прямоугольную область поверхности hDstSurface, заданную параметром prcDst.
*
****************************************************************************************/

bool Video_BltRectFromScreen(
	__in int x,
	__in int y,
	__in HANDLE hDstSurface,
	__in RECT *prcDst)
{
	RECT rcSrc;
	rcSrc.left = x;
	rcSrc.top = y;
	rcSrc.right = x + prcDst->right - prcDst->left;
	rcSrc.bottom = y + prcDst->bottom - prcDst->top;

	HRESULT ddrval = ((LPDIRECTDRAWSURFACE4) hDstSurface)->Blt(prcDst,
		g_pDDSPrimary, &rcSrc, DDBLT_WAIT, NULL);

	// проверить код возврата при смене разрешения экрана!!!

	if (ddrval != DD_OK)
	{
		LOG("IDirectDrawSurface4::Blt from screen failed (error 0x%X)\n", ddrval);
		ShowFatalError(MSGID_CANT_BLT_FROM_SCREEN, ddrval);
		return false;
	}

	return true;
}

/****************************************************************************************
*
*   Функция EstimateVSyncPeriod
*
*   Параметры
*       nSamples - количество элементов в выборке для произведения оценки
*
*   Возвращаемое значение
*       INDETERMINATE - VSync период не удалось оценить
*       в остальных случаях - VSync период в секундах
*
*   Данная функция находит оценку VSync периода.
*   Сначала замеряем время обратного хода луча. Это первый элемент выборки. Необходимо
*   повторить данный замер времени ещё (nSamples - 1) раз.
*   Затем для данной выборки находим среднее периодического процесса. Полученное число
*   и будет значением VSync периода (в секундах).
*
****************************************************************************************/

double Video_EstimateVSyncPeriod(DWORD nSamples)
{
/*
	double *pVSyncPeriodArray = (double *) VirtualAlloc(NULL,
								nSamples * sizeof(double),
								MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);

	if (pVSyncPeriodArray == NULL)
	{
		ShowFatalError(MSGID_CANT_ALLOC_MEMORY);
		return INDETERMINATE;
	}

	LARGE_INTEGER CounterBeginValue, CounterEndValue, Frequency;
	QueryPerformanceFrequency(&Frequency);

	for (DWORD i = 0; i < nSamples; i++)
	{
		DirectDraw_WaitForVerticalBlank();

		QueryPerformanceCounter(&CounterBeginValue);

		DirectDraw_WaitForVerticalBlank();

		QueryPerformanceCounter(&CounterEndValue);

		pVSyncPeriodArray[i] = (double) (CounterEndValue.QuadPart -
			CounterBeginValue.QuadPart) / Frequency.QuadPart;
	}
	/*
	LOG("MIN_VSYNC_PERIOD = %f\n", MIN_VSYNC_PERIOD);
	LOG("MAX_VSYNC_PERIOD = %f\n", MAX_VSYNC_PERIOD);

	int cSmallerValues = 0;
	for (int i = 0; i < nSamples; i++)
	{
		if (pVSyncPeriodArray[i] < 0.016643)
		{
			cSmallerValues ++;

			LOG("pVSyncPeriodArray[%d]\t\t= %.4f\n",
				i, pVSyncPeriodArray[i] * 1000);
		}
	}

	LOG("cSmallerValues = %d  from  %d values\n", cSmallerValues, nSamples);

	VirtualFree(pVSyncPeriodArray, 0, MEM_RELEASE);

	return INDETERMINATE;
	//*

	double drVSyncPeriod = EstimateMeanOfPeriodicProcess(pVSyncPeriodArray,
				nSamples, MIN_VSYNC_PERIOD, MAX_VSYNC_PERIOD);

	g_drMonitorFrequency = 1 / drVSyncPeriod;

	VirtualFree(pVSyncPeriodArray, 0, MEM_RELEASE);

	return drVSyncPeriod;
*/

	return 0; //!!!!
}

/****************************************************************************************
*
*   Функция CreatePrimarySurface
*
*   Параметры
*       Нет
*
*   Возвращаемое значение
*       true, если первичная поверхность успешно создана; иначе false.
*
*   Создаёт первичную поверхность и устанавливает у неё объект обрезки. На интерфейс
*   IDirectDrawClipper объекта обрезки должна указывать глобальная переменная
*   g_pDDClipper. Функция присваивает указатель на интерфейс IDirectDrawSurface4
*   созданной поверхности глобальной переменной g_pDDSPrimary.
*
****************************************************************************************/

static bool CreatePrimarySurface()
{
	if (g_pDDSPrimary != NULL || g_pDDClipper == NULL) return false;

	DDSURFACEDESC2 DDSDesc;

	ZeroMemory(&DDSDesc, sizeof(DDSDesc));
	DDSDesc.dwSize = sizeof(DDSDesc);
	DDSDesc.dwFlags = DDSD_CAPS;
	DDSDesc.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE;

	HRESULT ddrval = g_pDDraw4->CreateSurface(&DDSDesc, &g_pDDSPrimary, NULL);
	if (ddrval != DD_OK)
	{
		LOG("IDirectDraw4::CreateSurface failed (error 0x%X)\n", ddrval);
		ShowFatalError(MSGID_CANT_CREATE_PRIMARY_SURFACE, ddrval);
		return false;
	}

	ddrval = g_pDDSPrimary->SetClipper(g_pDDClipper);
	if (ddrval != DD_OK)
	{
		LOG("IDirectDrawSurface4::SetClipper failed (error 0x%X)\n", ddrval);
		ShowFatalError(MSGID_CANT_SET_CLIPPER, ddrval);
		return false;
	}

	return true;
}

/****************************************************************************************
*
*   Функция ReleasePrimarySurface
*
*   Параметры
*       Нет
*
*   Возвращаемое значение
*       Нет
*
*   Освобождает первичную поверхность. На интерфейс IDirectDrawSurface4 этой поверхности
*   должна указывать глобальная переменная g_pDDSPrimary.
*
****************************************************************************************/

static void ReleasePrimarySurface()
{
	if (g_pDDSPrimary != NULL)
	{
		g_pDDSPrimary->Release();
		g_pDDSPrimary = NULL;
	}
}

/****************************************************************************************
*
*   Функция GetScreenPixelFormat
*
*   Параметры
*       pRGBBitCount - указатель на переменную, в которую будет записано количество битов
*                      на пиксел
*       pRBitMask - указатель на переменную, в которую будет записана битовая маска для
*                   красного канала
*       pGBitMask - указатель на переменную, в которую будет записана битовая маска для
*                   зелёного канала
*       pBBitMask - указатель на переменную, в которую будет записана битовая маска для
*                   синего канала
*
*   Возвращаемое значение
*       true, если формат пикселов успешно возвращён; иначе false.
*
*   Возвращает формат пикселов экрана. Если формат пикселов отличен от RGB, функция
*   возвращает false.
*
****************************************************************************************/

static bool GetScreenPixelFormat(
	__out PDWORD pRGBBitCount,
	__out PDWORD pRBitMask,
	__out PDWORD pGBitMask,
	__out PDWORD pBBitMask)
{
	if (pRGBBitCount == NULL || pRBitMask == NULL ||
		pGBitMask == NULL || pBBitMask == NULL)
	{
		return false;
	}

	DDSURFACEDESC2 DDSDesc;

	ZeroMemory(&DDSDesc, sizeof(DDSDesc));
	DDSDesc.dwSize = sizeof(DDSDesc);
	DDSDesc.dwFlags = DDSD_PIXELFORMAT;

	HRESULT ddrval = g_pDDSPrimary->GetSurfaceDesc(&DDSDesc);
	if (ddrval != DD_OK)
	{
		LOG("IDirectDrawSurface4::GetSurfaceDesc failed (error 0x%X)\n", ddrval);
		ShowFatalError(MSGID_CANT_GET_SCREEN_PIXEL_FORMAT, ddrval);
		return false;
	}

	if ((DDSDesc.ddpfPixelFormat.dwFlags & DDPF_RGB) == 0)
	{
		LOG("screen pixel format is not RGB\n");
		ShowFatalError(MSGID_UNSUPPORTED_SCREEN_PIXEL_FORMAT);
		return false;
	}

	*pRGBBitCount = DDSDesc.ddpfPixelFormat.dwRGBBitCount;
	*pRBitMask = DDSDesc.ddpfPixelFormat.dwRBitMask;
	*pGBitMask = DDSDesc.ddpfPixelFormat.dwGBitMask;
	*pBBitMask = DDSDesc.ddpfPixelFormat.dwBBitMask;

	return true;
}

/****************************************************************************************
*
*   Функция RecoverPrimarySurface
*
*   Параметры
*       Нет
*
*   Возвращаемое значение
*       VIDEO_SUCCESS, если первичная поверхность успешно восстановлена;
*       VIDEO_FAIL, если первичную поверхность не удалось восстановить;
*       VIDEO_SCREEN_PIXEL_FORMAT_CHANGED, если первичная поверхность успешно
*                                          восстановлена, но формат пикселов экрана
*                                          изменился.
*
*   Восстанавливает первичную поверхность после того, как она была потеряна.
*
****************************************************************************************/

static VIDEORESULT RecoverPrimarySurface()
{
	HRESULT ddrval = g_pDDSPrimary->Restore(); 

	if (ddrval == DDERR_WRONGMODE)
	{
		ReleasePrimarySurface();

		if (!CreatePrimarySurface())
		{
			LOG("CreatePrimarySurface failed\n");
			return VIDEO_FAIL;
		}

		DWORD RGBBitCount;
		DWORD RBitMask;
		DWORD GBitMask;
		DWORD BBitMask;

		if (!GetScreenPixelFormat(&RGBBitCount, &RBitMask, &GBitMask, &BBitMask))
		{
			LOG("GetScreenPixelFormat failed\n");
			return VIDEO_FAIL;
		}

		if (RGBBitCount != g_RGBBitCount ||
			RBitMask != g_RBitMask ||
			GBitMask != g_GBitMask ||
			BBitMask != g_BBitMask)
		{
			g_RGBBitCount = RGBBitCount;
			g_RBitMask = RBitMask;
			g_GBitMask = GBitMask;
			g_BBitMask = BBitMask;

			return VIDEO_SCREEN_PIXEL_FORMAT_CHANGED;
		}
	}
	else if (ddrval != DD_OK)
	{
		LOG("IDirectDrawSurface4::Restore failed (error 0x%X)\n", ddrval);
		ShowFatalError(MSGID_CANT_RESTORE_PRIMARY_SURFACE, ddrval);
		return VIDEO_FAIL;
	}

	return VIDEO_SUCCESS;
}
