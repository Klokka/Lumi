//
// lumi/native.h
// Common native Lumi routines.
//
#define CHANGED_COORDS() \
  (p1->ix != p2->ix || p1->iy != p2->iy || \
   p1->iw != p2->iw || p1->dx != p2->dx || \
   p1->dy != p2->dy || p1->ih - HEIGHT_PAD != p2->ih)
#define CHANGED_COORDS_NO_PAD() \
  (p1->ix != p2->ix || p1->iy != p2->iy || \
   p1->iw != p2->iw || p1->dx != p2->dx || \
   p1->dy != p2->dy || p1->ih != p2->ih)
#define PLACE_COORDS() p2->h -= HEIGHT_PAD; p2->ih -= HEIGHT_PAD; *p1 = *p2
#define PLACE_COORDS_NO_PAD() *p1 = *p2

#ifndef LUMI_GTK
#define LUMI_FORCE_RADIO 1
#endif

#define LUMI_THREAD_DOWNLOAD 41
#define LUMI_IMAGE_DOWNLOAD  42
#define LUMI_MAX_MESSAGE     100

VALUE lumi_font_list(void);
VALUE lumi_load_font(const char *);
void lumi_native_init(void);
void lumi_native_cleanup(lumi_world_t *world);
void lumi_native_quit(void);
int lumi_throw_message(unsigned int, VALUE, void *);
void lumi_native_slot_mark(LUMI_SLOT_OS *);
void lumi_native_slot_reset(LUMI_SLOT_OS *);
void lumi_native_slot_clear(lumi_canvas *);
void lumi_native_slot_paint(LUMI_SLOT_OS *);
void lumi_native_slot_lengthen(LUMI_SLOT_OS *, int, int);
void lumi_native_slot_scroll_top(LUMI_SLOT_OS *);
int lumi_native_slot_gutter(LUMI_SLOT_OS *);
void lumi_native_remove_item(LUMI_SLOT_OS *, VALUE, char);
lumi_code lumi_app_cursor(lumi_app *, ID);
void lumi_native_app_resized(lumi_app *);
void lumi_native_app_title(lumi_app *, char *);
void lumi_native_app_fullscreen(lumi_app *, char);
lumi_code lumi_native_app_open(lumi_app *, char *, int);
void lumi_native_app_show(lumi_app *);
void lumi_native_loop(void);
void lumi_native_app_close(lumi_app *);
void lumi_browser_open(char *);
void lumi_slot_init(VALUE, LUMI_SLOT_OS *, int, int, int, int, int, int);
cairo_t *lumi_cairo_create(lumi_canvas *);
void lumi_slot_destroy(lumi_canvas *, lumi_canvas *);
void lumi_cairo_destroy(lumi_canvas *);
void lumi_group_clear(LUMI_GROUP_OS *);
void lumi_native_canvas_place(lumi_canvas *, lumi_canvas *);
void lumi_native_canvas_resize(lumi_canvas *);
void lumi_native_control_hide(LUMI_CONTROL_REF);
void lumi_native_control_show(LUMI_CONTROL_REF);
void lumi_native_control_position(LUMI_CONTROL_REF, lumi_place *, 
  VALUE, lumi_canvas *, lumi_place *);
void lumi_native_control_position_no_pad(LUMI_CONTROL_REF, lumi_place *, 
  VALUE, lumi_canvas *, lumi_place *);
void lumi_native_control_repaint(LUMI_CONTROL_REF, lumi_place *,
  lumi_canvas *, lumi_place *);
void lumi_native_control_repaint_no_pad(LUMI_CONTROL_REF, lumi_place *,
  lumi_canvas *, lumi_place *);
