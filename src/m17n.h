/* m17n.h -- header file for the SHELL API of the m17n library.
   Copyright (C) 2003, 2004
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
   Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
   02111-1307, USA.  */

#ifndef _M17N_H_
#define _M17N_H_

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#ifndef _M17N_CORE_H_
#include <m17n-core.h>
#endif

#ifdef __cplusplus
extern "C"
{
#endif

extern void m17n_init (void);
#undef M17N_INIT
#define M17N_INIT() m17n_init ()

extern void m17n_fini (void);
#undef M17N_FINI
#define M17N_FINI() m17n_fini ()

/***en @defgroup m17nShell SHELL API */
/***oldja @defgroup m17nShell SHELL API */
/*=*/

/*
 *  (11) Functions related to the m17n database
 */

/*** @ingroup m17nShell */
/***en @defgroup m17nDatabase Database */
/***oldja @defgroup m17nDatabase 言語情報データベース */
/*=*/
/* Directory of an application specific databases.  */
extern char *mdatabase_dir;

/***
    @ingroup m17nDatabase  */ 
/***en
    @brief Type of database.

    The type #MDatabase is for a database object.  Its internal
    structure is concealed from application programs.  */
/***oldja データベースの型宣言 */
typedef struct MDatabase MDatabase;

/*=*/

/* Look for a data.  */
extern MDatabase *mdatabase_find (MSymbol tag1, MSymbol tag2,
				  MSymbol tag3, MSymbol tag4);

extern MPlist *mdatabase_list (MSymbol tag0, MSymbol tag1,
			       MSymbol tag2, MSymbol tag3);

/* Load a data.  */
void *mdatabase_load (MDatabase *mdb);

/* Get tags of a data.  */
extern MSymbol *mdatabase_tag (MDatabase *mdb);

/* Define a data.  */
extern MDatabase *mdatabase_define (MSymbol tag1, MSymbol tag2,
				    MSymbol tag3, MSymbol tag4,
				    void *(*loader) (MSymbol *, void *),
				    void *extra_info);

/*=*/
/* (S2) Charset staffs */

/*** @ingroup m17nShell */
/***en @defgroup m17nCharset Charset */
/***oldja @defgroup m17nCharset 文字セット */
/*=*/
#define MCHAR_INVALID_CODE 0xFFFFFFFF

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
extern MSymbol Mcharset;

extern MSymbol mchar_define_charset (char *name, MPlist *plist);

extern MSymbol mchar_resolve_charset (MSymbol symbol);

extern int mchar_list_charset (MSymbol **symbols);

extern int mchar_decode (MSymbol charset_name, unsigned code);

extern unsigned mchar_encode (MSymbol charset_name, int c);

extern int mchar_map_charset (MSymbol charset_name,
			      void (*func) (int from, int to, void *arg),
			      void *func_arg);

/*=*/

/* (S3) code conversion */

/*** @ingroup m17nShell */
/***en @defgroup m17nConv Code Conversion */
/***oldja @defgroup m17nConv コード変換 */
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

/***oldja
    @brief コード変換の結果を示すコード

    これらの値のうち一つが @c MConverter-\>result に設定される。  */

enum MConversionResult
  {
    /***en Code conversion is successful. */
    /***oldja コード変換は成功。 */
    MCONVERSION_RESULT_SUCCESS,

    /***en On decoding, the source contains an invalid byte. */
    /***oldja デコードの際、ソースに不正なバイトが含まれる。 */
    MCONVERSION_RESULT_INVALID_BYTE,

    /***en On encoding, the source contains a character that cannot be
	encoded by the specified coding system. */

    /***oldja エンコードの際、ソースに指定のコード系で
	エンコードできない文字が含まれる。 */
    MCONVERSION_RESULT_INVALID_CHAR,

    /***en On decoding, the source ends with an incomplete byte sequence. */
    /***oldja デコードの際、ソースが不完全なバイト列で終わる。*/
    MCONVERSION_RESULT_INSUFFICIENT_SRC,

    /***en On encoding, the destination is too short to store the result. */
    /***oldja エンコードの際、結果を格納する領域が短かすぎる。 */
    MCONVERSION_RESULT_INSUFFICIENT_DST,

    /***en An I/O error occurred in the conversion.  */
    /***oldja コード変換中に I/O エラーが起こった。  */
    MCONVERSION_RESULT_IO_ERROR
  };
/*=*/

/*** @ingroup m17nConv */
/***en
    @brief Structure to be used in code conversion.

    The first three members are to control the conversion.  */

/***oldja
    @brief コード変換に用いられる構造体 

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

  /***oldja
      厳密な変換が必要でない場合にこのフラグビットをたてる。デフォ
      ルトでは、変換は厳密である。

      変換が厳密とは、デコードの際には最初の不正なバイトでコンバータ
      が止まること、エンコードの際には変換されるコード系でサポートさ
      れない最初の文字でコンバータが止まることを指す。これらの場合、
      @c MConverter-\>result はそれぞれ@c
      MCONVERSION_RESULT_INVALID_BYTE か@c
      MCONVERSION_RESULT_INVALID_CHAR となる。

      変換が厳密でない場合には、デコードの際の不正なバイトはそのバイ
      トのまま残る。またエンコードの際には、不正な文字はコード系ごと
      に定められたデフォルトの文字と置き換えられる。  */

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

    /***oldja
	文字コード列の終端部分をエンコードする際には、このフラグを立て
	る。この場合出力コードポイント列のコンテクストを元に戻すための
	数バイトが付加的に生成されることがある。

	このフラグはデフォルトでは立っておらず、コンバータは続けて他の
	文字をエンコードするものと仮定している。

	このフラグはデコードには関係しない。  */

  int last_block;

  /***en
      If the value is nonzero, it specifies at most how many
      characters to convert.  */

  unsigned at_most;

  /***en
      The following three members are to report the result of the
      conversion.  */

  /***en
      Number of characters most recently decoded or encoded. */

  /***oldja
      最後にデコード/エンコードされた文字数 */

  int nchars;

  /***en
      Number of bytes recently decoded or encoded. */

  /***oldja
      最後にデコード/エンコードされたバイト数 */

  int nbytes;

  /***en
      Result code of the conversion. */

  /***oldja
      コード変換の結果を示すコード */

  enum MConversionResult result;

  /***en
      Various information about the status of code conversion.  The
      contents depend on the type of coding system.  It is assured
      that @c status is aligned so that any type of casting is safe
      and at least 256 bytes of memory space can be used.  */

  /***oldja
      コード変換の状況に関する情報。内容はコード系のタイプによって異な
      る。@c status はどのような型へのキャストに対しても安全なようにメ
      モリアラインされており、また最低256バイトのメモリ領域が使えるよ
      うになっている。  */

  union {
    void *ptr;
    double dbl;
    char c[256];
  } status;

  /***en
      This member is for internally use only.  An application program
      should never touch it.  */
  void *internal_info;
} MConverter;
/*=*/

