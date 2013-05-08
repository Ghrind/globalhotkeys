/*
Copyright 2010-2011 Vincent Carmona
vinc4mai@gmail.com

This file is part of rghk.

    rghk is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    rghk is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with rghk; if not, write to the Free Software
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include <string.h>
#include <ruby.h>
#include <X11/Xlib.h>
#include <X11/X.h>
#include <gdk/gdkkeysyms.h>
#include <gdk/gdkwindow.h>
#include <gdk/gdkx.h>
#include <gdk/gdkevents.h>
//xev to see keycode
//take a look at http://www.gnu.org/prep/standards/standards.html

//Modifier Key
enum Modifier
{
	RGHK_SHIFT_MASK=1<<0,	
	RGHK_LOCK_MASK=1<<1,	
	RGHK_CONTROL_MASK=1<<2,	
	RGHK_MOD1_MASK=1<<3,	
	RGHK_MOD2_MASK=1<<4,	
	RGHK_MOD3_MASK=1<<5,	
	RGHK_MOD4_MASK=1<<6,	
	RGHK_MOD5_MASK=1<<7,
	RGHK_NUM_LOCK_MASK=RGHK_MOD2_MASK//Is it true for all keyboards?
	//RGHK_SCROLL_LOCK=??
};

GdkWindow * root_window;
char xlib_error[256];
VALUE mGlobalHotKeys;
VALUE eInitFailed;
VALUE eInvalidKeyVal;
VALUE eBinded;
VALUE eXlibError;
VALUE mKeyVal;
VALUE mModifier;
VALUE cKeyBinder;

/*
 * Converts a key name to a key value.
*/ 
VALUE
keyval_from_name(VALUE self, VALUE name)
{
	uint keyval;

	keyval=gdk_keyval_from_name(StringValuePtr(name));
	if (keyval==FIX2INT(rb_const_get(mKeyVal, rb_intern("KEY_VoidSymbol"))))
		return Qnil;

	return INT2FIX(keyval);
}

int
xlib_error_handler(Display *display, XErrorEvent *error)
{
//FIXME : cannot ungrab possible grabbed key in incriminate call of kb_bind_common function : this function should not generate internal X protocol requests
	char buff[256];

	if (error->error_code==BadAccess)
	{
		strcpy(xlib_error, "Xlib BadAccess error (key must be grabbed by another programm)");
	}
	else
	{
		if (strcmp(xlib_error, "")==0)
		{
			XGetErrorText(GDK_WINDOW_XDISPLAY(root_window), error->error_code,
				buff, sizeof(buff));
			strcpy(xlib_error, buff);
		}
	}
	return 0;
}

void
kb_bind_common(VALUE self, VALUE key, VALUE modifier, VALUE block)
{
	Display *xdisplay;
	uint keycode;
	/*	uint ignored[]={0, RGHK_LOCK_MASK, RGHK_NUM_LOCK_MASK, RGHK_SCROLL_LOCK,
		RGHK_LOCK_MASK|RGHK_NUM_LOCK_MASK, RGHK_LOCK_MASK|RGHK_SCROLL_LOCK, RGHK_NUM_LOCK_MASK|RGHK_SCROLL_LOCK,
		RGHK_LOCK_MASK|RGHK_NUM_LOCK_MASK|RGHK_SCROLL_LOCK};*/
	uint ignored[]={0, RGHK_LOCK_MASK, RGHK_NUM_LOCK_MASK, RGHK_LOCK_MASK|RGHK_NUM_LOCK_MASK};
	uint mod;
	XErrorHandler handler;

	if (rb_funcall(rb_cv_get(cKeyBinder, "@@stock"), rb_intern("include?"), 1, self)==Qtrue)
		rb_raise(eBinded, "KeyBinder allready binded.");

	xdisplay=GDK_WINDOW_XDISPLAY(root_window);

	keycode=XKeysymToKeycode(xdisplay, NUM2UINT(key));
	if (keycode==0)
		rb_raise(eInvalidKeyVal, "Invalid key value.");

	if NIL_P(modifier)
		mod=0;
	else
		mod=NUM2UINT(modifier);

	strcpy(xlib_error, "");
	handler=XSetErrorHandler(xlib_error_handler);
	int i;
	for (i=0; i<4 ; i++)
	{
		XGrabKey(xdisplay, keycode, mod|ignored[i],  GDK_WINDOW_XWINDOW(root_window), False,
			GrabModeAsync, GrabModeAsync);
	}
	XSync(xdisplay, FALSE);
	XSetErrorHandler(handler);
	if (strcmp(xlib_error, "")!=0)
		rb_raise(eXlibError, xlib_error);
//XGrabKey(xdisplay, keycode, AnyModifier, GDK_WINDOW_XWINDOW(root_window), False, GrabModeAsync, GrabModeAsync);

	rb_iv_set(self, "@key", key);
	rb_iv_set(self, "@mod", modifier);
	rb_iv_set(self, "block", block);
	rb_ary_push(rb_cv_get(cKeyBinder, "@@stock"), self);
}

