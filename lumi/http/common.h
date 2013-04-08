//
// lumi/http/common.h
// cross-platform structures common to Lumi and its stubs
//
#ifndef LUMI_HTTP_COMMON_H
#define LUMI_HTTP_COMMON_H

#define LUMI_CHUNKSIZE 16384

#define LUMI_DL_REDIRECTS 1
#define LUMI_DL_DEFAULTS  (LUMI_DL_REDIRECTS)

#define LUMI_DOWNLOAD_CONTINUE 0
#define LUMI_DOWNLOAD_HALT 1

#define LUMI_HTTP_STATUS    3
#define LUMI_HTTP_HEADER    4
#define LUMI_HTTP_CONNECTED 5
#define LUMI_HTTP_TRANSFER  10
#define LUMI_HTTP_COMPLETED 15
#define LUMI_HTTP_ERROR     20

#define HTTP_HEADER(ptr, realsize, handler, data) \
  { \
    char *colon, *val, *end; \
    for (colon = ptr; colon < ptr + realsize; colon++) \
      if (colon[0] == ':') \
        break; \
    for (val = colon + 1; val < ptr + realsize; val++) \
      if (val[0] != ' ') \
        break; \
    for (end = (ptr + realsize) - 1; end > ptr; end--) \
      if (end[0] != '\r' && end[0] != '\n' && end[0] != ' ') \
        break; \
    if (colon < ptr + realsize) \
    { \
      lumi_http_event *event = SHOE_ALLOC(lumi_http_event); \
      event->stage = LUMI_HTTP_HEADER; \
      event->hkey = ptr; \
      event->hkeylen = colon - ptr; \
      event->hval = val; \
      event->hvallen = (end - val) + 1; \
      if (handler != NULL) handler(event, data); \
      SHOE_FREE(event); \
    } \
  }

#define HTTP_EVENT(handler, s, last, perc, trans, tot, dat, bd, abort) \
{ LUMI_TIME ts; \
  lumi_get_time(&ts); \
  unsigned long elapsed = lumi_diff_time(&(last), &ts); \
  if (s != LUMI_HTTP_TRANSFER || elapsed > 600 ) { \
    lumi_http_event *event = SHOE_ALLOC(lumi_http_event); \
    event->stage = s; \
    if (s == LUMI_HTTP_COMPLETED) event->stage = LUMI_HTTP_TRANSFER; \
    event->percent = perc; \
    event->transferred = trans;\
    event->total = tot; \
    last = ts; \
    if (handler != NULL && (handler(event, dat) & LUMI_DOWNLOAD_HALT)) \
    { SHOE_FREE(event); event = NULL; abort; } \
    if (event != NULL && s == LUMI_HTTP_COMPLETED) { event->stage = s; \
      event->body = bd; \
      if (handler != NULL && (handler(event, dat) & LUMI_DOWNLOAD_HALT)) \
      { SHOE_FREE(event); event = NULL; abort; } \
    } \
    if (event != NULL) SHOE_FREE(event); \
  } }

typedef struct {
  unsigned char stage;
  unsigned long status;
  const char *hkey, *hval, *body;
  unsigned long hkeylen, hvallen, bodylen;
  unsigned LONG_LONG total;
  unsigned LONG_LONG transferred;
  unsigned long percent;
  LUMI_DOWNLOAD_ERROR error;
} lumi_http_event;

typedef int (*lumi_http_handler)(lumi_http_event *, void *);

void lumi_get_time(LUMI_TIME *);
unsigned long lumi_diff_time(LUMI_TIME *, LUMI_TIME *);

#endif
