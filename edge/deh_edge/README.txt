
DEH_EDGE
========

by Andrew Apted.  23rd January 2004.


Introduction
------------

DEH_EDGE is a utility for converting DeHackEd (.DEH) files into something
that can be loaded into the EDGE engine (http://edge.sourceforge.net).
The output is a WAD containing a bunch of DDF lumps.

DEH_EDGE is a command line utility, so the Windows version must be run
from the DOS box -- although dragging a DEH file onto the DEH_EDGE
executable may work.


Usage
-----

Run it like this:

   deh_edge batman.deh

which will create the file "batman_deh.wad". 


Options
-------

  -o    Output file.  This is optional, the default output filename
        will be the DEH filename with the extension removed and
        "_deh.wad" added onto the end.

  -q    Quiet mode, disable warning messages.

  -a    All.  Converts everything into DDF.  Normally only the stuff
        which has been modified is converted to DDF.


Limitations
-----------

1. No support for BOOM extended format (BEX), like [STRINGS] etc.
2. No support for BOOM/MBF mobjs/states/actions (like the DOG).

(The above two may be added to a future version).

3. No support for a few DEH patchables, e.g. 'God Mode Health'.
   Most (probably all) of them have little importance (like
   what certain cheats give you).

4. Text replacements only work when ENGLISH language is selected.


Acknowledgements
----------------

DEH_EDGE is based on:

  +  DeHackEd source code, by Greg Lewis.

  -  DOOM source code (C) 1993-1996 id Software, Inc.
  -  Linux DOOM Hack Editor, by Sam Lantinga.
  -  PrBoom's DEH/BEX code, by Ty Halderman, TeamTNT.


Copyright/License
-----------------

DEH_EDGE is Copyright (C) 2004 The EDGE Team.

All trademarks are the propriety of their owners.

DEH_EDGE is under the GNU General Public License (GPL).  See the file
COPYING.txt in the source package (or go to http://www.gnu.org) for
the full text, but to summarise:

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or (at
your option) any later version.

This program is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
General Public License for more details.


Contact
-------

Comments should be made on the EDGE forums:

    http://sourceforge.net/forum/?group_id=2232

