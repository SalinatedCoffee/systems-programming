/**
 * Finding Filesystems
 * CS 241 - Spring 2019
 */
#include "minixfs.h"
#include "minixfs_utils.h"
#include <errno.h>
#include <stdio.h>
#include <string.h>

/**
 * Virtual paths:
 *  Add your new virtual endpoint to minixfs_virtual_path_names
 */
char *minixfs_virtual_path_names[] = {"info", /* add your paths here*/};

/**
 * Forward declaring block_info_string so that we can attach unused on it
 * This prevents a compiler warning if you haven't used it yet.
 *
 * This function generates the info string that the virtual endpoint info should
 * emit when read
 */
static char *block_info_string(ssize_t num_used_blocks) __attribute__((unused));
static char *block_info_string(ssize_t num_used_blocks) {
    char *block_string = NULL;
    ssize_t curr_free_blocks = DATA_NUMBER - num_used_blocks;
    asprintf(&block_string, "Free blocks: %zd\n"
                            "Used blocks: %zd\n",
             curr_free_blocks, num_used_blocks);
    return block_string;
}

// Don't modify this line unless you know what you're doing
int minixfs_virtual_path_count =
    sizeof(minixfs_virtual_path_names) / sizeof(minixfs_virtual_path_names[0]);

int minixfs_chmod(file_system *fs, char *path, int new_permissions) {
	inode *node = get_inode(fs, path);
	new_permissions &= 0777;
	if(node) {
		uint16_t orig_mode = node->mode;
		orig_mode = (orig_mode >> RWX_BITS_NUMBER) << RWX_BITS_NUMBER;
		orig_mode |= new_permissions;
		node->mode = orig_mode;
		clock_gettime(CLOCK_REALTIME, &node->ctim);
		return 0;
	}
	else {
		errno = ENOENT;
		return -1;
	}
}

int minixfs_chown(file_system *fs, char *path, uid_t owner, gid_t group) {
	inode *node = get_inode(fs, path);
	if(node) {
		if(owner != (uid_t) -1) {
			node->uid = owner;
		}
		if(group != (gid_t) -1) {
			node->gid = group;
		}
		clock_gettime(CLOCK_REALTIME, &node->ctim);
		return 0;
	}
	else {
		errno = ENOENT;
		return -1;
	}
}

inode *minixfs_create_inode_for_path(file_system *fs, const char *path) {
	inode *node = get_inode(fs, path);
	if(node) {
		return NULL;
	}
	else {
		const char *filename;
		inode *parent = parent_directory(fs, path, &filename);
		char *filename_r = strdup(filename);
		inode_number new_inode_num = first_unused_inode(fs);
		if(new_inode_num == -1) {
			return NULL;
		}
		inode *new_inode = fs->inode_root + new_inode_num;
		minixfs_dirent new_inode_dirent;
		new_inode_dirent.name = filename_r;
		new_inode_dirent.inode_num = new_inode_num;
		int block_offset = (int) (parent->size / sizeof(data_block));
		int target_offset = parent->size % sizeof(data_block);
		char *pos = NULL;
		data_block_number target_block = 0;
		if(!target_offset) {	// if we need a new data block
			data_block_number new_block = add_data_block_to_inode(fs, parent);
			if(new_block == -1) {
				if(parent->indirect == UNASSIGNED_NODE) { // assign single ind block if none
					new_block = add_single_indirect_block(fs, parent);
					if(new_block == -1) { return NULL; } // failed to assign single ind block
				}
				new_block = add_data_block_to_indirect_block(fs, &new_block);
				if(new_block == -1) { return NULL; }
			}
			target_block = new_block;
		}
		if(!target_block) {
			if(block_offset < NUM_DIRECT_INODES) {
				target_block = parent->direct[block_offset];
				pos = (char*) (fs->data_root + target_block);
			}
			else {
				block_offset -= NUM_DIRECT_INODES;
				inode_number *inode_pos = (inode_number*) (fs->data_root + parent->indirect);
				pos = (char*) (fs->data_root + inode_pos[block_offset]);
			}
		}
		else {
			pos = (char*) (fs->data_root + target_block);
		}
		pos += target_offset;
		make_string_from_dirent(pos, new_inode_dirent);
		parent->size += 256;
		clock_gettime(CLOCK_REALTIME, &parent->atim);
		clock_gettime(CLOCK_REALTIME, &parent->mtim);
		clock_gettime(CLOCK_REALTIME, &parent->ctim);
		init_inode(parent, new_inode);
		return new_inode;
	}
}

ssize_t minixfs_virtual_read(file_system *fs, const char *path, void *buf,
                             size_t count, off_t *off) {
    if (!strcmp(path, "info")) {
        // TODO implement the "info" virtual file here
    }
    // TODO implement your own virtual file here
    errno = ENOENT;
    return -1;
}

ssize_t minixfs_write(file_system *fs, const char *path, const void *buf,
                      size_t count, off_t *off) {
	if(count + *off > (NUM_INDIRECT_INODES + NUM_DIRECT_INODES) * sizeof(data_block)) {
		errno = ENOSPC;
		return -1;
	}
	inode *node = get_inode(fs, path);
	if(!node) {
		node = minixfs_create_inode_for_path(fs, path);
	}
	int num_req_blocks = (int) ((count + *off) / sizeof(data_block));
	int err = minixfs_min_blockcount(fs, path, num_req_blocks);
	if(err == -1) {
		errno = ENOSPC;
		return -1;
	}
	size_t start_blknum = (size_t) (*off / sizeof(data_block));
	size_t start_indnum = *off % sizeof(data_block);
	char *pos;
	if(start_blknum < NUM_DIRECT_INODES) {
		pos = ((char*) (fs->data_root + node->direct[start_blknum])) + start_indnum;
	}
	return count;
}

ssize_t minixfs_read(file_system *fs, const char *path, void *buf, size_t count,
                     off_t *off) {
    const char *virtual_path = is_virtual_path(path);
    if (virtual_path)
        return minixfs_virtual_read(fs, virtual_path, buf, count, off);
    // 'ere be treasure!
    return -1;
}





































