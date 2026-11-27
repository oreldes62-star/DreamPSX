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
