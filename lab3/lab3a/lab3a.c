//NAME: Jongyun Park
//EMAIL: jp101200@g.ucla.edu
//ID: 505096047

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

#include "ext2_fs.h"

#define SUPER_OFFSET 1024

int img;

struct ext2_super_block superblock;
unsigned int block_size;

unsigned long block_offset(unsigned int block)
{
    return SUPER_OFFSET + (block - 1) * block_size;
}

void superblock_summary()
{
    pread(img, &superblock, sizeof(superblock), SUPER_OFFSET);

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
    char *bytes = (char *)malloc(block_size);
    if (bytes == NULL)
    {
        fprintf(stderr, "Unable to malloc free block summary reads \n");
        exit(1);
    }
    unsigned long offset = block_offset(block);
    unsigned int curr = superblock.s_first_data_block + group * superblock.s_blocks_per_group;
    pread(img, bytes, block_size, offset);
    unsigned int i = 0;
    unsigned int j;
    while (i < block_size)
    {
        j = 0;
        char x = bytes[i];
        while (j < 8)
        {
            // read the byte
            int used = 1 & x;
            if (!used)
            {
                fprintf(stdout, "BFREE,%d\n", curr);
            }
            x = x >> 1;
            curr++;
            j++;
        }
        i++;
    }
    free(bytes);
}

void get_time(time_t raw_time, char *buf)
{
    time_t epoch = raw_time;
    struct tm ts = *gmtime(&epoch);
    strftime(buf, 80, "%m/%d/%y %H:%M:%S", &ts);
}

void directory_entries_summary(unsigned int parent_inode, unsigned int block_num)
{
    struct ext2_dir_entry dir_entry;
    unsigned long offset = block_offset(block_num);
    unsigned int num_bytes = 0;

    while (num_bytes < block_size)
    {
        memset(dir_entry.name, 0, 256);
        pread(img, &dir_entry, sizeof(dir_entry), offset + num_bytes);
        if (dir_entry.inode != 0)
        {
            memset(&dir_entry.name[dir_entry.name_len], 0, 256 - dir_entry.name_len);
            fprintf(stdout, "DIRENT,%d,%d,%d,%d,%d,'%s'\n",
                    parent_inode,
                    num_bytes,
                    dir_entry.inode,
                    dir_entry.rec_len,
                    dir_entry.name_len,
                    dir_entry.name);
        }
        num_bytes += dir_entry.rec_len;
    }
}

