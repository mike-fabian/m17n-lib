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

bin_PROGRAMS = mconv mview mdate mdump medit

common_ldflags = ${top_srcdir}/src/libm17n-core.la ${top_srcdir}/src/libm17n.la
common_ldflags_gui = ${top_srcdir}/src/libm17n-gui.la

mdate_SOURCES = mdate.c
mdate_LDADD = ${common_ldflags}

mconv_SOURCES = mconv.c
mconv_LDADD = ${common_ldflags}

medit_SOURCES = medit.c linebreak.c
medit_LDADD = ${common_ldflags_gui}
medit_LDFLAGS = ${X_PRE_LIBS} ${X_LIBS} -lXaw -lXmu ${X_EXTRA_LIBS} @WORDCUT_LD_FLAGS@

mview_SOURCES = mview.c
mview_LDADD = ${common_ldflags_gui}
mview_LDFLAGS = ${X_PRE_LIBS} ${X_LIBS} -lXaw -lXmu ${X_EXTRA_LIBS}

mdump_SOURCES = mdump.c linebreak.c
mdump_LDADD = ${common_ldflags_gui}
mdump_LDFLAGS = @GD_LD_FLAGS@ @WORDCUT_LD_FLAGS@

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
libmimx_ispell_la_LDFLAGS = ${VINFO}
libmimx_anthy_la_SOURCES = mimx-anthy.c
libmimx_anthy_la_LIBADD = ${common_ldflags}
libmimx_anthy_la_LDFLAGS = @ANTHY_LD_FLAGS@  ${VINFO}

# Special targets to test the m17n library with Purify.  They are for
# the maintainers only.

PURIFY=/usr/local/rational/releases/purify.sol.2002.05.00/purify

purify_medit: medit ../src/.libs/libm17n-core.so ../src/.libs/libm17n.so ../src/.libs/libm17n-X.so linebreak.c
	${PURIFY} gcc -g medit.o linebreak.o -lXaw -lXmu -L/usr/X11R6/lib -R/usr/X11R6/lib -lSM -lICE -lX11 -lXt -L../src/.libs -lm17n-core -lm17n -lm17n-X -ldl

purify_mdate: mdate.o ../src/.libs/libm17n.so ../src/.libs/libm17n-X.so
	${PURIFY} gcc -g mdate.o -lXaw -lXmu -L/usr/X11R6/lib -R/usr/X11R6/lib -lSM -lICE -lX11 -lXt -L../src/.libs -lm17n -lm17n-X -ldl

