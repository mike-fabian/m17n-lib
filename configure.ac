dnl configure.ac -- autoconf script for the m17n library.

dnl Copyright (C) 2003, 2004
dnl   National Institute of Advanced Industrial Science and Technology (AIST)
dnl   Registration Number H15PRO112

dnl This file is part of the m17n library.

dnl The m17n library is free software; you can redistribute it and/or
dnl modify it under the terms of the GNU Lesser General Public License
dnl as published by the Free Software Foundation; either version 2.1 of
dnl the License, or (at your option) any later version.

dnl The m17n library is distributed in the hope that it will be useful,
dnl but WITHOUT ANY WARRANTY; without even the implied warranty of
dnl MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
dnl Lesser General Public License for more details.

dnl You should have received a copy of the GNU Lesser General Public
dnl License along with the m17n library; if not, write to the Free
dnl Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
dnl 02111-1307, USA.

dnl Process this file with autoconf to produce a configure script.

AC_INIT(m17n-lib, 1.1.0, m17n-lib-bug@m17n.org)
AM_INIT_AUTOMAKE(m17n-lib, 1.1.0)
AM_CONFIG_HEADER(src/config.h)

# Checks for programs for compiling.
AC_PROG_CC
AC_LIBTOOL_DLOPEN
AC_LIBLTDL_CONVENIENCE
AM_PROG_LIBTOOL

# Checks for X libraries.
AC_PATH_XTRA