/*** @ingroup m17nConv */
/***en @brief Types of coding system  */
/***oldja @brief コード系のタイプ  */

enum MCodingType
  {
    /***en
	A coding system of this type supports charsets directly.
	The dimension of each charset defines the length of bytes to
	represent a single character of the charset, and a byte
	sequence directly represents the code-point of a character.

	The m17n library provides the default decoding and encoding
	routines of this type.  */

    /***oldja
	このタイプのコード系は文字セットを直接サポートする。各文字セッ
	トの次元とは、その文字セットで一文字を表現するために必要なバイ
	ト数であり、バイト列は文字のコードポイントを直接表わす。

	m17n ライブラリはこのタイプ用のデフォルトのエンコード／デコー
	ドルーティンを提供する。  */

    MCODING_TYPE_CHARSET,

    /***en
	A coding system of this type supports byte sequences of a
	UTF (UTF-8, UTF-16, UTF-32) like structure.

	The m17n library provides the default decoding and encoding
	routines of this type.  */

    /***oldja
	このタイプのコード系は、UTF 系 (UTF-8, UTF-16, UTF-32) のバイ
	ト列をサポートする。

	m17n ライブラリはこのタイプ用のデフォルトのエンコード／デコー
	ドルーティンを提供する。  */

    MCODING_TYPE_UTF,

    /***en
	A coding system of this type supports byte sequences of an
	ISO-2022 like structure.  The details of each structure are
	specified by @c MCodingInfoISO2022 .

	The m17n library provides decoding and encoding routines of
	this type.  */

    /***oldja
	このタイプのコード系は、ISO-2022 系のバイト列をサポートする。
	これらのコード系の構造の詳細は @c MCodingInfoISO2022 で指定さ
	れる。

	m17n ライブラリはこのタイプ用のデフォルトのエンコード／デコー
	ドルーティンを提供する。  */

    MCODING_TYPE_ISO_2022,

    /***en
	A coding system of this type is for byte sequences of
	miscellaneous structures.

	The m17n library does not provide decoding and encoding
	routines of this type.  They must be provided by the
	application program.  */

    /***oldja
	このタイプのコード系は、その他の構造のバイト列のためのものであ
	る。

	m17n ライブラリはこのタイプ用のエンコード／デコードルーティン
        を提供しないので、アプリケーションプログラム側でそれらを準備す
        る必要がある。  */

    MCODING_TYPE_MISC
  };
