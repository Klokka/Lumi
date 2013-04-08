//
// lumi/native-cocoa.m
// ObjC Cocoa-specific code for Lumi.
//
#include "lumi/app.h"
#include "lumi/ruby.h"
#include "lumi/config.h"
#include "lumi/world.h"
#include "lumi/native.h"
#include "lumi/internal.h"
#include "lumi/http.h"

#define HEIGHT_PAD 6

#define INIT    NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init]
#define RELEASE [pool release]
#define COCOA_DO(statements) do {\
  INIT; \
  @try { statements; } \
  @catch (NSException *e) { ; } \
  RELEASE; \
} while (0)

@implementation LumiEvents
- (id)init
{
  if ((self = [super init]))
    count = 0;
  return self;
}
- (void)idle: (NSTimer *)t
{
  if (count < 100)
  {
    count++;
    if (count == 100 && RARRAY_LEN(lumi_world->apps) == 0)
      rb_eval_string("Lumi.splash");
  }
  rb_eval_string("sleep(0.001)");
}
- (BOOL) application: (NSApplication *) anApplication
    openFile: (NSString *) aFileName
{
  lumi_load([aFileName UTF8String]);

  return YES;
}
- (void)openFile: (id)sender
{
  rb_eval_string("Lumi.show_selector");
}
- (void)package: (id)sender
{
  rb_eval_string("Lumi.make_pack");
}
- (void)showLog: (id)sender
{
  rb_eval_string("Lumi.show_log");
}
- (void)emulateKey: (NSString *)key modifierFlags: (unsigned int)flags withoutModifiers: (NSString *)key2
{
  LumiWindow *win = [NSApp keyWindow];
  [win keyDown: [NSEvent keyEventWithType:NSKeyDown
    location:NSMakePoint(0,0) modifierFlags:flags
    timestamp:0 windowNumber:0 context:nil
    characters:key charactersIgnoringModifiers:key2 isARepeat:NO 
    keyCode:0]];
}
- (void)help: (id)sender
{
  rb_eval_string("Lumi.show_manual");
}
- (void)undo: (id)sender
{
  [self emulateKey: @"z" modifierFlags: NSCommandKeyMask withoutModifiers: @"z"];
}
- (void)redo: (id)sender
{
  [self emulateKey: @"Z" modifierFlags: NSCommandKeyMask|NSShiftKeyMask withoutModifiers: @"z"];
}
- (void)cut: (id)sender
{
  [self emulateKey: @"x" modifierFlags: NSCommandKeyMask withoutModifiers: @"x"];
}
- (void)copy: (id)sender
{
  [self emulateKey: @"c" modifierFlags: NSCommandKeyMask withoutModifiers: @"c"];
}
- (void)paste: (id)sender
{
  [self emulateKey: @"v" modifierFlags: NSCommandKeyMask withoutModifiers: @"v"];
}
- (void)selectAll: (id)sender
{
  [self emulateKey: @"a" modifierFlags: NSCommandKeyMask withoutModifiers: @"a"];
}
@end

@implementation LumiWindow
- (void)prepareWithApp: (VALUE)a
{
  app = a;
  [self center];
  [self makeKeyAndOrderFront: self];
  [self setAcceptsMouseMovedEvents: YES];
  [self setAutorecalculatesKeyViewLoop: YES];
  [self setDelegate: self];
}
- (void)disconnectApp
{
  app = Qnil;
}
- (void)sendMotion: (NSEvent *)e ofType: (ID)type withButton: (int)b
{
  lumi_app *a;
  lumi_canvas *canvas;
  NSPoint p = [e locationInWindow];
  Data_Get_Struct(app, lumi_app, a);
  Data_Get_Struct(a->canvas, lumi_canvas, canvas);
  if (type == s_motion)
    lumi_app_motion(a, ROUND(p.x), (canvas->height - ROUND(p.y)) + canvas->slot->scrolly);
  else if (type == s_click)
    lumi_app_click(a, b, ROUND(p.x), (canvas->height - ROUND(p.y)) + canvas->slot->scrolly);
  else if (type == s_release)
    lumi_app_release(a, b, ROUND(p.x), (canvas->height - ROUND(p.y)) + canvas->slot->scrolly);
}
- (void)mouseDown: (NSEvent *)e
{
  [self sendMotion: e ofType: s_click withButton: 1];
}
- (void)rightMouseDown: (NSEvent *)e
{
  [self sendMotion: e ofType: s_click withButton: 2];
}
- (void)otherMouseDown: (NSEvent *)e
{
  [self sendMotion: e ofType: s_click withButton: 3];
}
- (void)mouseUp: (NSEvent *)e
{
  [self sendMotion: e ofType: s_release withButton: 1];
}
- (void)rightMouseUp: (NSEvent *)e
{
  [self sendMotion: e ofType: s_release withButton: 2];
}
- (void)otherMouseUp: (NSEvent *)e
{
  [self sendMotion: e ofType: s_release withButton: 3];
}
- (void)mouseMoved: (NSEvent *)e
{
  [self sendMotion: e ofType: s_motion withButton: 0];
}
- (void)mouseDragged: (NSEvent *)e
{
  [self sendMotion: e ofType: s_motion withButton: 0];
}
- (void)rightMouseDragged: (NSEvent *)e
{
  [self sendMotion: e ofType: s_motion withButton: 0];
}
- (void)otherMouseDragged: (NSEvent *)e
{
  [self sendMotion: e ofType: s_motion withButton: 0];
}
- (void)scrollWheel: (NSEvent *)e
{
  ID wheel;
  CGFloat dy = [e deltaY];
  NSPoint p = [e locationInWindow];
  lumi_app *a;

  if (dy == 0)
    return;
  else if (dy > 0)
    wheel = s_up;
  else
  {
    wheel = s_down;
    dy = -dy;
  }

  Data_Get_Struct(app, lumi_app, a);
  for (; dy > 0.; dy--)
    lumi_app_wheel(a, wheel, ROUND(p.x), ROUND(p.y));
}
- (void)keyDown: (NSEvent *)e
{
  lumi_app *a;
  VALUE v = Qnil;
  NSUInteger modifier = [e modifierFlags];
  unsigned short key = [e keyCode];
  INIT;

  Data_Get_Struct(app, lumi_app, a);
  KEY_SYM(ESCAPE, escape)
  KEY_SYM(INSERT, insert)
  KEY_SYM(DELETE, delete)
  KEY_SYM(TAB, tab)
  KEY_SYM(BS, backspace)
  KEY_SYM(PRIOR, page_up)
  KEY_SYM(NEXT, page_down)
  KEY_SYM(HOME, home)
  KEY_SYM(END, end)
  KEY_SYM(LEFT, left)
  KEY_SYM(UP, up)
  KEY_SYM(RIGHT, right)
  KEY_SYM(DOWN, down)
  KEY_SYM(F1, f1)
  KEY_SYM(F2, f2)
  KEY_SYM(F3, f3)
  KEY_SYM(F4, f4)
  KEY_SYM(F5, f5)
  KEY_SYM(F6, f6)
  KEY_SYM(F7, f7)
  KEY_SYM(F8, f8)
  KEY_SYM(F9, f9)
  KEY_SYM(F10, f10)
  KEY_SYM(F11, f11)
  KEY_SYM(F12, f12)
  {
    NSString *str = [e charactersIgnoringModifiers];
    if (str)
    {
      char *utf8 = [str UTF8String];
      if (utf8[0] == '\r' && [str length] == 1)
        v = rb_str_new2("\n");
      else
        v = rb_str_new2(utf8);
    }
  }

  if (SYMBOL_P(v))
  {
    if ((modifier & NSCommandKeyMask) || (modifier & NSAlternateKeyMask))
      KEY_STATE(alt);
    if (modifier & NSShiftKeyMask)
      KEY_STATE(shift);
    if (modifier & NSControlKeyMask)
      KEY_STATE(control);
  }
  else
  {
    if ((modifier & NSCommandKeyMask) || (modifier & NSAlternateKeyMask))
      KEY_STATE(alt);
  }

  if (v != Qnil)
    lumi_app_keypress(a, v);
  RELEASE;
}
- (BOOL)canBecomeKeyWindow
{
  return YES;
}
- (BOOL)canBecomeMainWindow
{
  return YES;
}
- (void)windowWillClose: (NSNotification *)n
{
  if (!NIL_P(app)) {
    lumi_app *a;
    Data_Get_Struct(app, lumi_app, a);
    lumi_app_remove(a);
  }
}
@end

