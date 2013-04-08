//
// lumi/http/winhttp.c
// the central download routine
// (isolated so it can be reused in platform/msw/stub.c)
//
#include "lumi/app.h"
#include "lumi/ruby.h"
#include "lumi/internal.h"
#include "lumi/config.h"
#include "lumi/version.h"
#include "lumi/http/common.h"
#include "lumi/http/winhttp.h"

void lumi_get_time(LUMI_TIME *ts)
{
  *ts = GetTickCount();
}

unsigned long lumi_diff_time(LUMI_TIME *start, LUMI_TIME *end)
{
  return *end - *start;
}

void
lumi_winhttp_headers(HINTERNET req, lumi_http_handler handler, void *data)
{ 
  DWORD size;
  WinHttpQueryHeaders(req, WINHTTP_QUERY_RAW_HEADERS,
    WINHTTP_HEADER_NAME_BY_INDEX, NULL, &size, WINHTTP_NO_HEADER_INDEX);

  if(GetLastError() == ERROR_INSUFFICIENT_BUFFER)
  {
    int whdrlen = 0, hdrlen = 0;
    LPCWSTR whdr;
    LPSTR hdr = SHOE_ALLOC_N(CHAR, MAX_PATH);
    LPCWSTR hdrs = SHOE_ALLOC_N(WCHAR, size/sizeof(WCHAR));
    BOOL res = WinHttpQueryHeaders(req, WINHTTP_QUERY_RAW_HEADERS,
      WINHTTP_HEADER_NAME_BY_INDEX, (LPVOID)hdrs, &size, WINHTTP_NO_HEADER_INDEX);
    if (res)
    {
      for (whdr = hdrs; whdr - hdrs < size / sizeof(WCHAR); whdr += whdrlen)
      {
        WideCharToMultiByte(CP_UTF8, 0, whdr, -1, hdr, MAX_PATH, NULL, NULL);
        hdrlen = strlen(hdr);
        HTTP_HEADER(hdr, hdrlen, handler, data);
        whdrlen = wcslen(whdr) + 1;
      }
    }
    SHOE_FREE(hdrs);
    SHOE_FREE(hdr);
  }
}

