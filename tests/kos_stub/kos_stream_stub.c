#include <dc/sound/stream.h>
#include <string.h>

static snd_stream_callback_t current_callback;
static void *current_userdata;
static int initialized, started, stopped, destroyed, shutdown_count;
static int next_poll_result;
static int last_buffer_size, last_frequency, last_stereo;
static int16_t poll_samples[8];
static int poll_received;

int snd_stream_init(void) { initialized++; return 0; }
void snd_stream_shutdown(void) { shutdown_count++; }
snd_stream_hnd_t snd_stream_alloc(snd_stream_callback_t cb, int size) {
    current_callback = cb; last_buffer_size = size; return 7;
}
void snd_stream_set_userdata(snd_stream_hnd_t h, void *d) { (void)h; current_userdata=d; }
void *snd_stream_get_userdata(snd_stream_hnd_t h) { (void)h; return current_userdata; }
void snd_stream_start(snd_stream_hnd_t h, uint32_t f, int st) {
    (void)h; started++; last_frequency=(int)f; last_stereo=st;
}
int snd_stream_poll(snd_stream_hnd_t h) {
    int result; (void)h;
    if (current_callback) {
        void *data=current_callback(7, 2, &poll_received);
        if (data) memcpy(poll_samples, data, (size_t)poll_received*2u*sizeof(int16_t));
    }
    result=next_poll_result; next_poll_result=0; return result;
}
void snd_stream_stop(snd_stream_hnd_t h) { (void)h; stopped++; }
void snd_stream_destroy(snd_stream_hnd_t h) { (void)h; destroyed++; }
void kos_stub_reset(void) {
    current_callback=NULL; current_userdata=NULL; initialized=started=stopped=destroyed=shutdown_count=0;
    next_poll_result=0; last_buffer_size=last_frequency=last_stereo=0; poll_received=0;
    memset(poll_samples,0,sizeof(poll_samples));
}
void kos_stub_set_poll_result(int r) { next_poll_result=r; }
int kos_stub_initialized(void){return initialized;}
int kos_stub_started(void){return started;}
int kos_stub_stopped(void){return stopped;}
int kos_stub_destroyed(void){return destroyed;}
int kos_stub_shutdown_count(void){return shutdown_count;}
int kos_stub_buffer_size(void){return last_buffer_size;}
int kos_stub_frequency(void){return last_frequency;}
int kos_stub_stereo(void){return last_stereo;}
int kos_stub_poll_received(void){return poll_received;}
const int16_t *kos_stub_poll_samples(void){return poll_samples;}
