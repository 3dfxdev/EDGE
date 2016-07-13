cmake_minimum_required(VERSION 3.5)
project(3DGE)

# MSVC
if(MSVC)
	add_definitions(-D_CRT_SECURE_NO_WARNINGS)
endif()

add_definitions(
	-DDEH_EDGE_PLUGIN
)

# Module Paths
list(APPEND CMAKE_MODULE_PATH
	${CMAKE_CURRENT_SOURCE_DIR}
	${CMAKE_CURRENT_SOURCE_DIR}/modules
)

# SDL
find_package(SDL2 REQUIRED)
include_directories(${SDL2_INCLUDE_DIR})

# SDL_net
find_package(SDL2_net REQUIRED)
include_directories(${SDL2_NET_INCLUDE_DIR})

# GLEW
find_package(GLEW REQUIRED)
include_directories(${GLEW_INCLUDE_DIRS})

# libvorbis/libogg
find_package(OGG REQUIRED)
find_package(Vorbis REQUIRED)
include_directories(
	${OGG_INCLUDE_DIR}
	${VORBIS_INCLUDE_DIR}
)

# zlib
find_package(ZLIB REQUIRED)
include_directories(${ZLIB_INCLUDE_DIRS})

# libcpuid
find_package(cpuid REQUIRED)
include_directories(${CPUID_INCLUDE_DIR})

# png
find_package(PNG REQUIRED)
include_directories(${PNG_INCLUDE_DIRS})

# jpeg
find_package(JPEG REQUIRED)
include_directories(${JPEG_INCLUDE_DIR})

# 3DGE
include_directories(${CMAKE_CURRENT_SOURCE_DIR})
set(3DGE_SOURCES
	coal/c_compile.cc
	coal/c_execute.cc
	coal/c_memory.cc
	
	ddf/anim.cc
	ddf/attack.cc
	ddf/boom.cc
	ddf/colormap.cc
	ddf/font.cc
	ddf/game.cc
	ddf/image.cc
	ddf/language.cc
	ddf/level.cc
	ddf/line.cc
	ddf/main.cc
	ddf/playlist.cc
	ddf/sector.cc
	ddf/sfx.cc
	ddf/states.cc
	ddf/style.cc
	ddf/switch.cc
	ddf/thing.cc
	ddf/weapon.cc
	
	deh_edge/ammo.cc
	deh_edge/attacks.cc
	deh_edge/buffer.cc
	deh_edge/convert.cc
	deh_edge/frames.cc
	deh_edge/info.cc
	deh_edge/main.cc
	deh_edge/system.cc
	deh_edge/util.cc
	deh_edge/wad.cc
	deh_edge/mobj.cc
	deh_edge/sounds.cc
	deh_edge/things.cc
	deh_edge/weapons.cc
	deh_edge/misc.cc
	deh_edge/text.cc
	deh_edge/storage.cc
	deh_edge/patch.cc
	deh_edge/rscript.cc

	src/i_main.cc
	src/i_ctrl.cc
	src/i_video.cc
	src/i_sound.cc
	src/i_net.cc
	src/am_map.cc
	src/con_con.cc
	src/con_main.cc
	src/con_link.cc
	src/con_var.cc
	src/e_input.cc
	src/e_main.cc
	src/e_player.cc
	src/f_finale.cc
	src/f_interm.cc
	src/e_splash.cc
	src/g_game.cc
	src/hu_draw.cc
	src/hu_font.cc
	src/hu_stuff.cc
	src/hu_style.cc
	src/l_glbsp.cc
	src/l_deh.cc
	src/m_argv.cc
	src/m_bbox.cc
	src/m_cheat.cc
	src/m_math.cc
	src/m_menu.cc
	src/m_misc.cc
	src/m_option.cc
	src/m_netgame.cc
	src/m_random.cc
	src/m_shift.cc
	src/n_bcast.cc
	src/n_reliable.cc
	src/n_network.cc
	src/p_action.cc
	src/p_blockmap.cc
	src/p_bot.cc
	src/p_enemy.cc
	src/p_inter.cc
	src/p_lights.cc
	src/p_map.cc
	src/p_maputl.cc
	src/p_mobj.cc
	src/p_plane.cc
	src/p_setup.cc
	src/p_sight.cc
	src/p_spec.cc
	src/p_switch.cc
	src/p_tick.cc
	src/p_user.cc
	src/p_forces.cc
	src/p_telept.cc
	src/p_weapon.cc
	src/rad_act.cc
	src/rad_pars.cc
	src/rad_trig.cc
	src/r_draw.cc
	src/r_shader.cc
	src/r_render.cc
	src/r_effects.cc
	src/r_main.cc
	src/r_occlude.cc
	src/m_logo.cc
	src/r_things.cc
	src/r_units.cc
	src/r_wipe.cc
	src/r_misc.cc
	src/r_sky.cc
	src/r_colormap.cc
	src/r_modes.cc
	src/r_md2.cc
	src/r_md5.cc
	src/r_graph.cc 
	src/r_image.cc
	src/r_doomtex.cc
	src/r_texgl.cc
	src/r_bumpmap.cc
	src/r_playpal.cc
	src/s_blit.cc
	src/s_cache.cc
	src/s_sound.cc
	src/s_music.cc
	src/s_ogg.cc
	src/s_opl.cc
	src/s_timid.cc
	src/sv_chunk.cc
	src/sv_glob.cc
	src/sv_level.cc
	src/sv_load.cc
	src/sv_main.cc
	src/sv_misc.cc
	src/sv_mobj.cc
	src/sv_play.cc
	src/sv_save.cc
	src/tinybsp.cc
	src/w_flat.cc
	src/w_model.cc
	src/w_sprite.cc
	src/w_texture.cc
	src/w_wad.cc
	src/z_zone.cc
	src/vm_coal.cc
	src/vm_hud.cc
	src/vm_player.cc
	src/wlf_util.cc
	src/wlf_vswap.cc
	src/wlf_maps.cc
	src/wlf_setup.cc
	
	md5_conv/md5_load.cc
	md5_conv/md5_parse.cc
	md5_conv/md5_anim.cc
	md5_conv/md5_draw.cc
	md5_conv/kmq2/q2stuff.c
	
	epi/arrays.cc
	epi/bytearray.cc
	epi/exe_path.cc
	epi/file.cc
	epi/filesystem.cc
	epi/file_memory.cc
	epi/file_sub.cc
	epi/file_grp.cc
	epi/file_pak.cc
	epi/file_zip.cc
	epi/image_data.cc
	epi/image_hq2x.cc
	epi/image_jpeg.cc
	epi/image_png.cc
	epi/image_tga.cc
	epi/math_angle.cc
	epi/math_bbox.cc
	epi/math_crc.cc
	epi/math_md5.cc
	epi/math_oddity.cc
	epi/math_matrix.cc
	epi/math_vector.cc
	epi/math_quaternion.cc
	epi/math_random.cc
	epi/mersenne_twist.cc
	epi/mus_2_midi.cc
	epi/path.cc
	epi/str_format.cc
	epi/sound_data.cc
	epi/sound_gather.cc
	epi/sound_wav.cc
	epi/timestamp.cc
	epi/utility.cc
	epi/epi_win32.cc
	epi/filesystem_win32.cc
	
	glbsp/src/analyze.cc
	glbsp/src/blockmap.cc
	glbsp/src/glbsp.cc
	glbsp/src/level.cc
	glbsp/src/node.cc
	glbsp/src/reject.cc
	glbsp/src/seg.cc
	glbsp/src/system.cc
	glbsp/src/util.cc
	glbsp/src/wad.cc
	
	timidity/common.cc
	timidity/instrum.cc
	timidity/mix.cc
	timidity/playmidi.cc
	timidity/loadmidi.cc
	timidity/resample.cc
	timidity/tables.cc
	timidity/timidity.cc

	opllib/opl3.cc
	opllib/oplapi.cc

	
	
)

