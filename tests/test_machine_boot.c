#include "dreampsx/machine.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static bool raw_disc_read(void *context, size_t offset, uint8_t *destination, size_t size) {
 const uint8_t *bytes=(const uint8_t *)context;
 if(!bytes || !destination || offset > 25u*2352u || size > 25u*2352u-offset) return false;
 memcpy(destination,bytes+offset,size); return true;
}
static void put32(uint8_t *p,uint32_t v) {
 p[0]=(uint8_t)v; p[1]=(uint8_t)(v>>8u); p[2]=(uint8_t)(v>>16u); p[3]=(uint8_t)(v>>24u);
}
static void put_both32(uint8_t *p,uint32_t v) {
 put32(p,v); p[4]=(uint8_t)(v>>24u); p[5]=(uint8_t)(v>>16u); p[6]=(uint8_t)(v>>8u); p[7]=(uint8_t)v;
}
static uint8_t *record(uint8_t *p,uint32_t lba,uint32_t size,uint8_t flags,const uint8_t *name,uint8_t n) {
 uint8_t len=(uint8_t)(33u+n+((n&1u)==0u)); memset(p,0,len); p[0]=len;
 put_both32(p+2u,lba); put_both32(p+10u,size); p[25]=flags; p[28]=1u; p[31]=1u; p[32]=n; memcpy(p+33u,name,n); return p+len;
}
int main(void) {
 static uint8_t iso_image[25u*PSX_ISO9660_SECTOR_SIZE];
 static uint8_t raw_disc[25u*2352u];
 static const char cue[] = "FILE \"boot.bin\" BINARY\nTRACK 01 MODE1/2352\nINDEX 01 00:00:00\n";
 static uint8_t ram[PSX_RAM_SIZE], reader_ram[PSX_RAM_SIZE], bios[PSX_BIOS_SIZE];
 static uint16_t vram[1024u*512u], reader_vram[1024u*512u];
 PsxMachine machine, reader_machine; PsxBootConfig config;
 PsxDiscImage reader_disc;
 uint8_t sector_data[PSX_DISC_SECTOR_DATA_SIZE];
 const uint8_t dot=0u,dotdot=1u,cnf_name[]="SYSTEM.CNF;1",dir_name[]="GAME",exe_name[]="SLUS_123.45;1";
 const char cnf[]="BOOT = cdrom:\\GAME\\SLUS_123.45;1\r\n";
 uint8_t *pvd=iso_image+16u*2048u,*root=iso_image+20u*2048u,*game=iso_image+21u*2048u,*exe=iso_image+23u*2048u,*p;
 iso_image[0]=0x12u; iso_image[1]=0x34u; iso_image[2]=0x56u; iso_image[3]=0x78u;
 pvd[0]=1u; memcpy(pvd+1u,"CD001",5u); pvd[6]=1u; put_both32(pvd+80u,25u);
 (void)record(pvd+156u,20u,2048u,2u,&dot,1u);
 record(root,20u,2048u,2u,&dot,1u);
 p=record(root+34u,20u,2048u,2u,&dotdot,1u);
 p=record(p,22u,sizeof(cnf)-1u,0u,cnf_name,sizeof(cnf_name)-1u);
 (void)record(p,21u,2048u,2u,dir_name,sizeof(dir_name)-1u);
 record(game,21u,2048u,2u,&dot,1u);
 p=record(game+34u,20u,2048u,2u,&dotdot,1u);
 (void)record(p,23u,PSX_EXE_HEADER_SIZE+4u,0u,exe_name,sizeof(exe_name)-1u);
 memcpy(iso_image+22u*2048u,cnf,sizeof(cnf)-1u);
 memcpy(exe,"PS-X EXE",8u); put32(exe+0x10u,0x80010000u);
 put32(exe+0x14u,0x80018000u); put32(exe+0x18u,0x80010000u);
 put32(exe+0x1cu,4u); put32(exe+0x30u,0x801fff00u); put32(exe+0x34u,0x100u);
 put32(exe+PSX_EXE_HEADER_SIZE,0x2402002au); /* addiu v0, zero, 42 */
 /* Wrap every ISO user-data sector in a raw MODE1/2352 sector. */
 for (unsigned sector = 0u; sector < 25u; ++sector) {
  uint8_t *raw = raw_disc + (size_t)sector * 2352u;
  memset(raw, 0, 2352u);
  for (unsigned sync = 1u; sync <= 10u; ++sync) raw[sync] = 0xffu;
  raw[15] = 1u;
  memcpy(raw + 16u, iso_image + (size_t)sector * PSX_ISO9660_SECTOR_SIZE,
         PSX_ISO9660_SECTOR_SIZE);
 }
 assert(psx_machine_init(&machine,ram,sizeof(ram),bios,sizeof(bios),vram,1024u*512u));
 assert(psx_machine_boot_from_cue(&machine,cue,sizeof(cue)-1u,
                                  raw_disc,sizeof(raw_disc),&config));
 assert(machine.cdrom.disc.mounted && machine.cdrom.iso_data==NULL);
 assert(machine.cdrom.disc.source==raw_disc &&
        machine.cdrom.disc.sector_count==25u);
 assert(strcmp(config.boot_path,"/GAME/SLUS_123.45;1")==0);
 assert(machine.cpu.pc==0x80010000u && machine.cpu.next_pc==0x80010004u);
 assert(psx_cpu_read_gpr(&machine.cpu,28u)==0x80018000u);
 assert(psx_cpu_read_gpr(&machine.cpu,29u)==0x80200000u);
 assert(psx_machine_step(&machine));
 assert(psx_cpu_read_gpr(&machine.cpu,2u)==42u);
 assert(machine.cpu.pc==0x80010004u);
 assert(psx_cdrom_write8(&machine.cdrom,PSX_CDROM_BASE_ADDR,0u));
 assert(psx_cdrom_write8(&machine.cdrom,PSX_CDROM_BASE_ADDR+2u,0x00u));
 assert(psx_cdrom_write8(&machine.cdrom,PSX_CDROM_BASE_ADDR+2u,0x02u));
 assert(psx_cdrom_write8(&machine.cdrom,PSX_CDROM_BASE_ADDR+2u,0x00u));
 assert(psx_cdrom_write8(&machine.cdrom,PSX_CDROM_BASE_ADDR+1u,0x02u));
 assert(psx_cdrom_write8(&machine.cdrom,PSX_CDROM_BASE_ADDR+1u,0x06u));
 assert(psx_cdrom_write8(&machine.cdrom,PSX_CDROM_BASE_ADDR,2u));
 assert(psx_cdrom_write8(&machine.cdrom,PSX_CDROM_BASE_ADDR+2u,0x80u));
 { uint32_t sector_word=0u;
   assert(psx_cdrom_read_data32(&machine.cdrom,&sector_word));
   assert(sector_word==0x78563412u);
 }
 /* Repeat the same complete boot through a file-style random-access reader.
    This is the path used by the KallistiOS VFS CUE adapter. */
 assert(psx_disc_mount_reader(&reader_disc,sizeof(raw_disc),PSX_DISC_MODE1_2352,
                              raw_disc_read,raw_disc));
 assert(psx_machine_init(&reader_machine,reader_ram,sizeof(reader_ram),bios,sizeof(bios),
                         reader_vram,1024u*512u));
 assert(psx_machine_boot_from_disc_image(&reader_machine,&reader_disc,&config));
 assert(reader_machine.cdrom.disc.mounted);
 assert(reader_machine.cdrom.disc.source==NULL);
 assert(reader_machine.cdrom.disc.read_at==raw_disc_read);
 assert(reader_machine.cpu.pc==0x80010000u);
 assert(psx_machine_step(&reader_machine));
 assert(psx_cpu_read_gpr(&reader_machine.cpu,2u)==42u);
 assert(psx_disc_read_sector(&reader_machine.cdrom.disc,0u,sector_data));
 assert(sector_data[0]==0x12u && sector_data[1]==0x34u &&
        sector_data[2]==0x56u && sector_data[3]==0x78u);
 puts("machine CUE and random-access file-backed boot tests: OK");
 return 0;
}
