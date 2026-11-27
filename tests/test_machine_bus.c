#include "dreampsx/machine.h"
#include "dreampsx/irq.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

int main(void)
{
    PsxMachine machine;
    uint8_t *ram = (uint8_t *)calloc(1u, PSX_RAM_SIZE);
    uint8_t *bios = (uint8_t *)calloc(1u, PSX_BIOS_SIZE);
    uint16_t *gpu_vram = (uint16_t *)calloc(PSX_GPU_VRAM_WIDTH * PSX_GPU_VRAM_HEIGHT, sizeof(uint16_t));
    uint32_t word = 0u;
    uint16_t half = 0u;
    static uint8_t cdrom_iso[PSX_CDROM_SECTOR_SIZE * 2u];
    unsigned cdrom_i;

    assert(ram && bios && gpu_vram);
    assert(psx_machine_init(&machine, ram, PSX_RAM_SIZE, bios, PSX_BIOS_SIZE,
                            gpu_vram, PSX_GPU_VRAM_WIDTH * PSX_GPU_VRAM_HEIGHT));
    assert(machine.bus.initialized);
    {
        PsxDmaTransferEvent event;
        assert(!psx_dma_last_transfer_event(&machine.dma, &event));
    }

    /* DMA channel registers are visible through physical and KSEG1 bus
     * addresses; channel 2 is reserved for the GPU transfer path. */
    assert(psx_bus_write32(&machine.bus, 0x1f8010a0u, 0x00102003u));
    assert(psx_bus_write32(&machine.bus, 0xbf8010a4u, 0x00020008u));
    assert(psx_bus_write32(&machine.bus, 0xbf8010a8u, 0x00000201u));
    assert(psx_bus_read32(&machine.bus, 0x1f8010a0u, &word));
    assert(word == 0x00102000u);
    assert(psx_bus_read32(&machine.bus, 0xbf8010a4u, &word));
    assert(word == 0x00020008u);
    assert(psx_bus_read32(&machine.bus, 0x1f8010a8u, &word));
    assert(word == 0x00000201u);
    assert(psx_bus_read32(&machine.bus, 0x1f8010f0u, &word));
    assert(word == 0x07654321u);

    /* CPU-facing bus reaches RAM and its KSEG aliases. */
    assert(psx_bus_write32(&machine.bus, 0x00000100u, 0x1234abcdu));
    assert(psx_bus_read32(&machine.bus, 0x80000100u, &word));
    assert(word == 0x1234abcdu);
    assert(psx_bus_read32(&machine.bus, 0xa0000100u, &word));
    assert(word == 0x1234abcdu);

    /* The same bus reaches IRQ controller through physical and KSEG1 MMIO. */
    assert(psx_bus_write16(&machine.bus, PSX_IRQ_MASK_ADDR, (uint16_t)(1u << 2)));
    assert(psx_bus_read16(&machine.bus, 0xbf801074u, &half));
    assert(half == (uint16_t)(1u << 2));

    psx_irq_set_line(&machine.irq, 2u, true);
    assert(psx_bus_read16(&machine.bus, 0x1f801070u, &half));
    assert((half & (1u << 2)) != 0u);

    /* I_STAT is write-zero-to-clear through the CPU-facing bus. */
    assert(psx_bus_write16(&machine.bus, 0x1f801070u,
                           (uint16_t)~(1u << 2)));
    assert(psx_bus_read16(&machine.bus, PSX_IRQ_STAT_ADDR, &half));
    assert((half & (1u << 2)) == 0u);

    /* CD-ROM byte registers use the bank selector and route INT3 to IRQ2. */
    assert(psx_bus_write16(&machine.bus, PSX_IRQ_MASK_ADDR,
                           (uint16_t)(1u << 2u)));
    assert(psx_bus_write8(&machine.bus, PSX_CDROM_BASE_ADDR, 1u));
    assert(psx_bus_write8(&machine.bus, PSX_CDROM_BASE_ADDR + 2u, 0x04u));
    assert(psx_bus_write8(&machine.bus, PSX_CDROM_BASE_ADDR, 0u));
    assert(psx_bus_write8(&machine.bus, PSX_CDROM_BASE_ADDR + 2u, 0x00u));
    assert(psx_bus_write8(&machine.bus, PSX_CDROM_BASE_ADDR + 2u, 0x02u));
    assert(psx_bus_write8(&machine.bus, PSX_CDROM_BASE_ADDR + 2u, 0x15u));
    assert(psx_bus_write8(&machine.bus, PSX_CDROM_BASE_ADDR + 1u, 0x02u));
    assert((machine.irq.source_level & (1u << 2u)) != 0u);
    assert((psx_irq_read_stat(&machine.irq) & (1u << 2u)) != 0u);
    assert(psx_bus_write8(&machine.bus, PSX_CDROM_BASE_ADDR + 3u, 0x04u));
    assert((machine.irq.source_level & (1u << 2u)) == 0u);
    assert((psx_irq_read_stat(&machine.irq) & (1u << 2u)) != 0u);
    assert(psx_bus_write16(&machine.bus, PSX_IRQ_STAT_ADDR,
                           (uint16_t)~(1u << 2u)));
    assert((psx_irq_read_stat(&machine.irq) & (1u << 2u)) == 0u);

    /* Machine-level timed CD-ROM sector delivery and IRQ2 propagation. */
    for (cdrom_i = 0u; cdrom_i < sizeof(cdrom_iso); ++cdrom_i)
        cdrom_iso[cdrom_i] = (uint8_t)(
            ((cdrom_i / PSX_CDROM_SECTOR_SIZE) * 0x40u) +
            (cdrom_i & 0xffu));
    assert(psx_cdrom_mount_iso(&machine.cdrom, cdrom_iso,
                               sizeof(cdrom_iso)));
    assert(psx_bus_write8(&machine.bus, PSX_CDROM_BASE_ADDR, 1u));
    assert(psx_bus_write8(&machine.bus, PSX_CDROM_BASE_ADDR + 2u, 0x15u));
    assert(psx_bus_write8(&machine.bus, PSX_CDROM_BASE_ADDR, 0u));
    assert(psx_bus_write8(&machine.bus, PSX_CDROM_BASE_ADDR + 2u, 0x00u));
    assert(psx_bus_write8(&machine.bus, PSX_CDROM_BASE_ADDR + 2u, 0x02u));
    assert(psx_bus_write8(&machine.bus, PSX_CDROM_BASE_ADDR + 2u, 0x00u));
    assert(psx_bus_write8(&machine.bus, PSX_CDROM_BASE_ADDR + 1u, 0x02u));
    assert(psx_bus_write8(&machine.bus, PSX_CDROM_BASE_ADDR + 1u, 0x06u));
    assert(psx_bus_write8(&machine.bus, PSX_CDROM_BASE_ADDR, 2u));
    assert(psx_bus_write8(&machine.bus, PSX_CDROM_BASE_ADDR + 2u, 0x80u));
    assert(psx_bus_write8(&machine.bus, PSX_CDROM_BASE_ADDR, 0u));
    for (cdrom_i = 0u; cdrom_i < PSX_CDROM_SECTOR_SIZE; ++cdrom_i)
        assert(psx_bus_read8(&machine.bus, PSX_CDROM_BASE_ADDR + 2u,
                             (uint8_t *)&half));
    assert(machine.cdrom.data_count == 0u);
    assert(psx_bus_write8(&machine.bus, PSX_CDROM_BASE_ADDR + 3u, 0x05u));
    assert((machine.irq.source_level & (1u << 2u)) == 0u);
    assert(psx_machine_cdrom_advance_sector(&machine));
    assert(machine.cdrom.data_count == PSX_CDROM_SECTOR_SIZE);
    assert((machine.irq.source_level & (1u << 2u)) != 0u);
    assert(psx_bus_read8(&machine.bus, PSX_CDROM_BASE_ADDR + 2u,
                         (uint8_t *)&word));
    assert((word & 0xffu) == 0x40u);
    assert(psx_bus_write8(&machine.bus, PSX_CDROM_BASE_ADDR + 3u, 0x01u));
    for (cdrom_i = 1u; cdrom_i < PSX_CDROM_SECTOR_SIZE; ++cdrom_i)
        assert(psx_bus_read8(&machine.bus, PSX_CDROM_BASE_ADDR + 2u,
                             (uint8_t *)&half));
    assert(!psx_machine_cdrom_advance_sector(&machine));
    assert((machine.irq.source_level & (1u << 2u)) != 0u);
    assert(psx_bus_write8(&machine.bus, PSX_CDROM_BASE_ADDR + 3u, 0x10u));
    assert((machine.irq.source_level & (1u << 2u)) == 0u);

    /* Re-read sector zero through DMA3. CD-ROM DMA is a forced SyncMode-0
     * device-to-RAM burst, with the data FIFO enabled in bank 2. */
    psx_cdrom_reset(&machine.cdrom);
    assert(psx_bus_write8(&machine.bus, PSX_CDROM_BASE_ADDR, 1u));
    assert(psx_bus_write8(&machine.bus, PSX_CDROM_BASE_ADDR + 2u, 0x15u));
    assert(psx_bus_write8(&machine.bus, PSX_CDROM_BASE_ADDR, 0u));
    assert(psx_bus_write8(&machine.bus, PSX_CDROM_BASE_ADDR + 2u, 0x00u));
    assert(psx_bus_write8(&machine.bus, PSX_CDROM_BASE_ADDR + 2u, 0x02u));
    assert(psx_bus_write8(&machine.bus, PSX_CDROM_BASE_ADDR + 2u, 0x00u));
    assert(psx_bus_write8(&machine.bus, PSX_CDROM_BASE_ADDR + 1u, 0x02u));
    assert(psx_bus_write8(&machine.bus, PSX_CDROM_BASE_ADDR + 1u, 0x06u));
    assert(psx_bus_write8(&machine.bus, PSX_CDROM_BASE_ADDR, 2u));
    assert(psx_bus_write8(&machine.bus, PSX_CDROM_BASE_ADDR + 2u, 0x80u));
    assert(psx_bus_write8(&machine.bus, PSX_CDROM_BASE_ADDR, 0u));
    assert(machine.cdrom.data_count == PSX_CDROM_SECTOR_SIZE);
    assert(psx_bus_write32(&machine.bus, PSX_DMA_DPCR_ADDR,
                           0x07654321u | (1u << 15u)));
    assert(psx_bus_write32(&machine.bus, 0x1f8010b0u, 0x00001000u));
    assert(psx_bus_write32(&machine.bus, 0x1f8010b4u, 512u));
    assert(psx_bus_write32(&machine.bus, 0x1f8010b8u, 0x11000000u));
    assert(machine.cdrom.data_count == 0u);
    assert(psx_dma_cdrom_words_transferred(&machine.dma) == 512u);
    assert(psx_bus_read32(&machine.bus, 0x00001000u, &word));
    assert(word == 0x03020100u);
    assert(psx_bus_read32(&machine.bus, 0x00001004u, &word));
    assert(word == 0x07060504u);
    assert(psx_bus_read32(&machine.bus, 0x1f8010b8u, &word));
    assert((word & (1u << 24u)) == 0u);
    assert(psx_bus_read32(&machine.bus, PSX_DMA_DICR_ADDR, &word));
    assert((word & (1u << 27u)) != 0u);
    {
        PsxDmaTransferEvent event;
        assert(psx_dma_last_transfer_event(&machine.dma, &event));
        assert(event.channel == 3u);
        assert(event.sync_mode == 0u);
        assert(!event.ram_to_device);
        assert(event.completed);
        assert(event.payload_words == 512u);
    }

    /* CD-ROM sector cadence is now driven by the PS1 system clock:
     * 150 sectors/s at double speed, independent of cycle-call chunking. */
    assert(psx_machine_advance_system_cycles(
        &machine, PSX_CDROM_DOUBLE_SPEED_SECTOR_CYCLES / 2u));
    assert(machine.cdrom.data_count == 0u);
    assert(psx_machine_advance_system_cycles(
        &machine, PSX_CDROM_DOUBLE_SPEED_SECTOR_CYCLES / 2u - 1u));
    assert(machine.cdrom.data_count == 0u);
    assert(machine.cdrom_sector_cycle_phase ==
           PSX_CDROM_DOUBLE_SPEED_SECTOR_CYCLES - 1u);
    assert(psx_machine_advance_system_cycles(&machine, 1u));
    assert(machine.cdrom.data_count == PSX_CDROM_SECTOR_SIZE);
    assert(machine.cdrom.location[2] == 0x01u);
    {
        uint32_t cd_word = 0u;
        assert(psx_cdrom_read_data32(&machine.cdrom, &cd_word));
        assert(cd_word == 0x43424140u);
    }

    /* If the one-sector buffer is not drained by the next deadline, retain
     * one pending sector event. Once drained, a zero-cycle tick services it. */
    assert(psx_machine_advance_system_cycles(
        &machine, PSX_CDROM_DOUBLE_SPEED_SECTOR_CYCLES));
    assert(machine.cdrom.data_count == PSX_CDROM_SECTOR_SIZE - 4u);
    assert(machine.cdrom_sector_cycle_phase ==
           PSX_CDROM_DOUBLE_SPEED_SECTOR_CYCLES);
    for (cdrom_i = 4u; cdrom_i < PSX_CDROM_SECTOR_SIZE; ++cdrom_i)
        assert(psx_bus_read8(&machine.bus, PSX_CDROM_BASE_ADDR + 2u,
                             (uint8_t *)&half));
    assert(machine.cdrom.data_count == 0u);
    assert(psx_machine_advance_system_cycles(&machine, 0u));
    assert(!machine.cdrom.read_active);
    assert((machine.cdrom.interrupt_flags & (1u << 4u)) != 0u);
    assert(machine.cdrom_sector_cycle_phase == 0u);

    /* GPU registers share the machine bus and update its VRAM state. */
    assert(psx_bus_write32(&machine.bus, 0x1f801810u, 0x0200ff00u));
    assert(psx_bus_write32(&machine.bus, 0x1f801810u, 0u));
    assert(psx_bus_write32(&machine.bus, 0x1f801810u, 1u | (1u << 16u)));
    assert(psx_gpu_vram_read(&machine.gpu, 0u, 0u, &half));
    assert(half == 0x03e0u);
    assert(psx_bus_read32(&machine.bus, 0xbf801814u, &word));
    assert(word == 0x14802000u);

    /* Channel 2 manual DMA streams a GP0 fill command from RAM into VRAM. */
    assert(psx_bus_write32(&machine.bus, 0x1f801814u, 0x04000002u));
    assert(psx_bus_write32(&machine.bus, 0x00000400u, 0x0200ff00u));
    assert(psx_bus_write32(&machine.bus, 0x00000404u, 0x00000014u));
    assert(psx_bus_write32(&machine.bus, 0x00000408u, 0x00010002u));
    assert(psx_bus_write32(&machine.bus, 0x1f8010f0u,
                           0x07654321u | (1u << 11u)));
    assert(psx_bus_write32(&machine.bus, 0x1f8010a0u, 0x00000400u));
    assert(psx_bus_write32(&machine.bus, 0x1f8010a4u, 3u));
    assert(psx_bus_write32(&machine.bus, 0x1f8010a8u, 0x11000001u));
    assert(psx_gpu_vram_read(&machine.gpu, 20u, 0u, &half));
    assert(half == 0x03e0u);
    assert(psx_bus_read32(&machine.bus, 0x1f8010a0u, &word));
    assert(word == 0x0000040cu);
    assert(psx_bus_read32(&machine.bus, 0x1f8010a8u, &word));
    assert((word & ((1u << 24u) | (1u << 28u))) == 0u);
    assert(psx_bus_read32(&machine.bus, 0x1f8010f4u, &word));
    assert((word & (1u << 26u)) != 0u);
    assert(psx_dma_data_words_transferred(&machine.dma) == 515u);
    assert(psx_dma_manual_words_transferred(&machine.dma) == 3u);
    assert(psx_dma_request_words_transferred(&machine.dma) == 0u);
    assert(psx_dma_linked_list_words_transferred(&machine.dma) == 0u);
    assert(psx_dma_linked_list_headers_read(&machine.dma) == 0u);

    /* A channel armed while DPCR disables GPU DMA remains pending. Enabling
     * channel 2 later must launch the already-programmed transfer. */
    assert(psx_bus_write32(&machine.bus, 0x1f8010f0u, 0x07654321u));
    assert(psx_bus_write32(&machine.bus, 0x00000420u, 0x020000ffu));
    assert(psx_bus_write32(&machine.bus, 0x00000424u, 0x00000028u));
    assert(psx_bus_write32(&machine.bus, 0x00000428u, 0x00010010u));
    assert(psx_bus_write32(&machine.bus, 0x1f8010a0u, 0x00000420u));
    assert(psx_bus_write32(&machine.bus, 0x1f8010a4u, 3u));
    assert(psx_bus_write32(&machine.bus, 0x1f8010a8u, 0x11000001u));
    assert(psx_bus_read32(&machine.bus, 0x1f8010a8u, &word));
    assert((word & (1u << 24u)) != 0u);
    assert(psx_gpu_vram_read(&machine.gpu, 40u, 0u, &half));
    assert(half == 0u);
    assert(psx_bus_write32(&machine.bus, 0x1f8010f0u,
                           0x07654321u | (1u << 11u)));
    assert(psx_gpu_vram_read(&machine.gpu, 40u, 0u, &half));
    assert(half == 0x001fu);
    assert(psx_bus_read32(&machine.bus, 0x1f8010a8u, &word));
    assert((word & (1u << 24u)) == 0u);
    assert(psx_dma_data_words_transferred(&machine.dma) == 518u);
    assert(psx_dma_manual_words_transferred(&machine.dma) == 6u);

    /* GPU readback through channel 2: two VRAM pixels become one RAM word. */
    assert(psx_bus_write32(&machine.bus, 0x1f8010f4u, 1u << 26u));
    assert(psx_bus_write32(&machine.bus, 0x1f801810u, 0xc0000000u));
    assert(psx_bus_write32(&machine.bus, 0x1f801810u, 20u));
    assert(psx_bus_write32(&machine.bus, 0x1f801810u, 2u | (1u << 16u)));
    assert(psx_bus_write32(&machine.bus, 0x1f801814u, 0x04000003u));
    assert(psx_bus_write32(&machine.bus, 0x1f8010a0u, 0x00000500u));
    assert(psx_bus_write32(&machine.bus, 0x1f8010a4u, 1u));
    assert(psx_bus_write32(&machine.bus, 0x1f8010a8u, 0x11000000u));
    assert(psx_bus_read32(&machine.bus, 0x00000500u, &word));
    assert(word == 0x03e003e0u);
    assert(psx_bus_read32(&machine.bus, 0x1f8010a0u, &word));
    assert(word == 0x00000504u);
    assert(psx_bus_read32(&machine.bus, 0x1f8010a8u, &word));
    assert((word & ((1u << 24u) | (1u << 28u))) == 0u);
    assert(psx_bus_read32(&machine.bus, 0x1f8010f4u, &word));
    assert((word & (1u << 26u)) != 0u);
    assert(psx_dma_data_words_transferred(&machine.dma) == 519u);
    assert(psx_dma_manual_words_transferred(&machine.dma) == 7u);

    /* Linked-list DMA: a GP0 fill command is deliberately split across
     * two packets; packet headers are metadata, not GP0 command words. */
    assert(psx_bus_write32(&machine.bus, 0x1f8010f4u, 1u << 26u));
    assert(psx_bus_write32(&machine.bus, 0x1f801814u, 0x04000002u));
    assert(psx_bus_write32(&machine.bus, 0x00000600u, 0x02000610u));
    assert(psx_bus_write32(&machine.bus, 0x00000604u, 0x0200ff00u));
    assert(psx_bus_write32(&machine.bus, 0x00000608u, 0x0000001eu));
    assert(psx_bus_write32(&machine.bus, 0x00000610u, 0x01ffffffu));
    assert(psx_bus_write32(&machine.bus, 0x00000614u, 0x00010002u));
    assert(psx_bus_write32(&machine.bus, 0x1f8010a0u, 0x00000600u));
    assert(psx_bus_write32(&machine.bus, 0x1f8010a4u, 0u)); /* ignored in linked-list mode */
    assert(psx_bus_write32(&machine.bus, 0x1f8010a8u, 0x01000401u));
    assert(psx_gpu_vram_read(&machine.gpu, 30u, 0u, &half));
    assert(half == 0x03e0u);
    assert(psx_bus_read32(&machine.bus, 0x1f8010a0u, &word));
    assert(word == 0x00ffffffu);
    assert(psx_bus_read32(&machine.bus, 0x1f8010a8u, &word));
    assert((word & (1u << 24u)) == 0u);
    assert(psx_bus_read32(&machine.bus, 0x1f8010f4u, &word));
    assert((word & (1u << 26u)) != 0u);
    assert(psx_dma_data_words_transferred(&machine.dma) == 522u);
    assert(psx_dma_linked_list_words_transferred(&machine.dma) == 3u);
    assert(psx_dma_linked_list_headers_read(&machine.dma) == 2u);

    /* Request/block DMA: two three-word blocks carry two GP0 fills. */
    assert(psx_bus_write32(&machine.bus, 0x1f8010f4u, 1u << 26u));
    assert(psx_bus_write32(&machine.bus, 0x1f801814u, 0x04000002u));
    assert(psx_bus_write32(&machine.bus, 0x00000700u, 0x0200ff00u));
    assert(psx_bus_write32(&machine.bus, 0x00000704u, 0x00000028u));
    assert(psx_bus_write32(&machine.bus, 0x00000708u, 0x00010002u));
    assert(psx_bus_write32(&machine.bus, 0x0000070cu, 0x020000ffu));
    assert(psx_bus_write32(&machine.bus, 0x00000710u, 0x00000030u));
    assert(psx_bus_write32(&machine.bus, 0x00000714u, 0x00010002u));
    assert(psx_bus_write32(&machine.bus, 0x1f8010a0u, 0x00000700u));
    assert(psx_bus_write32(&machine.bus, 0x1f8010a4u, 0x00020003u));
    assert(psx_bus_write32(&machine.bus, 0x1f8010a8u, 0x01000201u));
    assert(psx_gpu_vram_read(&machine.gpu, 40u, 0u, &half));
    assert(half == 0x03e0u);
    assert(psx_gpu_vram_read(&machine.gpu, 48u, 0u, &half));
    assert(half == 0x001fu);
    assert(psx_bus_read32(&machine.bus, 0x1f8010a0u, &word));
    assert(word == 0x00000718u);
    assert(psx_bus_read32(&machine.bus, 0x1f8010a4u, &word));
    assert(word == 0u);
    assert(psx_bus_read32(&machine.bus, 0x1f8010a8u, &word));
    assert((word & (1u << 24u)) == 0u);
    assert(psx_bus_read32(&machine.bus, 0x1f8010f4u, &word));
    assert((word & (1u << 26u)) != 0u);
    assert(psx_dma_data_words_transferred(&machine.dma) == 528u);
    assert(psx_dma_request_words_transferred(&machine.dma) == 6u);

    /* Request/block DMA also supports GPUREAD-to-RAM image transfers. */
    assert(psx_bus_write32(&machine.bus, 0x1f8010f4u, 1u << 26u));
    assert(psx_bus_write32(&machine.bus, 0x1f801810u, 0xc0000000u));
    assert(psx_bus_write32(&machine.bus, 0x1f801810u, 40u));
    assert(psx_bus_write32(&machine.bus, 0x1f801810u, 2u | (1u << 16u)));
    assert(psx_bus_write32(&machine.bus, 0x1f801814u, 0x04000003u));
    assert(psx_bus_write32(&machine.bus, 0x1f8010a0u, 0x00000800u));
    assert(psx_bus_write32(&machine.bus, 0x1f8010a4u, 0x00010001u));
    assert(psx_bus_write32(&machine.bus, 0x1f8010a8u, 0x01000200u));
    assert(psx_bus_read32(&machine.bus, 0x00000800u, &word));
    assert(word == 0x03e003e0u);
    assert(psx_bus_read32(&machine.bus, 0x1f8010a0u, &word));
    assert(word == 0x00000804u);
    assert(psx_bus_read32(&machine.bus, 0x1f8010a8u, &word));
    assert((word & (1u << 24u)) == 0u);
    assert(psx_dma_data_words_transferred(&machine.dma) == 529u);
    assert(psx_dma_request_words_transferred(&machine.dma) == 7u);

    /* DMA master interrupt: DICR channel-2 enable plus master enable
     * must raise PS1 interrupt-controller source 3 on transfer completion. */
    assert(psx_bus_write32(&machine.bus, 0x1f8010f4u, 1u << 26u));
    assert(psx_bus_write32(&machine.bus, 0x1f8010f4u,
                           (1u << 23u) | (1u << 18u)));
    assert(psx_bus_write32(&machine.bus, 0x1f801814u, 0x04000002u));
    assert(psx_bus_write32(&machine.bus, 0x00000900u, 0x0200ff00u));
    assert(psx_bus_write32(&machine.bus, 0x00000904u, 0x00000032u));
    assert(psx_bus_write32(&machine.bus, 0x00000908u, 0x00010002u));
    assert(psx_bus_write32(&machine.bus, 0x1f8010a0u, 0x00000900u));
    assert(psx_bus_write32(&machine.bus, 0x1f8010a4u, 3u));
    assert(psx_bus_write32(&machine.bus, 0x1f8010a8u, 0x11000001u));
    assert(psx_bus_read16(&machine.bus, PSX_IRQ_STAT_ADDR, &half));
    assert((half & (1u << 3u)) != 0u);
    assert((machine.irq.source_level & (1u << 3u)) != 0u);
    assert(psx_dma_data_words_transferred(&machine.dma) == 532u);
    {
        PsxDmaTransferEvent event;
        assert(psx_dma_last_transfer_event(&machine.dma, &event));
        assert(event.sequence >= 1u);
        assert(event.channel == 2u);
        assert(event.sync_mode == 0u);
        assert(event.ram_to_device);
        assert(event.completed);
        assert(event.payload_words == 3u);
        assert(event.packet_headers == 0u);
    }
    assert(psx_dma_manual_words_transferred(&machine.dma) == 10u);
    assert(psx_dma_request_words_transferred(&machine.dma) == 7u);
    assert(psx_dma_linked_list_words_transferred(&machine.dma) == 3u);
    assert(psx_dma_manual_words_transferred(&machine.dma) +
           psx_dma_request_words_transferred(&machine.dma) +
           psx_dma_linked_list_words_transferred(&machine.dma) +
           psx_dma_cdrom_words_transferred(&machine.dma) ==
           psx_dma_data_words_transferred(&machine.dma));

    /* Clearing the DMA completion flag drops the source line, while the
     * latched I_STAT request remains until software acknowledges it. */
    assert(psx_bus_write32(&machine.bus, 0x1f8010f4u, 1u << 26u));
    assert((machine.irq.source_level & (1u << 3u)) == 0u);
    assert(psx_bus_read16(&machine.bus, PSX_IRQ_STAT_ADDR, &half));
    assert((half & (1u << 3u)) != 0u);
    assert(psx_bus_write16(&machine.bus, PSX_IRQ_STAT_ADDR,
                           (uint16_t)~(1u << 3u)));
    assert(psx_bus_read16(&machine.bus, PSX_IRQ_STAT_ADDR, &half));
    assert((half & (1u << 3u)) == 0u);

    /* DICR force-IRQ is independent of per-channel flags/enables and is
     * delivered through the same DMA source line. */
    assert(psx_bus_write32(&machine.bus, PSX_DMA_DICR_ADDR, 1u << 15u));
    assert((machine.irq.source_level & (1u << 3u)) != 0u);
    assert(psx_bus_read16(&machine.bus, PSX_IRQ_STAT_ADDR, &half));
    assert((half & (1u << 3u)) != 0u);
    assert(psx_bus_write32(&machine.bus, PSX_DMA_DICR_ADDR, 0u));
    assert((machine.irq.source_level & (1u << 3u)) == 0u);
    assert(psx_bus_write16(&machine.bus, PSX_IRQ_STAT_ADDR,
                           (uint16_t)~(1u << 3u)));
    assert(psx_bus_read16(&machine.bus, PSX_IRQ_STAT_ADDR, &half));
    assert((half & (1u << 3u)) == 0u);

    /* A channel completion flag alone must not assert the DMA IRQ when its
     * channel enable bit is clear, even if the master enable is set. */
    assert(psx_bus_write32(&machine.bus, PSX_DMA_DICR_ADDR,
                           (1u << 23u) | (1u << 26u)));
    assert((machine.irq.source_level & (1u << 3u)) == 0u);
    assert(psx_bus_read32(&machine.bus, PSX_DMA_DICR_ADDR, &word));
    assert((word & (1u << 31u)) == 0u);
    assert(psx_bus_write32(&machine.bus, PSX_DMA_DICR_ADDR, 1u << 26u));

    /* An out-of-range RAM source must produce a failed DMA event without
     * counting a payload word or clearing the outstanding request. */
    assert(psx_bus_write32(&machine.bus, 0x1f801814u, 0x04000002u));
    assert(psx_bus_write32(&machine.bus, 0x1f8010a0u, 0x00800000u));
    assert(psx_bus_write32(&machine.bus, 0x1f8010a4u, 1u));
    assert(!psx_bus_write32(&machine.bus, 0x1f8010a8u, 0x11000001u));
    {
        PsxDmaTransferEvent event;
        assert(psx_dma_last_transfer_event(&machine.dma, &event));
        assert(event.sequence >= 2u);
        assert(event.channel == 2u);
        assert(event.sync_mode == 0u);
        assert(event.ram_to_device);
        assert(!event.completed);
        assert(event.payload_words == 0u);
        assert(event.packet_headers == 0u);
    }
    assert((psx_dma_queued_mask(&machine.dma) & (1u << 2u)) != 0u);

    /* BIOS is readable; unmapped hardware and misaligned words are rejected. */
    assert(psx_bus_read32(&machine.bus, PSX_BIOS_BASE, &word));
    assert(!psx_bus_read32(&machine.bus, 0x1f801078u, &word));
    assert(!psx_bus_read32(&machine.bus, 0x00000101u, &word));

    /* LWC2/SWC2 unmapped accesses become COP0 data-bus exceptions. */
    machine.cop0_status = 0x40000000u; /* CU2 enabled */
    psx_cpu_write_gpr(&machine.cpu, 12u, 0x1f000000u);
    assert(psx_bus_write32(&machine.bus, 0x200u,
                           (0x32u << 26) | (12u << 21))); /* LWC2 D0,0(t4) */
    machine.cpu.pc = 0x200u;
    machine.cpu.next_pc = 0x204u;
    machine.cpu.delay_slot_pending = false;
    machine.cpu.load_delay_pending = false;
    assert(psx_machine_step(&machine));
    assert(machine.cop0_epc == 0x200u);
    assert(machine.cop0_badvaddr == 0x1f000000u);
    assert(((machine.cop0_cause & PSX_COP0_CAUSE_EXCCODE) >>
            PSX_COP0_CAUSE_EXCCODE_SHIFT) == PSX_EXCEPTION_BUS_DATA);

    machine.cop0_status = 0x40000000u;
    machine.cop0_cause = 0u;
    psx_cpu_write_gpr(&machine.cpu, 12u, 0x1f000000u);
    assert(psx_bus_write32(&machine.bus, 0x204u,
                           (0x3au << 26) | (12u << 21))); /* SWC2 D0,0(t4) */
    machine.cpu.pc = 0x204u;
    machine.cpu.next_pc = 0x208u;
    machine.cpu.delay_slot_pending = false;
    machine.cpu.load_delay_pending = false;
    assert(psx_machine_step(&machine));
    assert(machine.cop0_epc == 0x204u);
    assert(machine.cop0_badvaddr == 0x1f000000u);
    assert(((machine.cop0_cause & PSX_COP0_CAUSE_EXCCODE) >>
            PSX_COP0_CAUSE_EXCCODE_SHIFT) == PSX_EXCEPTION_BUS_DATA);

    /* GTE bus faults in a branch delay slot set Cause.BD and branch EPC. */
    machine.cop0_status = 0x40000000u; /* CU2 enabled */
    machine.cop0_cause = 0u;
    psx_cpu_write_gpr(&machine.cpu, 12u, 0x1f000000u);
    assert(psx_bus_write32(&machine.bus, 0x220u, 0x10000001u)); /* BEQ zero,zero,+1 */
    assert(psx_bus_write32(&machine.bus, 0x224u,
                           (0x32u << 26) | (12u << 21))); /* LWC2 delay slot */
    machine.cpu.pc = 0x220u;
    machine.cpu.next_pc = 0x224u;
    machine.cpu.delay_slot_pending = false;
    machine.cpu.load_delay_pending = false;
    assert(psx_machine_step(&machine));
    assert(machine.cpu.delay_slot_pending);
    assert(psx_machine_step(&machine));
    assert(machine.cop0_epc == 0x220u);
    assert((machine.cop0_cause & PSX_COP0_CAUSE_BD) != 0u);
    assert(machine.cop0_badvaddr == 0x1f000000u);
    assert(((machine.cop0_cause & PSX_COP0_CAUSE_EXCCODE) >>
            PSX_COP0_CAUSE_EXCCODE_SHIFT) == PSX_EXCEPTION_BUS_DATA);

    machine.cop0_status = 0x40000000u;
    machine.cop0_cause = 0u;
    psx_cpu_write_gpr(&machine.cpu, 12u, 0x1f000000u);
    assert(psx_bus_write32(&machine.bus, 0x228u, 0x10000001u)); /* BEQ zero,zero,+1 */
    assert(psx_bus_write32(&machine.bus, 0x22cu,
                           (0x3au << 26) | (12u << 21))); /* SWC2 delay slot */
    machine.cpu.pc = 0x228u;
    machine.cpu.next_pc = 0x22cu;
    machine.cpu.delay_slot_pending = false;
    machine.cpu.load_delay_pending = false;
    assert(psx_machine_step(&machine));
    assert(machine.cpu.delay_slot_pending);
    assert(psx_machine_step(&machine));
    assert(machine.cop0_epc == 0x228u);
    assert((machine.cop0_cause & PSX_COP0_CAUSE_BD) != 0u);
    assert(machine.cop0_badvaddr == 0x1f000000u);
    assert(((machine.cop0_cause & PSX_COP0_CAUSE_EXCCODE) >>
            PSX_COP0_CAUSE_EXCCODE_SHIFT) == PSX_EXCEPTION_BUS_DATA);

    /* A not-taken branch still marks its following LWC2 as a delay slot. */
    machine.cop0_status = 0x40000000u;
    machine.cop0_cause = 0u;
    psx_cpu_write_gpr(&machine.cpu, 8u, 7u);
    psx_cpu_write_gpr(&machine.cpu, 9u, 7u);
    psx_cpu_write_gpr(&machine.cpu, 12u, 0x1f000000u);
    assert(psx_bus_write32(&machine.bus, 0x230u,
                           (0x05u << 26) | (8u << 21) | (9u << 16) | 1u)); /* BNE equal: not taken */
    assert(psx_bus_write32(&machine.bus, 0x234u,
                           (0x32u << 26) | (12u << 21))); /* LWC2 delay slot */
    machine.cpu.pc = 0x230u;
    machine.cpu.next_pc = 0x234u;
    machine.cpu.delay_slot_pending = false;
    machine.cpu.load_delay_pending = false;
    assert(psx_machine_step(&machine));
    assert(machine.cpu.delay_slot_pending);
    assert(machine.cpu.next_pc == 0x238u); /* sequential path, despite delay slot */
    assert(psx_machine_step(&machine));
    assert(machine.cop0_epc == 0x230u);
    assert((machine.cop0_cause & PSX_COP0_CAUSE_BD) != 0u);
    assert(machine.cop0_badvaddr == 0x1f000000u);
    assert(((machine.cop0_cause & PSX_COP0_CAUSE_EXCCODE) >>
            PSX_COP0_CAUSE_EXCCODE_SHIFT) == PSX_EXCEPTION_BUS_DATA);

    /* Misaligned LWC2 in a branch delay slot reports AdEL with BD/EPC. */
    machine.cop0_status = 0x40000000u;
    machine.cop0_cause = 0u;
    psx_cpu_write_gpr(&machine.cpu, 12u, 0x601u);
    assert(psx_bus_write32(&machine.bus, 0x240u, 0x10000001u)); /* BEQ zero,zero,+1 */
    assert(psx_bus_write32(&machine.bus, 0x244u,
                           (0x32u << 26) | (12u << 21))); /* LWC2 delay slot */
    machine.cpu.pc = 0x240u;
    machine.cpu.next_pc = 0x244u;
    machine.cpu.delay_slot_pending = false;
    machine.cpu.load_delay_pending = false;
    assert(psx_machine_step(&machine));
    assert(psx_machine_step(&machine));
    assert(machine.cop0_epc == 0x240u);
    assert((machine.cop0_cause & PSX_COP0_CAUSE_BD) != 0u);
    assert(machine.cop0_badvaddr == 0x601u);
    assert(((machine.cop0_cause & PSX_COP0_CAUSE_EXCCODE) >>
            PSX_COP0_CAUSE_EXCCODE_SHIFT) == PSX_EXCEPTION_ADDRESS_LOAD);

    /* Misaligned SWC2 in a branch delay slot reports AdES with BD/EPC. */
    machine.cop0_status = 0x40000000u;
    machine.cop0_cause = 0u;
    psx_cpu_write_gpr(&machine.cpu, 12u, 0x603u);
    assert(psx_bus_write32(&machine.bus, 0x248u, 0x10000001u)); /* BEQ zero,zero,+1 */
    assert(psx_bus_write32(&machine.bus, 0x24cu,
                           (0x3au << 26) | (12u << 21))); /* SWC2 delay slot */
    machine.cpu.pc = 0x248u;
    machine.cpu.next_pc = 0x24cu;
    machine.cpu.delay_slot_pending = false;
    machine.cpu.load_delay_pending = false;
    assert(psx_machine_step(&machine));
    assert(psx_machine_step(&machine));
    assert(machine.cop0_epc == 0x248u);
    assert((machine.cop0_cause & PSX_COP0_CAUSE_BD) != 0u);
    assert(machine.cop0_badvaddr == 0x603u);
    assert(((machine.cop0_cause & PSX_COP0_CAUSE_EXCCODE) >>
            PSX_COP0_CAUSE_EXCCODE_SHIFT) == PSX_EXCEPTION_ADDRESS_STORE);

    free(gpu_vram);
    free(bios);
    free(ram);
    puts("DreamPSX machine-bus integration tests: OK");
    return 0;
}
