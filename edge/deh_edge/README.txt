
DEH_EDGE 1.1
============

by Andrew Apted.  6th July 2004.


Introduction
------------

DEH_EDGE is a utility for converting DeHackEd (.DEH and .BEX) files into
something you can use with the EDGE engine (http://edge.sourceforge.net).
The output is a special WAD file containing a bunch of DDF lumps.

DEH_EDGE is a command line utility, so the Windows version should be
run from the DOS box.  Dragging a .DEH or .BEX file onto the DEH_EDGE
executable may work, but you could miss some important messages.

DEH_EDGE handles all the DeHackEd patch formats: the old binary formats,
the newer text format, and the BOOM extensions (BEX).  A few obscure
things, like raw action offsets, are not supported.


Usage
-----

Run it like this:

   deh_edge batman.deh

It will create the file "batman.hwa".  This .HWA file is really
just a normal WAD file containing the DDF, but the HWA extension
means that it was created from a DeHackEd file ('H' for 'Hack').

Then run EDGE using this additional file, for example:

   gledge32 -file batman.wad batman.hwa

NOTE: the original .DEH file is not modified in any way.

Multiple .DEH or .BEX files can be given.  They will be merged
together to create one big mess.  I cannot take responsibility
for any insanity you may experience when trying this.

 
Usage with EDGE 1.27
--------------------

You need to specify the EDGE version number.  For example:

   deh_edge --edge 1.27 batman.deh

It will create the file "batman_deh.wad".  (Note that the
extension is not HWA like above -- .HWA files only work in
EDGE 1.28 and later).

Then run EDGE using this additional file, for example:

   gledge32 -file batman.wad batman_deh.wad


Options
-------

  -o --output XXX  Output file.  This is optional, the default output
                   filename will simply replace the DEH extension with
                   the HWA extension.

  -e --edge #.##   EDGE version to target.  The current default is 1.28.
                   This option will cause the output DDF to be optimised
                   for that Edge version, and workaround any known bugs.

  -q --quiet       Quiet mode, disable warning messages.

  -a --all         All: converts everything into DDF.  Normally only the
                   parts which has been modified are converted to DDF.
                   This option can make the output WAD much bigger !
                   Mainly useful when trouble-shooting problems.


Limitations
-----------

1. Doesn't yet convert some of the new BOOM/MBF actions (e.g. A_Die
   and A_Mushroom).  These will be added soon.

2. Doesn't handle the BEX "INCLUDE" directive.

3. No support for a few DEH patchables, e.g. 'God Mode Health'.
   These can't be changed in EDGE via DDF.  Most (probably all) of
   them have little importance (like what certain cheats give you).

4. Raw action offsets and sprite/sound name pointers are unsupported.
   These are heavily dependent on the exact version of the DOOM EXE.
   Since they are rarely used, and it requires so much work, they
   will probably never be supported.
 
5. Text replacements only work in EDGE when the language selected is
   "ENGLISH".  This is a nuisance for anyone who usually plays in a
   different language.  The replacements are usually English anyway,
   so this limitation is fairly minor.


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

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
General Public License for more details.


Contact
-------

Comments should be made on the EDGE forums:

    http://sourceforge.net/forum/?group_id=2232

