// NAME: Jongyun Park, William Chong
// EMAIL: jp101200@g.ucla.edu, williamchong256@gmail.com
// ID: 505096047, 205114665

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <limits.h>
#include <stdint.h>
#include <errno.h>
#include <time.h>
#include <fcntl.h>
#include <getopt.h>
#include <sys/stat.h>

#include "ext2_fs.h"

int img;

struct ext2_super_block superblock;
struct ext2_dir_entry dir_entry;
struct ext2_inode inode;
struct ext2_group_desc group_desc;

unsigned int block_size;
unsigned int block_bitmap;
unsigned int inode_bitmap;
unsigned int inode_table;

void Pread(int d, void *buf, size_t nbytes, off_t offset)
{
    if (pread(d, buf, nbytes, offset) < 0)
    {
        fprintf(stderr, "Unable to pread() summary!\n");
        exit(1);
    }
}

unsigned long block_offset(unsigned int block)
{
    return 1024 + (block - 1) * block_size;
}

void getTime(time_t real_time, char *buffer)
{
    // http://www.cplusplus.com/reference/ctime/
    time_t elapse = real_time;
    struct tm time = *gmtime(&elapse);
    strftime(buffer, 80, "%m/%d/%y %H:%M:%S", &time);
}

void superblock_summary()
{
    //  A single new-line terminated line, comprised of eight comma-separated fields (with no white-space), summarizing the key file system parameters:

    // SUPERBLOCK
    // total number of blocks (decimal)
    // total number of i-nodes (decimal)
    // block size (in bytes, decimal)
    // i-node size (in bytes, decimal)
    // blocks per group (decimal)
    // i-nodes per group (decimal)
    // first non-reserved i-node (decimal)
    if (pread(img, &superblock, sizeof(superblock), 1024) < 0)
    {
        fprintf(stderr, "Unable to read super block summary");
        exit(1);
    }

    fprintf(stdout, "SUPERBLOCK,%d,%d,%d,%d,%d,%d,%d\n",
            superblock.s_blocks_count,
            superblock.s_inodes_count,
            block_size,
            superblock.s_inode_size,
            superblock.s_blocks_per_group,
            superblock.s_inodes_per_group,
            superblock.s_first_ino);
}

void free_block_summary(int group, unsigned int block)
{
    // Scan the free block bitmap for each group. For each free block, produce a new-line terminated line, with two comma-separated fields (with no white space).

    // BFREE
    // number of the free block (decimal)

    // Take care to verify that you:

    // understand whether 1 means allocated or free.
    // have correctly understood the block number to which the first bit corresponds.
    // know how many blocks are in each group, and do not interpret more bits than there are blocks in the group.
    char *read_bytes = (char *)malloc(block_size);
    if (read_bytes == NULL)
    {
        fprintf(stderr, "Unable to malloc for free block summary \n");
        exit(1);
    }
    unsigned long offset = block_offset(block);
    if (pread(img, read_bytes, block_size, offset) < 0)
    {
        fprintf(stderr, "Unable to read free block summary");
        exit(1);
    }
    unsigned int read = superblock.s_first_data_block + (group * superblock.s_blocks_per_group);
    unsigned int i = 0;
    unsigned int j;
    while (i < block_size)
    {
        j = 0;
        char x = read_bytes[i];
        while (j < 8)
        {
            int allocated = 1 & x;
            if (!allocated)
            {
                fprintf(stdout, "BFREE,%d\n", read);
            }
            j++;
            x = x >> 1;
            read++;
        }
        i++;
    }
    free(read_bytes);
}

void directory_entries_summary(unsigned int prev_inode, unsigned int block_num)
{
    // For each directory I-node, scan every data block. For each valid (non-zero I-node number) directory entry, produce a new-line terminated line, with seven comma-separated fields (no white space).

    // DIRENT
    // parent inode number (decimal) ... the I-node number of the directory that contains this entry
    // logical byte offset (decimal) of this entry within the directory
    // inode number of the referenced file (decimal)
    // entry length (decimal)
    // name length (decimal)
    // name (string, surrounded by single-quotes). Dont worry about escaping, we promise there will be no single-quotes or commas in any of the file names.
    unsigned long offset = block_offset(block_num);
    unsigned int tot_bytes = 0;
    unsigned int directory_size = sizeof(dir_entry);

    while (tot_bytes < block_size)
    {
        memset(dir_entry.name, 0, 256);
        if (pread(img, &dir_entry, directory_size, offset + tot_bytes) >= 0 && dir_entry.inode != 0)
        {
            memset(&dir_entry.name[dir_entry.name_len], 0, 256 - dir_entry.name_len);
            fprintf(stdout, "DIRENT,%d,%d,%d,%d,%d,'%s'\n",
                    prev_inode,
                    tot_bytes,
                    dir_entry.inode,
                    dir_entry.rec_len,
                    dir_entry.name_len,
                    dir_entry.name);
        }
        tot_bytes += dir_entry.rec_len;
    }
}

