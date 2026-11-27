#include "dreampsx/bus.h"
#include "dreampsx/irq_mmio.h"

#define PSX_GPU_GP0_ADDR 0x1f801810u
#define PSX_GPU_GP1_ADDR 0x1f801814u

void psx_bus_attach_dma(PsxBus *bus, PsxDmaController *dma)
{
    if (bus && bus->initialized) bus->dma = dma;
}

void psx_bus_attach_cdrom(PsxBus *bus, PsxCdrom *cdrom)
{
    if (bus && bus->initialized) bus->cdrom = cdrom;
}

static uint32_t physical_address(uint32_t address)
{
    if ((address >= 0x80000000u && address <= 0x9fffffffu) ||
        (address >= 0xa0000000u && address <= 0xbfffffffu))
        return address & 0x1fffffffu;
    return address;
}

bool psx_bus_init(PsxBus *bus, PsxMemory *memory, PsxIrqController *irq,
                  PsxGpu *gpu, PsxTimers *timers)
{
    if (!bus || !memory || !memory->initialized || !irq || !gpu || !gpu->initialized || !timers) return false;
    bus->memory = memory;
    bus->irq = irq;
    bus->gpu = gpu;
    bus->timers = timers;
    bus->dma = NULL;
    bus->cdrom = NULL;
    bus->initialized = true;
    return true;
}

bool psx_bus_read8(const PsxBus *bus, uint32_t address, uint8_t *value)
{
    if (!bus || !bus->initialized || !value) return false;
    address = physical_address(address);
    if (bus->cdrom && psx_cdrom_read8(bus->cdrom, address, value)) return true;
    if (bus->spu && psx_spu_read8(bus->spu, address, value)) return true;
    if (psx_irq_mmio_read8(bus->irq, address, value)) return true;
    return psx_memory_read8(bus->memory, address, value);
}

bool psx_bus_read16(const PsxBus *bus, uint32_t address, uint16_t *value)
{
    if (!bus || !bus->initialized || !value) return false;
    address = physical_address(address);
    if (bus->spu && psx_spu_read16(bus->spu, address, value)) return true;
    if (psx_irq_mmio_read16(bus->irq, address, value)) return true;
    if (psx_timers_read16(bus->timers, address, value)) return true;
    return psx_memory_read16(bus->memory, address, value);
}

bool psx_bus_read32(const PsxBus *bus, uint32_t address, uint32_t *value)
{
    if (!bus || !bus->initialized || !value) return false;
    address = physical_address(address);
    if (bus->dma && psx_dma_read32(bus->dma, address, value)) return true;
    if (address == PSX_GPU_GP0_ADDR)
        return psx_gpu_read_data(bus->gpu, value);
    if (address == PSX_GPU_GP1_ADDR)
        return psx_gpu_read_status(bus->gpu, value);
    if (bus->spu && address >= PSX_SPU_BASE_ADDR &&
        address + 2u < PSX_SPU_BASE_ADDR + PSX_SPU_REGISTER_BYTES) {
        uint16_t low, high;
        if (!psx_spu_read16(bus->spu, address, &low) ||
            !psx_spu_read16(bus->spu, address + 2u, &high)) return false;
        *value = (uint32_t)low | ((uint32_t)high << 16u);
        return true;
    }
    if (psx_irq_mmio_read32(bus->irq, address, value)) return true;
    if (psx_timers_read32(bus->timers, address, value)) return true;
    return psx_memory_read32(bus->memory, address, value);
}

bool psx_bus_write8(PsxBus *bus, uint32_t address, uint8_t value)
{
    if (!bus || !bus->initialized) return false;
    address = physical_address(address);
    if (bus->cdrom && psx_cdrom_write8(bus->cdrom, address, value)) {
        psx_irq_set_line(bus->irq, 2u, psx_cdrom_irq_output(bus->cdrom));
        return true;
    }
    if (bus->spu && psx_spu_write8(bus->spu, address, value)) return true;
    if (psx_irq_mmio_write8(bus->irq, address, value)) return true;
    return psx_memory_write8(bus->memory, address, value);
}

