# Copyright (C) 2003, 2004, 2005, 2006, 2007, 2008, 2010
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

BASICBUILDS = \
	libm17n-core-@M17N_BINARY_VERSION@.la \
	libm17n-@M17N_BINARY_VERSION@.la \
	libm17n-flt-@M17N_BINARY_VERSION@.la
if WITH_GUI
BUILD_LIBS = $(BASICBUILDS) libm17n-gui-@M17N_BINARY_VERSION@.la
else
BUILD_LIBS = $(BASICBUILDS)
endif

lib_LTLIBRARIES = $(BUILD_LIBS)

libm17n_core_@M17N_BINARY_VERSION@_la_SOURCES = \
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

libm17n_core_@M17N_BINARY_VERSION@_la_LIBADD = @XML2_LD_FLAGS@ @THAI_WORDSEG_LD_FLAGS@
libm17n_core_@M17N_BINARY_VERSION@_la_LDFLAGS = -export-dynamic ${VINFO}

libm17n_@M17N_BINARY_VERSION@_la_SOURCES = \
	charset.h charset.c \
	coding.h coding.c \
	input.h input.c input-xml.c \
	language.h language.c \
	mlocale.h locale.c \
	m17n.h m17n.c
libm17n_@M17N_BINARY_VERSION@_la_LIBADD = ${top_builddir}/src/libm17n-core-@M17N_BINARY_VERSION@.la -ldl
libm17n_@M17N_BINARY_VERSION@_la_LDFLAGS = -export-dynamic ${VINFO}

FLT_SOURCES = \
	internal-flt.h \
	m17n-flt.h m17n-flt.c

libm17n_flt_@M17N_BINARY_VERSION@_la_SOURCES = ${FLT_SOURCES}
libm17n_flt_@M17N_BINARY_VERSION@_la_LIBADD = ${top_builddir}/src/libm17n-core-@M17N_BINARY_VERSION@.la
libm17n_flt_@M17N_BINARY_VERSION@_la_LDFLAGS = -export-dynamic ${VINFO}

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

libm17n_gui_@M17N_BINARY_VERSION@_la_SOURCES = ${GUI_SOURCES}
libm17n_gui_@M17N_BINARY_VERSION@_la_LIBADD = ${OPTIONAL_LD_FLAGS} ${top_builddir}/src/libm17n-core-@M17N_BINARY_VERSION@.la ${top_builddir}/src/libm17n-@M17N_BINARY_VERSION@.la ${top_builddir}/src/libm17n-flt-@M17N_BINARY_VERSION@.la
libm17n_gui_@M17N_BINARY_VERSION@_la_LDFLAGS = -export-dynamic ${VINFO}

if WITH_GUI

moduledir = ${libdir}/@M17N_MODULE_DIR@
module_LTLIBRARIES = libm17n-X.la libm17n-gd.la
X_LD_FLAGS = ${X_PRE_LIBS} ${X_LIBS} @X11_LD_FLAGS@ ${X_EXTRA_LIBS}

libm17n_X_la_SOURCES = m17n-X.h m17n-X.c
libm17n_X_la_LIBADD = ${X_LD_FLAGS} @XFT2_LD_FLAGS@ ${top_builddir}/src/libm17n-core-@M17N_BINARY_VERSION@.la ${top_builddir}/src/libm17n-@M17N_BINARY_VERSION@.la ${top_builddir}/src/libm17n-flt-@M17N_BINARY_VERSION@.la ${top_builddir}/src/libm17n-gui-@M17N_BINARY_VERSION@.la
libm17n_X_la_LDFLAGS = -avoid-version -module

libm17n_gd_la_SOURCES = m17n-gd.c
libm17n_gd_la_LIBADD = @GD_LD_FLAGS@ @FREETYPE_LD_FLAGS@ ${top_builddir}/src/libm17n-core-@M17N_BINARY_VERSION@.la ${top_builddir}/src/libm17n-@M17N_BINARY_VERSION@.la ${top_builddir}/src/libm17n-flt-@M17N_BINARY_VERSION@.la ${top_builddir}/src/libm17n-gui-@M17N_BINARY_VERSION@.la
libm17n_gd_la_LDFLAGS = -avoid-version -module

endif

AM_CPPFLAGS = -DM17NDIR=\"@M17NDIR@\" -DM17N_MODULE_DIR=\"${libdir}/@M17N_MODULE_DIR@\" -DGETTEXTDIR=\"@GETTEXTDIR@\"

BASICHEADERS = m17n-core.h m17n.h m17n-misc.h m17n-flt.h
if WITH_GUI
pkginclude_HEADERS = $(BASICHEADERS) m17n-gui.h m17n-X.h
else
pkginclude_HEADERS = $(BASICHEADERS)
endif

pkgincludedir=$(includedir)/m17n-@M17N_BINARY_VERSION@

SRC =	${libm17n_core_@M17N_BINARY_VERSION@_la_SOURCES} \
	${libm17n_@M17N_BINARY_VERSION@_la_SOURCES} \
	${libm17n_gui_@M17N_BINARY_VERSION@_la_SOURCES} \
	${libm17n_X_@M17N_BINARY_VERSION@_la_SOURCES} \
	${libm17n_gd_@M17N_BINARY_VERSION@_la_SOURCES}

TAGS: ${SRC}
	etags ${SRC}

if MAINTAINER_MODE
noinst_PROGRAMS = linkcore linkshell linkgui

linkcore_SOURCES = linkcore.c
linkcore_LDADD = ${top_builddir}/src/libm17n-core-@M17N_BINARY_VERSION@.la
linkcore_LDFLAGS = -static

linkshell_SOURCES = linkshell.c
linkshell_LDADD = ${top_builddir}/src/libm17n-@M17N_BINARY_VERSION@.la
linkshell_LDFLAGS = -static

linkgui_SOURCES = linkgui.c
linkgui_LDADD = ${top_builddir}/src/libm17n-gui-@M17N_BINARY_VERSION@.la
linkgui_LDFLAGS = -static
endif
