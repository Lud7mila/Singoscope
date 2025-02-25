/****************************************************************************************
*
*   ����������� ������ Video
*
*   ������������� ������� ��� ������ � �������.
*
*   �����: ������� ������������, 2010
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
*   ���������� ����������
*
****************************************************************************************/

// ��������� �� ��������� IDirectDraw
static LPDIRECTDRAW g_pDDraw = NULL;

// ��������� �� ��������� IDirectDraw4
static LPDIRECTDRAW4 g_pDDraw4 = NULL;

// ��������� �� ��������� IDirectDrawSurface4 ��� ��������� �����������
static LPDIRECTDRAWSURFACE4 g_pDDSPrimary = NULL;

// ��������� �� ��������� IDirectDrawClipper ��� ������� �������, ������������� ���
// ��������� �����������
static LPDIRECTDRAWCLIPPER g_pDDClipper = NULL;

// ���������� ����� �� ������ ������
static DWORD g_RGBBitCount;

// ������� ����� ��� �������� ������ ������� ������
static DWORD g_RBitMask;

// ������� ����� ��� ������� ������ ������� ������
static DWORD g_GBitMask;

// ������� ����� ��� ������ ������ ������� ������
static DWORD g_BBitMask;

/****************************************************************************************
*
*   ��������� �������, ����������� ����
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
*   ������� Video_Init
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
*   ������� Video_Uninit
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
*   ������� Video_CheckScreenStatus
*
*   ���������
*       ���
*
*   ������������ ��������
*       VIDEO_SUCCESS, ���� ������ �������� ������ �� ���������;
*       VIDEO_FAIL, ���� �� ������� ����������, ��������� �� ������ �������� ������;
*       VIDEO_SCREEN_PIXEL_FORMAT_CHANGED, ������ �������� ������ ���������.
*
*   ���������, ��������� �� ������ �������� ������. � ������, ���� ��������� �����������
*   ���� ��������, ������� ��������������� �.
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
*   ������� Video_GetScreenResolution
*
*   ���������
*       pScreenWidth - ��������� �� ����������, � ������� ����� �������� ������ ������
*       pScreenHeight - ��������� �� ����������, � ������� ����� �������� ������ ������
*
*   ������������ ��������
*       true, ���� ���������� ������ ������� ����������; ����� false.
*
*   ���������� ������ � ������ ������ � ��������.
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
*   ������� Video_CreateSurface
*
*   ���������
*       Width - ������ �����������
*       Height - ������ �����������
*       MemoryType - ��� ������, � ������� ������ ���� ������� �����������:
*                    MEMTYPE_VIDEO_MEMORY - � �����������,
*                    MEMTYPE_SYSTEM_MEMORY - � ��������� ������
*
*   ������������ ��������
*       ��������� ��������� �����������.
*
*   ������ ����������� � ����������� ���� � ��������� ������ � ����������� �� ���������
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
*   ������� Video_DeleteSurface
*
*   ���������
*       hSurface - ��������� �����������
*
*   ������������ ��������
*       ���
*
*   ������� ��������� �����������.
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
*   ������� Video_GetDC
*
*   ���������
*       hSurface - ��������� �����������
*
*   ������������ ��������
*       ��������� ���������� GDI-������������ ��������� ����������.
*
*   ������ GDI-����������� �������� ���������� ��� ��������� �����������.
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
*   ������� Video_ReleaseDC
*
*   ���������
*       hSurface - ��������� �����������
*       hdc - ��������� GDI-������������ ��������� ���������� ��� ����������� hSurface
*
*   ������������ ��������
*       ���
*
*   ����������� ��������� GDI-����������� �������� ���������� ��� ����������� hSurface.
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
*   ������� Video_SetHwndForClipping
*
*   ���������
*       hwnd - ��������� ����
*
*   ������������ ��������
*       true, ���� ��������� ���� ������� ����������; ����� false.
*
*   ������������� ��������� ����, ������� ����� �������������� ��� ������� �������������
*   ��������, ���������� �� ����� � ������� ������� Video_BltRectToScreen.
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
*   ������� Video_BltRectToScreen
*
*   ���������
*       x - �-���������� ������ �������� ���� ������������� ������� ������, � �������
*           ����� ����������� ������������� ������� prcSrc ����������� hSrcSurface
*       y - y-���������� ������ �������� ���� ������������� ������� ������, � �������
*           ����� ����������� ������������� ������� prcSrc ����������� hSrcSurface
*       hSrcSurface - ��������� �����������, � ������� ����� c���������� �������������
*                     ������� prcSrc �� �����
*       prcSrc - ��������� �� ��������� RECT, �������� ���������� ������������� �������
*                ����������� hSrcSurface, ������� ����� ����������� �� �����
*
*   ������������ ��������
*       VIDEO_SUCCESS, ���� ����������� ����������� �������;
*       VIDEO_FAIL, ���� ��������� ������������ ������;
*       VIDEO_SCREEN_PIXEL_FORMAT_CHANGED, ���� ��������� ������ �������� ������, � ����
*                                          ������ ���������� ����������� �����������
*                                          hSrcSurface � ������� ��� ������� �����.
*
*   �������� ������������� �������, �������� ���������� prcSrc, � ����������� hSrcSurface
*   �� ������������� ������� ������, ����� ������� ���� ������� ����� ����������� � � �.
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
*   ������� Video_BltRectFromScreen
*
*   ���������
*       x - �-���������� ������ �������� ���� ������������� ������� ������, ������� �����
*           ����������� � ������������� ������� prcDst ����������� hDstSurface
*       y - y-���������� ������ �������� ���� ������������� ������� ������, ������� �����
*           ����������� � ������������� ������� prcDst ����������� hDstSurface
*       hDstSurface - ��������� �����������, �� ������� ����� ����������� �������������
*                     ������� ������
*       prcDst - ��������� �� ��������� RECT, �������� ���������� ������������� �������
*                ����������� hDstSurface, �� ������� ����� ����������� �������������
*                ������� ������
*
*   ������������ ��������
*       true, ���� ����������� ����������� �������; ����� false.
*
*   �������� ������������� ������� ������, ����� ������� ���� ������� ����� ����������� �
*   � �, �� ������������� ������� ����������� hDstSurface, �������� ���������� prcDst.
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

	// ��������� ��� �������� ��� ����� ���������� ������!!!

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
*   ������� EstimateVSyncPeriod
*
*   ���������
*       nSamples - ���������� ��������� � ������� ��� ������������ ������
*
*   ������������ ��������
*       INDETERMINATE - VSync ������ �� ������� �������
*       � ��������� ������� - VSync ������ � ��������
*
*   ������ ������� ������� ������ VSync �������.
*   ������� �������� ����� ��������� ���� ����. ��� ������ ������� �������. ����������
*   ��������� ������ ����� ������� ��� (nSamples - 1) ���.
*   ����� ��� ������ ������� ������� ������� �������������� ��������. ���������� �����
*   � ����� ��������� VSync ������� (� ��������).
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
*   ������� CreatePrimarySurface
*
*   ���������
*       ���
*
*   ������������ ��������
*       true, ���� ��������� ����������� ������� �������; ����� false.
*
*   ������ ��������� ����������� � ������������� � �� ������ �������. �� ���������
*   IDirectDrawClipper ������� ������� ������ ��������� ���������� ����������
*   g_pDDClipper. ������� ����������� ��������� �� ��������� IDirectDrawSurface4
*   ��������� ����������� ���������� ���������� g_pDDSPrimary.
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
*   ������� ReleasePrimarySurface
*
*   ���������
*       ���
*
*   ������������ ��������
*       ���
*
*   ����������� ��������� �����������. �� ��������� IDirectDrawSurface4 ���� �����������
*   ������ ��������� ���������� ���������� g_pDDSPrimary.
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
*   ������� GetScreenPixelFormat
*
*   ���������
*       pRGBBitCount - ��������� �� ����������, � ������� ����� �������� ���������� �����
*                      �� ������
*       pRBitMask - ��������� �� ����������, � ������� ����� �������� ������� ����� ���
*                   �������� ������
*       pGBitMask - ��������� �� ����������, � ������� ����� �������� ������� ����� ���
*                   ������� ������
*       pBBitMask - ��������� �� ����������, � ������� ����� �������� ������� ����� ���
*                   ������ ������
*
*   ������������ ��������
*       true, ���� ������ �������� ������� ���������; ����� false.
*
*   ���������� ������ �������� ������. ���� ������ �������� ������� �� RGB, �������
*   ���������� false.
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
*   ������� RecoverPrimarySurface
*
*   ���������
*       ���
*
*   ������������ ��������
*       VIDEO_SUCCESS, ���� ��������� ����������� ������� �������������;
*       VIDEO_FAIL, ���� ��������� ����������� �� ������� ������������;
*       VIDEO_SCREEN_PIXEL_FORMAT_CHANGED, ���� ��������� ����������� �������
*                                          �������������, �� ������ �������� ������
*                                          ���������.
*
*   ��������������� ��������� ����������� ����� ����, ��� ��� ���� ��������.
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
