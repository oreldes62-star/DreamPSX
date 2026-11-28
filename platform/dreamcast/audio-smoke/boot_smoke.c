#include <kos.h>

#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>

#include "dreampsx/machine.h"
#include "dreampsx/kos_disc_file.h"
#include "dreampsx/kos_audio_backend.h"
#include "dreampsx/kos_video_backend.h"
#include "dreampsx/kos_input_backend.h"
#include "dreampsx/dreamcast_audio_stream.h"

#define PSX_BIOS_PATH "/cd/PSX/BIOS.BIN"
#define PSX_CUE_PATH  "/cd/PSX/GAME.CUE"
#define PS1_CYCLES_PER_MILLISECOND 33869u
#define PS1_RAM_VRAM_PIXELS (1024u * 512u)

static PsxMachine machine;
static PsxKosDiscFile disc_file;
static PsxDiscImage disc;
static PsxDreamcastAudioStream audio_stream;
static PsxKosAudioBackend audio_backend;
static PsxKosVideoBackend video_backend;
static PsxKosInputBackend input_backend;

static uint8_t psx_ram[PSX_RAM_SIZE];
static uint8_t psx_bios[PSX_BIOS_SIZE];
static uint16_t psx_vram[PS1_RAM_VRAM_PIXELS];

static bool load_bios(const char *path, uint8_t *destination, size_t expected_size)
{
    file_t handle;
    size_t total = 0u;
    size_t file_size;

    handle = fs_open(path, O_RDONLY);
    if (handle == FILEHND_INVALID) {
        printf("ERROR: cannot open PS1 BIOS: %s\n", path);
        return false;
    }

    file_size = fs_total(handle);
    if (file_size != expected_size) {
        printf("ERROR: BIOS size is %lu bytes; expected %lu\n",
               (unsigned long)file_size, (unsigned long)expected_size);
        fs_close(handle);
        return false;
    }

    while (total < expected_size) {
        ssize_t count = fs_read(handle, destination + total,
                                expected_size - total);
        if (count <= 0 || (size_t)count > expected_size - total) {
            printf("ERROR: short read while loading BIOS\n");
            fs_close(handle);
            return false;
        }
        total += (size_t)count;
    }

    fs_close(handle);
    return true;
}

static void shutdown_audio(void)
{
    psx_kos_audio_backend_stop(&audio_backend);
}

int main(int argc, char **argv)
{
    PsxBootConfig boot_config;
    uint32_t report_counter = 0u;
    uint32_t video_frame_divider = 0u;

    (void)argc;
    (void)argv;

    dbgio_dev_select("serial");
    printf("DreamPSX file-backed boot smoke test\n");
    printf("Expected files:\n  %s\n  %s and its relative BIN file\n",
           PSX_BIOS_PATH, PSX_CUE_PATH);

    if (!load_bios(PSX_BIOS_PATH, psx_bios, sizeof(psx_bios)))
        return 1;

    if (!psx_machine_init(&machine, psx_ram, sizeof(psx_ram),
                          psx_bios, sizeof(psx_bios),
                          psx_vram, PS1_RAM_VRAM_PIXELS)) {
        printf("ERROR: PS1 machine initialization failed\n");
        return 1;
    }

    if (!psx_kos_disc_file_open_cue(&disc_file, &disc, PSX_CUE_PATH)) {
        printf("ERROR: cannot open CUE or its referenced BIN/ISO\n");
        return 1;
    }

    if (!psx_machine_boot_from_disc_image(&machine, &disc, &boot_config)) {
        printf("ERROR: ISO9660 / SYSTEM.CNF / PS-X EXE boot failed\n");
        psx_kos_disc_file_close(&disc_file, &disc);
        return 1;
    }

    printf("Disc mounted through KOS VFS random-access reader\n");
    printf("SYSTEM.CNF selected: %s\n", boot_config.boot_path);
    printf("PS-X EXE entry: 0x%08lx\n", (unsigned long)machine.cpu.pc);
    printf("Beginning PS1 instruction execution\n");

    if (!psx_dreamcast_audio_stream_init(&audio_stream,
                                         &machine.audio_output) ||
        !psx_kos_audio_backend_start(&audio_backend, &audio_stream)) {
        printf("ERROR: AICA audio stream initialization failed\n");
        psx_kos_disc_file_close(&disc_file, &disc);
        return 1;
    }

    if (!psx_kos_video_backend_start(&video_backend)) {
        printf("ERROR: Dreamcast framebuffer initialization failed\n");
        shutdown_audio();
        psx_kos_disc_file_close(&disc_file, &disc);
        return 1;
    }
    printf("Dreamcast video output: 320x240 RGB565\n");

    /* Execute approximately one millisecond of PS1 master-clock cycles per
       loop. The machine clock schedules SPU samples and CD-ROM events. */
    for (;;) {
        uint32_t elapsed_cycles = 0u;

        while (elapsed_cycles < PS1_CYCLES_PER_MILLISECOND) {
            if (!psx_machine_step(&machine)) {
                printf("ERROR: PS1 CPU execution stopped at 0x%08lx\n",
                       (unsigned long)machine.cpu.pc);
                shutdown_audio();
                psx_kos_disc_file_close(&disc_file, &disc);
                return 1;
            }
            elapsed_cycles += psx_cpu_last_step_cycles(&machine.cpu);
        }

        {
            PsxKosInputState input_state;
            if (!psx_kos_input_backend_poll(&input_backend, &input_state)) {
                printf("ERROR: Maple controller polling failed\\n");
                shutdown_audio();
                psx_kos_disc_file_close(&disc_file, &disc);
                return 1;
            }
            psx_sio_set_pad_state(&machine.sio, input_state.connected,
                                  input_state.buttons);
        }

        if (!psx_kos_audio_backend_poll(&audio_backend)) {
            printf("ERROR: AICA stream polling failed\n");
            shutdown_audio();
            psx_kos_disc_file_close(&disc_file, &disc);
            return 1;
        }

        if (++video_frame_divider >= 16u) {
            PsxGpuDisplay display;
            video_frame_divider = 0u;
            if (psx_gpu_get_display(&machine.gpu, &display) &&
                !display.disabled &&
                !psx_kos_video_backend_present(&video_backend, &machine.gpu)) {
                printf("ERROR: GPU framebuffer presentation failed\n");
                shutdown_audio();
                psx_kos_disc_file_close(&disc_file, &disc);
                return 1;
            }
        }

        if (++report_counter >= 1000u) {
            printf("RUNNING: PC=0x%08lx audio-callbacks=%llu\n",
                   (unsigned long)machine.cpu.pc,
                   (unsigned long long)audio_stream.callback_count);
            report_counter = 0u;
        }
        thd_sleep(1);
    }
}
