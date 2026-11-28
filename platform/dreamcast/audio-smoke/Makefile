# DreamPSX KallistiOS cross-build: complete emulator core + audio smoke app
TARGET = dreampsx-audio-smoke.elf
BOOT_TARGET = dreampsx-boot-smoke.elf

CORE_SOURCES = \
    ../../../src/bus/bus.c \
    ../../../src/bus/cdrom.c \
    ../../../src/bus/dma.c \
    ../../../src/bus/irq.c \
    ../../../src/bus/irq_mmio.c \
    ../../../src/bus/memory.c \
    ../../../src/bus/spu.c \
    ../../../src/bus/sio.c \
    ../../../src/audio/spu_adpcm.c \
    ../../../src/audio/audio_buffer.c \
    ../../../src/audio/dreamcast_audio_stream.c \
    ../../../src/cpu/cop0.c \
    ../../../src/cpu/cop0_irq.c \
    ../../../src/cpu/cpu.c \
    ../../../src/gte/gte.c \
    ../../../src/gpu/gpu.c \
    ../../../src/media/iso9660.c \
    ../../../src/media/disc_image.c \
    ../../../src/media/xa_adpcm.c \
    ../../../src/media/boot_config.c \
    ../../../src/media/psx_exe.c \
    ../../../src/machine/machine.c \
    ../../../src/system/timers.c \
    ../../../src/platform/dreamcast/kos_audio_backend.c \
    ../../../src/platform/dreamcast/kos_disc_file.c \
    ../../../src/platform/dreamcast/kos_video_backend.c \
    ../../../src/platform/dreamcast/kos_input_backend.c

CORE_OBJECTS = $(CORE_SOURCES:.c=.o)

CFLAGS += -I../../../include

all: libdreampsx_core.a $(TARGET) $(BOOT_TARGET)

include $(KOS_BASE)/Makefile.rules

libdreampsx_core.a: $(CORE_OBJECTS)
	kos-ar rcs $@ $(CORE_OBJECTS)

$(TARGET): main.o libdreampsx_core.a
	kos-cc -o $@ main.o libdreampsx_core.a

$(BOOT_TARGET): boot_smoke.o libdreampsx_core.a
	kos-cc -o $@ boot_smoke.o libdreampsx_core.a

clean:
	-rm -f $(TARGET) $(BOOT_TARGET) main.o boot_smoke.o libdreampsx_core.a $(CORE_OBJECTS)

dist: $(TARGET)
	$(KOS_STRIP) $(TARGET)

.PHONY: all clean dist
