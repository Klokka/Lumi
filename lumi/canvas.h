//
// lumi/canvas.h
// Ruby methods for all the drawing ops.
//
#ifndef LUMI_CANVAS_H
#define LUMI_CANVAS_H

#include <cairo.h>
#include <cairo-svg.h>
#include <cairo-ps.h>
#include <cairo-pdf.h>

#include <pango/pangocairo.h>
#include <ruby.h>

#include "lumi/config.h"
#include "lumi/code.h"

struct _lumi_app;

typedef unsigned int PIXEL;

extern const double RAD2PI, PIM2, PI;
extern const char *dialog_title, *dialog_title_says;

#define REL_WINDOW  1
#define REL_CANVAS  2
#define REL_CURSOR  3
#define REL_TILE    4
#define REL_STICKY  5
#define REL_SCALE   8

#define REL_COORDS(x) (x & 0x07)
#define REL_FLAGS(x)  (x & 0xF8)

#define FLAG_POSITION 0x0F
#define FLAG_ABSX     0x10
#define FLAG_ABSY     0x20
#define FLAG_ORIGIN   0x40

#define HOVER_MOTION  0x01
#define HOVER_CLICK   0x02

//
// affine transforms, to avoid littering these structs everywhere
//
typedef struct {
  cairo_matrix_t tf;
  ID mode;
  int refs;
} lumi_transform;

//
// place struct
// (outlines the area where a control has been placed)
//
typedef struct {
  int x, y, w, h, dx, dy;
  int ix, iy, iw, ih;
  unsigned char flags;
} lumi_place;

#define SETUP_BASIC() \
  lumi_basic *basic; \
  Data_Get_Struct(self, lumi_basic, basic);
#define COPY_PENS(attr1, attr2) \
  if (NIL_P(ATTR(attr1, stroke))) ATTRSET(attr1, stroke, ATTR(attr2, stroke)); \
  if (NIL_P(ATTR(attr1, fill)))   ATTRSET(attr1, fill, ATTR(attr2, fill)); \
  if (NIL_P(ATTR(attr1, strokewidth))) ATTRSET(attr1, strokewidth, ATTR(attr2, strokewidth)); \
  if (NIL_P(ATTR(attr1, cap))) ATTRSET(attr1, cap, ATTR(attr2, cap));
#define DRAW(c, app, blk) \
  { \
    rb_ary_push(app->nesting, c); \
    blk; \
    rb_ary_pop(app->nesting); \
  }
#define PATTERN_DIM(self_t, x) (self_t->cached != NULL ? self_t->cached->x : 1)
#define PATTERN(self_t) (self_t->cached != NULL ? self_t->cached->pattern : self_t->pattern)
#define ABSX(place)   ((place).flags & FLAG_ABSX)
#define ABSY(place)   ((place).flags & FLAG_ABSY)
#define POS(place)    ((place).flags & FLAG_POSITION)
#define ORIGIN(place) ((place).flags & FLAG_ORIGIN)
#define CPX(c)  (c != NULL && (c->place.flags & FLAG_ORIGIN) ? 0 : c->place.ix)
#define CPY(c)  (c != NULL && (c->place.flags & FLAG_ORIGIN) ? 0 : c->place.iy)
#define CPB(c)  ((c->place.h - c->place.ih) - (c->place.iy - c->place.y))
#define CPH(c)  (ORIGIN(c->place) ? c->height : (c->fully - CPB(c)) - CPY(c))
#define CPW(c)  (c->place.iw)
#define CCR(c)  (c->cr == NULL ? c->app->scratch : c->cr)
#define SWPOS(x) ((int)sw % 2 == 0 ? x * 1. : x + .5)

//
// color struct
//
typedef struct {
  unsigned char r, g, b, a, on;
} lumi_color;

#define LUMI_COLOR_OPAQUE 0xFF
#define LUMI_COLOR_TRANSPARENT 0x0
#define LUMI_COLOR_DARK   (0x55 * 3)
#define LUMI_COLOR_LIGHT  (0xAA * 3)

