#include "dreampsx/cdrom.h"
#include <string.h>

static uint8_t cdrom_status(const PsxCdrom *cdrom)
{
    uint8_t status = (uint8_t)(cdrom->index & 3u);
    if (cdrom->parameter_count == 0u) status |= 1u << 3u;
    if (cdrom->parameter_count < PSX_CDROM_FIFO_CAPACITY)
        status |= 1u << 4u;
    if (cdrom->response_count != 0u) status |= 1u << 5u;
    if ((cdrom->request & 0x80u) != 0u && cdrom->data_count != 0u)
        status |= 1u << 6u;
    if ((cdrom->drive_status & 0x20u) != 0u) status |= 1u << 7u;
    return status;
}

static bool push_response(PsxCdrom *cdrom, uint8_t value)
{
    uint8_t tail;
    if (cdrom->response_count >= PSX_CDROM_FIFO_CAPACITY) return false;
    tail = (uint8_t)((cdrom->response_head + cdrom->response_count) %
                     PSX_CDROM_FIFO_CAPACITY);
    cdrom->responses[tail] = value;
    ++cdrom->response_count;
    return true;
}

static bool pop_parameter(PsxCdrom *cdrom, uint8_t *value)
{
    if (cdrom->parameter_count == 0u) return false;
    *value = cdrom->parameters[cdrom->parameter_head];
    cdrom->parameter_head = (uint8_t)((cdrom->parameter_head + 1u) %
                                      PSX_CDROM_FIFO_CAPACITY);
    --cdrom->parameter_count;
    return true;
}

static void command_complete(PsxCdrom *cdrom, uint8_t interrupt)
{
    if (interrupt >= 1u && interrupt <= 5u)
        cdrom->interrupt_flags |= (uint8_t)(1u << (interrupt - 1u));
}

static bool bcd_valid(uint8_t value, unsigned maximum)
{
    const unsigned high = (unsigned)(value >> 4u);
    const unsigned low = (unsigned)(value & 0x0fu);
    return high <= 9u && low <= 9u && high * 10u + low <= maximum;
}

static void command_error(PsxCdrom *cdrom)
{
    cdrom->drive_status |= 0x01u;
    (void)push_response(cdrom, cdrom->drive_status);
    command_complete(cdrom, 5u);
}


static void reset_xa_audio(PsxCdrom *cdrom)
{
    psx_xa_adpcm_reset(&cdrom->xa_decoder_state[0]);
    psx_xa_adpcm_reset(&cdrom->xa_decoder_state[1]);
    cdrom->xa_pcm_head = 0u;
    cdrom->xa_pcm_count = 0u;
    cdrom->xa_pcm_sample_rate = 0u;
    cdrom->xa_dropped_sectors = 0u;
}

/* Returns true when the sector is consumed by XA, including a bounded-queue drop. */
static bool queue_xa_sector(PsxCdrom *cdrom)
{
    PsxDiscXaAudioFormat format;
    PsxXaAdpcmState next_state[2];
    size_t frames = 0u;
    size_t tail;

    if (!psx_cdrom_get_xa_audio_format(cdrom, &format)) return false;
    if (cdrom->xa_pcm_count >= PSX_CDROM_XA_PCM_QUEUE_FRAMES) {
        ++cdrom->xa_dropped_sectors;
        return true;
    }

    if (format.bits_per_sample == 4u)
        frames = format.channels == 1u ? 4032u : 2016u;
    else if (format.bits_per_sample == 8u)
        frames = format.channels == 1u ? 2016u : 1008u;

    if (frames > PSX_CDROM_XA_PCM_QUEUE_FRAMES - cdrom->xa_pcm_count ||
        (cdrom->xa_pcm_count != 0u &&
         cdrom->xa_pcm_sample_rate != format.sample_rate_hz)) {
        ++cdrom->xa_dropped_sectors;
        return true;
    }

    tail = ((size_t)cdrom->xa_pcm_head + cdrom->xa_pcm_count) %
           PSX_CDROM_XA_PCM_QUEUE_FRAMES;
    next_state[0] = cdrom->xa_decoder_state[0];
    next_state[1] = cdrom->xa_decoder_state[1];
    if (!psx_xa_adpcm_decode_sector(
            cdrom->sector_data, cdrom->data_count, &format, next_state,
            cdrom->xa_pcm_left + tail, cdrom->xa_pcm_right + tail,
            PSX_CDROM_XA_PCM_STORAGE_FRAMES - tail, &frames))
        return false;

    /* The storage arrays include one sector of mirrored tail room, allowing
       a contiguous decode even when the ring write wraps around. */
    if (tail + frames > PSX_CDROM_XA_PCM_QUEUE_FRAMES) {
        size_t wrapped = tail + frames - PSX_CDROM_XA_PCM_QUEUE_FRAMES;
        memcpy(cdrom->xa_pcm_left,
               cdrom->xa_pcm_left + PSX_CDROM_XA_PCM_QUEUE_FRAMES,
               wrapped * sizeof(cdrom->xa_pcm_left[0]));
        memcpy(cdrom->xa_pcm_right,
               cdrom->xa_pcm_right + PSX_CDROM_XA_PCM_QUEUE_FRAMES,
               wrapped * sizeof(cdrom->xa_pcm_right[0]));
    }

    if (cdrom->xa_pcm_count == 0u)
        cdrom->xa_pcm_sample_rate = format.sample_rate_hz;
    cdrom->xa_decoder_state[0] = next_state[0];
    cdrom->xa_decoder_state[1] = next_state[1];
    cdrom->xa_pcm_count = (uint16_t)(cdrom->xa_pcm_count + frames);
    return true;
}

