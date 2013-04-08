//
// lumi/app.c
// Abstract windowing for GTK, Quartz (OSX) and Win32.
//
#include <glib.h>
#include "lumi/app.h"
#include "lumi/internal.h"
#include "lumi/ruby.h"
#include "lumi/canvas.h"
#include "lumi/world.h"
#include "lumi/native.h"

static void
lumi_app_mark(lumi_app *app)
{
  lumi_native_slot_mark(app->slot);
  rb_gc_mark_maybe(app->title);
  rb_gc_mark_maybe(app->location);
  rb_gc_mark_maybe(app->canvas);
  rb_gc_mark_maybe(app->keypresses);
  rb_gc_mark_maybe(app->nestslot);
  rb_gc_mark_maybe(app->nesting);
  rb_gc_mark_maybe(app->extras);
  rb_gc_mark_maybe(app->styles);
  rb_gc_mark_maybe(app->groups);
  rb_gc_mark_maybe(app->owner);
}

static void
lumi_app_free(lumi_app *app)
{
  SHOE_FREE(app->slot);
  cairo_destroy(app->scratch);
  RUBY_CRITICAL(free(app));
}

VALUE
lumi_app_alloc(VALUE klass)
{
  lumi_app *app = SHOE_ALLOC(lumi_app);
  SHOE_MEMZERO(app, lumi_app, 1);
  app->slot = SHOE_ALLOC(LUMI_SLOT_OS);
  SHOE_MEMZERO(app->slot, LUMI_SLOT_OS, 1);
  app->slot->owner = app;
  app->started = FALSE;
  app->owner = Qnil;
  app->location = Qnil;
  app->canvas = lumi_canvas_new(cLumi, app);
  app->keypresses = rb_hash_new();
  app->nestslot = Qnil;
  app->nesting = rb_ary_new();
  app->extras = rb_ary_new();
  app->groups = Qnil;
  app->styles = Qnil;
  app->title = Qnil;
  app->width = LUMI_APP_WIDTH;
  app->height = LUMI_APP_HEIGHT;
  app->minwidth = 0;
  app->minheight = 0;
  app->fullscreen = FALSE;
  app->resizable = TRUE;
  app->cursor = s_arrow;
  app->scratch = cairo_create(cairo_image_surface_create(CAIRO_FORMAT_ARGB32, 1, 1));
  app->self = Data_Wrap_Struct(klass, lumi_app_mark, lumi_app_free, app);
  rb_extend_object(app->self, cTypes);
  return app->self;
}

VALUE
lumi_app_new(VALUE klass)
{
  VALUE app = lumi_app_alloc(klass);
  rb_ary_push(lumi_world->apps, app);
  return app;
}

VALUE
lumi_apps_get(VALUE self)
{
  return rb_ary_dup(lumi_world->apps);
}

static void
lumi_app_clear(lumi_app *app)
{
  lumi_ele_remove_all(app->extras);
  lumi_canvas_clear(app->canvas);
  app->nestslot = Qnil;
  app->groups = Qnil;
}

//
// When a window is finished, call this to delete it from the master
// list.  Returns 1 if all windows are gone.
//
int
lumi_app_remove(lumi_app *app)
{
  lumi_app_clear(app);
  rb_ary_delete(lumi_world->apps, app->self);
  return (RARRAY_LEN(lumi_world->apps) == 0);
}

lumi_code
lumi_app_resize(lumi_app *app, int width, int height)
{
  app->width = width;
  app->height = height;
  lumi_native_app_resized(app);
  return LUMI_OK;
}

VALUE
lumi_app_window(int argc, VALUE *argv, VALUE self, VALUE owner)
{
  rb_arg_list args;
  VALUE attr = Qnil;
  VALUE app = lumi_app_new(self == cDialog ? cDialog : cApp);
  lumi_app *app_t;
  char *url = "/";
  Data_Get_Struct(app, lumi_app, app_t);

  switch (rb_parse_args(argc, argv, "h,s|h,", &args))
  {
    case 1:
      attr = args.a[0];
    break;

    case 2:
      url = RSTRING_PTR(args.a[0]);
      attr = args.a[1];
    break;
  }

  if (rb_block_given_p()) rb_iv_set(app, "@main_app", rb_block_proc());
  app_t->owner = owner;
  app_t->title = ATTR(attr, title);
  app_t->fullscreen = RTEST(ATTR(attr, fullscreen));
  app_t->resizable = (ATTR(attr, resizable) != Qfalse);
  app_t->hidden = (ATTR(attr, hidden) == Qtrue);
  lumi_app_resize(app_t, ATTR2(int, attr, width, LUMI_APP_WIDTH), ATTR2(int, attr, height, LUMI_APP_HEIGHT));
  if (RTEST(ATTR(attr, minwidth)))
    app_t->minwidth = (NUM2INT(ATTR(attr, minwidth)) - 1) / 2;
  if (RTEST(ATTR(attr, minheight)))
    app_t->minheight = (NUM2INT(ATTR(attr, minheight) -1)) / 2;
  lumi_canvas_init(app_t->canvas, app_t->slot, attr, app_t->width, app_t->height);
  if (lumi_world->mainloop)
    lumi_app_open(app_t, url);
  return app;
}

