#include "dreampsx/machine.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
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
 static uint8_t ram[PSX_RAM_SIZE], bios[PSX_BIOS_SIZE];
 static uint16_t vram[1024u*512u];
 PsxMachine machine; PsxBootConfig config;
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
 assert(psx_machine_init(&machine,ram,sizeof(ram),bios,sizeof(bios),vram,1024u*512u));
 assert(psx_machine_boot_from_disc(&machine,iso_image,sizeof(iso_image),&config));
 assert(machine.cdrom.iso_data==iso_image && machine.cdrom.iso_size==sizeof(iso_image));
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
 puts("machine ISO boot and first instruction tests: OK");
 return 0;
}