static bool load_current_sector(PsxCdrom *cdrom)
{
    const unsigned minute = (unsigned)(cdrom->location[0] >> 4u) * 10u +
                            (unsigned)(cdrom->location[0] & 0x0fu);
    const unsigned second = (unsigned)(cdrom->location[1] >> 4u) * 10u +
                            (unsigned)(cdrom->location[1] & 0x0fu);
    const unsigned frame = (unsigned)(cdrom->location[2] >> 4u) * 10u +
                           (unsigned)(cdrom->location[2] & 0x0fu);
    const unsigned absolute_frame = minute * 60u * 75u +
                                    second * 75u + frame;
    size_t offset;
    if ((!cdrom->iso_data && !cdrom->disc.mounted) || !bcd_valid(cdrom->location[0], 99u) ||
        !bcd_valid(cdrom->location[1], 59u) ||
        !bcd_valid(cdrom->location[2], 74u) || absolute_frame < 150u)
        return false;
    offset = (size_t)(absolute_frame - 150u);
    cdrom->xa_subheader_valid = false;
    if (cdrom->disc.mounted) {
        if (cdrom->disc.mode == PSX_DISC_MODE2_2352 &&
            psx_disc_read_sector_form2_ex(&cdrom->disc, offset,
                                          cdrom->sector_data,
                                          &cdrom->xa_subheader)) {
            cdrom->data_count = PSX_DISC_SECTOR_FORM2_DATA_SIZE;
            cdrom->xa_subheader_valid = true;
        } else {
            if (!psx_disc_read_sector(&cdrom->disc, offset,
                                      cdrom->sector_data))
                return false;
            cdrom->data_count = PSX_CDROM_SECTOR_SIZE;
        }
    } else {
        offset *= PSX_CDROM_SECTOR_SIZE;
        if (offset > cdrom->iso_size ||
            cdrom->iso_size - offset < PSX_CDROM_SECTOR_SIZE)
            return false;
        memcpy(cdrom->sector_data, cdrom->iso_data + offset,
               PSX_CDROM_SECTOR_SIZE);
        cdrom->data_count = PSX_CDROM_SECTOR_SIZE;
    }
    cdrom->data_index = 0u;

    /* XA audio+realtime sectors bypass the CPU FIFO when enabled. */
    if (cdrom->xa_subheader_valid &&
        (cdrom->xa_subheader.submode & 0x44u) == 0x44u) {
        bool filter_enabled = (cdrom->mode & 0x08u) != 0u;
        bool filter_match = psx_cdrom_xa_sector_matches_filter(cdrom);
        if (filter_enabled && !filter_match) {
            cdrom->data_count = 0u;
        } else if ((cdrom->mode & 0x40u) != 0u) {
            if (queue_xa_sector(cdrom)) cdrom->data_count = 0u;
        } else if (filter_enabled) {
            cdrom->data_count = 0u;
        }
    }

    cdrom->drive_status &= (uint8_t)~0x01u;
    cdrom->drive_status |= 0x22u;
    command_complete(cdrom, 1u);
    return true;
}