void single_indirect(char file_type, unsigned int inode_number)
{
    unsigned int uint_size = sizeof(uint32_t);
    uint32_t tot_pointer = block_size / uint_size;
    uint32_t *block_pointer = malloc(block_size);

    unsigned long indirect_offset = block_offset(inode.i_block[12]);
    Pread(img, block_pointer, block_size, indirect_offset);
    uint32_t block_number = inode.i_block[12];

    unsigned int i = 0;
    while (i < tot_pointer)
    {
        if (block_pointer[i] != 0)
        {
            if (file_type == 'd')
            {
                directory_entries_summary(inode_number, block_pointer[i]);
            }

            fprintf(stdout, "INDIRECT,%d,%d,%d,%d,%d\n",
                    inode_number,
                    1,
                    i + 12,
                    block_number,
                    block_pointer[i]);
        }
        i++;
    }

    free(block_pointer);
}

void double_indirect(char file_type, unsigned int inode_number)
{
    // For each non-zero block pointer you find, produce a new-line terminated line with six comma-separated fields (no white space).

    // INDIRECT
    // I-node number of the owning file (decimal)
    // (decimal) level of indirection for the block being scanned ... 1 for single indirect, 2 for double indirect, 3 for triple
    // logical block offset (decimal) represented by the referenced block. If the referenced block is a data block, this is the logical block offset of that block within the file. If the referenced block is a single- or double-indirect block, this is the same as the logical offset of the first data block to which it refers.
    // block number of the (1, 2, 3) indirect block being scanned (decimal) . . . not the highest level block (in the recursive scan), but the lower level block that contains the block reference reported by this entry.
    // block number of the referenced block (decimal)
    uint32_t *indirect_block_pointer = malloc(block_size);
    uint32_t tot_pointer = block_size / sizeof(uint32_t);
    uint32_t block_number = inode.i_block[13];

    unsigned long double_indirect_offset = block_offset(inode.i_block[13]);

    Pread(img, indirect_block_pointer, block_size, double_indirect_offset);

    unsigned int i = 0;
    while (i < tot_pointer)
    {
        if (indirect_block_pointer[i] != 0)
        {
            fprintf(stdout, "INDIRECT,%d,%d,%d,%d,%d\n",
                    inode_number,
                    2,
                    i + 256 + 12,
                    block_number,
                    indirect_block_pointer[i]);

            unsigned long indirect_offset = block_offset(indirect_block_pointer[i]);
            uint32_t *block_ptrs = malloc(block_size);
            Pread(img, block_ptrs, block_size, indirect_offset);

            unsigned int j = 0;
            while (j < tot_pointer)
            {
                if (block_ptrs[j] != 0)
                {
                    if (file_type == 'd')
                    {
                        directory_entries_summary(inode_number, block_ptrs[j]);
                    }
                    fprintf(stdout, "INDIRECT,%d,%d,%d,%d,%d\n",
                            inode_number,
                            1,
                            j + 256 + 12,
                            indirect_block_pointer[i],
                            block_ptrs[j]);
                }
                j++;
            }
            free(block_ptrs);
        }
        i++;
    }
    free(indirect_block_pointer);
}

