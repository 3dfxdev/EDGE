//----------------------------------------------------------------------------
//  EDGE Data Definition File Code (Language handling settings)
//----------------------------------------------------------------------------
// 
//  Copyright (c) 1999-2001  The EDGE Team.
// 
//  This program is free software; you can redistribute it and/or
//  modify it under the terms of the GNU General Public License
//  as published by the Free Software Foundation; either version 2
//  of the License, or (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//----------------------------------------------------------------------------
//
// Language handling Setup and Parser Code
//
// 1998/10/29 -KM- Allow cmd line selection of language.
//
// This is somewhat different to most DDF reading files. In order to read the
// language specific strings, it uses the format:
//
// <RefName>=<String>;
//
// as opposed to the normal entry, which should be:
//
// [<Refname>]
// STRING=<string>;
//
// also the file suffix is LDF (Language Def File), this is to avoid confusion with
// the oridnary DDF files. The default file is DEFAULT.LDF, which can be subbed by
// using -lang <NameOfLangFile>.
//

#include "i_defs.h"

#include "ddf_locl.h"
#include "ddf_main.h"
#include "dm_state.h"
#include "m_argv.h"
#include "p_spec.h"
#include "r_local.h"
#include "z_zone.h"

static language_t *dynamic_lang;

static const language_t template_lang =
{
  DDF_BASE_NIL,  // ddf
  NULL,  // refs
};

static const char *want_language;
int cur_lang_index = -1;

static boolean_t read_languages = false;


// array of languages

language_t ** languages = NULL;
int num_languages = 0;

static stack_array_t languages_a;


static void DDF_LanguageAddRef(language_t *lang, const char *ref, 
    const char *text);


//
// Hardcoded entries.
//
// These are _only_ needed for strings that are used before the
// LANGUAGE.LDF file or DDFLANG lump(s) have been read in.

static langref_t hardcoded_ldfs[] =
{
  { "DefaultLoad", "M_LoadDefaults: Loaded system defaults.\n", NULL },
  { "DevelopmentMode", "Development Mode is enabled.\n", NULL },
  { "TurboScale",  "Turbo scale: %i%%\n", NULL },
  { "WadFileInit", "W_Init: Init WADfiles.\n", NULL },

  { NULL, NULL, NULL }
};


//
//  DDF PARSING ROUTINES
//

static boolean_t LanguageStartEntry(const char *name)
{
  int i, index = -1;
  boolean_t replaces = false;

  if (name && name[0])
  {
    for (i=0; i < num_languages; i++)
    {
      if (DDF_CompareName(languages[i]->ddf.name, name) == 0)
      {
        dynamic_lang = languages[i];
        index = i;
        replaces = true;
        break;
      }
    }
  }

  // not found, create a new one
  if (! replaces)
  {
    index = num_languages;

    Z_SetArraySize(&languages_a, ++num_languages);

    dynamic_lang = languages[index];
    
    // initialise the new entry
    dynamic_lang[0] = template_lang;

    dynamic_lang->ddf.name = (name && name[0]) ? Z_StrDup(name) :
        DDF_MainCreateUniqueName("UNNAMED_LANGUAGE", num_languages);
  }

  if (DDF_CompareName(name, want_language) == 0)
  {
    cur_lang_index = index;
  }

  return replaces;
}

static void LanguageParseField(const char *field, const char *contents,
    int index, boolean_t is_last)
{
#if (DEBUG_DDF)  
  L_WriteDebug("LANGUAGE_PARSE: %s = %s;\n", field, contents);
#endif

  if (! is_last)
  {
    DDF_WarnError("Unexpected comma `,' in LANGUAGE.LDF\n");
    return;
  }
    
  DDF_LanguageAddRef(dynamic_lang, field, contents);
}

static void LanguageFinishEntry(void)
{
  // Compute CRC.  Not needed for languages.
  dynamic_lang->ddf.crc = 0;
}

static void LanguageClearAll(void)
{
  // safe to delete all language entries

  num_languages = 0;
  Z_SetArraySize(&languages_a, num_languages);
}


