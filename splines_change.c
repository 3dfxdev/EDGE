Revision: 48
Author: corbin
Date: Saturday, September 28, 2013 9:54:11 PM
Message:
changes by basteagui:

-monster muzzle flashes
-breakable/shattering spawn_models
-screen noise effects
----
Modified : /client/cl_effects.c
Modified : /client/cl_lights.c
Modified : /client/cl_particle.c
Modified : /client/cl_screen.c
Modified : /client/cl_tempent.c
Modified : /client/client.h
Modified : /client/particles.h
Modified : /game/g_model.c
Modified : /projects/VS2010/kmquake2_2010.sln
Modified : /splines/Splines.vcxproj

Revision: 47
Author: kadlubom
Date: Saturday, August 3, 2013 4:13:52 AM
Message:
cmaster.matso: Added camera info commands.

----
Modified : /client/cl_main.c
Modified : /client/cl_view.c
Modified : /client/ref.h
Modified : /renderer/r_main.c

Revision: 46
Author: kadlubom
Date: Saturday, April 13, 2013 11:05:38 PM
Message:
cmaster.matso: Cosmetisc in the code.

----
Modified : /client/cl_ents.c
Modified : /client/client.h
Modified : /client/ref.h
Modified : /game/p_client.c

Revision: 45
Author: kadlubom
Date: Sunday, December 30, 2012 4:33:25 AM
Message:
cmaster.matso: Further changes related to MD3 support.

----
Modified : /client/cl_parse.c
Modified : /client/client.h
Modified : /game/game.h
Modified : /game/p_client.c
Modified : /game/q_shared.h
Modified : /projects/VS2010/kmquake2_2010.vcxproj
Modified : /projects/VS2010/kmquake2_2010.vcxproj.filters
Added : /renderer/q3anims.h
Modified : /server/server.h
Modified : /server/sv_game.c
Modified : /server/sv_init.c

Revision: 44
Author: kadlubom
Date: Sunday, December 9, 2012 3:24:41 AM
Message:
cmaster.matso: Done porting code for Q3 player models - need some assests for testing, though.

----
Modified : /client/cl_ents.c
Modified : /client/cl_parse.c
Modified : /client/client.h
Modified : /client/ref.h
Modified : /game/q_shared.h
Modified : /qcommon/qfiles.h
Modified : /renderer/r_alias.c
Modified : /renderer/r_image.c
Modified : /renderer/r_local.h
Modified : /renderer/r_model.c
Modified : /renderer/r_model.h

Revision: 43
Author: kadlubom
Date: Friday, December 7, 2012 11:48:09 PM
Message:
cmaster.matso: MD3 support commit - finishing MD3 drawing code - only loading code (animations mainly) left to be updated, and some weapon rendering stuff.

----
Modified : /client/cl_ents.c
Modified : /client/cl_parse.c
Modified : /client/client.h
Modified : /game/q_shared.c
Modified : /game/q_shared.h
Modified : /renderer/r_alias.c
Modified : /renderer/r_model.h

Revision: 42
Author: kadlubom
Date: Wednesday, November 28, 2012 9:28:27 AM
Message:
cmaster.matso: On the way towards MD3 full animation capability - added some code for MD3 models rendering (there is much to do with it yet).

----
Modified : /client/cl_ents.c
Modified : /client/client.h
Modified : /client/ref.h
Modified : /game/q_shared.h

Revision: 41
Author: kadlubom
Date: Sunday, November 18, 2012 3:20:46 AM
Message:
cmaster.matso: On the way towards MD3 full animation capability - seek any 'cmaster.matso' tags to see my doubts. Some of the code from q3 player model source has been moved,
               thus the files are excluded from compilation for now. In future I will dispose of them completely.

----
Modified : /client/cl_parse.c
Modified : /client/client.h
Modified : /client/ref.h
Modified : /client/snd_stream.c
Modified : /game/acebot_spawn.c
Modified : /game/q_shared.h
Modified : /projects/VS2010/kmquake2_2010.sln
Modified : /projects/VS2010/kmquake2_2010.vcxproj
Modified : /projects/VS2010/kmquake2_2010.vcxproj.filters
Modified : /renderer/r_alias.c
Modified : /renderer/r_model.c
Modified : /renderer/r_model.h