//
// basic struct
//
typedef struct {
  VALUE parent;
  VALUE attr;
} lumi_basic;

typedef struct {
  VALUE parent;
  VALUE attr;
  lumi_place place;
} lumi_element;

//
// shape struct
//
typedef struct {
  VALUE parent;
  VALUE attr;
  lumi_place place;
  ID name;
  char hover;
  cairo_path_t *line;
  lumi_transform *st;
} lumi_shape;

//
// flow struct
//
typedef struct {
  VALUE parent;
  VALUE attr;
  VALUE contents;
} lumi_flow;

//
// link struct
//
typedef struct {
  int start;
  int end;
  VALUE ele;
} lumi_link;

//
// text cursor
//
typedef struct {
  int pos, x, y, hi;
} lumi_textcursor;

//
// text block struct
//
typedef struct {
  VALUE parent;
  VALUE attr;
  lumi_place place;
  VALUE texts;
  VALUE links;
  lumi_textcursor *cursor;
  PangoLayout *layout;
  PangoAttrList *pattr;
  GString *text;
  guint len;
  char cached, hover;
  lumi_transform *st;
} lumi_textblock;

//
// text struct
//
typedef struct {
  VALUE parent;
  VALUE attr;
  VALUE texts;
  char hover;
} lumi_text;

//
// cached image
//
typedef enum {
  LUMI_IMAGE_NONE,
  LUMI_IMAGE_PNG,
  LUMI_IMAGE_JPEG,
  LUMI_IMAGE_GIF
} lumi_image_format;

typedef struct {
  cairo_surface_t *surface;
  cairo_pattern_t *pattern;
  int width, height, mtime;
  lumi_image_format format;
} lumi_cached_image;

#define LUMI_CACHE_FILE  0
#define LUMI_CACHE_ALIAS 1
#define LUMI_CACHE_MEM   2

typedef struct {
  unsigned char type;
  lumi_cached_image *image;
} lumi_cache_entry;

//
// image struct
//
#define LUMI_IMAGE_EXPIRE (60 * 60)
typedef struct {
  VALUE parent;
  VALUE attr;
  lumi_place place;
  unsigned char type;
  lumi_cached_image *cached;
  lumi_transform *st;
  cairo_t *cr;
  VALUE path;
  char hover;
} lumi_image;

#define LUMI_VIDEO 0

//
// pattern struct
//
typedef struct {
  VALUE parent;
  VALUE attr;
  lumi_place place;
  VALUE source;
  char hover;
  lumi_cached_image *cached;
  cairo_pattern_t *pattern;
} lumi_pattern;

//
// native controls struct
//
#define CONTROL_NORMAL   0
#define CONTROL_READONLY 1
#define CONTROL_DISABLED 2

typedef struct {
  VALUE parent;
  VALUE attr;
  lumi_place place;
  LUMI_CONTROL_REF ref;
} lumi_control;

#define ANIM_NADA    0
#define ANIM_STARTED 1
#define ANIM_PAUSED  2
#define ANIM_STOPPED 3

//
// animation struct
//
typedef struct {
  VALUE parent;
  VALUE block;
  unsigned int rate, frame;
  char started;
  LUMI_TIMER_REF ref;
} lumi_timer;

typedef void (*lumi_effect_filter)(cairo_t *, VALUE attr, lumi_place *);

typedef struct {
  VALUE parent;
  VALUE attr;
  lumi_place place;
  lumi_effect_filter filter;
} lumi_effect;

typedef struct {
  VALUE parent;
  VALUE attr;
  VALUE response;
  unsigned char state;
  unsigned LONG_LONG total;
  unsigned LONG_LONG transferred;
  unsigned long percent;
} lumi_http_klass;

#define CANVAS_NADA    0
#define CANVAS_STARTED 1
#define CANVAS_PAINT   2
#define CANVAS_EMPTY   3
#define CANVAS_REMOVED 4
 