/*=*/

/*** @ingroup m17nConv */
/***en @brief Bit-masks to specify the detail of coding system whose type is
    MCODING_TYPE_ISO_2022.  */

/***oldja @brief MCODING_TYPE_ISO_2022 タイプのコード系の詳細を表わすビットマス
    ク  */

enum MCodingFlagISO2022
  {
    /***en
	On encoding, reset the invocation and designation status to
	initial at end of line.  */
    /***oldja エンコードの際、行末で呼び出し (invocation) と指示
	(designation) の状態を初期値に戻す。   */
    MCODING_ISO_RESET_AT_EOL =		0x1,

    /***en
	On encoding, reset the invocation and designation status to
	initial before any control codes.  */
    /***oldja
	エンコードの際、すべての制御文字の前で、呼び出し
	(invocation) と指示 (designation) の状態を初期値に戻す。        */
    MCODING_ISO_RESET_AT_CNTL =		0x2,

    /***en
	Use the right graphic plane.  */
    /***oldja
	図形文字集合の右側を使う。  */
    MCODING_ISO_EIGHT_BIT =		0x4,

    /***en
	Use the non-standard 4 bytes format for designation sequence
	for charsets JISX0208.1978, GB2312, and JISX0208.1983.  */
    /***oldja
	JISX0208.1978, GB2312, JISX0208.1983 の文字集合に対する指示シー
	クエンスとして、非標準の4バイト形式を用いる。 */

    MCODING_ISO_LONG_FORM =		0x8,

    /***en
	On encoding, unless explicitly specified, designate charsets
	to G0.  */
    /***oldja
	エンコードの際、特に指定されない限り、文字集合を G0 に
	指示する。*/
    MCODING_ISO_DESIGNATION_G0 =		0x10,

    /***en
	On encoding, unless explicitly specified, designate charsets
	except for ASCII to G1.  */
    /***oldja
	エンコードの際、特に指定されない限り、ASCII 以外の文字集合を G1 
	に指示する。*/
    MCODING_ISO_DESIGNATION_G1 =		0x20,

    /***en
	On encoding, unless explicitly specified, designate 94-chars
	charsets to G0, 96-chars charsets to G1.  */
    /***oldja
	エンコードの際、特に指定されない限り、94文字集合を G0 
	に、96文字集合を G1 に指示する。*/
    MCODING_ISO_DESIGNATION_CTEXT =	0x40,

    /***en
	On encoding, encode such charsets not conforming to ISO-2022
	by ESC % / ..., and encode non-supported Unicode characters by
	ESC % G ... ESC % @@ .  On decoding, handle those escape
	sequences.  */
    /***oldja
	エンコードの際、ISO-2022 に合致しない文字集合を ESC % / ... でエ
	ンコードする。サポートされていない Unicode 文字は ESC % G ...
	ESC % @@ でエンコードする。
	デコードの際、これらのエスケープ・シーケンスを解釈する。  */
    MCODING_ISO_DESIGNATION_CTEXT_EXT =	0x80,

    /***en
	Use locking shift.  */
    /***oldja
	ロッキングシフトを使う。  */
    MCODING_ISO_LOCKING_SHIFT =	0x100,

    /***en
	Use single shift (SS2 (0x8E or ESC N), SS3 (0x8F or ESC O)).  */
    /***oldja
	シングルシフト (SS2 or ESC N) を使う。  */
    MCODING_ISO_SINGLE_SHIFT =	0x200,

    /***en
	Use 7-bit single shift 2 (SS2 (0x19)).  */
    /***oldja
	シングルシフト (0x19) を使う。  */
    MCODING_ISO_SINGLE_SHIFT_7 =	0x400,

    /***en
	Use EUC-TW like special shifting.  */
    /***oldja
	EUC-TW 風の特別なシフトを使う。  */
    MCODING_ISO_EUC_TW_SHIFT =	0x800,

    /***en
	Use ISO-6429 escape sequences to indicate direction.
	Not yet implemented.  */
    /***oldja
	ISO-6429 のエスケープシークエンスで方向を指示する。  */
    MCODING_ISO_ISO6429 =		0x1000,

    /***en
	On encoding, if a charset has revision number, produce escape
	sequences to specify the number.  */
    /***oldja
	エンコードの際、文字セットに revision number があればそ
	れを表わすエスケープシークエンスを生成する。        */
    MCODING_ISO_REVISION_NUMBER =	0x2000,

    /***en
	Support all ISO-2022 charsets.  */
    /***oldja
	ISO-2022 の全文字集合をサポートする  */
    MCODING_ISO_FULL_SUPPORT =		0x3000,

    MCODING_ISO_FLAG_MAX
  };
/*=*/

/*** @ingroup m17nConv */
/***en
    @brief Structure for a coding system of type MCODING_TYPE_ISO_2022.

    Structure for extra information about a coding system of type
    MCODING_TYPE_ISO_2022.  */

/***oldja
    @brief MCODING_TYPE_ISO_2022 タイプのコード系で必要
    な付加情報用構造体

    @latexonly \IPAlabel{MCodingInfoISO2022} @endlatexonly  */

typedef struct
{
  /***en
      Table of numbers of an ISO2022 code extension element invoked
      to each graphic plane (Graphic Left and Graphic Right).  -1
      means no code extension element is invoked to that plane.  */

  /***oldja
      各図形文字領域 (Graphic Left と Graphic Right) に呼び出されてい
      る、ISO2022 符合拡張要素の番号のテーブル。-1 はその領域にどの符
      合拡張要素も呼び出されていないことを示す。   */

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

  /***oldja

  符合拡張要素のテーブル。N番目の要素は、$CHARSET_NAMES の N 番目
  の文字セットに対応する。$CHARSET_NAMES は関数 
  mconv_define_coding () の引数として使われる。

  値が 0..3 だったら、対応する文字セットを G0..G3 のそれぞれに指示
  することを意味する。さらに、初期状態ですでに G0..G3 に指示されて
  いる。

  値が -4..-1 だったら、対応する文字セットを G0..G3 のそれぞれに指
  示するが、初期状態ではどこにも指示されていないことを意味する。 */

  char designations[32];

  /***en
      Bitwise OR of @c enum @c MCodingFlagISO2022 .  */

  /***oldja
      @c enum @c MCodingFlagISO2022 のビット単位での論理 OR  */

  unsigned flags;

} MCodingInfoISO2022;
/*=*/

/*** @ingroup m17nConv */
/***en
    @brief Structure for extra information about a coding system of
    type #MCODING_TYPE_UTF.  */

