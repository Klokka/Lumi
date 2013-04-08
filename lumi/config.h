//
// lumi/config.h
// Most platform-specific definitions.  Window handles and GUI elements for GTK,
// OSX and Win32.
//
// About LUMI_APP_OS and LUMI_SLOT_OS:
// =========================
//
// Okay, so I should mention why these two are split up.  Obviously, these structures
// contain anything which is unique to a platform.  So, on GTK, you'll see a bunch of
// GtkWidget pointers in these two structures.  I try to isolate anything of that nature
// into LUMI_APP_OS or LUMI_SLOT_OS.  Of course, for native widgets, I just go ahead and put it
// in the widget structures, to fit that all in here would be too pedantic.
//
// LUMI_APP_OS covers the toplevel window.  So, there should only be one LUMI_APP_OS structure
// floating around.  In fact, the `global_app` variable in lumi/app.c should always
// point to that one struct.  Still, this struct has a pretty low visibility.  I don't
// like to use `global_app`, except to get around platform limitations.  So the LUMI_APP_OS
// struct is low-viz, it's only touched in the app-level API and in event handlers.
//
// LUMI_SLOT_OS travels down through nested windows, nested canvases.  It's always handy at
// any level in the canvas stack.  But, keep in mind, one is allocated per window or
// canvas.  I guess I think of each drawing layer as a "slot".  Each slot copies its
// parent slot.  So, it's possible that the bottom slot will simply reference pointers
// that are kept in the top slot.  But, in the case of nested fixed canvases (similar
// to a browser's IFRAMEs,) the slot will point to new window handles and pass that
// on to its children.
//
// Anyway, ultimately the idea is to occassionally use the native environment's window
// nesting, because these operating systems offer scrollbars (and some offer compositing)
// which would be wasteful to try to emulate.
//
#ifndef LUMI_CONFIG_H
#define LUMI_CONFIG_H

#define LUMI_BUFSIZE    4096

//
// gtk window struct
//
#ifdef LUMI_GTK
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include <gdk/gdkx.h>
#include <curl/curl.h>
#include <curl/easy.h>

#define LUMI_SIGNAL
#define LUMI_INIT_ARGS void
#define LUMI_EXTERN

typedef struct {
  GtkWidget *vscroll, *canvas;
  GdkEventExpose *expose;
  int scrolly, scrollh, scrollw;
  void *owner;
} lumi_slot_gtk, LUMI_SLOT_OS;

typedef struct {
  GtkWidget *layout;
  GtkWidget *radios;
} lumi_group_gtk, LUMI_GROUP_OS;

typedef struct {
  GtkWidget *window;
} lumi_app_gtk, LUMI_APP_OS;

typedef struct {
  int nada;
} lumi_world_gtk, LUMI_WORLD_OS;

#define USTR(str) str
#define LUMI_CONTROL_REF GtkWidget *
#define LUMI_SURFACE_REF GtkWidget *
#define LUMI_BOOL gboolean
#define LUMI_TIMER_REF guint
#define DC(slot) slot->canvas
#define HAS_DRAWABLE(slot) slot->canvas->window != 0
#define DRAWABLE(ref) GDK_DRAWABLE_XID(ref->window)
#define APP_WINDOW(app) (app == NULL ? NULL : GTK_WINDOW(app->os.window))
#define LUMI_TIME struct timespec
#define LUMI_DOWNLOAD_HEADERS struct curl_slist *
#define LUMI_DOWNLOAD_ERROR CURLcode
#endif

//
// quartz (osx) window struct
//
#ifdef LUMI_QUARTZ
// hacks to prevent T_DATA conflict between Ruby and Carbon headers
# define __OPENTRANSPORT__
# define __OPENTRANSPORTPROTOCOL__
# define __OPENTRANSPORTPROVIDERS__
#include <Cocoa/Cocoa.h>
#include "lumi/native/cocoa.h"
#include <cairo-quartz.h>

#define LUMI_SIGNAL
#define LUMI_HELP_MANUAL 3044
#define LUMI_CONTROL1    3045
#define LUMI_INIT_ARGS void
#define LUMI_EXTERN

