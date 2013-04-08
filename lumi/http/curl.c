//
// lumi/http/curl.c
// the downloader routines using libcurl.
//
#include "lumi/app.h"
#include "lumi/ruby.h"
#include "lumi/config.h"
#include "lumi/http.h"
#include "lumi/version.h"
#include "lumi/internal.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

typedef struct {
  char *mem;
  unsigned long memlen;
  FILE *fp;
  char *body;
  size_t size, total, readpos, bodylen;
  unsigned long status;
  lumi_http_handler handler;
  LUMI_TIME last;
  CURL *curl;
  void *data;
} lumi_curl_data;

const char *content_len_str = "Content-Length: ";

size_t
lumi_curl_header_funk(char *ptr, size_t size, size_t nmemb, void *user)
{
  lumi_curl_data *data = (lumi_curl_data *)user;
  size_t realsize = size * nmemb;
  if (data->status == 0 && data->handler != NULL)
  {
    curl_easy_getinfo(data->curl, CURLINFO_RESPONSE_CODE, (long *)&data->status);
    if (data->handler != NULL)
    {
      lumi_http_event *event = SHOE_ALLOC(lumi_http_event);
      SHOE_MEMZERO(event, lumi_http_event, 1);
      event->stage = LUMI_HTTP_STATUS;
      event->status = data->status;
      data->handler(event, data->data);
      SHOE_FREE(event);
    }
  }

  HTTP_HEADER(ptr, realsize, data->handler, data->data);
  if ((data->mem != NULL || data->fp != NULL) &&
      strncmp(ptr, content_len_str, strlen(content_len_str)) == 0)
  {
    data->total = strtoull(ptr + strlen(content_len_str), NULL, 10);
    if (data->mem != NULL && data->total > data->memlen)
    {
      data->memlen = data->total;
      SHOE_REALLOC_N(data->mem, char, data->memlen);
      if (data->mem == NULL) return -1;
    }
  }
  return realsize;
}

size_t
lumi_curl_read_funk(void *ptr, size_t size, size_t nmemb, void *user)
{
  lumi_curl_data *data = (lumi_curl_data *)user;
  size_t realsize = size * nmemb;
  if (realsize > data->bodylen - data->readpos)
    realsize = data->bodylen - data->readpos;
  SHOE_MEMCPY(ptr, &(data->body[data->readpos]), char, realsize);
  data->readpos += realsize;
  return realsize;
}

size_t
lumi_curl_write_funk(void *ptr, size_t size, size_t nmemb, void *user)
{
  lumi_curl_data *data = (lumi_curl_data *)user;
  size_t realsize = size * nmemb;
  if (data->size == 0)
  {
    HTTP_EVENT(data->handler, LUMI_HTTP_CONNECTED, data->last, 0, 0, data->total, data->data, NULL, return -1);
  }
  if (data->mem != NULL)
  {
    if (data->size + realsize > data->memlen)
    {
      while (data->size + realsize > data->memlen)
        data->memlen += LUMI_BUFSIZE;
      SHOE_REALLOC_N(data->mem, char, data->memlen);
      if (data->mem == NULL) return -1;
    }
    SHOE_MEMCPY(&(data->mem[data->size]), ptr, char, realsize);
  }
  if (data->fp != NULL)
    realsize = fwrite(ptr, size, nmemb, data->fp) * size;
  if (realsize > 0)
    data->size += realsize;
  return realsize;
}

int
lumi_curl_progress_funk(void *user, double dltotal, double dlnow, double ultotal, double ulnow)
{
  lumi_curl_data *data = (lumi_curl_data *)user;
  if (dltotal > 0.)
  {
    HTTP_EVENT(data->handler, LUMI_HTTP_TRANSFER, data->last, dlnow * 100.0 / dltotal, dlnow,
               dltotal, data->data, NULL, return 1);
  }
  return 0;
}