void inode_summary(unsigned int inode_table_id, unsigned int index, unsigned int inode_num)
{
    struct ext2_inode inode;

    unsigned long offset = block_offset(inode_table_id) + index * sizeof(inode);
    pread(img, &inode, sizeof(inode), offset);

    if (inode.i_mode == 0 || inode.i_links_count == 0)
    {
        return;
    }

    char filetype = '?';
    uint16_t file_val = (inode.i_mode >> 12) << 12;
    if (file_val == 0xa000)
    {
        filetype = 's';
    }
    else if (file_val == 0x8000)
    {
        filetype = 'f';
    }
    else if (file_val == 0x4000)
    {
        filetype = 'd';
    }

    unsigned int num_blocks = 2 * (inode.i_blocks / (2 << superblock.s_log_block_size));

    fprintf(stdout, "INODE,%d,%c,%o,%d,%d,%d,",
            inode_num,
            filetype,
            inode.i_mode & 0xFFF,
            inode.i_uid,
            inode.i_gid,
            inode.i_links_count);

    char ctime[20], mtime[20], atime[20];
    get_time(inode.i_ctime, ctime);
    get_time(inode.i_mtime, mtime);
    get_time(inode.i_atime, atime);
    fprintf(stdout, "%s,%s,%s,", ctime, mtime, atime);

    fprintf(stdout, "%d,%d",
            inode.i_size,
            num_blocks);

    unsigned int i;
    for (i = 0; i < 15; i++)
    {
        fprintf(stdout, ",%d", inode.i_block[i]);
    }
    fprintf(stdout, "\n");

    for (i = 0; i < 12; i++)
    {
        if (inode.i_block[i] != 0 && filetype == 'd')
        {
            directory_entries_summary(inode_num, inode.i_block[i]);
        }
    }

    if (inode.i_block[12] != 0)
    {
        uint32_t *block_ptrs = malloc(block_size);
        uint32_t num_ptrs = block_size / sizeof(uint32_t);

        unsigned long indir_offset = block_offset(inode.i_block[12]);
        pread(img, block_ptrs, block_size, indir_offset);

        unsigned int j;
        for (j = 0; j < num_ptrs; j++)
        {
            if (block_ptrs[j] != 0)
            {
                if (filetype == 'd')
                {
                    directory_entries_summary(inode_num, block_ptrs[j]);
                }
                fprintf(stdout, "INDIRECT,%d,%d,%d,%d,%d\n",
                        inode_num,
                        1,
                        12 + j,
                        inode.i_block[12],
                        block_ptrs[j]);
            }
        }
        free(block_ptrs);
    }

    if (inode.i_block[13] != 0)
    {
        uint32_t *indir_block_ptrs = malloc(block_size);
        uint32_t num_ptrs = block_size / sizeof(uint32_t);

        unsigned long indir2_offset = block_offset(inode.i_block[13]);
        pread(img, indir_block_ptrs, block_size, indir2_offset);

        unsigned int j;
        for (j = 0; j < num_ptrs; j++)
        {
            if (indir_block_ptrs[j] != 0)
            {
                fprintf(stdout, "INDIRECT,%d,%d,%d,%d,%d\n",
                        inode_num,
                        2,
                        256 + 12 + j,
                        inode.i_block[13],
                        indir_block_ptrs[j]);

                uint32_t *block_ptrs = malloc(block_size);
                unsigned long indir_offset = block_offset(indir_block_ptrs[j]);
                pread(img, block_ptrs, block_size, indir_offset);

                unsigned int k;
                for (k = 0; k < num_ptrs; k++)
                {
                    if (block_ptrs[k] != 0)
                    {
                        if (filetype == 'd')
                        {
                            directory_entries_summary(inode_num, block_ptrs[k]);
                        }
                        fprintf(stdout, "INDIRECT,%d,%d,%d,%d,%d\n",
                                inode_num,
                                1,
                                256 + 12 + k,
                                indir_block_ptrs[j],
                                block_ptrs[k]);
                    }
                }
                free(block_ptrs);
            }
        }
        free(indir_block_ptrs);
    }

    if (inode.i_block[14] != 0)
    {
        uint32_t *indir2_block_ptrs = malloc(block_size);
        uint32_t num_ptrs = block_size / sizeof(uint32_t);

        unsigned long indir3_offset = block_offset(inode.i_block[14]);
        pread(img, indir2_block_ptrs, block_size, indir3_offset);

        unsigned int j;
        for (j = 0; j < num_ptrs; j++)
        {
            if (indir2_block_ptrs[j] != 0)
            {
                fprintf(stdout, "INDIRECT,%d,%d,%d,%d,%d\n",
                        inode_num,
                        3,
                        65536 + 256 + 12 + j,
                        inode.i_block[14],
                        indir2_block_ptrs[j]);

                uint32_t *indir_block_ptrs = malloc(block_size);
                unsigned long indir2_offset = block_offset(indir2_block_ptrs[j]);
                pread(img, indir_block_ptrs, block_size, indir2_offset);

                unsigned int k;
                for (k = 0; k < num_ptrs; k++)
                {
                    if (indir_block_ptrs[k] != 0)
                    {
                        fprintf(stdout, "INDIRECT,%d,%d,%d,%d,%d\n",
                                inode_num,
                                2,
                                65536 + 256 + 12 + k,
                                indir2_block_ptrs[j],
                                indir_block_ptrs[k]);
                        uint32_t *block_ptrs = malloc(block_size);
                        unsigned long indir_offset = block_offset(indir_block_ptrs[k]);
                        pread(img, block_ptrs, block_size, indir_offset);

                        unsigned int l;
                        for (l = 0; l < num_ptrs; l++)
                        {
                            if (block_ptrs[l] != 0)
                            {
                                if (filetype == 'd')
                                {
                                    directory_entries_summary(inode_num, block_ptrs[l]);
                                }
                                fprintf(stdout, "INDIRECT,%d,%d,%d,%d,%d\n",
                                        inode_num,
                                        1,
                                        65536 + 256 + 12 + l,
                                        indir_block_ptrs[k],
                                        block_ptrs[l]);
                            }
                        }
                        free(block_ptrs);
                    }
                }
                free(indir_block_ptrs);
            }
        }
        free(indir2_block_ptrs);
    }
}