Revision: 40
Author: corbin
Date: Friday, November 16, 2012 5:43:21 PM
Message:
OGG path redundancy checks.
----
Modified : /client/snd_stream.c

Revision: 39
Author: corbin
Date: Friday, November 16, 2012 5:38:24 PM
Message:
Low-quality streaming audio (11KHz) can be garbled or non-existent, this fixes the problem.
----
Modified : /client/snd_dma.c

Revision: 38
Author: chui
Date: Saturday, November 10, 2012 4:02:32 PM
Message:
Init SDL drivers

----
Modified : /Makefile.linux32bit_standalone
Modified : /client/snd_mix.c
Added : /sdl
Added : /sdl/cd_sdl.c
Added : /sdl/gl_sdl.c
Added : /sdl/in_sdl.c
Added : /sdl/net_sdl.c
Added : /sdl/qgl_sdl.c
Added : /sdl/qsh_sdl.c
Added : /sdl/snd_sdl.c
Added : /sdl/sys_sdl.c
Added : /sdl/vid_sdl.c

Revision: 37
Author: chui
Date: Saturday, November 10, 2012 3:16:10 PM
Message:
Linux fix

----
Modified : /Makefile.linux32bit_standalone
Modified : /game/acebot_nodes.c
Modified : /game/acebot_spawn.c
Modified : /game/g_misc.c
Modified : /game/g_monster.c
Modified : /game/g_patchplayermodels.c
Modified : /game/g_utils.c
Modified : /unix/sys_unix.c

Revision: 36
Author: kadlubom
Date: Monday, September 17, 2012 12:59:42 PM
Message:
cmaster.matso: Committing missing files.
----
Modified : /Makefile
Added : /game/g_func_decs.h
Added : /game/g_func_list.h
Added : /game/g_mmove_decs.h
Added : /game/g_mmove_list.h
Added : /include
Added : /include/al
Added : /include/al/altypes.h
Added : /include/jpeg
Added : /include/jpeg/jconfig.h
Added : /include/jpeg/jmorecfg.h
Added : /include/jpeg/jpeglib.h
Added : /include/ogg
Added : /include/ogg/codec.h
Added : /include/ogg/ogg.h
Added : /include/ogg/os_types.h
Added : /include/ogg/vorbisfile.h
Added : /include/ogg_old
Added : /include/ogg_old/codec.h
Added : /include/ogg_old/ogg.h
Added : /include/ogg_old/os_types.h
Added : /include/ogg_old/vorbisfile.h
Added : /include/zlibpng
Added : /include/zlibpng/ioapi.h
Added : /include/zlibpng/png.h
Added : /include/zlibpng/pngconf.h
Added : /include/zlibpng/unzip.h
Added : /include/zlibpng/zconf.h
Added : /include/zlibpng/zip.h
Added : /include/zlibpng/zlib.h
Modified : /linux/gl_glx.c
Modified : /linux/net_udp.c
Modified : /linux/q_shlinux.c
Modified : /linux/qgl_linux.c
Modified : /linux/rw_x11.c
Modified : /linux/sys_linux.c
Modified : /linux/vid_so.c
Modified : /null/vid_null.c
Modified : /qcommon/vid_modes.h
Modified : /renderer/glext.h
Modified : /renderer/qgl.h
Added : /server/sv_legacy.h
Added : /ui/menu_credits_id.h
Added : /ui/menu_credits_rogue.h
Added : /ui/menu_credits_xatrix.h
Modified : /unix/gl_glx.c
Modified : /unix/in_unix.c
Modified : /unix/net_udp.c
Modified : /unix/qgl_unix.c
Modified : /unix/qsh_unix.c
Modified : /unix/sys_unix.c
Modified : /unix/vid_so.c
Added : /unix/zip/zip.c
Added : /unix/zip/zip.h
Modified : /win32/resource.h
Added : /win32/win_conproc.h
Modified : /win32/winquake.h

Revision: 35
Author: kadlubom
Date: Sunday, September 16, 2012 11:54:48 PM
Message:
cmaster.matso: Major marge of the current Scourge code with the new v021 KMquake2 engine code. In case of Windows use solutions from either 'projects/VS2008' or 'projects/2010'. Commit for VS2008 users.

----
Added : /projects/VS2008/kmquake2_2008.sln

Revision: 34
Author: kadlubom
Date: Sunday, September 16, 2012 11:54:10 PM
Message:
cmaster.matso: Major marge of the current Scourge code with the new v021 KMquake2 engine code. In case of Windows use solutions from either 'projects/VS2008' or 'projects/2010'. Commit for VS2008 users.

----
Added : /projects/VS2008
Added : /projects/VS2008/extractfuncs_2008.vcproj
Added : /projects/VS2008/kmq2_2008.rc
Added : /projects/VS2008/kmquake2_2008.vcproj
Added : /projects/VS2008/lazarus_2008.rc
Added : /projects/VS2008/lazarus_2008.vcproj
Added : /projects/VS2008/splines_2008.vcproj
Added : /win32/win_glw.h

Revision: 33
Author: kadlubom
Date: Sunday, September 16, 2012 11:52:49 PM
Message:
cmaster.matso: Major marge of the current Scourge code with the new v021 KMquake2 engine code. In case of Windows use solutions from either 'projects/VS2008' or 'projects/2010'. Commit for VS2010 users.

