//
// lumi/ruby.c
// Just little bits of Ruby I've become accustomed to.
//
#include "lumi/app.h"
#include "lumi/canvas.h"
#include "lumi/ruby.h"
#include "lumi/internal.h"
#include "lumi/world.h"
#include "lumi/native.h"
#include "lumi/version.h"
#include "lumi/http.h"
#include "lumi/effects.h"
#include "lumi/ruby.h"
#include <math.h>

VALUE cLumi, cApp, cDialog, cTypes, cLumiWindow, cMouse, cCanvas, cFlow, cStack, cMask, cWidget, cShape, cImage, cEffect, cVideo, cTimerBase, cTimer, cEvery, cAnim, cPattern, cBorder, cBackground, cTextBlock, cPara, cBanner, cTitle, cSubtitle, cTagline, cCaption, cInscription, cTextClass, cSpan, cDel, cStrong, cSub, cSup, cCode, cEm, cIns, cLinkUrl, cNative, cButton, cCheck, cRadio, cEditLine, cEditBox, cListBox, cProgress, cSlider, cColor, cDownload, cResponse, cColors, cLink, cLinkHover, ssNestSlot;
VALUE eVlcError, eImageError, eInvMode, eNotImpl;
VALUE reHEX_SOURCE, reHEX3_SOURCE, reRGB_SOURCE, reRGBA_SOURCE, reGRAY_SOURCE, reGRAYA_SOURCE, reLF;
VALUE symAltQuest, symAltSlash, symAltDot;
ID s_checked_q, s_perc, s_aref, s_mult;
SYMBOL_DEFS(SYMBOL_ID);

//
// Mauricio's instance_eval hack (he bested my cloaker back in 06 Jun 2006)
//
VALUE instance_eval_proc;

VALUE
mfp_instance_eval(VALUE obj, VALUE block)
{
  return rb_funcall(instance_eval_proc, s_call, 2, obj, block);
}

//
// From Guy Decoux [ruby-talk:144098]
//
static VALUE
ts_each(VALUE *tmp)
{
  return rb_funcall2(tmp[0], (ID)tmp[1], (int)tmp[2], (VALUE *)tmp[3]);
}

VALUE
ts_funcall2(VALUE obj, ID meth, int argc, VALUE *argv)
{
  VALUE tmp[4];
  if (!rb_block_given_p())
    return rb_funcall2(obj, meth, argc, argv);
  tmp[0] = obj;
  tmp[1] = (VALUE)meth;
  tmp[2] = (VALUE)argc;
  tmp[3] = (VALUE)argv;
  return rb_iterate((VALUE(*)(VALUE))ts_each, (VALUE)tmp, CASTHOOK(rb_yield), 0);
}

