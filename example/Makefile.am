# Makefile.am -- example level Makefile for the m17n library.
# Copyright (C) 2003, 2004, 2005, 2006, 2007, 2010
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

## Note: Source files have preifx "m" but executables have prefix
## "m17n-" to avoid confliction of program names.

BASICPROGS = m17n-conv
if WITH_GUI
noinst_PROGRAMS = $(BASICPROGS) m17n-view m17n-date m17n-dump m17n-edit
else
noinst_PROGRAMS = $(BASICPROGS)
endif

common_ldflags = ${top_builddir}/src/libm17n-core-@M17N_BINARY_VERSION@.la ${top_builddir}/src/libm17n-@M17N_BINARY_VERSION@.la
common_ldflags_gui = ${common_ldflags} ${top_builddir}/src/libm17n-flt-@M17N_BINARY_VERSION@.la ${top_builddir}/src/libm17n-gui-@M17N_BINARY_VERSION@.la
AM_CPPFLAGS=@CONFIG_FLAGS@

m17n_date_SOURCES = mdate.c
m17n_date_LDADD = ${common_ldflags}

m17n_conv_SOURCES = mconv.c
m17n_conv_LDADD = ${common_ldflags}

X_LD_FLAGS = ${X_PRE_LIBS} ${X_LIBS} @XAW_LD_FLAGS@ @X11_LD_FLAGS@ ${X_EXTRA_LIBS}

m17n_edit_SOURCES = medit.c
m17n_edit_LDADD = ${X_LD_FLAGS} ${common_ldflags_gui} -ldl

m17n_view_SOURCES = mview.c
m17n_view_LDADD = ${X_LD_FLAGS} ${common_ldflags_gui}

m17n_dump_SOURCES = mdump.c
m17n_dump_LDADD = @GD_LD_FLAGS@ ${common_ldflags_gui}

# Input method data files.

pkgdatadir=$(datadir)/m17n

EXTRA_DIST = \
	.gdbinit .gdb.util \
	HELLO.utf8 HELLO.xml HELLO-ja.utf8 HELLO-ja.xml HELLO.html M17NEdit.ja

# External modules used by the above input methods.

moduledir = ${libdir}/@M17N_MODULE_DIR@

BASICBUILDS = libmimx-anthy.la
if WITH_GUI
BUILD_LIBS = $(BASICBUILDS) libmimx-ispell.la 
else
BUILD_LIBS = $(BASICBUILDS)
endif
module_LTLIBRARIES = $(BUILD_LIBS)

libmimx_ispell_la_SOURCES = mimx-ispell.c
libmimx_ispell_la_LIBADD = ${common_ldflags_gui}
libmimx_ispell_la_LDFLAGS = -avoid-version -module
libmimx_anthy_la_SOURCES = mimx-anthy.c
libmimx_anthy_la_LIBADD = ${common_ldflags} @ANTHY_LD_FLAGS@ 
libmimx_anthy_la_LDFLAGS = -avoid-version -module
