# Makefile.am -- top level Makefile for the m17n library.
# Copyright (C) 2003, 2004, 2005, 2006, 2007, 2008, 2009, 2010
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

## Process this file with Automake to create Makefile.in

SUBDIRS = intl po src example

bin_SCRIPTS = m17n-config

EXTRA_DIST = m4/ChangeLog config.rpath mkinstalldirs \
	m17n-core.pc.in m17n-shell.pc.in m17n-flt.pc.in m17n-gui.pc.in bootstrap.sh

pkgconfigdir = ${libdir}/pkgconfig

BASICDATA = m17n-core.pc m17n-shell.pc m17n-flt.pc
if WITH_GUI
PKGDATA = $(BASICDATA) m17n-gui.pc
else
PKGDATA = $(BASICDATA)
endif
pkgconfig_DATA = $(PKGDATA)

ACLOCAL_AMFLAGS = -I m4