@implementation LumiView
- (id)initWithFrame: (NSRect)frame andCanvas: (VALUE)c
{
  if ((self = [super initWithFrame: frame]))
  {
    canvas = c;
  }
  return self;
}
- (BOOL)isFlipped
{
  return YES;
}
- (void)drawRect: (NSRect)rect
{
  lumi_canvas *c;
  NSRect bounds = [self bounds];
  Data_Get_Struct(canvas, lumi_canvas, c);

  c->width = ROUND(bounds.size.width);
  c->height = ROUND(bounds.size.height);
  if (c->slot->vscroll)
  {
    [c->slot->vscroll setFrame: NSMakeRect(c->width - [NSScroller scrollerWidth], 0,
      [NSScroller scrollerWidth], c->height)];
    lumi_native_slot_lengthen(c->slot, c->height, c->endy);
  }
  c->place.iw = c->place.w = c->width;
  c->place.ih = c->place.h = c->height;
  c->slot->context = (CGContextRef)[[NSGraphicsContext currentContext] graphicsPort];
  lumi_canvas_paint(canvas);
}
- (void)scroll: (NSScroller *)scroller
{
  lumi_canvas *c;
  Data_Get_Struct(canvas, lumi_canvas, c);

  switch ([scroller hitPart])
  {
    case NSScrollerIncrementLine:
      lumi_slot_scroll_to(c, 16, 1);
    break;
    case NSScrollerDecrementLine:
      lumi_slot_scroll_to(c, -16, 1);
    break;
    case NSScrollerIncrementPage:
      lumi_slot_scroll_to(c, c->height - 32, 1);
    break;
    case NSScrollerDecrementPage:
      lumi_slot_scroll_to(c, -(c->height - 32), 1);
    break;
    case NSScrollerKnobSlot:
    case NSScrollerKnob:
    default:
      lumi_slot_scroll_to(c, (c->endy - c->height) * [scroller floatValue], 0);
    break;
  }
}
@end

@implementation LumiButton
- (id)initWithType: (NSButtonType)t andObject: (VALUE)o
{
  if ((self = [super init]))
  {
    object = o;
    [self setButtonType: t];
    [self setBezelStyle: NSRoundedBezelStyle];
    [self setTarget: self];
    [self setAction: @selector(handleClick:)];
  }
  return self;
}
-(IBAction)handleClick: (id)sender
{
  lumi_button_send_click(object);
}
@end

@implementation LumiTextField
- (id)initWithFrame: (NSRect)frame andObject: (VALUE)o
{
  if ((self = [super initWithFrame: frame]))
  {
    object = o;
    [self setBezelStyle: NSRegularSquareBezelStyle];
    [self setDelegate: self];
  }
  return self;
}
-(void)textDidChange: (NSNotification *)n
{
  lumi_control_send(object, s_change);
}
@end

@implementation LumiSecureTextField
- (id)initWithFrame: (NSRect)frame andObject: (VALUE)o
{
  if ((self = [super initWithFrame: frame]))
  {
    object = o;
    [self setBezelStyle: NSRegularSquareBezelStyle];
    [self setDelegate: self];
  }
  return self;
}
-(void)textDidChange: (NSNotification *)n
{
  lumi_control_send(object, s_change);
}
@end

@implementation LumiTextView
- (id)initWithFrame: (NSRect)frame andObject: (VALUE)o
{
  if ((self = [super initWithFrame: frame]))
  {
    object = o;
    textView = [[NSTextView alloc] initWithFrame:
      NSMakeRect(0, 0, frame.size.width, frame.size.height)];
    [textView setVerticallyResizable: YES];
    [textView setHorizontallyResizable: YES];
    
    [self setBorderType: NSBezelBorder];
    [self setHasVerticalScroller: YES];
    [self setHasHorizontalScroller: NO];
    [self setDocumentView: textView];
    [textView setDelegate: self];
  }
  return self;
}
-(NSTextStorage *)textStorage
{
  return [textView textStorage];
}
-(void)textDidChange: (NSNotification *)n
{
  lumi_control_send(object, s_change);
}
@end

@implementation LumiPopUpButton
- (id)initWithFrame: (NSRect)frame andObject: (VALUE)o
{
  if ((self = [super initWithFrame: frame pullsDown: NO]))
  {
    object = o;
    [self setTarget: self];
    [self setAction: @selector(handleChange:)];
  }
  return self;
}
-(IBAction)handleChange: (id)sender
{
  lumi_control_send(object, s_change);
}
@end

@implementation LumiSlider
- (id)initWithObject: (VALUE)o
{
  if ((self = [super init]))
  {
    object = o;
    [self setTarget: self];
    [self setAction: @selector(handleChange:)];
  }
  return self;
}
-(IBAction)handleChange: (id)sender
{
  lumi_control_send(object, s_change);
}
@end

