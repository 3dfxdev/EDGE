#
# SConstruct file for EDGE
#
import os

SConscript('epi/SConscript')
SConscript('deh_edge/SConscript.plugin')
SConscript('glbsp/SConscript.plugin')
SConscript('lzo/SConscript')
# SConscript('humidity/SConscript.plugin')

env = Environment()

# warnings
env.Append(CCFLAGS = ['-Wall'])

# optimisation
env.Append(CCFLAGS = ['-O', '-g3', '-ffast-math'])

#----- LIBRARIES ----------------------------------

# epi
# env.Append(CPPPATH = ['../epi'])
env.Append(LIBPATH = ['./epi'])
env.Append(LIBS = ['epi'])

# GLBSP
env.Append(LIBPATH = ['./glbsp'])
env.Append(LIBS = ['glbsp'])

# Deh_Edge
env.Append(LIBPATH = ['./deh_edge'])
env.Append(LIBS = ['dehedge'])

# LZO
env.Append(LIBPATH = ['./lzo'])
env.Append(LIBS = ['lzo'])

# JPEG, PNG and ZLIB
env.Append(LIBS = ['png', 'jpeg', 'z'])

# SDL
env.ParseConfig('sdl-config --cflags --libs')

# OpenGL
env.Append(LIBS = ['GL'])

# OpenAL
env.Append(LIBS = ['openal'])

# Ogg/Vorbis
env.Append(LIBS = ['ogg', 'vorbis', 'vorbisfile'])

#------- OBJECTS -----------------------------------

source_list = [
 'l_glbsp.cpp',
 'SDL/i_cd.cpp', 'SDL/i_ctrl.cpp', 'SDL/i_loop.cpp', 'SDL/i_video.cpp',
 'mus_2_midi.cpp', 'oggplayer.cpp', 
 'am_map.cpp', 'con_con.cpp', 'con_cvar.cpp', 'con_main.cpp', 'ddf_anim.cpp',
 'ddf_atk.cpp', 'ddf_boom.cpp', 'ddf_colm.cpp', 'ddf_font.cpp', 
 'ddf_game.cpp', 'ddf_image.cpp', 'ddf_lang.cpp', 'ddf_levl.cpp', 
 'ddf_line.cpp', 'ddf_main.cpp', 'ddf_mobj.cpp', 'ddf_mus.cpp', 
 'ddf_sect.cpp', 'ddf_sfx.cpp', 'ddf_stat.cpp', 'ddf_style.cpp', 
 'ddf_swth.cpp', 'ddf_weap.cpp', 'dem_chunk.cpp', 'dem_glob.cpp', 
 'dm_defs.cpp', 'dm_state.cpp', 'e_demo.cpp', 'e_input.cpp', 'e_main.cpp', 
 'e_player.cpp', 'f_finale.cpp', 'g_game.cpp', 'gui_ctls.cpp', 'gui_main.cpp', 
 'hu_font.cpp', 'hu_lib.cpp', 'hu_stuff.cpp', 'hu_style.cpp', 'i_asm.cpp', 
 'l_deh.cpp', 'lu_gamma.cpp', 'm_argv.cpp', 'm_bbox.cpp', 
 'm_cheat.cpp', 'm_math.cpp', 'm_menu.cpp', 'm_misc.cpp', 'm_option.cpp', 
 'm_random.cpp', 'n_network.cpp', 'n_packet.cpp', 'n_proto.cpp', 
 'p_action.cpp', 'p_bot.cpp', 'p_enemy.cpp', 'p_inter.cpp', 'p_lights.cpp', 
 'p_map.cpp', 'p_maputl.cpp', 'p_mobj.cpp', 'p_plane.cpp', 'p_setup.cpp', 
 'p_sight.cpp', 'p_spec.cpp', 'p_switch.cpp', 'p_tick.cpp', 'p_user.cpp', 
 'p_forces.cpp', 'p_weapon.cpp',
 'r2_util.cpp', 'rad_act.cpp', 'rad_pars.cpp', 'rad_trig.cpp', 
 'r_bsp.cpp', 'r_data.cpp', 'rgl_bsp.cpp', 'rgl_fx.cpp', 'rgl_main.cpp', 
 'rgl_occ.cpp', 'rgl_sky.cpp', 'rgl_tex.cpp', 'rgl_thing.cpp', 'rgl_unit.cpp', 
 'rgl_wipe.cpp', 'r_layers.cpp', 'r_main.cpp', 'r_sky.cpp', 'r_things.cpp', 
 'r_vbinit.cpp', 'r_view.cpp', 's_music.cpp', 's_sound.cpp', 'st_lib.cpp', 
 'st_stuff.cpp', 'sv_chunk.cpp', 'sv_dump.cpp', 'sv_glob.cpp', 'sv_level.cpp', 
 'sv_load.cpp', 'sv_main.cpp', 'sv_misc.cpp', 'sv_mobj.cpp', 'sv_play.cpp', 
 'sv_save.cpp', 'v_colour.cpp', 'v_res.cpp', 'w_image.cpp', 'wi_stuff.cpp', 
 'wp_wipe.cpp', 'w_textur.cpp', 'w_wad.cpp', 'z_zone.cpp', 
 ]

# operating system specifics
if os.name == "nt":
	env.Append(CCFLAGS = '-DWIN32')
	source_list += ['win32/i_cd.cpp', 'win32/i_compen.cpp',
		'win32/i_main.cpp', 'win32/i_music.cpp',
		'win32/i_mus.cpp', 'win32/i_net.cpp',
		'win32/i_sound.cpp', 'win32/i_system.cpp']

elif os.name == "posix":
	env.Append(CCFLAGS = '-DLINUX')
	source_list += ['linux/i_main.cpp', 'linux/i_compen.cpp', 'linux/i_music.cpp', 
		 'linux/i_net.cpp', 'linux/i_sound.cpp', 'linux/i_system.cpp']
	# source_list += ['humdinger.cpp']

else:
	print 'Unknown OS type: ' + os.name
	Exit(1)

env.Program('gledge', source_list)

