#include "dreampsx/spu.h"

#include <assert.h>
#include <stdio.h>

int main(void)
{
    PsxSpu spu;
    uint16_t value;
    uint8_t byte;

    assert(psx_spu_init(&spu));
    assert(spu.initialized);
    assert(psx_spu_write16(&spu, PSX_SPU_BASE_ADDR, 0x1234u));
    assert(psx_spu_read16(&spu, PSX_SPU_BASE_ADDR, &value));
    assert(value == 0x1234u);

    /* Voice register space is ordinary 16-bit register storage. */
    assert(psx_spu_write16(&spu, PSX_SPU_BASE_ADDR + 0x0e0u, 0x7fffu));
    assert(psx_spu_read16(&spu, PSX_SPU_BASE_ADDR + 0x0e0u, &value));
    assert(value == 0x7fffu);

    /* Typed voice registers expose 24 independent parameter banks. */
    assert(psx_spu_write16(&spu, PSX_SPU_BASE_ADDR + 0x000u, 0x1111u));
    assert(psx_spu_write16(&spu, PSX_SPU_BASE_ADDR + 0x002u, 0x2222u));
    assert(psx_spu_write16(&spu, PSX_SPU_BASE_ADDR + 0x004u, 0xffffu));
    assert(psx_spu_write16(&spu, PSX_SPU_BASE_ADDR + 0x006u, 0x0123u));
    assert(psx_spu_write16(&spu, PSX_SPU_BASE_ADDR + 0x008u, 0x4567u));
    assert(psx_spu_write16(&spu, PSX_SPU_BASE_ADDR + 0x00au, 0x89abu));
    assert(psx_spu_write16(&spu, PSX_SPU_BASE_ADDR + 0x00eu, 0x0321u));
    assert(spu.voices[0].volume_left == 0x1111u);
    assert(spu.voices[0].volume_right == 0x2222u);
    assert(spu.voices[0].pitch == 0x3fffu);
    assert(spu.voices[0].start_address == 0x0123u);
    assert(spu.voices[0].adsr1 == 0x4567u);
    assert(spu.voices[0].adsr2 == 0x89abu);
    assert(spu.voices[0].repeat_address == 0x0321u);
    assert(psx_spu_write16(&spu, PSX_SPU_BASE_ADDR + 0x00cu, 0x7777u));
    assert(psx_spu_read16(&spu, PSX_SPU_BASE_ADDR + 0x00cu, &value));
    assert(value == 0u);
    assert(psx_spu_write16(&spu, PSX_SPU_BASE_ADDR + 0x170u, 0x5a5au));
    assert(spu.voices[23].volume_left == 0x5a5au);

    /* Key-on/off registers update the 24-voice active mask and read as zero. */
    assert(psx_spu_write16(&spu, PSX_SPU_BASE_ADDR +
                           PSX_SPU_KEY_ON_LOW_OFFSET, 0x8001u));
    assert(psx_spu_write16(&spu, PSX_SPU_BASE_ADDR +
                           PSX_SPU_KEY_ON_HIGH_OFFSET, 0x0081u));
    assert(psx_spu_active_voice_mask(&spu) == 0x00818001u);
    assert(psx_spu_read16(&spu, PSX_SPU_BASE_ADDR +
                          PSX_SPU_KEY_ON_LOW_OFFSET, &value));
    assert(value == 0u);
    assert(psx_spu_write16(&spu, PSX_SPU_BASE_ADDR +
                           PSX_SPU_KEY_OFF_LOW_OFFSET, 0x0001u));
    assert(psx_spu_write16(&spu, PSX_SPU_BASE_ADDR +
                           PSX_SPU_KEY_OFF_HIGH_OFFSET, 0x0080u));
    assert(psx_spu_active_voice_mask(&spu) == 0x00018000u);
    assert(psx_spu_write16(&spu, PSX_SPU_BASE_ADDR +
                           PSX_SPU_CONTROL_OFFSET, 0x1234u));
    assert(psx_spu_read16(&spu, PSX_SPU_BASE_ADDR +
                          PSX_SPU_CONTROL_OFFSET, &value));
    assert(value == 0x1234u);

    /* Transfer address is expressed in 8-byte units; data port advances
     * by one 16-bit word for each access. */
    assert(psx_spu_write16(&spu, PSX_SPU_BASE_ADDR +
                           PSX_SPU_TRANSFER_ADDR_OFFSET, 0x0010u));
    assert(spu.transfer_address == 0x80u);
    assert(psx_spu_write16(&spu, PSX_SPU_BASE_ADDR +
                           PSX_SPU_TRANSFER_DATA_OFFSET, 0xabcd));
    assert(psx_spu_write16(&spu, PSX_SPU_BASE_ADDR +
                           PSX_SPU_TRANSFER_DATA_OFFSET, 0x1357u));
    assert(spu.transfer_address == 0x84u);
    assert(psx_spu_write16(&spu, PSX_SPU_BASE_ADDR +
                           PSX_SPU_TRANSFER_ADDR_OFFSET, 0x0010u));
    assert(psx_spu_read16(&spu, PSX_SPU_BASE_ADDR +
                          PSX_SPU_TRANSFER_DATA_OFFSET, &value));
    assert(value == 0xabcdu);
    assert(psx_spu_read16(&spu, PSX_SPU_BASE_ADDR +
                          PSX_SPU_TRANSFER_DATA_OFFSET, &value));
    assert(value == 0x1357u);

    /* Byte accesses preserve the other byte in a register. */
    assert(psx_spu_write16(&spu, PSX_SPU_BASE_ADDR + 0x20u, 0x1122u));
    assert(psx_spu_write8(&spu, PSX_SPU_BASE_ADDR + 0x21u, 0xaau));
    assert(psx_spu_read8(&spu, PSX_SPU_BASE_ADDR + 0x20u, &byte));
    assert(byte == 0x22u);
    assert(psx_spu_read16(&spu, PSX_SPU_BASE_ADDR + 0x20u, &value));
    assert(value == 0xaa22u);

    /* The FIFO-like transfer port is intentionally word-only. */
    assert(!psx_spu_read8(&spu, PSX_SPU_BASE_ADDR +
                          PSX_SPU_TRANSFER_DATA_OFFSET, &byte));
    assert(!psx_spu_write8(&spu, PSX_SPU_BASE_ADDR +
                           PSX_SPU_TRANSFER_DATA_OFFSET, 0x12u));

    /* RAM wraps at 512 KiB. */
    assert(psx_spu_write16(&spu, PSX_SPU_BASE_ADDR +
                           PSX_SPU_TRANSFER_ADDR_OFFSET, 0xffffu));
    assert(spu.transfer_address == PSX_SPU_RAM_BYTES - 8u);
    assert(psx_spu_write16(&spu, PSX_SPU_BASE_ADDR +
                           PSX_SPU_TRANSFER_DATA_OFFSET, 0x55aau));
    assert(spu.transfer_address == PSX_SPU_RAM_BYTES - 6u);

    /* Reset clears registers, RAM and transfer position. */
    psx_spu_reset(&spu);
    assert(spu.transfer_address == 0u);
    assert(psx_spu_active_voice_mask(&spu) == 0u);
    assert(spu.voices[0].volume_left == 0u);
    assert(spu.voices[23].volume_left == 0u);
    assert(psx_spu_read16(&spu, PSX_SPU_BASE_ADDR, &value));
    assert(value == 0u);
    assert(psx_spu_write16(&spu, PSX_SPU_BASE_ADDR +
                           PSX_SPU_TRANSFER_DATA_OFFSET, 0x9876u));
    assert(psx_spu_write16(&spu, PSX_SPU_BASE_ADDR +
                           PSX_SPU_TRANSFER_ADDR_OFFSET, 0u));
    assert(psx_spu_read16(&spu, PSX_SPU_BASE_ADDR +
                          PSX_SPU_TRANSFER_DATA_OFFSET, &value));
    assert(value == 0x9876u);

    assert(!psx_spu_read16(&spu, PSX_SPU_BASE_ADDR + 1u, &value));
    assert(!psx_spu_read16(&spu, 0x1f801800u, &value));
    puts("DreamPSX SPU register/RAM tests: OK");
    return 0;
}