//
// temporary canvas (used internally for painting)
//
typedef struct {
  VALUE parent;
  VALUE attr;
  lumi_place place;
  cairo_t *cr, *shape;
  lumi_transform *st, **sts;
  int stl, stt;
  VALUE contents;
  unsigned char stage;
  long insertion;
  int cx, cy;               // cursor x and y (stored in absolute coords)
  int endx, endy;           // jump points if the cursor spills over
  int topy, fully;          // since we often stack vertically
  int width, height;        // the full height and width used by this box
  char hover;
  struct _lumi_app *app;
  LUMI_SLOT_OS *slot;
  LUMI_GROUP_OS group;
} lumi_canvas;

void lumi_control_hide_ref(LUMI_CONTROL_REF);
void lumi_control_show_ref(LUMI_CONTROL_REF);

VALUE lumi_app_main(int, VALUE *, VALUE);
VALUE lumi_app_window(int, VALUE *, VALUE, VALUE);
VALUE lumi_app_contents(VALUE);
VALUE lumi_app_get_width(VALUE);
VALUE lumi_app_get_height(VALUE);

VALUE lumi_basic_remove(VALUE);

lumi_transform *lumi_transform_new(lumi_transform *);
lumi_transform *lumi_transform_touch(lumi_transform *);
lumi_transform *lumi_transform_detach(lumi_transform *);
void lumi_transform_release(lumi_transform *);

VALUE lumi_canvas_info (VALUE, VALUE);
VALUE lumi_canvas_debug(VALUE, VALUE);
VALUE lumi_canvas_warn (VALUE, VALUE);
VALUE lumi_canvas_error(VALUE, VALUE);
void lumi_info (const char *fmt, ...);
void lumi_debug(const char *fmt, ...);
void lumi_warn (const char *fmt, ...);
void lumi_error(const char *fmt, ...);

