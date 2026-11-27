#include "dreampsx/bus.h"
#include "dreampsx/irq_mmio.h"

#define PSX_GPU_GP0_ADDR 0x1f801810u
#define PSX_GPU_GP1_ADDR 0x1f801814u

void psx_bus_attach_dma(PsxBus *bus, PsxDmaController *dma)
{
    if (bus && bus->initialized) bus->dma = dma;
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
    bus->initialized = true;
    return true;
}

bool psx_bus_read8(const PsxBus *bus, uint32_t address, uint8_t *value)
{
    if (!bus || !bus->initialized || !value) return false;
    address = physical_address(address);
    if (psx_irq_mmio_read8(bus->irq, address, value)) return true;
    return psx_memory_read8(bus->memory, address, value);
}

bool psx_bus_read16(const PsxBus *bus, uint32_t address, uint16_t *value)
{
    if (!bus || !bus->initialized || !value) return false;
    address = physical_address(address);
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
    if (psx_irq_mmio_read32(bus->irq, address, value)) return true;
    if (psx_timers_read32(bus->timers, address, value)) return true;
    return psx_memory_read32(bus->memory, address, value);
}

bool psx_bus_write8(PsxBus *bus, uint32_t address, uint8_t value)
{
    if (!bus || !bus->initialized) return false;
    address = physical_address(address);
    if (psx_irq_mmio_write8(bus->irq, address, value)) return true;
    return psx_memory_write8(bus->memory, address, value);
}

bool psx_bus_write16(PsxBus *bus, uint32_t address, uint16_t value)
{
    if (!bus || !bus->initialized) return false;
    address = physical_address(address);
    if (psx_irq_mmio_write16(bus->irq, address, value)) return true;
    if (psx_timers_write16(bus->timers, address, value)) return true;
    return psx_memory_write16(bus->memory, address, value);
}

static bool bus_try_gpu_dma(PsxBus *bus)
{
    int selected;
    uint32_t control;
    uint32_t sync_mode;
    bool transfer_ok;

    /* CHCR remains the source of truth; DPCR only controls arbitration. */
    psx_dma_refresh_queue(bus->dma);
    selected = psx_dma_select_channel(bus->dma,
                                      psx_dma_queued_mask(bus->dma));
    /* Other DMA channels are retained in the queue until their device
     * endpoints exist. They must not be silently consumed by the GPU path. */
    if (selected != 2) return true;

    control = bus->dma->channel[2].chcr;
    sync_mode = (control >> 9u) & 3u;
    if (sync_mode == 0u)
        transfer_ok = psx_dma_execute_gpu_manual(bus->dma, bus->memory,
                                                 bus->gpu);
    else if (sync_mode == 1u)
        transfer_ok = psx_dma_execute_gpu_request(bus->dma, bus->memory,
                                                  bus->gpu);
    else if (sync_mode == 2u)
        transfer_ok = psx_dma_execute_gpu_linked_list(bus->dma, bus->memory,
                                                      bus->gpu);
    else
        transfer_ok = true;

    /* A request stays queued if the device could not complete it. */
    psx_dma_refresh_queue(bus->dma);
    return transfer_ok;
}

bool psx_bus_write32(PsxBus *bus, uint32_t address, uint32_t value)
{
    if (!bus || !bus->initialized) return false;
    address = physical_address(address);
    if (bus->dma && psx_dma_write32(bus->dma, address, value)) {
        bool transfer_ok = true;
        /* A channel can be armed before DPCR enables it. Re-check channel 2
         * when either CHCR is programmed or its DPCR enable bit changes. */
        if ((address >= PSX_DMA_BASE_ADDR &&
             address < PSX_DMA_BASE_ADDR + 0x70u &&
             ((address - PSX_DMA_BASE_ADDR) & 0x0fu) == 0x08u) ||
            address == PSX_DMA_DPCR_ADDR)
            transfer_ok = bus_try_gpu_dma(bus);
        /* PS1 DMA is IRQ source 3. Keep the controller input synchronized
         * after DICR writes and after a channel completion changes its flags. */
        psx_irq_set_line(bus->irq, 3u, psx_dma_irq_output(bus->dma));
        return transfer_ok;
    }
    if (address == PSX_GPU_GP0_ADDR)
        return psx_gpu_gp0_write(bus->gpu, value);
    if (address == PSX_GPU_GP1_ADDR)
        return psx_gpu_gp1_write(bus->gpu, value);
    if (psx_irq_mmio_write32(bus->irq, address, value)) return true;
    if (psx_timers_write32(bus->timers, address, value)) return true;
    return psx_memory_write32(bus->memory, address, value);
}
