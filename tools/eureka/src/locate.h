/*
 *  locate.h - Locate class
 *  AYM 2003-03-24
 */

/*
This file is copyright André Majorel 2003.

This program is free software; you can redistribute it and/or modify it under
the terms of version 2 of the GNU General Public License as published by the
Free Software Foundation.

This program is distributed in the hope that it will be useful, but WITHOUT
ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program; if not, write to the Free Software Foundation, Inc., 59 Temple
Place, Suite 330, Boston, MA 02111-1307, USA.
*/


#ifndef YH_LOCATE  /* DO NOT INSERT ANYTHING BEFORE THIS LINE */
#define YH_LOCATE


class Locate
{
  public :
    Locate (const char *const *search_path, const char *name, bool backwards);
    void rewind ();
    const char *get_next ();

  private :
    const char *const *search_path;
    const char        *name;
    bool               absolute;
    bool               backwards;
    const char *const *cursor;
    bool               rewound;
    char               pathname[1025];
};


#endif  /* DO NOT ADD ANYTHING AFTER THIS LINE */
