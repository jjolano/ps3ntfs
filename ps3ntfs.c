#include <stddef.h>
#include <stdbool.h>
#include <inttypes.h>

#include <sys/spinlock.h>
#include <sys/types.h>

#include "vsh_exports.h"

#include "ntfs.h"
#include "ps3ntfs.h"

int spinlock_id;
bool show_msgs = false;

ntfs_md* mounts = NULL;
int num_mounts = 0;

extern bool prx_running;

ntfs_md* ps3ntfs_prx_mounts(void)
{
	return mounts;
}

int ps3ntfs_prx_num_mounts(void)
{
	return num_mounts;
}

void ps3ntfs_prx_lock(void)
{
	sys_spinlock_lock(&spinlock_id);
}

void ps3ntfs_prx_unlock(void)
{
	sys_spinlock_unlock(&spinlock_id);
}

void ps3ntfs_automount(uint64_t ptr)
{
	sys_spinlock_initialize(&spinlock_id);

	bool is_mounted[8];
	memset(is_mounted, false, sizeof(is_mounted));

	const DISC_INTERFACE* ntfs_usb_if[8] = {
		&__io_ntfs_usb000,
		&__io_ntfs_usb001,
		&__io_ntfs_usb002,
		&__io_ntfs_usb003,
		&__io_ntfs_usb004,
		&__io_ntfs_usb005,
		&__io_ntfs_usb006,
		&__io_ntfs_usb007
	};
	
	while(prx_running)
	{
		// Iterate ports and mount NTFS.
		int i;
		for(i = 0; i < 8; i++)
		{
			if(PS3_NTFS_IsInserted(i))
			{
				if(!is_mounted[i])
				{
					sec_t* partitions = NULL;
					int num_partitions = ntfsFindPartitions(ntfs_usb_if[i], &partitions);

					if(num_partitions > 0 && partitions)
					{
						sys_spinlock_lock(&spinlock_id);

						int j;
						for(j = 0; j < num_partitions; j++)
						{
							char name[32];
							sprintf(name, "ntfs%d-usb%03d", j, i);

							if(ntfsMount(name, ntfs_usb_if[i], partitions[j], CACHE_DEFAULT_PAGE_COUNT, CACHE_DEFAULT_PAGE_SIZE, NTFS_FORCE))
							{
								// add to mount struct
								mounts = (ntfs_md*) realloc(mounts, ++num_mounts * sizeof(ntfs_md));

								ntfs_md* mount = &mounts[num_mounts - 1];

								strcpy(mount->name, name);
								mount->interface = ntfs_usb_if[i];
								mount->startSector = partitions[j];
							}
						}

						sys_spinlock_unlock(&spinlock_id);

						free(partitions);
					}

					if(show_msgs)
					{
						char msg[256];
						sprintf(msg, "Mounted /dev_usb%03d", i);
						vshtask_notify(msg);
					}

					is_mounted[i] = true;
				}
			}
			else
			{
				if(is_mounted[i])
				{
					sys_spinlock_lock(&spinlock_id);

					int j;
					for(j = 0; j < num_mounts; j++)
					{
						// unmount all partitions of this device
						if(mounts[j].interface == ntfs_usb_if[i])
						{
							ntfsUnmount(mounts[j].name, true);

							// realloc
							memmove(&mounts[j], &mounts[j + 1], (num_mounts - j - 1) * sizeof(ntfs_md));
							mounts = (ntfs_md*) realloc(mounts, --num_mounts * sizeof(ntfs_md));

							--j;
						}
					}

					sys_spinlock_unlock(&spinlock_id);

					if(show_msgs)
					{
						char msg[256];
						sprintf(msg, "Unmounted /dev_usb%03d", i);
						vshtask_notify(msg);
					}

					is_mounted[i] = false;
				}
			}
		}

		sys_timer_sleep(1);
	}

	// Unmount NTFS.
	sys_spinlock_lock(&spinlock_id);

	while(num_mounts-- > 0)
	{
		ntfsUnmount(mounts[num_mounts].name, true);
	}

	if(mounts != NULL)
	{
		free(mounts);
		mounts = NULL;
	}

	sys_spinlock_unlock(&spinlock_id);
	
	sys_ppu_thread_exit(0);
}