@implementation LumiAlert
- (id)init
{
  if ((self = [super initWithContentRect: NSMakeRect(0, 0, 340, 140)
    styleMask: NSTitledWindowMask backing: NSBackingStoreBuffered defer: NO]))
  {
    answer = FALSE;
    [self setDelegate: self];
  }
  return self;
}
-(IBAction)cancelClick: (id)sender
{
  [[NSApplication sharedApplication] stopModal];
}
-(IBAction)okClick: (id)sender
{
  answer = TRUE;
  [[NSApplication sharedApplication] stopModal];
}
- (void)windowWillClose: (NSNotification *)n
{
  [[NSApplication sharedApplication] stopModal];
}
- (BOOL)accepted
{
  return answer;
}
@end

@implementation LumiTimer
- (id)initWithTimeInterval: (NSTimeInterval)i andObject: (VALUE)o repeats: (BOOL)r
{
  if ((self = [super init]))
  {
    object = o;
    timer = [NSTimer scheduledTimerWithTimeInterval: i
      target: self selector: @selector(animate:) userInfo: self
      repeats: r];
  }
  return self;
}
- (void)animate: (NSTimer *)t
{
  lumi_timer_call(object);
}
- (void)invalidate
{
  [timer invalidate];
}
@end

void
add_to_menubar(NSMenu *main, NSMenu *menu)
{
    NSMenuItem *dummyItem = [[NSMenuItem alloc] initWithTitle:@""
        action:nil keyEquivalent:@""];
    [dummyItem setSubmenu:menu];
    [main addItem:dummyItem];
    [dummyItem release];
}

void
create_apple_menu(NSMenu *main)
{
    NSMenuItem *menuitem;
    // Create the application (Apple) menu.
    NSMenu *menuApp = [[NSMenu alloc] initWithTitle: @"Apple Menu"];

    NSMenu *menuServices = [[NSMenu alloc] initWithTitle: @"Services"];
    [NSApp setServicesMenu:menuServices];

    menuitem = [menuApp addItemWithTitle:@"Open..."
        action:@selector(openFile:) keyEquivalent:@"o"];
    [menuitem setTarget: lumi_world->os.events];
    menuitem = [menuApp addItemWithTitle:@"Package..."
        action:@selector(package:) keyEquivalent:@"P"];
    [menuitem setTarget: lumi_world->os.events];
    [menuApp addItemWithTitle:@"Preferences..." action:nil keyEquivalent:@""];
    [menuApp addItem: [NSMenuItem separatorItem]];
    menuitem = [[NSMenuItem alloc] initWithTitle: @"Services"
        action:nil keyEquivalent:@""];
    [menuitem setSubmenu:menuServices];
    [menuApp addItem: menuitem];
    [menuitem release];
    [menuApp addItem: [NSMenuItem separatorItem]];
    menuitem = [[NSMenuItem alloc] initWithTitle:@"Hide"
        action:@selector(hide:) keyEquivalent:@""];
    [menuitem setTarget: NSApp];
    [menuApp addItem: menuitem];
    [menuitem release];
    menuitem = [[NSMenuItem alloc] initWithTitle:@"Hide Others"
        action:@selector(hideOtherApplications:) keyEquivalent:@""];
    [menuitem setTarget: NSApp];
    [menuApp addItem: menuitem];
    [menuitem release];
    menuitem = [[NSMenuItem alloc] initWithTitle:@"Show All"
        action:@selector(unhideAllApplications:) keyEquivalent:@""];
    [menuitem setTarget: NSApp];
    [menuApp addItem: menuitem];
    [menuitem release];
    [menuApp addItem: [NSMenuItem separatorItem]];
    menuitem = [[NSMenuItem alloc] initWithTitle:@"Quit"
        action:@selector(terminate:) keyEquivalent:@"q"];
    [menuitem setTarget: NSApp];
    [menuApp addItem: menuitem];
    [menuitem release];

    [NSApp setAppleMenu:menuApp];
    add_to_menubar(main, menuApp);
    [menuApp release];
}

void
create_edit_menu(NSMenu *main)
{
    NSMenuItem *menuitem;
    NSMenu *menuEdit = [[NSMenu alloc] initWithTitle: @"Edit"];

    menuitem = [menuEdit addItemWithTitle:@"Undo"
        action:@selector(undo:) keyEquivalent:@"z"];
    [menuitem setTarget: lumi_world->os.events];
    menuitem = [menuEdit addItemWithTitle:@"Redo"
        action:@selector(redo:) keyEquivalent:@"Z"];
    [menuitem setTarget: lumi_world->os.events];
    [menuEdit addItem: [NSMenuItem separatorItem]];
    menuitem = [menuEdit addItemWithTitle:@"Cut"
        action:@selector(cut:) keyEquivalent:@"x"];
    [menuitem setTarget: lumi_world->os.events];
    menuitem = [menuEdit addItemWithTitle:@"Copy"
        action:@selector(copy:) keyEquivalent:@"c"];
    [menuitem setTarget: lumi_world->os.events];
    menuitem = [menuEdit addItemWithTitle:@"Paste"
        action:@selector(paste:) keyEquivalent:@"v"];
    [menuitem setTarget: lumi_world->os.events];
    menuitem = [menuEdit addItemWithTitle:@"Select All"
        action:@selector(selectAll:) keyEquivalent:@"a"];
    [menuitem setTarget: lumi_world->os.events];
    add_to_menubar(main, menuEdit);
    [menuEdit release];
}

void
create_window_menu(NSMenu *main)
{   
    NSMenu *menuWindows = [[NSMenu alloc] initWithTitle: @"Window"];

    [menuWindows addItemWithTitle:@"Minimize"
        action:@selector(performMiniaturize:) keyEquivalent:@""];
    [menuWindows addItemWithTitle:@"Close current Window"
        action:@selector(performClose:) keyEquivalent:@"w"];
    [menuWindows addItem: [NSMenuItem separatorItem]];
    [menuWindows addItemWithTitle:@"Bring All to Front"
        action:@selector(arrangeInFront:) keyEquivalent:@""];

    [NSApp setWindowsMenu:menuWindows];
    add_to_menubar(main, menuWindows);
    [menuWindows release];
}

