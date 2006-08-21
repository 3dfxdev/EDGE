
Intro
=====

This site is built using a program called HTP (HTML Preprocessor).
It takes a source file, includes the contents of other files into it,
substitutes macro strings, and produces the output file.  Hence common
elements can be created once and used multiple times.


Files
=====

.htp  :  contains HTP source, which is HTML plus some extra
         tags which HTP handles.  These htp files become
         the .htm files in the output folder.

.inc  :  include file.

.tpl  :  template file.  These are just include files, but
         they are used at the end of a htp file to create a
         whole page.  The htp file only contains the content,
         and the template contains the top and bottom bits
         (like the <html>, <head>, <body> tags).

.rsp  :  response file.  Contains the list of input and output
         files for HTP to process.


Folders
=======

out/    :  where all the output goes.  Visit out/index.htm in
           your web browser to see the final pages.

main/   :  files which define the main area of the website,
           such as Home, About, Screenshots, etc..  Each of
           these pages have the same banner and menu.

ddf/    :  files for the DDF area.

rts/    :  files for the RTS area.

common/ :  common stuff.  Every htp file includes common/common.inc
           which contains macros that many pages may use.

shots/  :  contains the screenshot images, and htp files for
           showing each large screenshot by itself.  The large
           images must be 640x400 and thumbnail images must be
           160x100 in size.

css/    :  cascading style sheets.

images/ :  miscellaneous images.


Building the Site
=================

For Windows: in a console type: build

For Linux: in a terminal type: make