typedef struct {
  NSView *view;
  NSScroller *vscroll;
  CGContextRef context;
  cairo_surface_t *surface;
  VALUE controls;
  int scrolly;
  void *owner;
} lumi_slot_quartz, LUMI_SLOT_OS;

typedef struct {
  char none;
} lumi_group_quartz, LUMI_GROUP_OS;

typedef struct {
  LumiWindow *window;
  NSView *view;
  NSRect normal;
} lumi_app_quartz, LUMI_APP_OS;

typedef struct {
  LumiEvents *events;
} lumi_world_quartz, LUMI_WORLD_OS;

#define kLumiViewClassID CFSTR("org.hackety.LumiView")
#define kLumiBoundEvent  'Boun'
#define kLumiSlotData    'SLOT'

#define USTR(str) str
#define LUMI_CONTROL_REF NSControl *
#define LUMI_SURFACE_REF CGrafPtr
#define LUMI_BOOL BOOL
#define LUMI_TIMER_REF LumiTimer *
#define DC(slot) slot->view
#define HAS_DRAWABLE(slot) slot->context != NULL
#define DRAWABLE(ref) ref
#define LUMI_TIME struct timeval
#define LUMI_DOWNLOAD_HEADERS NSDictionary *
#define LUMI_DOWNLOAD_ERROR NSError *

#endif

#ifdef LUMI_WIN32
#include <windows.h>
#include <commctrl.h>
#include <shellapi.h>
#include <cairo-win32.h>

#ifdef RUBY_1_9
#include "ruby/win32.h"
#else
#include "win32/win32.h"
#endif

#ifndef WM_MOUSEWHEEL
#define WM_MOUSEWHEEL 0x020A
#endif
#ifndef WHEEL_DELTA
#define WHEEL_DELTA 120
#endif
#ifndef SPI_GETWHEELSCROLLLINES
#define SPI_GETWHEELSCROLLLINES 104
#endif
#ifndef WHEEL_PAGESCROLL
#define WHEEL_PAGESCROLL UINT_MAX
#endif

#define LUMI_CONTROL1   3045
#define LUMI_WM_MESSAGE (WM_APP + 3045)
#define LUMI_INIT_ARGS  HINSTANCE inst, int style
#define LUMI_EXTERN __declspec(dllexport)

typedef struct {
  PAINTSTRUCT ps;
  HDC dc, dc2;
  HWND window;
  VALUE focus;
  VALUE controls;
  cairo_surface_t *surface;
  int scrolly;
  char vscroll;
  void *owner;
  void *parent;
} lumi_slot_win32, LUMI_SLOT_OS;

typedef struct {
  char none;
} lumi_group_win32, LUMI_GROUP_OS;

typedef struct {
  LONG style;
  RECT normal;
  BOOL ctrlkey, altkey, shiftkey;
  HWND window;
} lumi_app_win32, LUMI_APP_OS;

typedef struct {
  HINSTANCE instance;
  int style;
  HWND hidden;
  WNDCLASSEX classex, slotex, vlclassex, hiddenex;
  ATOM classatom;
  BOOL doublebuffer;
} lumi_world_win32, LUMI_WORLD_OS;

#define USTR(str) (const char *)L##str
#define LUMI_CONTROL_REF HWND
#define LUMI_SURFACE_REF HWND
#define LUMI_BOOL BOOL
#define LUMI_TIMER_REF long
#define DC(slot) slot->window
#define HAS_DRAWABLE(slot) slot->window != NULL
#define DRAWABLE(ref) (libvlc_drawable_t)ref
#define APP_WINDOW(app) (app == NULL ? NULL : app->slot->window)
#define LUMI_TIME DWORD
#define LUMI_DOWNLOAD_HEADERS LPWSTR
#define LUMI_DOWNLOAD_ERROR DWORD

#endif

#define KEY_STATE(sym) \
  { \
    VALUE str = rb_str_new2("" # sym "_"); \
    rb_str_append(str, rb_funcall(v, s_to_s, 0)); \
    v = rb_str_intern(str); \
  }

#endif
