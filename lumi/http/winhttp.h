s//
// lumi/http/winhttp.h
// the lumi_winhttp function, used by platform/msw/stub.c.
//
#ifndef LUMI_HTTP_WINHTTP_H
#define LUMI_HTTP_WINHTTP_H

#include <stdio.h>
#include <windows.h>
#include <wchar.h>
#include <winhttp.h>
#include <shellapi.h>
#include "lumi/http/common.h"

#define HTTP_HANDLER(x) reinterpret_cast<lumi_http_handler>(x)

void lumi_winhttp(LPCWSTR, LPCWSTR, INTERNET_PORT, LPCWSTR, LPCWSTR, LPCWSTR, LPVOID, DWORD, TCHAR **, ULONG, HANDLE, LPDWORD, UCHAR, lumi_http_handler, void *);

#endif