void lumi_canvas_mark(lumi_canvas *);
VALUE lumi_canvas_alloc(VALUE);
VALUE lumi_canvas_new(VALUE, struct _lumi_app *);
void lumi_canvas_clear(VALUE);
lumi_canvas *lumi_canvas_init(VALUE, LUMI_SLOT_OS *, VALUE, int, int);
void lumi_slot_scroll_to(lumi_canvas *, int, int);
void lumi_canvas_paint(VALUE);
void lumi_apply_transformation(cairo_t *, lumi_transform *, lumi_place *, unsigned char);
void lumi_undo_transformation(cairo_t *, lumi_transform *, lumi_place *, unsigned char);
void lumi_canvas_shape_do(lumi_canvas *, double, double, double, double, unsigned char);
VALUE lumi_canvas_style(int, VALUE *, VALUE);
VALUE lumi_canvas_owner(VALUE);
VALUE lumi_canvas_close(VALUE);
VALUE lumi_canvas_get_top(VALUE);
VALUE lumi_canvas_get_left(VALUE);
VALUE lumi_canvas_get_width(VALUE);
VALUE lumi_canvas_get_height(VALUE);
VALUE lumi_canvas_get_scroll_height(VALUE);
VALUE lumi_canvas_get_scroll_max(VALUE);
VALUE lumi_canvas_get_scroll_top(VALUE);
VALUE lumi_canvas_set_scroll_top(VALUE, VALUE);
VALUE lumi_canvas_get_gutter_width(VALUE);
VALUE lumi_canvas_displace(VALUE, VALUE, VALUE);
VALUE lumi_canvas_move(VALUE, VALUE, VALUE);
VALUE lumi_canvas_nostroke(VALUE);
VALUE lumi_canvas_stroke(int, VALUE *, VALUE);
VALUE lumi_canvas_strokewidth(VALUE, VALUE);
VALUE lumi_canvas_dash(VALUE, VALUE);
VALUE lumi_canvas_cap(VALUE, VALUE);
VALUE lumi_canvas_nofill(VALUE);
VALUE lumi_canvas_fill(int, VALUE *, VALUE);
VALUE lumi_canvas_rgb(int, VALUE *, VALUE);
VALUE lumi_canvas_gray(int, VALUE *, VALUE);
VALUE lumi_canvas_rect(int, VALUE *, VALUE);
VALUE lumi_canvas_arc(int, VALUE *, VALUE);
VALUE lumi_canvas_oval(int, VALUE *, VALUE);
VALUE lumi_canvas_line(int, VALUE *, VALUE);
VALUE lumi_canvas_arrow(int, VALUE *, VALUE);
VALUE lumi_canvas_star(int, VALUE *, VALUE);
VALUE lumi_canvas_para(int argc, VALUE *argv, VALUE self);
VALUE lumi_canvas_banner(int argc, VALUE *argv, VALUE self);
VALUE lumi_canvas_title(int argc, VALUE *argv, VALUE self);
VALUE lumi_canvas_subtitle(int argc, VALUE *argv, VALUE self);
VALUE lumi_canvas_tagline(int argc, VALUE *argv, VALUE self);
VALUE lumi_canvas_caption(int argc, VALUE *argv, VALUE self);
VALUE lumi_canvas_inscription(int argc, VALUE *argv, VALUE self);
VALUE lumi_canvas_code(int argc, VALUE *argv, VALUE self);
VALUE lumi_canvas_del(int argc, VALUE *argv, VALUE self);
VALUE lumi_canvas_em(int argc, VALUE *argv, VALUE self);
VALUE lumi_canvas_ins(int argc, VALUE *argv, VALUE self);
VALUE lumi_canvas_link(int argc, VALUE *argv, VALUE self);
VALUE lumi_canvas_span(int argc, VALUE *argv, VALUE self);
VALUE lumi_canvas_strong(int argc, VALUE *argv, VALUE self);
VALUE lumi_canvas_sub(int argc, VALUE *argv, VALUE self);
VALUE lumi_canvas_sup(int argc, VALUE *argv, VALUE self);
VALUE lumi_canvas_background(int, VALUE *, VALUE);
VALUE lumi_canvas_border(int, VALUE *, VALUE);
VALUE lumi_canvas_video(int, VALUE *, VALUE);
VALUE lumi_canvas_blur(int, VALUE *, VALUE);
VALUE lumi_canvas_glow(int, VALUE *, VALUE);
VALUE lumi_canvas_shadow(int, VALUE *, VALUE);
VALUE lumi_canvas_image(int, VALUE *, VALUE);
VALUE lumi_canvas_animate(int, VALUE *, VALUE);
VALUE lumi_canvas_every(int, VALUE *, VALUE);
VALUE lumi_canvas_timer(int, VALUE *, VALUE);
VALUE lumi_canvas_imagesize(VALUE, VALUE);
VALUE lumi_canvas_shape(int, VALUE *, VALUE);
void lumi_canvas_remove_item(VALUE, VALUE, char, char);
VALUE lumi_canvas_move_to(VALUE, VALUE, VALUE);
VALUE lumi_canvas_line_to(VALUE, VALUE, VALUE);
VALUE lumi_canvas_curve_to(VALUE, VALUE, VALUE, VALUE, VALUE, VALUE, VALUE);
VALUE lumi_canvas_arc_to(VALUE, VALUE, VALUE, VALUE, VALUE, VALUE, VALUE);
VALUE lumi_canvas_transform(VALUE, VALUE);
VALUE lumi_canvas_translate(VALUE, VALUE, VALUE);
VALUE lumi_canvas_rotate(VALUE, VALUE);
VALUE lumi_canvas_scale(int, VALUE *, VALUE);
VALUE lumi_canvas_skew(int, VALUE *, VALUE);
VALUE lumi_canvas_push(VALUE);
VALUE lumi_canvas_pop(VALUE);
VALUE lumi_canvas_reset(VALUE);
VALUE lumi_canvas_button(int, VALUE *, VALUE);
VALUE lumi_canvas_list_box(int, VALUE *, VALUE);
VALUE lumi_canvas_edit_line(int, VALUE *, VALUE);
VALUE lumi_canvas_edit_box(int, VALUE *, VALUE);
VALUE lumi_canvas_progress(int, VALUE *, VALUE);
VALUE lumi_canvas_slider(int, VALUE *, VALUE);
VALUE lumi_canvas_check(int, VALUE *, VALUE);
VALUE lumi_canvas_radio(int, VALUE *, VALUE);
VALUE lumi_canvas_contents(VALUE);
VALUE lumi_canvas_children(VALUE);
void lumi_canvas_size(VALUE, int, int);
VALUE lumi_canvas_clear_contents(int, VALUE *, VALUE);
VALUE lumi_canvas_remove(VALUE);
VALUE lumi_canvas_draw(VALUE, VALUE, VALUE);
VALUE lumi_canvas_after(int, VALUE *, VALUE);
VALUE lumi_canvas_before(int, VALUE *, VALUE);
VALUE lumi_canvas_append(int, VALUE *, VALUE);
VALUE lumi_canvas_prepend(int, VALUE *, VALUE);
VALUE lumi_canvas_flow(int, VALUE *, VALUE);
VALUE lumi_canvas_stack(int, VALUE *, VALUE);
VALUE lumi_canvas_mask(int, VALUE *, VALUE);
VALUE lumi_canvas_widget(int, VALUE *, VALUE);
VALUE lumi_canvas_hide(VALUE);
VALUE lumi_canvas_show(VALUE);
VALUE lumi_canvas_toggle(VALUE);
VALUE lumi_canvas_mouse(VALUE);
VALUE lumi_canvas_start(int, VALUE *, VALUE);
VALUE lumi_canvas_finish(int, VALUE *, VALUE);
VALUE lumi_canvas_hover(int, VALUE *, VALUE);
VALUE lumi_canvas_leave(int, VALUE *, VALUE);
VALUE lumi_canvas_click(int, VALUE *, VALUE);
VALUE lumi_canvas_release(int, VALUE *, VALUE);
VALUE lumi_canvas_motion(int, VALUE *, VALUE);
VALUE lumi_canvas_keydown(int, VALUE *, VALUE);
VALUE lumi_canvas_keypress(int, VALUE *, VALUE);
VALUE lumi_canvas_keyup(int, VALUE *, VALUE);
int lumi_canvas_independent(lumi_canvas *);
VALUE lumi_find_canvas(VALUE);
VALUE lumi_canvas_get_app(VALUE);
void lumi_canvas_repaint_all(VALUE);
void lumi_canvas_compute(VALUE);
VALUE lumi_canvas_goto(VALUE, VALUE);
VALUE lumi_canvas_send_click(VALUE, int, int, int);
void lumi_canvas_send_release(VALUE, int, int, int);
VALUE lumi_canvas_send_motion(VALUE, int, int, VALUE);
void lumi_canvas_send_wheel(VALUE, ID, int, int);
void lumi_canvas_send_keydown(VALUE, VALUE);
void lumi_canvas_send_keypress(VALUE, VALUE);
void lumi_canvas_send_keyup(VALUE, VALUE);
VALUE lumi_canvas_get_cursor(VALUE);
VALUE lumi_canvas_set_cursor(VALUE, VALUE);
VALUE lumi_canvas_get_clipboard(VALUE);
VALUE lumi_canvas_set_clipboard(VALUE, VALUE);
VALUE lumi_canvas_window(int, VALUE *, VALUE);
VALUE lumi_canvas_dialog(int, VALUE *, VALUE);
VALUE lumi_canvas_window_plain(VALUE);
VALUE lumi_canvas_dialog_plain(VALUE);
VALUE lumi_canvas_snapshot(int, VALUE *, VALUE);
VALUE lumi_canvas_download(int, VALUE *, VALUE);

