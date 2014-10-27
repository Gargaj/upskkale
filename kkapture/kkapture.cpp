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
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <tchar.h>
#include "../kkapturedll/main.h"
#include "../kkapturedll/intercept.h"
#include "resource.h"

#pragma comment(lib,"vfw32.lib")
#pragma comment(lib,"msacm32.lib")

#define COUNTOF(x) (sizeof(x)/sizeof(*x))

static const TCHAR RegistryKeyName[] = _T("Software\\Farbrausch\\kkapture");
static const int MAX_ARGS = _MAX_PATH*2;

// global vars for parameter passing (yes this sucks...)
static TCHAR ExeName[_MAX_PATH];
static TCHAR Arguments[MAX_ARGS];
static ParameterBlock Params;

// ---- some dialog helpers

static BOOL ErrorMsg(const TCHAR *msg,HWND hWnd=0)
{
  MessageBox(hWnd,msg,_T(".kkapture"),MB_ICONERROR|MB_OK);
  return FALSE; // so you can conveniently "return ErrorMsg(...)"
}

static BOOL EnableDlgItem(HWND hWnd,int id,BOOL bEnable)
{
  HWND hCtrlWnd = GetDlgItem(hWnd,id);
  return EnableWindow(hCtrlWnd,bEnable);
}

static LRESULT CALLBACK EditBoxSubProc(HWND hWnd,UINT msg,WPARAM wParam,LPARAM lParam)
{
  TCHAR buffer[_MAX_PATH];
  WNDPROC chain = (WNDPROC) GetWindowLongPtr(hWnd,GWLP_USERDATA);
  HDROP drop;

  switch (msg)
  {
  case WM_DROPFILES:
    drop = (HDROP) wParam;
    if(DragQueryFile(drop,0,buffer,COUNTOF(buffer)))
    {
      SetWindowTextA(hWnd,buffer);
    }
    DragFinish(drop);
    return 0;
  }

  return CallWindowProc(chain,hWnd,msg,wParam,lParam);
}

static void EditBoxEnableDragDrop(HWND hWnd)
{
  LONG_PTR oldproc = SetWindowLongPtr(hWnd,GWLP_WNDPROC,(LONG_PTR) EditBoxSubProc);
  SetWindowLongPtr(hWnd,GWLP_USERDATA,oldproc);
  DragAcceptFiles(hWnd,TRUE);
}