static void increment_location(PsxCdrom *cdrom)
{
    unsigned minute = (unsigned)(cdrom->location[0] >> 4u) * 10u +
                      (unsigned)(cdrom->location[0] & 0x0fu);
    unsigned second = (unsigned)(cdrom->location[1] >> 4u) * 10u +
                      (unsigned)(cdrom->location[1] & 0x0fu);
    unsigned frame = (unsigned)(cdrom->location[2] >> 4u) * 10u +
                     (unsigned)(cdrom->location[2] & 0x0fu);
    if (++frame >= 75u) {
        frame = 0u;
        if (++second >= 60u) {
            second = 0u;
            if (minute < 99u) ++minute;
        }
    }
    cdrom->location[0] = (uint8_t)(((minute / 10u) << 4u) | (minute % 10u));
    cdrom->location[1] = (uint8_t)(((second / 10u) << 4u) | (second % 10u));
    cdrom->location[2] = (uint8_t)(((frame / 10u) << 4u) | (frame % 10u));
}

static void execute_command(PsxCdrom *cdrom, uint8_t command)
{
    switch (command) {
    case 0x01u: /* Getstat */
        (void)push_response(cdrom, cdrom->drive_status);
        command_complete(cdrom, 3u);
        break;
    case 0x02u: /* Setloc: three BCD MSF bytes. */
        if (cdrom->parameter_count < 3u ||
            !pop_parameter(cdrom, &cdrom->location[0]) ||
            !pop_parameter(cdrom, &cdrom->location[1]) ||
            !pop_parameter(cdrom, &cdrom->location[2]) ||
            !bcd_valid(cdrom->location[0], 99u) ||
            !bcd_valid(cdrom->location[1], 59u) ||
            !bcd_valid(cdrom->location[2], 74u)) {
            command_error(cdrom);
            break;
        }
        (void)push_response(cdrom, cdrom->drive_status);
        command_complete(cdrom, 3u);
        break;
    case 0x06u: /* ReadN starts a continuous sector stream. */
        cdrom->read_active = true;
        if (!load_current_sector(cdrom)) {
            cdrom->read_active = false;
            command_error(cdrom);
            break;
        }
        (void)push_response(cdrom, cdrom->drive_status);
        command_complete(cdrom, 3u);
        break;
    case 0x08u: /* Stop: acknowledgement now, mechanical completion later. */
    case 0x09u: { /* Pause */
        uint32_t delay;
        if (cdrom->pending_command != 0u) {
            command_error(cdrom);
            break;
        }
        if (command == 0x09u) {
            delay = (cdrom->mode & 0x80u) != 0u
                ? PSX_CDROM_PAUSE_DOUBLE_CYCLES
                : PSX_CDROM_PAUSE_NORMAL_CYCLES;
        } else {
            delay = (cdrom->mode & 0x80u) != 0u
                ? PSX_CDROM_STOP_DOUBLE_CYCLES
                : PSX_CDROM_STOP_NORMAL_CYCLES;
        }
        cdrom->pending_command = command;
        cdrom->command_cycles_remaining = delay;
        (void)push_response(cdrom, cdrom->drive_status);
        command_complete(cdrom, 3u);
        break;
    }
    case 0x0au: /* Init */
        cdrom->drive_status = 0u;
        cdrom->request = 0u;
        cdrom->data_index = 0u;
        cdrom->data_count = 0u;
        cdrom->read_active = false;
        cdrom->mode = 0u;
        reset_xa_audio(cdrom);
        cdrom->pending_command = 0u;
        cdrom->command_cycles_remaining = 0u;
        (void)push_response(cdrom, cdrom->drive_status);
        command_complete(cdrom, 3u);
        break;
    case 0x0du: /* Setfilter: XA file and channel selection. */
        {
            uint8_t old_file = cdrom->xa_filter_file;
            uint8_t old_channel = cdrom->xa_filter_channel;
            if (cdrom->parameter_count < 2u ||
                !pop_parameter(cdrom, &cdrom->xa_filter_file) ||
                !pop_parameter(cdrom, &cdrom->xa_filter_channel)) {
                command_error(cdrom);
                break;
            }
            if (old_file != cdrom->xa_filter_file ||
                old_channel != cdrom->xa_filter_channel)
                reset_xa_audio(cdrom);
        }
        (void)push_response(cdrom, cdrom->drive_status);
        command_complete(cdrom, 3u);
        break;
    case 0x0eu: /* Setmode */
        {
            uint8_t old_mode = cdrom->mode;
            uint8_t new_mode;
            if (cdrom->parameter_count < 1u ||
                !pop_parameter(cdrom, &new_mode)) {
                command_error(cdrom);
                break;
            }
            cdrom->mode = new_mode;
            if (((old_mode ^ new_mode) & 0x48u) != 0u)
                reset_xa_audio(cdrom);
        }
        (void)push_response(cdrom, cdrom->drive_status);
        command_complete(cdrom, 3u);
        break;
    case 0x11u: /* GetlocL requires sector-header decoding, not implemented. */
    default:
        command_error(cdrom);
        break;
    }
}

