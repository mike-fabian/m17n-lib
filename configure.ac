dnl configure.ac -- autoconf script for the m17n library.

dnl Copyright (C) 2003, 2004, 2005, 2006
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
dnl Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
dnl Boston, MA 02110-1301, USA.

dnl Process this file with autoconf to produce a configure script.

AC_INIT(m17n-lib, 1.3.4, m17n-lib-bug@m17n.org)
AM_INIT_AUTOMAKE
AM_CONFIG_HEADER(src/config.h)
AM_MAINTAINER_MODE

API_VERSION=1:0:1
AC_SUBST(API_VERSION)

AM_GNU_GETTEXT
GETTEXTDIR="$datadir/locale"
AC_SUBST(GETTEXTDIR)

dnl Checks for programs for compiling.
AC_PROG_CC
AC_LIBTOOL_DLOPEN
AC_LIBLTDL_CONVENIENCE
AM_PROG_LIBTOOL

dnl Checks for X libraries.
AC_PATH_XTRA

dnl Checks for standard header files.
AC_HEADER_STDC
AC_HEADER_DIRENT
AC_HEADER_TIME
AC_CHECK_HEADERS([fcntl.h langinfo.h limits.h locale.h stdlib.h \
			  string.h strings.h sys/time.h unistd.h])
AC_CHECK_HEADER(X11/Xaw/Command.h, HAVE_XAW=yes)

dnl Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_C_INLINE
AC_TYPE_SIZE_T
AC_STRUCT_TM

dnl Checks for endian.  This influence the default UTF-16 definition.
AC_C_BIGENDIAN

dnl Checks for library functions.
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

dnl Checks where the m17n database is installed.

AC_CHECK_PROG(HAVE_M17N_DB, m17n-db, yes)
if test "x$HAVE_M17N_DB" = "xyes"; then
  M17NDIR=`m17n-db`
else
  M17NDIR="${datadir}/m17n"
fi
AC_SUBST(M17NDIR)

dnl Checks which levels of APIs should be compiled.

AC_ARG_ENABLE(gui,
	      AC_HELP_STRING([--with-gui],
			     [with GUI level APIs (default is YES)]))

AM_CONDITIONAL(WITH_GUI, test x$with_gui != xno)

if test x$with_gui != xno; then

dnl Checks if dlopen exists, and if it's in libc or libdl.

AC_SEARCH_LIBS(dlopen, dl,
	       AC_DEFINE(HAVE_DLOPEN, 1,
	       [Define to 1 if you have the function dlopen.]))

dnl Check several libraries without adding -lxxx to LIBS, without
dnl defining HAVE_LIBXXX nor HAVE_XXX_H because they are used only by
dnl some libraries (e.g. libm17n-X.so).  Instead, define XXX_LD_FLAGS
dnl and HAVE_XXX if library XXX is available.  CPPFLAGS will be
dnl augmented.

M17N_EXT_LIBS=

AC_CHECK_PROG(HAVE_PKG_CONFIG, pkg-config, yes)

if test "x$no_x" != "xyes"; then
  AC_DEFINE(HAVE_X11, 1, [Define to 1 if you have X11.])
  X11_LD_FLAGS="-lXt -lX11"
  M17N_EXT_LIBS="$M17N_EXT_LIBS X11"

  if test "x$HAVE_XAW" = "xyes"; then
    AC_DEFINE(HAVE_X11_XAW_COMMAND_H, 1,
	      [Define to 1 if you have the Xaw header files.])
    XAW_LD_FLAGS="-lXaw -lXmu"
    CONFIG_FLAGS="$CONFIG_FLAGS -DHAVE_X11_XAW_COMMAND_H"
    M17N_EXT_LIBS="$M17N_EXT_LIBS Xaw"
  fi
fi
AC_SUBST(X11_LD_FLAGS)
AC_SUBST(XAW_LD_FLAGS)

dnl Check for fribidi library.
save_CPPFLAGS="$CPPFLAGS"
save_LIBS="$LIBS"
AC_CHECK_PROG(HAVE_FRIBIDI_CONFIG, fribidi-config, yes)
if test "x$HAVE_FRIBIDI_CONFIG" = "xyes"; then
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
  M17N_EXT_LIBS="$M17N_EXT_LIBS fribidi"
else
  CPPFLAGS="$save_CPPFLAGS"
  FRIBIDI_LD_FLAGS=
fi
LIBS="$save_LIBS"
AC_SUBST(FRIBIDI_LD_FLAGS)

dnl Check for otflib usability.
AC_ARG_WITH(libotf, 
	    AC_HELP_STRING([--with-libotf],
			   [with OpenType font suport (default is YES)]))

if test "x$with_libotf" != "xno"; then
  save_CPPFLAGS="$CPPFLAGS"
  save_LIBS="$LIBS"
  AC_CHECK_PROG(HAVE_OTFLIB_CONFIG, libotf-config, yes)
  if test "x$HAVE_OTFLIB_CONFIG" = "xyes"; then
    CPPFLAGS="$CPPFLAGS `libotf-config --cflags`"
    OTF_LD_FLAGS="`libotf-config --libs`"
    LIBS="$LIBS $OTF_LD_FLAGS"
  else
    OTF_LD_FLAGS=-lotf
  fi
  ## We check the availability of OTF_check_features
  ## because we need libotf-0.9.4 or the later.
  AC_CHECK_LIB(otf, OTF_check_features, HAVE_OTF=yes, HAVE_OTF=no)
  AC_CHECK_HEADER(otf.h,, HAVE_OTF=no)
  if test "x$HAVE_OTF" = "xyes"; then
    AC_DEFINE(HAVE_OTF, 1, 
	      [Define to 1 if you have OTF library and header file.])
    M17N_EXT_LIBS="$M17N_EXT_LIBS libotf"
  else
    CPPFLAGS="$save_CPPFLAGS"
    OTF_LD_FLAGS=
  fi
  LIBS="$save_LIBS"
fi
AC_SUBST(OTF_LD_FLAGS)

dnl Check for Freetype2 usability.
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
      FREETYPE_LD_FLAGS=`freetype-config --libs`
      AC_DEFINE(HAVE_FREETYPE, 1, 
		[Define to 1 if you have FreeType library and header file.])
      M17N_EXT_LIBS="$M17N_EXT_LIBS freetype"
      AC_CHECK_HEADER(freetype/ftbdf.h, HAVE_FTBDF_H=yes, HAVE_FTBDF_H=no,
		      [#include <ft2build.h>
#include FT_FREETYPE_H])
      if test "x$HAVE_FTBDF_H" = "xyes"; then
        AC_DEFINE(HAVE_FTBDF_H, 1,
		  [Define to 1 if you have freetype/ftbdf.h.])
      fi
      CONFIG_FLAGS="$CONFIG_FLAGS -DHAVE_FREETYPE"
    fi
  fi
fi
AC_SUBST(FREETYPE_LD_FLAGS)

dnl Check for Xft2 usability.
save_CPPFLAGS="$CPPFLAGS"
save_LIBS="$LIBS"
XFT2_LD_FLAGS="-lXft -lfreetype -lfontconfig -lXrender -lX11"
AC_CHECK_PROG(HAVE_XFT_CONFIG, xft-config, yes)
if test "x$HAVE_XFT_CONFIG" = "xyes"; then
  CPPFLAGS="$CPPFLAGS `xft-config --cflags`"
  XFT2_LD_FLAGS="`xft-config --libs`"
elif test "x$HAVE_PKG_CONFIG" = "xyes" ; then
  if pkg-config xft ; then
    CPPFLAGS="$CPPFLAGS `pkg-config --cflags xft`"
    XFT2_LD_FLAGS="`pkg-config --libs xft`"
  fi
fi
LIBS="$LIBS $XFT2_LD_FLAGS"
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
  M17N_EXT_LIBS="$M17N_EXT_LIBS xft2"
else
  XFT2_LD_FLAGS=
  CPPFLAGS="$save_CPPFLAGS"
fi
LIBS="$save_LIBS"
AC_SUBST(XFT2_LD_FLAGS)

dnl Check for fontconfig usability.
AC_ARG_WITH(fontconfig,
	    AC_HELP_STRING([--with-fontconfig],
	                   [with FontConfig library (default is YES)]))
if test "x$with_fontconfig" != "xno"; then
  save_CPPFLAGS="$CPPFLAGS"
  save_LIBS="$LIBS"
  FONTCONFIG_LD_FLAGS=-lfontconfig
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
    M17N_EXT_LIBS="$M17N_EXT_LIBS fontconfig"
  else
    FONTCONFIG_LD_FLAGS=
    CPPFLAGS="$save_CPPFLAGS"
  fi
  LIBS="$save_LIBS"
fi
AC_SUBST(FONTCONFIG_LD_FLAGS)

dnl Check for gdlib usability.
AC_ARG_WITH(gd, 
	    AC_HELP_STRING([--with-gd],
			   [suport graphic device by GD library (default is YES)]))

if test "x$with_gd" != "xno"; then
  save_LIBS="$LIBS"
  AC_CHECK_LIB(gd, gdImageCreate, HAVE_GD=yes, HAVE_GD=no)
  AC_CHECK_HEADER(gd.h, , HAVE_GD=no)
  if test "x$HAVE_GD" = "xyes"; then
    AC_CHECK_LIB(gd, gdImageCreateTrueColor, HAVE_GD=2, HAVE_GD=1)
    AC_DEFINE_UNQUOTED(HAVE_GD, $HAVE_GD,
	      [Define to 1 or 2 if you have gd library and header file.])
    M17N_EXT_LIBS="$M17N_EXT_LIBS gdlib"
    GD_LD_FLAGS=-lgd
    CONFIG_FLAGS="$CONFIG_FLAGS -DHAVE_GD=$HAVE_GD"
  fi
  LIBS="$save_LIBS"
fi
AC_SUBST(GD_LD_FLAGS)

fi

dnl Check for libxml2 usability.
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
  M17N_EXT_LIBS="$M17N_EXT_LIBS xml2"
else
  XML2_LD_FLAGS=
  CPPFLAGS="$save_CPPFLAGS"
fi
LIBS="$save_LIBS"
AC_SUBST(XML2_LD_FLAGS)

dnl Check for Anthy usability.
AC_CHECK_LIB(anthydic, anthy_init_sessions, HAVE_ANTHY=yes, HAVE_ANTHY=no)
if test "x$HAVE_ANTHY" = "xyes"; then
  AC_CHECK_LIB(anthy, anthy_init, HAVE_ANTHY=yes, HAVE_ANTHY=no, -lanthydic)
  if test "x$HAVE_ANTHY" = "xyes"; then
    AC_CHECK_HEADER(anthy/anthy.h, HAVE_ANTHY=yes, HAVE_ANTHY=no)
    if test "x$HAVE_ANTHY" = "xyes"; then
      AC_DEFINE(HAVE_ANTHY, 1,
		[Define to 1 if you have Anthy library and header file])
      M17N_EXT_LIBS="$M17N_EXT_LIBS anthy"
      ANTHY_LD_FLAGS="-lanthy -lanthydic"
      CONFIG_FLAGS="$CONFIG_FLAGS -DHAVE_ANTHY"
    fi
  fi
fi
AC_SUBST(ANTHY_LD_FLAGS)

dnl Check for Ispell usability.
AC_CHECK_PROG(HAVE_ISPELL, ispell, yes)
if test "x$HAVE_ISPELL" = "xyes"; then
  AC_DEFINE(HAVE_ISPELL, 1, [Define if ispell is available.])
  M17N_EXT_LIBS="$M17N_EXT_LIBS ispell"
  CONFIG_FLAGS="$CONFIG_FLAGS -DHAVE_ISPELL"
fi

dnl Check for Thai word-segmentation library.
dnl If we have one, define HAVE_THAI_WORDSEG and one of these:
dnl   HAVE_LIBTHAI, HAVE_WORDCUT, or HAVE_WORDCUT_OLD
dnl In addition, set THAI_WORDSEG_LD_FLAGS to a proper value.

PKG_CHECK_MODULES(LIBTHAI, libthai, HAVE_LIBTHAI=yes, HAVE_LIBTHAI=no)
PKG_CHECK_MODULES(WORDCUT, wordcut, HAVE_WORDCUT=yes, HAVE_WORDCUT=no)

if test "x$HAVE_LIBTHAI" == "xyes"; then

  AC_DEFINE(HAVE_LIBTHAI, 1, [Define if you have libthai])
  THAI_WORDSEG_LD_FLAGS="$LIBTHAI_LIBS"
  M17N_EXT_LIBS="$M17N_EXT_LIBS libthai"
  HAVE_THAI_WORDSEG=yes

elif test "x$HAVE_WORDCUT" = "xyes"; then

  save_CPPFLAGS="$CPPFLAGS"
  CPPFLAGS="$CPPFLAGS $WORDCUT_CFLAGS"
  AC_TRY_CPP([#include <wordcut/wcwordcut.h>], , HAVE_WORDCUT=no)
  if test "x$HAVE_WORDCUT" = "xno"; then
    PKG_CHECK_MODULES(GLIB_2_0, glib-2.0, HAVE_GLIB_2_0=yes, HAVE_GLIB_2_0=no)
    if test "x$HAVE_GLIB_2_0" = "xyes"; then
      CPPFLAGS="$CPPFLAGS $GLIB_2_0_CFLAGS"
      AC_TRY_CPP([#include <wordcut/wcwordcut.h>], HAVE_WORDCUT=yes)
    fi
  fi
  if test "x$HAVE_WORDCUT" = "xyes"; then
    THAI_WORDSEG_LD_FLAGS="$WORDCUT_LIBS"
    AC_DEFINE(HAVE_WORDCUT, 1,
	      [Define if you have the wordcut library and header file])
    M17N_EXT_LIBS="$M17N_EXT_LIBS wordcut"
    HAVE_THAI_WORDSEG=yes
  else
    CPPFLAGS="$save_CPPFLAGS"
  fi

else

  AC_CHECK_LIB(wordcut, wordcut_init, HAVE_WORDCUT_OLD=yes)
  if test "x$HAVE_WORDCUT_OLD" = "xyes"; then
    if test -f "/usr/share/wordcut/tdict.wcd"; then
      tdict="/usr/share/wordcut/tdict.wcd"
    elif test -f "/usr/local/share/wordcut/tdict.wcd"; then
      tdict="/usr/local/share/wordcut/tdict.wcd"
    fi
    echo "TDICT=$tdict"
    if test "x$tdict" != "x"; then
      AC_DEFINE(HAVE_WORDCUT_OLD, 1,
		[Define if you have the old version of wordcut library])
      AC_DEFINE_UNQUOTED(WORDCUT_TDICT, "$tdict", [Define tdict file name])
      THAI_WORDSEG_LD_FLAGS=-lwordcut
      M17N_EXT_LIBS="$M17N_EXT_LIBS wordcut-old"
      HAVE_THAI_WORDSEG=yes
    else
      HAVE_WORDCUT=no
    fi
  fi
fi  

if test "x$HAVE_THAI_WORDSEG" = "xyes"; then
  AC_DEFINE(HAVE_THAI_WORDSEG, 1,
            [Define if you have some Thai word-segmentation library])
fi
AC_SUBST(THAI_WORDSEG_LD_FLAGS)

AC_SUBST(CONFIG_FLAGS)

dnl We can't include X_CFLAGS in AM_CPPFLAGS because the generated
dnl Makefile put ${AM_CPPFLAGS} before ${CPPFLAGS} and that leads to
dnl inclusion of an incorrect header file.  So, append X_CFLAGS to
dnl CPPFLAGS here directly.

CPPFLAGS="$CPPFLAGS $X_CFLAGS"

dnl AC_ARG_ENABLE(xom, 
dnl	      [  --enable-xom            build and install XOM library.],
dnl	      XOM="$enableval")

SHLIB_EXT=".so"
case $host_os in
darwin* | rhapsody*)
  SHLIB_EXT=".dylib";;
esac
AC_DEFINE_UNQUOTED(DLOPEN_SHLIB_EXT, "$SHLIB_EXT",
		   [Define to loadable module extention])

AC_CONFIG_FILES([Makefile po/Makefile.in intl/Makefile
                 src/Makefile
                 example/Makefile
		 m17n-config
		 m17n-core.pc
		 m17n-shell.pc
		 m17n-gui.pc
		 ])

dnl if test "x$XOM" = "xyes"; then
dnl  AC_CONFIG_FILES(omM17N/Makefile)
dnl fi

AC_OUTPUT

echo "The m17n library is configured with these external libraries."
echo " $M17N_EXT_LIBS"

dnl Local Variables:
dnl comment-start: "dnl "
dnl comment-end: ""
dnl comment-start-skip: "\\bdnl\\b\\s *"
dnl End:
