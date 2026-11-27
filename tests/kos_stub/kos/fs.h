#ifndef KOS_FS_H
#define KOS_FS_H
#include <stddef.h>
#include <sys/types.h>
typedef int file_t;
#define FILEHND_INVALID (-1)
file_t fs_open(const char *path, int mode);
int fs_close(file_t handle);
ssize_t fs_read(file_t handle, void *buffer, size_t count);
off_t fs_seek(file_t handle, off_t offset, int whence);
ssize_t fs_total(file_t handle);
#endif