/***oldja
    @brief MCODING_TYPE_UTF タイプのコード系で必要な付加情報用の構造体

    @latexonly \IPApage{MCodingInfoUTF} @endlatexonly

    @latexonly \IPAlabel{MCodingInfoUTF} @endlatexonly  */

typedef struct
{
  /***en
      Specify bits of a code unit.  The value must be 8, 16, or 32.  */
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

      If <code_unit_bits> is 8, the value has no meaning.  */

  /***oldja
      先頭の BOM (バイトオーダーマーク) の取り扱いを指定する。値は 0,
      1, 2 のいずれかであり、それぞれの意味は以下のようになる。

      0: デコードの際に最初の2バイトを調べる。もしそれが BOM であれば、
      エンディアンをそれで判定する。もし最初の2バイトが BOM でなければ、
      メンバ @c endian に従ってエンディアンを決定する。エンコードの際
      には @c endian に従ったバイト列を BOM 付で生成する。

      1: デコードの際、最初の2バイトを BOM として扱わない。エン
      ディアンは @c endian で判定する。エンコードの際には、BOM 
      を出力せず、@c endian に応じたバイト列を生成する。

      2: デコードの際に最初の2バイトを BOMとして扱い、それに従っ
      てエンディアンを判定する。エンコードの際には @c endian に
      応じたバイト列を BOM 付きで生成する。  */
  int bom;

  /***en
      Specify the endian type.  The value must be 0 or 1.  0 means
      little endian, and 1 means big endian.

      If <code_unit_bits> is 8, the value has no meaning.  */
  /***oldja
      エンディアンのタイプを指定する。値は 0 か 1 であり、0 ならばリト
      ルエンディアン、1 ならばビッグエンディアンである。*/
  int endian;
} MCodingInfoUTF;
/*=*/

extern MSymbol mconv_define_coding (char *name, MPlist *plist,
				    int (*resetter) (MConverter *),
				    int (*decoder) (unsigned char *, int,
						    MText *, MConverter *),
				    int (*encoder) (MText *, int, int,
						    unsigned char *, int,
						    MConverter *),
				    void *extra_info);

extern MSymbol mconv_resolve_coding (MSymbol symbol);

extern int mconv_list_codings (MSymbol **symbols);

extern MConverter *mconv_buffer_converter (MSymbol coding, unsigned char *buf,
					   int n);

extern MConverter *mconv_stream_converter (MSymbol coding, FILE *fp);

extern int mconv_reset_converter (MConverter *converter);

extern void mconv_free_converter (MConverter *converter);

extern MConverter *mconv_rebind_buffer (MConverter *converter,
					unsigned char *buf, int n);

extern MConverter *mconv_rebind_stream (MConverter *converter, FILE *fp);

extern MText *mconv_decode (MConverter *converter, MText *mt);

MText *mconv_decode_buffer (MSymbol name, unsigned char *buf, int n);

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

/*** @ingroup m17nShell */
/***en @defgroup m17nLocale Locale */
/***oldja @defgroup m17nLocale ロケール */
/*=*/

/***en
    @brief @c struct @c MLocale

    The structure @c MLocale is used to hold information about name,
    language, territory, modifier, codeset, and the corresponding
    coding system of locales.

    The contents of this structure are implementation dependent.  Its
    internal structure is concealed from application programs.  */

/***oldja
    @brief @c MLocale 構造体

    @c MLocale 構造体は、ロケールの名前、言語、地域、モディファイア、
    コードセット、および対応するコード系に関する情報を保持するために用
    いられる。

    この構造体の内容は実装に依存する。 内部構造はアプリケーションプロ
    グラムからは見えない。  */

/***
    @seealso
    mlocale_get_prop () */

typedef struct MLocale MLocale;

/*=*/

extern MSymbol Mlanguage;
extern MSymbol Mterritory;
extern MSymbol Mmodifier;
extern MSymbol Mcodeset;

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

/*** @ingroup m17nShell */
/***en @defgroup m17nInputMethod Input Method (basic) */
/***oldja @defgroup m17nInputMethod 入力メソッド (基本) */
/*=*/

