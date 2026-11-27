# DMA register bank and bus integration — milestone 3.85

## Scope

This milestone introduces the PlayStation DMA controller's seven channel register bank and connects it to the CPU-visible system bus. It is the register/control foundation only; it does not yet execute DMA transfers.

## Implemented register map

- Seven channel triplets at `0x1f801080`–`0x1f8010ef`: MADR, BCR and CHCR, with a 16-byte stride per channel.
- DPCR at `0x1f8010f0`, including the documented reset value `0x07654321` and a 28-bit writable mask.
- DICR at `0x1f8010f4`, with writable control bits, write-one-to-clear channel flags, and a derived master IRQ flag.
- MADR writes are constrained to the 24-bit physical address field and word alignment.
- Reserved register holes, unaligned accesses and out-of-range channel slots are rejected.

## Machine integration

`PsxMachine` owns a `PsxDmaController`. The controller is initialized/reset with the machine and attached to `PsxBus`; aligned 32-bit CPU bus accesses in the DMA register window are routed to it, including KSEG aliases.

## Tests

`tests/test_dma.c` covers all three channel-2 register fields, DPCR, DICR control/W1C behavior, derived master IRQ logic, reset behavior and invalid register accesses. `tests/test_machine_bus.c` checks access through physical and KSEG1 bus addresses. CMake now builds the DMA module and the dedicated test.

## Deliberate limitations

CHCR is currently a stored register value: setting its start/trigger bits does not launch a transfer. There is no DMA scheduler, bus arbitration, channel priority execution, block-mode loop, linked-list mode, GPU FIFO service, DMA completion flag generation or IRQ delivery yet. Those form the next implementation milestone; software must not assume DMA is operational at this stage.

Tests have been added but have not been compiled or run in this environment.
