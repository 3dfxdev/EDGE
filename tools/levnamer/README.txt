
LEVNAMER
========

by Andrew Apted.  6th June 2001.


Introduction
------------

LevNamer is a simple utility to rename the level marker names (E1M2,
MAP29, etc) in WAD files.  Most useful used in conjunction with EDGE,
the Enhanced DOOM Gaming Engine.  It is a command line utility, so the
Windows version must be run from the DOS box.


Usage
-----

Running it looks something like this:

   levname map.wad -o map2.wad -convert MAP01-MAP32 JIM01

See below for more info...


Options
-------

  -o           gives the output file.  This is mandatory.

  -convert     specifies what the conversion is.  It is followed by
               two arguments: the source names and the destination
               names.  This option is also mandatory.

  -loadall     LevNamer will normally try to copy lumps from the
               input WAD file to the output file instead of loading
               them into memory.  This allows you to process very
               large WADS (e.g. 15 MB for DOOM II) on a low memory
               machine.

               This option causes everything from the input file to be
               loaded into memory.  This allows you to run LevNamer
               using the same file for both input and output, but I
               strongly recommend _against_ this: you could lose your
               original WAD if something goes wrong.


Conversions
-----------

Level names can contain zero, one or two digits, for example: "FRED",
"JIM7", "MAP03", "E4M2".  When they contain two digits that are
separated (like in "E4M2"), they are treated specially: the first
digit is an episode number (1-9) and the second digit is a mission
number (1-9), and the conversion takes this into account (e.g. E2M1
is the next name after E1M9, i.e. not E2M0).

Each argument after -convert is actually a set of level names.  There
are three options:

   (1) a single name, like "MAP32".
   (2) a range of names, like "E1M1-E1M9" (note the `-' in between).
   (3) a comma-separated list of names, like "MAP01,MAP03,MAP05".

(Note: no spaces are allowed in them)

The number of names in the source set has to be the same as the number
in the destination set, of course, with one useful exception: if the
source set has 2 or more names, and the destination is just a single
name, then it is assumed that the destination set contains all the
required levels counting upwards from the given name.  For example:

   -convert  E1M1-E1M9  MAP01

there are 9 level names in the source set, so the destination set is
really "MAP01-MAP09".


NOTE: Level names can be given in either upper or lower case.  In
      the WAD format, only upper case is allowed, so LevNamer will
      always make them uppercase.


GL Nodes
--------

When GL Nodes are present, their level names will also be converted,
automatically.  For example "GL_MAP04" would become "GL_JIM04".
There's a limit of 5 characters on level names, because of the "GL_"
takes up 3, and the WAD format is limited to 8 characters.


Copyright
---------

LevNamer is Copyright (C) 2001 Andrew Apted.

All trademarks are the propriety of their owners.


License
-------

LevNamer is under the GNU General Public License (GPL).  See the file
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


Links
-----

The EDGE homepage can be found here:
   http://edge.sourceforge.net/


Contact
-------

Comments to : edge-public@lists.sourceforge.net
Bugs to     : edge-bugs@lists.sourceforge.net