/* Struct forward declaration.  */
typedef struct MInputMethod MInputMethod;
typedef struct MInputContext MInputContext;

/*** @ingroup m17nInputMethod */

/***en
    @brief Type of input method callback functions.

    This is the type of callback functions called from input method
    drivers.  #IC is a pointer to an input context, #COMMAND is a name
    of callback for which the function is called.   */

typedef void (*MInputCallbackFunc) (MInputContext *ic, MSymbol command);
/*=*/

/***en
    @brief Structure of input method driver.

    The type @c MInputDriver is the structure of an input driver that
    contains several functions to handle an input method.  */

/***oldja
    @brief 入力ドライバ

    @c MInputDriver 型は、入力メソッドを取り扱う関数を含む入力ドライバ
    の構造体である。  */

typedef struct MInputDriver
{
  /***en
      @brief Open an input method.

      This function opens the input method $IC.  It is called from the
      function minput_open_im () after all member of $IM but <info>
      set.  If opening $IM succeeds, it returns 0.  Otherwise, it
      returns -1.  The function can setup $IM->info to keep various
      information that is referred by the other driver functions.  */

  /***oldja
      @brief 入力メソッドをオープンする

      この関数は、入力メソッド$IMをオープンする。$IM の<info>以外の全
      メンバーがセットされた後で、関数 minput_open_im () から呼ばれる。
      $IM をオープンできれば 0 を、できなければ -1を返す。この関数は 
      $IM->info を設定して、他のドライバ関数から参照される情報を保持す
      ることができる。
      */

  int (*open_im) (MInputMethod *im);

  /***en
      @brief Close an input method.

      This function closes the input method $IM.  It is called from
      the function minput_close_im ().  It frees all memory allocated
      for $IM->info (if any) after finishing all the tasks of closing
      the input method.  But, the other members of $IM should not be
      touched.  */

  /***oldja
      @brief 入力メソッドをクローズする

      この関数は関数 minput_close_im () から呼ばれ、入力メソッドをクロー
      ズする。。入力メソッドのクローズがすべて終了した時点で、
      $IM->info に割り当てられているメモリを(あれば)開放する。ただし、
      $IM の他のメンバに影響を与えてはならない。       */

  void (*close_im) (MInputMethod *im);

  /***en
      @brief Create an input context.

      This function creates the input context $IC.  It is called from
      the function minput_create_ic () after all members of $IC but
      <info> are set.  If creating $IC succeeds, it returns 0.
      Otherwise, it returns -1.  The function can setup $IC->info to
      keep various information that is referred by the other driver
      functions.  */

  /***oldja
      @brief 入力コンテクストを生成する

      入力コンテクスト $IC を生成する関数。この関数は、$IC の <info> 
      以外の全メンバーがセットされた後で、関数 minput_create_ic () か
      ら呼ばれる。$IC を生成できれば 0 を、できなければ -1 を返す。こ
      の関数は $IC->info を設定して、他のドライバ関数から参照される情
      報を保持することができる。  */


  int (*create_ic) (MInputContext *ic);

  /***en
      @brief Destroy an input context.

      This function is called from the function minput_destroy_ic ()
      and destroys the input context $IC.  It frees all memory
      allocated for $IC->info (if any) after finishing all the tasks
      of destroying the input method.  But, the other members of $IC
      should not be touched.  */

  /***oldja
      @brief 入力コンテクストを破壊する

      関数 minput_destroy_ic () から呼ばれ、入力コンテクスト $IC を破
      壊する。入力コンテクストの破壊がすべて終了した時点で、$IC->info 
      に割り当てられているメモリを(あれば)開放する。ただし、$IC の他の
      メンバに影響を与えてはならない。       */

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
      necessary for the member <callback>.

      The meaning of $ARG depends on the input driver.  See the
      documentation of @c minput_default_driver and @c
      minput_gui_driver for instance.  */

  /***oldja
      @brief 入力イベントあるいは入力キーをフィルタする

      関数 minput_filter () から呼ばれ、入力キーをフィルタする。引数 
      $KEY, $ARG は関数 minput_filter () のものと同じ。

      この関数は $KEY を処理し、$IC の内部状態をアップデートする。 
      $KEY が入力メソッドに吸収されてテキストが生成されなかった場合に
      は、 1 を返す。そうでなければ 0 を返す。

      メンバ <callback> に必要であれば、$IC->status, $IC->preedit,
      $IC->cursor_pos, $IC->ncandidates, $IC->candidates,
      $IC->produced をアップデートすることができる。

      $ARG の意味は入力ドライバに依存する。例は @c
      minput_default_driver または @c minput_gui_driver のドキュメント
      を参照のこと。 */

  int (*filter) (MInputContext *ic, MSymbol key, void *arg);

  /***en
      @brief  Lookup a produced text in an input context

      It is called from the function minput_lookup () and looks up a
      produced text in the input context $IC.  This function
      concatenate a text produced by the input key $KEY (if any) to
      M-text $MT.  If $KEY was correctly handled by the input method
      of $IC, it returns 0.  Otherwise, it returns 1.

      The meaning of $ARG depends on the input driver.  See the
      documentation of @c minput_default_driver and @c
      minput_gui_driver for instance.  */

  /***oldja
      @brief 入力コンテクストで生成されるテキストの獲得

      関数 minput_lookup () から呼ばれ、入力コンテクスト $IC で生成さ
      れるテキストを検索する。入力キー $KEY によって生成されるテキスト
      があれば、$IC->produced に設定する。 $KEY が入力メソッド $IC に
      よって正しく処理されれば 0 を返す。そうでなければ 1 を返す。

      $ARG の意味は入力ドライバに依存する。例は @c
      minput_default_driver または @c minput_gui_driver のドキュメント
      を参照のこと。 */

  int (*lookup) (MInputContext *ic, MSymbol key, void *arg, MText *mt);

  /***en
      @brief List of callback functions.

      List of callback functions.  Keys are one of
      #Minput_preedit_start, #Minput_preedit_draw,
      #Minput_preedit_done, #Minput_status_start, #Minput_status_draw,
      #Minput_status_done, #Minput_candidates_start,
      #Minput_candidates_draw, #Minput_candidates_done,
      #Minput_set_spot, and #Minput_toggle.  Values are functions of
      type #MInputCallbackFunc.  */
  MPlist *callback_list;

} MInputDriver;
/*=*/

