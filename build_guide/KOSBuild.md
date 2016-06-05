# 3DGE - KallistiOS Building Instructions
# (C) 2011-2016 Isotope SoftWorks & Contributors
##### id Tech/id Tech 2/ id Tech 3/ id Tech 4 (C) id Software, LLC
#### All id techs (and 3DGE itself) Licensed under the GPLv2
http://www.gnu.org/licenses/old-licenses/gpl-2.0.en.html
# See our 3DGE Wiki (http://3dfxdev.net/edgewiki/index.php/Main_Page)
---
This build uses [KallistiOS](http://gamedev.allusion.net/softprj/kos/) to interact with 3DGE's codebase. The version of KOS so far supported is 2.0, and you can check these builds out at their official [KOS Github page](https://github.com/ljsebald/KallistiOS). You need libraries specific to KOS built -- more information to come!

# Building with Make
---
 1) #### > make -f Makefile.dc

That will generate KallistiOS complaint code from the main branch, and produces an ELF-encodeded binary.

#### NOTE: The Dreamcast code currently experiences massive bitrot due to our rendering and sound upgrades. This version cannot be reliably built and tested as-is. Work is ongoing to bring the KOS version up-to-date with the rest of our engine.

---
(C) Isotope SoftWorks, 2016