void
create_help_menu(NSMenu *main)
{   
    NSMenuItem *menuitem;
    NSMenu *menuHelp = [[NSMenu alloc] initWithTitle: @"Help"];
    menuitem = [menuHelp addItemWithTitle:@"Console"
        action:@selector(showLog:) keyEquivalent:@"/"];
    [menuitem setTarget: lumi_world->os.events];
    [menuHelp addItem: [NSMenuItem separatorItem]];
    menuitem = [menuHelp addItemWithTitle:@"Manual"
        action:@selector(help:) keyEquivalent:@"m"];
    [menuitem setTarget: lumi_world->os.events];
    add_to_menubar(main, menuHelp);
    [menuHelp release];
}

VALUE
lumi_font_list()
{
  INIT;
  ATSFontIterator fi = NULL;
  ATSFontRef fontRef = 0;
  NSMutableArray *outArray;
  VALUE ary = rb_ary_new(); 
  if (noErr == ATSFontIteratorCreate(kATSFontContextLocal, nil, nil,
         kATSOptionFlagsUnRestrictedScope, &fi))
  {
    while (noErr == ATSFontIteratorNext(fi, &fontRef))
    {
      NSString *fontName;
      ATSFontGetName(fontRef, kATSOptionFlagsDefault, &fontName);
      if (fontName != NULL)
        rb_ary_push(ary, rb_str_new2([fontName UTF8String]));
    }
  }
  
  ATSFontIteratorRelease(&fi);
  RELEASE;
  rb_funcall(ary, rb_intern("uniq!"), 0);
  rb_funcall(ary, rb_intern("sort!"), 0);
  return ary;
}

VALUE
lumi_load_font(const char *filename)
{
  FSRef fsRef;
  FSSpec fsSpec;
  Boolean isDir;
  VALUE families = Qnil;
  ATSFontContainerRef ref;
  NSString *fontName;
  FSPathMakeRef(filename, &fsRef, &isDir);
  if (FSGetCatalogInfo(&fsRef, kFSCatInfoNone, NULL, NULL, &fsSpec, NULL) == noErr)
  {
    ATSFontActivateFromFileReference(&fsRef, kATSFontContextLocal, kATSFontFormatUnspecified, 
      NULL, kATSOptionFlagsDefault, &ref);
    if (ref != NULL)
    {
      int i = 0;
      ItemCount count = 0;
      ATSFontRef *fonts;
      ATSFontFindFromContainer(ref, kATSOptionFlagsDefault, 0, NULL, &count); 
      families = rb_ary_new();
      if (count > 0)
      {
        fonts = SHOE_ALLOC_N(ATSFontRef, count);
        ATSFontFindFromContainer(ref, kATSOptionFlagsDefault, count, fonts, &count); 
        for (i = 0; i < count; i++)
        {
          fontName = NULL;
          ATSFontGetName(fonts[i], kATSOptionFlagsDefault, &fontName);
          if (fontName != NULL)
            rb_ary_push(families, rb_str_new2([fontName UTF8String]));
        }
        SHOE_FREE(fonts);
      }
    }
  }

  lumi_update_fonts(lumi_font_list());
  return families;
}

void lumi_native_init()
{
  INIT;
  NSTimer *idle;
  NSApplication *NSApp = [NSApplication sharedApplication];
  NSMenu *main = [[NSMenu alloc] initWithTitle: @""];
  lumi_world->os.events = [[LumiEvents alloc] init];
  [NSApp setMainMenu: main];
  create_apple_menu(main);
  create_edit_menu(main);
  create_window_menu(main);
  create_help_menu(main);
  [NSApp setDelegate: lumi_world->os.events];

  idle = [NSTimer scheduledTimerWithTimeInterval: 0.01
    target: lumi_world->os.events selector: @selector(idle:) userInfo: nil
    repeats: YES];
  [[NSRunLoop currentRunLoop] addTimer: idle forMode: NSEventTrackingRunLoopMode];
  RELEASE;
}

void lumi_native_cleanup(lumi_world_t *world)
{
  INIT;
  [lumi_world->os.events release];
  RELEASE;
}

void lumi_native_quit()
{
  INIT;
  NSApplication *NSApp = [NSApplication sharedApplication];
  [NSApp stop: nil];
  RELEASE;
}

void lumi_get_time(LUMI_TIME *ts)
{
  gettimeofday(ts, NULL);
}

unsigned long lumi_diff_time(LUMI_TIME *start, LUMI_TIME *end)
{
  unsigned long usec;
  if ((end->tv_usec-start->tv_usec)<0) {
    usec = (end->tv_sec-start->tv_sec - 1) * 1000;
    usec += (1000000 + end->tv_usec - start->tv_usec) / 1000;
  } else {
    usec = (end->tv_sec - start->tv_sec) * 1000;
    usec += (end->tv_usec - start->tv_usec) / 1000;
  }
  return usec;
}

int lumi_throw_message(unsigned int name, VALUE obj, void *data)
{
  return lumi_catch_message(name, obj, data);
}

void lumi_native_slot_mark(LUMI_SLOT_OS *slot)
{
  rb_gc_mark_maybe(slot->controls);
}

void lumi_native_slot_reset(LUMI_SLOT_OS *slot)
{
  slot->controls = rb_ary_new();
  rb_gc_register_address(&slot->controls);
}

void lumi_native_slot_clear(lumi_canvas *canvas)
{
  rb_ary_clear(canvas->slot->controls);
  if (canvas->slot->vscroll)
  {
    lumi_native_slot_lengthen(canvas->slot, canvas->height, 1);
  }
}

void lumi_native_slot_paint(LUMI_SLOT_OS *slot)
{
  [slot->view setNeedsDisplay: YES];
}

void lumi_native_slot_lengthen(LUMI_SLOT_OS *slot, int height, int endy)
{
  if (slot->vscroll)
  {
    double s = slot->scrolly * 1., e = endy * 1., h = height * 1., d = (endy - height) * 1.;
    COCOA_DO({
      [slot->vscroll setDoubleValue: (d > 0 ? s / d : 0)];
      [slot->vscroll setKnobProportion: (h / e)];
      [slot->vscroll setHidden: endy <= height ? YES : NO];
    });
  }
}

void lumi_native_slot_scroll_top(LUMI_SLOT_OS *slot)
{
}

int lumi_native_slot_gutter(LUMI_SLOT_OS *slot)
{
  return (int)[NSScroller scrollerWidth];
}

void lumi_native_remove_item(LUMI_SLOT_OS *slot, VALUE item, char c)
{
  if (c)
  {
    long i = rb_ary_index_of(slot->controls, item);
    if (i >= 0)
      rb_ary_insert_at(slot->controls, i, 1, Qnil);
  }
}

