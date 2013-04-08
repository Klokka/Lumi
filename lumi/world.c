///
// lumi/world.c
// Abstract windowing for GTK, Quartz (OSX) and Win32.
//
#include "lumi/app.h"
#include "lumi/ruby.h"
#include "lumi/config.h"
#include "lumi/world.h"
#include "lumi/native.h"
#include "lumi/internal.h"
#ifdef LUMI_SIGNAL
#include <signal.h>

void
lumi_sigint()
{
  lumi_native_quit();
}
#endif

#ifdef __cplusplus
extern "C" {
#endif

lumi_world_t *lumi_world = NULL;

lumi_world_t *
lumi_world_alloc()
{
  lumi_world_t *world = SHOE_ALLOC(lumi_world_t);
  SHOE_MEMZERO(world, lumi_world_t, 1);
  world->apps = rb_ary_new();
  world->msgs = rb_ary_new();
  world->mainloop = FALSE;
  world->image_cache = st_init_strtable();
  world->blank_image = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, 1, 1);
  world->blank_cache = SHOE_ALLOC(lumi_cached_image);
  world->blank_cache->surface = world->blank_image;
  world->blank_cache->pattern = NULL;
  world->blank_cache->width = 1;
  world->blank_cache->height = 1;
  world->blank_cache->mtime = 0;
  world->default_font = pango_font_description_new();
  pango_font_description_set_family(world->default_font, "Arial");
  pango_font_description_set_absolute_size(world->default_font, 14. * PANGO_SCALE * (96./72.));
  rb_gc_register_address(&world->apps);
  rb_gc_register_address(&world->msgs);
  return world;
}

int
lumi_world_free_image_cache(char *key, lumi_cache_entry *cached, char *arg)
{
  if (cached->type != LUMI_CACHE_ALIAS && cached->image != NULL)
  {
    if (cached->image->pattern != NULL)
      cairo_pattern_destroy(cached->image->pattern);
    if (cached->image->surface != lumi_world->blank_image)
      cairo_surface_destroy(cached->image->surface);
    free(cached->image);
  }
  free(cached);
  free(key);
  return ST_CONTINUE;
}

void
lumi_world_free(lumi_world_t *world)
{
#ifdef VLC_0_9
  if (world->vlc != NULL) libvlc_release(world->vlc);
#endif
  lumi_native_cleanup(world);
  st_foreach(world->image_cache, CASTFOREACH(lumi_world_free_image_cache), 0);
  st_free_table(world->image_cache);
  SHOE_FREE(world->blank_cache);
  cairo_surface_destroy(world->blank_image);
  pango_font_description_free(world->default_font);
  rb_gc_unregister_address(&world->apps);
  rb_gc_unregister_address(&world->msgs);
  if (world != NULL)
    SHOE_FREE(world);
}

#ifdef RUBY_1_9
int
lumi_ruby_embed()
{
  VALUE v;
  char *argv[] = {"ruby", "-e", "1"};

  char**  sysinit_argv = NULL;
  RUBY_INIT_STACK;
#ifdef LUMI_WIN32
  int sysinit_argc = 0;
  ruby_sysinit( &sysinit_argc, &sysinit_argv );
#endif
  ruby_init();
  v = (VALUE)ruby_options(3, argv);
  return !FIXNUM_P(v);
}
#else
#define lumi_ruby_embed ruby_init
#endif

lumi_code
lumi_init(LUMI_INIT_ARGS)
{
#ifdef LUMI_SIGNAL
  signal(SIGINT,  lumi_sigint);
  signal(SIGQUIT, lumi_sigint);
#endif
  lumi_ruby_embed();
  lumi_ruby_init();
  lumi_world = lumi_world_alloc();
#ifdef LUMI_WIN32
  lumi_world->os.instance = inst;
  lumi_world->os.style = style;
#endif
  lumi_native_init();
  rb_const_set(cLumi, rb_intern("FONTS"), lumi_font_list());
  return LUMI_OK;
}

void
lumi_update_fonts(VALUE ary)
{
#if PANGO_VERSION_MAJOR > 1 || PANGO_VERSION_MINOR >= 22
  pango_cairo_font_map_set_default(NULL);
#endif
  rb_funcall(rb_const_get(cLumi, rb_intern("FONTS")), rb_intern("replace"), 1, ary);
}

static VALUE
lumi_load_begin(VALUE v)
{
  char *bootup = (char *)v;
  return rb_eval_string(bootup);
}

static VALUE
lumi_load_exception(VALUE v, VALUE exc)
{
  return exc;
}

lumi_code
lumi_load(char *path)
{
  char bootup[LUMI_BUFSIZE];

  if (path)
  {
    sprintf(bootup, "Lumi.visit(%%q<%s>);", path);

    VALUE v = rb_rescue2(CASTHOOK(lumi_load_begin), (VALUE)bootup, CASTHOOK(lumi_load_exception), Qnil, rb_cObject, 0);
    if (rb_obj_is_kind_of(v, rb_eException))
    {
      lumi_canvas_error(Qnil, v);
      rb_eval_string("Lumi.show_log");
    }
  }

  return LUMI_OK;
}

void
lumi_set_argv(int argc, char **argv)
{
  ruby_set_argv(argc, argv);
}

static VALUE
lumi_start_begin(VALUE v)
{
  return rb_eval_string("$LUMI_URI = Lumi.args!");
}

static VALUE
lumi_start_exception(VALUE v, VALUE exc)
{
  return exc;
}

lumi_code
lumi_start(char *path, char *uri)
{
  lumi_code code = LUMI_OK;
  char bootup[LUMI_BUFSIZE];
  int len = lumi_snprintf(bootup,
    LUMI_BUFSIZE,
    "begin;"
      "DIR = File.expand_path(File.dirname(%%q<%s>));"
      "$:.replace([DIR+'/ruby/lib/'+(ENV['LUMI_RUBY_ARCH'] || RUBY_PLATFORM), DIR+'/ruby/lib', DIR+'/lib', '.']);"
      "require 'lumi';"
      "DIR;"
    "rescue Object => e;"
      "puts(e.message);"
    "end",
    path);

  if (len < 0 || len >= LUMI_BUFSIZE)
  {
    QUIT("Path to script is too long.");
  }

  VALUE str = rb_eval_string(bootup);
  if (NIL_P(str))
    return LUMI_QUIT;

  StringValue(str);
  strcpy(lumi_world->path, RSTRING_PTR(str));

  char *load_uri_str = NULL;
  VALUE load_uri = rb_rescue2(CASTHOOK(lumi_start_begin), Qnil, CASTHOOK(lumi_start_exception), Qnil, rb_cObject, 0);
  if (!RTEST(load_uri))
    return LUMI_QUIT;
  if (rb_obj_is_kind_of(load_uri, rb_eException))
  {
    QUIT_ALERT(load_uri);
  }

  if (rb_obj_is_kind_of(load_uri, rb_cString))
    load_uri_str = RSTRING_PTR(load_uri);

  code = lumi_load(load_uri_str);
  if (code != LUMI_OK)
    goto quit;

  code = lumi_app_start(lumi_world->apps, uri);
quit:
  return code;
}

lumi_code
lumi_final()
{
  lumi_world_free(lumi_world);
  return LUMI_OK;
}

#ifdef __cplusplus
}
#endif
