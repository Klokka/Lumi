#include <stdio.h>
#include <stdlib.h>

#define LONG_LONG long long
#define LUMI_TIME DWORD
#define LUMI_DOWNLOAD_ERROR DWORD

#include "lumi/version.h"
#include "lumi/internal.h"
#include "lumi/http/winhttp.h"
#include "stub32.h"

#define BUFSIZE 512

HWND dlg;
BOOL http_abort = FALSE;

int
StubDownloadingLumi(lumi_http_event *event, void *data)
{
  TCHAR msg[512];
  if (http_abort) return LUMI_DOWNLOAD_HALT;
  if (event->stage == LUMI_HTTP_TRANSFER)
  {
    sprintf(msg, "Lumi is downloading. (%d%% done)", event->percent);
    SetDlgItemText(dlg, IDSHOE, msg);
    SendMessage(GetDlgItem(dlg, IDPROG), PBM_SETPOS, event->percent, 0L);
  }
  return LUMI_DOWNLOAD_CONTINUE;
}

void
CenterWindow(HWND hwnd)
{
  RECT rc;
  
  GetWindowRect (hwnd, &rc);
  
  SetWindowPos(hwnd, 0, 
    (GetSystemMetrics(SM_CXSCREEN) - rc.right)/2,
    (GetSystemMetrics(SM_CYSCREEN) - rc.bottom)/2,
     0, 0, SWP_NOZORDER|SWP_NOSIZE );
}

BOOL CALLBACK
stub_win32proc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
  switch (message)
  {
    case WM_INITDIALOG:
      CenterWindow(hwnd);
      return TRUE;

    case WM_COMMAND:
      if (LOWORD(wParam) == IDCANCEL)
      {
        http_abort = TRUE;
        EndDialog(hwnd, LOWORD(wParam));
        return TRUE;
      }
      break;

    case WM_CLOSE:
      http_abort = TRUE;
      EndDialog(hwnd, 0);
      return FALSE;
  }
  return FALSE;
}

void
lumi_silent_install(TCHAR *path)
{
  SHELLEXECUTEINFO shell = {0};
  SetDlgItemText(dlg, IDSHOE, "Setting up Lumi...");
  shell.cbSize = sizeof(SHELLEXECUTEINFO);
  shell.fMask = SEE_MASK_NOCLOSEPROCESS;
  shell.hwnd = NULL;
  shell.lpVerb = NULL;
  shell.lpFile = path;
  shell.lpParameters = "/S"; 
  shell.lpDirectory = NULL;
  shell.nShow = SW_SHOW;
  shell.hInstApp = NULL; 
  ShellExecuteEx(&shell);
  WaitForSingleObject(shell.hProcess,INFINITE);
}

char *setup_exe = "lumi-setup.exe";

DWORD WINAPI
lumi_auto_setup(IN DWORD mid, IN WPARAM w, LPARAM &l, IN LPVOID vinst)
{
  HINSTANCE inst = (HINSTANCE)vinst;
  TCHAR setup_path[BUFSIZE];
  GetTempPath(BUFSIZE, setup_path);
  strncat(setup_path, setup_exe, strlen(setup_exe));

  HANDLE install = CreateFile(setup_path, GENERIC_READ | GENERIC_WRITE,
    FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL);
  HRSRC setupres = FindResource(inst, "LUMI_SETUP", RT_RCDATA);
  DWORD len = 0, rlen = 0;
  LPVOID data = NULL;
  len = SizeofResource(inst, setupres);
  if (GetFileSize(install, NULL) != len)
  {
    HGLOBAL resdata = LoadResource(inst, setupres);
    data = LockResource(resdata);
    SetFilePointer(install, 0, 0, FILE_BEGIN);
    SetEndOfFile(install);
    WriteFile(install, (LPBYTE)data, len, &rlen, NULL);
  }
  CloseHandle(install);
  SendMessage(GetDlgItem(dlg, IDPROG), PBM_SETPOS, 50, 0L);

  lumi_silent_install(setup_path);
  return 0;
}

