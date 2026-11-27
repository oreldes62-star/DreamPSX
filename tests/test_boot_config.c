#include "dreampsx/boot_config.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
int main(void) {
 PsxBootConfig c;
 const char a[]="; comment\r\nBOOT = cdrom:\\SLUS_123.45;1\r\n";
 const char b[]="BOOT2=CDROM:\\GAME\\SCUS_999.99;1\n";
 const char bad[]="BOOT=host:game.exe\n";
 const char empty[]="BOOT=cdrom:\\";
 assert(psx_boot_config_parse(a,sizeof(a)-1,&c));
 assert(!c.boot2 && strcmp(c.boot_path,"/SLUS_123.45;1")==0);
 assert(psx_boot_config_parse(b,sizeof(b)-1,&c));
 assert(c.boot2 && strcmp(c.boot_path,"/GAME/SCUS_999.99;1")==0);
 assert(!psx_boot_config_parse(bad,sizeof(bad)-1,&c));
 assert(!psx_boot_config_parse(empty,sizeof(empty)-1,&c));
 puts("boot config tests: OK"); return 0;
}
