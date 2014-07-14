
TARGET = demo.elf
OBJS = minigl.o minigl_submit.o minigl_tex.o minigl_texconv.o minigl_prim.o minigl_mat.o minigl_state.o watchdog.o profile.o nehe16.o romdisk.o

		
KOS_ROMDISK_DIR = romdisk

all: rm-elf $(TARGET)

#NODEBUGING = -D NDEBUG
NODEBUGING = 
#EXTRAOPT=-Wall -O3 -fomit-frame-pointer -Wstrict-aliasing=2 -D __arch_dreamcast
EXTRAOPT=-Wall -O3 -mpretend-cmove -mrelax -mfused-madd -fomit-frame-pointer -Wstrict-aliasing=2 -D __arch_dreamcast -Wno-format
#EXTRAOPT=-Wall -O3 -mpretend-cmove -mrelax -mfused-madd -Wstrict-aliasing=2 -D __arch_dreamcast -Wno-format
MATHOPT=-ffast-math
#MATHOPT=
ALIGNS=-falign-loops=4 -falign-labels=4 -falign-functions=32 -falign-jumps=4


%.o: %.c
	kos-cc $(CFLAGS) $(NODEBUGING) -O3 -fomit-frame-pointer $(MATHOPT) $(EXTRAOPT) $(ALIGNS) -c $< -o $@
#kos-cc $(CFLAGS) $(NODEBUGING) -O3 $(MATHOPT) $(EXTRAOPT) $(ALIGNS) -c $< -o $@

%.o: %.cpp
	kos-c++ $(CFLAGS) $(NODEBUGING) -fno-exceptions -fno-rtti $(MATHOPT)  $(EXTRAOPT) $(CXXFLAGS) $(ALIGNS) -c $< -o $@

%.o: %.s
	kos-as $< -o $@

%.o: %.S
	kos-cc -c $(NODEBUGING) $< -o $@

	

subdirs: $(patsubst %, _dir_%, $(SUBDIRS))

$(patsubst %, _dir_%, $(SUBDIRS)):
	$(MAKE) -C $(patsubst _dir_%, %, $@)

clean_subdirs: $(patsubst %, _clean_dir_%, $(SUBDIRS))

$(patsubst %, _clean_dir_%, $(SUBDIRS)):
	$(MAKE) -C $(patsubst _clean_dir_%, %, $@) clean

# Define KOS_ROMDISK_DIR in your Makefile if you want these two handy rules.
ifdef KOS_ROMDISK_DIR
romdisk.img:
	$(KOS_GENROMFS) -f romdisk.img -d $(KOS_ROMDISK_DIR) -v -x .svn

romdisk.o: romdisk.img
	$(KOS_BASE)/utils/bin2o/bin2o romdisk.img romdisk romdisk.o
endif
######################################

clean:
	rm -f $(TARGET) $(OBJS)

rm-elf:
	rm -f $(TARGET)


$(TARGET): $(OBJS)
	$(KOS_CC) $(KOS_CFLAGS) $(KOS_LDFLAGS) -o $(TARGET) $(KOS_START) \
		$(OBJS) $(OBJEXTRA)  -lstdc++ -lm -lpcx -lkosutils $(KOS_LIBS)

#		$(OBJS) $(OBJEXTRA)  -lstdc++ -lm -lkglx -lpcx -lkosutils $(KOS_LIBS)

run: $(TARGET)
	$(KOS_LOADER) $(TARGET) -n

dist:
	rm -f $(OBJS)
	$(KOS_STRIP) $(TARGET)