VALUE
lumi_app_main(int argc, VALUE *argv, VALUE self)
{
  return lumi_app_window(argc, argv, self, Qnil);
}

VALUE
lumi_app_slot(VALUE app)
{
  lumi_app *app_t;
  Data_Get_Struct(app, lumi_app, app_t);
  return app_t->nestslot;
}

VALUE
lumi_app_get_width(VALUE app)
{
  lumi_app *app_t;
  Data_Get_Struct(app, lumi_app, app_t);
  return INT2NUM(app_t->width);
}

VALUE
lumi_app_get_height(VALUE app)
{
  lumi_app *app_t;
  Data_Get_Struct(app, lumi_app, app_t);
  return INT2NUM(app_t->height);
}

VALUE
lumi_app_get_title(VALUE app)
{
  lumi_app *app_t;
  Data_Get_Struct(app, lumi_app, app_t);
  return app_t->title;
}

VALUE
lumi_app_set_title(VALUE app, VALUE title)
{
  lumi_app *app_t;
  Data_Get_Struct(app, lumi_app, app_t);
  return app_t->title = title;
}

VALUE
lumi_app_get_fullscreen(VALUE app)
{
  lumi_app *app_t;
  Data_Get_Struct(app, lumi_app, app_t);
  return app_t->fullscreen ? Qtrue : Qfalse;
}

VALUE
lumi_app_set_fullscreen(VALUE app, VALUE yn)
{
  lumi_app *app_t;
  Data_Get_Struct(app, lumi_app, app_t);
  lumi_native_app_fullscreen(app_t, app_t->fullscreen = RTEST(yn));
  return yn;
}

void
lumi_app_title(lumi_app *app, VALUE title)
{
  char *msg;
  if (!NIL_P(title))
    app->title = title;
  else
    app->title = rb_str_new2(LUMI_APPNAME);
  msg = RSTRING_PTR(app->title);
  lumi_native_app_title(app, msg);
}

lumi_code
lumi_app_start(VALUE allapps, char *uri)
{
  int i;
  lumi_code code;
  lumi_app *app;

  for (i = 0; i < RARRAY_LEN(allapps); i++)
  {
    VALUE appobj2 = rb_ary_entry(allapps, i);
    Data_Get_Struct(appobj2, lumi_app, app);
    if (!app->started)
    {
      code = lumi_app_open(app, uri);
      app->started = TRUE;
      if (code != LUMI_OK)
        return code;
    }
  }

  return lumi_app_loop();
}

lumi_code
lumi_app_open(lumi_app *app, char *path)
{
  lumi_code code = LUMI_OK;
  int dialog = (rb_obj_class(app->self) == cDialog);

  code = lumi_native_app_open(app, path, dialog);
  if (code != LUMI_OK)
    return code;

  lumi_app_title(app, app->title);
  if (app->slot != NULL) lumi_native_slot_reset(app->slot);
  lumi_slot_init(app->canvas, app->slot, 0, 0, app->width, app->height, TRUE, TRUE);
  code = lumi_app_goto(app, path);
  if (code != LUMI_OK)
    return code;

  if (!app->hidden)
    lumi_native_app_show(app);

  return code;
}

lumi_code
lumi_app_loop()
{
  if (lumi_world->mainloop)
    return LUMI_OK;

  lumi_world->mainloop = TRUE;
  INFO("RUNNING LOOP.\n");
  lumi_native_loop();
  return LUMI_OK;
}

typedef struct
{
  lumi_app *app;
  VALUE canvas;
  VALUE block;
  char ieval;
  VALUE args;
} lumi_exec;

#if defined(RUBY_1_9) || defined(RUBY_2_0)
struct METHOD {
    VALUE oclass;		/* class that holds the method */
    VALUE rklass;		/* class of the receiver */
    VALUE recv;
    ID id, oid;
    void *body; /* NODE *body; */
};
#else
struct METHOD {
    VALUE klass, rklass;
    VALUE recv;
    ID id, oid;
    int safe_level;
    NODE *body;
};
#endif

