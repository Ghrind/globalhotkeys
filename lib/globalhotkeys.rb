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

require 'globalhotkeys.so'

#:main: GlobalHotKeys
module GlobalHotKeys
VERSION=[0, 3, 2]
end

GlobalHotKeys.init
