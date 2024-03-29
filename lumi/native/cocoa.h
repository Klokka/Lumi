//
// lumi/native/cocoa.h
// Custom Cocoa interfaces for Lumi
//
#import <Cocoa/Cocoa.h>

@interface LumiEvents : NSObject
{
  int count;
}
@end

@interface LumiWindow : NSWindow
{
  VALUE app;
}
@end

@interface LumiView : NSView
{
  VALUE canvas;
}
@end

@interface LumiButton : NSButton
{
  VALUE object;
}
@end

@interface LumiTextField : NSTextField
{
  VALUE object;
}
@end

@interface LumiSecureTextField : NSSecureTextField
{
  VALUE object;
}
@end

@interface LumiTextView : NSScrollView
{
  VALUE object;
  NSTextView *textView;
}
@end

@interface LumiPopUpButton : NSPopUpButton
{
  VALUE object;
}
@end

@interface LumiSlider : NSSlider
{
  VALUE object;
}
@end

@interface LumiAlert : NSWindow
{
  NSWindow *win;
  BOOL answer;
}
@end

@interface LumiTimer : NSObject
{
  VALUE object;
  NSTimer *timer;
}
@end

void add_to_menubar(NSMenu *main, NSMenu *menu);
void create_apple_menu(NSMenu *main);
void create_edit_menu(NSMenu *main);
void create_window_menu(NSMenu *main);
void create_help_menu(NSMenu *main);
void lumi_native_view_supplant(NSView *from, NSView *to);

#define VK_ESCAPE 53
#define VK_DELETE 117
#define VK_INSERT 114
#define VK_TAB   48
#define VK_BS    51
#define VK_PRIOR 116
#define VK_NEXT  121
#define VK_HOME  115
#define VK_END   119
#define VK_LEFT  123
#define VK_UP    126
#define VK_RIGHT 124
#define VK_DOWN  125
#define VK_F1    122
#define VK_F2    120
#define VK_F3     99
#define VK_F4    118
#define VK_F5     96
#define VK_F6     97
#define VK_F7     98
#define VK_F8    100
#define VK_F9    101
#define VK_F10   109
#define VK_F11   103
#define VK_F12   111

#define KEY_SYM(name, sym) \
  if (key == VK_##name) \
    v = ID2SYM(rb_intern("" # sym)); \
  else