void free_inode_summary(int group, int block, int inode_table_id)
{
    int num_bytes = superblock.s_inodes_per_group / 8;
    char *bytes = (char *)malloc(num_bytes);
    if (bytes == NULL)
    {
        fprintf(stderr, "Unable to malloc for inode summary read \n");
        exit(1);
    }

    unsigned long offset = block_offset(block);
    unsigned int curr = group * superblock.s_inodes_per_group + 1;
    unsigned int start = curr;
    pread(img, bytes, num_bytes, offset);

    int i = 0;
    int j;
    while (i < num_bytes)
    {
        char x = bytes[i];
        while (j < 8)
        {
            // read the byte
            int used = 1 & x;
            if (used)
            {
                inode_summary(inode_table_id, curr - start, curr);
            }
            else
            {
                fprintf(stdout, "IFREE,%d\n", curr);
            }
            x = x >> 1;
            curr++;
            j++;
        }
        i++;
    }
    free(bytes);
}

void group_summary(int group, int total_groups)
{
    struct ext2_group_desc group_desc;
    uint32_t descblock = 0;
    if (block_size == 1024)
    {
        descblock = 2;
    }
    else
    {
        descblock = 1;
    }

    unsigned long offset = block_size * descblock + 32 * group;
    pread(img, &group_desc, sizeof(group_desc), offset);

    unsigned int num_blocks_in_group = superblock.s_blocks_per_group;
    if (group == total_groups - 1)
    {
        num_blocks_in_group = superblock.s_blocks_count - (superblock.s_blocks_per_group * (total_groups - 1));
    }

    unsigned int num_inodes_in_group = superblock.s_inodes_per_group;
    if (group == total_groups - 1)
    {
        num_inodes_in_group = superblock.s_inodes_count - (superblock.s_inodes_per_group * (total_groups - 1));
    }

    fprintf(stdout, "GROUP,%d,%d,%d,%d,%d,%d,%d,%d\n",
            group,
            num_blocks_in_group,
            num_inodes_in_group,
            group_desc.bg_free_blocks_count,
            group_desc.bg_free_inodes_count,
            group_desc.bg_block_bitmap,
            group_desc.bg_inode_bitmap,
            group_desc.bg_inode_table);

    unsigned int block_bitmap = group_desc.bg_block_bitmap;
    free_block_summary(group, block_bitmap);

    unsigned int inode_bitmap = group_desc.bg_inode_bitmap;
    unsigned int inode_table = group_desc.bg_inode_table;
    free_inode_summary(group, inode_bitmap, inode_table);
}

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        fprintf(stderr, "Bad arguments\n");
        exit(1);
    }

    struct option options[] = {
        {0, 0, 0, 0}};

    if (getopt_long(argc, argv, "", options, NULL) != -1)
    {
        fprintf(stderr, "Bad arguments\n");
        exit(1);
    }

    if ((img = open(argv[1], O_RDONLY)) == -1)
    {
        fprintf(stderr, "Could not open file\n");
        exit(1);
    }
    block_size = EXT2_MIN_BLOCK_SIZE << superblock.s_log_block_size;
    superblock_summary();
    int num_groups = superblock.s_blocks_count / superblock.s_blocks_per_group;
    if ((double)num_groups < (double)superblock.s_blocks_count / superblock.s_blocks_per_group)
    {
        num_groups++;
    }
    int i;
    for (i = 0; i < num_groups; i++)
    {
        group_summary(i, num_groups);
    }

    return 0;
}