bool psx_cdrom_init(PsxCdrom *cdrom)
{
    if (!cdrom) return false;
    memset(cdrom, 0, sizeof(*cdrom));
    psx_cdrom_reset(cdrom);
    cdrom->initialized = true;
    return true;
}

void psx_cdrom_reset(PsxCdrom *cdrom)
{
    if (!cdrom) return;
    cdrom->index = 0u;
    cdrom->parameter_head = 0u;
    cdrom->parameter_count = 0u;
    cdrom->response_head = 0u;
    cdrom->response_count = 0u;
    cdrom->interrupt_enable = 0u;
    cdrom->interrupt_flags = 0u;
    cdrom->request = 0u;
    cdrom->drive_status = 0u;
    cdrom->data_index = 0u;
    cdrom->data_count = 0u;
    cdrom->read_active = false;
    cdrom->mode = 0u;
    cdrom->xa_filter_file = 0u;
    cdrom->xa_filter_channel = 0u;
    reset_xa_audio(cdrom);
    cdrom->pending_command = 0u;
    cdrom->command_cycles_remaining = 0u;
    cdrom->location[0] = 0u;
    cdrom->location[1] = 0u;
    cdrom->location[2] = 0u;
    memset(&cdrom->xa_subheader, 0, sizeof(cdrom->xa_subheader));
    cdrom->xa_subheader_valid = false;
}

bool psx_cdrom_mount_iso(PsxCdrom *cdrom, const uint8_t *image, size_t size)
{
    if (!cdrom || !cdrom->initialized || !image || size == 0u ||
        (size % PSX_CDROM_SECTOR_SIZE) != 0u) return false;
    psx_disc_unmount(&cdrom->disc);
    cdrom->iso_data = image;
    cdrom->iso_size = size;
    reset_xa_audio(cdrom);
    cdrom->drive_status &= (uint8_t)~0x01u;
    return true;
}

bool psx_cdrom_mount_disc(PsxCdrom *cdrom, const PsxDiscImage *disc)
{
    if (!cdrom || !cdrom->initialized || !disc || !disc->mounted ||
        !disc->source || disc->sector_count == 0u)
        return false;
    cdrom->disc = *disc;
    cdrom->iso_data = NULL;
    cdrom->iso_size = 0u;
    cdrom->data_index = 0u;
    cdrom->data_count = 0u;
    cdrom->read_active = false;
    reset_xa_audio(cdrom);
    cdrom->drive_status &= (uint8_t)~0x01u;
    return true;
}

bool psx_cdrom_mount_cue(PsxCdrom *cdrom, const char *cue_text,
                         size_t cue_size, const uint8_t *bin_data,
                         size_t bin_size)
{
    PsxDiscImage disc;
    return cdrom && cdrom->initialized &&
           psx_disc_mount_cue(&disc, cue_text, cue_size, bin_data, bin_size) &&
           psx_cdrom_mount_disc(cdrom, &disc);
}

void psx_cdrom_unmount(PsxCdrom *cdrom)
{
    if (!cdrom || !cdrom->initialized) return;
    cdrom->iso_data = NULL;
    cdrom->iso_size = 0u;
    psx_disc_unmount(&cdrom->disc);
    cdrom->data_index = 0u;
    cdrom->data_count = 0u;
    cdrom->read_active = false;
    reset_xa_audio(cdrom);
    cdrom->drive_status |= 0x01u;
    cdrom->drive_status &= (uint8_t)~0x20u;
    cdrom->interrupt_flags &= (uint8_t)~0x01u;
}