#define SET_ARG(o) args->a[n] = o, n = n + 1
#define CHECK_ARG(mac, t, d, cond, condarg) \
{ \
  if ((!x || m) && n < argc) \
  { \
    if (mac(argv[n]) == t) \
      SET_ARG(argv[n]); \
    else if (cond) \
      SET_ARG(condarg); \
    else \
    { \
      if (m) m = 0; \
      x = 1; \
    } \
  } \
  else if (m) \
    SET_ARG(d); \
  else \
    x = 1; \
}
#define CHECK_ARG_TYPE(t, d) CHECK_ARG(TYPE, t, d, 0, Qnil)
#define CHECK_ARG_NOT_NIL()  CHECK_ARG(NIL_P, 0, Qnil, 0, Qnil)
#define CHECK_ARG_COERCE(t, meth) \
  CHECK_ARG(TYPE, t, Qnil, rb_respond_to(argv[n], s_##meth), rb_funcall(argv[n], s_##meth, 0))
#define CHECK_ARG_DATA(func) CHECK_ARG(TYPE, T_DATA, Qnil, func(argv[n]), argv[n])

//
// rb_parse_args
// - a rb_scan_args replacement, designed to assist in typecasting, since
//   use of RSTRING_* and RARRAY_* macros are so common in Lumi.
//
// returns 0 if no match.
// returns 1 and up, the arg list matched.
// (args.n is set to the arg count, args.a is the args)
//
static int
rb_parse_args_p(unsigned char rais, int argc, const VALUE *argv, const char *fmt, rb_arg_list *args)
{
  int i = 1, m = 0, n = 0, nmin = 0, x = 0;
  const char *p = fmt;
  args->n = 0;

  do
  {
    if (*p == ',')
    {
      if ((x && !m) || n < argc) { i++; x = 0; if (nmin == 0 || nmin > n) { nmin = n; } n = 0; }
      else break;
    }
    else if (*p == '|')
    {
      if (!x) m = i;
    }
    else if (*p == 's')
      CHECK_ARG_COERCE(T_STRING, to_str)
    else if (*p == 'S')
      CHECK_ARG_COERCE(T_STRING, to_s)
    else if (*p == 'i')
      CHECK_ARG_COERCE(T_FIXNUM, to_int)
    else if (*p == 'I')
      CHECK_ARG_COERCE(T_FIXNUM, to_i)
    else if (*p == 'f')
      CHECK_ARG_TYPE(T_FLOAT, Qnil)
    else if (*p == 'F')
      CHECK_ARG_COERCE(T_FLOAT, to_f)
    else if (*p == 'a')
      CHECK_ARG_COERCE(T_ARRAY, to_ary)
    else if (*p == 'A')
      CHECK_ARG_COERCE(T_ARRAY, to_a)
    else if (*p == 'k')
      CHECK_ARG_TYPE(T_CLASS, Qnil)
    else if (*p == 'h')
      CHECK_ARG_TYPE(T_HASH, Qnil)
    else if (*p == 'o')
      CHECK_ARG_NOT_NIL()
    else if (*p == '&')
    {
      if (rb_block_given_p())
        SET_ARG(rb_block_proc());
      else
        SET_ARG(Qnil);
    }

    //
    // lumi-specific structures
    //
    else if (*p == 'e')
      CHECK_ARG_DATA(lumi_is_element)
    else if (*p == 'E')
      CHECK_ARG_DATA(lumi_is_any)
    else break;
  }
  while (p++);

  if (!x && n >= argc)
    m = i;
  if (m)
    args->n = n;

  // printf("rb_parse_args(%s): %d %d (%d)\n", fmt, m, n, x);

  if (!m && rais)
  {
    if (argc < nmin)
      rb_raise(rb_eArgError, "wrong number of arguments (%d for %d)", argc, nmin);
    else
      rb_raise(rb_eArgError, "bad arguments");
  }

  return m;
}

int
rb_parse_args(int argc, const VALUE *argv, const char *fmt, rb_arg_list *args)
{
  return rb_parse_args_p(1, argc, argv, fmt, args);
}

int
rb_parse_args_allow(int argc, const VALUE *argv, const char *fmt, rb_arg_list *args)
{
  return rb_parse_args_p(0, argc, argv, fmt, args);
}

long
rb_ary_index_of(VALUE ary, VALUE val)
{
  long i;

  for (i=0; i<RARRAY_LEN(ary); i++) {
    if (rb_equal(RARRAY_PTR(ary)[i], val))
      return i;
  }

  return -1;
}

VALUE
rb_ary_insert_at(VALUE ary, long index, int len, VALUE ary2)
{
  rb_funcall(ary, s_aref, 3, LONG2NUM(index), INT2NUM(len), ary2);
  return ary;
}

//
// from ruby's eval.c
//
static inline VALUE
call_cfunc(HOOK func, VALUE recv, int len, int argc, VALUE *argv)
{
  if (len >= 0 && argc != len) {
    rb_raise(rb_eArgError, "wrong number of arguments (%d for %d)",
      argc, len);
  }

  switch (len) {
    case -2:
    return (*func)(recv, rb_ary_new4(argc, argv));
    case -1:
    return (*func)(argc, argv, recv);
    case 0:
    return (*func)(recv);
    case 1:
    return (*func)(recv, argv[0]);
    case 2:
    return (*func)(recv, argv[0], argv[1]);
    case 3:
    return (*func)(recv, argv[0], argv[1], argv[2]);
    case 4:
    return (*func)(recv, argv[0], argv[1], argv[2], argv[3]);
    case 5:
    return (*func)(recv, argv[0], argv[1], argv[2], argv[3], argv[4]);
    case 6:
    return (*func)(recv, argv[0], argv[1], argv[2], argv[3], argv[4],
               argv[5]);
    case 7:
    return (*func)(recv, argv[0], argv[1], argv[2], argv[3], argv[4],
               argv[5], argv[6]);
    case 8:
    return (*func)(recv, argv[0], argv[1], argv[2], argv[3], argv[4],
               argv[5], argv[6], argv[7]);
    case 9:
    return (*func)(recv, argv[0], argv[1], argv[2], argv[3], argv[4],
               argv[5], argv[6], argv[7], argv[8]);
    case 10:
    return (*func)(recv, argv[0], argv[1], argv[2], argv[3], argv[4],
               argv[5], argv[6], argv[7], argv[8], argv[9]);
    case 11:
    return (*func)(recv, argv[0], argv[1], argv[2], argv[3], argv[4],
               argv[5], argv[6], argv[7], argv[8], argv[9], argv[10]);
    case 12:
    return (*func)(recv, argv[0], argv[1], argv[2], argv[3], argv[4],
               argv[5], argv[6], argv[7], argv[8], argv[9],
               argv[10], argv[11]);
    case 13:
    return (*func)(recv, argv[0], argv[1], argv[2], argv[3], argv[4],
               argv[5], argv[6], argv[7], argv[8], argv[9], argv[10],
               argv[11], argv[12]);
    case 14:
    return (*func)(recv, argv[0], argv[1], argv[2], argv[3], argv[4],
               argv[5], argv[6], argv[7], argv[8], argv[9], argv[10],
               argv[11], argv[12], argv[13]);
    case 15:
    return (*func)(recv, argv[0], argv[1], argv[2], argv[3], argv[4],
               argv[5], argv[6], argv[7], argv[8], argv[9], argv[10],
               argv[11], argv[12], argv[13], argv[14]);
    default:
    rb_raise(rb_eArgError, "too many arguments (%d)", len);
    break;
  }
  return Qnil;        /* not reached */
}

typedef struct
{
  VALUE canvas;
  VALUE block;
  VALUE args;
} safe_block;

static VALUE
lumi_safe_block_call(VALUE rb_sb)
{
  int i;
  VALUE vargs[10];
  safe_block *sb = (safe_block *)rb_sb;
  for (i = 0; i < RARRAY_LEN(sb->args); i++)
    vargs[i] = rb_ary_entry(sb->args, i);
  return rb_funcall2(sb->block, s_call, (int)RARRAY_LEN(sb->args), vargs);
}

static VALUE
lumi_safe_block_exception(VALUE rb_sb, VALUE e)
{
  safe_block *sb = (safe_block *)rb_sb;
  lumi_canvas_error(sb->canvas, e);
  return Qnil;
}

VALUE
lumi_safe_block(VALUE self, VALUE block, VALUE args)
{
  safe_block sb;
  VALUE v;

  sb.canvas = lumi_find_canvas(self);
  sb.block = block;
  sb.args = args;
  rb_gc_register_address(&args);

  v = rb_rescue2(CASTHOOK(lumi_safe_block_call), (VALUE)&sb,
    CASTHOOK(lumi_safe_block_exception), (VALUE)&sb, rb_cObject, 0);
  rb_gc_unregister_address(&args);
  return v;
}

int
lumi_px(VALUE obj, int dv, int pv, int nv)
{
  int px;
  if (TYPE(obj) == T_STRING) {
    char *ptr = RSTRING_PTR(obj);
    int len = (int)RSTRING_LEN(obj);
    obj = rb_funcall(obj, s_to_i, 0);
    if (len > 1 && ptr[len - 1] == '%')
    {
      obj = rb_funcall(obj, s_mult, 1, rb_float_new(0.01));
    }
  }
  if (rb_obj_is_kind_of(obj, rb_cFloat)) {
    px = (int)((double)pv * NUM2DBL(obj));
  } else {
    if (NIL_P(obj))
      px = dv;
    else
      px = NUM2INT(obj);
    if (px < 0 && nv == 1)
      px += pv;
  }
  return px;
}

int
lumi_px2(VALUE attr, ID k1, ID k2, int dv, int dr, int pv)
{
  int px;
  VALUE obj = lumi_hash_get(attr, k2);
  if (!NIL_P(obj))
  {
    px = lumi_px(obj, 0, pv, 0);
    px = (pv - dr) - px;
  }
  else
  {
    px = lumi_px(lumi_hash_get(attr, k1), dv, pv, 0);
  }
  return px;
}

VALUE
lumi_hash_set(VALUE hsh, ID key, VALUE val)
{
  if (NIL_P(hsh))
    hsh = rb_hash_new();
  rb_hash_aset(hsh, ID2SYM(key), val);
  return hsh;
}

VALUE
lumi_hash_get(VALUE hsh, ID key)
{
  VALUE v;

  if (TYPE(hsh) == T_HASH)
  {
    v = rb_hash_aref(hsh, ID2SYM(key));
    if (!NIL_P(v)) return v;
  }

  return Qnil;
}

int
lumi_hash_int(VALUE hsh, ID key, int dn)
{
  VALUE v = lumi_hash_get(hsh, key);
  if (!NIL_P(v)) return NUM2INT(v);
  return dn;
}

double
lumi_hash_dbl(VALUE hsh, ID key, double dn)
{
  VALUE v = lumi_hash_get(hsh, key);
  if (!NIL_P(v)) return NUM2DBL(v);
  return dn;
}

char *
lumi_hash_cstr(VALUE hsh, ID key, char *dn)
{
  VALUE v = lumi_hash_get(hsh, key);
  if (!NIL_P(v)) return RSTRING_PTR(v);
  return dn;
}

VALUE
rb_str_to_pas(VALUE str)
{
  VALUE str2;
  char slen[2];
  slen[0] = RSTRING_LEN(str);
  slen[1] = '\0';
  str2 = rb_str_new2(slen);
  rb_str_append(str2, str);
  return str2;
}

void
lumi_place_exact(lumi_place *place, VALUE attr, int ox, int oy)
{
  int r;
  VALUE x;
  place->dx = ATTR2(int, attr, displace_left, 0);
  place->dy = ATTR2(int, attr, displace_top, 0);
  place->flags = FLAG_ABSX | FLAG_ABSY;
  place->ix = place->x = ATTR2(int, attr, left, 0) + ox;
  place->iy = place->y = ATTR2(int, attr, top, 0) + oy;
  r = ATTR2(int, attr, radius, 0) * 2;
  place->iw = place->w = ATTR2(int, attr, width, r);
  place->ih = place->h = ATTR2(int, attr, height, place->w);
  x = ATTR(attr, right);
  if (!NIL_P(x)) place->iw = place->w = (NUM2INT(x) + ox) - place->x;
  x = ATTR(attr, bottom);
  if (!NIL_P(x)) place->ih = place->h = (NUM2INT(x) + oy) - place->y;

  if (RTEST(ATTR(attr, center)))
  {
    place->ix = place->x = place->x - (place->w / 2);
    place->iy = place->y = place->y - (place->h / 2);
  }
}

void
lumi_place_decide(lumi_place *place, VALUE c, VALUE attr, int dw, int dh, unsigned char rel, int padded)
{
  lumi_canvas *canvas = NULL;
  VALUE ck = rb_obj_class(c);
  VALUE stuck = ATTR(attr, attach);
  if (!NIL_P(c))
    Data_Get_Struct(c, lumi_canvas, canvas);

  if (REL_FLAGS(rel) & REL_SCALE)
  {
    VALUE rw = ATTR(attr, width), rh = ATTR(attr, height);
    if (NIL_P(rw) && !NIL_P(rh))
      dw = ROUND(((dw * 1.) / dh) * lumi_px(rh, dh, CPW(canvas), 1));
    else if (NIL_P(rh) && !NIL_P(rw))
      dh = ROUND(((dh * 1.) / dw) * lumi_px(rw, dw, CPW(canvas), 1));
  }

  ATTR_MARGINS(attr, 0, canvas);
  if (padded || dh == 0) dh += tmargin + bmargin;
  if (padded || dw == 0) dw += lmargin + rmargin;

  int testw = dw;
  if (testw == 0) testw = lmargin + 1 + rmargin;

  if (!NIL_P(stuck))
  {
    if (stuck == cLumiWindow)
      rel = REL_FLAGS(rel) | REL_WINDOW;
    else if (stuck == cMouse)
      rel = REL_FLAGS(rel) | REL_CURSOR;
    else
      rel = REL_FLAGS(rel) | REL_STICKY;
  }

  place->flags = rel;
  place->dx = place->dy = 0;
  if (canvas == NULL)
  {
    place->ix = place->x = 0;
    place->iy = place->y = 0;
    place->iw = place->w = dw;
    place->ih = place->h = dh;
  }
  else
  {
    int cx, cy, ox, oy, tw = dw, th = dh;

    switch (REL_COORDS(rel))
    {
      case REL_WINDOW:
        cx = 0; cy = 0;
        ox = 0; oy = canvas->slot->scrolly;
      break;

      case REL_CANVAS:
        cx = canvas->cx - CPX(canvas);
        cy = canvas->cy - CPY(canvas);
        ox = CPX(canvas);
        oy = CPY(canvas);
      break;

      case REL_CURSOR:
        cx = 0; cy = 0;
        ox = canvas->app->mousex; oy = canvas->app->mousey;
      break;

      case REL_TILE:
        cx = 0; cy = 0;
        ox = CPX(canvas);
        oy = CPY(canvas);
        testw = dw = CPW(canvas);
        dh = max(canvas->height, CPH(canvas));
      break;

      default:
        cx = 0; cy = 0;
        ox = canvas->cx; oy = canvas->cy;
        if ((REL_COORDS(rel) & REL_STICKY) && lumi_is_element(stuck))
        {
          lumi_element *element;
          Data_Get_Struct(stuck, lumi_element, element);
          ox = element->place.x;
          oy = element->place.y;
        }
      break;
    }

    place->w = PX(attr, width, testw, CPW(canvas));
    if (dw == 0 && place->w + (int)canvas->cx > canvas->place.iw) {
      canvas->cx = canvas->endx = CPX(canvas);
      canvas->cy = canvas->endy;
      place->w = canvas->place.iw;
    }
    place->h = PX(attr, height, dh, CPH(canvas));

    if (REL_COORDS(rel) != REL_TILE)
    {
      tw = place->w;
      th = place->h;
    }

    place->x = PX2(attr, left, right, cx, tw, canvas->place.iw) + ox;
    place->y = PX2(attr, top, bottom, cy, th,
      ORIGIN(canvas->place) ? canvas->height : canvas->fully) + oy;
    if (!ORIGIN(canvas->place))
    {
      place->dx = canvas->place.dx;
      place->dy = canvas->place.dy;
    }
    place->dx += PXN(attr, displace_left, 0, CPW(canvas));
    place->dy += PXN(attr, displace_top, 0, CPH(canvas));

    place->flags |= NIL_P(ATTR(attr, left)) && NIL_P(ATTR(attr, right)) ? 0 : FLAG_ABSX;
    place->flags |= NIL_P(ATTR(attr, top)) && NIL_P(ATTR(attr, bottom)) ? 0 : FLAG_ABSY;
    if (REL_COORDS(rel) != REL_TILE && ABSY(*place) == 0 && (ck == cStack || place->x + place->w > CPX(canvas) + canvas->place.iw))
    {
      canvas->cx = place->x = CPX(canvas);
      canvas->cy = place->y = canvas->endy;
    }
  }

  place->ix = place->x + lmargin;
  place->iy = place->y + tmargin;
  place->iw = place->w - (lmargin + rmargin);
  if (place->iw < 0) place->iw = 0;
  place->ih = place->h - (tmargin + bmargin);
  if (place->ih < 0) place->ih = 0;

  INFO("PLACE: (%d, %d), (%d: %d, %d: %d) [%d, %d] %x\n", place->x, place->y, place->w, place->iw, place->h, place->ih, ABSX(*place), ABSY(*place), place->flags);
}

//
// lumi_basic routines
//
VALUE
lumi_basic_remove(VALUE self)
{
  GET_STRUCT(basic, self_t);
  lumi_canvas_remove_item(self_t->parent, self, 0, 0);
  lumi_canvas_repaint_all(self_t->parent);
  return self;
}

unsigned char
lumi_is_element_p(VALUE ele, unsigned char any)
{
  void *dmark;
  if (TYPE(ele) != T_DATA)
    return 0;
  dmark = RDATA(ele)->dmark;
  return (dmark == lumi_canvas_mark || dmark == lumi_shape_mark ||
    dmark == lumi_image_mark || dmark == lumi_effect_mark ||
    dmark == lumi_pattern_mark || dmark == lumi_textblock_mark ||
    dmark == lumi_control_mark ||
    (any && (dmark == lumi_http_mark || dmark == lumi_timer_mark ||
             dmark == lumi_color_mark || dmark == lumi_link_mark ||
             dmark == lumi_text_mark))
  );
}

unsigned char
lumi_is_element(VALUE ele)
{
  return lumi_is_element_p(ele, 0);
}

unsigned char
lumi_is_any(VALUE ele)
{
  return lumi_is_element_p(ele, 1);
}

void
lumi_extras_remove_all(lumi_canvas *canvas)
{
  int i;
  lumi_basic *basic;
  lumi_canvas *parent;
  if (canvas->app == NULL) return;
  for (i = (int)RARRAY_LEN(canvas->app->extras) - 1; i >= 0; i--)
  {
    VALUE ele = rb_ary_entry(canvas->app->extras, i);
    Data_Get_Struct(ele, lumi_basic, basic);
    Data_Get_Struct(basic->parent, lumi_canvas, parent);
    if (parent == canvas)
    {
      rb_funcall(ele, s_remove, 0);
      rb_ary_delete_at(canvas->app->extras, i);
    }
  }
}

void
lumi_ele_remove_all(VALUE contents)
{
  if (!NIL_P(contents))
  {
    long i;
    VALUE ary;
    ary = rb_ary_dup(contents);
    rb_gc_register_address(&ary);
    for (i = 0; i < RARRAY_LEN(ary); i++)
      rb_funcall(rb_ary_entry(ary, i), s_remove, 0);
    rb_gc_unregister_address(&ary);
    rb_ary_clear(contents);
  }
}

void
lumi_cairo_arc(cairo_t *cr, double x, double y, double w, double h, double a1, double a2)
{
  cairo_translate(cr, x, y);
  cairo_scale(cr, w / 2., h / 2.);
  cairo_arc(cr, 0., 0., 1., a1, a2);
}

void
lumi_cairo_rect(cairo_t *cr, double x, double y, double w, double h, double r)
{
  double rc = r * BEZIER;
  cairo_move_to(cr, x + r, y);
  cairo_rel_line_to(cr, w - 2 * r, 0.0);
  if (r != 0.) cairo_rel_curve_to(cr, rc, 0.0, r, rc, r, r);
  cairo_rel_line_to(cr, 0, h - 2 * r);
  if (r != 0.) cairo_rel_curve_to(cr, 0.0, rc, rc - r, r, -r, r);
  cairo_rel_line_to(cr, -w + 2 * r, 0);
  if (r != 0.) cairo_rel_curve_to(cr, -rc, 0, -r, -rc, -r, -r);
  cairo_rel_line_to(cr, 0, -h + 2 * r);
  if (r != 0.) cairo_rel_curve_to(cr, 0.0, -rc, r - rc, -r, r, -r);
  cairo_close_path(cr);
}

void
lumi_control_hide_ref(LUMI_CONTROL_REF ref)
{
  if (ref != NULL) lumi_native_control_hide(ref);
}

void
lumi_control_show_ref(LUMI_CONTROL_REF ref)
{
  if (ref != NULL) lumi_native_control_show(ref);
}

//
// Macros for setting up drawing
//
#define SETUP(self_type, rel, dw, dh) \
  self_type *self_t; \
  lumi_place place; \
  lumi_canvas *canvas; \
  Data_Get_Struct(self, self_type, self_t); \
  Data_Get_Struct(c, lumi_canvas, canvas); \
  if (ATTR(self_t->attr, hidden) == Qtrue) return self; \
  lumi_place_decide(&place, c, self_t->attr, dw, dh, rel, REL_COORDS(rel) == REL_CANVAS)

#define SETUP_CONTROL(dh, dw, flex) \
  char *msg = ""; \
  int len = dw ? dw : 200; \
  lumi_control *self_t; \
  lumi_canvas *canvas; \
  lumi_place place; \
  VALUE text = Qnil, ck = rb_obj_class(c); \
  Data_Get_Struct(self, lumi_control, self_t); \
  Data_Get_Struct(c, lumi_canvas, canvas); \
  text = ATTR(self_t->attr, text); \
  if (!NIL_P(text)) { \
    text = lumi_native_to_s(text); \
    msg = RSTRING_PTR(text); \
    if (flex) len = ((int)RSTRING_LEN(text) * 8) + 32; \
  } \
  lumi_place_decide(&place, c, self_t->attr, len, 28 + dh, REL_CANVAS, TRUE)

#define FINISH() \
  if (!ABSY(place)) { \
    canvas->cx += place.w; \
    canvas->cy = place.y; \
    canvas->endx = canvas->cx; \
    canvas->endy = max(canvas->endy, place.y + place.h); \
  } \
  if (ck == cStack) { \
    canvas->cx = CPX(canvas); \
    canvas->cy = canvas->endy; \
  }

#define PATTERN_SCALE(self_t, place, sw) \
  if (self_t->cached == NULL) \
  { \
    double woff = abs(place.iw) + (sw * 2.), hoff = abs(place.ih) + (sw * 2.); \
    cairo_pattern_get_matrix(PATTERN(self_t), &matrix1); \
    cairo_pattern_get_matrix(PATTERN(self_t), &matrix2); \
    if (cairo_pattern_get_type(PATTERN(self_t)) == CAIRO_PATTERN_TYPE_RADIAL) \
      cairo_matrix_translate(&matrix2, (-place.ix * 1.) / woff, (-place.iy * 1.) / hoff); \
    cairo_matrix_scale(&matrix2, 1. / woff, 1. / hoff); \
    if (sw != 0.0) cairo_matrix_translate(&matrix2, sw, sw); \
    cairo_pattern_set_matrix(PATTERN(self_t), &matrix2); \
  }

#define PATTERN_RESET(self_t) \
  if (self_t->cached == NULL) \
  { \
    cairo_pattern_set_matrix(PATTERN(self_t), &matrix1); \
  }

#define CAP_SET(cr, cap) \
  if (cap == s_project) \
    cairo_set_line_cap(cr, CAIRO_LINE_CAP_SQUARE); \
  else if (cap == s_round || cap == s_curve) \
    cairo_set_line_cap(cr, CAIRO_LINE_CAP_ROUND); \
  else if (cap == s_square || cap == s_rect) \
    cairo_set_line_cap(cr, CAIRO_LINE_CAP_BUTT)

#define DASH_SET(cr, dash) \
  if (dash == s_onedot) \
  { \
    double dashes[] = {50.0, 10.0, 10.0, 10.0}; \
    int    ndash  = sizeof (dashes)/sizeof(dashes[0]); \
    double offset = -50.0; \
    cairo_set_dash(cr, dashes, ndash, offset); \
  } \
  else \
  { \
    cairo_set_dash(cr, NULL, 0, 0.0); \
  }

#define PATH_OUT(cr, attr, place, sw, cap, dash, pen, cfunc) \
{ \
  VALUE p = ATTR(attr, pen); \
  if (!NIL_P(p)) \
  { \
    CAP_SET(cr, cap); \
    DASH_SET(cr, dash); \
    cairo_set_line_width(cr, sw); \
    if (rb_obj_is_kind_of(p, cColor)) \
    { \
      lumi_color *color; \
      Data_Get_Struct(p, lumi_color, color); \
      cairo_set_source_rgba(cr, color->r / 255., color->g / 255., color->b / 255., color->a / 255.); \
      cfunc(cr); \
    } \
    else \
    { \
      if (!rb_obj_is_kind_of(p, cPattern)) \
        ATTRSET(attr, pen, p = lumi_pattern_new(cPattern, p, Qnil, Qnil)); \
      cairo_matrix_t matrix1, matrix2; \
      lumi_pattern *pattern; \
      Data_Get_Struct(p, lumi_pattern, pattern); \
      PATTERN_SCALE(pattern, (place), sw); \
      cairo_set_source(cr, PATTERN(pattern)); \
      cfunc(cr); \
      PATTERN_RESET(pattern); \
    } \
  } \
}

//
// Lumi::Shape
//
void
lumi_shape_mark(lumi_shape *path)
{
  rb_gc_mark_maybe(path->parent);
  rb_gc_mark_maybe(path->attr);
}

static void
lumi_shape_free(lumi_shape *path)
{
  lumi_transform_release(path->st);
  if (path->line != NULL) cairo_path_destroy(path->line);
  RUBY_CRITICAL(free(path));
}

VALUE
lumi_shape_attr(int argc, VALUE *argv, int syms, ...)
{
  int i;
  va_list args;
  VALUE hsh = Qnil;
  va_start(args, syms);
  if (argc < 1) hsh = rb_hash_new();
  else if (rb_obj_is_kind_of(argv[argc - 1], rb_cHash)) hsh = argv[argc - 1];
  for (i = 0; i < syms; i++)
  {
    ID sym = va_arg(args, ID);
    if (argc > i && !rb_obj_is_kind_of(argv[i], rb_cHash))
      hsh = lumi_hash_set(hsh, sym, argv[i]);
  }
  va_end(args);
  return hsh;
}

unsigned char
lumi_shape_check(cairo_t *cr, lumi_place *place)
{
  double ox1 = place->ix, oy1 = place->iy, ox2 = place->ix + place->iw, oy2 = place->iy + place->ih;
  double cx1, cy1, cx2, cy2;
  cairo_clip_extents(cr, &cx1, &cy1, &cx2, &cy2);
  if (place->iw < 0) ox1 = place->ix - (ox2 = -place->iw);
  if (place->ih < 0) oy1 = place->iy - (oy2 = -place->ih);
  if (cy2 - cy1 == 1.0 && cx2 - cx1 == 1.0) return 1;
  if ((ox1 < cx1 && ox2 < cx1) || (oy1 < cy1 && oy2 < cy1) ||
      (ox1 > cx2 && ox2 > cx2) || (oy1 > cy2 && oy2 > cy2)) return 0;
  return 1;
}

void
lumi_shape_sketch(cairo_t *cr, ID name, lumi_place *place, lumi_transform *st, VALUE attr, cairo_path_t* line, unsigned char draw)
{
  double sw = ATTR2(dbl, attr, strokewidth, 1.);
  if (name == s_oval && place->w > 0 && place->h > 0)
  {
    lumi_apply_transformation(cr, st, place, 1);
    if (!lumi_shape_check(cr, place))
      return lumi_undo_transformation(cr, st, place, 1);
    if (draw) cairo_new_path(cr);
    cairo_translate(cr, (place->x * 1.) + (place->w / 2.), (place->y * 1.) + (place->h / 2.));
    cairo_scale(cr, place->w / 2., place->h / 2.);
    cairo_arc(cr, 0., 0., 1., 0., LUMI_PIM2);
    cairo_close_path(cr);
    lumi_undo_transformation(cr, st, place, 1);
  }
  else if (name == s_arc && place->w > 0 && place->h > 0)
  {
    double a1 = ATTR2(dbl, attr, angle1, 0.);
    double a2 = ATTR2(dbl, attr, angle2, 0.);
    lumi_apply_transformation(cr, st, place, 0);
    if (!lumi_shape_check(cr, place))
      return lumi_undo_transformation(cr, st, place, 0);
    if (draw) cairo_new_path(cr);
    lumi_cairo_arc(cr, SWPOS(place->x), SWPOS(place->y),
      place->w * 1., place->h * 1., a1, a2);
    lumi_undo_transformation(cr, st, place, 0);
  }
  else if (name == s_rect && place->w > 0 && place->h > 0)
  {
    double cv = ATTR2(dbl, attr, curve, 0.);
    lumi_apply_transformation(cr, st, place, 0);
    if (!lumi_shape_check(cr, place))
      return lumi_undo_transformation(cr, st, place, 0);
    if (draw) cairo_new_path(cr);
    lumi_cairo_rect(cr, SWPOS(place->x), SWPOS(place->y), place->w * 1., place->h * 1., cv);
    lumi_undo_transformation(cr, st, place, 0);
  }
  else if (name == s_line)
  {
    lumi_apply_transformation(cr, st, place, 0);
    if (!lumi_shape_check(cr, place))
      return lumi_undo_transformation(cr, st, place, 0);
    cairo_move_to(cr, SWPOS(place->ix), SWPOS(place->iy));
    cairo_line_to(cr, SWPOS(place->ix + place->iw), SWPOS(place->iy + place->ih));
    lumi_undo_transformation(cr, st, place, 0);
  }
  else if (name == s_arrow && place->w > 0)
  {
    double h, tip, x;
    x = place->x + (place->w / 2.);
    h = place->w * 0.8;
    place->h = ROUND(h);
    tip = place->w * 0.42;

    lumi_apply_transformation(cr, st, place, 0);
    if (!lumi_shape_check(cr, place))
      return lumi_undo_transformation(cr, st, place, 0);
    if (draw) cairo_new_path(cr);
    cairo_move_to(cr, SWPOS(x), SWPOS(place->y));
    cairo_rel_line_to(cr, -tip, +(h*0.5));
    cairo_rel_line_to(cr, 0, -(h*0.25));
    cairo_rel_line_to(cr, -(place->w-tip), 0);
    cairo_rel_line_to(cr, 0, -(h*0.5));
    cairo_rel_line_to(cr, +(place->w-tip), 0);
    cairo_rel_line_to(cr, 0, -(h*0.25));
    cairo_close_path(cr);
    lumi_undo_transformation(cr, st, place, 0);
  }
  else if (name == s_star)
  {
    int i, points;
    double outer, inner, angle, r;
    points = ATTR2(int, attr, points, 10);
    outer = ATTR2(dbl, attr, outer, 100.);
    inner = ATTR2(dbl, attr, inner, 50.);

    if (outer > 0)
    {
      place->w = place->h = ROUND(outer);
      lumi_apply_transformation(cr, st, place, 0);
      if (!lumi_shape_check(cr, place))
        return lumi_undo_transformation(cr, st, place, 0);
      if (draw) cairo_new_path(cr);
      cairo_move_to(cr, place->x * 1., (place->y * 1.) + outer);
      for (i = 1; i <= points * 2; i++) {
        angle = (i * LUMI_PI) / (points * 1.);
        r = (i % 2 == 0 ? outer : inner);
        cairo_line_to(cr, place->x + r * sin(angle), place->y + r * cos(angle));
      }
      cairo_close_path(cr);
      lumi_undo_transformation(cr, st, place, 0);
    }
  }
  else if (name == s_shape)
  {
    lumi_apply_transformation(cr, st, place, 0);
    if (!lumi_shape_check(cr, place))
      return lumi_undo_transformation(cr, st, place, 0);
    cairo_translate(cr, SWPOS(place->x), SWPOS(place->y));
    cairo_append_path(cr, line);
    lumi_undo_transformation(cr, st, place, 0);
  }
  else return;

  if (draw)
  {
    ID cap = s_rect;
    if (!NIL_P(ATTR(attr, cap))) cap = SYM2ID(ATTR(attr, cap));
    ID dash = s_nodot;
    if (!NIL_P(ATTR(attr, dash))) dash = SYM2ID(ATTR(attr, dash));
    PATH_OUT(cr, attr, *place, sw, cap, dash, fill, cairo_fill_preserve);
    PATH_OUT(cr, attr, *place, sw, cap, dash, stroke, cairo_stroke);
  }
}

VALUE
lumi_shape_new(VALUE parent, ID name, VALUE attr, lumi_transform *st, cairo_path_t *line)
{
  lumi_shape *path;
  lumi_canvas *canvas;
  VALUE obj = lumi_shape_alloc(cShape);
  Data_Get_Struct(obj, lumi_shape, path);
  Data_Get_Struct(parent, lumi_canvas, canvas);
  path->parent = parent;
  path->attr = attr;
  path->name = name;
  path->st = lumi_transform_touch(st);
  path->line = line;
  COPY_PENS(path->attr, canvas->attr);
  return obj;
}

VALUE
lumi_shape_alloc(VALUE klass)
{
  VALUE obj;
  lumi_shape *shape = SHOE_ALLOC(lumi_shape);
  SHOE_MEMZERO(shape, lumi_shape, 1);
  obj = Data_Wrap_Struct(klass, lumi_shape_mark, lumi_shape_free, shape);
  shape->attr = Qnil;
  shape->parent = Qnil;
  shape->line = NULL;
  return obj;
}

VALUE
lumi_shape_draw(VALUE self, VALUE c, VALUE actual)
{
  lumi_place place;
  lumi_canvas *canvas;
  GET_STRUCT(shape, self_t);
  if (ATTR(self_t->attr, hidden) == Qtrue) return self;
  Data_Get_Struct(self_t->parent, lumi_canvas, canvas);
  lumi_place_exact(&place, self_t->attr, CPX(canvas), CPY(canvas));

  if (RTEST(actual))
    lumi_shape_sketch(CCR(canvas), self_t->name, &place, self_t->st, self_t->attr, self_t->line, 1);

  self_t->place = place;
  return self;
}

VALUE
lumi_shape_motion(VALUE self, int x, int y, char *touch)
{
  char h = 0;
  VALUE click;
  GET_STRUCT(shape, self_t);

  click = ATTR(self_t->attr, click);

  if (IS_INSIDE(self_t, x, y))
  {
    cairo_bool_t in_shape;
    cairo_t *cr = cairo_create(cairo_image_surface_create(CAIRO_FORMAT_ARGB32, 1, 1));
    // if (self_t->line != NULL)
    // {
    //   cairo_new_path(cr);
    //   cairo_append_path(cr, self_t->line);
    // }
    // else
      lumi_shape_sketch(cr, self_t->name, &self_t->place, self_t->st, self_t->attr, self_t->line, 0);
    in_shape = cairo_in_fill(cr, x, y);
    cairo_destroy(cr);

    if (in_shape)
    {
      if (!NIL_P(click))
      {
        lumi_canvas *canvas;
        Data_Get_Struct(self_t->parent, lumi_canvas, canvas);
        lumi_app_cursor(canvas->app, s_link);
      }
      h = 1;
    }
  }

  CHECK_HOVER(self_t, h, touch);

  return h ? click : Qnil;
  return Qnil;
}

VALUE
lumi_shape_send_click(VALUE self, int button, int x, int y)
{
  VALUE v = Qnil;

  if (button == 1)
  {
    GET_STRUCT(shape, self_t);
    v = lumi_shape_motion(self, x, y, NULL);
    if (self_t->hover & HOVER_MOTION)
      self_t->hover = HOVER_MOTION | HOVER_CLICK;
  }

  return v;
}

void
lumi_shape_send_release(VALUE self, int button, int x, int y)
{
  GET_STRUCT(shape, self_t);
  if (button == 1 && (self_t->hover & HOVER_CLICK))
  {
    VALUE proc = ATTR(self_t->attr, release);
    self_t->hover ^= HOVER_CLICK;
    if (!NIL_P(proc))
      lumi_safe_block(self, proc, rb_ary_new3(1, self));
  }
}

//
// Lumi::Image
//
void
lumi_image_mark(lumi_image *image)
{
  rb_gc_mark_maybe(image->path);
  rb_gc_mark_maybe(image->parent);
  rb_gc_mark_maybe(image->attr);
}

static void
lumi_image_free(lumi_image *image)
{
  if (image->type == LUMI_CACHE_MEM)
  {
    if (image->cr != NULL)     cairo_destroy(image->cr);
    if (image->cached != NULL) cairo_surface_destroy(image->cached->surface);
    SHOE_FREE(image->cached);
  }
  lumi_transform_release(image->st);
  RUBY_CRITICAL(SHOE_FREE(image));
}

VALUE
lumi_image_new(VALUE klass, VALUE path, VALUE attr, VALUE parent, lumi_transform *st)
{
  VALUE obj = Qnil;
  lumi_image *image;
  lumi_basic *basic;
  Data_Get_Struct(parent, lumi_basic, basic);

  obj = lumi_image_alloc(klass);
  Data_Get_Struct(obj, lumi_image, image);

  image->path = Qnil;
  image->st = lumi_transform_touch(st);
  image->attr = attr;
  image->parent = lumi_find_canvas(parent);
  COPY_PENS(image->attr, basic->attr);

  if (rb_obj_is_kind_of(path, cImage))
  {
    lumi_image *image2;
    Data_Get_Struct(path, lumi_image, image2);
    image->cached = image2->cached;
    image->type = LUMI_CACHE_ALIAS;
  }
  else if (!NIL_P(path))
  {
    path = lumi_native_to_s(path);
    image->path = path;
    image->cached = lumi_load_image(image->parent, path);
    image->type = LUMI_CACHE_FILE;
  }
  else
  {
    lumi_canvas *canvas;
    Data_Get_Struct(image->parent, lumi_canvas, canvas);
    int w = ATTR2(int, attr, width, canvas->width);
    int h = ATTR2(int, attr, height, canvas->height);
    VALUE block = ATTR(attr, draw);
    image->cached = lumi_cached_image_new(w, h, NULL);
    image->cr = cairo_create(image->cached->surface);
    image->type = LUMI_CACHE_MEM;
    if (!NIL_P(block)) DRAW(obj, canvas->app, rb_funcall(block, s_call, 0));
  }

  return obj;
}

VALUE
lumi_image_alloc(VALUE klass)
{
  VALUE obj;
  lumi_image *image = SHOE_ALLOC(lumi_image);
  SHOE_MEMZERO(image, lumi_image, 1);
  obj = Data_Wrap_Struct(klass, lumi_image_mark, lumi_image_free, image);
  image->path = Qnil;
  image->st = NULL;
  image->attr = Qnil;
  image->parent = Qnil;
  image->type = LUMI_CACHE_MEM;
  return obj;
}

VALUE
lumi_image_get_full_width(VALUE self)
{
  GET_STRUCT(image, image);
  return INT2NUM(image->cached->width);
}

VALUE
lumi_image_get_full_height(VALUE self)
{
  GET_STRUCT(image, image);
  return INT2NUM(image->cached->height);
}

void
lumi_image_ensure_dup(lumi_image *image)
{
  if (image->type == LUMI_CACHE_MEM)
    return;
  lumi_cached_image *cached = lumi_cached_image_new(image->cached->width, image->cached->height, NULL);
  image->cr = cairo_create(cached->surface);
  cairo_set_source_surface(image->cr, image->cached->surface, 0, 0);
  cairo_paint(image->cr);

  image->cached = cached;
  image->type = LUMI_CACHE_MEM;
}

unsigned char *
lumi_image_surface_get_pixel(lumi_cached_image *cached, int x, int y)
{
  if (x >= 0 && y >= 0 && x < cached->width && y < cached->height)
  {
    unsigned char* pixels = cairo_image_surface_get_data(cached->surface);
    if (cairo_image_surface_get_format(cached->surface) == CAIRO_FORMAT_ARGB32)
      return pixels + (y * (4 * cached->width)) + (4 * x);
  }
  return NULL;
}

VALUE
lumi_image_get_pixel(VALUE self, VALUE _x, VALUE _y)
{
  VALUE color = Qnil;
  int x = NUM2INT(_x), y = NUM2INT(_y);
  GET_STRUCT(image, image);
  unsigned char *pixels = lumi_image_surface_get_pixel(image->cached, x, y);
  if (pixels != NULL)
    color = lumi_color_new(pixels[2], pixels[1], pixels[0], pixels[3]);
  return color;
}

VALUE
lumi_image_set_pixel(VALUE self, VALUE _x, VALUE _y, VALUE col)
{
  int x = NUM2INT(_x), y = NUM2INT(_y);
  GET_STRUCT(image, image);
  lumi_image_ensure_dup(image);
  unsigned char *pixels = lumi_image_surface_get_pixel(image->cached, x, y);
  if (pixels != NULL)
  {
    if (TYPE(col) == T_STRING)
      col = lumi_color_parse(cColor, col);
    if (rb_obj_is_kind_of(col, cColor))
    {
      lumi_color *color;
      Data_Get_Struct(col, lumi_color, color);
      pixels[0] = color->b;
      pixels[1] = color->g;
      pixels[2] = color->r;
      pixels[3] = color->a;
    }
  }
  return self;
}

VALUE
lumi_image_get_path(VALUE self)
{
  GET_STRUCT(image, image);
  return image->path;
}

VALUE
lumi_image_set_path(VALUE self, VALUE path)
{
  GET_STRUCT(image, image);
  image->path = path;
  image->cached = lumi_load_image(image->parent, path);
  image->type = LUMI_CACHE_FILE;
  lumi_canvas_repaint_all(image->parent);
  return path;
}

static void
lumi_image_draw_surface(cairo_t *cr, lumi_image *self_t, lumi_place *place, cairo_surface_t *surf, int imw, int imh)
{
  lumi_apply_transformation(cr, self_t->st, place, 0);
  cairo_translate(cr, place->ix + place->dx, place->iy + place->dy);
  if (place->iw != imw || place->ih != imh)
    cairo_scale(cr, (place->iw * 1.) / imw, (place->ih * 1.) / imh);
  cairo_set_source_surface(cr, surf, 0., 0.);
  cairo_paint(cr);
  lumi_undo_transformation(cr, self_t->st, place, 0);
  self_t->place = *place;
}

#define LUMI_IMAGE_PLACE(type, imw, imh, surf) \
  SETUP(lumi_##type, (REL_CANVAS | REL_SCALE), imw, imh); \
  VALUE ck = rb_obj_class(c); \
  if (RTEST(actual)) \
    lumi_image_draw_surface(CCR(canvas), self_t, &place, surf, imw, imh); \
  FINISH(); \
  return self;

VALUE
lumi_image_draw(VALUE self, VALUE c, VALUE actual)
{
  LUMI_IMAGE_PLACE(image, self_t->cached->width, self_t->cached->height, self_t->cached->surface);
}

void
lumi_image_image(VALUE parent, VALUE path, VALUE attr)
{
  lumi_image *pi;
  lumi_place place;
  Data_Get_Struct(parent, lumi_image, pi);
  VALUE self = lumi_image_new(cImage, path, attr, parent, pi->st);
  GET_STRUCT(image, image);
  lumi_image_ensure_dup(pi);
  lumi_place_exact(&place, image->attr, 0, 0);
  if (place.iw < 1) place.w = place.iw = image->cached->width;
  if (place.ih < 1) place.h = place.ih = image->cached->height;
  lumi_image_draw_surface(pi->cr, image, &place, image->cached->surface, image->cached->width, image->cached->height);
}

VALUE
lumi_image_size(VALUE self)
{
  GET_STRUCT(image, self_t);
  return rb_ary_new3(2, INT2NUM(self_t->cached->width), INT2NUM(self_t->cached->height));
}

VALUE
lumi_image_motion(VALUE self, int x, int y, char *touch)
{
  char h = 0;
  VALUE click;
  GET_STRUCT(image, self_t);

  click = ATTR(self_t->attr, click);
  if (self_t->cached == NULL) return Qnil;

  if (IS_INSIDE(self_t, x, y))
  {
    if (!NIL_P(click))
    {
      lumi_canvas *canvas;
      Data_Get_Struct(self_t->parent, lumi_canvas, canvas);
      lumi_app_cursor(canvas->app, s_link);
    }
    h = 1;
  }

  CHECK_HOVER(self_t, h, touch);

  return h ? click : Qnil;
}

VALUE
lumi_image_send_click(VALUE self, int button, int x, int y)
{
  VALUE v = Qnil;

  if (button == 1)
  {
    GET_STRUCT(image, self_t);
    v = lumi_image_motion(self, x, y, NULL);
    if (self_t->hover & HOVER_MOTION)
      self_t->hover = HOVER_MOTION | HOVER_CLICK;
  }

  return v;
}

void
lumi_image_send_release(VALUE self, int button, int x, int y)
{
  GET_STRUCT(image, self_t);
  if (button == 1 && (self_t->hover & HOVER_CLICK))
  {
    VALUE proc = ATTR(self_t->attr, release);
    self_t->hover ^= HOVER_CLICK;
    if (!NIL_P(proc))
      lumi_safe_block(self, proc, rb_ary_new3(1, self));
  }
}

//
// Lumi::Effect
//
void
lumi_effect_mark(lumi_effect *fx)
{
  rb_gc_mark_maybe(fx->parent);
  rb_gc_mark_maybe(fx->attr);
}

static void
lumi_effect_free(lumi_effect *fx)
{
  RUBY_CRITICAL(free(fx));
}

lumi_effect_filter
lumi_effect_for_type(ID name)
{
  if (name == s_blur)
    return &lumi_gaussian_blur_filter;
  else if (name == s_shadow)
    return &lumi_shadow_filter;
  else if (name == s_glow)
    return &lumi_glow_filter;
  return NULL;
}

VALUE
lumi_effect_new(ID name, VALUE attr, VALUE parent)
{
  lumi_effect *fx;
  lumi_canvas *canvas;
  VALUE obj = lumi_effect_alloc(cEffect);
  Data_Get_Struct(obj, lumi_effect, fx);
  Data_Get_Struct(parent, lumi_canvas, canvas);
  fx->parent = parent;
  fx->attr = attr;
  fx->filter = lumi_effect_for_type(name);
  return obj;
}

VALUE
lumi_effect_alloc(VALUE klass)
{
  VALUE obj;
  lumi_effect *fx = SHOE_ALLOC(lumi_effect);
  SHOE_MEMZERO(fx, lumi_effect, 1);
  obj = Data_Wrap_Struct(klass, lumi_effect_mark, lumi_effect_free, fx);
  fx->attr = Qnil;
  fx->parent = Qnil;
  return obj;
}

VALUE
lumi_effect_draw(VALUE self, VALUE c, VALUE actual)
{
  SETUP(lumi_effect, REL_TILE, canvas->width, canvas->height);

  if (RTEST(actual) && self_t->filter != NULL)
    self_t->filter(CCR(canvas), self_t->attr, &self_t->place);

  self_t->place = place;
  return self;
}

//
// Lumi::Pattern
//
void
lumi_pattern_mark(lumi_pattern *pattern)
{
  rb_gc_mark_maybe(pattern->source);
  rb_gc_mark_maybe(pattern->parent);
  rb_gc_mark_maybe(pattern->attr);
}

static void
lumi_pattern_free(lumi_pattern *pattern)
{
  if (pattern->pattern != NULL)
    cairo_pattern_destroy(pattern->pattern);
  RUBY_CRITICAL(free(pattern));
}

static void
lumi_pattern_gradient(lumi_pattern *pattern, VALUE r1, VALUE r2, VALUE attr)
{
  double angle = ATTR2(dbl, attr, angle, 0.);
  double rads = angle * LUMI_RAD2PI;
  double dx = sin(rads);
  double dy = cos(rads);
  double edge = (fabs(dx) + fabs(dy)) * 0.5;

  if (rb_obj_is_kind_of(r1, rb_cString))
    r1 = lumi_color_parse(cColor, r1);
  if (rb_obj_is_kind_of(r2, rb_cString))
    r2 = lumi_color_parse(cColor, r2);

  VALUE radius = ATTR(attr, radius);
  if (!NIL_P(radius))
  {
    double r = 0.001;
    if (rb_obj_is_kind_of(r, rb_cFloat)) r = NUM2DBL(radius);
    pattern->pattern = cairo_pattern_create_radial(0.5, 0.5, r, edge / 2., edge / 2., edge / 2.);
  }
  else
  {
    pattern->pattern = cairo_pattern_create_linear(0.5 + (-dx * edge), 0.5 + (-dy * edge),
      0.5 + (dx * edge), 0.5 + (dy * edge));
  }
  lumi_color_grad_stop(pattern->pattern, 0.0, r1);
  lumi_color_grad_stop(pattern->pattern, 1.0, r2);
}

VALUE
lumi_pattern_set_fill(VALUE self, VALUE source)
{
  lumi_pattern *pattern;
  Data_Get_Struct(self, lumi_pattern, pattern);

  if (pattern->pattern != NULL)
    cairo_pattern_destroy(pattern->pattern);
  pattern->pattern = NULL;

  if (rb_obj_is_kind_of(source, rb_cRange))
  {
    VALUE r1 = rb_funcall(source, s_begin, 0);
    VALUE r2 = rb_funcall(source, s_end, 0);
    lumi_pattern_gradient(pattern, r1, r2, pattern->attr);
  }
  else
  {
    if (!rb_obj_is_kind_of(source, cColor))
    {
      VALUE rgb = lumi_color_parse(cColor, source);
      if (!NIL_P(rgb)) source = rgb;
    }

    if (rb_obj_is_kind_of(source, cColor))
    {
      pattern->pattern = lumi_color_pattern(source);
    }
    else
    {
      pattern->cached = lumi_load_image(pattern->parent, source);
      if (pattern->cached != NULL && pattern->cached->pattern == NULL)
        pattern->cached->pattern = cairo_pattern_create_for_surface(pattern->cached->surface);
    }
    cairo_pattern_set_extend(PATTERN(pattern), CAIRO_EXTEND_REPEAT);
  }

  pattern->source = source;
  return source;
}

VALUE
lumi_pattern_get_fill(VALUE self)
{
  lumi_pattern *pattern;
  Data_Get_Struct(self, lumi_pattern, pattern);
  return pattern->source;
}

VALUE
lumi_pattern_self(VALUE self)
{
  return self;
}

VALUE
lumi_pattern_args(int argc, VALUE *argv, VALUE self)
{
  VALUE source, attr;
  rb_scan_args(argc, argv, "11", &source, &attr);
  CHECK_HASH(attr);
  return lumi_pattern_new(cPattern, source, attr, Qnil);
}

VALUE
lumi_pattern_new(VALUE klass, VALUE source, VALUE attr, VALUE parent)
{
  lumi_pattern *pattern;
  VALUE obj = lumi_pattern_alloc(klass);
  Data_Get_Struct(obj, lumi_pattern, pattern);
  pattern->source = Qnil;
  pattern->attr = attr;
  pattern->parent = parent;
  lumi_pattern_set_fill(obj, source);
  return obj;
}

VALUE
lumi_pattern_method(VALUE klass, VALUE source)
{
  return lumi_pattern_new(cPattern, source, Qnil, Qnil);
}

VALUE
lumi_pattern_alloc(VALUE klass)
{
  VALUE obj;
  lumi_pattern *pattern = SHOE_ALLOC(lumi_pattern);
  SHOE_MEMZERO(pattern, lumi_pattern, 1);
  obj = Data_Wrap_Struct(klass, lumi_pattern_mark, lumi_pattern_free, pattern);
  pattern->source = Qnil;
  pattern->attr = Qnil;
  pattern->parent = Qnil;
  return obj;
}

VALUE
lumi_background_draw(VALUE self, VALUE c, VALUE actual)
{
  cairo_matrix_t matrix1, matrix2;
  double r = 0., sw = 1.;
  SETUP(lumi_pattern, REL_TILE, PATTERN_DIM(self_t, width), PATTERN_DIM(self_t, height));
  r = ATTR2(dbl, self_t->attr, curve, 0.);

  if (RTEST(actual))
  {
    cairo_t *cr = CCR(canvas);
    cairo_save(cr);
    cairo_translate(cr, place.ix + place.dx, place.iy + place.dy);
    PATTERN_SCALE(self_t, place, sw);
    cairo_new_path(cr);
    lumi_cairo_rect(cr, 0, 0, place.iw, place.ih, r);
    cairo_set_source(cr, PATTERN(self_t));
    cairo_fill(cr);
    cairo_restore(cr);
    PATTERN_RESET(self_t);
  }

  self_t->place = place;
  INFO("BACKGROUND: (%d, %d), (%d, %d)\n", place.x, place.y, place.w, place.h);
  return self;
}

VALUE
lumi_border_draw(VALUE self, VALUE c, VALUE actual)
{
  cairo_matrix_t matrix1, matrix2;
  ID cap = s_rect;
  ID dash = s_nodot;
  double r = 0., sw = 1.;
  SETUP(lumi_pattern, REL_TILE, PATTERN_DIM(self_t, width), PATTERN_DIM(self_t, height));
  r = ATTR2(dbl, self_t->attr, curve, 0.);
  sw = ATTR2(dbl, self_t->attr, strokewidth, 1.);
  if (!NIL_P(ATTR(self_t->attr, cap))) cap = SYM2ID(ATTR(self_t->attr, cap));
  if (!NIL_P(ATTR(self_t->attr, dash))) dash = SYM2ID(ATTR(self_t->attr, dash));

  place.iw -= ROUND(sw);
  place.ih -= ROUND(sw);
  place.ix += ROUND(sw / 2.);
  place.iy += ROUND(sw / 2.);

  if (RTEST(actual))
  {
    cairo_t *cr = CCR(canvas);
    cairo_save(cr);
    cairo_translate(cr, place.ix + place.dx, place.iy + place.dy);
    PATTERN_SCALE(self_t, place, sw);
    cairo_set_source(cr, PATTERN(self_t));
    cairo_new_path(cr);
    lumi_cairo_rect(cr, 0, 0, place.iw, place.ih, r);
    cairo_set_antialias(cr, CAIRO_ANTIALIAS_NONE);
    CAP_SET(cr, cap);
    DASH_SET(cr, dash);
    cairo_set_line_width(cr, sw);
    cairo_stroke(cr);
    cairo_restore(cr);
    PATTERN_RESET(self_t);
  }

  self_t->place = place;
  INFO("BORDER: (%d, %d), (%d, %d)\n", place.x, place.y, place.w, place.h);
  return self;
}

VALUE
lumi_subpattern_new(VALUE klass, VALUE pat, VALUE parent)
{
  lumi_pattern *back, *pattern;
  VALUE obj = lumi_pattern_alloc(klass);
  Data_Get_Struct(obj, lumi_pattern, back);
  Data_Get_Struct(pat, lumi_pattern, pattern);
  back->source = pattern->source;
  back->cached = pattern->cached;
  back->pattern = pattern->pattern;
  if (back->pattern != NULL) cairo_pattern_reference(back->pattern);
  back->attr = pattern->attr;
  back->parent = parent;
  return obj;
}

//
// Lumi::Color
//
void
lumi_color_mark(lumi_color *color)
{
}

static void
lumi_color_free(lumi_color *color)
{
  RUBY_CRITICAL(free(color));
}

VALUE
lumi_color_new(int r, int g, int b, int a)
{
  lumi_color *color;
  VALUE obj = lumi_color_alloc(cColor);
  Data_Get_Struct(obj, lumi_color, color);
  color->r = r;
  color->g = g;
  color->b = b;
  color->a = a;
  return obj;
}

VALUE
lumi_color_alloc(VALUE klass)
{
  VALUE obj;
  lumi_color *color = SHOE_ALLOC(lumi_color);
  SHOE_MEMZERO(color, lumi_color, 1);
  obj = Data_Wrap_Struct(klass, lumi_color_mark, lumi_color_free, color);
  color->a = LUMI_COLOR_OPAQUE;
  color->on = TRUE;
  return obj;
}

VALUE
lumi_color_rgb(int argc, VALUE *argv, VALUE self)
{
  int a;
  VALUE _r, _g, _b, _a;
  rb_scan_args(argc, argv, "31", &_r, &_g, &_b, &_a);

  a = LUMI_COLOR_OPAQUE;
  if (!NIL_P(_a)) a = NUM2RGBINT(_a);
  return lumi_color_new(NUM2RGBINT(_r), NUM2RGBINT(_g), NUM2RGBINT(_b), a);
}

VALUE
lumi_color_gradient(int argc, VALUE *argv, VALUE self)
{
  lumi_pattern *pattern;
  VALUE obj, r1, r2;
  VALUE attr = Qnil;
  rb_scan_args(argc, argv, "21", &r1, &r2, &attr);
  CHECK_HASH(attr);

  obj = lumi_pattern_alloc(cPattern);
  Data_Get_Struct(obj, lumi_pattern, pattern);
  pattern->source = Qnil;
  lumi_pattern_gradient(pattern, r1, r2, attr);
  return obj;
}

VALUE
lumi_color_gray(int argc, VALUE *argv, VALUE self)
{
  VALUE _g, _a;
  int g, a;
  rb_scan_args(argc, argv, "02", &_g, &_a);

  a = LUMI_COLOR_OPAQUE;
  g = 128;
  if (!NIL_P(_g)) g = NUM2RGBINT(_g);
  if (!NIL_P(_a)) a = NUM2RGBINT(_a);
  return lumi_color_new(g, g, g, a);
}

cairo_pattern_t *
lumi_color_pattern(VALUE self)
{
  GET_STRUCT(color, color);
  if (color->a == 255)
    return cairo_pattern_create_rgb(color->r / 255., color->g / 255., color->b / 255.);
  else
    return cairo_pattern_create_rgba(color->r / 255., color->g / 255., color->b / 255., color->a / 255.);
}

void
lumi_color_grad_stop(cairo_pattern_t *pattern, double stop, VALUE self)
{
  GET_STRUCT(color, color);
  if (color->a == 255)
    return cairo_pattern_add_color_stop_rgb(pattern, stop, color->r / 255., color->g / 255., color->b / 255.);
  else
    return cairo_pattern_add_color_stop_rgba(pattern, stop, color->r / 255., color->g / 255., color->b / 255., color->a / 255.);
}

VALUE
lumi_color_args(int argc, VALUE *argv, VALUE self)
{
  VALUE _r, _g, _b, _a, _color;
  argc = rb_scan_args(argc, argv, "13", &_r, &_g, &_b, &_a);

  if (argc == 1 && rb_obj_is_kind_of(_r, cColor))
    _color = _r;
  else if (argc == 1 && rb_obj_is_kind_of(_r, rb_cString))
    _color = lumi_color_parse(cColor, _r);
  else if (argc == 1 || argc == 2)
    _color = lumi_color_gray(argc, argv, cColor);
  else
    _color = lumi_color_rgb(argc, argv, cColor);

  return _color;
}

#define NEW_COLOR(v, o) \
  lumi_color *v; \
  VALUE o = lumi_color_alloc(cColor); \
  Data_Get_Struct(o, lumi_color, v)

VALUE
lumi_color_parse(VALUE self, VALUE source)
{
  VALUE reg;

  reg = rb_funcall(source, s_match, 1, reHEX3_SOURCE);
  if (!NIL_P(reg))
  {
    NEW_COLOR(color, obj);
    color->r = NUM2INT(rb_str2inum(rb_reg_nth_match(1, reg), 16)) * 17;
    color->g = NUM2INT(rb_str2inum(rb_reg_nth_match(2, reg), 16)) * 17;
    color->b = NUM2INT(rb_str2inum(rb_reg_nth_match(3, reg), 16)) * 17;
    return obj;
  }

  reg = rb_funcall(source, s_match, 1, reHEX_SOURCE);
  if (!NIL_P(reg))
  {
    NEW_COLOR(color, obj);
    color->r = NUM2INT(rb_str2inum(rb_reg_nth_match(1, reg), 16));
    color->g = NUM2INT(rb_str2inum(rb_reg_nth_match(2, reg), 16));
    color->b = NUM2INT(rb_str2inum(rb_reg_nth_match(3, reg), 16));
    return obj;
  }

  reg = rb_funcall(source, s_match, 1, reRGB_SOURCE);
  if (!NIL_P(reg))
  {
    NEW_COLOR(color, obj);
    color->r = NUM2INT(rb_Integer(rb_reg_nth_match(1, reg)));
    color->g = NUM2INT(rb_Integer(rb_reg_nth_match(2, reg)));
    color->b = NUM2INT(rb_Integer(rb_reg_nth_match(3, reg)));
    return obj;
  }

  reg = rb_funcall(source, s_match, 1, reRGBA_SOURCE);
  if (!NIL_P(reg))
  {
    NEW_COLOR(color, obj);
    color->r = NUM2INT(rb_Integer(rb_reg_nth_match(1, reg)));
    color->g = NUM2INT(rb_Integer(rb_reg_nth_match(2, reg)));
    color->b = NUM2INT(rb_Integer(rb_reg_nth_match(3, reg)));
    color->a = NUM2INT(rb_Integer(rb_reg_nth_match(4, reg)));
    return obj;
  }

  reg = rb_funcall(source, s_match, 1, reGRAY_SOURCE);
  if (!NIL_P(reg))
  {
    NEW_COLOR(color, obj);
    color->r = color->g = color->b = NUM2INT(rb_Integer(rb_reg_nth_match(1, reg)));
    return obj;
  }

  reg = rb_funcall(source, s_match, 1, reGRAYA_SOURCE);
  if (!NIL_P(reg))
  {
    NEW_COLOR(color, obj);
    color->r = color->g = color->b = NUM2INT(rb_Integer(rb_reg_nth_match(1, reg)));
    color->a = NUM2INT(rb_Integer(rb_reg_nth_match(2, reg)));
    return obj;
  }

  return Qnil;
}

VALUE
lumi_color_spaceship(VALUE self, VALUE c2)
{
  int v1, v2;
  lumi_color *color2;
  GET_STRUCT(color, color);
  if (!rb_obj_is_kind_of(c2, cColor)) return Qnil;
  Data_Get_Struct(c2, lumi_color, color2);
  if ((color->r == color2->r) && (color->g == color2->g) && (color->b == color2->b))
  {
    return INT2FIX(0);
  } else
  {
    v1 = color->r + color->g + color->b;
    v2 = color2->r + color2->g + color2->b;
    if (v1 > v2)  return INT2FIX(1);
    else          return INT2FIX(-1);
  }
}

VALUE
lumi_color_get_red(VALUE self)
{
  GET_STRUCT(color, color);
  return INT2NUM(color->r);
}

VALUE
lumi_color_get_green(VALUE self)
{
  GET_STRUCT(color, color);
  return INT2NUM(color->g);
}

VALUE
lumi_color_get_blue(VALUE self)
{
  GET_STRUCT(color, color);
  return INT2NUM(color->b);
}

VALUE
lumi_color_get_alpha(VALUE self)
{
  GET_STRUCT(color, color);
  return INT2NUM(color->a);
}

VALUE
lumi_color_is_black(VALUE self)
{
  GET_STRUCT(color, color);
  return (color->r + color->g + color->b == 0) ? Qtrue : Qfalse;
}

VALUE
lumi_color_is_dark(VALUE self)
{
  GET_STRUCT(color, color);
  return ((int)color->r + (int)color->g + (int)color->b < LUMI_COLOR_DARK) ? Qtrue : Qfalse;
}

VALUE
lumi_color_is_light(VALUE self)
{
  GET_STRUCT(color, color);
  return ((int)color->r + (int)color->g + (int)color->b > LUMI_COLOR_LIGHT) ? Qtrue : Qfalse;
}

VALUE
lumi_color_is_opaque(VALUE self)
{
  GET_STRUCT(color, color);
  return (color->a == LUMI_COLOR_OPAQUE) ? Qtrue : Qfalse;
}

VALUE
lumi_color_is_transparent(VALUE self)
{
  GET_STRUCT(color, color);
  return (color->a == LUMI_COLOR_TRANSPARENT) ? Qtrue : Qfalse;
}

VALUE
lumi_color_is_white(VALUE self)
{
  GET_STRUCT(color, color);
  return (color->r + color->g + color->b == 765) ? Qtrue : Qfalse;
}

VALUE
lumi_color_invert(VALUE self)
{
  GET_STRUCT(color, color);
  NEW_COLOR(color2, obj);
  color2->r = 255 - color->r; color2->g = 255 - color->g; color2->b = 255 - color->b;
  return obj;
}

VALUE
lumi_color_to_s(VALUE self)
{
  GET_STRUCT(color, color);

  VALUE ary = rb_ary_new3(4,
    INT2NUM(color->r), INT2NUM(color->g), INT2NUM(color->b),
    rb_float_new((color->a * 1.) / 255.));

  if (color->a == 255)
    return rb_funcall(rb_str_new2("rgb(%d, %d, %d)"), s_perc, 1, ary);
  else
    return rb_funcall(rb_str_new2("rgb(%d, %d, %d, %0.1f)"), s_perc, 1, ary);
}

VALUE
lumi_color_to_pattern(VALUE self)
{
  return lumi_pattern_method(cPattern, self);
}

VALUE
lumi_color_method_missing(int argc, VALUE *argv, VALUE self)
{
  VALUE alpha;
  VALUE cname = argv[0];
  VALUE c = Qnil;
  if (rb_obj_is_kind_of(cColors, rb_cHash))
    c = rb_hash_aref(cColors, cname);
  if (NIL_P(c))
  {
    self = rb_inspect(self);
    rb_raise(rb_eNoMethodError, "undefined method `%s' for %s",
      rb_id2name(SYM2ID(cname)), RSTRING_PTR(self));
  }

  rb_scan_args(argc, argv, "11", &cname, &alpha);
  if (!NIL_P(alpha))
  {
    lumi_color *color;
    Data_Get_Struct(c, lumi_color, color);
    c = lumi_color_new(color->r, color->g, color->b, NUM2RGBINT(alpha));
  }

  return c;
}

VALUE
lumi_app_method_missing(int argc, VALUE *argv, VALUE self)
{
  VALUE cname, canvas;
  GET_STRUCT(app, app);

  cname = argv[0];
  canvas = rb_ary_entry(app->nesting, RARRAY_LEN(app->nesting) - 1);
  if (!NIL_P(canvas) && rb_respond_to(canvas, SYM2ID(cname)))
    return ts_funcall2(canvas, SYM2ID(cname), argc - 1, argv + 1);
  return lumi_color_method_missing(argc, argv, self);
}

//
// Lumi::LinkUrl
//
void
lumi_link_mark(lumi_link *link)
{
}

static void
lumi_link_free(lumi_link *link)
{
  RUBY_CRITICAL(free(link));
}

VALUE
lumi_link_new(VALUE ele, int start, int end)
{
  lumi_link *link;
  VALUE obj = lumi_link_alloc(cLinkUrl);
  Data_Get_Struct(obj, lumi_link, link);
  link->ele = ele;
  link->start = start;
  link->end = end;
  return obj;
}

VALUE
lumi_link_alloc(VALUE klass)
{
  VALUE obj;
  lumi_link *link = SHOE_ALLOC(lumi_link);
  SHOE_MEMZERO(link, lumi_link, 1);
  obj = Data_Wrap_Struct(klass, lumi_link_mark, lumi_link_free, link);
  link->ele = Qnil;
  return obj;
}

static VALUE
lumi_link_at(lumi_textblock *t, VALUE self, int index, int blockhover, VALUE *clicked, char *touch)
{
  char h = 0;
  VALUE url = Qnil;
  lumi_text *self_t;

  GET_STRUCT(link, link);
  Data_Get_Struct(link->ele, lumi_text, self_t);
  if (blockhover && link->start <= index && link->end >= index)
  {
    h = 1;
    if (clicked != NULL) *clicked = link->ele;
    url = ATTR(self_t->attr, click);
  }

  self = link->ele;
  CHECK_HOVER(self_t, h, touch);
  t->hover = (t->hover & HOVER_CLICK) | h;

  return url;
}

//
// Lumi::Text
//
void
lumi_text_mark(lumi_text *text)
{
  rb_gc_mark_maybe(text->texts);
  rb_gc_mark_maybe(text->attr);
  rb_gc_mark_maybe(text->parent);
}

static void
lumi_text_free(lumi_text *text)
{
  RUBY_CRITICAL(free(text));
}

static VALUE
lumi_text_check(VALUE texts, VALUE parent)
{
  long i;
  for (i = 0; i < RARRAY_LEN(texts); i++)
  {
    VALUE ele = rb_ary_entry(texts, i);
    if (rb_obj_is_kind_of(ele, cTextClass))
    {
      lumi_text *text;
      Data_Get_Struct(ele, lumi_text, text);
      text->parent = parent;
    }
  }
  return texts;
}

VALUE
lumi_text_to_s(VALUE self)
{
  GET_STRUCT(textblock, self_t);
  return rb_funcall(self_t->texts, s_to_s, 0);
}

VALUE
lumi_text_new(VALUE klass, VALUE texts, VALUE attr)
{
  lumi_text *text;
  VALUE obj = lumi_text_alloc(klass);
  Data_Get_Struct(obj, lumi_text, text);
  text->hover = 0;
  text->texts = lumi_text_check(texts, obj);
  text->attr = attr;
  return obj;
}

VALUE
lumi_text_alloc(VALUE klass)
{
  VALUE obj;
  lumi_text *text = SHOE_ALLOC(lumi_text);
  SHOE_MEMZERO(text, lumi_text, 1);
  obj = Data_Wrap_Struct(klass, lumi_text_mark, lumi_text_free, text);
  text->texts = Qnil;
  text->attr = Qnil;
  text->parent = Qnil;
  return obj;
}

VALUE
lumi_text_parent(VALUE self)
{
  GET_STRUCT(text, text);
  return text->parent;
}

VALUE
lumi_text_children(VALUE self)
{
  GET_STRUCT(text, text);
  return text->texts;
}

//
// Lumi::TextBlock
//
void
lumi_textblock_mark(lumi_textblock *text)
{
  rb_gc_mark_maybe(text->texts);
  rb_gc_mark_maybe(text->links);
  rb_gc_mark_maybe(text->attr);
  rb_gc_mark_maybe(text->parent);
}

//
// Frees the pango attribute list.
// The `all` flag means the string has changed as well.
//
static void
lumi_textblock_uncache(lumi_textblock *text, unsigned char all)
{
  if (text->pattr != NULL)
    pango_attr_list_unref(text->pattr);
  text->pattr = NULL;
  if (all)
  {
    if (text->text != NULL)
      g_string_free(text->text, TRUE);
    text->text = NULL;
    text->cached = 0;
  }
}

static void
lumi_textblock_free(lumi_textblock *text)
{
  lumi_transform_release(text->st);
  lumi_textblock_uncache(text, TRUE);
  if (text->cursor != NULL)
    SHOE_FREE(text->cursor);
  if (text->layout != NULL)
    g_object_unref(text->layout);
  RUBY_CRITICAL(free(text));
}

VALUE
lumi_textblock_new(VALUE klass, VALUE texts, VALUE attr, VALUE parent, lumi_transform *st)
{
  lumi_canvas *canvas;
  lumi_textblock *text;
  VALUE obj = lumi_textblock_alloc(klass);
  Data_Get_Struct(obj, lumi_textblock, text);
  Data_Get_Struct(parent, lumi_canvas, canvas);
  text->texts = lumi_text_check(texts, obj);
  text->attr = attr;
  text->parent = parent;
  text->st = lumi_transform_touch(st);
  return obj;
}

VALUE
lumi_textblock_alloc(VALUE klass)
{
  VALUE obj;
  lumi_textblock *text = SHOE_ALLOC(lumi_textblock);
  SHOE_MEMZERO(text, lumi_textblock, 1);
  obj = Data_Wrap_Struct(klass, lumi_textblock_mark, lumi_textblock_free, text);
  text->texts = Qnil;
  text->links = Qnil;
  text->attr = Qnil;
  text->parent = Qnil;
  text->cursor = NULL;
  return obj;
}

VALUE
lumi_textblock_children(VALUE self)
{
  GET_STRUCT(textblock, text);
  return text->texts;
}

static void
lumi_textcursor_reset(lumi_textcursor *c)
{
  c->pos = c->hi = c->x = c->y = INT_MAX;
}

VALUE
lumi_textblock_set_cursor(VALUE self, VALUE pos)
{
  GET_STRUCT(textblock, self_t);
  if (self_t->cursor == NULL)
  {
    if (NIL_P(pos)) return Qnil;
    else            lumi_textcursor_reset(self_t->cursor = SHOE_ALLOC(lumi_textcursor));
  }

  if (NIL_P(pos)) lumi_textcursor_reset(self_t->cursor);
  else if (pos == ID2SYM(s_marker)) {
    if (self_t->cursor->hi != INT_MAX) {
      self_t->cursor->pos = min(self_t->cursor->pos, self_t->cursor->hi);
      self_t->cursor->hi = INT_MAX;
    }
  }
  else            self_t->cursor->pos = NUM2INT(pos);
  lumi_textblock_uncache(self_t, FALSE);
  lumi_canvas_repaint_all(self_t->parent);
  return pos;
}

VALUE
lumi_textblock_get_cursor(VALUE self)
{
  GET_STRUCT(textblock, self_t);
  if (self_t->cursor == NULL || self_t->cursor->pos == INT_MAX) return Qnil;
  return INT2NUM(self_t->cursor->pos);
}

VALUE
lumi_textblock_cursorx(VALUE self)
{
  GET_STRUCT(textblock, self_t);
  if (self_t->cursor == NULL || self_t->cursor->x == INT_MAX) return Qnil;
  return INT2NUM(self_t->cursor->x);
}

VALUE
lumi_textblock_cursory(VALUE self)
{
  GET_STRUCT(textblock, self_t);
  if (self_t->cursor == NULL || self_t->cursor->y == INT_MAX) return Qnil;
  return INT2NUM(self_t->cursor->y);
}

VALUE
lumi_textblock_set_marker(VALUE self, VALUE pos)
{
  GET_STRUCT(textblock, self_t);
  if (self_t->cursor == NULL)
  {
    if (NIL_P(pos)) return Qnil;
    else            lumi_textcursor_reset(self_t->cursor = SHOE_ALLOC(lumi_textcursor));
  }

  if (NIL_P(pos)) self_t->cursor->hi = INT_MAX;
  else            self_t->cursor->hi = NUM2INT(pos);
  lumi_textblock_uncache(self_t, FALSE);
  lumi_canvas_repaint_all(self_t->parent);
  return pos;
}

VALUE
lumi_textblock_get_marker(VALUE self)
{
  GET_STRUCT(textblock, self_t);
  if (self_t->cursor == NULL || self_t->cursor->hi == INT_MAX) return Qnil;
  return INT2NUM(self_t->cursor->hi);
}

VALUE
lumi_textblock_get_highlight(VALUE self)
{
  int marker, start, len;
  GET_STRUCT(textblock, self_t);
  if (self_t->cursor == NULL || self_t->cursor->pos == INT_MAX) return Qnil;
  marker = self_t->cursor->hi;
  if (marker == INT_MAX) marker = self_t->cursor->pos;
  start = min(self_t->cursor->pos, marker);
  len = max(self_t->cursor->pos, marker) - start;
  return rb_ary_new3(2, INT2NUM(start), INT2NUM(len));
}

static VALUE
lumi_find_textblock(VALUE self)
{
  while (!NIL_P(self) && !rb_obj_is_kind_of(self, cTextBlock))
  {
    SETUP_BASIC();
    self = basic->parent;
  }
  return self;
}

static VALUE
lumi_textblock_send_hover(VALUE self, int x, int y, VALUE *clicked, char *t)
{
  VALUE url = Qnil;
  int index, trailing, i, hover;
  GET_STRUCT(textblock, self_t);
  if (self_t->layout == NULL || NIL_P(self_t->links)) return Qnil;
  if (!NIL_P(self_t->attr) && ATTR(self_t->attr, hidden) == Qtrue) return Qnil;

  x -= self_t->place.ix + self_t->place.dx;
  y -= self_t->place.iy + self_t->place.dy;
  hover = pango_layout_xy_to_index(self_t->layout, x * PANGO_SCALE, y * PANGO_SCALE, &index, &trailing);
  if (hover != (self_t->hover & HOVER_MOTION))
  {
    lumi_textblock_uncache(self_t, FALSE);
    INFO("HOVER (%d, %d) OVER (%d, %d)\n", x, y, self_t->place.ix + self_t->place.dx, self_t->place.iy + self_t->place.dy);
  }
  for (i = 0; i < RARRAY_LEN(self_t->links); i++)
  {
    VALUE urll = lumi_link_at(self_t, rb_ary_entry(self_t->links, i), index, hover, clicked, t);
    if (NIL_P(url)) url = urll;
  }

  return url;
}

VALUE
lumi_textblock_motion(VALUE self, int x, int y, char *t)
{
  VALUE url = lumi_textblock_send_hover(self, x, y, NULL, t);
  if (!NIL_P(url))
  {
    lumi_canvas *canvas;
    GET_STRUCT(textblock, self_t);
    Data_Get_Struct(self_t->parent, lumi_canvas, canvas);
    lumi_app_cursor(canvas->app, s_link);
  }
  return url;
}

VALUE
lumi_textblock_hit(VALUE self, VALUE _x, VALUE _y)
{
  int x = NUM2INT(_x), y = NUM2INT(_y), index, trailing;
  GET_STRUCT(textblock, self_t);
  x -= self_t->place.ix + self_t->place.dx;
  y -= self_t->place.iy + self_t->place.dy;
  if (x < 0 || x > self_t->place.iw || y < 0 || y > self_t->place.ih)
    return Qnil;
  pango_layout_xy_to_index(self_t->layout, x * PANGO_SCALE, y * PANGO_SCALE, &index, &trailing);
  return INT2NUM(index);
}

VALUE
lumi_textblock_send_click(VALUE self, int button, int x, int y, VALUE *clicked)
{
  VALUE v = Qnil;

  if (button == 1)
  {
    GET_STRUCT(textblock, self_t);
    v = lumi_textblock_send_hover(self, x, y, clicked, NULL);
    if (self_t->hover & HOVER_MOTION)
      self_t->hover = HOVER_MOTION | HOVER_CLICK;
  }

  return v;
}

void
lumi_textblock_send_release(VALUE self, int button, int x, int y)
{
  GET_STRUCT(textblock, self_t);
  if (button == 1 && (self_t->hover & HOVER_CLICK))
  {
    VALUE proc = ATTR(self_t->attr, release);
    self_t->hover ^= HOVER_CLICK;
    if (!NIL_P(proc))
      lumi_safe_block(self, proc, rb_ary_new3(1, self));
  }
}

#define APPLY_ATTR() \
  if (attr != NULL) { \
    attr->start_index = start_index; \
    attr->end_index = end_index; \
    pango_attr_list_insert_before(block->pattr, attr); \
    attr = NULL; \
  }

#define GET_STYLE(name) \
  attr = NULL; \
  str = Qnil; \
  if (!NIL_P(oattr)) str = rb_hash_aref(oattr, ID2SYM(s_##name)); \
  if (!NIL_P(hsh) && NIL_P(str)) str = rb_hash_aref(hsh, ID2SYM(s_##name))

#define APPLY_STYLE_COLOR(name, func) \
  GET_STYLE(name); \
  if (!NIL_P(str)) \
  { \
    if (TYPE(str) == T_STRING) \
      str = lumi_color_parse(cColor, str); \
    if (rb_obj_is_kind_of(str, cColor)) \
    { \
      lumi_color *color; \
      Data_Get_Struct(str, lumi_color, color); \
      attr = pango_attr_##func##_new(color->r * 255, color->g * 255, color-> b * 255); \
    } \
    APPLY_ATTR(); \
  }

static void
lumi_app_style_for(lumi_textblock *block, lumi_app *app, VALUE klass, VALUE oattr, guint start_index, guint end_index)
{
  VALUE str = Qnil;
  VALUE hsh = rb_hash_aref(app->styles, klass);
  if (NIL_P(hsh) && NIL_P(oattr)) return;

  PangoAttribute *attr = NULL;

  APPLY_STYLE_COLOR(stroke, foreground);
  APPLY_STYLE_COLOR(fill, background);
  APPLY_STYLE_COLOR(strikecolor, strikethrough_color);
  APPLY_STYLE_COLOR(undercolor, underline_color);

  GET_STYLE(font);
  if (!NIL_P(str))
  {
    if (TYPE(str) == T_STRING)
    {
      attr = pango_attr_font_desc_new(pango_font_description_from_string(RSTRING_PTR(str)));
    }
    APPLY_ATTR();
  }

  GET_STYLE(size);
  if (!NIL_P(str))
  {
    if (TYPE(str) == T_STRING)
    {
      if (strncmp(RSTRING_PTR(str), "xx-small", 8) == 0)
        attr = pango_attr_scale_new(PANGO_SCALE_XX_SMALL);
      else if (strncmp(RSTRING_PTR(str), "x-small", 7) == 0)
        attr = pango_attr_scale_new(PANGO_SCALE_X_SMALL);
      else if (strncmp(RSTRING_PTR(str), "small", 5) == 0)
        attr = pango_attr_scale_new(PANGO_SCALE_SMALL);
      else if (strncmp(RSTRING_PTR(str), "medium", 6) == 0)
        attr = pango_attr_scale_new(PANGO_SCALE_MEDIUM);
      else if (strncmp(RSTRING_PTR(str), "large", 5) == 0)
        attr = pango_attr_scale_new(PANGO_SCALE_LARGE);
      else if (strncmp(RSTRING_PTR(str), "x-large", 7) == 0)
        attr = pango_attr_scale_new(PANGO_SCALE_X_LARGE);
      else if (strncmp(RSTRING_PTR(str), "xx-large", 8) == 0)
        attr = pango_attr_scale_new(PANGO_SCALE_XX_LARGE);
      else
        str = rb_funcall(str, s_to_i, 0);
    }
    if (TYPE(str) == T_FIXNUM)
    {
      int i = NUM2INT(str);
      if (i > 0)
        attr = pango_attr_size_new_absolute(ROUND(i * PANGO_SCALE * (96./72.)));
    }
    APPLY_ATTR();
  }

  GET_STYLE(family);
  if (!NIL_P(str))
  {
    if (TYPE(str) == T_STRING)
    {
      attr = pango_attr_family_new(RSTRING_PTR(str));
    }
    APPLY_ATTR();
  }

  GET_STYLE(weight);
  if (!NIL_P(str))
  {
    if (TYPE(str) == T_STRING)
    {
      if (strncmp(RSTRING_PTR(str), "ultralight", 10) == 0)
        attr = pango_attr_weight_new(PANGO_WEIGHT_ULTRALIGHT);
      else if (strncmp(RSTRING_PTR(str), "light", 5) == 0)
        attr = pango_attr_weight_new(PANGO_WEIGHT_LIGHT);
      else if (strncmp(RSTRING_PTR(str), "normal", 6) == 0)
        attr = pango_attr_weight_new(PANGO_WEIGHT_NORMAL);
      else if (strncmp(RSTRING_PTR(str), "semibold", 8) == 0)
        attr = pango_attr_weight_new(PANGO_WEIGHT_SEMIBOLD);
      else if (strncmp(RSTRING_PTR(str), "bold", 4) == 0)
        attr = pango_attr_weight_new(PANGO_WEIGHT_BOLD);
      else if (strncmp(RSTRING_PTR(str), "ultrabold", 9) == 0)
        attr = pango_attr_weight_new(PANGO_WEIGHT_ULTRABOLD);
      else if (strncmp(RSTRING_PTR(str), "heavy", 5) == 0)
        attr = pango_attr_weight_new(PANGO_WEIGHT_HEAVY);
    }
    else if (TYPE(str) == T_FIXNUM)
    {
      int i = NUM2INT(str);
      if (i >= 100 && i <= 900)
        attr = pango_attr_weight_new((PangoWeight)i);
    }
    APPLY_ATTR();
  }

  GET_STYLE(rise);
  if (!NIL_P(str))
  {
    if (TYPE(str) == T_STRING)
      str = rb_funcall(str, s_to_i, 0);
    if (TYPE(str) == T_FIXNUM)
    {
      int i = NUM2INT(str);
      attr = pango_attr_rise_new(i * PANGO_SCALE);
    }
    APPLY_ATTR();
  }

  GET_STYLE(kerning);
  if (!NIL_P(str))
  {
    if (TYPE(str) == T_STRING)
      str = rb_funcall(str, s_to_i, 0);
    if (TYPE(str) == T_FIXNUM)
    {
      int i = NUM2INT(str);
      attr = pango_attr_letter_spacing_new(i * PANGO_SCALE);
    }
    APPLY_ATTR();
  }

  GET_STYLE(emphasis);
  if (!NIL_P(str))
  {
    if (TYPE(str) == T_STRING)
    {
      if (strncmp(RSTRING_PTR(str), "normal", 6) == 0)
        attr = pango_attr_style_new(PANGO_STYLE_NORMAL);
      else if (strncmp(RSTRING_PTR(str), "oblique", 7) == 0)
        attr = pango_attr_style_new(PANGO_STYLE_OBLIQUE);
      else if (strncmp(RSTRING_PTR(str), "italic", 6) == 0)
        attr = pango_attr_style_new(PANGO_STYLE_ITALIC);
    }
    APPLY_ATTR();
  }

  GET_STYLE(strikethrough);
  if (!NIL_P(str))
  {
    if (TYPE(str) == T_STRING)
    {
      if (strncmp(RSTRING_PTR(str), "none", 4) == 0)
        attr = pango_attr_strikethrough_new(FALSE);
      else if (strncmp(RSTRING_PTR(str), "single", 6) == 0)
        attr = pango_attr_strikethrough_new(TRUE);
    }
    APPLY_ATTR();
  }

  GET_STYLE(stretch);
  if (!NIL_P(str))
  {
    if (TYPE(str) == T_STRING)
    {
      if (strncmp(RSTRING_PTR(str), "condensed", 9) == 0)
        attr = pango_attr_stretch_new(PANGO_STRETCH_CONDENSED);
      else if (strncmp(RSTRING_PTR(str), "normal", 6) == 0)
        attr = pango_attr_stretch_new(PANGO_STRETCH_NORMAL);
      else if (strncmp(RSTRING_PTR(str), "expanded", 8) == 0)
        attr = pango_attr_stretch_new(PANGO_STRETCH_EXPANDED);
    }
    APPLY_ATTR();
  }

  GET_STYLE(underline);
  if (!NIL_P(str))
  {
    if (TYPE(str) == T_STRING)
    {
      if (strncmp(RSTRING_PTR(str), "none", 4) == 0)
        attr = pango_attr_underline_new(PANGO_UNDERLINE_NONE);
      else if (strncmp(RSTRING_PTR(str), "single", 6) == 0)
        attr = pango_attr_underline_new(PANGO_UNDERLINE_SINGLE);
      else if (strncmp(RSTRING_PTR(str), "double", 6) == 0)
        attr = pango_attr_underline_new(PANGO_UNDERLINE_DOUBLE);
      else if (strncmp(RSTRING_PTR(str), "low", 3) == 0)
        attr = pango_attr_underline_new(PANGO_UNDERLINE_LOW);
      else if (strncmp(RSTRING_PTR(str), "error", 5) == 0)
        attr = pango_attr_underline_new(PANGO_UNDERLINE_ERROR);
    }
    APPLY_ATTR();
  }

  GET_STYLE(variant);
  if (!NIL_P(str))
  {
    if (TYPE(str) == T_STRING)
    {
      if (strncmp(RSTRING_PTR(str), "normal", 6) == 0)
        attr = pango_attr_variant_new(PANGO_VARIANT_NORMAL);
      else if (strncmp(RSTRING_PTR(str), "smallcaps", 9) == 0)
        attr = pango_attr_variant_new(PANGO_VARIANT_SMALL_CAPS);
    }
    APPLY_ATTR();
  }
}

static void
lumi_textblock_iter_pango(VALUE texts, lumi_textblock *block, lumi_app *app)
{
  VALUE v;
  long i;

  if (NIL_P(texts))
    return;

  for (i = 0; i < RARRAY_LEN(texts); i++)
  {
    v = rb_ary_entry(texts, i);
    if (rb_obj_is_kind_of(v, cTextClass))
    {
      VALUE tklass = rb_obj_class(v);
      guint start;
      lumi_text *text;
      Data_Get_Struct(v, lumi_text, text);

      start = block->len;
      lumi_textblock_iter_pango(text->texts, block, app);
      if ((text->hover & HOVER_MOTION) && tklass == cLink)
        tklass = cLinkHover;
      lumi_app_style_for(block, app, tklass, text->attr, start, block->len);
      if (!block->cached && rb_obj_is_kind_of(v, cLink) && !NIL_P(text->attr))
      {
        rb_ary_push(block->links, lumi_link_new(v, start, block->len));
      }
    }
    else if (rb_obj_is_kind_of(v, rb_cArray))
    {
      lumi_textblock_iter_pango(v, block, app);
    }
    else
    {
      char *start, *end;
      v = rb_funcall(v, s_to_s, 0);
      block->len += (guint)RSTRING_LEN(v);
      if (!block->cached)
      {
        start = RSTRING_PTR(v);
        if (!g_utf8_validate(start, RSTRING_LEN(v), (const gchar **)&end))
          lumi_error("not a valid UTF-8 string: %.*s", end - start, start);
        if (end > start)
          g_string_append_len(block->text, start, end - start);
      }
    }
  }
}

static void
lumi_textblock_make_pango(lumi_app *app, VALUE klass, lumi_textblock *block)
{
  if (!block->cached)
  {
    block->text = g_string_new(NULL);
    block->links = rb_ary_new();
  }
  block->len = 0;
  block->pattr = pango_attr_list_new();

  lumi_textblock_iter_pango(block->texts, block, app);
  lumi_app_style_for(block, app, klass, block->attr, 0, block->len);

  if (block->cursor != NULL && block->cursor->pos != INT_MAX &&
      block->cursor->hi != INT_MAX && block->cursor->pos != block->cursor->hi)
  {
    PangoAttribute *attr = pango_attr_background_new(255 * 255, 255 * 255, 0);
    attr->start_index = min(block->cursor->pos, block->cursor->hi);
    attr->end_index = max(block->cursor->pos, block->cursor->hi);
    pango_attr_list_insert(block->pattr, attr);
  }

  block->cached = 1;
}

static void
lumi_textblock_on_layout(lumi_app *app, VALUE klass, lumi_textblock *block)
{
  char *attr = NULL;
  VALUE str = Qnil, hsh = Qnil, oattr = Qnil;
  g_return_if_fail(block != NULL);
  g_return_if_fail(PANGO_IS_LAYOUT(block->layout));

  oattr = block->attr;
  hsh = rb_hash_aref(app->styles, klass);

  if (!block->cached || block->pattr == NULL)
    lumi_textblock_make_pango(app, klass, block);
  pango_layout_set_text(block->layout, block->text->str, -1);
  pango_layout_set_attributes(block->layout, block->pattr);

  GET_STYLE(justify);
  if (!NIL_P(str))
    pango_layout_set_justify(block->layout, RTEST(str));

  GET_STYLE(align);
  if (TYPE(str) == T_STRING)
  {
    if (strncmp(RSTRING_PTR(str), "left", 4) == 0)
      pango_layout_set_alignment(block->layout, PANGO_ALIGN_LEFT);
    else if (strncmp(RSTRING_PTR(str), "center", 6) == 0)
      pango_layout_set_alignment(block->layout, PANGO_ALIGN_CENTER);
    else if (strncmp(RSTRING_PTR(str), "right", 5) == 0)
      pango_layout_set_alignment(block->layout, PANGO_ALIGN_RIGHT);
  }

  GET_STYLE(wrap);
  if (TYPE(str) == T_STRING)
  {
    if (strncmp(RSTRING_PTR(str), "word", 4) == 0)
      pango_layout_set_wrap(block->layout, PANGO_WRAP_WORD);
    else if (strncmp(RSTRING_PTR(str), "char", 4) == 0)
      pango_layout_set_wrap(block->layout, PANGO_WRAP_CHAR);
    else if (strncmp(RSTRING_PTR(str), "trim", 4) == 0)
      pango_layout_set_ellipsize(block->layout, PANGO_ELLIPSIZE_END);
  }
}

VALUE
lumi_textblock_draw(VALUE self, VALUE c, VALUE actual)
{
  double crx = 0., cry = 0.;
  int px, py, pd, li, ld;
  cairo_t *cr;
  lumi_canvas *canvas;
  PangoLayoutLine *last;
  PangoRectangle crect, lrect;

  VALUE ck = rb_obj_class(c);
  GET_STRUCT(textblock, self_t);
  Data_Get_Struct(c, lumi_canvas, canvas);
  cr = CCR(canvas);

  if (!NIL_P(self_t->attr) && ATTR(self_t->attr, hidden) == Qtrue)
    return self;

  ATTR_MARGINS(self_t->attr, 4, canvas);
  if (NIL_P(ATTR(self_t->attr, margin)) && NIL_P(ATTR(self_t->attr, margin_bottom)))
    bmargin = 12;
  self_t->place.flags = REL_CANVAS;
  self_t->place.flags |= NIL_P(ATTR(self_t->attr, left)) && NIL_P(ATTR(self_t->attr, right)) ? 0 : FLAG_ABSX;
  self_t->place.flags |= NIL_P(ATTR(self_t->attr, top)) && NIL_P(ATTR(self_t->attr, bottom)) ? 0 : FLAG_ABSY;
  self_t->place.x = ATTR2(int, self_t->attr, left, canvas->cx);
  self_t->place.y = ATTR2(int, self_t->attr, top, canvas->cy);
  if (!ORIGIN(canvas->place))
  {
    self_t->place.dx = canvas->place.dx;
    self_t->place.dy = canvas->place.dy;
  }
  else
  {
    self_t->place.dx = 0;
    self_t->place.dy = 0;
  }
  self_t->place.dx += PXN(self_t->attr, displace_left, 0, CPW(canvas));
  self_t->place.dy += PXN(self_t->attr, displace_top, 0, CPH(canvas));
  self_t->place.w = ATTR2(int, self_t->attr, width, canvas->place.iw - (canvas->cx - self_t->place.x));
  self_t->place.iw = self_t->place.w - (lmargin + rmargin);
  ld = ATTR2(int, self_t->attr, leading, 4);

  if (self_t->layout != NULL)
    g_object_unref(self_t->layout);

  self_t->layout = pango_cairo_create_layout(cr);
  pd = 0;
  if (!ABSX(self_t->place) && self_t->place.x == canvas->cx)
  {
    if (self_t->place.x - CPX(canvas) > self_t->place.w)
    {
      self_t->place.x = CPX(canvas);
      canvas->cy = self_t->place.y = canvas->endy;
    }
    else
    {
      if (self_t->place.x > CPX(canvas)) {
        pd = self_t->place.x - CPX(canvas);
        pango_layout_set_indent(self_t->layout, pd * PANGO_SCALE);
        self_t->place.x = CPX(canvas);
      }
    }
  }

  pango_layout_set_width(self_t->layout, self_t->place.iw * PANGO_SCALE);
  pango_layout_set_spacing(self_t->layout, ld * PANGO_SCALE);
  lumi_textblock_on_layout(canvas->app, rb_obj_class(self), self_t);
  pango_layout_set_font_description(self_t->layout, lumi_world->default_font);

  //
  // Line up the first line with the y-cursor
  //
  if (!ABSX(self_t->place) && !ABSY(self_t->place) && pd)
  {
    last = pango_layout_get_line(self_t->layout, 0);
    pango_layout_line_get_pixel_extents(last, NULL, &lrect);
    if (lrect.width > self_t->place.iw - pd)
    {
      pango_layout_set_indent(self_t->layout, 0);
      self_t->place.x = CPX(canvas);
      canvas->cy = self_t->place.y = canvas->endy;
      pd = 0;
    }
  }
  self_t->place.ix = self_t->place.x + lmargin;
  self_t->place.iy = self_t->place.y + tmargin;

  li = pango_layout_get_line_count(self_t->layout) - 1;
  last = pango_layout_get_line(self_t->layout, li);
  pango_layout_line_get_pixel_extents(last, NULL, &lrect);
  pango_layout_get_pixel_size(self_t->layout, &px, &py);

  if (self_t->cursor != NULL && self_t->cursor->pos != INT_MAX)
  {
    int cursor = self_t->cursor->pos;
    if (cursor < 0) cursor += self_t->text->len + 1;
    pango_layout_index_to_pos(self_t->layout, cursor, &crect);
    crx = (self_t->place.ix + self_t->place.dx) + (crect.x / PANGO_SCALE);
    cry = (self_t->place.iy + self_t->place.dy) + (crect.y / PANGO_SCALE);
    self_t->cursor->x = (int)crx;
    self_t->cursor->y = (int)cry;
  }

  if (RTEST(actual))
  {
    lumi_apply_transformation(cr, self_t->st, &self_t->place, 0);
    if (lumi_shape_check(cr, &self_t->place))
    {
      cairo_move_to(cr, self_t->place.ix + self_t->place.dx, self_t->place.iy + self_t->place.dy);
      cairo_set_source_rgb(cr, 0., 0., 0.);
      pango_cairo_update_layout(cr, self_t->layout);
      pango_cairo_show_layout(cr, self_t->layout);

      if (self_t->cursor != NULL && self_t->cursor->pos != INT_MAX)
      {
        cairo_save(cr);
        cairo_new_path(cr);
        cairo_move_to(cr, crx, cry);
        cairo_line_to(cr, crx, cry + (crect.height / PANGO_SCALE));
        cairo_set_antialias(cr, CAIRO_ANTIALIAS_NONE);
        cairo_set_source_rgb(cr, 0., 0., 0.);
        cairo_set_line_width(cr, 1.);
        cairo_stroke(cr);
        cairo_restore(cr);
      }
    }

    lumi_undo_transformation(cr, self_t->st, &self_t->place, 0);
  }

  if (li == 0) {
    self_t->place.iw = px;
    self_t->place.w = px + lmargin + rmargin;
  }
  self_t->place.ih = py;
  self_t->place.h = py + tmargin + bmargin;
  INFO("TEXT: %d, %d (%d, %d) / (%d: %d, %d: %d) %d, %d [%d]\n", canvas->cx, canvas->cy,
    canvas->place.w, canvas->height, self_t->place.x, self_t->place.ix,
    self_t->place.y, self_t->place.iy, self_t->place.w, self_t->place.h, pd);

  if (!ABSY(self_t->place)) {
    // newlines have an empty size
    if (ck != cStack) {
      if (li == 0) {
        canvas->cx = self_t->place.x + lrect.x + lrect.width + rmargin + pd;
      } else {
        canvas->cy = self_t->place.y + py - lrect.height;
        if (lrect.width == 0) {
          canvas->cx = self_t->place.x + lrect.x;
        } else {
          canvas->cx = self_t->place.x + lrect.width + rmargin;
        }
      }
    }

    canvas->endy = max(self_t->place.y + self_t->place.h, canvas->endy);
    canvas->endx = canvas->cx;

    if (ck == cStack || canvas->cx - CPX(canvas) > canvas->width) {
      canvas->cx = CPX(canvas);
      canvas->cy = canvas->endy;
    }
    if (NIL_P(ATTR(self_t->attr, margin)) && NIL_P(ATTR(self_t->attr, margin_top)))
      bmargin = lrect.height;

    INFO("CX: (%d, %d) / LRECT: (%d, %d) / END: (%d, %d)\n",
      canvas->cx, canvas->cy,
      lrect.x, lrect.width,
      canvas->endx, canvas->endy);
  }
  return self;
}

VALUE
lumi_textblock_string(VALUE self)
{
  lumi_canvas *canvas;
  GET_STRUCT(textblock, self_t);
  Data_Get_Struct(self_t->parent, lumi_canvas, canvas);
  if (!self_t->cached || self_t->pattr == NULL)
    lumi_textblock_make_pango(canvas->app, rb_obj_class(self), self_t);
  return rb_str_new(self_t->text->str, self_t->text->len);
}

//
// Lumi::Button
//
void
lumi_control_mark(lumi_control *control)
{
  rb_gc_mark_maybe(control->parent);
  rb_gc_mark_maybe(control->attr);
}

static void
lumi_control_free(lumi_control *control)
{
  if (control->ref != NULL) lumi_native_control_free(control->ref);
  RUBY_CRITICAL(free(control));
}

VALUE
lumi_control_new(VALUE klass, VALUE attr, VALUE parent)
{
  lumi_control *control;
  VALUE obj = lumi_control_alloc(klass);
  Data_Get_Struct(obj, lumi_control, control);
  control->attr = attr;
  control->parent = parent;
  return obj;
}

VALUE
lumi_control_alloc(VALUE klass)
{
  VALUE obj;
  lumi_control *control = SHOE_ALLOC(lumi_control);
  SHOE_MEMZERO(control, lumi_control, 1);
  obj = Data_Wrap_Struct(klass, lumi_control_mark, lumi_control_free, control);
  control->attr = Qnil;
  control->parent = Qnil;
  return obj;
}

VALUE
lumi_control_focus(VALUE self)
{
  GET_STRUCT(control, self_t);
  ATTRSET(self_t->attr, hidden, Qtrue);
  if (self_t->ref != NULL) lumi_native_control_focus(self_t->ref);
  return self;
}

VALUE
lumi_control_get_state(VALUE self)
{
  GET_STRUCT(control, self_t);
  return ATTR(self_t->attr, state);
}

static VALUE
lumi_control_try_state(lumi_control *self_t, VALUE state)
{
  unsigned char cstate;
  if (NIL_P(state))
    cstate = CONTROL_NORMAL;
  else if (TYPE(state) == T_STRING)
  {
    if (strncmp(RSTRING_PTR(state), "disabled", 8) == 0)
      cstate = CONTROL_DISABLED;
    else if (strncmp(RSTRING_PTR(state), "readonly", 8) == 0)
      cstate = CONTROL_READONLY;
    else
    {
      lumi_error("control can't have :state of %s\n", RSTRING_PTR(state));
      return Qfalse;
    }
  }
  else return Qfalse;

  if (self_t->ref != NULL)
  {
    if (cstate == CONTROL_NORMAL)
      lumi_native_control_state(self_t->ref, TRUE, TRUE);
    else if (cstate == CONTROL_DISABLED)
      lumi_native_control_state(self_t->ref, FALSE, TRUE);
    else if (cstate == CONTROL_READONLY)
      lumi_native_control_state(self_t->ref, TRUE, FALSE);
  }
  return Qtrue;
}

VALUE
lumi_control_set_state(VALUE self, VALUE state)
{
  GET_STRUCT(control, self_t);
  if (lumi_control_try_state(self_t, state))
    ATTRSET(self_t->attr, state, state);
  return self;
}

VALUE
lumi_control_temporary_hide(VALUE self)
{
  GET_STRUCT(control, self_t);
  if (self_t->ref != NULL) lumi_control_hide_ref(self_t->ref);
  return self;
}

VALUE
lumi_control_hide(VALUE self)
{
  GET_STRUCT(control, self_t);
  ATTRSET(self_t->attr, hidden, Qtrue);
  if (self_t->ref != NULL) lumi_control_hide_ref(self_t->ref);
  return self;
}

VALUE
lumi_control_temporary_show(VALUE self)
{
  GET_STRUCT(control, self_t);
  if (self_t->ref != NULL) lumi_control_show_ref(self_t->ref);
  return self;
}

VALUE
lumi_control_show(VALUE self)
{
  GET_STRUCT(control, self_t);
  ATTRSET(self_t->attr, hidden, Qfalse);
  if (self_t->ref != NULL) lumi_control_show_ref(self_t->ref);
  return self;
}

VALUE
lumi_control_remove(VALUE self)
{
  lumi_canvas *canvas;
  GET_STRUCT(control, self_t);
  lumi_canvas_remove_item(self_t->parent, self, 1, 0);

  Data_Get_Struct(self_t->parent, lumi_canvas, canvas);
  if (self_t->ref != NULL) {
    LUMI_CONTROL_REF ref = self_t->ref;
    self_t->ref = NULL;
    lumi_native_control_remove(ref, canvas);
  }
  return self;
}

void
lumi_control_check_styles(lumi_control *self_t)
{
  VALUE x = ATTR(self_t->attr, state);
  lumi_control_try_state(self_t, x);
}

void
lumi_control_send(VALUE self, ID event)
{
  VALUE click;
  GET_STRUCT(control, self_t);

  if (rb_respond_to(self, s_checked_q))
    ATTRSET(self_t->attr, checked, rb_funcall(self, s_checked_q, 0));

  if (!NIL_P(self_t->attr))
  {
    click = rb_hash_aref(self_t->attr, ID2SYM(event));
    if (!NIL_P(click))
      lumi_safe_block(self_t->parent, click, rb_ary_new3(1, self));
  }
}

VALUE
lumi_button_draw(VALUE self, VALUE c, VALUE actual)
{
  SETUP_CONTROL(2, 0, TRUE);

#ifdef LUMI_QUARTZ
  place.h += 8;
  place.ih += 8;
#endif
  if (RTEST(actual))
  {
    if (self_t->ref == NULL)
    {
      self_t->ref = lumi_native_button(self, canvas, &place, msg);
      lumi_control_check_styles(self_t);
      lumi_native_control_position(self_t->ref, &self_t->place, self, canvas, &place);
    }
    else
      lumi_native_control_repaint(self_t->ref, &self_t->place, canvas, &place);
  }

  FINISH();

  return self;
}

VALUE
lumi_edit_line_get_text(VALUE self)
{
  GET_STRUCT(control, self_t);
  if (self_t->ref == NULL) return Qnil;
  return lumi_native_edit_line_get_text(self_t->ref);
}

VALUE
lumi_edit_line_set_text(VALUE self, VALUE text)
{
  char *msg = "";
  GET_STRUCT(control, self_t);
  if (!NIL_P(text))
  {
    text = lumi_native_to_s(text);
    ATTRSET(self_t->attr, text, text);
    msg = RSTRING_PTR(text);
  }
  if (self_t->ref != NULL) lumi_native_edit_line_set_text(self_t->ref, msg);
  return text;
}

VALUE
lumi_edit_line_draw(VALUE self, VALUE c, VALUE actual)
{
  SETUP_CONTROL(0, 0, FALSE);

#ifdef LUMI_QUARTZ
  place.x += 4; place.ix += 4;
  place.y += 4; place.iy += 4;
  place.h += 4; place.ih += 4;
  place.w += 4; place.iw += 4;
#endif
  if (RTEST(actual))
  {
    if (self_t->ref == NULL)
    {
      self_t->ref = lumi_native_edit_line(self, canvas, &place, self_t->attr, msg);
      lumi_control_check_styles(self_t);
      lumi_native_control_position(self_t->ref, &self_t->place, self, canvas, &place);
    }
    else
      lumi_native_control_repaint(self_t->ref, &self_t->place, canvas, &place);
  }

  FINISH();

  return self;
}

VALUE
lumi_edit_box_get_text(VALUE self)
{
  GET_STRUCT(control, self_t);
  if (self_t->ref == NULL) return Qnil;
  return lumi_native_edit_box_get_text(self_t->ref);
}

VALUE
lumi_edit_box_set_text(VALUE self, VALUE text)
{
  char *msg = "";
  GET_STRUCT(control, self_t);
  if (!NIL_P(text))
  {
    text = lumi_native_to_s(text);
    ATTRSET(self_t->attr, text, text);
    msg = RSTRING_PTR(text);
  }
  if (self_t->ref != NULL) lumi_native_edit_box_set_text(self_t->ref, msg);
  return text;
}


VALUE
lumi_edit_box_draw(VALUE self, VALUE c, VALUE actual)
{
  SETUP_CONTROL(80, 0, FALSE);

  if (RTEST(actual))
  {
    if (self_t->ref == NULL)
    {
      self_t->ref = lumi_native_edit_box(self, canvas, &place, self_t->attr, msg);
      lumi_control_check_styles(self_t);
      lumi_native_control_position(self_t->ref, &self_t->place, self, canvas, &place);
    }
    else
      lumi_native_control_repaint(self_t->ref, &self_t->place, canvas, &place);
  }

  FINISH();

  return self;
}

VALUE
lumi_list_box_choose(VALUE self, VALUE item)
{
  VALUE items = Qnil;
  GET_STRUCT(control, self_t);
  ATTRSET(self_t->attr, choose, item);
  if (self_t->ref == NULL) return self;

  items = ATTR(self_t->attr, items);
  lumi_native_list_box_set_active(self_t->ref, items, item);
  return self;
}

VALUE
lumi_list_box_text(VALUE self)
{
  GET_STRUCT(control, self_t);
  if (self_t->ref == NULL) return Qnil;
  return lumi_native_list_box_get_active(self_t->ref, ATTR(self_t->attr, items));
}

VALUE
lumi_list_box_items_get(VALUE self)
{
  GET_STRUCT(control, self_t);
  return ATTR(self_t->attr, items);
}

VALUE
lumi_list_box_items_set(VALUE self, VALUE items)
{
  VALUE opt = lumi_list_box_text(self);
  GET_STRUCT(control, self_t);
  if (!rb_obj_is_kind_of(items, rb_cArray))
    rb_raise(rb_eArgError, "ListBox items must be an array.");
  ATTRSET(self_t->attr, items, items);
  if (self_t->ref != NULL) lumi_native_list_box_update(self_t->ref, items);
  if (!NIL_P(opt)) lumi_list_box_choose(self, opt);
  return items;
}

VALUE
lumi_list_box_draw(VALUE self, VALUE c, VALUE actual)
{
  SETUP_CONTROL(0, 0, TRUE);

#ifdef LUMI_QUARTZ
  place.h += 4;
  place.ih += 4;
#endif
  if (RTEST(actual))
  {
    if (self_t->ref == NULL)
    {
      VALUE items = ATTR(self_t->attr, items);
      self_t->ref = lumi_native_list_box(self, canvas, &place, self_t->attr, msg);

      if (!NIL_P(items))
      {
        lumi_list_box_items_set(self, items);
        lumi_control_check_styles(self_t);
        if (!NIL_P(ATTR(self_t->attr, choose)))
          lumi_native_list_box_set_active(self_t->ref, items, ATTR(self_t->attr, choose));
      }

#ifdef LUMI_WIN32
      lumi_native_control_position_no_pad(self_t->ref, &self_t->place, self, canvas, &place);
#else
      lumi_native_control_position(self_t->ref, &self_t->place, self, canvas, &place);
#endif
    }
    else
#ifdef LUMI_WIN32
      lumi_native_control_repaint_no_pad(self_t->ref, &self_t->place, canvas, &place);
#else
      lumi_native_control_repaint(self_t->ref, &self_t->place, canvas, &place);
#endif
  }

  FINISH();

  return self;
}

VALUE
lumi_progress_draw(VALUE self, VALUE c, VALUE actual)
{
  SETUP_CONTROL(0, 0, FALSE);

  if (RTEST(actual))
  {
    if (self_t->ref == NULL)
    {
      self_t->ref = lumi_native_progress(self, canvas, &place, self_t->attr, msg);
      lumi_native_control_position(self_t->ref, &self_t->place, self, canvas, &place);
    }
    else
      lumi_native_control_repaint(self_t->ref, &self_t->place, canvas, &place);
  }

  FINISH();

  return self;
}

VALUE
lumi_progress_get_fraction(VALUE self)
{
  double perc = 0.;
  GET_STRUCT(control, self_t);
  if (self_t->ref != NULL)
    perc = lumi_native_progress_get_fraction(self_t->ref);
  return rb_float_new(perc);
}

VALUE
lumi_progress_set_fraction(VALUE self, VALUE _perc)
{
  double perc = min(max(NUM2DBL(_perc), 0.0), 1.0);
  GET_STRUCT(control, self_t);
  if (self_t->ref != NULL)
    lumi_native_progress_set_fraction(self_t->ref, perc);
  return self;
}

VALUE
lumi_check_draw(VALUE self, VALUE c, VALUE actual)
{
  SETUP_CONTROL(0, 20, FALSE);

  if (RTEST(actual))
  {
    if (self_t->ref == NULL)
    {
      self_t->ref = lumi_native_check(self, canvas, &place, self_t->attr, msg);
      if (RTEST(ATTR(self_t->attr, checked))) lumi_native_check_set(self_t->ref, Qtrue);
      lumi_control_check_styles(self_t);
      lumi_native_control_position(self_t->ref, &self_t->place, self, canvas, &place);
    }
    else
      lumi_native_control_repaint(self_t->ref, &self_t->place, canvas, &place);
  }

  FINISH();

  return self;
}

VALUE
lumi_slider_draw(VALUE self, VALUE c, VALUE actual)
{
  SETUP_CONTROL(0, 0, FALSE);

  if (RTEST(actual))
  {
    if (self_t->ref == NULL)
    {
      self_t->ref = lumi_native_slider(self, canvas, &place, self_t->attr, msg);
      lumi_native_control_position(self_t->ref, &self_t->place, self, canvas, &place);
    }
    else
      lumi_native_control_repaint(self_t->ref, &self_t->place, canvas, &place);
  }

  FINISH();

  return self;
}

VALUE
lumi_slider_get_fraction(VALUE self)
{
  double perc = 0.;
  GET_STRUCT(control, self_t);
  if (self_t->ref != NULL)
    perc = lumi_native_slider_get_fraction(self_t->ref);
  return rb_float_new(perc);
}

VALUE
lumi_slider_set_fraction(VALUE self, VALUE _perc)
{
  double perc = min(max(NUM2DBL(_perc), 0.0), 1.0);
  GET_STRUCT(control, self_t);
  if (self_t->ref != NULL)
    lumi_native_slider_set_fraction(self_t->ref, perc);
  return self;
}

VALUE
lumi_check_is_checked(VALUE self)
{
  GET_STRUCT(control, self_t);
  return lumi_native_check_get(self_t->ref);
}

VALUE
lumi_button_group(VALUE self)
{
  GET_STRUCT(control, self_t);
  if (!NIL_P(self_t->parent))
  {
    lumi_canvas *canvas;
    VALUE group = ATTR(self_t->attr, group);
    if (NIL_P(group)) group = self_t->parent;
    Data_Get_Struct(self_t->parent, lumi_canvas, canvas);
    return lumi_hash_get(canvas->app->groups, group);
  }
  return Qnil;
}

VALUE
lumi_check_set_checked(VALUE self, VALUE on)
{
  GET_STRUCT(control, self_t);
  ATTRSET(self_t->attr, checked, on);
  if (self_t->ref != NULL)
    lumi_native_check_set(self_t->ref, RTEST(on));
  return on;
}

VALUE
lumi_check_set_checked_m(VALUE self, VALUE on)
{
#ifdef LUMI_FORCE_RADIO
  if (RTEST(on))
  {
    VALUE glist = lumi_button_group(self);
    if (!NIL_P(glist))
    {
      long i;
      for (i = 0; i < RARRAY_LEN(glist); i++)
      {
        VALUE ele = rb_ary_entry(glist, i);
        lumi_check_set_checked(ele, ele == self ? Qtrue : Qfalse);
      }
    }
    return on;
  }
#endif
  lumi_check_set_checked(self, on);
  return on;
}

void
lumi_button_send_click(VALUE control)
{
  if (rb_obj_is_kind_of(control, cRadio))
    lumi_check_set_checked_m(control, Qtrue);
  lumi_control_send(control, s_click);
}

VALUE
lumi_radio_draw(VALUE self, VALUE c, VALUE actual)
{
  SETUP_CONTROL(0, 20, FALSE);

  if (RTEST(actual))
  {
    if (self_t->ref == NULL)
    {
      VALUE group = ATTR(self_t->attr, group);
      if (NIL_P(group)) group = c;

      VALUE glist = lumi_hash_get(canvas->app->groups, group);
      self_t->ref = lumi_native_radio(self, canvas, &place, self_t->attr, glist);

      if (NIL_P(glist))
        canvas->app->groups = lumi_hash_set(canvas->app->groups, group, (glist = rb_ary_new3(1, self)));
      else
        rb_ary_push(glist, self);

      if (RTEST(ATTR(self_t->attr, checked))) lumi_native_check_set(self_t->ref, Qtrue);
      lumi_control_check_styles(self_t);
      lumi_native_control_position(self_t->ref, &self_t->place, self, canvas, &place);
    }
    else
      lumi_native_control_repaint(self_t->ref, &self_t->place, canvas, &place);
  }

  FINISH();

  return self;
}

//
// Transformations
//
#define TRANS_COMMON(ele, repaint) \
  VALUE \
  lumi_##ele##_transform(VALUE self, VALUE _m) \
  { \
    GET_STRUCT(ele, self_t); \
    ID m = SYM2ID(_m); \
    if (m == s_center || m == s_corner) \
    { \
      self_t->st = lumi_transform_detach(self_t->st); \
      self_t->st->mode = m; \
    } \
    else \
    { \
      rb_raise(rb_eArgError, "transform must be called with either :center or :corner."); \
    } \
    return self; \
  } \
  VALUE \
  lumi_##ele##_translate(VALUE self, VALUE _x, VALUE _y) \
  { \
    double x, y; \
    GET_STRUCT(ele, self_t); \
    x = NUM2DBL(_x); \
    y = NUM2DBL(_y); \
    self_t->st = lumi_transform_detach(self_t->st); \
    cairo_matrix_translate(&self_t->st->tf, x, y); \
    return self; \
  } \
  VALUE \
  lumi_##ele##_rotate(VALUE self, VALUE _deg) \
  { \
    double rad; \
    GET_STRUCT(ele, self_t); \
    rad = NUM2DBL(_deg) * LUMI_RAD2PI; \
    self_t->st = lumi_transform_detach(self_t->st); \
    cairo_matrix_rotate(&self_t->st->tf, -rad); \
    if (repaint) lumi_canvas_repaint_all(self_t->parent); \
    return self; \
  } \
  VALUE \
  lumi_##ele##_scale(int argc, VALUE *argv, VALUE self) \
  { \
    VALUE _sx, _sy; \
    double sx, sy; \
    GET_STRUCT(ele, self_t); \
    rb_scan_args(argc, argv, "11", &_sx, &_sy); \
    sx = NUM2DBL(_sx); \
    if (NIL_P(_sy)) sy = sx; \
    else            sy = NUM2DBL(_sy); \
    self_t->st = lumi_transform_detach(self_t->st); \
    cairo_matrix_scale(&self_t->st->tf, sx, sy); \
    if (repaint) lumi_canvas_repaint_all(self_t->parent); \
    return self; \
  } \
  VALUE \
  lumi_##ele##_skew(int argc, VALUE *argv, VALUE self) \
  { \
    cairo_matrix_t matrix; \
    VALUE _sx, _sy; \
    double sx, sy; \
    GET_STRUCT(ele, self_t); \
    rb_scan_args(argc, argv, "11", &_sx, &_sy); \
    sx = NUM2DBL(_sx) * LUMI_RAD2PI; \
    sy = 0.0; \
    if (!NIL_P(_sy)) sy = NUM2DBL(_sy) * LUMI_RAD2PI; \
    cairo_matrix_init(&matrix, 1.0, sy, sx, 1.0, 0.0, 0.0); \
    self_t->st = lumi_transform_detach(self_t->st); \
    cairo_matrix_multiply(&self_t->st->tf, &self_t->st->tf, &matrix); \
    if (repaint) lumi_canvas_repaint_all(self_t->parent); \
    return self; \
  }

#define REPLACE_COMMON(ele) \
  VALUE \
  lumi_##ele##_replace(int argc, VALUE *argv, VALUE self) \
  { \
    long i; \
    lumi_textblock *block_t; \
    VALUE texts, attr, block; \
    GET_STRUCT(ele, self_t); \
    attr = Qnil; \
    texts = rb_ary_new(); \
    for (i = 0; i < argc; i++) \
    { \
      if (rb_obj_is_kind_of(argv[i], rb_cHash)) \
        attr = argv[i]; \
      else \
        rb_ary_push(texts, argv[i]); \
    } \
    self_t->texts = texts; \
    if (!NIL_P(attr)) self_t->attr = attr; \
    block = lumi_find_textblock(self); \
    Data_Get_Struct(block, lumi_textblock, block_t); \
    lumi_textblock_uncache(block_t, TRUE); \
    lumi_canvas_repaint_all(self_t->parent); \
    return self; \
  }

//
// Common methods
//
#define EVENT_COMMON(ele, est, sym) \
  VALUE \
  lumi_##ele##_##sym(int argc, VALUE *argv, VALUE self) \
  { \
    VALUE str = Qnil, blk = Qnil; \
    GET_STRUCT(est, self_t); \
  \
    rb_scan_args(argc, argv, "01&", &str, &blk); \
    if (NIL_P(self_t->attr)) self_t->attr = rb_hash_new(); \
    rb_hash_aset(self_t->attr, ID2SYM(s_##sym), NIL_P(blk) ? str : blk ); \
    return self; \
  }

#define CLASS_COMMON(ele) \
  VALUE \
  lumi_##ele##_style(int argc, VALUE *argv, VALUE self) \
  { \
    rb_arg_list args; \
    GET_STRUCT(ele, self_t); \
    switch (rb_parse_args(argc, argv, "h,", &args)) { \
      case 1: \
        if (NIL_P(self_t->attr)) self_t->attr = rb_hash_new(); \
        rb_funcall(self_t->attr, s_update, 1, args.a[0]); \
        lumi_canvas_repaint_all(self_t->parent); \
      break; \
      case 2: return rb_obj_freeze(rb_obj_dup(self_t->attr)); \
    } \
    return self; \
  } \
  \
  VALUE \
  lumi_##ele##_displace(VALUE self, VALUE x, VALUE y) \
  { \
    GET_STRUCT(ele, self_t); \
    ATTRSET(self_t->attr, displace_left, x); \
    ATTRSET(self_t->attr, displace_top, y); \
    lumi_canvas_repaint_all(self_t->parent); \
    return self; \
  } \
  \
  VALUE \
  lumi_##ele##_move(VALUE self, VALUE x, VALUE y) \
  { \
    GET_STRUCT(ele, self_t); \
    ATTRSET(self_t->attr, left, x); \
    ATTRSET(self_t->attr, top, y); \
    lumi_canvas_repaint_all(self_t->parent); \
    return self; \
  }

#define CLASS_COMMON2(ele) \
  VALUE \
  lumi_##ele##_hide(VALUE self) \
  { \
    GET_STRUCT(ele, self_t); \
    ATTRSET(self_t->attr, hidden, Qtrue); \
    lumi_canvas_repaint_all(self_t->parent); \
    return self; \
  } \
  \
  VALUE \
  lumi_##ele##_show(VALUE self) \
  { \
    GET_STRUCT(ele, self_t); \
    ATTRSET(self_t->attr, hidden, Qfalse); \
    lumi_canvas_repaint_all(self_t->parent); \
    return self; \
  } \
  \
  VALUE \
  lumi_##ele##_toggle(VALUE self) \
  { \
    GET_STRUCT(ele, self_t); \
    ATTRSET(self_t->attr, hidden, ATTR(self_t->attr, hidden) == Qtrue ? Qfalse : Qtrue); \
    lumi_canvas_repaint_all(self_t->parent); \
    return self; \
  } \
  CLASS_COMMON(ele); \
  EVENT_COMMON(ele, ele, change); \
  EVENT_COMMON(ele, ele, click); \
  EVENT_COMMON(ele, ele, release); \
  EVENT_COMMON(ele, ele, hover); \
  EVENT_COMMON(ele, ele, leave);

#define PLACE_COMMON(ele) \
  VALUE \
  lumi_##ele##_get_parent(VALUE self) \
  { \
    GET_STRUCT(ele, self_t); \
    return self_t->parent; \
  } \
  \
  VALUE \
  lumi_##ele##_get_left(VALUE self) \
  { \
    lumi_canvas *canvas = NULL; \
    GET_STRUCT(ele, self_t); \
    if (!NIL_P(self_t->parent)) Data_Get_Struct(self_t->parent, lumi_canvas, canvas); \
    return INT2NUM(self_t->place.x - CPX(canvas)); \
  } \
  \
  VALUE \
  lumi_##ele##_get_top(VALUE self) \
  { \
    lumi_canvas *canvas = NULL; \
    GET_STRUCT(ele, self_t); \
    if (!NIL_P(self_t->parent)) Data_Get_Struct(self_t->parent, lumi_canvas, canvas); \
    return INT2NUM(self_t->place.y - CPY(canvas)); \
  } \
  \
  VALUE \
  lumi_##ele##_get_height(VALUE self) \
  { \
    GET_STRUCT(ele, self_t); \
    return INT2NUM(self_t->place.h); \
  } \
  \
  VALUE \
  lumi_##ele##_get_width(VALUE self) \
  { \
    GET_STRUCT(ele, self_t); \
    return INT2NUM(self_t->place.w); \
  }

PLACE_COMMON(canvas)
TRANS_COMMON(canvas, 0);
PLACE_COMMON(control)
CLASS_COMMON(control)
EVENT_COMMON(control, control, click)
EVENT_COMMON(control, control, change)
CLASS_COMMON(text)
REPLACE_COMMON(text)
#ifdef VIDEO
PLACE_COMMON(video)
CLASS_COMMON(video)
#endif
PLACE_COMMON(image)
CLASS_COMMON2(image)
TRANS_COMMON(image, 1);
EVENT_COMMON(linktext, text, click);
EVENT_COMMON(linktext, text, release);
EVENT_COMMON(linktext, text, hover);
EVENT_COMMON(linktext, text, leave);
PLACE_COMMON(shape)
CLASS_COMMON2(shape)
TRANS_COMMON(shape, 1);
CLASS_COMMON2(pattern)
PLACE_COMMON(textblock)
CLASS_COMMON2(textblock)
REPLACE_COMMON(textblock)

VALUE
lumi_textblock_style_m(int argc, VALUE *argv, VALUE self)
{
  GET_STRUCT(textblock, self_t);
  VALUE obj = lumi_textblock_style(argc, argv, self);
  lumi_textblock_uncache(self_t, FALSE);
  return obj;
}

//
// Lumi::Timer
//
//
void
lumi_timer_call(VALUE self)
{
  GET_STRUCT(timer, timer);
  lumi_safe_block(timer->parent, timer->block, rb_ary_new3(1, INT2NUM(timer->frame)));
  timer->frame++;

  if (rb_obj_is_kind_of(self, cTimer))
  {
    timer->ref = 0;
    timer->started = ANIM_STOPPED;
  }
}

void
lumi_timer_mark(lumi_timer *timer)
{
  rb_gc_mark_maybe(timer->block);
  rb_gc_mark_maybe(timer->parent);
}

static void
lumi_timer_free(lumi_timer *timer)
{
  RUBY_CRITICAL(free(timer));
}

VALUE
lumi_timer_new(VALUE klass, VALUE rate, VALUE block, VALUE parent)
{
  lumi_timer *timer;
  VALUE obj = lumi_timer_alloc(klass);
  Data_Get_Struct(obj, lumi_timer, timer);
  timer->block = block;
  if (!NIL_P(rate))
  {
    if (klass == cAnim)
      timer->rate = 1000 / NUM2INT(rate);
    else
      timer->rate = (int)(1000. * NUM2DBL(rate));
  }
  if (timer->rate < 1) timer->rate = 1;
  timer->parent = parent;
  return obj;
}

VALUE
lumi_timer_alloc(VALUE klass)
{
  VALUE obj;
  lumi_timer *timer = SHOE_ALLOC(lumi_timer);
  SHOE_MEMZERO(timer, lumi_timer, 1);
  obj = Data_Wrap_Struct(klass, lumi_timer_mark, lumi_timer_free, timer);
  timer->block = Qnil;
  timer->rate = 1000 / 12;  // 12 frames per second
  timer->parent = Qnil;
  timer->started = ANIM_NADA;
  return obj;
}

VALUE
lumi_timer_remove(VALUE self)
{
  GET_STRUCT(timer, self_t);
  lumi_timer_stop(self);
  lumi_canvas_remove_item(self_t->parent, self, 0, 1);
  return self;
}

VALUE
lumi_timer_stop(VALUE self)
{
  GET_STRUCT(timer, self_t);
  if (self_t->started == ANIM_STARTED)
  {
    lumi_canvas *canvas;
    Data_Get_Struct(self_t->parent, lumi_canvas, canvas);
    lumi_native_timer_remove(canvas, self_t->ref);
    self_t->started = ANIM_PAUSED;
  }
  return self;
}

VALUE
lumi_timer_start(VALUE self)
{
  GET_STRUCT(timer, self_t);
  unsigned int interval = self_t->rate;
  if (self_t->started != ANIM_STARTED)
  {
    lumi_canvas *canvas;
    Data_Get_Struct(self_t->parent, lumi_canvas, canvas);
    self_t->ref = lumi_native_timer_start(self, canvas, interval);
    self_t->started = ANIM_STARTED;
  }
  return self;
}

VALUE
lumi_timer_toggle(VALUE self)
{
  GET_STRUCT(timer, self_t);
  return self_t->started == ANIM_STARTED ? lumi_timer_stop(self) : lumi_timer_start(self);
}

VALUE
lumi_timer_draw(VALUE self, VALUE c, VALUE actual)
{
  lumi_canvas *canvas;
  GET_STRUCT(timer, self_t);
  Data_Get_Struct(self_t->parent, lumi_canvas, canvas);
  if (RTEST(actual) && self_t->started == ANIM_NADA)
  {
    self_t->frame = 0;
    lumi_timer_start(self);
  }
  return self;
}

void
lumi_msg(ID typ, VALUE str)
{
#ifdef RUBY_1_9
  ID func = rb_frame_this_func();
  rb_ary_push(lumi_world->msgs, rb_ary_new3(6,
    ID2SYM(typ), str, rb_funcall(rb_cTime, s_now, 0),
    func ? ID2SYM(func) : Qnil,
    rb_str_new2("<unknown>"), INT2NUM(0)));
#else
  ID func = rb_frame_last_func();
  rb_ary_push(lumi_world->msgs, rb_ary_new3(6,
    ID2SYM(typ), str, rb_funcall(rb_cTime, s_now, 0),
    func ? ID2SYM(func) : Qnil,
    rb_str_new2(ruby_sourcefile), INT2NUM(ruby_sourceline)));
#endif
}

#define DEBUG_TYPE(t) \
  VALUE \
  lumi_canvas_##t(VALUE self, VALUE str) \
  { \
    lumi_msg(s_##t, str); \
    return Qnil; \
  } \
  \
  void \
  lumi_##t(const char *fmt, ...) \
  { \
    va_list args; \
    char buf[BUFSIZ]; \
  \
    va_start(args,fmt); \
    vsnprintf(buf, BUFSIZ, fmt, args); \
    va_end(args); \
    lumi_msg(s_##t, rb_str_new2(buf)); \
  }

//
// Lumi::Download
//
void
lumi_http_mark(lumi_http_klass *dl)
{
  rb_gc_mark_maybe(dl->parent);
  rb_gc_mark_maybe(dl->attr);
  rb_gc_mark_maybe(dl->response);
}

static void
lumi_http_free(lumi_http_klass *dl)
{
  RUBY_CRITICAL(free(dl));
}

VALUE
lumi_http_new(VALUE klass, VALUE parent, VALUE attr)
{
  lumi_http_klass *dl;
  VALUE obj = lumi_http_alloc(klass);
  Data_Get_Struct(obj, lumi_http_klass, dl);
  dl->parent = parent;
  dl->attr = attr;
  return obj;
}

VALUE
lumi_http_alloc(VALUE klass)
{
  VALUE obj;
  lumi_http_klass *dl = SHOE_ALLOC(lumi_http_klass);
  SHOE_MEMZERO(dl, lumi_http_klass, 1);
  obj = Data_Wrap_Struct(klass, lumi_http_mark, lumi_http_free, dl);
  dl->parent = Qnil;
  dl->attr = Qnil;
  dl->response = Qnil;
  return obj;
}

VALUE
lumi_http_remove(VALUE self)
{
  GET_STRUCT(http_klass, self_t);
  self_t->state = LUMI_DOWNLOAD_HALT;
  lumi_canvas_remove_item(self_t->parent, self, 0, 1);
  return self;
}

VALUE
lumi_http_abort(VALUE self)
{
  GET_STRUCT(http_klass, self_t);
  self_t->state = LUMI_DOWNLOAD_HALT;
  return self;
}

int
lumi_message_download(VALUE self, void *data)
{
  VALUE proc = Qnil;
  lumi_http_event *de = (lumi_http_event *)data;
  GET_STRUCT(http_klass, dl);
  INFO("EVENT: %d (%d), %lu, %llu, %llu\n", (int)de->stage, (int)de->error, de->percent,
    de->transferred, de->total);

  switch (de->stage)
  {
    case LUMI_HTTP_STATUS:
      dl->response = lumi_response_new(cResponse, (int)de->status);
    return 0;

    case LUMI_HTTP_HEADER:
    {
      VALUE h = lumi_response_headers(dl->response);
      rb_hash_aset(h, rb_str_new(de->hkey, de->hkeylen), rb_str_new(de->hval, de->hvallen));
    }
    return 0;

    case LUMI_HTTP_ERROR:
      proc = ATTR(dl->attr, error);
      if (!NIL_P(proc))
        lumi_safe_block(dl->parent, proc, rb_ary_new3(2, self, lumi_http_err(de->error)));
      else
        lumi_canvas_error(self, lumi_http_err(de->error));
    return 0;

    case LUMI_HTTP_COMPLETED:
      if (de->body != NULL) rb_iv_set(dl->response, "body", rb_str_new(de->body, de->total));
  }

  dl->percent = de->percent;
  dl->total = de->total;
  dl->transferred = de->transferred;

  switch (de->stage) {
    case LUMI_HTTP_CONNECTED: proc = ATTR(dl->attr, start); break;
    case LUMI_HTTP_TRANSFER:  proc = ATTR(dl->attr, progress); break;
    case LUMI_HTTP_COMPLETED: proc = ATTR(dl->attr, finish); break;
  }

  if (!NIL_P(proc))
    lumi_safe_block(dl->parent, proc, rb_ary_new3(1, self));
  return dl->state;
}

typedef struct {
  LUMI_CONTROL_REF ref;
  VALUE download;
} lumi_doth_data;

int
lumi_doth_handler(lumi_http_event *de, void *data)
{
  lumi_doth_data *doth = (lumi_doth_data *)data;
  lumi_http_event *de2 = SHOE_ALLOC(lumi_http_event);
  SHOE_MEMCPY(de2, de, lumi_http_event, 1);
  return lumi_throw_message(LUMI_THREAD_DOWNLOAD, doth->download, de2);
}

void
lumi_http_request_free(lumi_http_request *req)
{
  if (req->url != NULL) free(req->url);
  if (req->scheme != NULL) free(req->scheme);
  if (req->host != NULL) free(req->host);
  if (req->path != NULL) free(req->path);
  if (req->method != NULL) free(req->method);
  if (req->filepath != NULL) free(req->filepath);
  if (req->body != NULL) free(req->body);
  if (req->headers != NULL) lumi_http_headers_free(req->headers);
  if (req->mem != NULL) free(req->mem);
}

VALUE
lumi_http_threaded(VALUE self, VALUE url, VALUE attr)
{
  VALUE obj = lumi_http_new(cDownload, self, attr);
  GET_STRUCT(canvas, self_t);
  char *url_string = NULL;

  if (!rb_respond_to(url, s_host)) {
    url_string = strdup(RSTRING_PTR(url));
    url = rb_funcall(rb_mKernel, s_URI, 1, url);
  }

  VALUE scheme = rb_funcall(url, s_scheme, 0);
  VALUE host = rb_funcall(url, s_host, 0);
  VALUE port = rb_funcall(url, s_port, 0);
  VALUE path = rb_funcall(url, s_request_uri, 0);

  if (url_string == NULL) {
    url_string = SHOE_ALLOC_N(char, LUMI_BUFSIZE);
    char slash[2] = "/";
    if (RSTRING_PTR(path)[0] == '/') slash[0] = '\0';
    sprintf(url_string, "%s://%s:%d%s%s", scheme, host, port, slash, path);
  }

  lumi_http_request *req = SHOE_ALLOC(lumi_http_request);
  SHOE_MEMZERO(req, lumi_http_request, 1);
  req->url = url_string;
  req->scheme = strdup(RSTRING_PTR(scheme));
  req->host = strdup(RSTRING_PTR(host));
  req->port = NUM2INT(port);
  req->path = strdup(RSTRING_PTR(path));
  req->handler = lumi_doth_handler;
  req->flags = LUMI_DL_DEFAULTS;
  if (ATTR(attr, redirect) == Qfalse) req->flags ^= LUMI_DL_REDIRECTS;

  VALUE method = ATTR(attr, method);
  VALUE headers = ATTR(attr, headers);
  VALUE body = ATTR(attr, body);
  if (!NIL_P(body))
  {
    req->body = strdup(RSTRING_PTR(body));
    req->bodylen = RSTRING_LEN(body);
  }
  if (!NIL_P(method))  req->method = strdup(RSTRING_PTR(method));
  if (!NIL_P(headers)) req->headers = lumi_http_headers(headers);

  VALUE save = ATTR(attr, save);
  if (req->method == NULL || strcmp(req->method, "HEAD") != 0)
  {
    if (NIL_P(save))
    {
      req->mem = SHOE_ALLOC_N(char, LUMI_BUFSIZE);
      req->memlen = LUMI_BUFSIZE;
    }
    else
    {
      req->filepath = strdup(RSTRING_PTR(save));
    }
  }

  lumi_doth_data *data = SHOE_ALLOC(lumi_doth_data);
  data->download = obj;
  req->data = data;

  lumi_queue_download(req);
  return obj;
}

VALUE
lumi_http_length(VALUE self)
{
  GET_STRUCT(http_klass, dl);
  return rb_ull2inum(dl->total);
}

VALUE
lumi_http_percent(VALUE self)
{
  GET_STRUCT(http_klass, dl);
  return rb_uint2inum(dl->percent);
}

VALUE
lumi_http_response(VALUE self)
{
  GET_STRUCT(http_klass, dl);
  return dl->response;
}

VALUE
lumi_http_transferred(VALUE self)
{
  GET_STRUCT(http_klass, dl);
  return rb_ull2inum(dl->transferred);
}

EVENT_COMMON(http, http_klass, start);
EVENT_COMMON(http, http_klass, progress);
EVENT_COMMON(http, http_klass, finish);
EVENT_COMMON(http, http_klass, error);

//
// Lumi::Response
//
VALUE
lumi_response_new(VALUE klass, int status)
{
  VALUE obj = rb_obj_alloc(cResponse);
  rb_iv_set(obj, "body", Qnil);
  rb_iv_set(obj, "headers", rb_hash_new());
  rb_iv_set(obj, "status", INT2NUM(status));
  return obj;
}

VALUE
lumi_response_body(VALUE self)
{
  return rb_iv_get(self, "body");
}

VALUE
lumi_response_headers(VALUE self)
{
  return rb_iv_get(self, "headers");
}

VALUE
lumi_response_status(VALUE self)
{
  return rb_iv_get(self, "status");
}

// TODO: handle exceptions
int lumi_catch_message(unsigned int name, VALUE obj, void *data) {
  int ret = LUMI_DOWNLOAD_CONTINUE;
  switch (name) {
    case LUMI_THREAD_DOWNLOAD:
      ret = lumi_message_download(obj, data);
      free(data);
    break;
    case LUMI_IMAGE_DOWNLOAD:
    {
      VALUE hash, etag = Qnil, uri, uext, path, realpath;
      lumi_image_download_event *side = (lumi_image_download_event *)data;
      if (lumi_image_downloaded(side))
      {
        lumi_canvas_repaint_all(side->slot);

        path = rb_str_new2(side->filepath);
        uri = rb_str_new2(side->uripath);
        hash = rb_str_new2(side->hexdigest);
        if (side->etag != NULL) etag = rb_str_new2(side->etag);
        uext = rb_funcall(rb_cFile, rb_intern("extname"), 1, path);

        rb_funcall(rb_const_get(rb_cObject, rb_intern("DATABASE")),
          rb_intern("notify_cache_of"), 3, uri, etag, hash);
        if (side->status != 304)
        {
          realpath = rb_funcall(cLumi, rb_intern("image_cache_path"), 2, hash, uext);
          rename(side->filepath, RSTRING_PTR(realpath));
        }
      }

      free(side->filepath);
      free(side->uripath);
      if (side->etag != NULL) free(side->etag);
      free(data);
    }
    break;
  }
  return ret;
}

DEBUG_TYPE(info);
DEBUG_TYPE(debug);
DEBUG_TYPE(warn);
DEBUG_TYPE(error);

VALUE
lumi_p(VALUE self, VALUE obj)
{
  return lumi_canvas_debug(self, rb_inspect(obj));
}

VALUE
lumi_log(VALUE self)
{
  return lumi_world->msgs;
}

VALUE
lumi_font(VALUE self, VALUE path)
{
  StringValue(path);
  return lumi_load_font(RSTRING_PTR(path));
}

//
// Defines a redirecting function which applies the element or transformation
// to the currently active canvas.  This is used in place of the old instance_eval
// and ensures that you have access to the App's instance variables while
// assembling elements in a layout.
//
#define FUNC_M(name, func, argn) \
  VALUE \
  lumi_canvas_c_##func(int argc, VALUE *argv, VALUE self) \
  { \
    VALUE canvas, obj; \
    GET_STRUCT(canvas, self_t); \
    char *n = name; \
    if (rb_ary_entry(self_t->app->nesting, 0) == self || \
         ((rb_obj_is_kind_of(self, cWidget) || self == self_t->app->nestslot) && \
          RARRAY_LEN(self_t->app->nesting) > 0)) \
      canvas = rb_ary_entry(self_t->app->nesting, RARRAY_LEN(self_t->app->nesting) - 1); \
    else \
      canvas = self; \
    if (!rb_obj_is_kind_of(canvas, cCanvas)) \
      return ts_funcall2(canvas, rb_intern(n + 1), argc, argv); \
    obj = call_cfunc(CASTHOOK(lumi_canvas_##func), canvas, argn, argc, argv); \
    if (n[0] == '+' && RARRAY_LEN(self_t->app->nesting) == 0) lumi_canvas_repaint_all(self); \
    return obj; \
  } \
  VALUE \
  lumi_app_c_##func(int argc, VALUE *argv, VALUE self) \
  { \
    VALUE canvas; \
    char *n = name; \
    GET_STRUCT(app, app); \
    if (RARRAY_LEN(app->nesting) > 0) \
      canvas = rb_ary_entry(app->nesting, RARRAY_LEN(app->nesting) - 1); \
    else \
      canvas = app->canvas; \
    if (!rb_obj_is_kind_of(canvas, cCanvas)) \
      return ts_funcall2(canvas, rb_intern(n + 1), argc, argv); \
    return lumi_canvas_c_##func(argc, argv, canvas); \
  }

//
// See ruby.h for the complete list of App methods which redirect to Canvas.
//
CANVAS_DEFS(FUNC_M);

#define C(n, s) \
  re##n = rb_eval_string(s); \
  rb_const_set(cLumi, rb_intern("" # n), re##n);

VALUE progname;

//
// Everything exposed to Ruby is exposed here.
//
void
lumi_ruby_init()
{
  progname = rb_str_new2("(eval)");
  rb_define_variable("$0", &progname);
  rb_define_variable("$PROGRAM_NAME", &progname);

  instance_eval_proc = rb_eval_string("lambda{|o,b| o.instance_eval(&b)}");
  rb_gc_register_address(&instance_eval_proc);
  ssNestSlot = rb_eval_string("{:height => 1.0}");
  rb_gc_register_address(&ssNestSlot);

  s_aref = rb_intern("[]=");
  s_perc = rb_intern("%");
  s_mult = rb_intern("*");
  s_checked_q = rb_intern("checked?");
  SYMBOL_DEFS(SYMBOL_INTERN);

  symAltQuest = ID2SYM(rb_intern("alt_?"));
  symAltSlash = ID2SYM(rb_intern("alt_/"));
  symAltDot = ID2SYM(rb_intern("alt_."));

  //
  // I want all elements to be addressed Lumi::Name, but also available in
  // a separate mixin (cTypes), for inclusion in every Lumi.app block.
  //
  cTypes = rb_define_module("Lumi");
  rb_mod_remove_const(rb_cObject, rb_str_new2("Lumi"));

  cLumiWindow = rb_define_class_under(cTypes, "Window", rb_cObject);
  cMouse = rb_define_class_under(cTypes, "Mouse", rb_cObject);

  cCanvas = rb_define_class_under(cTypes, "Canvas", rb_cObject);
  rb_define_alloc_func(cCanvas, lumi_canvas_alloc);
  rb_define_method(cCanvas, "top", CASTHOOK(lumi_canvas_get_top), 0);
  rb_define_method(cCanvas, "left", CASTHOOK(lumi_canvas_get_left), 0);
  rb_define_method(cCanvas, "width", CASTHOOK(lumi_canvas_get_width), 0);
  rb_define_method(cCanvas, "height", CASTHOOK(lumi_canvas_get_height), 0);
  rb_define_method(cCanvas, "scroll_height", CASTHOOK(lumi_canvas_get_scroll_height), 0);
  rb_define_method(cCanvas, "scroll_max", CASTHOOK(lumi_canvas_get_scroll_max), 0);
  rb_define_method(cCanvas, "scroll_top", CASTHOOK(lumi_canvas_get_scroll_top), 0);
  rb_define_method(cCanvas, "scroll_top=", CASTHOOK(lumi_canvas_set_scroll_top), 1);
  rb_define_method(cCanvas, "displace", CASTHOOK(lumi_canvas_displace), 2);
  rb_define_method(cCanvas, "move", CASTHOOK(lumi_canvas_move), 2);
  rb_define_method(cCanvas, "style", CASTHOOK(lumi_canvas_style), -1);
  rb_define_method(cCanvas, "parent", CASTHOOK(lumi_canvas_get_parent), 0);
  rb_define_method(cCanvas, "contents", CASTHOOK(lumi_canvas_contents), 0);
  rb_define_method(cCanvas, "children", CASTHOOK(lumi_canvas_children), 0);
  rb_define_method(cCanvas, "draw", CASTHOOK(lumi_canvas_draw), 2);
  rb_define_method(cCanvas, "hide", CASTHOOK(lumi_canvas_hide), 0);
  rb_define_method(cCanvas, "show", CASTHOOK(lumi_canvas_show), 0);
  rb_define_method(cCanvas, "toggle", CASTHOOK(lumi_canvas_toggle), 0);
  rb_define_method(cCanvas, "remove", CASTHOOK(lumi_canvas_remove), 0);

  cLumi = rb_define_class("Lumi", cCanvas);
  rb_include_module(cLumi, cTypes);
  rb_const_set(cLumi, rb_intern("Types"), cTypes);
  rb_const_set(cTypes, rb_intern("RELEASE_NAME"), rb_str_new2(LUMI_RELEASE_NAME));
  rb_const_set(cTypes, rb_intern("RELEASE_ID"), INT2NUM(LUMI_RELEASE_ID));
  rb_const_set(cTypes, rb_intern("REVISION"), INT2NUM(LUMI_REVISION));
  rb_const_set(cTypes, rb_intern("VERSION"), rb_str_new2(LUMI_RELEASE_NAME));
  rb_const_set(cTypes, rb_intern("RAD2PI"), rb_float_new(LUMI_RAD2PI));
  rb_const_set(cTypes, rb_intern("TWO_PI"), rb_float_new(LUMI_PIM2));
  rb_const_set(cTypes, rb_intern("HALF_PI"), rb_float_new(LUMI_HALFPI));
  rb_const_set(cTypes, rb_intern("PI"), rb_float_new(LUMI_PI));
  rb_const_set(cTypes, rb_intern("VIDEO"), LUMI_VIDEO ? Qtrue : Qfalse);

  cApp = rb_define_class_under(cTypes, "App", rb_cObject);
  rb_define_alloc_func(cApp, lumi_app_alloc);
  rb_define_method(cApp, "fullscreen", CASTHOOK(lumi_app_get_fullscreen), 0);
  rb_define_method(cApp, "fullscreen=", CASTHOOK(lumi_app_set_fullscreen), 1);
  rb_define_method(cApp, "name", CASTHOOK(lumi_app_get_title), 0);
  rb_define_method(cApp, "name=", CASTHOOK(lumi_app_set_title), 1);
  rb_define_method(cApp, "location", CASTHOOK(lumi_app_location), 0);
  rb_define_method(cApp, "started?", CASTHOOK(lumi_app_is_started), 0);
  rb_define_method(cApp, "width", CASTHOOK(lumi_app_get_width), 0);
  rb_define_method(cApp, "height", CASTHOOK(lumi_app_get_height), 0);
  rb_define_method(cApp, "slot", CASTHOOK(lumi_app_slot), 0);
  cDialog = rb_define_class_under(cTypes, "Dialog", cApp);

  eInvMode = rb_define_class_under(cTypes, "InvalidModeError", rb_eStandardError);
  eNotImpl = rb_define_class_under(cTypes, "NotImplementedError", rb_eStandardError);
  eVlcError = rb_define_class_under(cTypes, "VideoError", rb_eStandardError);
  eImageError = rb_define_class_under(cTypes, "ImageError", rb_eStandardError);
  C(HEX_SOURCE, "/^(?:0x|#)?([0-9A-F]{2})([0-9A-F]{2})([0-9A-F]{2})$/i");
  C(HEX3_SOURCE, "/^(?:0x|#)?([0-9A-F])([0-9A-F])([0-9A-F])$/i");
  C(RGB_SOURCE, "/^rgb\\((\\d+), *(\\d+), *(\\d+)\\)$/i");
  C(RGBA_SOURCE, "/^rgb\\((\\d+), *(\\d+), *(\\d+), *(\\d+)\\)$/i");
  C(GRAY_SOURCE, "/^gray\\((\\d+)\\)$/i");
  C(GRAYA_SOURCE, "/^gray\\((\\d+), *(\\d+)\\)$/i");
  C(LF, "/\\r?\\n/");
  rb_eval_string(
    "def Lumi.escape(string);"
       "string.gsub(/&/n, '&amp;').gsub(/\\\"/n, '&quot;').gsub(/>/n, '&gt;').gsub(/</n, '&lt;');"
    "end"
  );

  rb_define_singleton_method(cLumi, "APPS", CASTHOOK(lumi_apps_get), 0);
  rb_define_singleton_method(cLumi, "app", CASTHOOK(lumi_app_main), -1);
  rb_define_singleton_method(cLumi, "p", CASTHOOK(lumi_p), 1);
  rb_define_singleton_method(cLumi, "log", CASTHOOK(lumi_log), 0);

  //
  // Canvas methods
  // See ruby.h for the complete list of Canvas method signatures.
  // Macros are used to build App redirection methods, which should be
  // speedier than method_missing.
  //
#define RUBY_M(name, func, argc) \
  rb_define_method(cCanvas, name + 1, CASTHOOK(lumi_canvas_c_##func), -1); \
  rb_define_method(cApp, name + 1, CASTHOOK(lumi_app_c_##func), -1)

  CANVAS_DEFS(RUBY_M);

  //
  // Lumi Kernel methods
  //
  rb_define_method(rb_mKernel, "rgb", CASTHOOK(lumi_color_rgb), -1);
  rb_define_method(rb_mKernel, "gray", CASTHOOK(lumi_color_gray), -1);
  rb_define_method(rb_mKernel, "gradient", CASTHOOK(lumi_color_gradient), -1);
  rb_define_method(rb_mKernel, "pattern", CASTHOOK(lumi_pattern_method), 1);
  rb_define_method(rb_mKernel, "quit", CASTHOOK(lumi_app_quit), 0);
  rb_define_method(rb_mKernel, "exit", CASTHOOK(lumi_app_quit), 0);

  rb_define_method(rb_mKernel, "debug", CASTHOOK(lumi_canvas_debug), 1);
  rb_define_method(rb_mKernel, "info", CASTHOOK(lumi_canvas_info), 1);
  rb_define_method(rb_mKernel, "warn", CASTHOOK(lumi_canvas_warn), 1);
  rb_define_method(rb_mKernel, "error", CASTHOOK(lumi_canvas_error), 1);

  cFlow       = rb_define_class_under(cTypes, "Flow", cLumi);
  cStack      = rb_define_class_under(cTypes, "Stack", cLumi);
  cMask       = rb_define_class_under(cTypes, "Mask", cLumi);
  cWidget     = rb_define_class_under(cTypes, "Widget", cLumi);

  cShape    = rb_define_class_under(cTypes, "Shape", rb_cObject);
  rb_define_alloc_func(cShape, lumi_shape_alloc);
  rb_define_method(cShape, "app", CASTHOOK(lumi_canvas_get_app), 0);
  rb_define_method(cShape, "displace", CASTHOOK(lumi_shape_displace), 2);
  rb_define_method(cShape, "draw", CASTHOOK(lumi_shape_draw), 2);
  rb_define_method(cShape, "move", CASTHOOK(lumi_shape_move), 2);
  rb_define_method(cShape, "parent", CASTHOOK(lumi_shape_get_parent), 0);
  rb_define_method(cShape, "top", CASTHOOK(lumi_shape_get_top), 0);
  rb_define_method(cShape, "left", CASTHOOK(lumi_shape_get_left), 0);
  rb_define_method(cShape, "width", CASTHOOK(lumi_shape_get_width), 0);
  rb_define_method(cShape, "height", CASTHOOK(lumi_shape_get_height), 0);
  rb_define_method(cShape, "remove", CASTHOOK(lumi_basic_remove), 0);
  rb_define_method(cShape, "style", CASTHOOK(lumi_shape_style), -1);
  rb_define_method(cShape, "hide", CASTHOOK(lumi_shape_hide), 0);
  rb_define_method(cShape, "show", CASTHOOK(lumi_shape_show), 0);
  rb_define_method(cShape, "toggle", CASTHOOK(lumi_shape_toggle), 0);
  rb_define_method(cShape, "click", CASTHOOK(lumi_shape_click), -1);
  rb_define_method(cShape, "release", CASTHOOK(lumi_shape_release), -1);
  rb_define_method(cShape, "hover", CASTHOOK(lumi_shape_hover), -1);
  rb_define_method(cShape, "leave", CASTHOOK(lumi_shape_leave), -1);

  cImage    = rb_define_class_under(cTypes, "Image", rb_cObject);
  rb_define_alloc_func(cImage, lumi_image_alloc);
  rb_define_method(cImage, "[]", CASTHOOK(lumi_image_get_pixel), 2);
  rb_define_method(cImage, "[]=", CASTHOOK(lumi_image_set_pixel), 3);
  rb_define_method(cImage, "nostroke", CASTHOOK(lumi_canvas_nostroke), 0);
  rb_define_method(cImage, "stroke", CASTHOOK(lumi_canvas_stroke), -1);
  rb_define_method(cImage, "strokewidth", CASTHOOK(lumi_canvas_strokewidth), 1);
  rb_define_method(cImage, "dash", CASTHOOK(lumi_canvas_dash), 1);
  rb_define_method(cImage, "cap", CASTHOOK(lumi_canvas_cap), 1);
  rb_define_method(cImage, "nofill", CASTHOOK(lumi_canvas_nofill), 0);
  rb_define_method(cImage, "fill", CASTHOOK(lumi_canvas_fill), -1);
  rb_define_method(cImage, "arrow", CASTHOOK(lumi_canvas_arrow), -1);
  rb_define_method(cImage, "line", CASTHOOK(lumi_canvas_line), -1);
  rb_define_method(cImage, "arc", CASTHOOK(lumi_canvas_arc), -1);
  rb_define_method(cImage, "oval", CASTHOOK(lumi_canvas_oval), -1);
  rb_define_method(cImage, "rect", CASTHOOK(lumi_canvas_rect), -1);
  rb_define_method(cImage, "shape", CASTHOOK(lumi_canvas_shape), -1);
  rb_define_method(cImage, "star", CASTHOOK(lumi_canvas_star), -1);
  rb_define_method(cImage, "move_to", CASTHOOK(lumi_canvas_move_to), 2);
  rb_define_method(cImage, "line_to", CASTHOOK(lumi_canvas_line_to), 2);
  rb_define_method(cImage, "curve_to", CASTHOOK(lumi_canvas_curve_to), 6);
  rb_define_method(cImage, "arc_to", CASTHOOK(lumi_canvas_arc_to), 6);
  rb_define_method(cImage, "blur", CASTHOOK(lumi_canvas_blur), -1);
  rb_define_method(cImage, "glow", CASTHOOK(lumi_canvas_glow), -1);
  rb_define_method(cImage, "shadow", CASTHOOK(lumi_canvas_shadow), -1);
  rb_define_method(cImage, "image", CASTHOOK(lumi_canvas_image), -1);
  rb_define_method(cImage, "path", CASTHOOK(lumi_image_get_path), 0);
  rb_define_method(cImage, "path=", CASTHOOK(lumi_image_set_path), 1);
  rb_define_method(cImage, "app", CASTHOOK(lumi_canvas_get_app), 0);
  rb_define_method(cImage, "displace", CASTHOOK(lumi_image_displace), 2);
  rb_define_method(cImage, "draw", CASTHOOK(lumi_image_draw), 2);
  rb_define_method(cImage, "size", CASTHOOK(lumi_image_size), 0);
  rb_define_method(cImage, "move", CASTHOOK(lumi_image_move), 2);
  rb_define_method(cImage, "parent", CASTHOOK(lumi_image_get_parent), 0);
  rb_define_method(cImage, "rotate", CASTHOOK(lumi_image_rotate), 1);
  rb_define_method(cImage, "scale", CASTHOOK(lumi_image_scale), -1);
  rb_define_method(cImage, "skew", CASTHOOK(lumi_image_skew), -1);
  rb_define_method(cImage, "transform", CASTHOOK(lumi_image_transform), 1);
  rb_define_method(cImage, "translate", CASTHOOK(lumi_image_translate), 2);
  rb_define_method(cImage, "top", CASTHOOK(lumi_image_get_top), 0);
  rb_define_method(cImage, "left", CASTHOOK(lumi_image_get_left), 0);
  rb_define_method(cImage, "width", CASTHOOK(lumi_image_get_width), 0);
  rb_define_method(cImage, "height", CASTHOOK(lumi_image_get_height), 0);
  rb_define_method(cImage, "full_width", CASTHOOK(lumi_image_get_full_width), 0);
  rb_define_method(cImage, "full_height", CASTHOOK(lumi_image_get_full_height), 0);
  rb_define_method(cImage, "remove", CASTHOOK(lumi_basic_remove), 0);
  rb_define_method(cImage, "style", CASTHOOK(lumi_image_style), -1);
  rb_define_method(cImage, "hide", CASTHOOK(lumi_image_hide), 0);
  rb_define_method(cImage, "show", CASTHOOK(lumi_image_show), 0);
  rb_define_method(cImage, "toggle", CASTHOOK(lumi_image_toggle), 0);
  rb_define_method(cImage, "click", CASTHOOK(lumi_image_click), -1);
  rb_define_method(cImage, "release", CASTHOOK(lumi_image_release), -1);
  rb_define_method(cImage, "hover", CASTHOOK(lumi_image_hover), -1);
  rb_define_method(cImage, "leave", CASTHOOK(lumi_image_leave), -1);

  cEffect   = rb_define_class_under(cTypes, "Effect", rb_cObject);
  rb_define_alloc_func(cEffect, lumi_effect_alloc);
  rb_define_method(cEffect, "draw", CASTHOOK(lumi_effect_draw), 2);
  rb_define_method(cEffect, "remove", CASTHOOK(lumi_basic_remove), 0);

#ifdef VIDEO
  cVideo    = rb_define_class_under(cTypes, "Video", rb_cObject);
  rb_define_alloc_func(cVideo, lumi_video_alloc);
  rb_define_method(cVideo, "app", CASTHOOK(lumi_canvas_get_app), 0);
  rb_define_method(cVideo, "displace", CASTHOOK(lumi_video_displace), 2);
  rb_define_method(cVideo, "draw", CASTHOOK(lumi_video_draw), 2);
  rb_define_method(cVideo, "style", CASTHOOK(lumi_video_style), -1);
  rb_define_method(cVideo, "hide", CASTHOOK(lumi_video_hide), 0);
  rb_define_method(cVideo, "show", CASTHOOK(lumi_video_show), 0);
  rb_define_method(cVideo, "move", CASTHOOK(lumi_video_move), 2);
  rb_define_method(cVideo, "parent", CASTHOOK(lumi_video_get_parent), 0);
  rb_define_method(cVideo, "top", CASTHOOK(lumi_video_get_top), 0);
  rb_define_method(cVideo, "left", CASTHOOK(lumi_video_get_left), 0);
  rb_define_method(cVideo, "width", CASTHOOK(lumi_video_get_width), 0);
  rb_define_method(cVideo, "height", CASTHOOK(lumi_video_get_height), 0);
  rb_define_method(cVideo, "remove", CASTHOOK(lumi_video_remove), 0);
  rb_define_method(cVideo, "playing?", CASTHOOK(lumi_video_is_playing), 0);
  rb_define_method(cVideo, "play", CASTHOOK(lumi_video_play), 0);
  rb_define_method(cVideo, "clear", CASTHOOK(lumi_video_clear), 0);
  rb_define_method(cVideo, "previous", CASTHOOK(lumi_video_prev), 0);
  rb_define_method(cVideo, "next", CASTHOOK(lumi_video_next), 0);
  rb_define_method(cVideo, "pause", CASTHOOK(lumi_video_pause), 0);
  rb_define_method(cVideo, "stop", CASTHOOK(lumi_video_stop), 0);
  rb_define_method(cVideo, "length", CASTHOOK(lumi_video_get_length), 0);
  rb_define_method(cVideo, "position", CASTHOOK(lumi_video_get_position), 0);
  rb_define_method(cVideo, "position=", CASTHOOK(lumi_video_set_position), 1);
  rb_define_method(cVideo, "time", CASTHOOK(lumi_video_get_time), 0);
  rb_define_method(cVideo, "time=", CASTHOOK(lumi_video_set_time), 1);
#endif

  cPattern = rb_define_class_under(cTypes, "Pattern", rb_cObject);
  rb_define_alloc_func(cPattern, lumi_pattern_alloc);
  rb_define_method(cPattern, "displace", CASTHOOK(lumi_pattern_displace), 2);
  rb_define_method(cPattern, "move", CASTHOOK(lumi_pattern_move), 2);
  rb_define_method(cPattern, "remove", CASTHOOK(lumi_basic_remove), 0);
  rb_define_method(cPattern, "to_pattern", CASTHOOK(lumi_pattern_self), 0);
  rb_define_method(cPattern, "style", CASTHOOK(lumi_pattern_style), -1);
  rb_define_method(cPattern, "fill", CASTHOOK(lumi_pattern_get_fill), 0);
  rb_define_method(cPattern, "fill=", CASTHOOK(lumi_pattern_set_fill), 1);
  rb_define_method(cPattern, "hide", CASTHOOK(lumi_pattern_hide), 0);
  rb_define_method(cPattern, "show", CASTHOOK(lumi_pattern_show), 0);
  rb_define_method(cPattern, "toggle", CASTHOOK(lumi_pattern_toggle), 0);
  cBackground = rb_define_class_under(cTypes, "Background", cPattern);
  rb_define_method(cBackground, "draw", CASTHOOK(lumi_background_draw), 2);
  cBorder = rb_define_class_under(cTypes, "Border", cPattern);
  rb_define_method(cBorder, "draw", CASTHOOK(lumi_border_draw), 2);

  cTextBlock = rb_define_class_under(cTypes, "TextBlock", rb_cObject);
  rb_define_alloc_func(cTextBlock, lumi_textblock_alloc);
  rb_define_method(cTextBlock, "app", CASTHOOK(lumi_canvas_get_app), 0);
  rb_define_method(cTextBlock, "contents", CASTHOOK(lumi_textblock_children), 0);
  rb_define_method(cTextBlock, "children", CASTHOOK(lumi_textblock_children), 0);
  rb_define_method(cTextBlock, "parent", CASTHOOK(lumi_textblock_get_parent), 0);
  rb_define_method(cTextBlock, "displace", CASTHOOK(lumi_textblock_displace), 2);
  rb_define_method(cTextBlock, "draw", CASTHOOK(lumi_textblock_draw), 2);
  rb_define_method(cTextBlock, "cursor=", CASTHOOK(lumi_textblock_set_cursor), 1);
  rb_define_method(cTextBlock, "cursor", CASTHOOK(lumi_textblock_get_cursor), 0);
  rb_define_method(cTextBlock, "cursor_left", CASTHOOK(lumi_textblock_cursorx), 0);
  rb_define_method(cTextBlock, "cursor_top", CASTHOOK(lumi_textblock_cursory), 0);
  rb_define_method(cTextBlock, "highlight", CASTHOOK(lumi_textblock_get_highlight), 0);
  rb_define_method(cTextBlock, "hit", CASTHOOK(lumi_textblock_hit), 2);
  rb_define_method(cTextBlock, "marker=", CASTHOOK(lumi_textblock_set_marker), 1);
  rb_define_method(cTextBlock, "marker", CASTHOOK(lumi_textblock_get_marker), 0);
  rb_define_method(cTextBlock, "move", CASTHOOK(lumi_textblock_move), 2);
  rb_define_method(cTextBlock, "top", CASTHOOK(lumi_textblock_get_top), 0);
  rb_define_method(cTextBlock, "left", CASTHOOK(lumi_textblock_get_left), 0);
  rb_define_method(cTextBlock, "width", CASTHOOK(lumi_textblock_get_width), 0);
  rb_define_method(cTextBlock, "height", CASTHOOK(lumi_textblock_get_height), 0);
  rb_define_method(cTextBlock, "remove", CASTHOOK(lumi_basic_remove), 0);
  rb_define_method(cTextBlock, "to_s", CASTHOOK(lumi_textblock_string), 0);
  rb_define_method(cTextBlock, "text", CASTHOOK(lumi_textblock_string), 0);
  rb_define_method(cTextBlock, "text=", CASTHOOK(lumi_textblock_replace), -1);
  rb_define_method(cTextBlock, "replace", CASTHOOK(lumi_textblock_replace), -1);
  rb_define_method(cTextBlock, "style", CASTHOOK(lumi_textblock_style_m), -1);
  rb_define_method(cTextBlock, "hide", CASTHOOK(lumi_textblock_hide), 0);
  rb_define_method(cTextBlock, "show", CASTHOOK(lumi_textblock_show), 0);
  rb_define_method(cTextBlock, "toggle", CASTHOOK(lumi_textblock_toggle), 0);
  rb_define_method(cTextBlock, "click", CASTHOOK(lumi_textblock_click), -1);
  rb_define_method(cTextBlock, "release", CASTHOOK(lumi_textblock_release), -1);
  rb_define_method(cTextBlock, "hover", CASTHOOK(lumi_textblock_hover), -1);
  rb_define_method(cTextBlock, "leave", CASTHOOK(lumi_textblock_leave), -1);
  cPara = rb_define_class_under(cTypes, "Para", cTextBlock);
  cBanner = rb_define_class_under(cTypes, "Banner", cTextBlock);
  cTitle = rb_define_class_under(cTypes, "Title", cTextBlock);
  cSubtitle = rb_define_class_under(cTypes, "Subtitle", cTextBlock);
  cTagline = rb_define_class_under(cTypes, "Tagline", cTextBlock);
  cCaption = rb_define_class_under(cTypes, "Caption", cTextBlock);
  cInscription = rb_define_class_under(cTypes, "Inscription", cTextBlock);

  cTextClass = rb_define_class_under(cTypes, "Text", rb_cObject);
  rb_define_alloc_func(cTextClass, lumi_text_alloc);
  rb_define_method(cTextClass, "app", CASTHOOK(lumi_canvas_get_app), 0);
  rb_define_method(cTextClass, "contents", CASTHOOK(lumi_text_children), 0);
  rb_define_method(cTextClass, "children", CASTHOOK(lumi_text_children), 0);
  rb_define_method(cTextClass, "parent", CASTHOOK(lumi_text_parent), 0);
  rb_define_method(cTextClass, "style", CASTHOOK(lumi_text_style), -1);
  rb_define_method(cTextClass, "to_s", CASTHOOK(lumi_text_to_s), 0);
  rb_define_method(cTextClass, "text", CASTHOOK(lumi_text_children), 0);
  rb_define_method(cTextClass, "text=", CASTHOOK(lumi_text_replace), -1);
  rb_define_method(cTextClass, "replace", CASTHOOK(lumi_text_replace), -1);
  cCode      = rb_define_class_under(cTypes, "Code", cTextClass);
  cDel       = rb_define_class_under(cTypes, "Del", cTextClass);
  cEm        = rb_define_class_under(cTypes, "Em", cTextClass);
  cIns       = rb_define_class_under(cTypes, "Ins", cTextClass);
  cSpan      = rb_define_class_under(cTypes, "Span", cTextClass);
  cStrong    = rb_define_class_under(cTypes, "Strong", cTextClass);
  cSub       = rb_define_class_under(cTypes, "Sub", cTextClass);
  cSup       = rb_define_class_under(cTypes, "Sup", cTextClass);

  cLink      = rb_define_class_under(cTypes, "Link", cTextClass);
  rb_define_method(cTextClass, "click", CASTHOOK(lumi_linktext_click), -1);
  rb_define_method(cTextClass, "release", CASTHOOK(lumi_linktext_release), -1);
  rb_define_method(cTextClass, "hover", CASTHOOK(lumi_linktext_hover), -1);
  rb_define_method(cTextClass, "leave", CASTHOOK(lumi_linktext_leave), -1);
  cLinkHover = rb_define_class_under(cTypes, "LinkHover", cTextClass);

  cNative  = rb_define_class_under(cTypes, "Native", rb_cObject);
  rb_define_alloc_func(cNative, lumi_control_alloc);
  rb_define_method(cNative, "app", CASTHOOK(lumi_canvas_get_app), 0);
  rb_define_method(cNative, "parent", CASTHOOK(lumi_control_get_parent), 0);
  rb_define_method(cNative, "style", CASTHOOK(lumi_control_style), -1);
  rb_define_method(cNative, "displace", CASTHOOK(lumi_control_displace), 2);
  rb_define_method(cNative, "focus", CASTHOOK(lumi_control_focus), 0);
  rb_define_method(cNative, "hide", CASTHOOK(lumi_control_hide), 0);
  rb_define_method(cNative, "show", CASTHOOK(lumi_control_show), 0);
  rb_define_method(cNative, "state=", CASTHOOK(lumi_control_set_state), 1);
  rb_define_method(cNative, "state", CASTHOOK(lumi_control_get_state), 0);
  rb_define_method(cNative, "move", CASTHOOK(lumi_control_move), 2);
  rb_define_method(cNative, "top", CASTHOOK(lumi_control_get_top), 0);
  rb_define_method(cNative, "left", CASTHOOK(lumi_control_get_left), 0);
  rb_define_method(cNative, "width", CASTHOOK(lumi_control_get_width), 0);
  rb_define_method(cNative, "height", CASTHOOK(lumi_control_get_height), 0);
  rb_define_method(cNative, "remove", CASTHOOK(lumi_control_remove), 0);
  cButton  = rb_define_class_under(cTypes, "Button", cNative);
  rb_define_method(cButton, "draw", CASTHOOK(lumi_button_draw), 2);
  rb_define_method(cButton, "click", CASTHOOK(lumi_control_click), -1);
  cEditLine  = rb_define_class_under(cTypes, "EditLine", cNative);
  rb_define_method(cEditLine, "text", CASTHOOK(lumi_edit_line_get_text), 0);
  rb_define_method(cEditLine, "text=", CASTHOOK(lumi_edit_line_set_text), 1);
  rb_define_method(cEditLine, "draw", CASTHOOK(lumi_edit_line_draw), 2);
  rb_define_method(cEditLine, "change", CASTHOOK(lumi_control_change), -1);
  cEditBox  = rb_define_class_under(cTypes, "EditBox", cNative);
  rb_define_method(cEditBox, "text", CASTHOOK(lumi_edit_box_get_text), 0);
  rb_define_method(cEditBox, "text=", CASTHOOK(lumi_edit_box_set_text), 1);
  rb_define_method(cEditBox, "draw", CASTHOOK(lumi_edit_box_draw), 2);
  rb_define_method(cEditBox, "change", CASTHOOK(lumi_control_change), -1);
  cListBox  = rb_define_class_under(cTypes, "ListBox", cNative);
  rb_define_method(cListBox, "text", CASTHOOK(lumi_list_box_text), 0);
  rb_define_method(cListBox, "draw", CASTHOOK(lumi_list_box_draw), 2);
  rb_define_method(cListBox, "choose", CASTHOOK(lumi_list_box_choose), 1);
  rb_define_method(cListBox, "change", CASTHOOK(lumi_control_change), -1);
  rb_define_method(cListBox, "items", CASTHOOK(lumi_list_box_items_get), 0);
  rb_define_method(cListBox, "items=", CASTHOOK(lumi_list_box_items_set), 1);
  cProgress  = rb_define_class_under(cTypes, "Progress", cNative);
  rb_define_method(cProgress, "draw", CASTHOOK(lumi_progress_draw), 2);
  rb_define_method(cProgress, "fraction", CASTHOOK(lumi_progress_get_fraction), 0);
  rb_define_method(cProgress, "fraction=", CASTHOOK(lumi_progress_set_fraction), 1);
  cSlider  = rb_define_class_under(cTypes, "Slider", cNative);
  rb_define_method(cSlider, "draw", CASTHOOK(lumi_slider_draw), 2);
  rb_define_method(cSlider, "fraction", CASTHOOK(lumi_slider_get_fraction), 0);
  rb_define_method(cSlider, "fraction=", CASTHOOK(lumi_slider_set_fraction), 1);
  rb_define_method(cSlider, "change", CASTHOOK(lumi_control_change), -1);
  cCheck  = rb_define_class_under(cTypes, "Check", cNative);
  rb_define_method(cCheck, "draw", CASTHOOK(lumi_check_draw), 2);
  rb_define_method(cCheck, "checked?", CASTHOOK(lumi_check_is_checked), 0);
  rb_define_method(cCheck, "checked=", CASTHOOK(lumi_check_set_checked), 1);
  rb_define_method(cCheck, "click", CASTHOOK(lumi_control_click), -1);
  cRadio  = rb_define_class_under(cTypes, "Radio", cNative);
  rb_define_method(cRadio, "draw", CASTHOOK(lumi_radio_draw), 2);
  rb_define_method(cRadio, "checked?", CASTHOOK(lumi_check_is_checked), 0);
  rb_define_method(cRadio, "checked=", CASTHOOK(lumi_check_set_checked_m), 1);
  rb_define_method(cRadio, "click", CASTHOOK(lumi_control_click), -1);

  cTimerBase   = rb_define_class_under(cTypes, "TimerBase", rb_cObject);
  rb_define_alloc_func(cTimerBase, lumi_timer_alloc);
  rb_define_method(cTimerBase, "draw", CASTHOOK(lumi_timer_draw), 2);
  rb_define_method(cTimerBase, "remove", CASTHOOK(lumi_timer_remove), 0);
  rb_define_method(cTimerBase, "start", CASTHOOK(lumi_timer_start), 0);
  rb_define_method(cTimerBase, "stop", CASTHOOK(lumi_timer_stop), 0);
  rb_define_method(cTimerBase, "toggle", CASTHOOK(lumi_timer_toggle), 0);
  cAnim    = rb_define_class_under(cTypes, "Animation", cTimerBase);
  cEvery   = rb_define_class_under(cTypes, "Every", cTimerBase);
  cTimer   = rb_define_class_under(cTypes, "Timer", cTimerBase);

  cColor   = rb_define_class_under(cTypes, "Color", rb_cObject);
  rb_define_alloc_func(cColor, lumi_color_alloc);
  rb_define_method(rb_mKernel, "rgb", CASTHOOK(lumi_color_rgb), -1);
  rb_define_method(rb_mKernel, "gray", CASTHOOK(lumi_color_gray), -1);
  rb_define_singleton_method(cColor, "rgb", CASTHOOK(lumi_color_rgb), -1);
  rb_define_singleton_method(cColor, "gray", CASTHOOK(lumi_color_gray), -1);
  rb_define_singleton_method(cColor, "parse", CASTHOOK(lumi_color_parse), 1);
  rb_include_module(cColor, rb_mComparable);
  rb_define_method(cColor, "<=>", CASTHOOK(lumi_color_spaceship), 1);
  rb_define_method(cColor, "red", CASTHOOK(lumi_color_get_red), 0);
  rb_define_method(cColor, "green", CASTHOOK(lumi_color_get_green), 0);
  rb_define_method(cColor, "blue", CASTHOOK(lumi_color_get_blue), 0);
  rb_define_method(cColor, "alpha", CASTHOOK(lumi_color_get_alpha), 0);
  rb_define_method(cColor, "black?", CASTHOOK(lumi_color_is_black), 0);
  rb_define_method(cColor, "dark?", CASTHOOK(lumi_color_is_dark), 0);
  rb_define_method(cColor, "inspect", CASTHOOK(lumi_color_to_s), 0);
  rb_define_method(cColor, "invert", CASTHOOK(lumi_color_invert), 0);
  rb_define_method(cColor, "light?", CASTHOOK(lumi_color_is_light), 0);
  rb_define_method(cColor, "opaque?", CASTHOOK(lumi_color_is_opaque), 0);
  rb_define_method(cColor, "to_s", CASTHOOK(lumi_color_to_s), 0);
  rb_define_method(cColor, "to_pattern", CASTHOOK(lumi_color_to_pattern), 0);
  rb_define_method(cColor, "transparent?", CASTHOOK(lumi_color_is_transparent), 0);
  rb_define_method(cColor, "white?", CASTHOOK(lumi_color_is_white), 0);

  cDownload   = rb_define_class_under(cTypes, "Download", rb_cObject);
  rb_define_alloc_func(cDownload, lumi_http_alloc);
  rb_define_method(cDownload, "abort", CASTHOOK(lumi_http_abort), 0);
  rb_define_method(cDownload, "finish", CASTHOOK(lumi_http_finish), -1);
  rb_define_method(cDownload, "remove", CASTHOOK(lumi_http_remove), 0);
  rb_define_method(cDownload, "length", CASTHOOK(lumi_http_length), 0);
  rb_define_method(cDownload, "percent", CASTHOOK(lumi_http_percent), 0);
  rb_define_method(cDownload, "progress", CASTHOOK(lumi_http_progress), -1);
  rb_define_method(cDownload, "response", CASTHOOK(lumi_http_response), 0);
  rb_define_method(cDownload, "size", CASTHOOK(lumi_http_length), 0);
  rb_define_method(cDownload, "start", CASTHOOK(lumi_http_start), -1);
  rb_define_method(cDownload, "transferred", CASTHOOK(lumi_http_transferred), 0);

  cResponse   = rb_define_class_under(cDownload, "Response", rb_cObject);
  rb_define_method(cResponse, "body", CASTHOOK(lumi_response_body), 0);
  rb_define_method(cResponse, "headers", CASTHOOK(lumi_response_headers), 0);
  rb_define_method(cResponse, "status", CASTHOOK(lumi_response_status), 0);
  rb_define_method(cResponse, "text", CASTHOOK(lumi_response_body), 0);

  rb_define_method(cCanvas, "method_missing", CASTHOOK(lumi_color_method_missing), -1);
  rb_define_method(cApp, "method_missing", CASTHOOK(lumi_app_method_missing), -1);

  rb_const_set(cTypes, rb_intern("ALL_NAMED_COLORS"), rb_hash_new());
  cColors = rb_const_get(cTypes, rb_intern("ALL_NAMED_COLORS"));
  rb_const_set(cTypes, rb_intern("COLORS"), cColors);
  DEF_COLOR(aliceblue, 240, 248, 255);
  DEF_COLOR(antiquewhite, 250, 235, 215);
  DEF_COLOR(aqua, 0, 255, 255);
  DEF_COLOR(aquamarine, 127, 255, 212);
  DEF_COLOR(azure, 240, 255, 255);
  DEF_COLOR(beige, 245, 245, 220);
  DEF_COLOR(bisque, 255, 228, 196);
  DEF_COLOR(black, 0, 0, 0);
  DEF_COLOR(blanchedalmond, 255, 235, 205);
  DEF_COLOR(blue, 0, 0, 255);
  DEF_COLOR(blueviolet, 138, 43, 226);
  DEF_COLOR(brown, 165, 42, 42);
  DEF_COLOR(burlywood, 222, 184, 135);
  DEF_COLOR(cadetblue, 95, 158, 160);
  DEF_COLOR(chartreuse, 127, 255, 0);
  DEF_COLOR(chocolate, 210, 105, 30);
  DEF_COLOR(coral, 255, 127, 80);
  DEF_COLOR(cornflowerblue, 100, 149, 237);
  DEF_COLOR(cornsilk, 255, 248, 220);
  DEF_COLOR(crimson, 220, 20, 60);
  DEF_COLOR(cyan, 0, 255, 255);
  DEF_COLOR(darkblue, 0, 0, 139);
  DEF_COLOR(darkcyan, 0, 139, 139);
  DEF_COLOR(darkgoldenrod, 184, 134, 11);
  DEF_COLOR(darkgray, 169, 169, 169);
  DEF_COLOR(darkgreen, 0, 100, 0);
  DEF_COLOR(darkkhaki, 189, 183, 107);
  DEF_COLOR(darkmagenta, 139, 0, 139);
  DEF_COLOR(darkolivegreen, 85, 107, 47);
  DEF_COLOR(darkorange, 255, 140, 0);
  DEF_COLOR(darkorchid, 153, 50, 204);
  DEF_COLOR(darkred, 139, 0, 0);
  DEF_COLOR(darksalmon, 233, 150, 122);
  DEF_COLOR(darkseagreen, 143, 188, 143);
  DEF_COLOR(darkslateblue, 72, 61, 139);
  DEF_COLOR(darkslategray, 47, 79, 79);
  DEF_COLOR(darkturquoise, 0, 206, 209);
  DEF_COLOR(darkviolet, 148, 0, 211);
  DEF_COLOR(deeppink, 255, 20, 147);
  DEF_COLOR(deepskyblue, 0, 191, 255);
  DEF_COLOR(dimgray, 105, 105, 105);
  DEF_COLOR(dodgerblue, 30, 144, 255);
  DEF_COLOR(firebrick, 178, 34, 34);
  DEF_COLOR(floralwhite, 255, 250, 240);
  DEF_COLOR(forestgreen, 34, 139, 34);
  DEF_COLOR(fuchsia, 255, 0, 255);
  DEF_COLOR(gainsboro, 220, 220, 220);
  DEF_COLOR(ghostwhite, 248, 248, 255);
  DEF_COLOR(gold, 255, 215, 0);
  DEF_COLOR(goldenrod, 218, 165, 32);
  DEF_COLOR(gray, 128, 128, 128);
  DEF_COLOR(green, 0, 128, 0);
  DEF_COLOR(greenyellow, 173, 255, 47);
  DEF_COLOR(honeydew, 240, 255, 240);
  DEF_COLOR(hotpink, 255, 105, 180);
  DEF_COLOR(indianred, 205, 92, 92);
  DEF_COLOR(indigo, 75, 0, 130);
  DEF_COLOR(ivory, 255, 255, 240);
  DEF_COLOR(khaki, 240, 230, 140);
  DEF_COLOR(lavender, 230, 230, 250);
  DEF_COLOR(lavenderblush, 255, 240, 245);
  DEF_COLOR(lawngreen, 124, 252, 0);
  DEF_COLOR(lemonchiffon, 255, 250, 205);
  DEF_COLOR(lightblue, 173, 216, 230);
  DEF_COLOR(lightcoral, 240, 128, 128);
  DEF_COLOR(lightcyan, 224, 255, 255);
  DEF_COLOR(lightgoldenrodyellow, 250, 250, 210);
  DEF_COLOR(lightgreen, 144, 238, 144);
  DEF_COLOR(lightgrey, 211, 211, 211);
  DEF_COLOR(lightpink, 255, 182, 193);
  DEF_COLOR(lightsalmon, 255, 160, 122);
  DEF_COLOR(lightseagreen, 32, 178, 170);
  DEF_COLOR(lightskyblue, 135, 206, 250);
  DEF_COLOR(lightslategray, 119, 136, 153);
  DEF_COLOR(lightsteelblue, 176, 196, 222);
  DEF_COLOR(lightyellow, 255, 255, 224);
  DEF_COLOR(lime, 0, 255, 0);
  DEF_COLOR(limegreen, 50, 205, 50);
  DEF_COLOR(linen, 250, 240, 230);
  DEF_COLOR(magenta, 255, 0, 255);
  DEF_COLOR(maroon, 128, 0, 0);
  DEF_COLOR(mediumaquamarine, 102, 205, 170);
  DEF_COLOR(mediumblue, 0, 0, 205);
  DEF_COLOR(mediumorchid, 186, 85, 211);
  DEF_COLOR(mediumpurple, 147, 112, 219);
  DEF_COLOR(mediumseagreen, 60, 179, 113);
  DEF_COLOR(mediumslateblue, 123, 104, 238);
  DEF_COLOR(mediumspringgreen, 0, 250, 154);
  DEF_COLOR(mediumturquoise, 72, 209, 204);
  DEF_COLOR(mediumvioletred, 199, 21, 133);
  DEF_COLOR(midnightblue, 25, 25, 112);
  DEF_COLOR(mintcream, 245, 255, 250);
  DEF_COLOR(mistyrose, 255, 228, 225);
  DEF_COLOR(moccasin, 255, 228, 181);
  DEF_COLOR(navajowhite, 255, 222, 173);
  DEF_COLOR(navy, 0, 0, 128);
  DEF_COLOR(oldlace, 253, 245, 230);
  DEF_COLOR(olive, 128, 128, 0);
  DEF_COLOR(olivedrab, 107, 142, 35);
  DEF_COLOR(orange, 255, 165, 0);
  DEF_COLOR(orangered, 255, 69, 0);
  DEF_COLOR(orchid, 218, 112, 214);
  DEF_COLOR(palegoldenrod, 238, 232, 170);
  DEF_COLOR(palegreen, 152, 251, 152);
  DEF_COLOR(paleturquoise, 175, 238, 238);
  DEF_COLOR(palevioletred, 219, 112, 147);
  DEF_COLOR(papayawhip, 255, 239, 213);
  DEF_COLOR(peachpuff, 255, 218, 185);
  DEF_COLOR(peru, 205, 133, 63);
  DEF_COLOR(pink, 255, 192, 203);
  DEF_COLOR(plum, 221, 160, 221);
  DEF_COLOR(powderblue, 176, 224, 230);
  DEF_COLOR(purple, 128, 0, 128);
  DEF_COLOR(red, 255, 0, 0);
  DEF_COLOR(rosybrown, 188, 143, 143);
  DEF_COLOR(royalblue, 65, 105, 225);
  DEF_COLOR(saddlebrown, 139, 69, 19);
  DEF_COLOR(salmon, 250, 128, 114);
  DEF_COLOR(sandybrown, 244, 164, 96);
  DEF_COLOR(seagreen, 46, 139, 87);
  DEF_COLOR(seashell, 255, 245, 238);
  DEF_COLOR(sienna, 160, 82, 45);
  DEF_COLOR(silver, 192, 192, 192);
  DEF_COLOR(skyblue, 135, 206, 235);
  DEF_COLOR(slateblue, 106, 90, 205);
  DEF_COLOR(slategray, 112, 128, 144);
  DEF_COLOR(snow, 255, 250, 250);
  DEF_COLOR(springgreen, 0, 255, 127);
  DEF_COLOR(steelblue, 70, 130, 180);
  DEF_COLOR(tan, 210, 180, 140);
  DEF_COLOR(teal, 0, 128, 128);
  DEF_COLOR(thistle, 216, 191, 216);
  DEF_COLOR(tomato, 255, 99, 71);
  DEF_COLOR(turquoise, 64, 224, 208);
  DEF_COLOR(violet, 238, 130, 238);
  DEF_COLOR(wheat, 245, 222, 179);
  DEF_COLOR(white, 255, 255, 255);
  DEF_COLOR(whitesmoke, 245, 245, 245);
  DEF_COLOR(yellow, 255, 255, 0);
  DEF_COLOR(yellowgreen, 154, 205, 50);

  cLinkUrl = rb_define_class_under(cTypes, "LinkUrl", rb_cObject);

  rb_define_method(rb_mKernel, "alert", CASTHOOK(lumi_dialog_alert), 1);
  rb_define_method(rb_mKernel, "ask", CASTHOOK(lumi_dialog_ask), -1);
  rb_define_method(rb_mKernel, "confirm", CASTHOOK(lumi_dialog_confirm), 1);
  rb_define_method(rb_mKernel, "ask_color", CASTHOOK(lumi_dialog_color), 1);
  rb_define_method(rb_mKernel, "ask_open_file", CASTHOOK(lumi_dialog_open), -1);
  rb_define_method(rb_mKernel, "ask_save_file", CASTHOOK(lumi_dialog_save), -1);
  rb_define_method(rb_mKernel, "ask_open_folder", CASTHOOK(lumi_dialog_open_folder), -1);
  rb_define_method(rb_mKernel, "ask_save_folder", CASTHOOK(lumi_dialog_save_folder), -1);
  rb_define_method(rb_mKernel, "font", CASTHOOK(lumi_font), 1);
}