LUMI_SLOT_OS *lumi_slot_alloc(lumi_canvas *, LUMI_SLOT_OS *, int);
VALUE lumi_slot_new(VALUE, VALUE, VALUE);
VALUE lumi_flow_new(VALUE, VALUE);
VALUE lumi_stack_new(VALUE, VALUE);
VALUE lumi_mask_new(VALUE, VALUE);
VALUE lumi_widget_new(VALUE, VALUE, VALUE);

void lumi_control_mark(lumi_control *);
VALUE lumi_control_new(VALUE, VALUE, VALUE);
VALUE lumi_control_alloc(VALUE);
void lumi_control_send(VALUE, ID);
VALUE lumi_control_get_top(VALUE);
VALUE lumi_control_get_left(VALUE);
VALUE lumi_control_get_width(VALUE);
VALUE lumi_control_get_height(VALUE);
VALUE lumi_control_remove(VALUE);
VALUE lumi_control_show(VALUE);
VALUE lumi_control_temporary_show(VALUE);
VALUE lumi_control_hide(VALUE);
VALUE lumi_control_temporary_hide(VALUE);
VALUE lumi_control_focus(VALUE);
VALUE lumi_control_get_state(VALUE);
VALUE lumi_control_set_state(VALUE, VALUE);

VALUE lumi_button_draw(VALUE, VALUE, VALUE);
void lumi_button_send_click(VALUE);
VALUE lumi_edit_line_draw(VALUE, VALUE, VALUE);
VALUE lumi_edit_line_get_text(VALUE);
VALUE lumi_edit_line_set_text(VALUE, VALUE);
VALUE lumi_edit_box_draw(VALUE, VALUE, VALUE);
VALUE lumi_edit_box_get_text(VALUE);
VALUE lumi_edit_box_set_text(VALUE, VALUE);
VALUE lumi_list_box_text(VALUE);
VALUE lumi_list_box_draw(VALUE, VALUE, VALUE);
VALUE lumi_progress_draw(VALUE, VALUE, VALUE);