bool psx_cdrom_advance_sector(PsxCdrom *cdrom)
{
    if (!cdrom || !cdrom->initialized || !cdrom->read_active ||
        cdrom->data_count != 0u)
        return false;
    increment_location(cdrom);
    if (!load_current_sector(cdrom)) {
        cdrom->read_active = false;
        cdrom->drive_status |= 0x01u;
        cdrom->drive_status &= (uint8_t)~0x20u;
        command_complete(cdrom, 5u);
        return false;
    }
    return true;
}

uint32_t psx_cdrom_sector_period_cycles(const PsxCdrom *cdrom)
{
    if (!cdrom || !cdrom->initialized) return 0u;
    return (cdrom->mode & 0x80u) != 0u
        ? PSX_CDROM_DOUBLE_SPEED_SECTOR_CYCLES
        : PSX_CDROM_NORMAL_SPEED_SECTOR_CYCLES;
}

void psx_cdrom_advance_cycles(PsxCdrom *cdrom, uint32_t cycles)
{
    if (!cdrom || !cdrom->initialized ||
        cdrom->pending_command == 0u)
        return;
    if (cycles < cdrom->command_cycles_remaining) {
        cdrom->command_cycles_remaining -= cycles;
        return;
    }

    cdrom->command_cycles_remaining = 0u;
    if (cdrom->pending_command == 0x09u) {
        cdrom->read_active = false;
        cdrom->drive_status &= (uint8_t)~0x20u;
        cdrom->drive_status |= 0x02u;
    } else if (cdrom->pending_command == 0x08u) {
        cdrom->read_active = false;
        cdrom->drive_status &= (uint8_t)~0x22u;
        cdrom->data_index = 0u;
        cdrom->data_count = 0u;
        cdrom->request = 0u;
    }
    cdrom->pending_command = 0u;
    (void)push_response(cdrom, cdrom->drive_status);
    command_complete(cdrom, 2u);
}

bool psx_cdrom_read_data32(PsxCdrom *cdrom, uint32_t *value)
{
    uint32_t word = 0u;
    if (!cdrom || !cdrom->initialized || !value ||
        (cdrom->request & 0x80u) == 0u || cdrom->data_count < 4u)
        return false;

    for (unsigned byte = 0u; byte < 4u; ++byte) {
        word |= (uint32_t)cdrom->sector_data[cdrom->data_index++] <<
                (byte * 8u);
        --cdrom->data_count;
    }
    if (cdrom->data_count == 0u)
        cdrom->drive_status &= (uint8_t)~0x20u;
    *value = word;
    return true;
}

bool psx_cdrom_irq_output(const PsxCdrom *cdrom)
{
    return cdrom && cdrom->initialized &&
           (cdrom->interrupt_flags & cdrom->interrupt_enable & 0x1fu) != 0u;
}

bool psx_cdrom_read8(PsxCdrom *cdrom, uint32_t address, uint8_t *value)
{
    unsigned reg;
    if (!cdrom || !cdrom->initialized || !value ||
        address < PSX_CDROM_BASE_ADDR ||
        address >= PSX_CDROM_BASE_ADDR + 4u) return false;
    reg = (unsigned)(address - PSX_CDROM_BASE_ADDR);
    switch (reg) {
    case 0u:
        *value = cdrom_status(cdrom);
        return true;
    case 1u:
        if (cdrom->index == 0u && cdrom->response_count != 0u) {
            *value = cdrom->responses[cdrom->response_head];
            cdrom->response_head = (uint8_t)(
                (cdrom->response_head + 1u) % PSX_CDROM_FIFO_CAPACITY);
            --cdrom->response_count;
        } else {
            *value = 0u;
        }
        return true;
    case 2u:
        if (cdrom->index == 1u) {
            *value = cdrom->interrupt_enable;
        } else if (cdrom->index == 0u) {
            if ((cdrom->request & 0x80u) != 0u &&
                cdrom->data_count != 0u) {
                *value = cdrom->sector_data[cdrom->data_index++];
                --cdrom->data_count;
                if (cdrom->data_count == 0u)
                    cdrom->drive_status &= (uint8_t)~0x20u;
            } else {
                *value = 0u;
            }
        } else {
            *value = 0u;
        }
        return true;
    case 3u:
        *value = cdrom->index == 1u
            ? (uint8_t)(cdrom->interrupt_flags | 0xe0u) : 0u;
        return true;
    default:
        return false;
    }
}