void lumi_native_control_focus(LUMI_CONTROL_REF);
void lumi_native_control_state(LUMI_CONTROL_REF, LUMI_BOOL, LUMI_BOOL);
void lumi_native_control_remove(LUMI_CONTROL_REF, lumi_canvas *);
void lumi_native_control_free(LUMI_CONTROL_REF);
LUMI_SURFACE_REF lumi_native_surface_new(lumi_canvas *, VALUE, lumi_place *);
void lumi_native_surface_position(LUMI_SURFACE_REF, lumi_place *, 
  VALUE, lumi_canvas *, lumi_place *);
void lumi_native_surface_hide(LUMI_SURFACE_REF);
void lumi_native_surface_show(LUMI_SURFACE_REF);
void lumi_native_surface_remove(lumi_canvas *, LUMI_SURFACE_REF);
LUMI_CONTROL_REF lumi_native_button(VALUE, lumi_canvas *, lumi_place *, char *);
LUMI_CONTROL_REF lumi_native_edit_line(VALUE, lumi_canvas *, lumi_place *, VALUE, char *);
VALUE lumi_native_edit_line_get_text(LUMI_CONTROL_REF);
void lumi_native_edit_line_set_text(LUMI_CONTROL_REF, char *);
LUMI_CONTROL_REF lumi_native_edit_box(VALUE, lumi_canvas *, lumi_place *, VALUE, char *);
VALUE lumi_native_edit_box_get_text(LUMI_CONTROL_REF);
void lumi_native_edit_box_set_text(LUMI_CONTROL_REF, char *);
LUMI_CONTROL_REF lumi_native_list_box(VALUE, lumi_canvas *, lumi_place *, VALUE, char *);
void lumi_native_list_box_update(LUMI_CONTROL_REF, VALUE);
VALUE lumi_native_list_box_get_active(LUMI_CONTROL_REF, VALUE);
LUMI_CONTROL_REF lumi_native_progress(VALUE, lumi_canvas *, lumi_place *, VALUE, char *);
double lumi_native_progress_get_fraction(LUMI_CONTROL_REF);
void lumi_native_progress_set_fraction(LUMI_CONTROL_REF, double);
LUMI_CONTROL_REF lumi_native_slider(VALUE, lumi_canvas *, lumi_place *, VALUE, char *);
double lumi_native_slider_get_fraction(LUMI_CONTROL_REF);
void lumi_native_slider_set_fraction(LUMI_CONTROL_REF, double);
LUMI_CONTROL_REF lumi_native_check(VALUE, lumi_canvas *, lumi_place *, VALUE, char *);
VALUE lumi_native_check_get(LUMI_CONTROL_REF);
void lumi_native_check_set(LUMI_CONTROL_REF, int);
void lumi_native_list_box_set_active(LUMI_CONTROL_REF, VALUE, VALUE);
LUMI_CONTROL_REF lumi_native_radio(VALUE, lumi_canvas *, lumi_place *, VALUE, VALUE);
void lumi_native_timer_remove(lumi_canvas *, LUMI_TIMER_REF);
LUMI_TIMER_REF lumi_native_timer_start(VALUE, lumi_canvas *, unsigned int);
VALUE lumi_native_clipboard_get(lumi_app *);
void lumi_native_clipboard_set(lumi_app *, VALUE);
VALUE lumi_native_to_s(VALUE);
char *lumi_native_to_utf8(VALUE, int *);
VALUE lumi_native_window_color(lumi_app *);
VALUE lumi_native_dialog_color(lumi_app *);
VALUE lumi_dialog_alert(VALUE, VALUE);
VALUE lumi_dialog_ask(int argc, VALUE *argv, VALUE self);
VALUE lumi_dialog_confirm(VALUE, VALUE);
VALUE lumi_dialog_color(VALUE, VALUE);
VALUE lumi_dialog_open(int,VALUE*,VALUE);
VALUE lumi_dialog_save(int,VALUE*,VALUE);
VALUE lumi_dialog_open_folder(int,VALUE*,VALUE);
VALUE lumi_dialog_save_folder(int,VALUE*,VALUE);