DWORD WINAPI
lumi_http_thread(IN DWORD mid, IN WPARAM w, LPARAM &l, IN LPVOID data)
{
  DWORD len = 0;
  WCHAR path[BUFSIZE];
  TCHAR *buf = SHOE_ALLOC_N(TCHAR, BUFSIZE);
  TCHAR *empty = NULL;
  HANDLE file;
  TCHAR *nl;
  TCHAR setup_path[BUFSIZE];
  GetTempPath(BUFSIZE, setup_path);
  strncat(setup_path, setup_exe, strlen(setup_exe));

  lumi_winhttp(NULL, L"www.rin-shun.com", 80, L"/pkg/win32/lumi",
    NULL, NULL, NULL, 0, &buf, BUFSIZE,
    INVALID_HANDLE_VALUE, &len, LUMI_DL_DEFAULTS, NULL, NULL);
  if (len == 0)
    return 0;

  nl = strstr(buf, "\n");
  if (nl) nl[0] = '\0';

  len = 0;
  MultiByteToWideChar(CP_ACP, 0, buf, -1, path, BUFSIZE);
  file = CreateFile(setup_path, GENERIC_READ | GENERIC_WRITE,
    FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
  lumi_winhttp(NULL, L"www.rin-shun.com", 80, path,
    NULL, NULL, NULL, 0, &empty, 0, file, &len,
    LUMI_DL_DEFAULTS, HTTP_HANDLER(StubDownloadingLumi), NULL);
  CloseHandle(file);

  lumi_silent_install(setup_path);
  return 0;
}

static BOOL
file_exists(char *fname)
{
  WIN32_FIND_DATA data;
  if (FindFirstFile(fname, &data) != INVALID_HANDLE_VALUE)
    return !(data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY);
  return FALSE;
}

static BOOL
reg_s(HKEY key, char* sub_key, char* val, LPBYTE data, LPDWORD data_len) {
  HKEY hkey;
  BOOL ret = FALSE;
  LONG retv;

  retv = RegOpenKeyEx(key, sub_key, 0, KEY_QUERY_VALUE, &hkey);
  if (retv == ERROR_SUCCESS)
  {
    retv = RegQueryValueEx(hkey, val, NULL, NULL, data, data_len);
    if (retv == ERROR_SUCCESS)
      return TRUE;
  }
  return FALSE;
}

int WINAPI
WinMain(HINSTANCE inst, HINSTANCE inst2, LPSTR arg, int style)
{
  HRSRC nameres, shyres, setupres;
  DWORD len = 0, rlen = 0, tid = 0;
  LPVOID data = NULL;
  TCHAR buf[BUFSIZE], path[BUFSIZE], cmd[BUFSIZE];
  HKEY hkey;
  BOOL lumi;
  DWORD plen;
  HANDLE payload, th;
  MSG msg;
  char *key = "SOFTWARE\\Hackety.org\\Lumi";

  nameres = FindResource(inst, "LUMI_FILENAME", RT_STRING);
  shyres = FindResource(inst, "LUMI_PAYLOAD", RT_RCDATA);
  if (nameres == NULL || shyres == NULL)
  {
    MessageBox(NULL, "This is an empty Lumi stub.", "lumi!! feel yeah!!", MB_OK);
    return 0;
  }

  setupres = FindResource(inst, "LUMI_SETUP", RT_RCDATA);
  plen = sizeof(path);
  if (!(lumi = reg_s((hkey=HKEY_LOCAL_MACHINE), key, "", (LPBYTE)&path, &plen)))
    lumi = reg_s((hkey=HKEY_CURRENT_USER), key, "", (LPBYTE)&path, &plen);

  if (lumi)
  {
    sprintf(cmd, "%s\\lumi.exe", path);
    if (!file_exists(cmd)) lumi = FALSE;
    memset(cmd, 0, BUFSIZE);
  }

  if (!lumi)
  {
    LPTHREAD_START_ROUTINE back_action = (LPTHREAD_START_ROUTINE)lumi_auto_setup;

    INITCOMMONCONTROLSEX InitCtrlEx;
    InitCtrlEx.dwSize = sizeof(INITCOMMONCONTROLSEX);
    InitCtrlEx.dwICC = ICC_PROGRESS_CLASS;
    InitCommonControlsEx(&InitCtrlEx);

    dlg = CreateDialog(inst, MAKEINTRESOURCE(ASKDLG), NULL, stub_win32proc);
    ShowWindow(dlg, SW_SHOW);

    if (setupres == NULL)
      back_action = (LPTHREAD_START_ROUTINE)lumi_http_thread;

    if (!(th = CreateThread(0, 0, back_action, inst, 0, &tid)))
      return 0;

    while (WaitForSingleObject(th, 10) != WAIT_OBJECT_0)   
    {       
        while (PeekMessage(&msg, NULL, NULL, NULL, PM_REMOVE))         
        {            
            TranslateMessage(&msg);           
            DispatchMessage(&msg);         
        }        
    }
    CloseHandle(th);

    if (!(lumi = reg_s((hkey=HKEY_LOCAL_MACHINE), key, "", (LPBYTE)&path, &plen)))
      lumi = reg_s((hkey=HKEY_CURRENT_USER), key, "", (LPBYTE)&path, &plen);
  }

  if (lumi)
  {
    GetTempPath(BUFSIZE, buf);
    data = LoadResource(inst, nameres);
    len = SizeofResource(inst, nameres);
    strncat(buf, (LPTSTR)data, len);

    payload = CreateFile(buf, GENERIC_READ | GENERIC_WRITE,
      FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL);
    len = SizeofResource(inst, shyres);
    if (GetFileSize(payload, NULL) != len)
    {
      HGLOBAL resdata = LoadResource(inst, shyres);
      data = LockResource(resdata);
      SetFilePointer(payload, 0, 0, FILE_BEGIN);
      SetEndOfFile(payload);
      WriteFile(payload, (LPBYTE)data, len, &rlen, NULL);
    }
    CloseHandle(payload);

    sprintf(cmd, "%s\\..\\lumi.bat", path);
    ShellExecute(NULL, "open", cmd, buf, NULL, 0);
  }

  return 0;
}