lumi_code
lumi_app_cursor(lumi_app *app, ID cursor)
{
  if (app->os.window == NULL || app->cursor == cursor)
    goto done;

  if (cursor == s_hand || cursor == s_link)
    [[NSCursor pointingHandCursor] set];
  else if (cursor == s_arrow)
    [[NSCursor arrowCursor] set];
  else if (cursor == s_text)
    [[NSCursor IBeamCursor] set];
  else
    goto done;

  app->cursor = cursor;

done:
  return LUMI_OK;
}

void
lumi_native_app_resized(lumi_app *app)
{
  NSRect rect = [app->os.window frame];
  rect.size.width = app->width;
  rect.size.height = app->height;
  [app->os.window setFrame: rect display: YES];
}

void
lumi_native_app_title(lumi_app *app, char *msg)
{
  COCOA_DO([app->os.window setTitle: [NSString stringWithUTF8String: msg]]);
}

static LumiWindow *
lumi_native_app_window(lumi_app *app, int dialog)
{
  LumiWindow *window;
  unsigned int mask = NSTitledWindowMask | NSClosableWindowMask | NSMiniaturizableWindowMask;
  NSRect rect = NSMakeRect(0, 0, app->width, app->height);
  NSSize size = {app->minwidth, app->minheight};

  if (app->resizable)
    mask |= NSResizableWindowMask;
  if (app->fullscreen) {
    mask = NSBorderlessWindowMask;
    rect = [[NSScreen mainScreen] frame];
  }
  window = [[LumiWindow alloc] initWithContentRect: rect
    styleMask: mask backing: NSBackingStoreBuffered defer: NO];
  if (app->minwidth > 0 || app->minheight > 0)
    [window setContentMinSize: size];
  [window prepareWithApp: app->self];
  return window;
}

void
lumi_native_view_supplant(NSView *from, NSView *to)
{
  for (id subview in [from subviews])
    [to addSubview:subview];
}

void
lumi_native_app_fullscreen(lumi_app *app, char yn)
{
  LumiWindow *old = app->os.window;
  if (yn)
  {
    int level;
    NSRect screen;
    if (CGDisplayCapture(kCGDirectMainDisplay) != kCGErrorSuccess)
      return;
    app->os.normal = [old frame];
    level = CGShieldingWindowLevel();
    screen = [[NSScreen mainScreen] frame];
    COCOA_DO({
      app->width = ROUND(screen.size.width);
      app->height = ROUND(screen.size.height);
      app->os.window = lumi_native_app_window(app, 0);
      [app->os.window setLevel: level];
      lumi_native_view_supplant([old contentView], [app->os.window contentView]);
      app->os.view = [app->os.window contentView];
      [old disconnectApp];
      [old close];
      [app->os.window setFrame: screen display: YES];
    });
  }
  else
  {
    COCOA_DO({
      app->width = ROUND(app->os.normal.size.width);
      app->height = ROUND(app->os.normal.size.height);
      app->os.window = lumi_native_app_window(app, 0);
      [app->os.window setLevel: NSNormalWindowLevel];
      CGDisplayRelease(kCGDirectMainDisplay);
      lumi_native_view_supplant([old contentView], [app->os.window contentView]);
      app->os.view = [app->os.window contentView];
      [old disconnectApp];
      [old close];
      [app->os.window setFrame: app->os.normal display: YES];
    });
  }
}

lumi_code
lumi_native_app_open(lumi_app *app, char *path, int dialog)
{
  lumi_code code = LUMI_OK;
  app->os.normal = NSMakeRect(0, 0, app->width, app->height);
  COCOA_DO({
    app->os.window = lumi_native_app_window(app, dialog);
    app->slot->view = [app->os.window contentView];
  });
quit:
  return code;
}

void
lumi_native_app_show(lumi_app *app)
{
  COCOA_DO([app->os.window orderFront: nil]);
}

void
lumi_native_loop()
{
  NSApplication *NSApp = [NSApplication sharedApplication];
  [NSApp run];
}

void
lumi_native_app_close(lumi_app *app)
{
  COCOA_DO([app->os.window close]);
}

void
lumi_browser_open(char *url)
{
  VALUE browser = rb_str_new2("open ");
  rb_str_cat2(browser, url);
  lumi_sys(RSTRING_PTR(browser), 1);
}

void
lumi_slot_init(VALUE c, LUMI_SLOT_OS *parent, int x, int y, int width, int height, int scrolls, int toplevel)
{
  lumi_canvas *canvas;
  LUMI_SLOT_OS *slot;
  Data_Get_Struct(c, lumi_canvas, canvas);

  COCOA_DO({
    slot = lumi_slot_alloc(canvas, parent, toplevel);
    slot->controls = parent->controls;
    slot->view = [[LumiView alloc] initWithFrame: NSMakeRect(x, y, width, height) andCanvas: c];
    [slot->view setAutoresizesSubviews: NO];
    if (toplevel)
      [slot->view setAutoresizingMask: (NSViewWidthSizable | NSViewHeightSizable)];
    slot->vscroll = NULL;
    if (scrolls)
    {
      slot->vscroll = [[NSScroller alloc] initWithFrame: 
        NSMakeRect(width - [NSScroller scrollerWidth], 0, [NSScroller scrollerWidth], height)];
      [slot->vscroll setEnabled: YES];
      [slot->vscroll setTarget: slot->view];
      [slot->vscroll setAction: @selector(scroll:)];
      [slot->view addSubview: slot->vscroll];
    }
    if (parent->vscroll)
      [parent->view addSubview: slot->view positioned: NSWindowBelow relativeTo: parent->vscroll];
    else
      [parent->view addSubview: slot->view];
  });
}

void
lumi_slot_destroy(lumi_canvas *canvas, lumi_canvas *pc)
{
  INIT;
  if (canvas->slot->vscroll != NULL)
    [canvas->slot->vscroll removeFromSuperview];
  [canvas->slot->view removeFromSuperview];
  RELEASE;
}

cairo_t *
lumi_cairo_create(lumi_canvas *canvas)
{
  cairo_t *cr;
  canvas->slot->surface = cairo_quartz_surface_create_for_cg_context(canvas->slot->context,
    canvas->width, canvas->height);
  cr = cairo_create(canvas->slot->surface);
  cairo_translate(cr, 0, 0 - canvas->slot->scrolly);
  return cr;
}

void lumi_cairo_destroy(lumi_canvas *canvas)
{
  cairo_surface_destroy(canvas->slot->surface);
}

void
lumi_group_clear(LUMI_GROUP_OS *group)
{
}