void lumi_shape_mark(lumi_shape *);
VALUE lumi_shape_attr(int, VALUE *, int, ...);
void lumi_shape_sketch(cairo_t *, ID, lumi_place *, lumi_transform *, VALUE, cairo_path_t *, unsigned char);
VALUE lumi_shape_new(VALUE, ID, VALUE, lumi_transform *, cairo_path_t *);
VALUE lumi_shape_alloc(VALUE);
VALUE lumi_shape_draw(VALUE, VALUE, VALUE);
VALUE lumi_shape_move(VALUE, VALUE, VALUE);
VALUE lumi_shape_get_top(VALUE);
VALUE lumi_shape_get_left(VALUE);
VALUE lumi_shape_get_width(VALUE);
VALUE lumi_shape_get_height(VALUE);
VALUE lumi_shape_motion(VALUE, int, int, char *);
VALUE lumi_shape_send_click(VALUE, int, int, int);
void lumi_shape_send_release(VALUE, int, int, int);

void lumi_image_ensure_dup(lumi_image *);
void lumi_image_mark(lumi_image *);
VALUE lumi_image_new(VALUE, VALUE, VALUE, VALUE, lumi_transform *);
VALUE lumi_image_alloc(VALUE);
void lumi_image_image(VALUE, VALUE, VALUE);
VALUE lumi_image_draw(VALUE, VALUE, VALUE);
VALUE lumi_image_get_top(VALUE);
VALUE lumi_image_get_left(VALUE);
VALUE lumi_image_get_width(VALUE);
VALUE lumi_image_get_height(VALUE);
VALUE lumi_image_motion(VALUE, int, int, char *);
VALUE lumi_image_send_click(VALUE, int, int, int);
void lumi_image_send_release(VALUE, int, int, int);

lumi_effect_filter lumi_effect_for_type(ID);
void lumi_effect_mark(lumi_effect *);
VALUE lumi_effect_new(ID, VALUE, VALUE);
VALUE lumi_effect_alloc(VALUE);
VALUE lumi_effect_draw(VALUE, VALUE, VALUE);

VALUE lumi_pattern_self(VALUE);
VALUE lumi_pattern_method(VALUE, VALUE);
VALUE lumi_pattern_args(int, VALUE *, VALUE);
void lumi_pattern_mark(lumi_pattern *);
VALUE lumi_pattern_new(VALUE, VALUE, VALUE, VALUE);
VALUE lumi_pattern_alloc(VALUE);
VALUE lumi_pattern_motion(VALUE, int, int, char *);
VALUE lumi_background_draw(VALUE, VALUE, VALUE);
VALUE lumi_border_draw(VALUE, VALUE, VALUE);
VALUE lumi_subpattern_new(VALUE, VALUE, VALUE);

