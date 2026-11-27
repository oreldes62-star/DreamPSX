#include "dreampsx/kos_audio_backend.h"

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <dc/sound/stream.h>

void kos_stub_reset(void);
void kos_stub_set_poll_result(int result);
void kos_stub_set_init_result(int result);
void kos_stub_set_alloc_result(int result);
int kos_stub_initialized(void);
int kos_stub_started(void);
int kos_stub_stopped(void);
int kos_stub_destroyed(void);
int kos_stub_shutdown_count(void);
int kos_stub_buffer_size(void);
int kos_stub_frequency(void);
int kos_stub_stereo(void);
int kos_stub_poll_received(void);
const int16_t *kos_stub_poll_samples(void);

int main(void)
{
    PsxAudioBuffer queue;
    PsxDreamcastAudioStream stream;
    PsxKosAudioBackend backend;
    int16_t pcm[4] = {123, -123, 456, -456};
    const int16_t *received;

    kos_stub_reset();
    assert(psx_audio_buffer_init(&queue));
    assert(psx_dreamcast_audio_stream_init(&stream, &queue));
    assert(psx_audio_buffer_write(&queue, pcm, 2u) == 2u);

    assert(!psx_kos_audio_backend_start(NULL, &stream));
    assert(psx_kos_audio_backend_start(&backend, &stream));
    assert(backend.initialized);
    assert(kos_stub_initialized() == 1);
    assert(kos_stub_started() == 1);
    assert(kos_stub_frequency() == 44100);
    assert(kos_stub_stereo() == 1);
    assert(kos_stub_buffer_size() ==
           (int)(PSX_DREAMCAST_AUDIO_CALLBACK_MAX_FRAMES * sizeof(int16_t)));

    assert(psx_kos_audio_backend_poll(&backend));
    assert(kos_stub_poll_received() == 8);
    received = kos_stub_poll_samples();
    assert(received[0] == 123 && received[1] == -123);
    assert(received[2] == 456 && received[3] == -456);

    kos_stub_set_poll_result(-3);
    assert(!psx_kos_audio_backend_poll(&backend));
    psx_kos_audio_backend_stop(&backend);
    assert(!backend.initialized);
    assert(kos_stub_stopped() == 1);
    assert(kos_stub_destroyed() == 1);
    assert(kos_stub_shutdown_count() == 1);
    assert(!psx_kos_audio_backend_poll(&backend));
    psx_kos_audio_backend_stop(&backend);
    assert(kos_stub_stopped() == 1);

    /* Invalid input must reset stale state before any KOS call. */
    backend.stream_handle = 7;
    backend.stream_system_initialized = true;
    backend.initialized = true;
    assert(!psx_kos_audio_backend_start(&backend, NULL));
    assert(backend.stream_handle == SND_STREAM_INVALID);
    assert(!backend.stream_system_initialized);
    assert(!backend.initialized);
    psx_kos_audio_backend_stop(&backend);
    assert(kos_stub_stopped() == 1);
    assert(kos_stub_destroyed() == 1);
    assert(kos_stub_shutdown_count() == 1);

    /* A failed stream subsystem init must not attempt shutdown. */
    kos_stub_reset();
    kos_stub_set_init_result(-1);
    assert(!psx_kos_audio_backend_start(&backend, &stream));
    assert(!backend.initialized);
    assert(!backend.stream_system_initialized);
    assert(kos_stub_shutdown_count() == 0);

    /* An allocation failure must unwind the initialized stream subsystem. */
    kos_stub_reset();
    kos_stub_set_alloc_result(SND_STREAM_INVALID);
    assert(!psx_kos_audio_backend_start(&backend, &stream));
    assert(!backend.initialized);
    assert(!backend.stream_system_initialized);
    assert(kos_stub_shutdown_count() == 1);
    assert(kos_stub_stopped() == 0);
    assert(kos_stub_destroyed() == 0);

    puts("KallistiOS audio backend lifecycle tests passed");
    return 0;
}
