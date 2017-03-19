#include "prx.h"
#include "ntfs.h"
#include "ps3ntfs.h"

SYS_MODULE_START(prx_start);
SYS_MODULE_STOP(prx_stop);
SYS_MODULE_EXIT(prx_exit);
SYS_MODULE_INFO(NTFSD, 0, 0, 1);

SYS_LIB_DECLARE_WITH_STUB(NTFSD, SYS_LIB_AUTO_EXPORT, libps3ntfs_prx);

SYS_LIB_EXPORT(ps3ntfs_prx_mounts, NTFSD);
SYS_LIB_EXPORT(ps3ntfs_prx_num_mounts, NTFSD);

/* ntfs.h */
SYS_LIB_EXPORT(ntfsFindPartitions, NTFSD);
SYS_LIB_EXPORT(ntfsMountAll, NTFSD);
SYS_LIB_EXPORT(ntfsMountDevice, NTFSD);
SYS_LIB_EXPORT(ntfsMount, NTFSD);
SYS_LIB_EXPORT(ntfsUnmount, NTFSD);
SYS_LIB_EXPORT(ntfsGetVolumeName, NTFSD);
SYS_LIB_EXPORT(ntfsSetVolumeName, NTFSD);

SYS_LIB_EXPORT(ps3ntfs_open, NTFSD);
SYS_LIB_EXPORT(ps3ntfs_close, NTFSD);
SYS_LIB_EXPORT(ps3ntfs_write, NTFSD);
SYS_LIB_EXPORT(ps3ntfs_read, NTFSD);
SYS_LIB_EXPORT(ps3ntfs_seek, NTFSD);
SYS_LIB_EXPORT(ps3ntfs_seek64, NTFSD);
SYS_LIB_EXPORT(ps3ntfs_fstat, NTFSD);
SYS_LIB_EXPORT(ps3ntfs_stat, NTFSD);
SYS_LIB_EXPORT(ps3ntfs_link, NTFSD);
SYS_LIB_EXPORT(ps3ntfs_unlink, NTFSD);
SYS_LIB_EXPORT(ps3ntfs_chdir, NTFSD);
SYS_LIB_EXPORT(ps3ntfs_rename, NTFSD);
SYS_LIB_EXPORT(ps3ntfs_mkdir, NTFSD);
SYS_LIB_EXPORT(ps3ntfs_file_to_sectors, NTFSD);
//SYS_LIB_EXPORT(ps3ntfs_get_fd_from_FILE, NTFSD);
SYS_LIB_EXPORT(ps3ntfs_diropen, NTFSD);
SYS_LIB_EXPORT(ps3ntfs_dirreset, NTFSD);
SYS_LIB_EXPORT(ps3ntfs_dirnext, NTFSD);
SYS_LIB_EXPORT(ps3ntfs_dirclose, NTFSD);

SYS_LIB_EXPORT(NTFS_init_system_io, NTFSD);
SYS_LIB_EXPORT(NTFS_deinit_system_io, NTFSD);

SYS_LIB_EXPORT(ps3ntfs_statvfs, NTFSD);
SYS_LIB_EXPORT(ps3ntfs_ftruncate, NTFSD);
SYS_LIB_EXPORT(ps3ntfs_fsync, NTFSD);
SYS_LIB_EXPORT(ps3ntfs_errno, NTFSD);

SYS_LIB_EXPORT(PS3_NTFS_IsInserted, NTFSD);
SYS_LIB_EXPORT(PS3_NTFS_Shutdown, NTFSD);

ntfs_md* mounts = NULL;
int num_mounts = 0;
sys_ppu_thread_t prx_tid;
bool prx_running = false;

ntfs_md** ps3ntfs_prx_mounts(void)
{
	return &mounts;
}

int* ps3ntfs_prx_num_mounts(void)
{
	return &num_mounts;
}

inline void _sys_ppu_thread_exit(uint64_t val)
{
	system_call_1(41, val);
}

void finalize_module(void)
{
	uint64_t meminfo[5];

	sys_prx_id_t prx = sys_prx_get_module_id_by_address(finalize_module);

	meminfo[0] = 0x28;
	meminfo[1] = 2;
	meminfo[3] = 0;

	system_call_3(482, prx, 0, (uintptr_t) meminfo);
}

void prx_unload(void)
{
	sys_prx_id_t prx = sys_prx_get_module_id_by_address(prx_unload);
	system_call_3(483, prx, 0, NULL);
}

int prx_stop(void)
{
	if(prx_running)
	{
		prx_running = false;

		uint64_t prx_exitcode;
		sys_ppu_thread_join(prx_tid, &prx_exitcode);
	}
	
	finalize_module();
	_sys_ppu_thread_exit(0);
	return SYS_PRX_STOP_OK;
}

int prx_exit(void)
{
	if(prx_running)
	{
		prx_running = false;

		uint64_t prx_exitcode;
		sys_ppu_thread_join(prx_tid, &prx_exitcode);
	}

	prx_unload();
	_sys_ppu_thread_exit(0);
	return SYS_PRX_STOP_OK;
}

void prx_main(uint64_t ptr)
{
	prx_running = true;
	
	while(prx_running)
	{
		sys_ppu_thread_yield();
		sys_timer_sleep(1);
	}

	sys_ppu_thread_yield();
	sys_timer_sleep(1);

	// Unmount NTFS.
	while(num_mounts-- > 0)
	{
		ntfsUnmount(mounts[num_mounts].name, true);
	}

	if(mounts != NULL)
	{
		free(mounts);
	}
	
	sys_ppu_thread_exit(0);
}

int prx_start(size_t args, void* argv)
{
	if(sys_ppu_thread_create(&prx_tid, prx_main, 0, 1000, 0x1000, SYS_PPU_THREAD_CREATE_JOINABLE, (char*) "ps3ntfs") != 0)
	{
		finalize_module();
		_sys_ppu_thread_exit(SYS_PRX_NO_RESIDENT);
		return SYS_PRX_NO_RESIDENT;
	}

	_sys_ppu_thread_exit(SYS_PRX_START_OK);
	return SYS_PRX_START_OK;
}
