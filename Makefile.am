# Makefile.am -- top level Makefile for the m17n library.
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
# Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
# 02111-1307, USA.

## Process this file with Automake to create Makefile.in

SUBDIRS = src example

bin_SCRIPTS = m17n-config

EXTRA_DIST = \
	m17n-core.pc.in m17n-shell.pc.in m17n-gui.pc.in bootstrap.sh

pkgconfigdir = ${libdir}/pkgconfig

if WITH_GUI
PKGDATA = m17n-core.pc m17n-shell.pc m17n-gui.pc
else
PKGDATA = m17n-core.pc m17n-shell.pc
endif
pkgconfig_DATA = $(PKGDATA)
