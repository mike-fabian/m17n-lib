# Copyright (C) 2003, 2004, 2005, 2006, 2007, 2008
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

VINFO = -version-info @API_VERSION@

BASICBUILDS = libm17n-core.la libm17n.la libm17n-flt.la
if WITH_GUI
BUILD_LIBS = $(BASICBUILDS) libm17n-gui.la libm17n-X.la libm17n-gd.la
else
BUILD_LIBS = $(BASICBUILDS)
endif

lib_LTLIBRARIES = $(BUILD_LIBS)

libm17n_core_la_SOURCES = \
	character.h character.c \
	chartab.h chartab.c \
	database.h database.c \
	internal.h \
	plist.h plist.c \
	m17n-core.h m17n-core.c \
	m17n-misc.h \
	mtext.h mtext.c \
	symbol.h symbol.c \
	textprop.h textprop.c \
	mtext-lbrk.c mtext-wseg.c

libm17n_core_la_LIBADD = @XML2_LD_FLAGS@ @THAI_WORDSEG_LD_FLAGS@
libm17n_core_la_LDFLAGS = -export-dynamic ${VINFO}

libm17n_la_SOURCES = \
	charset.h charset.c \
	coding.h coding.c \
	input.h input.c \
	language.h language.c \
	mlocale.h locale.c \
	m17n.h m17n.c
libm17n_la_LIBADD = ${top_builddir}/src/libm17n-core.la -ldl
libm17n_la_LDFLAGS = -export-dynamic ${VINFO}

FLT_SOURCES = \
	internal-flt.h \
	m17n-flt.h m17n-flt.c

libm17n_flt_la_SOURCES = ${FLT_SOURCES}
libm17n_flt_la_LIBADD = ${top_builddir}/src/libm17n-core.la
libm17n_flt_la_LDFLAGS = -export-dynamic ${VINFO}

GUI_SOURCES = \
	face.h face.c \
	font.h font.c font-ft.c \
	fontset.h fontset.c \
	draw.c \
	input-gui.c \
	internal-gui.h \
	m17n-gui.h m17n-gui.c

OPTIONAL_LD_FLAGS = \
	@FREETYPE_LD_FLAGS@ \
	@FRIBIDI_LD_FLAGS@ \
	@OTF_LD_FLAGS@ \
	@FONTCONFIG_LD_FLAGS@

libm17n_gui_la_SOURCES = ${GUI_SOURCES}
libm17n_gui_la_LIBADD = ${OPTIONAL_LD_FLAGS} ${top_builddir}/src/libm17n-core.la ${top_builddir}/src/libm17n.la ${top_builddir}/src/libm17n-flt.la
libm17n_gui_la_LDFLAGS = -export-dynamic ${VINFO}

X_LD_FLAGS = ${X_PRE_LIBS} ${X_LIBS} @X11_LD_FLAGS@ ${X_EXTRA_LIBS}

libm17n_X_la_SOURCES = m17n-X.h m17n-X.c
libm17n_X_la_LIBADD = ${X_LD_FLAGS} @XFT2_LD_FLAGS@ ${top_builddir}/src/libm17n-core.la ${top_builddir}/src/libm17n.la ${top_builddir}/src/libm17n-flt.la ${top_builddir}/src/libm17n-gui.la
libm17n_X_la_LDFLAGS = -module ${VINFO}

libm17n_gd_la_SOURCES = m17n-gd.c
libm17n_gd_la_LIBADD = @GD_LD_FLAGS@ @FREETYPE_LD_FLAGS@ ${top_builddir}/src/libm17n-core.la ${top_builddir}/src/libm17n.la ${top_builddir}/src/libm17n-flt.la ${top_builddir}/src/libm17n-gui.la
libm17n_gd_la_LDFLAGS = -module ${VINFO}

AM_CPPFLAGS = -DM17NDIR=\"@M17NDIR@\" -DGETTEXTDIR=\"@GETTEXTDIR@\"


BASICHEADERS = m17n-core.h m17n.h m17n-misc.h m17n-flt.h
if WITH_GUI
include_HEADERS = $(BASICHEADERS) m17n-gui.h m17n-X.h
else
include_HEADERS = $(BASICHEADERS)
endif

SRC =	${libm17n_core_la_SOURCES} \
	${libm17n_la_SOURCES} \
	${libm17n_gui_la_SOURCES} \
	${libm17n_X_la_SOURCES} \
	${libm17n_gd_la_SOURCES}

TAGS: ${SRC}
	etags ${SRC}

if MAINTAINER_MODE
noinst_PROGRAMS = linkcore linkshell linkgui

linkcore_SOURCES = linkcore.c
linkcore_LDADD = ${top_builddir}/src/libm17n-core.la
linkcore_LDFLAGS = -static

linkshell_SOURCES = linkshell.c
linkshell_LDADD = ${top_builddir}/src/libm17n.la
linkshell_LDFLAGS = -static

linkgui_SOURCES = linkgui.c
linkgui_LDADD = ${top_builddir}/src/libm17n-gui.la
linkgui_LDFLAGS = -static
endif
