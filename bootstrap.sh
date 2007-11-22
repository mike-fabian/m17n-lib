#!/bin/sh
# bootstrap.sh -- shell script to build the m17n library from CVS.
# Copyright (C) 2003, 2004, 2005, 2006
#   National Institute of Advanced Industrial Science and Technology (AIST)
#   Registration Number H15PRO112
# See the end for copying conditions.

echo "Cleaning up old files..."
rm -rf INSTALL aclocal.m4 autom4te.cache config.guess. config.sub install-sh ltmain.sh missing libtool
echo "Running aclocal..."
aclocal -I m4
echo "Running autoheader..."
autoheader
echo "Running libtoolize..."
libtoolize --automake
echo "Running automake..."
automake -a -c
echo "Running autoconf..."
autoconf
echo "The remaining steps to install this library are:"
echo "  % ./configure"
echo "  % make"
echo "  % make install"

# Copyright (C) 2003, 2004
#   National Institute of Advanced Industrial Science and Technology (AIST)
#   Registration Number H15PRO112

# This file is part of the m17n library.

# The m17n library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public License
# as published by the Free Software Foundation; either version 2.1 of
# the License, or (at your option) any later version.

# The m17n library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Lesser General Public License for more details.

# You should have received a copy of the GNU Lesser General Public
# License along with the m17n library; if not, write to the Free
# Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
# Boston, MA 02110-1301, USA.