void lumi_timer_mark(lumi_timer *);
VALUE lumi_timer_new(VALUE, VALUE, VALUE, VALUE);
VALUE lumi_timer_alloc(VALUE);
VALUE lumi_timer_init(VALUE, VALUE);
VALUE lumi_timer_remove(VALUE);
VALUE lumi_timer_start(VALUE);
VALUE lumi_timer_stop(VALUE);
void lumi_timer_call(VALUE);

void lumi_color_mark(lumi_color *);
VALUE lumi_color_new(int, int, int, int);
VALUE lumi_color_alloc(VALUE);
VALUE lumi_color_rgb(int, VALUE *, VALUE);
VALUE lumi_color_gray(int, VALUE *, VALUE);
cairo_pattern_t *lumi_color_pattern(VALUE);
void lumi_color_grad_stop(cairo_pattern_t *, double, VALUE);
VALUE lumi_color_args(int, VALUE *, VALUE);
VALUE lumi_color_parse(VALUE, VALUE);
VALUE lumi_color_is_black(VALUE);
VALUE lumi_color_is_dark(VALUE);
VALUE lumi_color_is_light(VALUE);
VALUE lumi_color_is_white(VALUE);
VALUE lumi_color_invert(VALUE);
VALUE lumi_color_to_s(VALUE);
VALUE lumi_color_to_pattern(VALUE);
VALUE lumi_color_gradient(int, VALUE *, VALUE);

void lumi_link_mark(lumi_link *);
VALUE lumi_link_new(VALUE, int, int);
VALUE lumi_link_alloc(VALUE);
VALUE lumi_text_new(VALUE, VALUE, VALUE);
VALUE lumi_text_alloc(VALUE);

void lumi_text_mark(lumi_text *);
void lumi_textblock_mark(lumi_textblock *);
VALUE lumi_textblock_new(VALUE, VALUE, VALUE, VALUE, lumi_transform *);
VALUE lumi_textblock_alloc(VALUE);
VALUE lumi_textblock_get_top(VALUE);
VALUE lumi_textblock_get_left(VALUE);
VALUE lumi_textblock_get_width(VALUE);
VALUE lumi_textblock_get_height(VALUE);
VALUE lumi_textblock_set_cursor(VALUE, VALUE);
VALUE lumi_textblock_get_cursor(VALUE);
VALUE lumi_textblock_draw(VALUE, VALUE, VALUE);
VALUE lumi_textblock_motion(VALUE, int, int, char *);
VALUE lumi_textblock_send_click(VALUE, int, int, int, VALUE *);
void lumi_textblock_send_release(VALUE, int, int, int);

void lumi_http_mark(lumi_http_klass *);
VALUE lumi_http_new(VALUE, VALUE, VALUE);
VALUE lumi_http_alloc(VALUE);
VALUE lumi_http_threaded(VALUE, VALUE, VALUE);
int lumi_message_download(VALUE, void *);
int lumi_catch_message(unsigned int name, VALUE obj, void *data);

VALUE lumi_response_new(VALUE, int);
VALUE lumi_response_body(VALUE);
VALUE lumi_response_headers(VALUE);
VALUE lumi_response_status(VALUE);

VALUE lumi_p(VALUE, VALUE);

extern const double LUMI_PIM2, LUMI_PI, LUMI_RAD2PI, LUMI_HALFPI;

//
// lumi/image.c
//
typedef struct {
  unsigned long status;
  char *cachepath, *filepath, *uripath, *etag;
  char hexdigest[42];
  VALUE slot;
} lumi_image_download_event;

lumi_code lumi_load_imagesize(VALUE, int *, int *);
lumi_cached_image *lumi_cached_image_new(int, int, cairo_surface_t *);
lumi_cached_image *lumi_load_image(VALUE, VALUE);
unsigned char lumi_image_downloaded(lumi_image_download_event *);

#endif
