#include "dreampsx/boot_config.h"
#include <string.h>
static char upper_ascii(char c) { return c >= 'a' && c <= 'z' ? (char)(c - 32) : c; }
static bool equal_key(const char *p, size_t n, const char *key) {
 size_t i; if (n != strlen(key)) return false;
 for (i=0;i<n;++i) if (upper_ascii(p[i]) != key[i]) return false;
 return true;
}
bool psx_boot_config_parse(const char *text, size_t size, PsxBootConfig *config) {
 size_t pos=0; if (!text || !config) return false; memset(config,0,sizeof(*config));
 while (pos<size) {
  size_t end=pos, eq, key_end, start, value_end, n; bool boot2;
  while(end<size && text[end]!='\n') ++end;
  eq=pos; while(eq<end && text[eq]!='=') ++eq;
  if(eq<end) {
   key_end=eq; while(key_end>pos && (text[key_end-1]==' ' || text[key_end-1]=='\t')) --key_end;
   boot2=equal_key(text+pos,key_end-pos,"BOOT2");
   if(boot2 || equal_key(text+pos,key_end-pos,"BOOT")) {
    start=eq+1; while(start<end && (text[start]==' ' || text[start]=='\t')) ++start;
    value_end=end; while(value_end>start && (text[value_end-1]=='\r' || text[value_end-1]==' ' || text[value_end-1]=='\t')) --value_end;
    if(value_end-start<7 || upper_ascii(text[start])!='C' || upper_ascii(text[start+1])!='D' ||
       upper_ascii(text[start+2])!='R' || upper_ascii(text[start+3])!='O' ||
       upper_ascii(text[start+4])!='M' || text[start+5]!=':' || text[start+6]!='\\') return false;
    n=value_end-(start+6);
    if(n==1 || n>=sizeof(config->boot_path)) return false;
    config->boot_path[0]='/'; memcpy(config->boot_path+1,text+start+7,n-1);
    config->boot_path[n]='\0'; config->boot2=boot2; return true;
   }
  }
  pos=end+(end<size);
 }
 return false;
}
bool psx_boot_config_resolve(const PsxIso9660 *iso, PsxBootConfig *config, PsxIso9660File *exe) {
 PsxIso9660File cnf; char data[2048];
 if(!iso || !config || !exe || !psx_iso9660_lookup(iso,"/SYSTEM.CNF",&cnf) ||
    cnf.directory || cnf.size==0 || cnf.size>sizeof(data) ||
    !psx_iso9660_read(iso,&cnf,0,data,cnf.size) ||
    !psx_boot_config_parse(data,cnf.size,config)) return false;
 return psx_iso9660_lookup(iso,config->boot_path,exe) && !exe->directory;
}