# Checks for standard header files.
AC_HEADER_STDC
AC_HEADER_DIRENT
AC_HEADER_TIME
AC_CHECK_HEADERS([fcntl.h langinfo.h limits.h locale.h stdlib.h \
			  string.h strings.h sys/time.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_C_INLINE
AC_TYPE_SIZE_T
AC_STRUCT_TM

# Checks for endian.  This influence the default UTF-16 definition.
AC_C_BIGENDIAN

# Checks for library functions.
AC_FUNC_ALLOCA
AC_FUNC_MALLOC
AC_FUNC_REALLOC
AC_FUNC_MEMCMP
AC_FUNC_STAT
AC_FUNC_STRCOLL
AC_FUNC_STRFTIME
AC_FUNC_STRTOD
AC_CHECK_FUNCS(memmove memset nl_langinfo putenv regcomp setlocale)
AC_CHECK_FUNCS(strchr strdup gettimeofday)

# Check several libraries without adding -lxxx to LIBS, without
# defining HAVE_LIBXXX nor HAVE_XXX_H because they are used only for
# libm17-X.so.  Instead, define XXX_LD_FLAGS and HAVE_XXX if library
# XXX is available.  CPPFLAGS will be augmented.

# Check for fribidi library.
save_CPPFLAGS="$CPPFLAGS"
save_LIBS="$LIBS"
AC_CHECK_PROG(HAVE_FRIBIDI_CONFIG, fribidi-config, yes)
if test "$HAVE_FRIBIDI_CONFIG" = "xyes"; then
  CPPFLAGS="$CPPFLAGS `fribidi-config --cflags`"
  FRIBIDI_LD_FLAGS="`fribidi-config --libs`"
  LIBS="$LIBS $FRIBIDI_LD_FLAGS"
else
  FRIBIDI_LD_FLAGS=-lfribidi
fi
AC_CHECK_LIB(fribidi, fribidi_set_mirroring, HAVE_FRIBIDI=yes, HAVE_FRIBIDI=no)
AC_CHECK_HEADER(fribidi/fribidi.h,, HAVE_FRIBIDI=no)
if test "x$HAVE_FRIBIDI" = "xyes"; then
  AC_DEFINE(HAVE_FRIBIDI, 1, 
	    [Define to 1 if you have Fribidi library and header file.])
else
  CPPFLAGS="$save_CPPFLAGS"
  FRIBIDI_LD_FLAGS=
fi
LIBS="$save_LIBS"
AC_SUBST(FRIBIDI_LD_FLAGS)

# Check for otflib usability.
save_CPPFLAGS="$CPPFLAGS"
save_LIBS="$LIBS"
AC_CHECK_PROG(HAVE_OTFLIB_CONFIG, libotf-config, yes)
if test "x$HAVE_OTFLIB_CONFIG" = "xyes"; then
  CPPFLAGS="$CPPFLAGS `libotf-config --cflags`"
  OTF_LD_FLAGS="`libotf-config --libs`  "
  LIBS="$LIBS $OTF_LD_FLAGS"
else
  OTF_LD_FLAGS=-lotf
fi
AC_CHECK_LIB(otf, OTF_open, HAVE_OTF=yes, HAVE_OTF=no)
AC_CHECK_HEADER(otf.h,, HAVE_OTF=no)
CPPFLAGS="$save_CPPFLAGS"
if test "x$HAVE_OTF" = "xyes"; then
  AC_DEFINE(HAVE_OTF, 1, 
	    [Define to 1 if you have OTF library and header file.])
else
  CPPFLAGS="$save_CPPFLAGS"
  OTF_LD_FLAGS=
fi
LIBS="$save_LIBS"
AC_SUBST(OTF_LD_FLAGS)

# Check for Freetype2 usability.
AC_CHECK_PROG(HAVE_FREETYPE_CONFIG, freetype-config, yes)
if test "x$HAVE_FREETYPE_CONFIG" = "xyes"; then
  FREETYPE_INC=`freetype-config --cflags`
  save_CPPFLAGS="$CPPFLAGS"
  CPPFLAGS="$CPPFLAGS $FREETYPE_INC"
  AC_CHECK_HEADER(ft2build.h, HAVE_FREETYPE=yes,
  			      HAVE_FREETYPE=no CPPFLAGS="$save_CPPFLAGS")
  if test "x$HAVE_FREETYPE" = "xyes" ; then
    save_LIBS="$LIBS"
    LIBS="$LIBS `freetype-config --libs`"
    AC_CHECK_LIB(freetype, FT_Init_FreeType, HAVE_FREETYPE=yes,
    			   		     HAVE_FREETYPE=no)
    LIBS="$save_LIBS"
    if test "x$HAVE_FREETYPE" = "xyes"; then
      FREETYPE_LD_FLAGS=`freetype-config --libtool`
      AC_DEFINE(HAVE_FREETYPE, 1, 
		[Define to 1 if you have FreeType library and header file.])
    fi
  fi
fi
AC_SUBST(FREETYPE_LD_FLAGS)

# Check for Xft2 usability.
save_CPPFLAGS="$CPPFLAGS"
save_LIBS="$LIBS"
AC_CHECK_PROG(HAVE_XFT_CONFIG, xft-config, yes)
if test "x$HAVE_XFT_CONFIG" = "xyes"; then
  CPPFLAGS="$CPPFLAGS `xft-config --cflags`"
  XFT2_LD_FLAGS="`xft-config --libs`"
  LIBS="$LIBS $XFT2_LD_FLAGS"
else
  XFT2_LD_FLAGS="-lXft -lfreetype -lfontconfig -lXrender -lX11"
fi
AC_CHECK_LIB(Xft, XftDrawCreate, HAVE_XFT2=yes, HAVE_XFT2=no)
AC_CHECK_HEADER(X11/Xft/Xft.h,, HAVE_XFT2=no, 
		[#include <X11/Xft/Xft.h>
#if XftVersion >= 20000
#else
Version too old.  Compiling this line should fail.
#endif])
if test "x$HAVE_XFT2" = "xyes"; then
  AC_DEFINE(HAVE_XFT2, 1,
	    [Define to 1 if you have Xft2 library and header file.])
else
  CPPFLAGS="$save_CPPFLAGS"
fi
LIBS="$save_LIBS"
AC_SUBST(XFT2_LD_FLAGS)

# Check for fontconfig usability.
save_CPPFLAGS="$CPPFLAGS"
save_LIBS="$LIBS"
FONTCONFIG_LD_FLAGS=-lfontconfig
AC_CHECK_PROG(HAVE_PKG_CONFIG, pkg-config, yes)
if test "x$HAVE_PKG_CONFIG" = "xyes"; then
  if pkg-config --exists fontconfig; then
    CPPFLAGS="$CPPFLAGS `pkg-config --cflags fontconfig`"
    FONTCONFIG_LD_FLAGS="`pkg-config --libs fontconfig`"
    LIBS="$LIBS $FONTCONFIG_LD_FLAGS"
  fi
fi
AC_CHECK_LIB(fontconfig, FcInit, HAVE_FONTCONFIG=yes, HAVE_FONTCONFIG=no)
AC_CHECK_HEADER(fontconfig/fontconfig.h, , HAVE_FONTCONFIG=no)
if test "x$HAVE_FONTCONFIG" = "xyes"; then
  AC_DEFINE(HAVE_FONTCONFIG, 1,
	      [Define to 1 if you have fontconfig library and header file.])
else
  FONTCONFIG_LD_FLAGS=
  CPPFLAGS="$save_CPPFLAGS"
fi
LIBS="$save_LIBS"
AC_SUBST(FONTCONFIG_LD_FLAGS)

# Check for gdlib usability.
save_LIBS="$LIBS"
AC_CHECK_LIB(gd, gdImageCreate, HAVE_GD=yes, HAVE_GD=no)
AC_CHECK_HEADER(gd.h, , HAVE_GD=no)
if test "x$HAVE_GD" = "xyes"; then
  AC_CHECK_LIB(gd, gdImageCreateTrueColor, HAVE_GD=2, HAVE_GD=1)
  AC_DEFINE_UNQUOTED(HAVE_GD, $HAVE_GD,
	    [Define to 1 if you have gd library and header file.])
  GD_LD_FLAGS=-lgd
fi
LIBS="$save_LIBS"
AC_SUBST(GD_LD_FLAGS)

# Check for libxml2 usability.
save_CPPFLAGS="$CPPFLAGS"
save_LIBS="$LIBS"
AC_CHECK_PROG(HAVE_XML2_CONFIG, xml2-config, yes)
if test "x$HAVE_XML2_CONFIG" = "xyes"; then
  CPPFLAGS="$CPPFLAGS `xml2-config --cflags`"
  XML2_LD_FLAGS="`xml2-config --libs`"
  LIBS="$LIBS $XML2_LD_FLAGS"
else
  CPPFLAGS="$CPPFLAGS -I/usr/include/libxml2"
  XML2_LD_FLAGS="-lxml2 -lz -lpthread -lm"
fi
AC_CHECK_LIB(xml2, xmlParseMemory, HAVE_XML2=yes, HAVE_XML2=no)
AC_CHECK_HEADER(libxml/tree.h,, HAVE_XML2=no, /**/)
if test "x$HAVE_XML2" = "xyes"; then
  AC_DEFINE(HAVE_XML2, 1,
	    [Define to 1 if you have libxml2 library and header file])
else
  CPPFLAGS="$save_CPPFLAGS"
fi
LIBS="$save_LIBS"
AC_SUBST(XML2_LD_FLAGS)

# Check for Anthy usability.
AC_CHECK_LIB(anthydic, anthy_init_sessions, HAVE_ANTHY=yes, HAVE_ANTHY=no)
if test "x$HAVE_ANTHY" = "xyes"; then
  AC_CHECK_LIB(anthy, anthy_init, HAVE_ANTHY=yes, HAVE_ANTHY=no, -lanthydic)
  if test "x$HAVE_ANTHY" = "xyes"; then
    AC_CHECK_HEADER(anthy/anthy.h, HAVE_ANTHY=yes, HAVE_ANTHY=no)
    if test "x$HAVE_ANTHY" = "xyes"; then
    AC_DEFINE(HAVE_ANTHY, 1,
	      [Define to 1 if you have Anthy library and header file])
    ANTHY_LD_FLAGS="-lanthy -lanthydic"
    fi
  fi
fi
AC_SUBST(ANTHY_LD_FLAGS)

# Check for Ispell usability.
AC_CHECK_PROG(HAVE_ISPELL, ispell, yes)
if test "x$HAVE_ISPELL" = "xyes"; then
  AC_DEFINE(HAVE_ISPELL, 1, [Define if ispell is available.])
fi

# Check for libwordcut (for Thai).
AC_CHECK_LIB(wordcut, wordcut_init, HAVE_WORDCUT=yes, HAVE_WORDCUT=no)
if test "x$HAVE_WORDCUT" = "xyes"; then
  if test -f "/usr/share/wordcut/tdict.wcd"; then
    tdict="/usr/share/wordcut/tdict.wcd"
  elif test -f "/usr/local/share/wordcut/tdict.wcd"; then
    tdict="/usr/local/share/wordcut/tdict.wcd"
  fi
  echo "TDICT=$tdict"
  if test "x$tdict" != "x"; then
    AC_DEFINE(HAVE_WORDCUT, 1,
    	      [Define if you have the wordcut library and header file])
    AC_DEFINE_UNQUOTED(WORDCUT_TDICT, "$tdict", [Define to tdict file name])
    WORDCUT_LD_FLAGS=-lwordcut
  fi
fi
AC_SUBST(WORDCUT_LD_FLAGS)

dnl AC_ARG_ENABLE(xom, 
dnl	      [  --enable-xom            build and install XOM library.],
dnl	      XOM="$enableval")

case $host_os in
darwin* | rhapsody*)
  AC_DEFINE(DLOPEN_SHLIB_EXT, ".dylib",
  	    [Define to loadable module extention]);;
esac

AC_CONFIG_FILES([Makefile
                 src/Makefile
                 example/Makefile
		 m17n-config
		 ])

dnl if test "x$XOM" = "xyes"; then
dnl  AC_CONFIG_FILES(omM17N/Makefile)
dnl fi

AC_OUTPUT

dnl Local Variables:
dnl comment-start: "dnl "
dnl comment-end: ""
dnl comment-start-skip: "\\bdnl\\b\\s *"
dnl End:
