/* m17n.h -- header file for the SHELL API of the m17n library.
   Copyright (C) 2003, 2004, 2005, 2006, 2007, 2008, 2009, 2010, 2011, 2012
     National Institute of Advanced Industrial Science and Technology (AIST)
     Registration Number H15PRO112

   This file is part of the m17n library.

   The m17n library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public License
   as published by the Free Software Foundation; either version 2.1 of
   the License, or (at your option) any later version.

   The m17n library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with the m17n library; if not, write to the Free
   Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   02111-1307, USA.  */

#ifndef _M17N_H_
#define _M17N_H_

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#ifndef _M17N_CORE_H_
#include <m17n-core.h>
#endif

M17N_BEGIN_HEADER

#if !defined (FOR_DOXYGEN) || defined (DOXYGEN_INTERNAL_MODULE)
extern void m17n_init (void);
#undef M17N_INIT
#ifdef _M17N_FLT_H_
#define M17N_INIT()	\
  do {			\
    m17n_init ();	\
    m17n_init_flt ();	\
  } while (0)
#else  /* not _M17N_FLT_H_ */
#define M17N_INIT() m17n_init ()
#endif	/* not _M17N_FLT_H_ */

extern void m17n_fini (void);
#undef M17N_FINI
#ifdef _M17N_FLT_H_
#define M17N_FINI()	\
  do {			\
    m17n_fini_flt ();	\
    m17n_fini ();	\
  } while (0)
#else  /* not _M17N_FLT_H_ */
#define M17N_FINI() m17n_fini ()
#endif	/* not _M17N_FLT_H_ */

#endif

/***en @defgroup m17nShell SHELL API
    @brief API provided by libm17n.so */
/***ja @defgroup m17nShell シェル API
    @brief libm17n.so が提供する API */
/*=*/

/*
 *  (11) Functions related to the m17n database
 */
/*=*/

/*=*/
/* (S2) Charset staffs */
/*=*/

/*** @ingroup m17nShell */
/***en @defgroup m17nCharset Charset */
/***ja @defgroup m17nCharset 文字セット */
/*=*/

#if !defined (FOR_DOXYGEN) || defined (DOXYGEN_INTERNAL_MODULE)
#define MCHAR_INVALID_CODE 0xFFFFFFFF
#endif

/* Predefined charsets */ 
extern MSymbol Mcharset_ascii;
extern MSymbol Mcharset_iso_8859_1;
extern MSymbol Mcharset_unicode;
extern MSymbol Mcharset_m17n;
extern MSymbol Mcharset_binary;

/* Predefined keys for mchar_define_charset ().  */ 
extern MSymbol Mmethod;
extern MSymbol Mdimension;
extern MSymbol Mmin_range;
extern MSymbol Mmax_range;
extern MSymbol Mmin_code;
extern MSymbol Mmax_code;
extern MSymbol Mascii_compatible;
extern MSymbol Mfinal_byte;
extern MSymbol Mrevision;
extern MSymbol Mmin_char;
extern MSymbol Mmapfile;
extern MSymbol Mparents;
extern MSymbol Msubset_offset;
extern MSymbol Mdefine_coding;
extern MSymbol Maliases;

/* Methods of a charset.  */
extern MSymbol Moffset;
extern MSymbol Mmap;
extern MSymbol Munify;
extern MSymbol Msubset;
extern MSymbol Msuperset;

/* etc. */
extern MSymbol mchar_define_charset (const char *name, MPlist *plist);

extern MSymbol mchar_resolve_charset (MSymbol symbol);

extern int mchar_list_charset (MSymbol **symbols);

extern int mchar_decode (MSymbol charset_name, unsigned code);

extern unsigned mchar_encode (MSymbol charset_name, int c);

extern int mchar_map_charset (MSymbol charset_name,
			      void (*func) (int from, int to, void *arg),
			      void *func_arg);

/*=*/

/* (S3) code conversion */
/*=*/
/*** @ingroup m17nShell */
/***en @defgroup m17nConv Code Conversion */
/***ja @defgroup m17nConv コード変換 */
/*=*/

/* Predefined coding systems */
extern MSymbol Mcoding_us_ascii;
extern MSymbol Mcoding_iso_8859_1;
extern MSymbol Mcoding_utf_8;
extern MSymbol Mcoding_utf_8_full;
extern MSymbol Mcoding_utf_16;
extern MSymbol Mcoding_utf_16be;
extern MSymbol Mcoding_utf_16le;
extern MSymbol Mcoding_utf_32;
extern MSymbol Mcoding_utf_32be;
extern MSymbol Mcoding_utf_32le;
extern MSymbol Mcoding_sjis;

/* Parameter keys for mconv_define_coding ().  */
extern MSymbol Mtype;
extern MSymbol Mcharsets;
extern MSymbol Mflags;
extern MSymbol Mdesignation;
extern MSymbol Minvocation;
extern MSymbol Mcode_unit;
extern MSymbol Mbom;
extern MSymbol Mlittle_endian;

/* Symbols representing coding system type.  */
extern MSymbol Mutf;
extern MSymbol Miso_2022;

/* Symbols appearing in the value of Mfrag parameter.  */
extern MSymbol Mreset_at_eol;
extern MSymbol Mreset_at_cntl;
extern MSymbol Meight_bit;
extern MSymbol Mlong_form;
extern MSymbol Mdesignation_g0;
extern MSymbol Mdesignation_g1;
extern MSymbol Mdesignation_ctext;
extern MSymbol Mdesignation_ctext_ext;
extern MSymbol Mlocking_shift;
extern MSymbol Msingle_shift;
extern MSymbol Msingle_shift_7;
extern MSymbol Meuc_tw_shift;
extern MSymbol Miso_6429;
extern MSymbol Mrevision_number;
extern MSymbol Mfull_support;

/* etc */
extern MSymbol Mcoding;
extern MSymbol Mmaybe;

/*** @ingroup m17nConv */
/***en
    @brief Codes that represent the result of code conversion.

    One of these values is set in @c MConverter-\>result.   */

/***ja
    @brief コード変換の結果を示すコード.

    これらの値のうち一つが @c MConverter-\>result に設定される。  */

enum MConversionResult
  {
    /***en Code conversion is successful. */
    /***ja コード変換は成功. */
    MCONVERSION_RESULT_SUCCESS,

    /***en On decoding, the source contains an invalid byte. */
    /***ja デコード時、ソースに不正なバイトが含まれている. */
    MCONVERSION_RESULT_INVALID_BYTE,

    /***en On encoding, the source contains a character that cannot be
	encoded by the specified coding system. */
    /***ja エンコード時、指定のコード系でエンコードできない文字がソースに含まれている. */
    MCONVERSION_RESULT_INVALID_CHAR,

    /***en On decoding, the source ends with an incomplete byte sequence. */
    /***ja デコード時、不完全なバイト列でソースが終わっている。*/
    MCONVERSION_RESULT_INSUFFICIENT_SRC,

    /***en On encoding, the destination is too short to store the result. */
    /***ja エンコード時、結果を格納する領域が短かすぎる。 */
    MCONVERSION_RESULT_INSUFFICIENT_DST,

    /***en An I/O error occurred in the conversion.  */
    /***ja コード変換中に I/O エラーが起こった。  */
    MCONVERSION_RESULT_IO_ERROR
  };
/*=*/

/*** @ingroup m17nConv */
/***en
    @brief Structure to be used in code conversion.

    Structure to be used in code conversion.  The first three members
    are to control the conversion.  */

/***ja
    @brief コード変換に用いられる構造体.

    コード変換に用いられる構造体。最初の３つのメンバは変換を制御する。

    @latexonly \IPAlabel{MConverter} @endlatexonly  
*/

typedef struct
{
  /***en
      Set the value to nonzero if the conversion should be lenient.
      By default, the conversion is strict (i.e. not lenient).

      If the conversion is strict, the converter stops at the first
      invalid byte (on decoding) or at the first character not
      supported by the coding system (on encoding).  If this happens,
      @c MConverter-\>result is set to @c
      MCONVERSION_RESULT_INVALID_BYTE or @c
      MCONVERSION_RESULT_INVALID_CHAR accordingly.

      If the conversion is lenient, on decoding, an invalid byte is
      kept per se, and on encoding, an invalid character is replaced
      with "<U+XXXX>" (if the character is a Unicode character) or
      with "<M+XXXXXX>" (otherwise).  */

  /***ja 
      厳密な変換が必要でない場合に値を 0 以外にする。
      デフォルトでは、変換は厳密である。

      変換が厳密とは、デコードの際には最初の不正なバイトでコンバータが止まること、
      エンコードの際には変換されるコード系でサポートされない最初の文字でコンバータが止まることを指す。
      これらの場合、@c MConverter-\>result はそれぞれ @c
      MCONVERSION_RESULT_INVALID_BYTE か@c
      MCONVERSION_RESULT_INVALID_CHAR となる。

      変換が厳密でない場合には、デコードの際の不正なバイトはそのバイトのまま残る。
      またエンコードの際には、不正な文字が Unicode 文字の場合には "<U+XXXX>"
      に、そうでない場合には "<M+XXXXXX>" に置き換えられる。  */

  int lenient;

  /***en
      Set the value to nonzero before decoding or encoding the last
      block of the byte sequence or the character sequence
      respectively.  The value influences the conversion as below.

      On decoding, in the case that the last few bytes are too short
      to form a valid byte sequence:

	If the value is nonzero, the conversion terminates by error
	(MCONVERSION_RESULT_INVALID_BYTE) at the first byte of the
	sequence.

	If the value is zero, the conversion terminates successfully.
	Those bytes are stored in the converter as carryover and are
	prepended to the byte sequence of the further conversion.

      On encoding, in the case that the coding system is context
      dependent:

	If the value is nonzero, the conversion may produce a byte
	sequence at the end to reset the context to the initial state
	even if the source characters are zero.

	If the value is zero, the conversion never produce such a byte
	sequence at the end.  */

/***ja 

     バイト列の終端のブロックをデコードする際、または文字列の終端のブロックをエンコードする際は、値を
     0 以外にする。この値は以下のように変換に影響する。

     デコーディングの際に最後の数バイトが正しいバイトシークエンスを形成するには短すぎる場合：

       値が 0 でなければ、変換はそのシークエンスの最初のバイトにおいて、エラー
       (MCONVERSION_RESULT_INVALID_BYTE) で終る。

       値が 0 ならば、変換は成功して終る。
       問題の数バイトはキャリーオーバとしてコンバータに保持され、変換の続きを行う際に変換するバイト列の前に付けられる。

     エンコーディングの際にコード系が文脈依存の場合、

       値が 0 でなければ、コンテキストを最初に戻すためのバイト列がソースの文字とかかわりなく変換の結果生成されることがある。

       値が 0 ならば、そのようなバイト列は生成されない。

	 */

  int last_block;

  /***en
      If the value is nonzero, it specifies at most how many
      characters to convert.  */
  /***ja
      0 でなければ、変換される最大の文字数を指定する。
      */

  unsigned at_most;

  /***en
      The following three members are to report the result of the
      conversion.  */
  /***ja
      以下の３つのメンバは変換の結果を表すためのものである。  */

  /***en
      Number of characters most recently decoded or encoded. */
  /***ja
      最近にデコード/エンコードされた文字数。 */

  int nchars;

  /***en
      Number of bytes recently decoded or encoded. */

  /***ja
      最近にデコード/エンコードされたバイト数。 */

  int nbytes;

  /***en
      Result code of the conversion. */

  /***ja
      コード変換の結果を示すコード。 */

  enum MConversionResult result;

  /***en
      Various information about the status of code conversion.  The
      contents depend on the type of coding system.  It is assured
      that @c status is aligned so that any type of casting is safe
      and at least 256 bytes of memory space can be used.  */

  /***ja
      コード変換の状況に関する種々の情報。内容はコード系のタイプによって異なる。
      @c status はどのような型へのキャストに対しても安全なようにメモリアラインされており、また最低256バイトのメモリ領域が使えるようになっている。  */

  union {
    void *ptr;
    double dbl;
    char c[256];
  } status;

  /***en
      This member is for internally use only.  An application program
      should never touch it.  */
  /***ja このメンバは内部的に使用され、アプリケーションプログラムは触れてはならない。 */
  void *internal_info;
} MConverter;
/*=*/

/*** @ingroup m17nConv */
/***en 
    @brief Types of coding system.  */
/***ja 
    @brief コード系のタイプ.  */

enum MCodingType
  {
    /***en
	A coding system of this type supports charsets directly.
	The dimension of each charset defines the length of bytes to
	represent a single character of the charset, and a byte
	sequence directly represents the code-point of a character.
	The m17n library provides the default decoding and encoding
	routines of this type.  */

    /***ja
	このタイプのコード系は文字セットを直接サポートする。
	各文字セットの次元とは、その文字セットで一文字を表現するために必要なバイト数であり、バイト列は文字のコードポイントを直接表す。
	m17n ライブラリはこのタイプ用のデフォルトのエンコード／デコードルーティンを提供する。  */

    MCODING_TYPE_CHARSET,

    /***en
	A coding system of this type supports byte sequences of a
	UTF (UTF-8, UTF-16, UTF-32) like structure.
	The m17n library provides the default decoding and encoding
	routines of this type.  */

    /***ja
	このタイプのコード系は、UTF 系 (UTF-8, UTF-16, UTF-32) のバイト列をサポートする。
	m17n ライブラリはこのタイプ用のデフォルトのエンコード／デコードルーティンを提供する。  */

    MCODING_TYPE_UTF,

    /***en
	A coding system of this type supports byte sequences of an
	ISO-2022 like structure.  The details of each structure are
	specified by @c MCodingInfoISO2022 .
	The m17n library provides decoding and encoding routines of
	this type.  */

    /***ja 
        このタイプのコード系は、ISO-2022 系のバイト列をサポートする。
	各コード系の構造の詳細は @c MCodingInfoISO2022 で指定される。
	m17n ライブラリはこのタイプ用のデフォルトのエンコード／デコードルーティンを提供する。  */

    MCODING_TYPE_ISO_2022,

    /***en
	A coding system of this type is for byte sequences of
	miscellaneous structures.
	The m17n library does not provide decoding and encoding
	routines of this type.  They must be provided by the
	application program.  */

    /***ja
	このタイプのコード系は、その他の構造のバイト列のためのものである。
	m17n ライブラリはこのタイプ用のエンコード／デコードルーティンを提供しないので、アプリケーションプログラム側で準備する必要がある。  */

    MCODING_TYPE_MISC
  };
/*=*/

/*** @ingroup m17nConv */
/***en 
    @brief  Bit-masks to specify the detail of coding system whose type is MCODING_TYPE_ISO_2022.  */
/***ja 
    @brief MCODING_TYPE_ISO_2022 タイプのコード系の詳細を表わすビットマスク.  */

enum MCodingFlagISO2022
  {
    /***en
	On encoding, reset the invocation and designation status to
	initial at end of line.  */
    /***ja エンコードの際、行末で呼び出し (invocation) と指示
	(designation) の状態を初期値に戻す。   */
    MCODING_ISO_RESET_AT_EOL =		0x1,

    /***en
	On encoding, reset the invocation and designation status to
	initial before any control codes.  */
    /***ja
	エンコードの際、すべての制御文字の前で、呼び出し
	(invocation) と指示 (designation) の状態を初期値に戻す。        */
    MCODING_ISO_RESET_AT_CNTL =		0x2,

    /***en
	Use the right graphic plane.  */
    /***ja
	図形文字集合の右側を使う。  */
    MCODING_ISO_EIGHT_BIT =		0x4,

    /***en
	Use the non-standard 4 bytes format for designation sequence
	for charsets JISX0208-1978, GB2312, and JISX0208-1983.  */
    /***ja
	JISX0208-1978, GB2312, JISX0208-1983 
	の文字集合に対する指示シークエンスとして、非標準の4バイト形式を用いる。 */

    MCODING_ISO_LONG_FORM =		0x8,

    /***en
	On encoding, unless explicitly specified, designate charsets
	to G0.  */
    /***ja 
        エンコードの際、特に指定されない限り、文字集合を G0 
	に指示する。*/
    MCODING_ISO_DESIGNATION_G0 =		0x10,

    /***en
	On encoding, unless explicitly specified, designate charsets
	except for ASCII to G1.  */
    /***ja
	エンコードの際、特に指定されない限り、ASCII 以外の文字集合を 
	G1 に指示する。*/
    MCODING_ISO_DESIGNATION_G1 =		0x20,

    /***en
	On encoding, unless explicitly specified, designate 94-chars
	charsets to G0, 96-chars charsets to G1.  */
    /***ja
	エンコードの際、特に指定されない限り、94文字集合を G0 
	に、96文字集合を G1 に指示する。*/
    MCODING_ISO_DESIGNATION_CTEXT =	0x40,

    /***en
	On encoding, encode such charsets not conforming to ISO-2022
	by ESC % / ..., and encode non-supported Unicode characters by
	ESC % G ... ESC % @@ .  On decoding, handle those escape
	sequences.  */
    /***ja
	エンコードの際、ISO-2022 に合致しない文字集合を ESC % / ... 
	でエンコードする。サポートされていない Unicode 文字は ESC % G ...
	ESC % @@ でエンコードする。
	デコードの際、これらのエスケープ・シーケンスを解釈する。  */
    MCODING_ISO_DESIGNATION_CTEXT_EXT =	0x80,

    /***en
	Use locking shift.  */
    /***ja
	ロッキングシフトを使う。  */
    MCODING_ISO_LOCKING_SHIFT =	0x100,

    /***en
	Use single shift (SS2 (0x8E or ESC N), SS3 (0x8F or ESC O)).  */
    /***ja
	シングルシフト (SS2 (0x8E or ESC N), SS3 (0x8F or ESC O)) を使う。  */
    MCODING_ISO_SINGLE_SHIFT =	0x200,

    /***en
	Use 7-bit single shift 2 (SS2 (0x19)).  */
    /***ja
	7ビットシングルシフト 2 (SS2 (0x19)) を使う。  */
    MCODING_ISO_SINGLE_SHIFT_7 =	0x400,

    /***en
	Use EUC-TW like special shifting.  */
    /***ja
	EUC-TW 風の特別なシフトを使う。  */
    MCODING_ISO_EUC_TW_SHIFT =	0x800,

    /***en
	Use ISO-6429 escape sequences to indicate direction.
	Not yet implemented.  */
    /***ja
	ISO-6429 のエスケープシークエンスで方向を指示する。未実装。 */
    MCODING_ISO_ISO6429 =		0x1000,

    /***en
	On encoding, if a charset has revision number, produce escape
	sequences to specify the number.  */
    /***ja
	エンコードの際、文字セットに revision number 
	があればそれを表わすエスケープシークエンスを生成する。        */
    MCODING_ISO_REVISION_NUMBER =	0x2000,

    /***en
	Support all ISO-2022 charsets.  */
    /***ja
	ISO-2022 の全文字集合をサポートする。  */
    MCODING_ISO_FULL_SUPPORT =		0x3000,

    MCODING_ISO_FLAG_MAX
  };
/*=*/

/*** @ingroup m17nConv */
/***en
    @brief Structure for a coding system of type #MCODING_TYPE_ISO_2022.

    Structure for extra information about a coding system of type
    MCODING_TYPE_ISO_2022.  */

/***ja 
    @brief #MCODING_TYPE_ISO_2022 タイプのコード系で必要な付加情報用構造体.

    MCODING_TYPE_ISO_2022 タイプのコード系で必要な付加情報用を保持するための構造体。

    @latexonly \IPAlabel{MCodingInfoISO2022} @endlatexonly  */

typedef struct
{
  /***en
      Table of numbers of an ISO2022 code extension element invoked
      to each graphic plane (Graphic Left and Graphic Right).  -1
      means no code extension element is invoked to that plane.  */

  /***ja
      各図形文字領域 (Graphic Left と Graphic Right) に呼び出されている、
      ISO2022 符合拡張要素の番号のテーブル。-1 
      はその領域にどの符合拡張要素も呼び出されていないことを示す。   */

  int initial_invocation[2];

  /***en
      Table of code extension elements.  The Nth element corresponds
      to the Nth charset in $CHARSET_NAMES, which is an argument given
      to the mconv_define_coding () function.

      If an element value is 0..3, it specifies a graphic register
      number to designate the corresponds charset.  In addition, the
      charset is initially designated to that graphic register.

      If the value is -4..-1, it specifies a graphic register number
      0..3 respectively to designate the corresponds charset.
      Initially, the charset is not designated to any graphic
      register.  */

  /***ja
      符合拡張要素のテーブル。N番目の要素は、$CHARSET_NAMES の N 
      番目の文字セットに対応する。$CHARSET_NAMES は関数 
      mconv_define_coding () の引数となる。

      値が 0..3 だったら、対応する文字セットを G0..G3 
      のそれぞれに指示すること、また初期状態ですでに G0..G3
      に指示されていることを意味する。

      値が -4..-1 だったら、対応する文字セットを G0..G3 
      のそれぞれに指示すること、しかし初期状態ではどこにも指示されていないことを意味する。
        */

  char designations[32];

  /***en
      Bitwise OR of @c enum @c MCodingFlagISO2022 .  */

  /***ja
      @c enum @c MCodingFlagISO2022 のビット単位での論理 OR  */

  unsigned flags;

} MCodingInfoISO2022;
/*=*/

/*** @ingroup m17nConv */
/***en
    @brief Structure for extra information about a coding system of type #MCODING_TYPE_UTF.
    */

/***ja
    @brief #MCODING_TYPE_UTF タイプのコード系で必要な付加情報用の構造体.

    @latexonly \IPApage{MCodingInfoUTF} @endlatexonly

    @latexonly \IPAlabel{MCodingInfoUTF} @endlatexonly  */

typedef struct
{
  /***en
      Specify bits of a code unit.  The value must be 8, 16, or 32.  */
  /***ja
      コード長（ビット数）の指定。値は 8, 16, 32 のいずれか。  */
  int code_unit_bits;

  /***en
      Specify how to handle the heading BOM (byte order mark).  The
      value must be 0, 1, or 2.  The meanings are as follows:

      0: On decoding, check the first two byte.  If they are BOM,
      decide endian by them. If not, decide endian by the member @c
      endian.  On encoding, produce byte sequence according to
      @c endian with heading BOM.

      1: On decoding, do not handle the first two bytes as BOM, and
      decide endian by @c endian.  On encoding, produce byte sequence
      according to @c endian without BOM.

      2: On decoding, handle the first two bytes as BOM and decide
      ending by them.  On encoding, produce byte sequence according to
      @c endian with heading BOM.

      If \<code_unit_bits\> is 8, the value has no meaning.  */

  /***ja
      先頭の BOM (バイトオーダーマーク) の取り扱いを指定する。値は 0,
      1, 2 のいずれかであり、それぞれの意味は以下のようになる。

      0: デコードの際に最初の 2 バイトを調べる。もしそれが BOM 
      であれば、エンディアンをそれで判定する。そうでなければ、メンバ @c
      endian に従ってエンディアンを決定する。エンコードの際には @c
      endian に従ったバイト列を先頭に BOM 付で生成する。

      1: デコードの際、最初の 2 バイトを BOM として扱わず、エンディアンは
      @c endian で判定する。エンコードの際には、BOM を出力せず、
      @c endian に応じたバイト列を生成する。

      2: デコードの際に最初の2バイトを BOMとして扱い、それに従ってエンディアンを判定する。
      エンコードの際には @c endian に応じたバイト列を先頭に BOM 付きで生成する。  */
  int bom;

  /***en
      Specify the endian type.  The value must be 0 or 1.  0 means
      little endian, and 1 means big endian.

      If \<code_unit_bits\> is 8, the value has no meaning.  */
  /***ja
      エンディアンのタイプを指定する。値は 0 か 1 であり、0 
      ならばリトルエンディアン、1 ならばビッグエンディアンである。

      \<code_unit_bits\> が 8 の場合には、この値は意味を持たない。
  */
  int endian;
} MCodingInfoUTF;
/*=*/

extern MSymbol mconv_define_coding (const char *name, MPlist *plist,
				    int (*resetter) (MConverter *),
				    int (*decoder) (const unsigned char *, int,
						    MText *, MConverter *),
				    int (*encoder) (MText *, int, int,
						    unsigned char *, int,
						    MConverter *),
				    void *extra_info);

extern MSymbol mconv_resolve_coding (MSymbol symbol);

extern int mconv_list_codings (MSymbol **symbols);

extern MConverter *mconv_buffer_converter (MSymbol coding,
					   const unsigned char *buf,
					   int n);

extern MConverter *mconv_stream_converter (MSymbol coding, FILE *fp);

extern int mconv_reset_converter (MConverter *converter);

extern void mconv_free_converter (MConverter *converter);

extern MConverter *mconv_rebind_buffer (MConverter *converter,
					const unsigned char *buf, int n);

extern MConverter *mconv_rebind_stream (MConverter *converter, FILE *fp);

extern MText *mconv_decode (MConverter *converter, MText *mt);

MText *mconv_decode_buffer (MSymbol name, const unsigned char *buf, int n);

MText *mconv_decode_stream (MSymbol name, FILE *fp);   

extern int mconv_encode (MConverter *converter, MText *mt);

extern int mconv_encode_range (MConverter *converter, MText *mt,
			       int from, int to);

extern int mconv_encode_buffer (MSymbol name, MText *mt,
				unsigned char *buf, int n);

extern int mconv_encode_stream (MSymbol name, MText *mt, FILE *fp);

extern int mconv_getc (MConverter *converter);

extern int mconv_ungetc (MConverter *converter, int c);

extern int mconv_putc (MConverter *converter, int c);

extern MText *mconv_gets (MConverter *converter, MText *mt);

/* (S4) Locale related functions corresponding to libc functions */
/*=*/
/*** @ingroup m17nShell */
/***en @defgroup m17nLocale Locale */
/***ja @defgroup m17nLocale ロケール */
/*=*/

/*** @ingroup m17nLocale */
/***en
    @brief @c struct @c MLocale.

    The structure @c MLocale is used to hold information about name,
    language, territory, modifier, codeset, and the corresponding
    coding system of locales.

    The contents of this structure are implementation dependent.  Its
    internal structure is concealed from application programs.  */

/***ja
    @brief @c MLocale 構造体.

    @c MLocale 構造体は、ロケールの名前、言語、地域、モディファイア、コードセット、および対応するコード系に関する情報を保持するために用いられる。

    この構造体の内容は実装に依存する。 
    内部構造はアプリケーションプログラムからは見えない。  */

/***
    @seealso
    mlocale_get_prop () */

typedef struct MLocale MLocale;

/*=*/

extern MSymbol Miso639_1, Miso639_2;
extern MSymbol Mterritory;
extern MSymbol Mmodifier;
extern MSymbol Mcodeset;

extern MPlist *mlanguage_list (void);

extern MSymbol mlanguage_code (MSymbol language, int len);

extern MPlist *mlanguage_name_list (MSymbol language, MSymbol target,
				    MSymbol script, MSymbol territory);

extern MText *mlanguage_text (MSymbol language);

extern MPlist *mscript_list (void);

extern MPlist *mscript_language_list (MSymbol script);

extern MSymbol mlanguage_name (MSymbol language);

extern MLocale *mlocale_set (int category, const char *locale);

extern MSymbol mlocale_get_prop (MLocale *locale, MSymbol key);

extern int mtext_ftime (MText *mt, const char *format, const struct tm *tm,
			MLocale *locale);

extern MText *mtext_getenv (const char *name);

extern int mtext_putenv (MText *mt);

extern int mtext_coll (MText *mt1, MText *mt2);

/*
 *  (9) Miscellaneous functions of libc level (not yet implemented)
 */

/*
extern int mtext_width (MText *mt, int n);
extern MText *mtext_tolower (MText *mt);
extern MText *mtext_toupper (MText *mt);
*/

/*
 *  (10) Input method
 */
/*=*/
/*** @ingroup m17nShell */
/***en @defgroup m17nInputMethod Input Method (basic) */
/***ja @defgroup m17nInputMethod 入力メソッド (基本部分) */
/*=*/

/*** @addtogroup m17nInputMethod 
     @{ */
/*=*/
/* Struct forward declaration.  */
/***
    @brief See struct MInputMethod */
typedef struct MInputMethod MInputMethod;
/*=*/
/***
    @brief See struct MInputContext */
typedef struct MInputContext MInputContext;
/*=*/

/***en
    @brief Type of input method callback functions.

    This is the type of callback functions called from input method
    drivers.  $IC is a pointer to an input context, $COMMAND is a name
    of callback for which the function is called.   */
/***ja
    @brief 入力メソッドコールバック関数の型宣言.

    入力メソッドから呼ばれるコールバック関数の型である。$IC 
    は入力コンテクストへのポインタ、$COMMAND は関数が呼ばれるコールバックの名前である。  */

typedef void (*MInputCallbackFunc) (MInputContext *ic, MSymbol command);
/*=*/

/***en
    @brief Structure of input method driver.

    The type @c MInputDriver is the structure of an input method driver that
    contains several functions to handle an input method.  */

/***ja
    @brief 入力ドライバ用構造体.

    @c MInputDriver は、入力メソッドを取り扱う関数を含む入力メソッドドライバの構造体の型である。  */

typedef struct MInputDriver
{
  /***en
      @brief Open an input method.

      This function opens the input method $IM.  It is called from the
      function minput_open_im () after all member of $IM but \<info\>
      set.  If opening $IM succeeds, it returns 0.  Otherwise, it
      returns -1.  The function can setup $IM->info to keep various
      information that is referred by the other driver functions.  */

  /***ja
      @brief 入力メソッドをオープンする.

      この関数は、入力メソッド $IM をオープンする。$IM の \<info\> 
      以外の全メンバーがセットされた後で、関数 minput_open_im () 
      から呼ばれる。$IM をオープンできれば 0 を、できなければ -1を返す。
      この関数は $IM->info 
      を設定して、他のドライバ関数から参照される情報を保持することができる。
      */

  int (*open_im) (MInputMethod *im);

  /***en
      @brief Close an input method.

      This function closes the input method $IM.  It is called from
      the function minput_close_im ().  It frees all memory allocated
      for $IM->info (if any) after finishing all the tasks of closing
      the input method.  But, the other members of $IM should not be
      touched.  */

  /***ja
      @brief 入力メソッドをクローズする.

      この関数は、入力メソッド $IM をクローズする。関数 
      minput_close_im () から呼ばれる。
      入力メソッドのクローズがすべて終了した時点で、この関数は$IM->info 
      に割り当てられているメモリを(あれば)すべて開放する。
      ただし、$IM の他のメンバに影響を与えてはならない。
        */

  void (*close_im) (MInputMethod *im);

  /***en
      @brief Create an input context.

      This function creates the input context $IC.  It is called from
      the function minput_create_ic () after all members of $IC but
      \<info\> are set.  If creating $IC succeeds, it returns 0.
      Otherwise, it returns -1.  The function can setup $IC->info to
      keep various information that is referred by the other driver
      functions.  */

  /***ja
      @brief 入力コンテクストを生成する.

      この関数は入力コンテクスト $IC を生成する。
      $IC の \<info\> 以外の全メンバーがセットされた後で、関数
      minput_create_ic () から呼ばれる。
      $IC を生成できれば 0 を、できなければ -1 を返す。
      この関数は $IC->info を設定して、他のドライバ関数から参照される情報を保持することができる。  */


  int (*create_ic) (MInputContext *ic);

  /***en
      @brief Destroy an input context.

      This function is called from the function minput_destroy_ic ()
      and destroys the input context $IC.  It frees all memory
      allocated for $IC->info (if any) after finishing all the tasks
      of destroying the input method.  But, the other members of $IC
      should not be touched.  */

  /***ja
      @brief 入力コンテクストを破壊する.

      関数 minput_destroy_ic () から呼ばれ、入力コンテクスト $IC 
      を破壊する。入力コンテクストの破壊がすべて終了した時点で、$IC->info 
      に割り当てられているメモリを(あれば)すべて開放する。ただし、$IC 
      の他のメンバに影響を与えてはならない。  */

  void (*destroy_ic) (MInputContext *ic);

  /***en
      @brief Filter an input key.

      This function is called from the function minput_filter () and
      filters an input key.  $KEY and $ARG are the same as what given
      to minput_filter ().

      The task of the function is to handle $KEY, update the internal
      state of $IC.  If $KEY is absorbed by the input method and no
      text is produced, it returns 1.  Otherwise, it returns 0.

      It may update $IC->status, $IC->preedit, $IC->cursor_pos,
      $IC->ncandidates, $IC->candidates, and $IC->produced if that is
      necessary for the member \<callback\>.

      The meaning of $ARG depends on the input method river.  See the
      documentation of @c minput_default_driver and @c
      minput_gui_driver for instance.  */

  /***ja
      @brief 入力キーをフィルタする.

      関数 minput_filter () から呼ばれ、入力キーをフィルタする。引数 
      $KEY, $ARG は関数 minput_filter () のものと同じ。

      この関数は $KEY を処理し、$IC の内部状態を更新する。 $KEY 
      が入力メソッドに吸収されてテキストが生成されなかった場合には、 
      1 を返す。そうでなければ 0 を返す。

      メンバ \<callback\> に必要であれば、$IC->status, $IC->preedit,
      $IC->cursor_pos, $IC->ncandidates, $IC->candidates,
      $IC->produced を更新できる。

      $ARG の意味は入力メソッドドライバに依存する。例は @c
      minput_default_driver または @c minput_gui_driver 
      の説明を参照のこと。 */

  int (*filter) (MInputContext *ic, MSymbol key, void *arg);

  /***en
      @brief  Lookup a produced text in an input context.

      It is called from the function minput_lookup () and looks up a
      produced text in the input context $IC.  This function
      concatenate a text produced by the input key $KEY (if any) to
      M-text $MT.  If $KEY was correctly handled by the input method
      of $IC, it returns 0.  Otherwise, it returns 1.

      The meaning of $ARG depends on the input method driver.  See the
      documentation of @c minput_default_driver and @c
      minput_gui_driver for instance.  */

  /***ja
      @brief 入力コンテクストで生成されるテキストの獲得.

      関数 minput_lookup () から呼ばれ、入力コンテクスト $IC 
      で生成されるテキストを検索する。入力キー $KEY 
      によって生成されるテキストがあれば、M-text $MT に追加する。 $KEY
      が入力メソッド $IC によって正しく処理されれば 0 を返す。そうでなければ 1 を返す。

      $ARG の意味は入力メソッドドライバに依存する。例は @c
      minput_default_driver または @c minput_gui_driver の説明を参照の
      こと。 */

  int (*lookup) (MInputContext *ic, MSymbol key, void *arg, MText *mt);

  /***en
      @brief List of callback functions.

      List of callback functions.  Keys are one of
      @b Minput_preedit_start, @b Minput_preedit_draw,
      @b Minput_preedit_done, @b Minput_status_start, @b Minput_status_draw,
      @b Minput_status_done, @b Minput_candidates_start,
      @b Minput_candidates_draw, @b Minput_candidates_done,
      @b Minput_set_spot, @b Minput_toggle, @b Minput_reset,
      @b Minput_get_surrounding_text, @b Minput_delete_surrounding_text.
      Values are functions of type #MInputCallbackFunc.  */
  /***ja
      @brief コールバック関数のリスト.

      コールバック関数のリスト。キーは次のいずれか。
      @b Minput_preedit_start, @b Minput_preedit_draw,
      @b Minput_preedit_done, @b Minput_status_start, @b Minput_status_draw,
      @b Minput_status_done, @b Minput_candidates_start,
      @b Minput_candidates_draw, @b Minput_candidates_done,
      @b Minput_set_spot, @b Minput_toggle, @b Minput_reset,
      @b Minput_get_surrounding_text, @b Minput_delete_surrounding_text。
      値は#MInputCallbackFunc 型の関数。  */
  MPlist *callback_list;

} MInputDriver;
/*=*/
/*** @} */
/*=*/

extern MInputDriver minput_default_driver;

extern MSymbol Minput_method;
extern MSymbol Minput_driver;

extern MInputDriver *minput_driver;

/** Symbols for callback commands.  */
extern MSymbol Minput_preedit_start;
extern MSymbol Minput_preedit_draw;
extern MSymbol Minput_preedit_done;
extern MSymbol Minput_status_start;
extern MSymbol Minput_status_draw;
extern MSymbol Minput_status_done;
extern MSymbol Minput_candidates_start;
extern MSymbol Minput_candidates_draw;
extern MSymbol Minput_candidates_done;
extern MSymbol Minput_set_spot;
extern MSymbol Minput_toggle;
extern MSymbol Minput_reset;
extern MSymbol Minput_get_surrounding_text;
extern MSymbol Minput_delete_surrounding_text;

/** Symbols for special input key event.  */
extern MSymbol Minput_focus_move;
extern MSymbol Minput_focus_in;
extern MSymbol Minput_focus_out;

/** Symbols describing input method command/variable.  */
extern MSymbol Minherited;
extern MSymbol Mcustomized;
extern MSymbol Mconfigured;

/*** @addtogroup m17nInputMethod 
     @{ */
/*=*/
/***en
    @brief Structure of input method.

    The type @c MInputMethod is the structure of input method
    objects.  */
/***ja
    @brief 入力メソッドの構造体.

    @c MInputMethod は、入力メソッドオブジェクト用の構造体の型である。  */

struct MInputMethod
{
  /***en Which language this input method is for.  The value is @c
      Mnil if the input method is foreign.  */
  /***ja どの言語用の入力メソッドか。
      入力メソッドが外部のものである場合の値は @c Mnil 。  */
  MSymbol language;

  /***en Name of the input method.  If the input method is foreign, it
      must has a property of key @c Minput_driver and the value must be a
      pointer to a proper input method driver.  */
  /***ja 入力メソッドの名前。外部メソッドである場合には、@c
      Minput_driver をキーとするプロパティを持ち、その値は適切な入力メソッドドライバへのポインタでなくてはならない。*/
  MSymbol name;

  /***en Input method driver of the input method.  */
  /***ja その入力メソッド用の入力メソッドドライバ。  */
  MInputDriver driver;

  /***en The argument given to minput_open_im (). */
  /***ja minput_open_im () に渡される引数。  */
  void *arg;

  /***en Pointer to extra information that \<driver\>.open_im ()
      setups. */
  /***ja \<driver\>.open_im () が設定する追加情報へのポインタ。 */
  void *info;
};

/*=*/

/***en
    @brief Bit-masks to specify how candidates of input method is changed.  */

/***ja
    @brief 入力メソッドの入力候補がどう変更されたかを示すビットマスク.  */

enum MInputCandidatesChanged
  {
    MINPUT_CANDIDATES_LIST_CHANGED = 1,
    MINPUT_CANDIDATES_INDEX_CHANGED = 2,
    MINPUT_CANDIDATES_SHOW_CHANGED = 4,
    MINPUT_CANDIDATES_CHANGED_MAX
  };


/*=*/

/***en
    @brief Structure of input context.

    The type @c MInputContext is the structure of input context
    objects.  */

/***ja
    @brief 入力コンテクスト用構造体.

    @c MInputContext は、入力コンテクストオブジェクト用の構造体の型である。  */

struct MInputContext
{
  /***en Backward pointer to the input method.  It is set up by the
      function minput_create_ic ().  */
  /***ja 入力メソッドへの逆ポインタ。関数 minput_create_ic () 
      によって設定される。  */ 
  MInputMethod *im;

  /***en M-text produced by the input method.  It is set up by the
      function minput_filter () .  */
  /***ja 入力メソッドによって生成される M-text。関数 minput_filter () 
      によって設定される。  */
  MText *produced;

  /***en Argument given to the function minput_create_ic (). */
  /***ja 関数 minput_create_ic () に渡される引数。 */
  void *arg;

  /***en Flag telling whether the input context is currently active or
      inactive.  The value is set to 1 (active) when the input context
      is created.  It is toggled by the function minput_toggle ().  */
  /***ja 入力コンテクストがアクティブかどうかを示すフラグ。
      入力コンテクストが生成された時点では値は 1 （アクティブ）であり、関数 
      minput_toggle () によってトグルされる。  */
  int active;

  /***en Spot location and size of the input context.  */
  /***ja 入力コンテクストのスポットの位置と大きさ.  */
  struct {
    /***en X and Y coordinate of the spot.  */
    /***ja スポットの X, Y 座標.  */
    int x, y;

    /***en Ascent and descent pixels of the line of the spot.  */
    /***ja スポットのアセントとディセントのピクセル数.  */
    int ascent, descent;

    /***en Font size for preedit text in 1/10 point.  */
    /***ja preedit テキスト用のフォントサイズ (1/10 ポイント単位).  */
    int fontsize;

    /***en M-text at the spot, or NULL.  */
    /***ja スポット上の M-text、または NULL.  */
    MText *mt;

    /***en Character position in \<mt\> at the spot.  */
    /***ja \<mt\> におけるスポットの文字位置.  */
    int pos;
  } spot;

  /***en The usage of the following members depends on the input
      method driver.  The descriptions below are for the driver of an
      internal input method.  They are set by the function
      \<im\>->driver.filter ().  */
  /***ja 以下のメンバの使用法は入力メソッドドライバによって異なる。
      以下の説明は、内部入力メソッド用の入力ドライバに対するものである。
      これらは関数 \<im\>->driver.filter () によって設定される。  */

  /***en Pointer to extra information that \<im\>->driver.create_ic ()
      setups.  It is used to record the internal state of the input
      context.  */
  /***ja \<im\>->driver.create_ic () が設定する追加情報へのポインタ。
      入力コンテクストの内部状態を記録するために用いられる。 */
  void *info;

  /***en M-text describing the current status of the input
      context.  */
  /***ja 入力コンテクストの現在の状態を表す M-text  */
  MText *status;

  /***en The function \<im\>->driver.filter () sets the value to 1 when
      it changes \<status\>.  */
  /***ja 関数 \<im\>->driver.filter () は、\<status\> を変えた際にこの値を 1 
      に設定する。  */
  int status_changed;

