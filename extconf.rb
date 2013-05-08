#!/usr/bin/env ruby
# encoding: UTF-8

=begin
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
=end

require 'mkmf'
def add_distclean(file)
	$distcleanfiles ||= []
	$distcleanfiles << file
end

find_header('ruby.h') or exit 1
gtk_cflags=`pkg-config --cflags gtk+-2.0`.chomp!
$INCFLAGS+=gtk_cflags
find_library('X11', 'XKeysymToKeycode') or exit 1
find_library('gobject-2.0', 'g_type_init') or exit 1
find_library('gdk-x11-2.0', '') or exit 1
find_header('string.h') or exit 1
find_header('X11/X.h') or exit 1
find_header('X11/Xlib.h') or exit 1
find_header('gdk/gdkwindow.h') or exit 1
find_header('gdk/gdkevents.h') or exit 1
find_header('gdk/gdkx.h') or exit 1

gdkkeysyms_file=nil
gtk_cflags.scan(/-I.+?\ /).map{|pp| File.join(pp[2..-2], "gdk/gdkkeysyms.h")}.each{|file|
	if File.exist?(file)
		gdkkeysyms_file=file
		break
	end
}
f=File.open(gdkkeysyms_file, 'r')
keysyms_file=File.join(File.dirname(__FILE__), "keysyms.h")
File.open(keysyms_file, 'w'){|file|
	f.each_line{|line|
		if line =~ /^#define\s+(GDK_\w+)\s+\d+/
			gdk_name=$1
			name= gdk_name =~/GDK_KEY_/ ? gdk_name[4..-1] : "KEY_"+gdk_name[4..-1]
			file.puts "rb_define_const(mKeyVal, \"#{name}\", INT2FIX(#{gdk_name}));"
		end
	}
}
f.close
add_distclean(keysyms_file)

create_makefile("globalhotkeys")