void
lumi_native_canvas_place(lumi_canvas *self_t, lumi_canvas *pc)
{
  NSRect rect, rect2;
  int newy = (self_t->place.iy + self_t->place.dy) - pc->slot->scrolly;
  rect.origin.x = (self_t->place.ix + self_t->place.dx) * 1.;
  rect.origin.y = ((newy) * 1.);
  rect.size.width = (self_t->place.iw * 1.);
  rect.size.height = (self_t->place.ih * 1.);
  rect2 = [self_t->slot->view frame];
  if (rect.origin.x != rect2.origin.x || rect.origin.y != rect2.origin.y ||
      rect.size.width != rect2.size.width || rect.size.height != rect2.size.height)
  {
    [self_t->slot->view setFrame: rect];
  }
}

void
lumi_native_canvas_resize(lumi_canvas *canvas)
{
  NSSize size = {canvas->width, canvas->height};
  [canvas->slot->view setFrameSize: size];
}

void
lumi_native_control_hide(LUMI_CONTROL_REF ref)
{
  COCOA_DO([ref setHidden: YES]);
}

void
lumi_native_control_show(LUMI_CONTROL_REF ref)
{
  COCOA_DO([ref setHidden: NO]);
}

static void
lumi_native_control_frame(LUMI_CONTROL_REF ref, lumi_place *p)
{
  NSRect rect;
  rect.origin.x = p->ix + p->dx; rect.origin.y = p->iy + p->dy;
  rect.size.width = p->iw; rect.size.height = p->ih;
  [ref setFrame: rect];
}

void
lumi_native_control_position(LUMI_CONTROL_REF ref, lumi_place *p1, VALUE self,
  lumi_canvas *canvas, lumi_place *p2)
{
  PLACE_COORDS();
  if (canvas->slot->vscroll)
    [canvas->slot->view addSubview: ref positioned: NSWindowBelow relativeTo: canvas->slot->vscroll];
  else
    [canvas->slot->view addSubview: ref];
  lumi_native_control_frame(ref, p2);
  rb_ary_push(canvas->slot->controls, self);
}

void
lumi_native_control_repaint(LUMI_CONTROL_REF ref, lumi_place *p1,
  lumi_canvas *canvas, lumi_place *p2)
{
  p2->iy -= canvas->slot->scrolly;
  if (CHANGED_COORDS()) {
    PLACE_COORDS();
    lumi_native_control_frame(ref, p2);
  }
  p2->iy += canvas->slot->scrolly;
}

void
lumi_native_control_state(LUMI_CONTROL_REF ref, BOOL sensitive, BOOL setting)
{
  COCOA_DO({
    [ref setEnabled: sensitive];
    if ([ref respondsToSelector: @selector(setEditable:)])
      [ref setEditable: setting];
  });
}

void
lumi_native_control_focus(LUMI_CONTROL_REF ref)
{
  COCOA_DO([[ref window] makeFirstResponder: ref]);
}

void
lumi_native_control_remove(LUMI_CONTROL_REF ref, lumi_canvas *canvas)
{
  COCOA_DO([ref removeFromSuperview]);
}

void
lumi_native_control_free(LUMI_CONTROL_REF ref)
{
}

LUMI_SURFACE_REF
lumi_native_surface_new(lumi_canvas *canvas, VALUE self, lumi_place *place)
{
  return canvas->app->os.window;
}

void
lumi_native_surface_position(LUMI_SURFACE_REF ref, lumi_place *p1, 
  VALUE self, lumi_canvas *canvas, lumi_place *p2)
{
  PLACE_COORDS();
}

void
lumi_native_surface_hide(LUMI_SURFACE_REF ref)
{
  HIViewSetVisible(ref, false);
}

void
lumi_native_surface_show(LUMI_SURFACE_REF ref)
{
  HIViewSetVisible(ref, true);
}

void
lumi_native_surface_remove(lumi_canvas *canvas, LUMI_SURFACE_REF ref)
{
}

LUMI_CONTROL_REF
lumi_native_button(VALUE self, lumi_canvas *canvas, lumi_place *place, char *msg)
{
  INIT;
  LumiButton *button = [[LumiButton alloc] initWithType: NSMomentaryPushInButton
    andObject: self];
  [button setTitle: [NSString stringWithUTF8String: msg]];
  RELEASE;
  return (NSControl *)button;
}

LUMI_CONTROL_REF
lumi_native_edit_line(VALUE self, lumi_canvas *canvas, lumi_place *place, VALUE attr, char *msg)
{
  INIT;
  NSTextField *field;
  if (RTEST(ATTR(attr, secret)))
    field = [[LumiSecureTextField alloc] initWithFrame:
      NSMakeRect(place->ix + place->dx, place->iy + place->dy,
      place->ix + place->dx + place->iw, place->iy + place->dy + place->ih)
      andObject: self];
  else
    field = [[LumiTextField alloc] initWithFrame:
      NSMakeRect(place->ix + place->dx, place->iy + place->dy,
      place->ix + place->dx + place->iw, place->iy + place->dy + place->ih)
      andObject: self];
  [field setStringValue: [NSString stringWithUTF8String: msg]];
  [field setEditable: YES];
  RELEASE;
  return (NSControl *)field;
}

VALUE
lumi_native_edit_line_get_text(LUMI_CONTROL_REF ref)
{
  VALUE text = Qnil;
  INIT;
  text = rb_str_new2([[ref stringValue] UTF8String]);
  RELEASE;
  return text;
}

void
lumi_native_edit_line_set_text(LUMI_CONTROL_REF ref, char *msg)
{
  COCOA_DO([ref setStringValue: [NSString stringWithUTF8String: msg]]);
}

LUMI_CONTROL_REF
lumi_native_edit_box(VALUE self, lumi_canvas *canvas, lumi_place *place, VALUE attr, char *msg)
{
  INIT;
  LumiTextView *tv = [[LumiTextView alloc] initWithFrame:
    NSMakeRect(place->ix + place->dx, place->iy + place->dy,
    place->ix + place->dx + place->iw, place->iy + place->dy + place->ih)
    andObject: self];
  lumi_native_edit_box_set_text((NSControl *)tv, msg);
  RELEASE;
  return (NSControl *)tv;
}

VALUE
lumi_native_edit_box_get_text(LUMI_CONTROL_REF ref)
{
  VALUE text = Qnil;
  INIT;
  text = rb_str_new2([[[(LumiTextView *)ref textStorage] string] UTF8String]);
  RELEASE;
  return text;
}

void
lumi_native_edit_box_set_text(LUMI_CONTROL_REF ref, char *msg)
{
  COCOA_DO([[[(LumiTextView *)ref textStorage] mutableString] setString: [NSString stringWithUTF8String: msg]]);
}

