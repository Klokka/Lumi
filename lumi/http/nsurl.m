//
// lumi/http/nsurl.m
// the downloader routines using nsurl classes.
//
#include "lumi/app.h"
#include "lumi/ruby.h"
#include "lumi/internal.h"
#include "lumi/config.h"
#include "lumi/http.h"
#include "lumi/version.h"
#include "lumi/world.h"
#include "lumi/native.h"

#import <Cocoa/Cocoa.h>

@interface LumiHttp : NSObject
{
  char *dest;
  unsigned long destlen;
  NSURLConnection *conn;
  NSURLDownload *down;
  NSURLResponse *resp;
  NSFileManager *fm;
  long long size;
  long long total;
  unsigned char flags;
  lumi_http_handler handler;
  LUMI_TIME last;
  NSMutableData *bytes;
  void *data;
}
@end

@implementation LumiHttp
- (id)init
{
  if ((self = [super init]))
  {
    bytes = [[NSMutableData data] retain];
    fm = [NSFileManager defaultManager];
    destlen = 0;
  }
  return self;
}
- (void)releaseData
{
  if (dest != NULL) free(dest);
  if (data != NULL) free(data);
}
- (void)download: (lumi_http_request *)req
{
  NSString *url = [NSString stringWithFormat: @"%s", req->url];
  NSString *uagent = [NSString stringWithFormat: @"Lumi/0.r%d (%s) %s/%d", 
    LUMI_REVISION, LUMI_PLATFORM, LUMI_RELEASE_NAME, LUMI_BUILD_DATE];
  NSMutableURLRequest *nsreq = [NSMutableURLRequest requestWithURL: 
    [NSURL URLWithString: url]
    cachePolicy: NSURLRequestUseProtocolCachePolicy
    timeoutInterval: 60.0];
  [nsreq setValue: uagent forHTTPHeaderField: @"User-Agent"];
  [nsreq setHTTPShouldHandleCookies: FALSE];

  flags = req->flags;
  handler = req->handler;
  data = req->data;
  if (req->headers != NULL)
    [nsreq setAllHTTPHeaderFields: req->headers];
  if (req->body != NULL && req->bodylen > 0)
    [nsreq setHTTPBody: [NSData dataWithBytes: req->body length: req->bodylen]];
  if (req->method != NULL)
    [nsreq setHTTPMethod: [NSString stringWithUTF8String: req->method]];
  last.tv_sec = 0;
  last.tv_usec = 0;
  size = total = 0;
  if (req->filepath != NULL)
  {
    dest = req->filepath;
    down = [[NSURLDownload alloc] initWithRequest: nsreq delegate: self];
    req->filepath = NULL;
  }
  else
  {
    dest = req->mem;
    destlen = req->memlen;
    conn = [[NSURLConnection alloc] initWithRequest: nsreq
      delegate: self];
    req->mem = NULL;
  }
}
- (void)readHeaders: (NSURLResponse *)response
{
  if ([response respondsToSelector:@selector(allHeaderFields)] && handler != NULL)
  {
    lumi_http_event *event = SHOE_ALLOC(lumi_http_event);
    NSHTTPURLResponse* httpresp = (NSHTTPURLResponse *)response;
    if ([httpresp statusCode])
    {
      SHOE_MEMZERO(event, lumi_http_event, 1);
      event->stage = LUMI_HTTP_STATUS;
      event->status = [httpresp statusCode];
      handler(event, data);
    }

    NSDictionary *hdrs = [httpresp allHeaderFields];
    if (hdrs)
    {
      NSString *key;
      NSEnumerator *keys = [hdrs keyEnumerator];
      while (key = [keys nextObject])
      {
        NSString *val = [hdrs objectForKey: key];
        SHOE_MEMZERO(event, lumi_http_event, 1);
        event->stage = LUMI_HTTP_HEADER;
        event->hkey = [key UTF8String];
        event->hkeylen = strlen(event->hkey);
        event->hval = [val UTF8String];
        event->hvallen = strlen(event->hval);
        handler(event, data);
      }
    }

    SHOE_FREE(event);
  }
}
- (void)handleError: (NSError *)error
{
  if (handler != NULL)
  {
    lumi_http_event *event = SHOE_ALLOC(lumi_http_event);
    SHOE_MEMZERO(event, lumi_http_event, 1);
    event->stage = LUMI_HTTP_ERROR;
    event->error = error;
    handler(event, data);
    SHOE_FREE(event);
  }
}
- (NSURLRequest *)connection: (NSURLConnection *)connection
  willSendRequest: (NSURLRequest *)request
  redirectResponse: (NSURLResponse *)redirectResponse
{
  NSURLRequest *newRequest = request;
  if (redirectResponse && !(flags & LUMI_DL_REDIRECTS))
    newRequest = nil;
  return newRequest;
}
- (void)connection: (NSURLConnection *)c didReceiveResponse: (NSURLResponse *)response
{
  [self readHeaders: response];
  size = total = 0;
  if ([response expectedContentLength] != NSURLResponseUnknownLength)
    total = [response expectedContentLength];
  HTTP_EVENT(handler, LUMI_HTTP_CONNECTED, last, 0, 0, total, data, NULL, [c cancel]);
  [bytes setLength: 0];
}
- (void)connection: (NSURLConnection *)c didFailWithError:(NSError *)error
{
  [c release];
  [self handleError: error];
}
- (void)connection: (NSURLConnection *)c didReceiveData: (NSData *)chunk
{
  [bytes appendData: chunk];
  size += [chunk length];
  HTTP_EVENT(handler, LUMI_HTTP_TRANSFER, last, size * 100.0 / total, size,
             total, data, NULL, [c cancel]);
}
- (void)connectionDidFinishLoading: (NSURLConnection *)c
{
  total = [bytes length];
  if (dest != NULL)
  {
    if ([bytes length] > destlen)
      SHOE_REALLOC_N(dest, char, [bytes length]);
    [bytes getBytes: dest];
  }
  HTTP_EVENT(handler, LUMI_HTTP_COMPLETED, last, 100, total, total, data, [bytes mutableBytes], 1);
  [c release];
  [self releaseData];
}
- (NSURLRequest *)download: (NSURLDownload *)download
  willSendRequest: (NSURLRequest *)request
  redirectResponse: (NSURLResponse *)redirectResponse
{
  NSURLRequest *newRequest = request;
  if (redirectResponse && !(flags & LUMI_DL_REDIRECTS))
    newRequest = nil;
  return newRequest;
}
- (void)download: (NSURLDownload *)download decideDestinationWithSuggestedFilename: (NSString *)filename
{
  NSString *path = [NSString stringWithUTF8String: dest];
  [download setDestination: path allowOverwrite: YES];
}
- (void)setDownloadResponse: (NSURLResponse *)aDownloadResponse
{
  [aDownloadResponse retain];
  [resp release];
  resp = aDownloadResponse;
}
- (void)download: (NSURLDownload *)download didReceiveResponse: (NSURLResponse *)response
{
  [self readHeaders: response];
  size = total = 0;
  if ([response expectedContentLength] != NSURLResponseUnknownLength)
    total = [response expectedContentLength];
  HTTP_EVENT(handler, LUMI_HTTP_CONNECTED, last, 0, 0, total, data, NULL, [download cancel]);
  [self setDownloadResponse: response];
}
- (void)download: (NSURLDownload *)download didFailWithError:(NSError *)error
{
  [download release];
  [self handleError: error];
}
- (void)download: (NSURLDownload *)download didReceiveDataOfLength: (unsigned)length
{
  size += length;
  HTTP_EVENT(handler, LUMI_HTTP_TRANSFER, last, size * 100.0 / total, size,
             total, data, NULL, [download cancel]);
}
- (void)downloadDidFinish: (NSURLDownload *)download
{
  HTTP_EVENT(handler, LUMI_HTTP_COMPLETED, last, 100, total, total, data, NULL, 1);
  [download release];
  [self releaseData];
}
@end