static INT_PTR CALLBACK MainDialogProc(HWND hWndDlg,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
  switch(uMsg)
  {
  case WM_INITDIALOG:
    {
      // center this window
      RECT rcWork,rcDlg;
      SystemParametersInfo(SPI_GETWORKAREA,0,&rcWork,0);
      GetWindowRect(hWndDlg,&rcDlg);
      SetWindowPos(hWndDlg,0,(rcWork.left+rcWork.right-rcDlg.right+rcDlg.left)/2,
        (rcWork.top+rcWork.bottom-rcDlg.bottom+rcDlg.top)/2,-1,-1,SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
    }
    return TRUE;

  case WM_COMMAND:
    switch(LOWORD(wParam))
    {
    case IDCANCEL:
      EndDialog(hWndDlg,0);
      return TRUE;

    case IDOK:
      {
        Params.VersionTag = PARAMVERSION;

        GetDlgItemText(hWndDlg,IDC_DEMO,ExeName,_MAX_PATH);
        GetDlgItemText(hWndDlg,IDC_ARGUMENTS,Arguments,MAX_ARGS);

        // validate everything and fill out parameter block
        HANDLE hFile = CreateFile(ExeName,GENERIC_READ,0,0,OPEN_EXISTING,0,0);
        if(hFile == INVALID_HANDLE_VALUE)
          return !ErrorMsg(_T("You need to specify a valid executable in the 'demo' field."),hWndDlg);
        else
          CloseHandle(hFile);

        // that's it.
        EndDialog(hWndDlg,1);
      }
      return TRUE;

    case IDC_BDEMO:
      {
        OPENFILENAME ofn;
        TCHAR filename[_MAX_PATH];

        GetDlgItemText(hWndDlg,IDC_DEMO,filename,_MAX_PATH);

        ZeroMemory(&ofn,sizeof(ofn));
        ofn.lStructSize   = sizeof(ofn);
        ofn.hwndOwner     = hWndDlg;
        ofn.hInstance     = GetModuleHandle(0);
        ofn.lpstrFilter   = _T("Executable files (*.exe)\0*.exe\0");
        ofn.nFilterIndex  = 1;
        ofn.lpstrFile     = filename;
        ofn.nMaxFile      = sizeof(filename)/sizeof(*filename);
        ofn.Flags         = OFN_FILEMUSTEXIST|OFN_HIDEREADONLY;
        if(GetOpenFileName(&ofn))
        {
          SetDlgItemText(hWndDlg,IDC_DEMO,ofn.lpstrFile);
        }
      }
      return TRUE;

    case IDC_BTARGET:
      {
        OPENFILENAME ofn;
        TCHAR filename[_MAX_PATH];

        GetDlgItemText(hWndDlg,IDC_TARGET,filename,_MAX_PATH);

        ZeroMemory(&ofn,sizeof(ofn));
        ofn.lStructSize   = sizeof(ofn);
        ofn.hwndOwner     = hWndDlg;
        ofn.hInstance     = GetModuleHandle(0);
        ofn.lpstrFilter   = _T("AVI files (*.avi)\0*.avi\0");
        ofn.nFilterIndex  = 1;
        ofn.lpstrFile     = filename;
        ofn.nMaxFile      = sizeof(filename)/sizeof(*filename);
        ofn.Flags         = OFN_HIDEREADONLY|OFN_PATHMUSTEXIST|OFN_OVERWRITEPROMPT;
        if(GetSaveFileName(&ofn))
          SetDlgItemText(hWndDlg,IDC_TARGET,ofn.lpstrFile);
      }
      return TRUE;

    case IDC_ENCODER:
      if(HIWORD(wParam) == CBN_SELCHANGE)
      {
        LRESULT selection = SendDlgItemMessage(hWndDlg,IDC_ENCODER,CB_GETCURSEL,0,0);
        BOOL allowCodecSelect = selection != 0;

        EnableDlgItem(hWndDlg,IDC_VIDEOCODEC,allowCodecSelect);
        EnableDlgItem(hWndDlg,IDC_VCPICK,allowCodecSelect);
      }
      return TRUE;
    }
    break;
  }

  return FALSE;
}

// ----

int WINAPI WinMain(HINSTANCE hInstance,HINSTANCE hPrevInstance,LPSTR lpCmdLine,int nCmdShow)
{
  if(DialogBox(hInstance,MAKEINTRESOURCE(IDD_MAINWIN),0,MainDialogProc))
  {
    Params.IsDebugged = IsDebuggerPresent() != 0;

    // create file mapping object with parameter block
    HANDLE hParamMapping = CreateFileMapping(INVALID_HANDLE_VALUE,0,PAGE_READWRITE,
      0,sizeof(ParameterBlock),_T("__kkapture_parameter_block"));

    // copy parameters
    LPVOID ParamBlock = MapViewOfFile(hParamMapping,FILE_MAP_WRITE,0,0,sizeof(ParameterBlock));
    if(ParamBlock)
    {
      memcpy(ParamBlock,&Params,sizeof(ParameterBlock));
      UnmapViewOfFile(ParamBlock);
    }

    // prepare command line
    TCHAR commandLine[_MAX_PATH+MAX_ARGS+4];
    _tcscpy(commandLine,_T("\""));
    _tcscat(commandLine,ExeName);
    _tcscat(commandLine,_T("\" "));
    _tcscat(commandLine,Arguments);

    // create process
	  STARTUPINFOA si;
	  PROCESS_INFORMATION pi;

    ZeroMemory(&si,sizeof(si));
    ZeroMemory(&pi,sizeof(pi));
    si.cb = sizeof(si);

    // change to directory that contains executable
    TCHAR exepath[_MAX_PATH];
    TCHAR drive[_MAX_DRIVE],dir[_MAX_DIR],fname[_MAX_FNAME],ext[_MAX_EXT];

    _tsplitpath(ExeName,drive,dir,fname,ext);
    _tmakepath(exepath,drive,dir,_T(""),_T(""));
    SetCurrentDirectory(exepath);

    // actually start the target
    int err = CreateInstrumentedProcessA(ExeName,commandLine,0,0,TRUE,
      CREATE_DEFAULT_ERROR_MODE,0,0,&si,&pi);
    switch(err)
    {
    case ERR_OK:
      // wait for target process to finish
      WaitForSingleObject(pi.hProcess,INFINITE);
      break;

    case ERR_INSTRUMENTATION_FAILED:
      ErrorMsg(_T("Startup instrumentation failed"));
      break;

    case ERR_COULDNT_EXECUTE:
      ErrorMsg(_T("Couldn't execute target process"));
      break;
    }

    // cleanup
    CloseHandle(pi.hProcess);
    CloseHandle(hParamMapping);
  }

  return 0;
}