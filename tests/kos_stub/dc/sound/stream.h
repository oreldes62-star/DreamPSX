#ifndef TEST_KOS_DC_SOUND_STREAM_H
#define TEST_KOS_DC_SOUND_STREAM_H
#include <stddef.h>
#include <stdint.h>
typedef int snd_stream_hnd_t;
#define SND_STREAM_INVALID -1
typedef void *(*snd_stream_callback_t)(snd_stream_hnd_t, int, int *);
int snd_stream_init(void);
void snd_stream_shutdown(void);
snd_stream_hnd_t snd_stream_alloc(snd_stream_callback_t callback, int buffer_size);
void snd_stream_set_userdata(snd_stream_hnd_t handle, void *data);
void *snd_stream_get_userdata(snd_stream_hnd_t handle);
void snd_stream_start(snd_stream_hnd_t handle, uint32_t frequency, int stereo);
int snd_stream_poll(snd_stream_hnd_t handle);
void snd_stream_stop(snd_stream_hnd_t handle);
void snd_stream_destroy(snd_stream_hnd_t handle);
#endif