set(3DGE_WIN32_SOURCES
	src/w32_music.cc
	src/w32_mus.cc
	src/w32_net.cc
	src/w32_system.cc
	src/w32_res.rc
)

set(3DGE_UNIX_SOURCES
	src/unx_main.cc
	src/unx_music.cc
	src/unx_net.cc
	src/unx_system.cc
)

set(3DGE_DREAMCAST_SOURCES
	src/dc_music.cc
	src/dc_net.cc
	src/dc_system.cc
	src/i_videodc.cc
)

if(WIN32)
	set(3DGE_SOURCES ${3DGE_SOURCES}
		${3DGE_WIN32_SOURCES}
	)
elseif(UNIX)
	set(3DGE_SOURCES ${3DGE_SOURCES}
		${3DGE_UNIX_SOURCES}
	)
elseif(DREAMCAST)
	set(3DGE_SOURCES ${3DGE_SOURCES}
		${3DGE_DREAMCAST_SOURCES}
	)
endif()

add_executable(3DGE
	${3DGE_SOURCES}
)

target_link_libraries(3DGE
	${SDL2_LIBRARY}
	${SDL2MAIN_LIBRARY}
	${SDL2_NET_LIBRARY}
	${OGG_LIBRARIES}
	${VORBIS_LIBRARIES}
	${CPUID_LIBRARY}
	${ZLIB_LIBRARIES}
	${GLEW_LIBRARIES}
	${PNG_LIBRARIES}
	${JPEG_LIBRARIES}
	opengl32
)

if (WIN32)
	target_link_libraries(3DGE
		wsock32
		winmm
		gdi32
		dxguid
	)
endif()