static VALUE
rb_unbound_get_class(VALUE method)
{
  struct METHOD *data;
  Data_Get_Struct(method, struct METHOD, data);
  return data->rklass;
}

static VALUE
lumi_app_run(VALUE rb_exec)
{
  lumi_exec *exec = (lumi_exec *)rb_exec;
  rb_ary_push(exec->app->nesting, exec->canvas);
  if (exec->ieval)
  {
    VALUE obj;
    obj = mfp_instance_eval(exec->app->self, exec->block);
    return obj;
  }
  else
  {
    int i;
    VALUE vargs[10];
    for (i = 0; i < RARRAY_LEN(exec->args); i++)
      vargs[i] = rb_ary_entry(exec->args, i);
    return rb_funcall2(exec->block, s_call, (int)RARRAY_LEN(exec->args), vargs);
  }
}

static VALUE
lumi_app_exception(VALUE rb_exec, VALUE e)
{
  lumi_exec *exec = (lumi_exec *)rb_exec;
  rb_ary_clear(exec->app->nesting);
  lumi_canvas_error(exec->canvas, e);
  return Qnil;
}

lumi_code
lumi_app_visit(lumi_app *app, char *path)
{
  lumi_exec exec;
  lumi_canvas *canvas;
  VALUE meth;
  Data_Get_Struct(app->canvas, lumi_canvas, canvas);

  canvas->slot->scrolly = 0;
  lumi_native_slot_clear(canvas);
  lumi_app_clear(app);
  lumi_app_reset_styles(app);
  meth = rb_funcall(cLumi, s_run, 1, app->location = rb_str_new2(path));

  VALUE app_block = rb_iv_get(app->self, "@main_app");
  if (!NIL_P(app_block))
    rb_ary_store(meth, 0, app_block);

  exec.app = app;
  exec.block = rb_ary_entry(meth, 0);
  exec.args = rb_ary_entry(meth, 1);
  if (rb_obj_is_kind_of(exec.block, rb_cUnboundMethod)) {
    //VALUE klass = rb_unbound_get_class(exec.block);
    VALUE klass = rb_ary_entry(meth, 2);
    exec.canvas = app->nestslot = lumi_slot_new(klass, ssNestSlot, app->canvas);
    exec.block = rb_funcall(exec.block, s_bind, 1, exec.canvas);
    exec.ieval = 0;
    rb_ary_push(canvas->contents, exec.canvas);
  } else {
    exec.canvas = app->nestslot = app->canvas;
    exec.ieval = 1;
  }

  rb_rescue2(CASTHOOK(lumi_app_run), (VALUE)&exec, CASTHOOK(lumi_app_exception), (VALUE)&exec, rb_cObject, 0);

  rb_ary_clear(exec.app->nesting);
  return LUMI_OK;
}

lumi_code
lumi_app_paint(lumi_app *app)
{
  lumi_canvas_paint(app->canvas);
  return LUMI_OK;
}

lumi_code
lumi_app_motion(lumi_app *app, int x, int y)
{
  app->mousex = x; app->mousey = y;
  lumi_canvas_send_motion(app->canvas, x, y, Qnil);
  return LUMI_OK;
}

lumi_code
lumi_app_click(lumi_app *app, int button, int x, int y)
{
  app->mouseb = button;
  lumi_canvas_send_click(app->canvas, button, x, y);
  return LUMI_OK;
}

lumi_code
lumi_app_release(lumi_app *app, int button, int x, int y)
{
  app->mouseb = 0;
  lumi_canvas_send_release(app->canvas, button, x, y);
  return LUMI_OK;
}

lumi_code
lumi_app_wheel(lumi_app *app, ID dir, int x, int y)
{
  lumi_canvas *canvas;
  Data_Get_Struct(app->canvas, lumi_canvas, canvas);
  if (canvas->slot->vscroll)
  {
    if (dir == s_up)
      lumi_slot_scroll_to(canvas, -16, 1);
    else if (dir == s_down)
      lumi_slot_scroll_to(canvas, 16, 1);
  }
  lumi_canvas_send_wheel(app->canvas, dir, x, y);
  return LUMI_OK;
}

lumi_code
lumi_app_keydown(lumi_app *app, VALUE key)
{
  if (!RTEST(rb_hash_aref(app->keypresses, key))) {
    rb_hash_aset(app->keypresses, key, Qtrue);
    lumi_canvas_send_keydown(app->canvas, key);
  }
  return LUMI_OK;
}