----
Modified : /client/cl_cin.c
Modified : /client/cl_cinematic.c
Modified : /client/cl_console.c
Modified : /client/cl_download.c
Modified : /client/cl_effects.c
Modified : /client/cl_ents.c
Added : /client/cl_hud.c
Modified : /client/cl_input.c
Modified : /client/cl_keys.c
Modified : /client/cl_lights.c
Modified : /client/cl_main.c
Modified : /client/cl_parse.c
Modified : /client/cl_particle.c
Added : /client/cl_predict.c
Modified : /client/cl_screen.c
Modified : /client/cl_string.c
Modified : /client/cl_tempent.c
Modified : /client/cl_utils.c
Modified : /client/cl_view.c
Modified : /client/client.h
Modified : /client/keys.h
Modified : /client/particles.h
Modified : /client/ref.h
Modified : /client/screen.h
Modified : /client/snd_dma.c
Modified : /client/snd_loc.h
Modified : /client/snd_mem.c
Modified : /client/snd_stream.c
Modified : /client/vid.h
Added : /extractfuncs
Added : /extractfuncs/ef_local.h
Added : /extractfuncs/extractfuncs.c
Added : /extractfuncs/l_log.c
Added : /extractfuncs/l_log.h
Added : /extractfuncs/l_memory.c
Added : /extractfuncs/l_memory.h
Added : /extractfuncs/l_precomp.c
Added : /extractfuncs/l_precomp.h
Added : /extractfuncs/l_script.c
Added : /extractfuncs/l_script.h
Added : /game/acebot.h
Added : /game/acebot_ai.c
Added : /game/acebot_cmds.c
Added : /game/acebot_compress.c
Added : /game/acebot_items.c
Added : /game/acebot_movement.c
Added : /game/acebot_nodes.c
Added : /game/acebot_spawn.c
Modified : /game/g_camera.c
Modified : /game/g_cmds.c
Modified : /game/g_crane.c
Modified : /game/g_ctf.c
Modified : /game/g_ctf.h
Modified : /game/g_fog.c
Modified : /game/g_func.c
Modified : /game/g_items.c
Modified : /game/g_lights.c
Modified : /game/g_local.h
Modified : /game/g_lock.c
Modified : /game/g_main.c
Modified : /game/g_misc.c
Modified : /game/g_model.c
Modified : /game/g_monster.c
Modified : /game/g_patchplayermodels.c
Modified : /game/g_phys.c
Modified : /game/g_reflect.c
Modified : /game/g_save.c
Modified : /game/g_spawn.c
Modified : /game/g_svcmds.c
Modified : /game/g_target.c
Modified : /game/g_tracktrain.c
Modified : /game/g_trigger.c
Modified : /game/g_turret.c
Modified : /game/g_utils.c
Modified : /game/g_vehicle.c
Modified : /game/g_weapon.c
Modified : /game/game.h
Modified : /game/km_cvar.c
Modified : /game/km_cvar.h
Modified : /game/m_actor.c
Modified : /game/m_actor_weap.c
Modified : /game/m_brain.c
Modified : /game/m_flipper.c
Modified : /game/m_move.c
Modified : /game/m_soldier.c
Modified : /game/p_client.c
Modified : /game/p_hud.c
Modified : /game/p_menu.c
Modified : /game/p_text.c
Modified : /game/p_view.c
Modified : /game/p_weapon.c
Modified : /game/q_shared.c
Modified : /game/q_shared.h
Added : /projects
Added : /projects/VS2010
Added : /projects/VS2010/extractfuncs_2010.vcxproj
Added : /projects/VS2010/extractfuncs_2010.vcxproj.filters
Added : /projects/VS2010/kmq2_2010.rc
Added : /projects/VS2010/kmquake2_2010.sln
Added : /projects/VS2010/kmquake2_2010.vcxproj
Added : /projects/VS2010/kmquake2_2010.vcxproj.filters
Added : /projects/VS2010/lazarus_2010.rc
Added : /projects/VS2010/lazarus_2010.vcxproj
Added : /projects/VS2010/lazarus_2010.vcxproj.filters
Modified : /qcommon/cmd.c
Modified : /qcommon/cmodel.c
Modified : /qcommon/common.c
Modified : /qcommon/cvar.c
Added : /qcommon/filesystem.c
Added : /qcommon/filesystem.h
Modified : /qcommon/net_chan.c
Modified : /qcommon/pmove.c
Modified : /qcommon/qcommon.h
Modified : /qcommon/qfiles.h
Modified : /renderer/r_alias.c
Modified : /renderer/r_alias.h
Modified : /renderer/r_alias_misc.c
Modified : /renderer/r_arb_program.c
Modified : /renderer/r_backend.c
Modified : /renderer/r_beam.c
Modified : /renderer/r_bloom.c
Modified : /renderer/r_draw.c
Modified : /renderer/r_entity.c
Modified : /renderer/r_fog.c
Modified : /renderer/r_fragment.c
Modified : /renderer/r_glstate.c
Modified : /renderer/r_image.c
Modified : /renderer/r_light.c
Modified : /renderer/r_local.h
Modified : /renderer/r_main.c
Modified : /renderer/r_misc.c
Modified : /renderer/r_model.c
Modified : /renderer/r_model.h
Modified : /renderer/r_particle.c
Modified : /renderer/r_sky.c
Modified : /renderer/r_sprite.c
Modified : /renderer/r_surface.c
Modified : /renderer/r_vlights.c
Modified : /renderer/r_warp.c
Modified : /server/server.h
Modified : /server/sv_ccmds.c
Modified : /server/sv_game.c
Modified : /server/sv_init.c
Modified : /server/sv_main.c
Modified : /server/sv_send.c
Modified : /server/sv_user.c
Modified : /splines/Splines.vcxproj
Modified : /splines/Splines.vcxproj.filters
Modified : /splines/q_shared.cpp
Modified : /splines/q_shared.h
Added : /ui/menu_apply_changes.c
Added : /ui/menu_credits.c
Added : /ui/menu_defaults_confirm.c
Added : /ui/menu_game.c
Added : /ui/menu_game_load.c
Added : /ui/menu_game_save.c
Added : /ui/menu_main.c
Added : /ui/menu_mods.c
Added : /ui/menu_mp_addressbook.c
Added : /ui/menu_mp_dmoptions.c
Added : /ui/menu_mp_download.c
Added : /ui/menu_mp_joinserver.c
Added : /ui/menu_mp_playersetup.c
Added : /ui/menu_mp_startserver.c
Added : /ui/menu_multiplayer.c
Added : /ui/menu_options.c
Added : /ui/menu_options_controls.c
Added : /ui/menu_options_effects.c
Added : /ui/menu_options_interface.c
Added : /ui/menu_options_keys.c
Added : /ui/menu_options_screen.c
Added : /ui/menu_options_sound.c
Added : /ui/menu_quit.c
Added : /ui/menu_video.c
Added : /ui/menu_video_advanced.c
Added : /ui/ui_draw.c
Modified : /ui/ui_local.h
Modified : /ui/ui_main.c
Added : /ui/ui_menu.c
Added : /ui/ui_mouse.c
Added : /ui/ui_utils.c
Added : /ui/ui_widgets.c
Added : /win32/win_cd.c
Added : /win32/win_conproc.c
Added : /win32/win_dedconsole.c
Added : /win32/win_glimp.c
Added : /win32/win_input.c
Added : /win32/win_main.c
Added : /win32/win_net.c
Added : /win32/win_qgl.c
Added : /win32/win_qsh.c
Added : /win32/win_snd.c
Added : /win32/win_vid.c
Added : /win32/win_wndproc.c

