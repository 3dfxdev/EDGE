
DEH_EDGE 0.9
============

by Andrew Apted.  23rd February 2004.


Introduction
------------

DEH_EDGE is a utility for converting DeHackEd (.DEH) files into something
that can be loaded into the EDGE engine (http://edge.sourceforge.net).
The output is a WAD containing a bunch of DDF lumps.

DEH_EDGE is a command line utility, so the Windows version must be run
from the DOS box.  Although dragging a DEH file onto the DEH_EDGE
executable may work, you could miss some important messages.

DEH_EDGE handles all the DeHackEd patch formats (the old binary formats
and the new text format).  Text strings and code-pointers in the binary
formats are not supported.  DEH_EDGE does __NOT__ support BOOM extensions
(BEX) at this time.  


Usage
-----

Run it like this:

   deh_edge atc2.deh

which will create the file "atc2_deh.wad".  Then run EDGE using this
additional file, for example:

   gledge32 -file ... atc2_deh.wad


Options
-------

  -o --output    Output file.  This is optional, the default output
				 filename will be the DEH filename with the extension
				 removed and "_deh.wad" added onto the end.

  -q --quiet     Quiet mode, disable warning messages.

  -a --all       All.  Converts everything into DDF.  Normally only
				 the stuff which has been modified is converted to DDF.
				 This option makes the output WAD much bigger.


Limitations
-----------

1. Can only handle a single DEH patch file at a time.

2. No support for BOOM extended format (BEX), like [STRINGS] etc.

3. No support for BOOM/MBF things/frames/actions (e.g. the DOG).

(The above three may be added in a future version).

4. No support for a few DEH patchables, e.g. 'God Mode Health'.
   These can't be changed in EDGE via DDF.  Most (probably all) of them
   have little importance (like what certain cheats give you).

5. Text strings, code pointers, and sprite/sound name pointers
   from binary patch files are not supported.  These are heavily
   dependent on the exact version of the DOOM EXE.  Even DeHackEd
   itself can't load these when the EXE version is different.
 
   Raw action offsets and sprite/sound offsets in text patch files
   are also not handled, for the same reason.  Normal code pointers
   are supported, of course.

6. Text replacements only work when ENGLISH language is selected in
   EDGE.  (The replacements are probably english anyway, so I think
   this limitation is a minor one).


Acknowledgements
----------------

DEH_EDGE uses source code from:

  +  DeHackEd 2.3, by Greg Lewis.

  -  DOOM source code (C) 1993-1996 id Software, Inc.
  -  Linux DOOM Hack Editor, by Sam Lantinga.
  -  PrBoom's DEH/BEX code, by Ty Halderman, TeamTNT.


Copyright/License
-----------------

DEH_EDGE is Copyright (C) 2004 The EDGE Team.

All trademarks are the propriety of their owners.

DEH_EDGE is under the GNU General Public License (GPL).  See the file
COPYING.txt in the source package (or visit http://www.gnu.org) for
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