void triple_indirect(char file_type, unsigned int inode_num)
{
    //  If an I-node contains a triple indirect block:

    // the triple indirect block number would be included in the INODE summary.
    // INDIRECT entries (with level 3) would be produced for each double indirect block pointed to by that triple indirect block.
    // INDIRECT entries (with level 2) would be produced for each indirect block pointed to by one of those double indirect blocks.
    // INDIRECT entries (with level 1) would be produced for each data block pointed to by one of those indirect blocks.
    uint32_t *indirect_block_ptr = malloc(block_size);
    uint32_t tot_pointer = block_size / sizeof(uint32_t);
    uint32_t block_number = inode.i_block[14];

    unsigned long triple_indirect_offset = block_offset(inode.i_block[14]);

    Pread(img, indirect_block_ptr, block_size, triple_indirect_offset);

    unsigned int i = 0;
    while (i < tot_pointer)
    {
        if (indirect_block_ptr[i] != 0)
        {
            fprintf(stdout, "INDIRECT,%d,%d,%d,%d,%d\n",
                    inode_num,
                    3,
                    i + 65536 + 256 + 12, //logical block offset
                    block_number,
                    indirect_block_ptr[i]);

            unsigned long double_indirect_offset = block_offset(indirect_block_ptr[i]);
            uint32_t *indirect_block_pointer = malloc(block_size);

            Pread(img, indirect_block_pointer, block_size, double_indirect_offset);

            unsigned int j = 0;
            while (j < tot_pointer)
            {
                if (indirect_block_pointer[j] != 0)
                {
                    fprintf(stdout, "INDIRECT,%d,%d,%d,%d,%d\n",
                            inode_num,
                            2,
                            j + 65536 + 256 + 12,
                            indirect_block_ptr[i],
                            indirect_block_pointer[j]);

                    unsigned long indirect_offset = block_offset(indirect_block_pointer[j]);
                    uint32_t *block_ptrs = malloc(block_size);

                    Pread(img, block_ptrs, block_size, indirect_offset);

                    unsigned int k = 0;
                    while (k < tot_pointer)
                    {
                        if (block_ptrs[k] != 0)
                        {
                            if (file_type == 'd')
                            {
                                directory_entries_summary(inode_num, block_ptrs[k]);
                            }
                            fprintf(stdout, "INDIRECT,%d,%d,%d,%d,%d\n",
                                    inode_num,
                                    1,
                                    k + 65536 + 256 + 12,
                                    indirect_block_pointer[j],
                                    block_ptrs[k]);
                        }
                        k++;
                    }
                    free(block_ptrs);
                }
                j++;
            }
            free(indirect_block_pointer);
        }
        i++;
    }
    free(indirect_block_ptr);
}

void inode_summary(unsigned int inode_table_id, unsigned int index, unsigned int inode_number)
{
    // Scan the I-nodes for each group. For each allocated (non-zero mode and non-zero link count) I-node, produce a new-line terminated line, with up to 27 comma-separated fields (with no white space). The first twelve fields are i-node attributes:

    // INODE
    // inode number (decimal)
    // file type ('f' for file, 'd' for directory, 's' for symbolic link, '?' for anything else)
    // mode (low order 12-bits, octal ... suggested format "%o")
    // owner (decimal)
    // group (decimal)
    // link count (decimal)
    // time of last I-node change (mm/dd/yy hh:mm:ss, GMT)
    // modification time (mm/dd/yy hh:mm:ss, GMT)
    // time of last access (mm/dd/yy hh:mm:ss, GMT)
    // file size (decimal)
    // number of (512 byte) blocks of disk space (decimal) taken up by this files
    unsigned long offset = block_offset(inode_table_id) + (index * sizeof(inode));

    Pread(img, &inode, sizeof(inode), offset);

    if (inode.i_links_count == 0 || inode.i_mode == 0)
    {
        return;
    }

    //get file type, if symbolic link, regular file, or directory; else '?'
    char file_type = '?';
    if ((inode.i_mode & 0xF000) == S_IFLNK)
    {
        file_type = 's';
    }
    else if ((inode.i_mode & 0xF000) == S_IFREG)
    {
        file_type = 'f';
    }
    else if ((inode.i_mode & 0xF000) == S_IFDIR)
    {
        file_type = 'd';
        unsigned int i;
        //get directory entries summary
        for (i = 0; i < 12; i++)
        {
            if (inode.i_block[i] != 0)
            {
                directory_entries_summary(inode_number, inode.i_block[i]);
            }
        }
    }

    //mode
    unsigned int status = inode.i_mode & 0xFFF;

    char cur_time[20];
    char max_time[20];
    char after_time[20];

    getTime(inode.i_ctime, cur_time);
    getTime(inode.i_atime, after_time);
    getTime(inode.i_mtime, max_time);

    //INODE ATTRIBUTES
    //print INODE, inode number, file type, mode (the lower 12 bits of mode),
    //owner, group, link count, last inode change time, modification time, last access time
    //file size, number of (512 byte) blocks of disk space taken up by this file
    fprintf(stdout, "INODE,%d,%c,%o,%d,%d,%d,%s,%s,%s,%d,%d",
            inode_number,
            file_type,
            status,
            inode.i_uid,
            inode.i_gid,
            inode.i_links_count,
            cur_time,
            max_time,
            after_time,
            inode.i_size,
            inode.i_blocks);

    if (file_type == 's' && inode.i_size < 60)
    {
        fprintf(stdout, ",%d", inode.i_block[0]);
    }
    else
    {
        for (unsigned int i = 0; i < 15; i++)
        {
            fprintf(stdout, ",%d", inode.i_block[i]);
        }
    }
    fprintf(stdout, "\n");

    //indirect blocks
    if (!(file_type == 's' && inode.i_size < 60))
    {
        //if indirect block exists
        if (inode.i_block[12] != 0)
        {
            single_indirect(file_type, inode_number);
        }

        //if double indirect block exists
        if (inode.i_block[13] != 0)
        {
            double_indirect(file_type, inode_number);
        }

        //if triple indirect block exists
        if (inode.i_block[14] != 0)
        {
            triple_indirect(file_type, inode_number);
        }
    }
}