Revision: 32
Author: chui
Date: Saturday, September 15, 2012 1:49:04 PM
Message:
Standalone compile without dynamic game library using -DGAME_HARD_LINKED


----
Added : /Makefile.linux32bit_standalone
Modified : /game/acesrc/acebot_nodes.c
Modified : /game/acesrc/acebot_spawn.c
Modified : /game/g_cmds.c
Modified : /game/g_crane.c
Modified : /game/g_ctf.c
Modified : /game/g_fog.c
Modified : /game/g_misc.c
Modified : /game/g_monster.c
Modified : /game/g_patchplayermodels.c
Modified : /game/g_target.c
Modified : /game/g_trigger.c
Modified : /game/g_utils.c
Modified : /game/m_actor.c
Modified : /game/p_client.c
Modified : /game/p_view.c
Modified : /game/p_weapon.c
Modified : /unix/sys_unix.c

Revision: 31
Author: chui
Date: Saturday, September 15, 2012 8:44:33 AM
Message:
Linux compiling.


----
Modified : /client/client.h
Modified : /game/acesrc/acebot_ai.c
Modified : /game/acesrc/acebot_cmds.c
Modified : /game/acesrc/acebot_items.c
Modified : /game/acesrc/acebot_movement.c
Modified : /game/acesrc/acebot_nodes.c
Modified : /game/acesrc/acebot_spawn.c
Modified : /game/g_fog.c
Modified : /game/g_local.h
Modified : /game/g_misc.c
Modified : /game/g_monster.c
Modified : /game/g_patchplayermodels.c
Modified : /game/p_text.c
Modified : /qcommon/files.c
Modified : /qcommon/qcommon.h
Modified : /renderer/include/jpeglib.h
Modified : /renderer/r_alias.c
Modified : /renderer/r_main.c
Modified : /renderer/r_vlights.c
Modified : /renderer/r_warp.c
Modified : /unix/gl_glx.c
Modified : /unix/in_unix.c
Modified : /unix/net_udp.c
Modified : /unix/qgl_unix.c
Modified : /unix/vid_so.c

Revision: 30
Author: kadlubom
Date: Sunday, August 12, 2012 11:58:50 PM
Message:
cmaster.matso: Doing cleaning...

----
Modified : /quake2.vcproj

Revision: 29
Author: kadlubom
Date: Thursday, August 9, 2012 7:41:32 AM
Message:
cmaster.matso: Started integration of Ya3dag MD3 models code, yet there are some problems with it (review code changes for details).

----
Modified : /client/ref.h
Modified : /game/q_shared.h
Modified : /quake2.vcproj
Added : /renderer/q3_PlayerModel.h
Modified : /renderer/r_local.h
Modified : /renderer/r_model.h
Added : /renderer/r_q3_PlayerModel.c

Revision: 28
Author: kadlubom
Date: Thursday, August 2, 2012 2:05:26 AM
Message:
cmaster.matso: The cplipping bug related to the scripted camera is no longer a problem.

