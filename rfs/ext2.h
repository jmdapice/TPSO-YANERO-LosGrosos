/*
 * ext2.h
 *
 *  Created on: 06/05/2012
 *      Author: utnso
 */

#ifndef EXT2_H_
#define EXT2_H_

#include <stdint.h>
#include <stdbool.h>

#define EXT2_SUPER_MAGIC 0xEF53

#define EXT2_ROOT_INODE_INDEX 1

// t_ext2_superblock -> state
#define EXT2_VALID_FS 1 //Unmounted cleanly
#define EXT2_ERROR_FS 2 //Errors detected


// t_ext2_superblock -> errors
#define EXT2_ERRORS_CONTINUE 1 //continue as if nothing happened
#define EXT2_ERRORS_RO 2 //remount read-only
#define EXT2_ERRORS_PANIC 3 //cause a kernel panic


// t_ext2_superblock -> creator_os
#define EXT2_OS_LINUX 0 //Linux
#define EXT2_OS_HURD 1 //GNU HURD
#define EXT2_OS_MASIX 2 //MASIX
#define EXT2_OS_FREEBSD 3 //FreeBSD
#define EXT2_OS_LITES 4 //Lites


// t_ext2_superblock -> rev_level
#define EXT2_GOOD_OLD_REV 0 //Revision 0
#define EXT2_DYNAMIC_REV 1 //Revision 1 with variable inode sizes, extended attributes, etc.


// t_ext2_inode -> rev_level
#define EXT2_INODE_HAS_MODE_FLAG(inode, flag) ((inode.i_mode & 0xF000) == flag)

const uint8_t EXT2_INODES_INDIRECTION_LEVEL[] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 2, 3 };

// -- file format --
#define EXT2_IFSOCK 0xC000 //socket
#define EXT2_IFLNK 0xA000 //symbolic link
#define EXT2_IFREG 0x8000 //regular file
#define EXT2_IFBLK 0x6000 //block device
#define EXT2_IFDIR 0x4000 //directory
#define EXT2_IFCHR 0x2000 //character device
#define EXT2_IFIFO 0x1000 //fifo
// -- process execution user/group override --
#define EXT2_ISUID 0x0800 //Set process User ID
#define EXT2_ISGID 0x0400 //Set process Group ID
#define EXT2_ISVTX 0x0200 //sticky bit
// -- access rights --
#define EXT2_IRUSR 0x0100 //user read
#define EXT2_IWUSR 0x0080 //user write
#define EXT2_IXUSR 0x0040 //user execute
#define EXT2_IRGRP 0x0020 //group read
#define EXT2_IWGRP 0x0010 //group write
#define EXT2_IXGRP 0x0008 //group execute
#define EXT2_IROTH 0x0004 //others read
#define EXT2_IWOTH 0x0002 //others write
#define EXT2_IXOTH 0x0001 //others execute


#endif /* EXT2_H_ */