  /***en M-text containing the current preedit text.  The function
      \<im\>->driver.filter () sets the value.  */
  /***ja 現在の preedit テキストを含む M-text。関数 
      \<im\>->driver.filter () によって設定される。  */ 
  MText *preedit;

  /***en The function \<im\>->driver.filter () sets the value to 1 when
      it changes \<preedit\>.  */
  /***ja 関数 \<im\>->driver.filter () は、\<preedit\> を変えた際にこの値を 
      1 に設定する。  */
  int preedit_changed;

  /***en Cursor position of \<preedit\>.  */
  /***ja \<preedit\>のカーソル位置  */
  int cursor_pos;

  /***en The function \<im\>->driver.filter () sets the value to 1 when
      it changes \<cursor_pos\>.  */
  /***ja 関数 \<im\>->driver.filter () は、\<cursor_pos\> を変えた際にこの値を 
      1 に設定する。  */
  int cursor_pos_changed;

  /***en Plist of the current candidate groups.  Each element is an
      M-text or a plist.  If an element is an M-text (i.e. the key is Mtext),
      candidates in that group are characters in the M-text.  If it is
      a plist (i.e. the key is Mplist), each element is an M-text, and
      candidates in that group are those M-texts.  */
  /***ja 現在の候補グループの Plist 。各要素は M-text か plist である。
      要素が M-text の場合（キーが Mtext である場合）には、そのグループの候補はその 
      M-text 中の各文字である。 要素が plist の場合（キーが Mplist
      である場合）には、そのリストの各要素は M-text であり、それらがそのグループの候補となる。  */
  MPlist *candidate_list;

  /***en Index number of the currently selected candidate in all the
      candidates. The index of the first candidate is 0.  If the
      number is 8, and the first candidate group contains 7
      candidates, the currently selected candidate is the second element of the
      second candidate group.  */
  /***ja 現在選択されている候補が全候補中で何番目かをを示すインデックス。
      最初の候補のインデックスは 0。最初の候補グループに七つの候補が含まれており、この値が
      8 ならば、現在の候補は二番目の候補グループの二番目の要素ということになる。
      */
  int candidate_index;

  /* @{ */
  /***en Start and the end positions of the preedit text where
       \<candidate_list\> corresponds to.  */
  /***ja preedit テキスト中で、\<candidate_list\>に対応する最初と最後の位置。
       */
  int candidate_from, candidate_to;
  /* @} */

  /***en Flag telling whether the current candidate group must be
      shown or not.  The function \<im\>->driver.filter () sets the
      value to 1 when an input method required to show candidates, and
      sets the value to 0 otherwise.  */
  /***ja 現在の候補グループを表示するかどうかを示すフラグ。
      関数 \<im\>->driver.filter () は、入力メソッドが候補の表示を要求した時この値を
      1 に、それ以外の時 0 に設定する。  */
  int candidate_show;

  /***en The function \<im\>->driver.filter () sets the value to bitwise
      OR of @c enum @c MInputCandidatesChanged when it changed any of
      the above members (\<candidate_XXX\>), and sets the value to 0
      otherwise.  */
  /***ja 関数 \<im\>->driver.filter () は、上記のメンバ \<candidate_XXX\>
      の１つでも変更した際には、この値を @c enum @c
      MInputCandidatesChanged のビット単位での論理 OR に設定する。そう
      でなければ 0 に設定する。 */
  int candidates_changed;

  /***en Plist that can be freely used by \<im\>->driver functions.
      The driver of internal input method uses it to exchange extra
      arguments and result for callback functions.  The function
      \<im\>->driver.create_ic () sets this to an empty plist, and the
      function \<im\>->driver.destroy_ic () frees it by using
      m17n_object_unref ().  */
  /***ja \<im\>->driver の関数群によって自由に使用できる plist。
      内部入力メソッド用ドライバはこれをコールバック関数との引数や返値
      の受渡しに使用する。関数 \<im\>->driver.create_ic () はこの plist
      を空に設定する。関数\<im\>->driver.destroy_ic () は
      m17n_object_unref () を用いてこの plist を解放する。  */
  MPlist *plist;
};

/*=*/
/*** @} */
/*=*/

extern MInputMethod *minput_open_im (MSymbol language, MSymbol name,
				     void *arg);

/*=*/

extern void minput_close_im (MInputMethod *im);

extern MInputContext *minput_create_ic (MInputMethod *im, void *arg);

extern void minput_destroy_ic (MInputContext *ic);

extern int minput_filter (MInputContext *ic, MSymbol key, void *arg);

extern int minput_lookup (MInputContext *ic, MSymbol key, void *arg,
			  MText *mt);
extern void minput_set_spot (MInputContext *ic, int x, int y, int ascent,
			     int descent, int fontsize, MText *mt, int pos);
extern void minput_toggle (MInputContext *ic);

extern void minput_reset_ic (MInputContext *ic);

extern MText *minput_get_description (MSymbol language, MSymbol name);

extern MPlist *minput_get_title_icon (MSymbol language, MSymbol name);

extern MPlist *minput_get_command (MSymbol language, MSymbol name,
				   MSymbol command);
extern int minput_config_command (MSymbol language, MSymbol name,
				  MSymbol command, MPlist *keyseq);
extern MPlist *minput_get_variable (MSymbol language, MSymbol name,
				    MSymbol variable);
extern int minput_config_variable (MSymbol language, MSymbol name,
				   MSymbol variable, MPlist *value);
extern char *minput_config_file (void);

extern int minput_save_config (void);

extern int minput_callback (MInputContext *ic, MSymbol command);

/* obsolete functions */
extern MPlist *minput_get_commands (MSymbol language, MSymbol name);

extern int minput_assign_command_keys (MSymbol language, MSymbol name,
				       MSymbol command, MPlist *keys);
extern MPlist *minput_get_variables (MSymbol language, MSymbol name);

extern int minput_set_variable (MSymbol language, MSymbol name,
				MSymbol variable, void *value);

extern MPlist *minput_list (MSymbol lang);

extern MInputMethod *mdebug_dump_im (MInputMethod *im, int indent);

M17N_END_HEADER

#endif /* _M17N_H_ */

/*
  Local Variables:
  coding: euc-japan
  End:
*/