bool psx_bus_write16(PsxBus *bus, uint32_t address, uint16_t value)
{
    if (!bus || !bus->initialized) return false;
    address = physical_address(address);
    if (bus->spu && psx_spu_write16(bus->spu, address, value)) return true;
    if (psx_irq_mmio_write16(bus->irq, address, value)) return true;
    if (psx_timers_write16(bus->timers, address, value)) return true;
    return psx_memory_write16(bus->memory, address, value);
}

static bool bus_try_dma(PsxBus *bus)
{
    int selected;
    unsigned channel;
    uint32_t control;
    uint32_t sync_mode;
    bool transfer_ok;
    uint64_t payload_before;
    uint64_t headers_before;

    psx_dma_refresh_queue(bus->dma);
    selected = psx_dma_select_channel(bus->dma,
                                      psx_dma_queued_mask(bus->dma));
    /* Only endpoints implemented in this build may claim the DMA bus. */
    if (selected != 2 && selected != 3) return true;
    channel = (unsigned)selected;
    if (psx_dma_begin_channel(bus->dma) != selected) return true;

    control = bus->dma->channel[channel].chcr;
    sync_mode = (control >> 9u) & 3u;
    payload_before = psx_dma_data_words_transferred(bus->dma);
    headers_before = psx_dma_linked_list_headers_read(bus->dma);

    if (channel == 3u) {
        transfer_ok = sync_mode == 0u &&
            psx_dma_execute_cdrom(bus->dma, bus->memory, bus->cdrom);
    } else if (sync_mode == 0u) {
        transfer_ok = psx_dma_execute_gpu_manual(bus->dma, bus->memory,
                                                 bus->gpu);
    } else if (sync_mode == 1u) {
        transfer_ok = psx_dma_execute_gpu_request(bus->dma, bus->memory,
                                                  bus->gpu);
    } else if (sync_mode == 2u) {
        transfer_ok = psx_dma_execute_gpu_linked_list(bus->dma, bus->memory,
                                                      bus->gpu);
    } else {
        transfer_ok = false;
    }

    if (!transfer_ok) psx_dma_abort_channel(bus->dma, channel);
    psx_dma_record_transfer_event(bus->dma, channel, control,
                                  payload_before, headers_before,
                                  transfer_ok);
    psx_dma_refresh_queue(bus->dma);
    return transfer_ok;
}

bool psx_bus_write32(PsxBus *bus, uint32_t address, uint32_t value)
{
    if (!bus || !bus->initialized) return false;
    address = physical_address(address);
    if (bus->dma && psx_dma_write32(bus->dma, address, value)) {
        bool transfer_ok = true;
        /* Re-arbitrate after any channel is armed or DPCR is changed. */
        if ((address >= PSX_DMA_BASE_ADDR &&
             address < PSX_DMA_BASE_ADDR + 0x70u &&
             ((address - PSX_DMA_BASE_ADDR) & 0x0fu) == 0x08u) ||
            address == PSX_DMA_DPCR_ADDR)
            transfer_ok = bus_try_dma(bus);
        /* PS1 DMA is IRQ source 3. Keep the controller input synchronized
         * after DICR writes and after a channel completion changes its flags. */
        psx_irq_set_line(bus->irq, 3u, psx_dma_irq_output(bus->dma));
        return transfer_ok;
    }
    if (bus->spu && address >= PSX_SPU_BASE_ADDR &&
        address + 2u < PSX_SPU_BASE_ADDR + PSX_SPU_REGISTER_BYTES) {
        return psx_spu_write16(bus->spu, address, (uint16_t)value) &&
               psx_spu_write16(bus->spu, address + 2u,
                               (uint16_t)(value >> 16u));
    }
    if (address == PSX_GPU_GP0_ADDR) {
        const bool ok = psx_gpu_gp0_write(bus->gpu, value);
        if (ok) psx_irq_set_line(bus->irq, 1u,
                                 psx_gpu_irq_output(bus->gpu));
        return ok;
    }
    if (address == PSX_GPU_GP1_ADDR) {
        const bool ok = psx_gpu_gp1_write(bus->gpu, value);
        if (ok) psx_irq_set_line(bus->irq, 1u,
                                 psx_gpu_irq_output(bus->gpu));
        return ok;
    }
    if (psx_irq_mmio_write32(bus->irq, address, value)) return true;
    if (psx_timers_write32(bus->timers, address, value)) return true;
    return psx_memory_write32(bus->memory, address, value);
}
