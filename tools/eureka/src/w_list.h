//------------------------------------------------------------------------
//  WAD LISTS
//------------------------------------------------------------------------
//
//  Eureka DOOM Editor
//
//  Copyright (C) 2001-2009 Andrew Apted
//  Copyright (C) 1997-2003 André Majorel et al
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
//------------------------------------------------------------------------
//
//  Based on Yadex which incorporated code from DEU 5.21 that was put
//  in the public domain in 1994 by Raphaël Quinet and Brendon Wyber.
//
//------------------------------------------------------------------------


#ifndef YH_WADLIST  /* DO NOT INSERT ANYTHING BEFORE THIS LINE */
#define YH_WADLIST


/*
 *  Wad_list - list of open wads
 *
 *  The Wad_list class is designed to hold the list of open
 *  wads. It will probably have only one (global) instance.
 *  Here is how it's supposed to be used :
 *
 *    extern Wad_list wl;
 *    Wad_file *wf;
 *
 *    // Is foo.wad in our list ?
 *    for (wl.rewind (); wl.get (wf);)
 *      if (strcmp (wf->filename, "foo.wad") == 0)
 *      {
 *        puts ("Got it !");
 *        break;
 *      }
 *
 *    // Remove from the list all wads beginning with a "z"
 *    for (wl.rewind (); wl.get (wf);)
 *      if (*wf->filename == 'z')
 *        wl.del ();
 */


class Wad_list_priv;


class Wad_list
{
public:
    Wad_list    ();
    ~Wad_list   ();

public:
    void rewind ();
    bool get    (Wad_file0 *& wf);
    void insert (Wad_file0 *);
    void del    ();

private :
    Wad_list (const Wad_list&);     // Too lazy to implement it
    Wad_list& operator= (const Wad_list&);  // Too lazy to implement it
    Wad_list_priv *priv;
};


extern Wad_list wad_list;   // One global instance


#endif  /* DO NOT ADD ANYTHING AFTER THIS LINE */
//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