----
Modified : /client/cl_view.c
Modified : /renderer/r_main.c
Modified : /splines/splines.cpp
Modified : /splines/splines.h

Revision: 27
Author: kadlubom
Date: Wednesday, August 1, 2012 4:37:01 AM
Message:
cmaster.matso: Camera triggers are ready and hot!

----
Modified : /splines/splines.cpp
Modified : /splines/triggers.h

Revision: 26
Author: kadlubom
Date: Tuesday, July 31, 2012 2:36:12 AM
Message:
cmaster.matso: Triggers loading is bugged! Need to feagure it out.

----
Modified : /splines/splines.cpp

Revision: 25
Author: kadlubom
Date: Tuesday, July 31, 2012 1:51:11 AM
Message:
cmaster.matso: Loading trigger script is ready, but not tested yet.

----
Modified : /qcommon/common.c
Modified : /server/sv_main.c
Modified : /splines/splines.cpp
Modified : /splines/splines.h
Modified : /splines/triggers.h

Revision: 24
Author: kadlubom
Date: Monday, July 30, 2012 11:10:25 AM
Message:
cmaster.matso: Removed some minor bug related with the camera triggers.

----
Modified : /splines/splines.cpp
Modified : /splines/splines.h
Modified : /splines/triggers.h

Revision: 23
Author: kadlubom
Date: Monday, July 30, 2012 4:52:58 AM
Message:
cmaster.matso: Need to implement loading of 5 triggers parameters from CFG file and work about them is done.

----
Modified : /splines/splines.cpp
Modified : /splines/splines.h
Modified : /splines/triggers.h

Revision: 22
Author: kadlubom
Date: Monday, July 30, 2012 4:11:28 AM
Message:
cmaster.matso: Need to implement loading of 5 triggers parameters from CFG file and work about them is done.

----
Modified : /splines/splines.h
Modified : /splines/triggers.cpp
Modified : /splines/triggers.h

Revision: 21
Author: kadlubom
Date: Monday, July 30, 2012 4:03:40 AM
Message:
cmaster.matso: Need to implement loading of 5 triggers parameters from CFG file and work about them is done.

----
Modified : /client/cl_main.c
Modified : /splines/splines.cpp
Modified : /splines/splines.h

Revision: 20
Author: kadlubom
Date: Monday, July 30, 2012 3:25:13 AM
Message:
cmaster.matso: Added camera triggers in simplest of forms - one thing to do is to add triggers loading from a CFG script.

----
Modified : /client/ref.h
Modified : /renderer/r_main.c
Modified : /splines/Splines.vcproj
Modified : /splines/splines.cpp
Modified : /splines/splines.h
Added : /splines/triggers.cpp
Added : /splines/triggers.h

Revision: 19
Author: kadlubom
Date: Tuesday, July 24, 2012 7:22:24 AM
Message:
cmaster.matso: Small changes to the camera modes code - moved stuff to a routine for clearer code.

----
Modified : /renderer/r_main.c

Revision: 18
Author: corbin
Date: Monday, July 16, 2012 6:16:05 PM
Message:
Fixed g_target.c in gamecode; target_command_use (edict_t *self, edict_t *activator, edict_t *other) to send message to player rather than itself, should avoid crashes and fix the function. 
----
Modified : /game/g_target.c

Revision: 17
Author: kadlubom
Date: Tuesday, July 10, 2012 3:00:35 AM
Message:
cmaster.matso: Tracking camera has now following mode, but it is buggy (still working on it).

----
Modified : /client/cl_main.c
Modified : /splines/splines.cpp
Modified : /splines/splines.h

Revision: 16
Author: kadlubom
Date: Wednesday, July 4, 2012 11:33:08 PM
Message:
cmaster.matso: Tracking camera code is ready and working like a devil.

----
Modified : /client/cl_ents.c
Modified : /client/cl_main.c
Modified : /client/ref.h
Modified : /renderer/r_main.c
Modified : /splines/splines.cpp
Modified : /splines/splines.h