lumi_code
lumi_app_keypress(lumi_app *app, VALUE key)
{
  if (key == symAltSlash)
    rb_eval_string("Lumi.show_log");
  else if (key == symAltQuest)
    rb_eval_string("Lumi.show_manual");
  else if (key == symAltDot)
    rb_eval_string("Lumi.show_selector");
  else
    lumi_canvas_send_keypress(app->canvas, key);
  return LUMI_OK;
}

lumi_code
lumi_app_keyup(lumi_app *app, VALUE key)
{
  rb_hash_aset(app->keypresses, key, Qfalse);
  lumi_canvas_send_keyup(app->canvas, key);
  return LUMI_OK;
}

VALUE
lumi_sys(char *cmd, int detach)
{
  if (detach)
    return rb_funcall(rb_mKernel, rb_intern("system"), 1, rb_str_new2(cmd));
  else
    return rb_funcall(rb_mKernel, '`', 1, rb_str_new2(cmd));
}

lumi_code
lumi_app_goto(lumi_app *app, char *path)
{
  lumi_code code = LUMI_OK;
  const char http_scheme[] = "http://";
  if (strlen(path) > strlen(http_scheme) && strncmp(http_scheme, path, strlen(http_scheme)) == 0) {
    lumi_browser_open(path);
  } else {
    code = lumi_app_visit(app, path);
    if (code == LUMI_OK)
    {
      lumi_app_motion(app, app->mousex, app->mousey);
      lumi_slot_repaint(app->slot);
    }
  }
  return code;
}

lumi_code
lumi_slot_repaint(LUMI_SLOT_OS *slot)
{
  lumi_native_slot_paint(slot);
  return LUMI_OK;
}

static void
lumi_style_set(VALUE styles, VALUE klass, VALUE k, VALUE v)
{
  VALUE hsh = rb_hash_aref(styles, klass);
  if (NIL_P(hsh))
    rb_hash_aset(styles, klass, hsh = rb_hash_new());
  rb_hash_aset(hsh, k, v);
}

#define STYLE(klass, k, v) \
  lumi_style_set(app->styles, klass, \
    ID2SYM(rb_intern("" # k)), rb_str_new2("" # v))

void
lumi_app_reset_styles(lumi_app *app)
{
  app->styles = rb_hash_new();
  STYLE(cBanner,      size, 48);
  STYLE(cTitle,       size, 34);
  STYLE(cSubtitle,    size, 26);
  STYLE(cTagline,     size, 18);
  STYLE(cCaption,     size, 14);
  STYLE(cPara,        size, 12);
  STYLE(cInscription, size, 10);

  STYLE(cCode,        family, monospace);
  STYLE(cDel,         strikethrough, single);
  STYLE(cEm,          emphasis, italic);
  STYLE(cIns,         underline, single);
  STYLE(cLink,        underline, single);
  STYLE(cLink,        stroke, #06E);
  STYLE(cLinkHover,   underline, single);
  STYLE(cLinkHover,   stroke, #039);
  STYLE(cStrong,      weight, bold);
  STYLE(cSup,         rise,   10);
  STYLE(cSup,         size,   x-small);
  STYLE(cSub,         rise,   -10);
  STYLE(cSub,         size,   x-small);
}

void
lumi_app_style(lumi_app *app, VALUE klass, VALUE hsh)
{
  long i;
  VALUE keys = rb_funcall(hsh, s_keys, 0);
  for ( i = 0; i < RARRAY_LEN(keys); i++ )
  {
    VALUE key = rb_ary_entry(keys, i);
    VALUE val = rb_hash_aref(hsh, key);
    if (!SYMBOL_P(key)) key = rb_str_intern(key);
    lumi_style_set(app->styles, klass, key, val);
  }
}

VALUE
lumi_app_close_window(lumi_app *app)
{
  lumi_native_app_close(app);
  return Qnil;
}

VALUE
lumi_app_location(VALUE self)
{
  lumi_app *app;
  Data_Get_Struct(self, lumi_app, app);
  return app->location;
}

VALUE
lumi_app_is_started(VALUE self)
{
  lumi_app *app;
  Data_Get_Struct(self, lumi_app, app);
  return app->started ? Qtrue : Qfalse;
}

VALUE
lumi_app_contents(VALUE self)
{
  lumi_app *app;
  Data_Get_Struct(self, lumi_app, app);
  return lumi_canvas_contents(app->canvas);
}

VALUE
lumi_app_quit(VALUE self)
{
  lumi_native_quit();
  return self;
}
