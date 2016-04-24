#
# LINK STEP (run after: scons release=true)
#

LIBS=	src/libedge1.a \
	ddf/libddf.a \
	timidity/libtimidity.a \
	epi/libepi.a \
	glbsp/libglbsp.a \
	deh_edge/libdehedge.a \
	linux_lib/lua-5.1.3/src/liblua.a \
	linux_lib/libpng-1.2.12/libpng.a \
	linux_lib/jpeg-6b/libjpeg.a \
	linux_lib/zlib-1.2.3/libz.a \
	linux_lib/fltk-1.1.9/lib/libfltk.a \
	linux_lib/fltk-1.1.9/lib/libfltk_images.a \
	-lSDL \
	linux_lib/glew-1.4/lib/libGLEW.a \
	-lGL \
	linux_lib/libvorbis-1.1.2/lib/libvorbisfile.a \
	linux_lib/libvorbis-1.1.2/lib/libvorbis.a \
	linux_lib/libogg-1.1.3/src/libogg.a

gledge32:
	g++ -o $@ -Wl,--warn-common main.o $(LIBS)
	strip --strip-unneeded $@

#--- editor settings ------------
# vi:ts=8:sw=8:noexpandtab