void DDF_ReadLangs(void *data, int size)
{
  readinfo_t language;

  language.memfile = (char*)data;
  language.memsize = size;
  language.tag = "LANGUAGES";
  language.entries_per_dot = 1;
  
  if (language.memfile)
  {
    language.message = NULL;
    language.filename = NULL;
    language.lumpname = "DDFLANG";
  }
  else
  {
    language.message = "DDF_InitLanguage";
    language.filename = "language.ldf";
    language.lumpname = NULL;
  }

  language.start_entry  = LanguageStartEntry;
  language.parse_field  = LanguageParseField;
  language.finish_entry = LanguageFinishEntry;
  language.clear_all    = LanguageClearAll;

  DDF_MainReadFile(&language);

  read_languages = true;
}

void DDF_LanguageInit(void)
{
  want_language = M_GetParm("-lang");
  cur_lang_index = -1;

  if (!want_language)
  {
    // -AJA- FIXME: should be config-file-selectable
    want_language = "ENGLISH";
  }

  Z_InitStackArray(&languages_a, (void ***)&languages, sizeof(language_t), 0);
}

void DDF_LanguageCleanUp(void)
{
   if (cur_lang_index < 0)
     I_Error("Unknown language: %s\n", want_language);
}

static langref_t *FindLanguageRef(const char *refname)
{
  int i;
  char *tmpname;
  langref_t *entry;

  if (cur_lang_index < 0)
    return NULL;

  DEV_ASSERT2(cur_lang_index < num_languages);

  // convert to uppercase

  tmpname = (char*)I_TmpMalloc(strlen(refname) + 1);
  for (i = 0; refname[i]; i++)
  {
    tmpname[i] = toupper(refname[i]);
  }
  tmpname[i] = 0;

  // when a reference cannot be found in the current language, we look
  // for it in the other languages.

  for (i=cur_lang_index; i < cur_lang_index+num_languages; i++)
  {
    language_t *lang = languages[i % num_languages];

    entry = lang->refs;

    // -ES- 2000/02/04 Optimisation: replaced stricmp with strcmp
    while (entry != NULL && strcmp(tmpname, entry->refname))
      entry = entry->next;
    
    if (entry)
    {
      I_TmpFree(tmpname);
      return entry;
    }
  }

  // not found !
  I_TmpFree(tmpname);
  return NULL;
}

//
// DDF_LanguageLookup
//
// Globally Visibile to all files that directly or indirectly include ddf_main.h;
// This compares the ref name given with the refnames in the language lookup
// table. If one compares with the other, a pointer to the string is returned. If
// one is not found than an error is generated.

const char *DDF_LanguageLookup(const char *refname)
{
  int i;
  langref_t *entry;

  // -AJA- Due to the new "DDF lumps in EDGE.WAD" thing, certain
  //       messages need to be looked up before any LDF entries have
  //       been read in (from EDGE.WAD).  These entries must be
  //       hardcoded in this file.  
  if (! read_languages)
  {
    for (i=0; hardcoded_ldfs[i].refname; i++)
    {
      if (DDF_CompareName(refname, hardcoded_ldfs[i].refname) == 0)
        return hardcoded_ldfs[i].string;
    }
  }
  else
  {
    entry = FindLanguageRef(refname);
  
    if (entry != NULL)
      return entry->string;
  }
  
  if (strict_errors)
    DDF_Error("DDF_LanguageLookup: Unknown String Ref: %s\n", refname);
  
  return refname;
}

//
// DDF_LanguageValidRef
//
// Returns whether the given ref is valid.
// -ES- 2000/02/04 Added

boolean_t DDF_LanguageValidRef(const char *refname)
{
  langref_t *entry = FindLanguageRef(refname);
 
  return (entry == NULL) ? false : true;
}


//
// DDF_LanguageAddRef
//
// Puts the string into the buffer entry and adds it to the linked list.

static void DDF_LanguageAddRef(language_t *lang, const char *ref, 
    const char *text)
{
  langref_t *entry;

  // look for entry with same name

  for (entry=lang->refs; entry; entry=entry->next)
  {
    if (DDF_CompareName(entry->refname, ref) == 0)
      break;
  }

  if (entry)
  {
    // replace existing entry

    // -ES- INTENTIONAL CONST OVERRIDE
    Z_Free((char*) entry->string);

    entry->string = Z_StrDup(text);
    return;
  }

  // create new entry
  
  entry = Z_New(langref_t, 1);

  entry->refname = Z_StrDup(ref);
  entry->string  = Z_StrDup(text);

  // link it in
  entry->next = lang->refs;
  lang->refs = entry;
}