void free_inode_summary(int group, int block, int inode_table_id)
{
    // Scan the free I-node bitmap for each group. For each free I-node, produce a new-line terminated line, with two comma-separated fields (with no white space).
    // IFREE
    // number of the free I-node (decimal)

    // Take care to verify that you:

    // understand whether 1 means allocated or free.
    // have correctly understood the I-node number to which the first bit corresponds.
    // know how many I-nodes are in each group, and do not interpret more bits than there are I-nodes in the group.
    int num_bytes = superblock.s_inodes_per_group / 8;
    char *read_bytes = (char *)malloc(num_bytes);
    if (read_bytes == NULL)
    {
        fprintf(stderr, "Unable to malloc for free inode summary \n");
        exit(1);
    }
    unsigned long offset = block_offset(block);
    unsigned int read = group * superblock.s_inodes_per_group + 1;
    unsigned int start = read;
    if (pread(img, read_bytes, num_bytes, offset) < 0)
    {
        fprintf(stderr, "Unable to read free inode summary \n");
        exit(1);
    }

    int i = 0;
    int j;
    while (i < num_bytes)
    {
        j = 0;
        char x = read_bytes[i];
        for (j = 0; j < 8; j++)
            while (j < 8)
            {
                int allocated = 1 & x;
                if (allocated)
                {
                    inode_summary(inode_table_id, read - start, read);
                }
                else
                {
                    fprintf(stdout, "IFREE,%d\n", read);
                }
                j++;
                x = x >> 1;
                read++;
            }
        i++;
    }
    free(read_bytes);
}

void group_summary(int group_num, int total_groups)
{
    // Scan each of the groups in the file system. For each group, produce a new-line terminated line for each group, each comprised of nine comma-separated fields (with no white space), summarizing its contents.

    // GROUP
    // group number (decimal, starting from zero)
    // total number of blocks in this group (decimal)
    // total number of i-nodes in this group (decimal)
    // number of free blocks (decimal)
    // number of free i-nodes (decimal)
    // block number of free block bitmap for this group (decimal)
    // block number of free i-node bitmap for this group (decimal)
    // block number of first block of i-nodes in this group (decimal)

    unsigned long offset;
    if (block_size == 1024)
    {
        offset = block_size * 2 + 32 * group_num;
    }
    else
    {
        offset = block_size + 32 * group_num;
    }

    if (pread(img, &group_desc, sizeof(group_desc), offset) < 0)
    {
        fprintf(stderr, "Unable to read group summary");
        exit(1);
    }

    unsigned int num_blocks_in_group = superblock.s_blocks_per_group;
    unsigned int num_inodes_in_group = superblock.s_inodes_per_group;

    if (group_num == total_groups - 1)
    {
        num_blocks_in_group = superblock.s_blocks_count - (superblock.s_blocks_per_group * (total_groups - 1));
        num_inodes_in_group = superblock.s_inodes_count - (superblock.s_inodes_per_group * (total_groups - 1));
    }

    fprintf(stdout, "GROUP,%d,%d,%d,%d,%d,%d,%d,%d\n",
            group_num,
            num_blocks_in_group,
            num_inodes_in_group,
            group_desc.bg_free_blocks_count,
            group_desc.bg_free_inodes_count,
            group_desc.bg_block_bitmap,
            group_desc.bg_inode_bitmap,
            group_desc.bg_inode_table);

    block_bitmap = group_desc.bg_block_bitmap;
    inode_bitmap = group_desc.bg_inode_bitmap;
    inode_table = group_desc.bg_inode_table;
}

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        fprintf(stderr, "Unable to parse arguments \n");
        exit(1);
    }
    if ((img = open(argv[1], O_RDONLY)) < 0)
    {
        fprintf(stderr, "Unable to open file \n");
        exit(1);
    }
    block_size = EXT2_MIN_BLOCK_SIZE << superblock.s_log_block_size;
    superblock_summary();
    int i;
    int num_groups = superblock.s_blocks_count / superblock.s_blocks_per_group;
    if ((double)num_groups < (double)superblock.s_blocks_count / superblock.s_blocks_per_group)
    {
        for (i = 0; i <= num_groups; i++)
        {
            group_summary(i, num_groups + 1);
            free_block_summary(i, block_bitmap);
            free_inode_summary(i, inode_bitmap, inode_table);
        }
    }
    else
    {
    }
    for (i = 0; i < num_groups; i++)
    {
        group_summary(i, num_groups);
        free_block_summary(i, block_bitmap);
        free_inode_summary(i, inode_bitmap, inode_table);
    }
    return 0;
}