/*
 * call-seq: bind(key, modifier) {block}
 * 
 * key: a key value.
 * 
 * modifier: a GlobalHotKeys::Modifier.
 * 
 * Set a global hotkey.
 * block will be called when the key and the modifier are hitted.
*/
VALUE
kb_bind(VALUE self, VALUE key, VALUE modifier)
{
	kb_bind_common(self, key, modifier, rb_block_proc());
	return Qtrue;
}

/*
 * call-seq: 
 *  new
 *  new(key, modifier){block}
 * 
 * new: create a new (unbinded) GlobalHotKeys::KeyBinder object.
 * 
 * new(key, modifier){block}: create a new GlobalHotKeys::KeyBinder object and bind it.
*/
VALUE
kb_init(int argc, VALUE* argv, VALUE self)
{
	VALUE key, modifier, block;
	rb_scan_args(argc, argv, "02", &key, &modifier);
	if (!NIL_P(key))
		kb_bind_common(self, key, modifier, rb_block_proc());
	return Qnil;
}

/*
 * Unset a global hotkey.
*/
VALUE
kb_unbind(VALUE self)
{
	VALUE del, ret;
	Display *xdisplay;
	uint keycode, mod;
	uint ignored[]={0, RGHK_LOCK_MASK, RGHK_NUM_LOCK_MASK, RGHK_LOCK_MASK|RGHK_NUM_LOCK_MASK};

	del=rb_funcall(rb_cv_get(cKeyBinder, "@@stock"), rb_intern("delete"), 1, self);
	if NIL_P(del)
		return Qfalse;

	xdisplay=GDK_WINDOW_XDISPLAY(root_window);
	keycode=XKeysymToKeycode(xdisplay, FIX2UINT(rb_iv_get(self, "@key")));
	mod=FIX2UINT(rb_iv_get(self, "@mod"));

	int i;
	for (i=0; i<4 ; i++)
		XUngrabKey(xdisplay, keycode, mod|ignored[i], GDK_WINDOW_XWINDOW (root_window));
	return Qtrue;
}

/*
 * Unset all global hotkeys.
*/
VALUE
kb_unbind_all(VALUE self)
{
	rb_iterate(rb_each, rb_cv_get(cKeyBinder, "@@stock"), kb_unbind, Qnil);
	return Qnil;
}

static VALUE
process(VALUE kb_obj, VALUE xkey)
{
//rb_funcall(rb_stdout, rb_intern("puts"), 1, RARRAY(xkey)->ptr[1]);
	uint keycode=XKeysymToKeycode(GDK_WINDOW_XDISPLAY(root_window), FIX2UINT(rb_iv_get(kb_obj, "@key")));

	if (keycode==FIX2UINT(RARRAY(xkey)->ptr[0]))
	{
		uint ignored=RGHK_LOCK_MASK|RGHK_NUM_LOCK_MASK;
		uint mod=FIX2UINT(RARRAY(xkey)->ptr[1])&~ignored;
		uint keymod;

		if NIL_P(rb_iv_get(kb_obj, "@mod"))
			keymod=0;
		else
			keymod=FIX2UINT(rb_iv_get(kb_obj, "@mod"));

		if (keymod==mod)
			rb_funcall(rb_iv_get(kb_obj, "block"), rb_intern("call"), 1, kb_obj);
	}
	return Qnil;
}

