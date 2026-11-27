#include "dreampsx/boot_config.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static void put32(uint8_t *p, uint32_t v) {
 p[0]=(uint8_t)v; p[1]=(uint8_t)(v>>8u); p[2]=(uint8_t)(v>>16u); p[3]=(uint8_t)(v>>24u);
}
static void put_both32(uint8_t *p, uint32_t v) {
 put32(p,v); p[4]=(uint8_t)(v>>24u); p[5]=(uint8_t)(v>>16u); p[6]=(uint8_t)(v>>8u); p[7]=(uint8_t)v;
}
static uint8_t *record(uint8_t *p,uint32_t lba,uint32_t size,uint8_t flags,const uint8_t *name,uint8_t n) {
 uint8_t len=(uint8_t)(33u+n+((n&1u)==0u)); memset(p,0,len); p[0]=len;
 put_both32(p+2u,lba); put_both32(p+10u,size); p[25]=flags;
 p[28]=1u; p[31]=1u; p[32]=n; memcpy(p+33u,name,n); return p+len;
}
int main(void) {
 PsxBootConfig c; PsxIso9660 iso; PsxExeInfo info; PsxMemory memory;
 const char a[]="; comment\r\nBOOT = cdrom:\\SLUS_123.45;1\r\n";
 const char b[]="BOOT2=CDROM:\\GAME\\SCUS_999.99;1\n";
 const char bad[]="BOOT=host:game.exe\n", empty[]="BOOT=cdrom:\\";
 static uint8_t image[26u*PSX_ISO9660_SECTOR_SIZE];
 static uint8_t ram_data[PSX_RAM_SIZE], bios_data[PSX_BIOS_SIZE];
 const uint8_t dot=0u,dotdot=1u,cnf_name[]="SYSTEM.CNF;1",dir_name[]="GAME",exe_name[]="SLUS_123.45;1";
 uint8_t *pvd=image+16u*PSX_ISO9660_SECTOR_SIZE,*root=image+20u*PSX_ISO9660_SECTOR_SIZE,*p;
 uint8_t *game=image+21u*PSX_ISO9660_SECTOR_SIZE,*exe=image+23u*PSX_ISO9660_SECTOR_SIZE;
 const char cnf[]="BOOT = cdrom:\\GAME\\SLUS_123.45;1\r\n";
 assert(psx_boot_config_parse(a,sizeof(a)-1,&c));
 assert(!c.boot2 && strcmp(c.boot_path,"/SLUS_123.45;1")==0);
 assert(psx_boot_config_parse(b,sizeof(b)-1,&c));
 assert(c.boot2 && strcmp(c.boot_path,"/GAME/SCUS_999.99;1")==0);
 assert(!psx_boot_config_parse(bad,sizeof(bad)-1,&c));
 assert(!psx_boot_config_parse(empty,sizeof(empty)-1,&c));
 pvd[0]=1u; memcpy(pvd+1u,"CD001",5u); pvd[6]=1u; put_both32(pvd+80u,26u);
 (void)record(pvd+156u,20u,2048u,2u,&dot,1u);
 record(root,20u,2048u,2u,&dot,1u);
 p=record(root+34u,20u,2048u,2u,&dotdot,1u);
 p=record(p,22u,sizeof(cnf)-1u,0u,cnf_name,sizeof(cnf_name)-1u);
 (void)record(p,21u,2048u,2u,dir_name,sizeof(dir_name)-1u);
 record(game,21u,2048u,2u,&dot,1u);
 p=record(game+34u,20u,2048u,2u,&dotdot,1u);
 (void)record(p,23u,PSX_EXE_HEADER_SIZE+16u,0u,exe_name,sizeof(exe_name)-1u);
 memcpy(image+22u*2048u,cnf,sizeof(cnf)-1u);
 memcpy(exe,"PS-X EXE",8u); put32(exe+0x10u,0x80010000u);
 put32(exe+0x18u,0x80010000u); put32(exe+0x1cu,16u);
 put32(exe+0x30u,0x801fff00u); put32(exe+0x34u,0x100u);
 for(unsigned i=0;i<16u;++i) exe[PSX_EXE_HEADER_SIZE+i]=(uint8_t)(0x40u+i);
 memset(&memory,0,sizeof(memory));
 assert(psx_memory_init(&memory,ram_data,sizeof(ram_data),bios_data,sizeof(bios_data)));
 assert(psx_iso9660_mount(&iso,image,sizeof(image)));
 assert(psx_boot_load_from_iso(&memory,&iso,&c,&info));
 assert(strcmp(c.boot_path,"/GAME/SLUS_123.45;1")==0);
 assert(info.entry_pc==0x80010000u && info.file_size==16u);
 assert(memory.ram[0x10000u]==0x40u && memory.ram[0x1000fu]==0x4fu);
 psx_iso9660_unmount(&iso);
 puts("boot config and ISO executable loading tests: OK"); return 0;
}
