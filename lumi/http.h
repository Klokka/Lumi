//
// lumi/http.h
// the lumi downloader, which uses platform code and threads
// to achieve much faster and brain-dead http access.
//
#ifndef LUMI_HTTP_H
#define LUMI_HTTP_H

#include "lumi/http/common.h"

typedef struct {
  char *url;
  char *scheme;
  char *host;
  int port;
  char *path;

  char *method, *body;
  unsigned long bodylen;
  LUMI_DOWNLOAD_HEADERS headers;

  char *mem;
  unsigned long memlen;
  char *filepath;
  unsigned LONG_LONG size;
  lumi_http_handler handler;
  void *data;
  unsigned char flags;
} lumi_http_request;

void lumi_download(lumi_http_request *req);
void lumi_queue_download(lumi_http_request *req);
VALUE lumi_http_err(LUMI_DOWNLOAD_ERROR error);
LUMI_DOWNLOAD_HEADERS lumi_http_headers(VALUE hsh);
void lumi_http_request_free(lumi_http_request *);
void lumi_http_headers_free(LUMI_DOWNLOAD_HEADERS);

#ifdef LUMI_WIN32
#include "lumi/http/winhttp.h"
#else
#define HTTP_HANDLER(x)
#endif

#endif