LUMI_CONTROL_REF
lumi_native_list_box(VALUE self, lumi_canvas *canvas, lumi_place *place, VALUE attr, char *msg)
{
  INIT;
  LumiPopUpButton *pop = [[LumiPopUpButton alloc] initWithFrame:
    NSMakeRect(place->ix + place->dx, place->iy + place->dy,
    place->ix + place->dx + place->iw, place->iy + place->dy + place->ih)
    andObject: self];
  RELEASE;
  return (NSControl *)pop;
}

void
lumi_native_list_box_update(LUMI_CONTROL_REF ref, VALUE ary)
{
  long i;
  LumiPopUpButton *pop = (LumiPopUpButton *)ref;
  COCOA_DO({
    [pop removeAllItems];
    for (i = 0; i < RARRAY_LEN(ary); i++)
    {
      VALUE msg_s = lumi_native_to_s(rb_ary_entry(ary, i));
      char *msg = RSTRING_PTR(msg_s);
      [[pop menu] insertItemWithTitle: [NSString stringWithUTF8String: msg] action: nil
        keyEquivalent: @"" atIndex: i];
    }
  });
}

VALUE
lumi_native_list_box_get_active(LUMI_CONTROL_REF ref, VALUE items)
{
  NSInteger sel = [(LumiPopUpButton *)ref indexOfSelectedItem];
  if (sel >= 0)
    return rb_ary_entry(items, sel);
  return Qnil;
}

void
lumi_native_list_box_set_active(LUMI_CONTROL_REF ref, VALUE ary, VALUE item)
{
  long idx = rb_ary_index_of(ary, item);
  if (idx < 0) return;
  COCOA_DO([(LumiPopUpButton *)ref selectItemAtIndex: idx]);
}

LUMI_CONTROL_REF
lumi_native_progress(VALUE self, lumi_canvas *canvas, lumi_place *place, VALUE attr, char *msg)
{
  INIT;
  NSProgressIndicator *pop = [[NSProgressIndicator alloc] init];
  [pop setIndeterminate: FALSE];
  [pop setDoubleValue: 0.];
  [pop setBezeled: YES];
  RELEASE;
  return (NSControl *)pop;
}

double
lumi_native_progress_get_fraction(LUMI_CONTROL_REF ref)
{
  return [(NSProgressIndicator *)ref doubleValue] * 0.01;
}

void
lumi_native_progress_set_fraction(LUMI_CONTROL_REF ref, double perc)
{
  COCOA_DO([(NSProgressIndicator *)ref setDoubleValue: perc * 100.]);
}

LUMI_CONTROL_REF
lumi_native_slider(VALUE self, lumi_canvas *canvas, lumi_place *place, VALUE attr, char *msg)
{
  INIT;
  LumiSlider *pop = [[LumiSlider alloc] initWithObject: self];
  [pop setMinValue: 0.];
  [pop setMaxValue: 100.];
  RELEASE;
  return (NSControl *)pop;
}

double
lumi_native_slider_get_fraction(LUMI_CONTROL_REF ref)
{
  return [(LumiSlider *)ref doubleValue] * 0.01;
}

void
lumi_native_slider_set_fraction(LUMI_CONTROL_REF ref, double perc)
{
  COCOA_DO([(LumiSlider *)ref setDoubleValue: perc * 100.]);
}

LUMI_CONTROL_REF
lumi_native_check(VALUE self, lumi_canvas *canvas, lumi_place *place, VALUE attr, char *msg)
{
  INIT;
  LumiButton *button = [[LumiButton alloc] initWithType: NSSwitchButton andObject: self];
  RELEASE;
  return (NSControl *)button;
}

VALUE
lumi_native_check_get(LUMI_CONTROL_REF ref)
{
  return [(LumiButton *)ref state] == NSOnState ? Qtrue : Qfalse;
}

void
lumi_native_check_set(LUMI_CONTROL_REF ref, int on)
{
  COCOA_DO([(LumiButton *)ref setState: on ? NSOnState : NSOffState]);
}

LUMI_CONTROL_REF
lumi_native_radio(VALUE self, lumi_canvas *canvas, lumi_place *place, VALUE attr, VALUE group)
{
  INIT;
  LumiButton *button = [[LumiButton alloc] initWithType: NSRadioButton
    andObject: self];
  RELEASE;
  return (NSControl *)button;
}

void
lumi_native_timer_remove(lumi_canvas *canvas, LUMI_TIMER_REF ref)
{
  COCOA_DO([ref invalidate]);
}

LUMI_TIMER_REF
lumi_native_timer_start(VALUE self, lumi_canvas *canvas, unsigned int interval)
{
  INIT;
  LumiTimer *timer = [[LumiTimer alloc] initWithTimeInterval: interval * 0.001
    andObject: self repeats:(rb_obj_is_kind_of(self, cTimer) ? NO : YES)];
  RELEASE;
  return timer;
}

VALUE
lumi_native_clipboard_get(lumi_app *app)
{
  VALUE txt = Qnil;
  INIT;
  NSString *paste = [[NSPasteboard generalPasteboard] stringForType: NSStringPboardType];
  if (paste) txt = rb_str_new2([paste UTF8String]);
  RELEASE;
  return txt;
}

void
lumi_native_clipboard_set(lumi_app *app, VALUE string)
{
  COCOA_DO({
    [[NSPasteboard generalPasteboard] declareTypes: [NSArray arrayWithObject: NSStringPboardType] owner: nil];
    [[NSPasteboard generalPasteboard] setString: [NSString stringWithUTF8String: RSTRING_PTR(string)]
      forType: NSStringPboardType];
  });
}

VALUE
lumi_native_to_s(VALUE text)
{
  text = rb_funcall(text, s_to_s, 0);
  return text;
}

VALUE
lumi_native_window_color(lumi_app *app)
{
  // float r, g, b, a;
  // INIT;
  // [[[app->os.window backgroundColor] colorUsingColorSpace: [NSColorSpace genericRGBColorSpace]] 
  //   getRed: &r green: &g blue: &b alpha: &a];
  // RELEASE;
  // return lumi_color_new((int)(r * 255), (int)(g * 255), (int)(b * 255), (int)(a * 255));
  return lumi_color_new(255, 255, 255, 255);
}

VALUE
lumi_native_dialog_color(lumi_app *app)
{
  return lumi_native_window_color(app);
}

VALUE
lumi_dialog_alert(VALUE self, VALUE msg)
{
  VALUE answer = Qnil;
  COCOA_DO({
    msg = lumi_native_to_s(msg);
    NSAlert *alert = [NSAlert alertWithMessageText: @"Lumi says:"
      defaultButton: @"OK" alternateButton: nil otherButton: nil 
      informativeTextWithFormat: [NSString stringWithUTF8String: RSTRING_PTR(msg)]];
    [alert runModal];
  });
  return Qnil;
}

