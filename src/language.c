/* language.c -- language module.
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

#include <config.h>
#include <stdlib.h>
#include "m17n.h"
#include "m17n-misc.h"
#include "internal.h"
#include "language.h"
#include "symbol.h"


/* Internal API */

int
mlang__init ()
{
  /* ISO 639 */
  struct {
    char *name, *fullname;
  } lang_rec[] =
      { {"ab", "Abkhazian"},
	{"aa", "Afar"},
	{"af", "Afrikaans"},
	{"sq", "Albanian"},
	{"am", "Amharic"},
	{"ar", "Arabic"},
	{"hy", "Armenian"},
	{"as", "Assamese"},
	{"ay", "Aymara"},
	{"az", "Azerbaijani"},
	{"ba", "Bashkir"},
	{"eu", "Basque"},
	{"bn", "Bengali"},		/* Bangla */
	{"dz", "Bhutani"},
	{"bh", "Bihari"},
	{"bi", "Bislama"},
	{"br", "Breton"},
	{"bg", "Bulgarian"},
	{"my", "Burmese"},
	{"be", "Byelorussian"},	/* Belarusian */
	{"km", "Cambodian"},	/* Khmer */
	{"ca", "Catalan"},
#if 0
	{"??", "Cherokee"},
	{"??", "Chewa"},
#endif
	{"zh", "Chinese"},
	{"co", "Corsican"},
	{"hr", "Croatian"},
	{"cs", "Czech"},
	{"da", "Danish"},
	{"dv", "Dhivehi"},
	{"nl", "Dutch"},
#if 0
	{"??", "Edo"},
#endif
	{"en", "English"},
	{"eo", "Esperanto"},
	{"et", "Estonian"},
	{"fo", "Faeroese"},
	{"fa", "Farsi"},
	{"fj", "Fiji"},
	{"fi", "Finnish"},
#if 0
	{"??", "Flemish"},
#endif
	{"fr", "French"},
	{"fy", "Frisian"},
#if 0
	{"??", "Fulfulde"},
#endif
	{"gl", "Galician"},
	{"gd", "Gaelic(Scottish)"},		/* Scottish */
	{"gv", "Gaelic(Manx)"},		/* Manx */
	{"ka", "Georgian"},
	{"de", "German"},
	{"el", "Greek"},
	{"kl", "Greenlandic"},
	{"gn", "Guarani"},
	{"gu", "Gujarati"},
	{"ha", "Hausa"},
#if 0
	{"??", "Hawaiian"},
	{"iw", "Hebrew"},
#endif
	{"he", "Hebrew"},
	{"hi", "Hindi"},
	{"hu", "Hungarian"},
#if 0
	{"??", "Ibibio"},
#endif
	{"is", "Icelandic"},
#if 0
	{"??", "Igbo"},
	{"in", "Indonesian"},
#endif
	{"id", "Indonesian"},
#if 0
	{"ia", "Interlingua"},
	{"ie", "Interlingue"},
#endif
	{"iu", "Inuktitut"},
	{"ik", "Inupiak"},
	{"ga", "Irish"},
	{"it", "Italian"},
	{"ja", "Japanese"},
	{"jw", "Javanese"},
	{"kn", "Kannada"},
#if 0
	{"??", "Kanuri"},
#endif
	{"ks", "Kashmiri"},
	{"kk", "Kazakh"},
	{"rw", "Kinyarwanda"},	/* Ruanda */
	{"ky", "Kirghiz"},
	{"rn", "Kirundi"},		/* Rundi */
	{"ko", "Korean"},
	{"ku", "Kurdish"},
	{"lo", "Laothian"},
	{"la", "Latin"},
	{"lv", "Latvian"},		/* Lettish */
	{"ln", "Lingala"},
	{"lt", "Lithuanian"},
	{"mk", "Macedonian"},
	{"mg", "Malagasy"},
	{"ms", "Malay"},
	{"ml", "Malayalam"},
#if 0
	{"??", "Manipuri"},
#endif
	{"mt", "Maltese"},
	{"mi", "Maori"},
	{"mr", "Marathi"},
	{"mo", "Moldavian"},
	{"mn", "Mongolian"},
	{"na", "Nauru"},
	{"ne", "Nepali"},
	{"no", "Norwegian"},
	{"oc", "Occitan"},
	{"or", "Oriya"},
	{"om", "Oromo"},		/* Afan, Galla */
#if 0
	{"??", "Papiamentu"},
#endif
	{"ps", "Pashto"},		/* Pushto */
	{"pl", "Polish"},
	{"pt", "Portuguese"},
	{"pa", "Punjabi"},
	{"qu", "Quechua"},
	{"rm", "Rhaeto-Romance"},
	{"ro", "Romanian"},
	{"ru", "Russian"},
#if 0
	{"??", "Sami"},		/* Lappish */
#endif
	{"sm", "Samoan"},
	{"sg", "Sangro"},
	{"sa", "Sanskrit"},
	{"sr", "Serbian"},
	{"sh", "Serbo-Croatian"},
	{"st", "Sesotho"},
	{"tn", "Setswana"},
	{"sn", "Shona"},
	{"sd", "Sindhi"},
	{"si", "Sinhalese"},
	{"ss", "Siswati"},
	{"sk", "Slovak"},
	{"sl", "Slovenian"},
	{"so", "Somali"},
	{"es", "Spanish"},
	{"su", "Sundanese"},
	{"sw", "Swahili"},		/* Kiswahili */
	{"sv", "Swedish"},
#if 0
	{"??", "Syriac"},
#endif
	{"tl", "Tagalog"},
	{"tg", "Tajik"},
#if 0
	{"??", "Tamazight"},
#endif
	{"ta", "Tamil"},
	{"tt", "Tatar"},
	{"te", "Telugu"},
	{"th", "Thai"},
	{"bo", "Tibetan"},
	{"ti", "Tigrinya"},
	{"to", "Tonga"},
	{"ts", "Tsonga"},
	{"tr", "Turkish"},
	{"tk", "Turkmen"},
	{"tw", "Twi"},
	{"ug", "Uighur"},
	{"uk", "Ukrainian"},
	{"ur", "Urdu"},
	{"uz", "Uzbek"},
#if 0
	{"??", "Venda"},
#endif
	{"vi", "Vietnamese"},
	{"vo", "Volapuk"},
	{"cy", "Welsh"},
	{"wo", "Wolof"},
	{"xh", "Xhosa"},
#if 0
	{"??", "Yi"},
	{"ji", "Yiddish"},
#endif
	{"yi", "Yiddish"},
	{"yo", "Yoruba"},
	{"zu", "Zulu"} };
  int i;

  Mlanguage = msymbol ("language");
  msymbol_put (Mlanguage, Mtext_prop_serializer,
	       (void *) msymbol__serializer);
  msymbol_put (Mlanguage, Mtext_prop_deserializer,
	       (void *) msymbol__deserializer);
  for (i = 0; i < ((sizeof lang_rec) / (sizeof lang_rec[0])); i++)
    msymbol_put (msymbol (lang_rec[i].name), Mlanguage,
		 msymbol (lang_rec[i].fullname));
  return 0;
}

void
mlang__fini (void)
{
}