bool psx_cdrom_write8(PsxCdrom *cdrom, uint32_t address, uint8_t value)
{
    unsigned reg;
    if (!cdrom || !cdrom->initialized ||
        address < PSX_CDROM_BASE_ADDR ||
        address >= PSX_CDROM_BASE_ADDR + 4u) return false;
    reg = (unsigned)(address - PSX_CDROM_BASE_ADDR);
    switch (reg) {
    case 0u:
        cdrom->index = (uint8_t)(value & 3u);
        return true;
    case 1u:
        if (cdrom->index == 0u) execute_command(cdrom, value);
        return true;
    case 2u:
        if (cdrom->index == 0u) {
            uint8_t tail;
            if (cdrom->parameter_count >= PSX_CDROM_FIFO_CAPACITY)
                return false;
            tail = (uint8_t)((cdrom->parameter_head +
                              cdrom->parameter_count) %
                             PSX_CDROM_FIFO_CAPACITY);
            cdrom->parameters[tail] = value;
            ++cdrom->parameter_count;
        } else if (cdrom->index == 1u) {
            cdrom->interrupt_enable = (uint8_t)(value & 0x1fu);
        } else if (cdrom->index == 2u) {
            cdrom->request = value;
        }
        return true;
    case 3u:
        if (cdrom->index == 0u) {
            cdrom->interrupt_flags &= (uint8_t)~(value & 0x1fu);
            if ((value & 0x40u) != 0u) {
                cdrom->parameter_head = 0u;
                cdrom->parameter_count = 0u;
            }
        }
        return true;
    default:
        return false;
    }
}


bool psx_cdrom_get_xa_audio_format(const PsxCdrom *cdrom,
                                   PsxDiscXaAudioFormat *format)
{
    if (!cdrom || !cdrom->initialized || !cdrom->xa_subheader_valid)
        return false;
    return psx_disc_xa_audio_format(&cdrom->xa_subheader, format);
}


bool psx_cdrom_xa_sector_matches_filter(const PsxCdrom *cdrom)
{
    PsxDiscXaAudioFormat format;
    return cdrom && cdrom->initialized && cdrom->xa_subheader_valid &&
           psx_disc_xa_audio_format(&cdrom->xa_subheader, &format) &&
           cdrom->xa_subheader.file == cdrom->xa_filter_file &&
           cdrom->xa_subheader.channel == cdrom->xa_filter_channel;
}


bool psx_cdrom_read_xa_audio(PsxCdrom *cdrom, int16_t *left, int16_t *right,
                             size_t max_frames, size_t *frames_read,
                             uint16_t *sample_rate_hz)
{
    size_t count;
    if (!cdrom || !cdrom->initialized || !left || !right ||
        !frames_read || !sample_rate_hz)
        return false;

    count = max_frames < cdrom->xa_pcm_count
        ? max_frames : cdrom->xa_pcm_count;
    for (size_t i = 0u; i < count; ++i) {
        size_t index = ((size_t)cdrom->xa_pcm_head + i) %
                       PSX_CDROM_XA_PCM_QUEUE_FRAMES;
        left[i] = cdrom->xa_pcm_left[index];
        right[i] = cdrom->xa_pcm_right[index];
    }
    cdrom->xa_pcm_head = (uint16_t)(
        ((size_t)cdrom->xa_pcm_head + count) %
        PSX_CDROM_XA_PCM_QUEUE_FRAMES);
    cdrom->xa_pcm_count = (uint16_t)(cdrom->xa_pcm_count - count);
    *frames_read = count;
    *sample_rate_hz = cdrom->xa_pcm_count == 0u
        ? 0u : cdrom->xa_pcm_sample_rate;
    if (cdrom->xa_pcm_count == 0u) {
        cdrom->xa_pcm_head = 0u;
        cdrom->xa_pcm_sample_rate = 0u;
    }
    return true;
}
