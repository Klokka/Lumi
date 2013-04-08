//
// lumi/app.h
// Abstract windowing for GTK, Quartz (OSX) and Win32.
//
// This is really just a light wrapper around Cairo, which does most of the
// work anyway.  I'm not sure why they don't do this for ya.  Probably so I
// could do it in Lumi!!
//

#ifndef LUMI_APP_H
#define LUMI_APP_H

#include <cairo.h>
#include <ruby.h>

#include "lumi/canvas.h"
#include "lumi/code.h"
#include "lumi/config.h"

#define LUMI_APP_HEIGHT 500
#define LUMI_APP_WIDTH  600
#define LUMI_SHORTNAME  "lumi"
#define LUMI_APPNAME    "Lumi"
#define LUMI_VLCLASS    "Lumi VLC"
#define LUMI_SLOTCLASS  "Lumi Slot"
#define LUMI_HIDDENCLS  "Lumi Hidden"

//
// abstract window struct
//
typedef struct _lumi_app {
  LUMI_APP_OS os;
  LUMI_SLOT_OS *slot;
  cairo_t *scratch;
  int width, height, mouseb, mousex, mousey,
    resizable, hidden, started, fullscreen,
    minwidth, minheight;
  VALUE self;
  VALUE canvas;
  VALUE keypresses;
  VALUE nestslot;
  VALUE nesting;
  VALUE extras;
  VALUE styles;
  VALUE groups;
  ID cursor;
  VALUE title;
  VALUE location;
  VALUE owner;
} lumi_app;

//
// function signatures
//
VALUE lumi_app_alloc(VALUE);
VALUE lumi_app_new(VALUE);
VALUE lumi_apps_get(VALUE);
int lumi_app_remove(lumi_app *);
VALUE lumi_app_get_title(VALUE);
VALUE lumi_app_set_title(VALUE, VALUE);
VALUE lumi_app_get_fullscreen(VALUE);
VALUE lumi_app_set_fullscreen(VALUE, VALUE);
VALUE lumi_app_slot(VALUE);
lumi_code lumi_app_start(VALUE, char *);
lumi_code lumi_app_open(lumi_app *, char *);
lumi_code lumi_app_loop(void);
lumi_code lumi_app_visit(lumi_app *, char *);
lumi_code lumi_app_paint(lumi_app *);
lumi_code lumi_app_motion(lumi_app *, int, int);
lumi_code lumi_app_click(lumi_app *, int, int, int);
lumi_code lumi_app_release(lumi_app *, int, int, int);
lumi_code lumi_app_wheel(lumi_app *, ID, int, int);
lumi_code lumi_app_keydown(lumi_app *, VALUE);
lumi_code lumi_app_keypress(lumi_app *, VALUE);
lumi_code lumi_app_keyup(lumi_app *, VALUE);
VALUE lumi_app_close_window(lumi_app *);
VALUE lumi_sys(char *, int);
lumi_code lumi_app_goto(lumi_app *, char *);
lumi_code lumi_slot_repaint(LUMI_SLOT_OS *);
void lumi_app_reset_styles(lumi_app *);
void lumi_app_style(lumi_app *, VALUE, VALUE);
VALUE lumi_app_location(VALUE);
VALUE lumi_app_is_started(VALUE);
VALUE lumi_app_quit(VALUE);

#endif
