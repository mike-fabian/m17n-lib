# Makefile.am -- example level Makefile for the m17n library.
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

## Note: Source files have preifx "m" but executables have prefix
## "m17n-" to avoid confliction of program names.

bin_PROGRAMS = m17n-conv m17n-view m17n-date m17n-dump m17n-edit

common_ldflags = ${top_srcdir}/src/libm17n-core.la ${top_srcdir}/src/libm17n.la
common_ldflags_gui = ${common_ldflags} ${top_srcdir}/src/libm17n-gui.la

m17n_date_SOURCES = mdate.c
m17n_date_LDADD = ${common_ldflags}

m17n_conv_SOURCES = mconv.c
m17n_conv_LDADD = ${common_ldflags}

X_LD_FLAGS = ${X_PRE_LIBS} ${X_LIBS} -lXaw -lXmu -lXt -lX11 ${X_EXTRA_LIBS}

m17n_edit_SOURCES = medit.c linebreak.c
m17n_edit_LDADD = ${X_LD_FLAGS} @WORDCUT_LD_FLAGS@ ${common_ldflags_gui}

m17n_view_SOURCES = mview.c
m17n_view_LDADD = ${X_LD_FLAGS} ${common_ldflags_gui}

m17n_dump_SOURCES = mdump.c linebreak.c
m17n_dump_LDADD = @GD_LD_FLAGS@ @WORDCUT_LD_FLAGS@ ${common_ldflags_gui}

# Input method data files.

pkgdatadir=$(datadir)/m17n

EXTRA_DIST = \
	.gdbinit .gdb.util \
	HELLO.utf8 HELLO.xml HELLO-ja.utf8 HELLO-ja.xml MEdit.ja

# External modules used by the above input methods.

VINFO = -version-info @API_VERSION@

lib_LTLIBRARIES = libmimx-ispell.la libmimx-anthy.la

libmimx_ispell_la_SOURCES = mimx-ispell.c
libmimx_ispell_la_LIBADD = ${common_ldflags_gui}
libmimx_ispell_la_LDFLAGS = -module ${VINFO}
libmimx_anthy_la_SOURCES = mimx-anthy.c
libmimx_anthy_la_LIBADD = ${common_ldflags} @ANTHY_LD_FLAGS@ 
libmimx_anthy_la_LDFLAGS = -module ${VINFO}

# Special targets to test the m17n library with Purify.  They are for
# the maintainers only.

PURIFY=/usr/local/rational/releases/purify.sol.2002.05.00/purify

purify_medit: medit ../src/.libs/libm17n-core.so ../src/.libs/libm17n.so ../src/.libs/libm17n-X.so linebreak.c
	${PURIFY} gcc -g medit.o linebreak.o -lXaw -lXmu -L/usr/X11R6/lib -R/usr/X11R6/lib -lSM -lICE -lX11 -lXt -L../src/.libs -lm17n-core -lm17n -lm17n-X -ldl

purify_mdate: mdate.o ../src/.libs/libm17n.so ../src/.libs/libm17n-X.so
	${PURIFY} gcc -g mdate.o -lXaw -lXmu -L/usr/X11R6/lib -R/usr/X11R6/lib -lSM -lICE -lX11 -lXt -L../src/.libs -lm17n -lm17n-X -ldl

