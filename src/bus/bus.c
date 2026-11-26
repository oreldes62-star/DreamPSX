#include "dreampsx/bus.h"
#include "dreampsx/irq_mmio.h"

#define PSX_GPU_GP0_ADDR 0x1f801810u
#define PSX_GPU_GP1_ADDR 0x1f801814u

static uint32_t physical_address(uint32_t address)
{
    if ((address >= 0x80000000u && address <= 0x9fffffffu) ||
        (address >= 0xa0000000u && address <= 0xbfffffffu))
        return address & 0x1fffffffu;
    return address;
}

bool psx_bus_init(PsxBus *bus, PsxMemory *memory, PsxIrqController *irq,\n                  PsxGpu *gpu)
{
    if (!bus || !memory || !memory->initialized || !irq || !gpu || !gpu->initialized) return false;
    bus->memory = memory;
    bus->irq = irq;\n    bus->gpu = gpu;
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
    return psx_memory_read16(bus->memory, address, value);
}

bool psx_bus_read32(const PsxBus *bus, uint32_t address, uint32_t *value)
{
    if (!bus || !bus->initialized || !value) return false;
    address = physical_address(address);
    if (address == PSX_GPU_GP1_ADDR)\n        return psx_gpu_read_status(bus->gpu, value);\n    if (psx_irq_mmio_read32(bus->irq, address, value)) return true;
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
    return psx_memory_write16(bus->memory, address, value);
}

bool psx_bus_write32(PsxBus *bus, uint32_t address, uint32_t value)
{
    if (!bus || !bus->initialized) return false;
    address = physical_address(address);
    if (address == PSX_GPU_GP0_ADDR)\n        return psx_gpu_gp0_write(bus->gpu, value);\n    if (address == PSX_GPU_GP1_ADDR)\n        return psx_gpu_gp1_write(bus->gpu, value);\n    if (psx_irq_mmio_write32(bus->irq, address, value)) return true;
    return psx_memory_write32(bus->memory, address, value);
}