void
lumi_download(lumi_http_request *req)
{
  LumiHttp *http = [[LumiHttp alloc] init];
  [http download: req];
  lumi_http_request_free(req);
  free(req);
}

void
lumi_queue_download(lumi_http_request *req)
{
  lumi_download(req);
}

VALUE
lumi_http_err(LUMI_DOWNLOAD_ERROR code)
{
  const char *errorString = [[code localizedDescription] UTF8String];
  return rb_str_new2(errorString);
}

LUMI_DOWNLOAD_HEADERS
lumi_http_headers(VALUE hsh)
{
  long i;
  NSDictionary *d = NULL;
  VALUE keys = rb_funcall(hsh, s_keys, 0);
  if (RARRAY_LEN(keys) > 0)
  {
    d = [[NSMutableDictionary dictionaryWithCapacity: RARRAY_LEN(keys)] retain];
    for (i = 0; i < RARRAY_LEN(keys); i++)
    {
      VALUE key = rb_ary_entry(keys, i);
      VALUE val = rb_hash_aref(hsh, key);
      if(!NIL_P(val))
        [d setValue: [NSString stringWithUTF8String: RSTRING_PTR(val)]
           forKey:   [NSString stringWithUTF8String: RSTRING_PTR(key)]];
    }
  }
  return d;
}

void
lumi_http_headers_free(LUMI_DOWNLOAD_HEADERS headers)
{
  [headers release];
}
