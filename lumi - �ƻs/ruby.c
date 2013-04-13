// 
// lumi/ruby.c
// 
#include "lumi/ruby.h"

VALUE mLumi;
VALUE cWindow;

// 
// Put everything into ruby.
void
lm_ruby_init()
{
  mLumi = rb_define_module("Lumi");
  rb_mod_remove_const(rb_cObject, rb_str_new2("Lumi"));
  rb_const_set(mLumi, rb_intern("VERSION"), rb_str_new2("0.1"));

  cWindow = rb_define_class_under(mLumi, "Window", rb_cObject);

}