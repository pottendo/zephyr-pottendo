/*
 * Copyright (c) 2019 Tavish Naruka <tavishnaruka@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* Sample which uses the filesystem API and SDHC driver */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/storage/disk_access.h>
#include <zephyr/logging/log.h>
#include <zephyr/fs/fs.h>
#include <ff.h>
#include <stdio.h>

LOG_MODULE_REGISTER(main);

static int lsdir(const char *path);

static FATFS fat_fs;
/* mounting info */
static struct fs_mount_t mp = {
	.type = FS_FATFS,
	.fs_data = &fat_fs,
};

/*
*  Note the fatfs library is able to mount only strings inside _VOLUME_STRS
*  in ffconf.h
*/
static const char *disk_mount_pt = "/SD:";

#define BSIZE 128
static char buf[BSIZE + 1];

int main(void)
{

	/* raw disk i/o */
	do {
		static const char *disk_pdrv = "SD";
		uint64_t memory_size_mb;
		uint32_t block_count;
		uint32_t block_size;

		if (disk_access_init(disk_pdrv) != 0) {
			LOG_ERR("Storage init ERROR!");
			break;
		}

		if (disk_access_ioctl(disk_pdrv,
				DISK_IOCTL_GET_SECTOR_COUNT, &block_count)) {
			LOG_ERR("Unable to get sector count");
			break;
		}
		LOG_INF("Block count %u", block_count);

		if (disk_access_ioctl(disk_pdrv,
				DISK_IOCTL_GET_SECTOR_SIZE, &block_size)) {
			LOG_ERR("Unable to get sector size");
			break;
		}
		printk("Sector size %u\n", block_size);

		memory_size_mb = (uint64_t)block_count * block_size;
		printk("Memory Size(MB) %u\n", (uint32_t)(memory_size_mb >> 20));
	} while (0);

	mp.mnt_point = disk_mount_pt;

	int res = fs_mount(&mp);
	struct fs_file_t fp;
	fs_file_t_init(&fp);
	int ret;
	ret = fs_open(&fp, "/SD:/sudo.con", FS_O_RDWR);
	if (ret != 0)
		printf("open failed - %d.\n", ret);
	do
	{
		ret = fs_read(&fp, buf, BSIZE);
		if (ret > 0)
		{
			buf[ret] = '\0';
			printf("%s", buf);
		}
	}
	while (ret > 0);
	if (ret != 0)
		printf("read() failed: %d\n", ret);
	printf("\n");
	ret = fs_close(&fp);
	if (ret != 0)
		printf("close failed - %d.\n", ret);

	ret = fs_open(&fp, "/SD:/sudo.con", FS_O_RDWR);
	if (ret != 0)
		printf("open failed - %d.\n", ret);
	struct fs_file_t fpw;
	fs_file_t_init(&fpw);
	fs_open(&fpw, "/SD:/CPY", FS_O_CREATE);
	do 
	{
		ret = fs_read(&fp, buf, BSIZE);
		if (ret > 0)
		{
			ret = fs_write(&fpw, buf, ret);
		}
	}
	while (ret > 0);
	if (ret < 0)
		printf("read/write() failed: %d\n", ret);
	ret = fs_close(&fp);
	if (ret != 0)
		printf("close failed - %d.\n", ret);
	ret = fs_close(&fpw);
	if (ret != 0)
		printf("close failed - %d.\n", ret);
	if (res == FR_OK) {
		printk("Disk mounted.\n");
		lsdir(disk_mount_pt);
	} else {
		printk("Error mounting disk.\n");
	}

	while (1) {
		k_sleep(K_MSEC(1000));
	}
}

static int lsdir(const char *path)
{
	int res;
	struct fs_dir_t dirp;
	static struct fs_dirent entry;

	fs_dir_t_init(&dirp);

	/* Verify fs_opendir() */
	res = fs_opendir(&dirp, path);
	if (res) {
		printk("Error opening dir %s [%d]\n", path, res);
		return res;
	}

	printk("\nListing dir %s ...\n", path);
	for (;;) {
		/* Verify fs_readdir() */
		res = fs_readdir(&dirp, &entry);

		/* entry.name[0] == 0 means end-of-dir */
		if (res || entry.name[0] == 0) {
			break;
		}

		if (entry.type == FS_DIR_ENTRY_DIR) {
			printk("[DIR ] %s\n", entry.name);
		} else {
			printk("[FILE] %s (size = %zu)\n",
				entry.name, entry.size);
		}
	}

	/* Verify fs_closedir() */
	fs_closedir(&dirp);

	return res;
}