static GdkFilterReturn
filter_func(GdkXEvent *gdk_xevent, GdkEvent *event, gpointer data)
{
	GdkFilterReturn ret;
	XEvent *xevent=(XEvent*)gdk_xevent;

	if (xevent->type==KeyPress)
	{
		VALUE xev=rb_ary_new3(2, INT2FIX(xevent->xkey.keycode), INT2FIX(xevent->xkey.state));

		rb_iterate(rb_each, rb_cv_get(cKeyBinder, "@@stock"), process, xev);
	}

	return GDK_FILTER_CONTINUE;
}

/*:nodoc: It is called by rghk when it is required*/
//:nodoc:
VALUE
mInit(VALUE self)
{
	g_type_init();
	root_window=gdk_get_default_root_window();//gdk_get_default_root_window(void) in gdkwindow and  gdk_screen_get_root_window(GdkScreen *screen) in gdkscreen!!
	if (root_window==NULL)
		rb_raise(eInitFailed, "Cannot get root window.");
	gdk_window_add_filter(root_window, filter_func, NULL);
	return Qtrue;
}

void
Init_globalhotkeys()
{
/*
 * rghk provides a simple way to set global hotkeys.
 * 
 * Example:
 *   kb=GlobalHotKeys::KeyBinder.new
 *   kb.bind(GlobalHotKeys::KeyVal::KEY_a,  GlobalHotKeys::Modifier::CONTROL_MASK){puts 'Binded'}
 *   kb.unbind
 * 
 * It probably a good idea to require 'gtk2' before 'globalhotkeys'.
 * The librairy rghk  was not tested without ruby/gtk2.
*/
	mGlobalHotKeys=rb_define_module("GlobalHotKeys");
	rb_define_module_function(mGlobalHotKeys, "init", mInit, 0);

	eInitFailed=rb_define_class_under(mGlobalHotKeys, "InitFailed", rb_eException);
	eInvalidKeyVal=rb_define_class_under(mGlobalHotKeys, "InvalidKeyVal", rb_eException);
	eBinded=rb_define_class_under(mGlobalHotKeys, "Binded", rb_eException);
	eXlibError=rb_define_class_under(mGlobalHotKeys, "XlibError", rb_eException);

	mKeyVal=rb_define_module_under(mGlobalHotKeys, "KeyVal");
	rb_define_module_function(mKeyVal, "from_name", keyval_from_name, 1);
#include "keysyms.h"

	mModifier=rb_define_module_under(mGlobalHotKeys, "Modifier");
	rb_define_const(mModifier, "SHIFT_MASK", INT2FIX(RGHK_SHIFT_MASK));
	rb_define_const(mModifier, "CONTROL_MASK", INT2FIX(RGHK_CONTROL_MASK));
	rb_define_const(mModifier, "MOD1_MASK", INT2FIX(RGHK_MOD1_MASK));//Alt
	//rb_define_const(mModifier, "MOD2_MASK", INT2FIX(RGHK_MOD2_MASK));//NUM_LOCK
	rb_define_const(mModifier, "MOD3_MASK", INT2FIX(RGHK_MOD3_MASK));
	rb_define_const(mModifier, "MOD4_MASK", INT2FIX(RGHK_MOD4_MASK));
	rb_define_const(mModifier, "MOD5_MASK", INT2FIX(RGHK_MOD5_MASK));//AltGr?

	cKeyBinder=rb_define_class_under(mGlobalHotKeys, "KeyBinder", rb_cObject);
	rb_define_class_variable(cKeyBinder, "@@stock", rb_ary_new());
	rb_define_method(cKeyBinder, "initialize", kb_init, -1);
	rb_define_method(cKeyBinder, "bind", kb_bind, 2);
	rb_define_method(cKeyBinder, "unbind", kb_unbind, 0);
	rb_define_module_function(cKeyBinder, "unbind_all", kb_unbind_all, 0);
}
