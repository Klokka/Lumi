//
// lumi/world.h
//
// The lumi_world struct contains global information about the environment which is shared between
// app windows.
//
#ifndef LUMI_WORLD_H
#define LUMI_WORLD_H

#include "lumi/config.h"
#include "lumi/ruby.h"
#include "lumi/code.h"

#ifdef __cplusplus
extern "C" {
#endif

LUMI_EXTERN typedef struct _lumi_world_t {
  LUMI_WORLD_OS os;
  int mainloop;
  char path[LUMI_BUFSIZE];
  VALUE apps, msgs;
  st_table *image_cache;
  guint thread_event;
  cairo_surface_t *blank_image;
  lumi_cached_image *blank_cache;
  PangoFontDescription *default_font;
#ifdef VLC_0_9
  libvlc_instance_t *vlc;
#endif
} lumi_world_t;

extern LUMI_EXTERN lumi_world_t *lumi_world;

#define GLOBAL_APP(appvar) \
  lumi_app *appvar = NULL; \
  if (RARRAY_LEN(lumi_world->apps) > 0) \
    Data_Get_Struct(rb_ary_entry(lumi_world->apps, 0), lumi_app, appvar)

#define ROUND(x) ((x) >= 0 ? (int)round((x)+0.5) : (int)round((x)-0.5))

//
// Lumi World
// 
LUMI_EXTERN lumi_world_t *lumi_world_alloc(void);
LUMI_EXTERN void lumi_world_free(lumi_world_t *);
void lumi_update_fonts(VALUE);

//
// Lumi
// 
LUMI_EXTERN lumi_code lumi_init(LUMI_INIT_ARGS);
LUMI_EXTERN lumi_code lumi_load(char *);
LUMI_EXTERN lumi_code lumi_start(char *, char *);
#ifdef LUMI_WIN32
LUMI_EXTERN int lumi_win32_cmdvector(const char *, char ***);
#endif
LUMI_EXTERN void lumi_set_argv(int, char **);
LUMI_EXTERN lumi_code lumi_final(void);

#ifdef __cplusplus
}
#endif

#endif
