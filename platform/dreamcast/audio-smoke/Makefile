# DreamPSX KallistiOS AICA audio smoke test
TARGET = dreampsx-audio-smoke.elf
OBJS = main.o \
       ../../../src/audio/audio_buffer.o \
       ../../../src/audio/dreamcast_audio_stream.o \
       ../../../src/platform/dreamcast/kos_audio_backend.o

CFLAGS += -I../../../include

all: $(TARGET)

include $(KOS_BASE)/Makefile.rules

$(TARGET): $(OBJS)
	kos-cc -o $@ $(OBJS)

clean:
	-rm -f $(TARGET) $(OBJS)

dist: $(TARGET)
	$(KOS_STRIP) $(TARGET)

.PHONY: all clean dist