VALUE
lumi_dialog_ask(int argc, VALUE *argv, VALUE self)
{
  rb_arg_list args;
  VALUE answer = Qnil;
  rb_parse_args(argc, argv, "s|h", &args);
  COCOA_DO({
    NSApplication *NSApp = [NSApplication sharedApplication];
    LumiAlert *alert = [[LumiAlert alloc] init];
    NSButton *okButton = [[[NSButton alloc] initWithFrame:
      NSMakeRect(244, 10, 88, 30)] autorelease];
    NSButton *cancelButton = [[[NSButton alloc] initWithFrame:
      NSMakeRect(156, 10, 88, 30)] autorelease];
    NSRect windowFrame = [alert frame];
    NSString *str = [[[NSString alloc] initWithUTF8String:
      RSTRING_PTR(args.a[0])] autorelease];
    NSTextField *input;
    if (RTEST(ATTR(args.a[1], secret)))
      input = [[NSSecureTextField alloc] initWithFrame:NSMakeRect(20, 72, 300, 24)];
    else
      input = [[NSTextField alloc] initWithFrame:NSMakeRect(20, 72, 300, 24)];

    NSTextStorage *textStorage = [[[NSTextStorage alloc] initWithString:str] autorelease];
    NSTextContainer *textContainer = [[[NSTextContainer alloc] initWithContainerSize:NSMakeSize(260, FLT_MAX)] autorelease];
    NSLayoutManager *layoutManager = [[[NSLayoutManager alloc] init] autorelease];
    [layoutManager addTextContainer:textContainer];
    [textStorage addLayoutManager:layoutManager];
    [textStorage addAttribute:NSFontAttributeName value:[input font] range:NSMakeRange(0, [textStorage length])];
    [textContainer setLineFragmentPadding:0.0];
    (void) [layoutManager glyphRangeForTextContainer:textContainer];
    CGFloat strheight = [layoutManager usedRectForTextContainer:textContainer].size.height;
    NSTextView *text = [[[NSTextView alloc] initWithFrame: NSMakeRect(20, 110, 260, strheight)] autorelease];

    [alert setTitle: @"Lumi asks:"];
    windowFrame.size.height = 140 + strheight;
    [alert setFrame: windowFrame display: TRUE];
    [text insertText: str];
    [text setBackgroundColor: [NSColor windowBackgroundColor]];
    [text setEditable: NO];
    [text setSelectable: NO];
    [text setFont:[input font]];
    [[alert contentView] addSubview: text];
    [input setStringValue:@""];
    [[alert contentView] addSubview: input];
    [okButton setTitle: @"OK"];
    [okButton setBezelStyle: 1];
    [okButton setTarget: alert];
    [okButton setAction: @selector(okClick:)];
    [[alert contentView] addSubview: okButton];
    [cancelButton setTitle: @"Cancel"];
    [cancelButton setBezelStyle: 1];
    [cancelButton setTarget: alert];
    [cancelButton setAction: @selector(cancelClick:)];
    [[alert contentView] addSubview: cancelButton];
    [alert setDefaultButtonCell: okButton];
    [NSApp runModalForWindow: alert];
    if ([alert accepted])
      answer = rb_str_new2([[input stringValue] UTF8String]);
    [alert close];
  });
  return answer;
}

VALUE
lumi_dialog_confirm(VALUE self, VALUE quiz)
{
  char *msg;
  VALUE answer = Qnil;
  COCOA_DO({
    quiz = lumi_native_to_s(quiz);
    msg = RSTRING_PTR(quiz);
    NSAlert *alert = [NSAlert alertWithMessageText: @"Lumi asks:"
      defaultButton: @"OK" alternateButton: @"Cancel" otherButton:nil 
      informativeTextWithFormat: [NSString stringWithUTF8String: msg]];
    answer = ([alert runModal] == NSOKButton ? Qtrue : Qfalse);
  });
  return answer;
}

VALUE
lumi_dialog_color(VALUE self, VALUE title)
{
  Point where;
  RGBColor colwh = { 0xFFFF, 0xFFFF, 0xFFFF };
  RGBColor _color;
  VALUE color = Qnil;
  GLOBAL_APP(app);

  where.h = where.v = 0;
  title = lumi_native_to_s(title);
  if (GetColor(where, RSTRING_PTR(title), &colwh, &_color))
  {
    color = lumi_color_new(_color.red/256, _color.green/256, _color.blue/256, LUMI_COLOR_OPAQUE);
  }
  return color;
}

static VALUE
lumi_dialog_chooser(VALUE self, NSString *title, BOOL directories, VALUE attr)
{
  VALUE path = Qnil;
  COCOA_DO({
    NSOpenPanel* openDlg = [NSOpenPanel openPanel];
    [openDlg setCanChooseFiles: !directories];
    [openDlg setCanChooseDirectories: directories];
    [openDlg setAllowsMultipleSelection: NO];
    if ( [openDlg runModalForDirectory: nil file: nil] == NSOKButton )
    {
      NSArray* files = [openDlg filenames];
      const char *filename = [[files objectAtIndex: 0] UTF8String];
      path = rb_str_new2(filename);
    }
  });
  return path;
}

VALUE
lumi_dialog_open(int argc, VALUE *argv, VALUE self)
{
  rb_arg_list args;
  rb_parse_args(argc, argv, "|h", &args);
  return lumi_dialog_chooser(self, @"Open file...", NO, args.a[0]);
}

VALUE
lumi_dialog_save(int argc, VALUE *argv, VALUE self)
{
  VALUE path = Qnil;
  COCOA_DO({
    NSSavePanel* saveDlg = [NSSavePanel savePanel];
    if ( [saveDlg runModalForDirectory:nil file:nil] == NSOKButton )
    {
      const char *filename = [[saveDlg filename] UTF8String];
      path = rb_str_new2(filename);
    }
  });
  return path;
}

VALUE
lumi_dialog_open_folder(int argc, VALUE *argv, VALUE self)
{
  rb_arg_list args;
  rb_parse_args(argc, argv, "|h", &args);
  return lumi_dialog_chooser(self, @"Open folder...", YES, args.a[0]);
}

VALUE
lumi_dialog_save_folder(int argc, VALUE *argv, VALUE self)
{
  rb_arg_list args;
  rb_parse_args(argc, argv, "|h", &args);
  return lumi_dialog_chooser(self, @"Save folder...", YES, args.a[0]);
}
