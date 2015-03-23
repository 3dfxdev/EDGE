OBJDIR=../obj_win32

EPI_OBJS= \
	$(OBJDIR)/epi/arrays.o       \
	$(OBJDIR)/epi/bytearray.o    \
	$(OBJDIR)/epi/exe_path.o     \
	$(OBJDIR)/epi/file.o         \
	$(OBJDIR)/epi/filesystem.o   \
	$(OBJDIR)/epi/file_memory.o  \
	$(OBJDIR)/epi/file_sub.o     \
	$(OBJDIR)/epi/image_data.o   \
	$(OBJDIR)/epi/image_hq2x.o   \
	$(OBJDIR)/epi/image_jpeg.o   \
	$(OBJDIR)/epi/image_png.o    \
	$(OBJDIR)/epi/image_tga.o    \
	$(OBJDIR)/epi/math_angle.o   \
	$(OBJDIR)/epi/math_bbox.o    \
	$(OBJDIR)/epi/math_color.o   \
	$(OBJDIR)/epi/math_crc.o     \
	$(OBJDIR)/epi/math_md5.o     \
	$(OBJDIR)/epi/math_oddity.o  \
	$(OBJDIR)/epi/math_matrix.o  \
	$(OBJDIR)/epi/math_vector.o  \
	$(OBJDIR)/epi/math_quaternion.o  \
	$(OBJDIR)/epi/math_random.o  \
	$(OBJDIR)/epi/mersenne_twist.o \
	$(OBJDIR)/epi/mus_2_midi.o   \
	$(OBJDIR)/epi/path.o         \
	$(OBJDIR)/epi/str_format.o   \
	$(OBJDIR)/epi/sound_data.o   \
	$(OBJDIR)/epi/sound_gather.o \
	$(OBJDIR)/epi/sound_wav.o    \
	$(OBJDIR)/epi/timestamp.o    \
	$(OBJDIR)/epi/utility.o      \
	$(OBJDIR)/epi/epi_win32.o    \
	$(OBJDIR)/epi/filesystem_win32.o

TARGET = a.exe
OBJS = kmq2/hunk.o kmq2/byteorder.o kmq2/q2stuff.o md5_parse.o md5_load.o md5_anim.o md5_draw.o png.o debug.o main.o $(EPI_OBJS)
#OBJS = ../commonc.o md2.o md3.o main.o

EPILIBS = ../lib_win32/libpng-1.2.12/libpng.a ../lib_win32/jpeg-6b/libjpeg.a ../lib_win32/zlib-1.2.3/libz.a

NODEBUGING = 
EXTRAOPT=-Wall -O3 -msse2 -Werror -I.. -I/usr/include/opengl
#EXTRAOPT=
MATHOPT=-ffast-math
#MATHOPT=
ALIGNS=-falign-loops=4 -falign-labels=4 -falign-functions=4 -falign-jumps=4
ALIGNS=

#CFLAGS  += -I$lib_win32/libpng-1.2.12

%.o: %.c
	gcc $(CFLAGS) $(NODEBUGING) -O3 $(MATHOPT) $(EXTRAOPT) $(ALIGNS) -c $< -o $@

%.o: %.cc
	gcc $(CFLAGS) $(NODEBUGING) -D __CPLUSPLUS $(MATHOPT)  $(EXTRAOPT) $(CXXFLAGS) $(ALIGNS) -c $< -o $@
	
%.o: %.cpp
	gcc $(CFLAGS) $(NODEBUGING) -D __CPLUSPLUS $(MATHOPT)  $(EXTRAOPT) $(CXXFLAGS) $(ALIGNS) -c $< -o $@


#$ gcc -L/lib/w32api/ -I/usr/local/include/ -mno-cygwin -Wall -O3 render.c platformer.c -lSDL -lopengl32 -lglu32 -lglut32 -lSDL_image -lSDL_gfx && ./a.exe
$(OBJDIR)/epi/%.o: ../epi/%.cc
	$(CXX) $(CFLAGS) -o $@ -c $< 

$(TARGET): $(OBJS)
	$(CXX) -g -O3 -static-libgcc -static-libstdc++ -o $(TARGET) \
		$(OBJS) $(EPILIBS) -L/lib/w32api/ -lglu32 -lglut32 -lopengl32

clean:
	rm -f $(TARGET) $(OBJS)
		