extern MInputDriver minput_default_driver;

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

/***en
    @brief Structure of input method.

    The type @c MInputMethod is the structure of input method
    objects.  */
/***oldja
    @brief 入力メソッド用構造体

    @c MInputMethod 型は、入力メソッドオブジェクト用の構造体である。  */

struct MInputMethod
{
  /***en Which language this input method is for.  The value is @c
      Mnil if the input method is foreign.  */
  /***oldja どの言語用の入力メソッドか。入力メソッドが外部のものである場
      合には値として @c を持つ。  */
  MSymbol language;

  /***en Name of the input method.  If the input method is foreign, it
      must has a property of key @c Minput_driver and the value must be a
      pointer to a proper input driver.  */
  /***oldja 入力メソッドの名前。外部メソッドである場合には、@c
      Minput_driver をキーとするプロパティを持ち、その値は適切な入力ド
      ライバへのポインタでなくてはならない。*/
  MSymbol name;

  /***en Input driver of the input method.  */
  /***oldja その入力メソッド用の入力ドライバ  */
  MInputDriver driver;

  /***en The argument given to minput_open_im (). */
  /***oldja minput_open_im () に与えられる引数  */
  void *arg;

  /***en Pointer to extra information that <driver>.open_im ()
      setups. */
  /***oldja <driver>.open_im () が設定する追加情報へのポインタ */
  void *info;
};

/*=*/

/***en
    @brief Structure of input context.

    The type @c MInputContext is the structure of input context
    objects.  */

