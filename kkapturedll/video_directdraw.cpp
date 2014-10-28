/* kkapture: intrusive demo video capturing.
 * Copyright (c) 2005-2010 Fabian "ryg/farbrausch" Giesen.
 *
 * This program is free software; you can redistribute and/or modify it under
 * the terms of the Artistic License, Version 2.0beta5, or (at your opinion)
 * any later version; all distributions of this program should contain this
 * license in a file named "LICENSE.txt".
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT UNLESS REQUIRED BY
 * LAW OR AGREED TO IN WRITING WILL ANY COPYRIGHT HOLDER OR CONTRIBUTOR
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY,
 * OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
 * OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "stdafx.h"
#include "video.h"

#include <InitGuid.h>
#include <ddraw.h>

// DON'T LOOK, this is by far the messiest source in this project...

typedef HRESULT (__stdcall *PDirectDrawCreate)(GUID *lpGUID,LPDIRECTDRAW *lplpDD,IUnknown *pUnkOuter);
typedef HRESULT (__stdcall *PDirectDrawCreateEx)(GUID *lpGUID,LPVOID *lplpDD,REFIID iid,IUnknown *pUnkOuter);

typedef HRESULT (__stdcall *PQueryInterface)(IUnknown *dd,REFIID iid,LPVOID *ppObj);
typedef HRESULT (__stdcall *PDDraw_CreateSurface)(IUnknown *dd,LPDDSURFACEDESC ddsd,LPDIRECTDRAWSURFACE *surf,IUnknown *pUnkOuter);
typedef HRESULT (__stdcall *PDDraw_SetCooperativeLevel)(IUnknown *dd,HWND, DWORD);
typedef HRESULT (__stdcall *PDDraw_SetDisplayMode)(IUnknown *dd, DWORD, DWORD,DWORD);
typedef HRESULT (__stdcall *PDDraw2_SetDisplayMode)(IUnknown *dd, DWORD, DWORD,DWORD, DWORD, DWORD);
typedef HRESULT (__stdcall *PDDraw4_SetDisplayMode)(IUnknown *dd, DWORD, DWORD,DWORD, DWORD, DWORD);
typedef HRESULT (__stdcall *PDDraw7_SetDisplayMode)(IUnknown *dd, DWORD, DWORD,DWORD, DWORD, DWORD);
typedef HRESULT (__stdcall *PDDrawSurface_Flip)(IUnknown *dds,IUnknown *surf,DWORD flags);
typedef HRESULT (__stdcall *PDDrawSurface_Lock)(IUnknown *dds,LPRECT rect,LPDDSURFACEDESC desc,DWORD flags,HANDLE hnd);
typedef HRESULT (__stdcall *PDDrawSurface_Unlock)(IUnknown *dds,void *ptr);

static PDirectDrawCreate Real_DirectDrawCreate = 0;
static PDirectDrawCreateEx Real_DirectDrawCreateEx = 0;

static PQueryInterface Real_DDraw_QueryInterface = 0;
static PDDraw_CreateSurface Real_DDraw_CreateSurface = 0;
static PDDraw_SetCooperativeLevel Real_DDraw_SetCooperativeLevel = 0;
static PDDraw_SetDisplayMode Real_DDraw_SetDisplayMode = 0;
static PQueryInterface Real_DDrawSurface_QueryInterface = 0;
static PDDrawSurface_Flip Real_DDrawSurface_Flip = 0;
static PDDrawSurface_Lock Real_DDrawSurface_Lock = 0;
static PDDrawSurface_Unlock Real_DDrawSurface_Unlock = 0;

static PQueryInterface Real_DDraw2_QueryInterface = 0;
static PDDraw_CreateSurface Real_DDraw2_CreateSurface = 0;
static PDDraw_SetCooperativeLevel Real_DDraw2_SetCooperativeLevel = 0;
static PDDraw2_SetDisplayMode Real_DDraw2_SetDisplayMode = 0;
static PQueryInterface Real_DDrawSurface2_QueryInterface = 0;
static PDDrawSurface_Flip Real_DDrawSurface2_Flip = 0;
static PDDrawSurface_Lock Real_DDrawSurface2_Lock = 0;
static PDDrawSurface_Unlock Real_DDrawSurface2_Unlock = 0;

static PQueryInterface Real_DDrawSurface3_QueryInterface = 0;
static PDDrawSurface_Flip Real_DDrawSurface3_Flip = 0;
static PDDrawSurface_Lock Real_DDrawSurface3_Lock = 0;
static PDDrawSurface_Unlock Real_DDrawSurface3_Unlock = 0;

static PQueryInterface Real_DDraw4_QueryInterface = 0;
static PDDraw_CreateSurface Real_DDraw4_CreateSurface = 0;
static PDDraw_SetCooperativeLevel Real_DDraw4_SetCooperativeLevel = 0;
static PDDraw4_SetDisplayMode Real_DDraw4_SetDisplayMode = 0;
static PQueryInterface Real_DDrawSurface4_QueryInterface = 0;
static PDDrawSurface_Flip Real_DDrawSurface4_Flip = 0;
static PDDrawSurface_Lock Real_DDrawSurface4_Lock = 0;
static PDDrawSurface_Unlock Real_DDrawSurface4_Unlock = 0;

static PQueryInterface Real_DDraw7_QueryInterface = 0;
static PDDraw_CreateSurface Real_DDraw7_CreateSurface = 0;
static PDDraw_SetCooperativeLevel Real_DDraw7_SetCooperativeLevel = 0;
static PDDraw7_SetDisplayMode Real_DDraw7_SetDisplayMode = 0;
static PQueryInterface Real_DDrawSurface7_QueryInterface = 0;
static PDDrawSurface_Flip Real_DDrawSurface7_Flip = 0;
static PDDrawSurface_Lock Real_DDrawSurface7_Lock = 0;
static PDDrawSurface_Unlock Real_DDrawSurface7_Unlock = 0;

static IUnknown *PrimaryDDraw = 0;
static IUnknown *PrimarySurface = 0;
static int PrimarySurfaceVersion = 0;

static ULONG DDrawRefCount = 0;

// TODO: Scale up to a non-multiple resolution (desktop?), then use integer scaling and center image

DWORD dwMultiplier = 4; // currently hardwired
DWORD dwOriginalWidth = 0;
DWORD dwOriginalHeight = 0;
DWORD dwOffsetX = 0;
DWORD dwOffsetY = 0;
HWND hwOriginal = NULL;
DWORD dwOriginalBPP = 0;
void ModifyResolution( DWORD* dwWidth, DWORD* dwHeight )
{
  dwOriginalWidth = *dwWidth;
  dwOriginalHeight = *dwHeight;

  printLog("current resolution: %d * %d\n",*dwWidth,*dwHeight);
  if (*dwWidth < 640 || *dwHeight < 480)
  {
//     *dwWidth *= dwMultiplier;
//     *dwHeight *= dwMultiplier;
    *dwWidth  = GetSystemMetrics( SM_CXSCREEN );
    *dwHeight = GetSystemMetrics( SM_CYSCREEN );
    DWORD nZoomedX = 0;
    DWORD nZoomedY = 0;
    for (int i = 1; dwOriginalWidth * i <= *dwWidth && dwOriginalHeight * i <= *dwHeight; i++)
    {
      dwMultiplier = i;
      nZoomedX = dwOriginalWidth  * i;
      nZoomedY = dwOriginalHeight * i;
    }

    dwOffsetX = (*dwWidth  - nZoomedX) / 2;
    dwOffsetY = (*dwHeight - nZoomedY) / 2;

    SetWindowPos(hwOriginal, 0, 0, 0, *dwWidth, *dwHeight, SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE );
    SetWindowLong(hwOriginal, GWL_STYLE, WS_POPUP | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | WS_VISIBLE );
    ShowWindow(hwOriginal, SW_SHOW);
    SetForegroundWindow(hwOriginal);
    SetFocus(hwOriginal);

    printLog("scaled resolution: %d * %d\n",*dwWidth,*dwHeight);
  }
}

static void PatchDDrawInterface(IUnknown *dd,int version);
static void PatchDDrawSurface(IUnknown *surf,int version);

static HRESULT DDrawQueryInterface(HRESULT hr,REFIID iid,LPVOID *ppObject)
{
  if(FAILED(hr) || !ppObject || !*ppObject)
    return hr;

  IUnknown *iface = (IUnknown *) *ppObject;
  if(iid == IID_IDirectDraw)
    PatchDDrawInterface(iface,1);
  else if(iid == IID_IDirectDraw2)
    PatchDDrawInterface(iface,2);
  else if(iid == IID_IDirectDraw4)
    PatchDDrawInterface(iface,4);
  else if(iid == IID_IDirectDraw7)
    PatchDDrawInterface(iface,7);

  return hr;
}

static HRESULT DDrawSurfQueryInterface(HRESULT hr,REFIID iid,LPVOID *ppObject)
{
  if(FAILED(hr) || !ppObject || !*ppObject)
    return hr;

  IUnknown *iface = (IUnknown *) *ppObject;
  if(iid == IID_IDirectDrawSurface)
    PatchDDrawSurface(iface,1);
  else if(iid == IID_IDirectDrawSurface2)
    PatchDDrawSurface(iface,2);
  else if(iid == IID_IDirectDrawSurface3)
    PatchDDrawSurface(iface,3);
  else if(iid == IID_IDirectDrawSurface4)
    PatchDDrawSurface(iface,4);
  else if(iid == IID_IDirectDrawSurface7)
    PatchDDrawSurface(iface,7);

  return hr;
}

BYTE * pTempBuffer = NULL;

void LockAndAllocate( LPDDSURFACEDESC desc )
{
  if (!pTempBuffer)
  {
    pTempBuffer = new BYTE[ dwOriginalWidth * dwOriginalHeight * (dwOriginalBPP / 8) ];
  }
  desc->lPitch = dwOriginalWidth * (dwOriginalBPP / 8);
  desc->dwWidth = dwOriginalWidth;
  desc->dwHeight = dwOriginalHeight;
  desc->lpSurface = pTempBuffer;
}

HRESULT __stdcall UnlockAndScale( IUnknown * me, PDDrawSurface_Lock lock, PDDrawSurface_Unlock unlock )
{
  DDSURFACEDESC desc;
  ZeroMemory(&desc, sizeof(DDSURFACEDESC));
  desc.dwSize = sizeof (DDSURFACEDESC);
  HRESULT hRes = lock( me, NULL, &desc, DDLOCK_SURFACEMEMORYPTR | DDLOCK_WAIT, NULL );
  if (hRes != DD_OK) return hRes;

  ZeroMemory( desc.lpSurface, desc.lPitch * desc.dwHeight );

  BYTE pixelSize = (dwOriginalBPP / 8);
  BYTE * pSrc = pTempBuffer;
  BYTE * pDst = (BYTE*)desc.lpSurface + dwOffsetY * desc.lPitch + dwOffsetX * pixelSize;
  for (int y=0; y<dwOriginalHeight; y++)
  {
    BYTE * pSrcLine = pSrc;
    for (int my = 0; my < dwMultiplier; my++)
    {
      BYTE * pDstLine = pDst;
      pSrc = pSrcLine;
      for (int x=0; x<dwOriginalWidth; x++)
      {
        for (int mx = 0; mx < dwMultiplier; mx++)
        {
          CopyMemory( pDst, pSrc, pixelSize );
          pDst += pixelSize;
        }
        pSrc += pixelSize;
      }
      pDst = pDstLine + desc.lPitch;
    }
    pSrc = pSrcLine + pixelSize * dwOriginalWidth;
  }

  return unlock( me, NULL );
}

// ---- directdraw 1

static HRESULT __stdcall Mine_DDraw_QueryInterface(IUnknown *dd,REFIID iid,LPVOID *ppObj)
{
  return DDrawQueryInterface(Real_DDraw_QueryInterface(dd,iid,ppObj),iid,ppObj);
}

static HRESULT __stdcall Mine_DDraw_CreateSurface(IDirectDraw *dd,LPDDSURFACEDESC ddsd,LPDIRECTDRAWSURFACE *pSurf,IUnknown *pUnkOuter)
{
  HRESULT hr = Real_DDraw_CreateSurface(dd,ddsd,pSurf,pUnkOuter);
  if(SUCCEEDED(hr))
  {
    PatchDDrawSurface(*pSurf,1);
  }

  return hr;
}

static HRESULT __stdcall Mine_DDraw_SetCooperativeLevel(IDirectDraw *dd,HWND hWnd, DWORD dwFlags)
{
  hwOriginal = hWnd;
  return Real_DDraw_SetCooperativeLevel(dd,hWnd,dwFlags);
}

static HRESULT __stdcall Mine_DDraw_SetDisplayMode(IDirectDraw *dd,DWORD dwWidth,DWORD dwHeight,DWORD dwBPP)
{
  ModifyResolution(&dwWidth,&dwHeight);
  dwOriginalBPP = dwBPP;
  HRESULT hr = Real_DDraw_SetDisplayMode(dd,dwWidth, dwHeight, dwBPP);
  return hr;
}

static HRESULT __stdcall Mine_DDrawSurface_QueryInterface(IUnknown *dd,REFIID iid,LPVOID *ppObj)
{
  return DDrawSurfQueryInterface(Real_DDrawSurface_QueryInterface(dd,iid,ppObj),iid,ppObj);
}

static HRESULT __stdcall Mine_DDrawSurface_Flip(IUnknown *me,IUnknown *other,DWORD flags)
{
  return Real_DDrawSurface_Flip(me,other,flags);
}

static HRESULT __stdcall Mine_DDrawSurface_Lock(IUnknown *me,LPRECT rect,LPDDSURFACEDESC desc,DWORD flags,HANDLE hnd)
{
  LockAndAllocate(desc);
  return DD_OK;
}

static HRESULT __stdcall Mine_DDrawSurface_Unlock(IUnknown *me,void *ptr)
{
  return UnlockAndScale(me, Real_DDrawSurface_Lock, Real_DDrawSurface_Unlock);
}

// ---- directdraw 2

static HRESULT __stdcall Mine_DDraw2_QueryInterface(IUnknown *dd,REFIID iid,LPVOID *ppObj)
{
  return DDrawQueryInterface(Real_DDraw2_QueryInterface(dd,iid,ppObj),iid,ppObj);
}

static HRESULT __stdcall Mine_DDraw2_CreateSurface(IDirectDraw2 *dd,LPDDSURFACEDESC ddsd,LPDIRECTDRAWSURFACE *pSurf,IUnknown *pUnkOuter)
{
  HRESULT hr = Real_DDraw2_CreateSurface(dd,ddsd,pSurf,pUnkOuter);
  if(SUCCEEDED(hr))
  {
    PatchDDrawSurface(*pSurf,1);
  }

  return hr;
}

static HRESULT __stdcall Mine_DDraw2_SetCooperativeLevel(IDirectDraw *dd,HWND hWnd, DWORD dwFlags)
{
  hwOriginal = hWnd;
  return Real_DDraw2_SetCooperativeLevel(dd,hWnd,dwFlags);
}

static HRESULT __stdcall Mine_DDraw2_SetDisplayMode(IDirectDraw *dd,DWORD dwWidth,DWORD dwHeight,DWORD dwBPP,DWORD dwRefreshRate,DWORD dwFlags)
{
  ModifyResolution(&dwWidth,&dwHeight);
  dwOriginalBPP = dwBPP;
  HRESULT hr = Real_DDraw2_SetDisplayMode(dd,dwWidth, dwHeight, dwBPP, dwRefreshRate, dwFlags);
  return hr;
}

static HRESULT __stdcall Mine_DDrawSurface2_QueryInterface(IUnknown *dd,REFIID iid,LPVOID *ppObj)
{
  return DDrawSurfQueryInterface(Real_DDrawSurface2_QueryInterface(dd,iid,ppObj),iid,ppObj);
}

static HRESULT __stdcall Mine_DDrawSurface2_Flip(IUnknown *me,IUnknown *other,DWORD flags)
{
  return Real_DDrawSurface2_Flip(me,other,flags);
}

static HRESULT __stdcall Mine_DDrawSurface2_Lock(IUnknown *me,LPRECT rect,LPDDSURFACEDESC desc,DWORD flags,HANDLE hnd)
{
  LockAndAllocate(desc);
  return DD_OK;
}

static HRESULT __stdcall Mine_DDrawSurface2_Unlock(IUnknown *me,void *ptr)
{
  return UnlockAndScale(me, Real_DDrawSurface2_Lock, Real_DDrawSurface2_Unlock);
}

// ---- directdraw 3

static HRESULT __stdcall Mine_DDrawSurface3_QueryInterface(IUnknown *dd,REFIID iid,LPVOID *ppObj)
{
  return DDrawSurfQueryInterface(Real_DDrawSurface3_QueryInterface(dd,iid,ppObj),iid,ppObj);
}

static HRESULT __stdcall Mine_DDrawSurface3_Flip(IUnknown *me,IUnknown *other,DWORD flags)
{
  return Real_DDrawSurface3_Flip(me,other,flags);
}

static HRESULT __stdcall Mine_DDrawSurface3_Lock(IUnknown *me,LPRECT rect,LPDDSURFACEDESC desc,DWORD flags,HANDLE hnd)
{
  LockAndAllocate(desc);
  return DD_OK;
}

static HRESULT __stdcall Mine_DDrawSurface3_Unlock(IUnknown *me,void *ptr)
{
  return UnlockAndScale(me, Real_DDrawSurface3_Lock, Real_DDrawSurface3_Unlock);
}

// ---- directdraw 4

static HRESULT __stdcall Mine_DDraw4_QueryInterface(IUnknown *dd,REFIID iid,LPVOID *ppObj)
{
  return DDrawQueryInterface(Real_DDraw4_QueryInterface(dd,iid,ppObj),iid,ppObj);
}
static HRESULT __stdcall Mine_DDraw4_SetCooperativeLevel(IDirectDraw *dd,HWND hWnd, DWORD dwFlags)
{
  hwOriginal = hWnd;
  return Real_DDraw4_SetCooperativeLevel(dd,hWnd,dwFlags);
}
static HRESULT __stdcall Mine_DDraw4_SetDisplayMode(IDirectDraw *dd,DWORD dwWidth,DWORD dwHeight,DWORD dwBPP,DWORD dwRefreshRate,DWORD dwFlags)
{
  ModifyResolution(&dwWidth,&dwHeight);
  dwOriginalBPP = dwBPP;
  HRESULT hr = Real_DDraw4_SetDisplayMode(dd,dwWidth, dwHeight, dwBPP, dwRefreshRate, dwFlags);
  return hr;
}

static HRESULT __stdcall Mine_DDraw4_CreateSurface(IDirectDraw4 *dd,LPDDSURFACEDESC2 ddsd,LPDIRECTDRAWSURFACE4 *pSurf,IUnknown *pUnkOuter)
{
  HRESULT hr = Real_DDraw4_CreateSurface(dd,(LPDDSURFACEDESC) ddsd,(LPDIRECTDRAWSURFACE *) pSurf,pUnkOuter);
  if(SUCCEEDED(hr))
  {
    PatchDDrawSurface(*pSurf,4);
  }

  return hr;
}

static HRESULT __stdcall Mine_DDrawSurface4_QueryInterface(IUnknown *dd,REFIID iid,LPVOID *ppObj)
{
  return DDrawSurfQueryInterface(Real_DDrawSurface4_QueryInterface(dd,iid,ppObj),iid,ppObj);
}

static HRESULT __stdcall Mine_DDrawSurface4_Flip(IUnknown *me,IUnknown *other,DWORD flags)
{
  return Real_DDrawSurface4_Flip(me,other,flags);
}

static HRESULT __stdcall Mine_DDrawSurface4_Lock(IUnknown *me,LPRECT rect,LPDDSURFACEDESC desc,DWORD flags,HANDLE hnd)
{
  LockAndAllocate(desc);
  return DD_OK;
}


static HRESULT __stdcall Mine_DDrawSurface4_Unlock(IUnknown *me,void *ptr)
{
  return UnlockAndScale(me, Real_DDrawSurface4_Lock, Real_DDrawSurface4_Unlock);
}

// ---- directdraw 7

static HRESULT __stdcall Mine_DDraw7_QueryInterface(IUnknown *dd,REFIID iid,LPVOID *ppObj)
{
  return DDrawQueryInterface(Real_DDraw7_QueryInterface(dd,iid,ppObj),iid,ppObj);
}
static HRESULT __stdcall Mine_DDraw7_SetCooperativeLevel(IDirectDraw *dd,HWND hWnd, DWORD dwFlags)
{
  hwOriginal = hWnd;
  return Real_DDraw7_SetCooperativeLevel(dd,hWnd,dwFlags);
}
static HRESULT __stdcall Mine_DDraw7_SetDisplayMode(IDirectDraw *dd,DWORD dwWidth,DWORD dwHeight,DWORD dwBPP,DWORD dwRefreshRate,DWORD dwFlags)
{
  ModifyResolution(&dwWidth,&dwHeight);
  dwOriginalBPP = dwBPP;
  HRESULT hr = Real_DDraw7_SetDisplayMode(dd,dwWidth, dwHeight, dwBPP, dwRefreshRate, dwFlags);
  return hr;
}

static HRESULT __stdcall Mine_DDraw7_CreateSurface(IDirectDraw7 *dd,LPDDSURFACEDESC2 ddsd,LPDIRECTDRAWSURFACE7 *pSurf,IUnknown *pUnkOuter)
{
  HRESULT hr = Real_DDraw7_CreateSurface(dd,(LPDDSURFACEDESC) ddsd,(LPDIRECTDRAWSURFACE *) pSurf,pUnkOuter);
  if(SUCCEEDED(hr))
  {
    PatchDDrawSurface(*pSurf,7);
  }

  return hr;
}

static HRESULT __stdcall Mine_DDrawSurface7_QueryInterface(IUnknown *dd,REFIID iid,LPVOID *ppObj)
{
  return DDrawSurfQueryInterface(Real_DDrawSurface7_QueryInterface(dd,iid,ppObj),iid,ppObj);
}

static HRESULT __stdcall Mine_DDrawSurface7_Flip(IUnknown *me,IUnknown *other,DWORD flags)
{
  return Real_DDrawSurface7_Flip(me,other,flags);
}

static HRESULT __stdcall Mine_DDrawSurface7_Lock(IUnknown *me,LPRECT rect,LPDDSURFACEDESC desc,DWORD flags,HANDLE hnd)
{
  LockAndAllocate(desc);
  return DD_OK;
}

static HRESULT __stdcall Mine_DDrawSurface7_Unlock(IUnknown *me,void *ptr)
{
  return UnlockAndScale(me, Real_DDrawSurface7_Lock, Real_DDrawSurface7_Unlock);
}

// ---- again, common stuff

static void PatchDDrawInterface(IUnknown *dd,int version)
{
  printLog("DirectDraw version: %d\n",version);

#define DDRAW_HOOKS(ver) \
  HookCOMOnce(&Real_DDraw ## ver ## QueryInterface,       dd, 0,  Mine_DDraw ## ver ## QueryInterface); \
  HookCOMOnce(&Real_DDraw ## ver ## CreateSurface,        dd, 6,  (PDDraw_CreateSurface)  Mine_DDraw ## ver ## CreateSurface); \
  HookCOMOnce(&Real_DDraw ## ver ## SetCooperativeLevel,  dd, 20, (PDDraw_SetCooperativeLevel)  Mine_DDraw ## ver ## SetCooperativeLevel); \
  HookCOMOnce(&Real_DDraw ## ver ## SetDisplayMode,       dd, 21, (PDDraw ## ver ## SetDisplayMode) Mine_DDraw ## ver ## SetDisplayMode)

  switch(version)
  {
  case 1: DDRAW_HOOKS(_); break;
  case 2: DDRAW_HOOKS(2_); break;
  case 4: DDRAW_HOOKS(4_); break;
  case 7: DDRAW_HOOKS(7_); break;
  }

#undef DDRAW_HOOKS
}

static void PatchDDrawSurface(IUnknown *dd,int version)
{
  printLog("DirectDraw surface version: %d\n",version);

#define DDRAW_SURFACE_HOOKS(ver) \
  HookCOMOnce(&Real_DDrawSurface ## ver ## QueryInterface,  dd,  0, Mine_DDrawSurface ## ver ## QueryInterface); \
  HookCOMOnce(&Real_DDrawSurface ## ver ## Flip,            dd, 11, Mine_DDrawSurface ## ver ## Flip); \
  HookCOMOnce(&Real_DDrawSurface ## ver ## Lock,            dd, 25, Mine_DDrawSurface ## ver ## Lock); \
  HookCOMOnce(&Real_DDrawSurface ## ver ## Unlock,          dd, 32, Mine_DDrawSurface ## ver ## Unlock)

  switch(version)
  {
  case 1: DDRAW_SURFACE_HOOKS(_); break;
  case 2: DDRAW_SURFACE_HOOKS(2_); break;
  case 3: DDRAW_SURFACE_HOOKS(3_); break;
  case 4: DDRAW_SURFACE_HOOKS(4_); break;
  case 7: DDRAW_SURFACE_HOOKS(7_); break;
  }

#undef DDRAW_SURFACE_HOOKS
}

HRESULT __stdcall Mine_DirectDrawCreate(GUID *lpGUID,LPDIRECTDRAW *lplpDD,IUnknown *pUnkOuter)
{
  HRESULT hr = Real_DirectDrawCreate(lpGUID,lplpDD,pUnkOuter);
  if(SUCCEEDED(hr))
  {
    printLog("video: DirectDrawCreate successful\n");
    PatchDDrawInterface(*lplpDD,1);
  }

  return hr;
}

HRESULT __stdcall Mine_DirectDrawCreateEx(GUID *lpGUID,LPVOID *lplpDD,REFIID iid,IUnknown *pUnkOuter)
{
  HRESULT hr = Real_DirectDrawCreateEx(lpGUID,lplpDD,iid,pUnkOuter);
  if(SUCCEEDED(hr))
  {
    printLog("video: DirectDrawCreateEx successful\n");
    DDrawQueryInterface(hr,iid,lplpDD);
  }

  return hr;
}

void initVideo_DirectDraw()
{
  HMODULE ddraw = LoadLibraryA("ddraw.dll");
  if (!ddraw)
    return;

  Real_DirectDrawCreate = (PDirectDrawCreate)GetProcAddress(ddraw, "DirectDrawCreate");
  if (Real_DirectDrawCreate)
    HookFunction(&Real_DirectDrawCreate,Mine_DirectDrawCreate);

  Real_DirectDrawCreateEx = (PDirectDrawCreateEx)GetProcAddress(ddraw, "DirectDrawCreateEx");
  if (Real_DirectDrawCreateEx)
    HookFunction(&Real_DirectDrawCreateEx,Mine_DirectDrawCreateEx);
}

void deinitVideo_DirectDraw()
{
  if (pTempBuffer) 
  {
    delete[] pTempBuffer;
    pTempBuffer = NULL;
  }
}