Revision: 15
Author: kadlubom
Date: Tuesday, July 3, 2012 11:52:33 PM
Message:
cmaster.matso: Tracking camera code is ready - need only to get player position.

----
Modified : /renderer/r_main.c
Modified : /splines/splines.cpp

Revision: 14
Author: kadlubom
Date: Saturday, June 23, 2012 12:52:44 AM
Message:
cmaster.matso: Added code for tracking camera, but it is only beginning.

----
Modified : /client/ref.h
Modified : /renderer/r_main.c
Modified : /splines/splines.cpp
Modified : /splines/splines.h

Revision: 13
Author: kadlubom
Date: Friday, June 22, 2012 2:37:19 AM
Message:
cmaster.matso: Activating scripted camera causes the player to switch to thirdperson view.

----
Modified : /client/cl_main.c
Modified : /client/client.h
Modified : /client/ref.h
Modified : /quake2.vcproj
Modified : /renderer/r_main.c

Revision: 12
Author: corbin
Date: Friday, June 22, 2012 1:58:30 AM
Message:
fixing an image loading memory leak, in JPEG loading code
----
Modified : /renderer/r_image.c

Revision: 11
Author: kadlubom
Date: Tuesday, June 19, 2012 3:59:03 AM
Message:
cmaster.matso: Now scripted camera is not blocked by the console.

----
Modified : /client/cl_main.c
Modified : /renderer/r_main.c
Modified : /splines/splines.cpp
Modified : /splines/splines.h

Revision: 10
Author: kadlubom
Date: Tuesday, June 19, 2012 2:29:09 AM
Message:
cmaster.matso: Scripted camera new functionality - switching between multiple active cameras implemented.

----
Modified : /client/cl_main.c
Modified : /splines/splines.cpp

Revision: 9
Author: kadlubom
Date: Tuesday, June 19, 2012 2:13:35 AM
Message:
cmaster.matso: Scripted camera new functionality - added multiple cameras management. Up to 16 cameras available.

----
Modified : /client/cl_main.c
Modified : /client/ref.h
Modified : /renderer/r_main.c
Modified : /splines/splines.cpp

Revision: 8
Author: kadlubom
Date: Friday, June 15, 2012 6:56:07 AM
Message:
cmaster.matso: Testing and debugging the scripted camera - fighting with the engine...

----
Modified : /renderer/r_main.c
Modified : /splines/splines.cpp

Revision: 7
Author: kadlubom
Date: Thursday, June 14, 2012 2:43:34 PM
Message:
cmaster.matso: Testing and debugging the scripted camera - getting close...

----
Modified : /client/cl_main.c
Modified : /client/ref.h
Modified : /renderer/r_main.c
Modified : /splines/splines.cpp

Revision: 6
Author: kadlubom
Date: Wednesday, June 13, 2012 11:59:02 PM
Message:
cmaster.matso: Made the code compile - need to populate againg any changes made recently concerning the renderer's functionality.

----
Modified : /client/cl_main.c
Modified : /client/ref.h
Modified : /game/q_shared.h
Modified : /quake2.vcproj
Modified : /renderer/r_image.c
Modified : /renderer/r_light.c
Modified : /renderer/r_local.h
Modified : /renderer/r_main.c
Modified : /renderer/r_surface.c
Modified : /server/sv_user.c

Revision: 5
Author: kadlubom
Date: Tuesday, May 29, 2012 1:16:15 AM
Message:
cMaster: There are some problems with the code. Did anyone compile it with success? There are definitions/declarations/structure members missing which cause about 500 compilation errors.
I changed the solution file, for it was trying to load the source from a wrong directory. Also added splines project file to the solution.

----
Modified : /scourge.sln
Modified : /splines/Splines.vcproj

Revision: 4
Author: kadlubom
Date: Thursday, May 10, 2012 11:51:11 AM
Message:
cMaster: Added code for loading and starting a scripted camera - testing needed.

----
Modified : /client/cl_main.c
Modified : /client/ref.h
Modified : /game/q_shared.h
Modified : /renderer/r_main.c