/***oldja
    @brief 入力コンテクスト用構造体

    @c MInputContext 型は、入力コンテクストオブジェクト用の構造体であ
    る。  */

struct MInputContext
{
  /***en Backward pointer to the input method.  It is set up be the
      function minput_create_ic ().  */
  /***oldja 入力メソッドへの逆ポインタ。関数 minput_create_ic () によって
      設定される。  */ 
  MInputMethod *im;

  /***en M-text produced by the input method.  It is set up by the
      function minput_lookup () .  */
  /***oldja 入力メソッドによって生成される M-text。関数 minput_lookup () 
      によって設定される。  */
  MText *produced;

  /***en Argument given to the function minput_create_im (). */
  /***oldja 関数 minput_create_ic () に渡される引数 */
  void *arg;

  /***en Flag telling whether the input context is currently active or
      inactive.  The value is set to 1 (active) when the input context
      is created.  It can be toggled by the function minput_toggle
      ().  */
  /***oldja 入力コンテクストがアクティブかどうかを示すフラグ。入力コンテ
      クストが生成された時点では値は 1 （アクティブ）であり、関数 
      minput_toggle () によってトグルできる。  */
  int active;


  /***en Spot location and size of the input context.  */
  /***oldja 入力コンテクストのスポットの位置と大きさ  */
  struct {
    /***en X and Y coordinate of the spot.  */
    /***oldja スポットの X, Y 座標  */
    int x, y;
    /***en Ascent and descent pixels of the line of the spot.  */
    /***oldja スポットのアセントとディセントのピクセル数  */
    int ascent, descent;

    /***en Font size for preedit text in 1/10 point.  */
    int fontsize;

    /***en M-text at the spot, or NULL.  */
    MText *mt;
    /***en Character position in <mt> at the spot.  */
    int pos;
  } spot;

  /***en The usage of the following members depends on the input
      driver.  The descriptions below are for the input driver of an
      internal input method.  They are set by the function
      <im>->driver.filter ().  */
  /***oldja 以下のメンバの使用法は入力ドライバによって異なる。以下の説明
      は、内部入力メソッド用の入力ドライバに対するものである。 */

  /***en Pointer to extra information that <im>->driver.create_ic ()
      setups.  It is used to record the internal state of the input
      context.  */
  /***oldja <im>->driver.create_ic () が設定する追加情報へのポインタ。入
      力コンテクストの内部状態を記録するために用いられる。 */
  void *info;

  /***en M-text describing the current status of the input
      context.  */
  /***oldja 入力コンテクストの現在の状況を表す M-text  */
  MText *status;

  /***en The function <im>->driver.filter () sets the value to 1 when
      it changes <status>.  */
  int status_changed;

  /***en M-text containing the current preedit text.  The function
      <im>->driver.filter () sets the value.  */
  /***oldja 現在の preedit テキストを含む M-text */
  MText *preedit;

  /***en The function <im>->driver.filter () sets the value to 1 when
      it changes <preedit>.  */
  int preedit_changed;

  /***en Cursor position of <preedit>.  */
  /***oldja <preedit>のカーソル位置  */
  int cursor_pos;

  int cursor_pos_changed;

  /***en Array of the current candidate texts.  */
  /***oldja 現在のテキスト候補のグループのリスト  */
  MPlist *candidate_list;
  int candidate_index;
  int candidate_from, candidate_to;
  int candidate_show;

  /***en The function <im>->driver.filter () sets the value to 1 when
      it changes one of the above members.  */
  int candidates_changed;

  MPlist *plist;
};

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

extern void minput_set_spot (MInputContext *ic, int x, int y,
			     int ascent, int descent, int fontsize,
			     MText *mt, int pos);

extern void minput_toggle (MInputContext *ic);

extern MSymbol minput_char_to_key (int c);

/*=*/

extern MInputMethod *mdebug_dump_im (MInputMethod *im, int indent);

#ifdef __cplusplus
}
#endif

#endif /* _M17N_H_ */

/*
  Local Variables:
  coding: euc-japan
  End:
*/
