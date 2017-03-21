#pragma once

#include "ntfs.h"

ntfs_md* ps3ntfs_prx_mounts(void);
int ps3ntfs_prx_num_mounts(void);

void ps3ntfs_prx_lock(void);
void ps3ntfs_prx_unlock(void);