void
lumi_winhttp(LPCWSTR scheme, LPCWSTR host, INTERNET_PORT port, LPCWSTR path, LPCWSTR method,
  LPCWSTR headers, LPVOID body, DWORD bodylen, TCHAR **mem, ULONG memlen, HANDLE file,
  LPDWORD size, UCHAR flags, lumi_http_handler handler, void *data)
{
  LPWSTR proxy;
  DWORD len = 0, rlen = 0, status = 0, complete = 0, flen = 0, total = 0, written = 0;
  LPTSTR buf = SHOE_ALLOC_N(TCHAR, LUMI_BUFSIZE);
  LPTSTR fbuf = SHOE_ALLOC_N(TCHAR, LUMI_CHUNKSIZE);
  LPWSTR uagent = SHOE_ALLOC_N(WCHAR, LUMI_BUFSIZE);
  HINTERNET sess = NULL, conn = NULL, req = NULL;
  LUMI_TIME last = 0;

  if (buf == NULL || fbuf == NULL || uagent == NULL)
    goto done;

  _snwprintf(uagent, LUMI_BUFSIZE, L"Lumi/0.r%d (%S) %S/%d", LUMI_REVISION, LUMI_PLATFORM,
    LUMI_RELEASE_NAME, LUMI_BUILD_DATE);
  sess = WinHttpOpen(uagent, WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
    WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
  if (sess == NULL)
    goto done;

  conn = WinHttpConnect(sess, host, port, 0);
  if (conn == NULL)
    goto done;

  if (method == NULL) method = L"GET";
  req = WinHttpOpenRequest(conn, method, path,
    NULL, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, 0);
  if (req == NULL)
    goto done;

  proxy = _wgetenv(L"http_proxy");
  if (proxy != NULL)
  {
    WINHTTP_PROXY_INFO proxy_info;
    proxy_info.dwAccessType = WINHTTP_ACCESS_TYPE_NAMED_PROXY;
    proxy_info.lpszProxy = proxy;
    proxy_info.lpszProxyBypass = NULL;
    WinHttpSetOption(req, WINHTTP_OPTION_PROXY, &proxy_info, sizeof(proxy_info));
  }

  if (!(flags & LUMI_DL_REDIRECTS))
  {
    DWORD options = WINHTTP_DISABLE_REDIRECTS;
    WinHttpSetOption(req, WINHTTP_OPTION_DISABLE_FEATURE, &options, sizeof(options));
  }

  if (headers != NULL)
    WinHttpAddRequestHeaders(req, headers, -1L, WINHTTP_ADDREQ_FLAG_ADD | WINHTTP_ADDREQ_FLAG_REPLACE);

  if (!WinHttpSendRequest(req, WINHTTP_NO_ADDITIONAL_HEADERS, 0,
    (LPVOID)body, bodylen, bodylen, 0))
    goto done;

  if (!WinHttpReceiveResponse(req, NULL))
    goto done;

  len = sizeof(DWORD);
  if (!WinHttpQueryHeaders(req, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
    NULL, &status, &len, NULL))
    goto done;
  else if (handler != NULL)
  {
    lumi_http_event *event = SHOE_ALLOC(lumi_http_event);
    SHOE_MEMZERO(event, lumi_http_event, 1);
    event->stage = LUMI_HTTP_STATUS;
    event->status = status;
    handler(event, data);
    SHOE_FREE(event);
  }

  if (handler != NULL) lumi_winhttp_headers(req, handler, data);

  *size = 0;
  len = sizeof(buf);
  if (WinHttpQueryHeaders(req, WINHTTP_QUERY_CONTENT_LENGTH | WINHTTP_QUERY_FLAG_NUMBER,
    NULL, size, &len, NULL))
  {
    if (*mem != NULL && *size > memlen)
    {
      SHOE_REALLOC_N(*mem, char, (memlen = *size));
      if (*mem == NULL) goto done;
    }
  }

  HTTP_EVENT(handler, LUMI_HTTP_CONNECTED, last, 0, 0, *size, data, NULL, goto done);

  total = *size * 100;
  rlen = *size;
  while (1)
  {
    len = 0;
    WinHttpReadData(req, fbuf, LUMI_CHUNKSIZE, &len);
    if (len <= 0)
      break;

    if (*mem != NULL)
    {
      if (written + len > memlen)
      {
        while (written + len > memlen)
          memlen += LUMI_BUFSIZE;
        SHOE_REALLOC_N(*mem, char, memlen);
        if (*mem == NULL) goto done;
      }
      SHOE_MEMCPY(*mem + written, fbuf, char, len);
    }
    if (file != INVALID_HANDLE_VALUE)
      WriteFile(file, (LPBYTE)fbuf, len, &flen, NULL);
    written += len;

    if (*size == 0) total = written * 100;
    if (total > 0)
    {
      HTTP_EVENT(handler, LUMI_HTTP_TRANSFER, last, (int)((total - (rlen * 100)) / (total / 100)),
                 (total / 100) - rlen, (total / 100), data, NULL, break);
    }

    if (rlen > len) rlen -= len;
  }

  *size = written;

  if (file != INVALID_HANDLE_VALUE)
  {
    CloseHandle(file);
    file = INVALID_HANDLE_VALUE;
  }

  HTTP_EVENT(handler, LUMI_HTTP_COMPLETED, last, 100, *size, *size, data, *mem, goto done);
  complete = 1;

done:
  if (buf != NULL)    SHOE_FREE(buf);
  if (fbuf != NULL)   SHOE_FREE(fbuf);
  if (uagent != NULL) SHOE_FREE(uagent);

  if (!complete)
  {
    lumi_http_event *event = SHOE_ALLOC(lumi_http_event);
    SHOE_MEMZERO(event, lumi_http_event, 1);
    event->stage = LUMI_HTTP_ERROR;
    event->error = GetLastError();
    int hx = handler(event, data);
    SHOE_FREE(event);
    if (hx & LUMI_DOWNLOAD_HALT) goto done;
  }

  if (file != INVALID_HANDLE_VALUE)
    CloseHandle(file);

  if (req)
    WinHttpCloseHandle(req);

  if (conn)
    WinHttpCloseHandle(conn);

  if (sess)
    WinHttpCloseHandle(sess);
}