void
lumi_download(lumi_http_request *req)
{
  char uagent[LUMI_BUFSIZE];
  CURL *curl = curl_easy_init();
  CURLcode res;
  lumi_curl_data *cdata = SHOE_ALLOC(lumi_curl_data);
  if (curl == NULL) return;

  sprintf(uagent, "Lumi/0.r%d (%s) %s/%d", LUMI_REVISION, LUMI_PLATFORM,
    LUMI_RELEASE_NAME, LUMI_BUILD_DATE);

  cdata->mem = req->mem;
  cdata->memlen = req->memlen;
  cdata->fp = NULL;
  cdata->size = cdata->readpos = req->size = 0;
  cdata->total = 0;
  cdata->handler = req->handler;
  cdata->data = req->data;
  cdata->last.tv_sec = 0;
  cdata->last.tv_nsec = 0;
  cdata->status = 0;
  cdata->curl = curl;
  cdata->body = NULL;

  if (req->mem == NULL && req->filepath != NULL)
  {
    cdata->fp = fopen(req->filepath, "wb");
    if (cdata->fp == NULL) goto done;
  } 

  curl_easy_setopt(curl, CURLOPT_NOSIGNAL, TRUE);
  curl_easy_setopt(curl, CURLOPT_URL, req->url);
  curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, lumi_curl_header_funk);
  curl_easy_setopt(curl, CURLOPT_WRITEHEADER, cdata);
  if (cdata->mem != NULL || cdata->fp != NULL)
  {
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, lumi_curl_write_funk);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, cdata);
    curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);
    curl_easy_setopt(curl, CURLOPT_PROGRESSFUNCTION, lumi_curl_progress_funk);
    curl_easy_setopt(curl, CURLOPT_PROGRESSDATA, cdata);
  }
  else
  {
    curl_easy_setopt(curl, CURLOPT_NOBODY, 1L);
  }
  curl_easy_setopt(curl, CURLOPT_USERAGENT, uagent);
  if (req->flags & LUMI_DL_REDIRECTS)
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1);
  if (req->method)
    curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, req->method);
  if (req->headers)
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, req->headers);
  if (req->body)
  {
    cdata->body = req->body;
    cdata->bodylen = req->bodylen;
    curl_easy_setopt(curl, CURLOPT_UPLOAD, 1);
    curl_easy_setopt(curl, CURLOPT_INFILE, cdata);
    curl_easy_setopt(curl, CURLOPT_INFILESIZE, cdata->bodylen);
    curl_easy_setopt(curl, CURLOPT_READFUNCTION, lumi_curl_read_funk);
  }

  res = curl_easy_perform(curl);
  req->size = cdata->size;
  req->mem = cdata->mem;

  if (res != CURLE_OK)
  {
    if (req->handler != NULL)
    {
      lumi_http_event *event = SHOE_ALLOC(lumi_http_event);
      SHOE_MEMZERO(event, lumi_http_event, 1);
      event->stage = LUMI_HTTP_ERROR;
      event->error = res;
      req->handler(event, req->data);
      SHOE_FREE(event);
    }
    goto done;
  }

  if (cdata->fp != NULL)
  { 
    fclose(cdata->fp);
    cdata->fp = NULL;
  }

  HTTP_EVENT(cdata->handler, LUMI_HTTP_COMPLETED, cdata->last, 100, req->size, req->size, cdata->data, req->mem, goto done);

done:
  if (cdata->fp != NULL)
    fclose(cdata->fp);
  if (cdata != NULL)
    SHOE_FREE(cdata);

  curl_easy_cleanup(curl);
}

void *
lumi_download2(void *data)
{
  lumi_http_request *req = (lumi_http_request *)data;
  lumi_download(req);
  lumi_http_request_free(req);
  free(req);
  return NULL;
}

void
lumi_queue_download(lumi_http_request *req)
{
  pthread_t tid;
  pthread_create(&tid, NULL, lumi_download2, req);
}

VALUE
lumi_http_err(LUMI_DOWNLOAD_ERROR code)
{
  return rb_str_new2(curl_easy_strerror(code));
}

LUMI_DOWNLOAD_HEADERS
lumi_http_headers(VALUE hsh)
{
  long i;
  struct curl_slist *slist = NULL;
  VALUE keys = rb_funcall(hsh, s_keys, 0);
  for (i = 0; i < RARRAY_LEN(keys); i++ )
  {
    VALUE key = rb_ary_entry(keys, i);
    VALUE header = rb_str_dup(key);
    rb_str_cat2(header, ": ");
    rb_str_append(header, rb_hash_aref(hsh, key));
    slist = curl_slist_append(slist, RSTRING_PTR(header));
  }
  return slist;
}

void
lumi_http_headers_free(struct curl_slist *slist)
{
  curl_slist_free_all(slist);
}
