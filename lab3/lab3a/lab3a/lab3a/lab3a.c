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

void Pread(int d, void* buf, size_t nbytes, off_t offset)
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

void superblock_summary()
{
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
    char *bytes = (char *)malloc(block_size);
    if (bytes == NULL)
    {
        fprintf(stderr, "Unable to malloc for free block summary \n");
        exit(1);
    }
    unsigned long offset = block_offset(block);
    if (pread(img, bytes, block_size, offset) < 0)
    {
        fprintf(stderr, "Unable to read free block summary");
        exit(1);
    }
    unsigned int curr = superblock.s_first_data_block + (group * superblock.s_blocks_per_group);
    unsigned int i = 0;
    unsigned int j;
    while (i < block_size)
    {
        j = 0;
        char x = bytes[i];
        while (j < 8)
        {
            int used = 1 & x;
            if (!used)
            {
                fprintf(stdout, "BFREE,%d\n", curr);
            }
            j++;
            x = x >> 1;
            curr++;
        }
        i++;
    }
    free(bytes);
}

void getTime(time_t raw_time, char *buf)
{
    time_t epoch = raw_time;
    struct tm ts = *gmtime(&epoch);
    strftime(buf, 80, "%m/%d/%y %H:%M:%S", &ts);
}


void directory_entries_summary(unsigned int parent_inode, unsigned int block_num)
{
    unsigned long offset = block_offset(block_num);
    unsigned int num_bytes = 0;

    while (num_bytes < block_size)
    {
        memset(dir_entry.name, 0, 256);
        if (pread(img, &dir_entry, sizeof(dir_entry), offset + num_bytes) >= 0 && dir_entry.inode != 0)
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

void single_indirect(char file_type, unsigned int inode_num)
{
    uint32_t* block_ptrs = malloc(block_size);
    uint32_t numptrs = block_size / sizeof(uint32_t);

    unsigned long single_indirect_offset = block_offset(inode.i_block[12]);

    Pread(img, block_ptrs, block_size, single_indirect_offset);
    uint32_t block_num = inode.i_block[12];

    unsigned int i;
    for (i = 0; i < numptrs; i++)
    {
        if (block_ptrs[i] != 0)
        {
            if (file_type == 'd')
            {
                directory_entries_summary(inode_num, block_ptrs[i]);
            }
            
            fprintf(stdout, "INDIRECT,%d,%d,%d,%d,%d\n",
                    inode_num,
                    1,
                    i+12,
                    block_num,
                    block_ptrs[i]);
        }
    }
    
    free(block_ptrs);
}

void double_indirect(char file_type, unsigned int inode_num)
{
    uint32_t *indirect1_block_ptrs = malloc(block_size);
    uint32_t numptrs = block_size / sizeof(uint32_t);
    uint32_t block_num = inode.i_block[13];

    unsigned long double_indirect_offset = block_offset(inode.i_block[13]);
    
    Pread(img, indirect1_block_ptrs, block_size, double_indirect_offset);

    unsigned int i=0;
    while (i<numptrs)
    {
        if (indirect1_block_ptrs[i] != 0)
        {
            fprintf(stdout, "INDIRECT,%d,%d,%d,%d,%d\n",
                    inode_num,
                    2,
                    i + 256 + 12,
                    block_num,
                    indirect1_block_ptrs[i]);

            uint32_t *block_ptrs = malloc(block_size);
            
            unsigned long single_indirect_offset = block_offset(indirect1_block_ptrs[i]);
            
            Pread(img, block_ptrs, block_size, single_indirect_offset);

            unsigned int j=0;
            while(j<numptrs)
            {
                if (block_ptrs[j] != 0)
                {
                    if (file_type == 'd')
                    {
                        directory_entries_summary(inode_num, block_ptrs[j]);
                    }
                    fprintf(stdout, "INDIRECT,%d,%d,%d,%d,%d\n",
                            inode_num,
                            1,
                            j + 256 + 12,
                            indirect1_block_ptrs[i],
                            block_ptrs[j]);
                }
                j++;
            }
            free(block_ptrs);
        }
        i++;
    }
    free(indirect1_block_ptrs);
}

void triple_indirect(char file_type, unsigned int inode_num)
{
    uint32_t *indirect2_block_ptrs = malloc(block_size);
    uint32_t numptrs = block_size / sizeof(uint32_t);
    uint32_t block_num = inode.i_block[14];

    unsigned long triple_indirect_offset = block_offset(inode.i_block[14]);
    
    Pread(img, indirect2_block_ptrs, block_size, triple_indirect_offset);

    unsigned int i=0;
    while (i<numptrs)
    {
        if (indirect2_block_ptrs[i] != 0)
        {
            fprintf(stdout, "INDIRECT,%d,%d,%d,%d,%d\n",
                    inode_num,
                    3,
                    i+ 65536 + 256 + 12, //logical block offset
                    block_num,
                    indirect2_block_ptrs[i]);

            uint32_t *indirect1_block_ptrs = malloc(block_size);
            unsigned long double_indirect_offset = block_offset(indirect2_block_ptrs[i]);
            
            Pread(img, indirect1_block_ptrs, block_size, double_indirect_offset);

            unsigned int j=0;
            while (j<numptrs)
            {
                if (indirect1_block_ptrs[j] != 0)
                {
                    fprintf(stdout, "INDIRECT,%d,%d,%d,%d,%d\n",
                            inode_num,
                            2,
                            j + 65536 + 256 + 12,
                            indirect2_block_ptrs[i],
                            indirect1_block_ptrs[j]);
                    
                    uint32_t *block_ptrs = malloc(block_size);
                    unsigned long indir_offset = block_offset(indirect1_block_ptrs[j]);
                    
                    Pread(img, block_ptrs, block_size, indir_offset);

                    unsigned int k=0;
                    while (k<numptrs)
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
                                    indirect1_block_ptrs[j],
                                    block_ptrs[k]);
                        }
                        k++;
                    }
                    free(block_ptrs);
                }
                j++;
            }
            free(indirect1_block_ptrs);
        }
        i++;
    }
    free(indirect2_block_ptrs);
}

void inode_summary(unsigned int inode_table_id, unsigned int index, unsigned int inode_num)
{
    unsigned long offset = block_offset(inode_table_id) + (index * sizeof(inode));
    
    Pread(img, &inode, sizeof(inode), offset);

    if (inode.i_mode == 0 || inode.i_links_count == 0)
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
                directory_entries_summary(inode_num, inode.i_block[i]);
            }
        }
    }

    //mode
    unsigned int mode = inode.i_mode & 0xFFF;
    
    char ctime[20];
    char mtime[20];
    char atime[20];
    
    getTime(inode.i_ctime, ctime);
    getTime(inode.i_mtime, mtime);
    getTime(inode.i_atime, atime);
    
    //INODE ATTRIBUTES
    //print INODE, inode number, file type, mode (the lower 12 bits of mode),
    //owner, group, link count, last inode change time, modification time, last access time
    //file size, number of (512 byte) blocks of disk space taken up by this file
    fprintf(stdout, "INODE,%d,%c,%o,%d,%d,%d,%s,%s,%s,%d,%d",
            inode_num,
            file_type,
            mode,
            inode.i_uid,
            inode.i_gid,
            inode.i_links_count,
            ctime,
            mtime,
            atime,
            inode.i_size,
            inode.i_blocks);

    if (file_type == 's' && inode.i_size < 60)
    {
        fprintf(stdout, ",%d", inode.i_block[0]);
    }
    else {
        for (unsigned int i = 0; i < 15; i++)
        {
            fprintf(stdout, ",%d", inode.i_block[i]);
        }
    }
    fprintf(stdout, "\n");

    //indirect blocks
    if ( !(file_type == 's' && inode.i_size < 60) )
    {
        //if indirect block exists
        if (inode.i_block[12] != 0)
        {
            single_indirect(file_type, inode_num);
        }

        //if double indirect block exists
        if (inode.i_block[13] != 0)
        {
            double_indirect(file_type, inode_num);
        }

        //if triple indirect block exists
        if (inode.i_block[14] != 0)
        {
            triple_indirect(file_type, inode_num);
        }
    }
}


void free_inode_summary(int group, int block, int inode_table_id)
{
    int num_bytes = superblock.s_inodes_per_group / 8;
    char *bytes = (char *)malloc(num_bytes);
    if (bytes == NULL)
    {
        fprintf(stderr, "Unable to malloc for free inode summary \n");
        exit(1);
    }
    unsigned long offset = block_offset(block);
    unsigned int curr = group * superblock.s_inodes_per_group + 1;
    unsigned int start = curr;
    if (pread(img, bytes, num_bytes, offset) < 0)
    {
        fprintf(stderr, "Unable to read free inode summary \n");
        exit(1);
    }

    int i = 0;
    int j;
    while (i < num_bytes)
    {
        j = 0;
        char x = bytes[i];
        for (j = 0; j < 8; j++)
            while (j < 8)
            {
                int used = 1 & x;
                if (used)
                {
                    inode_summary(inode_table_id, curr - start, curr);
                }
                else
                {
                    fprintf(stdout, "IFREE,%d\n", curr);
                }
                j++;
                x = x >> 1;
                curr++;
            }
        i++;
    }
    free(bytes);
}

void group_summary(int group, int total_groups)
{
    unsigned long offset;
    if (block_size == 1024)
    {
        offset = block_size * 2 + 32 * group;
    }
    else
    {
        offset = block_size + 32 * group;
    }

    if (pread(img, &group_desc, sizeof(group_desc), offset) < 0)
    {
        fprintf(stderr, "Unable to read group summary");
        exit(1);
    }

    unsigned int num_blocks_in_group = superblock.s_blocks_per_group;
    unsigned int num_inodes_in_group = superblock.s_inodes_per_group;

    if (group == total_groups - 1)
    {
        num_blocks_in_group = superblock.s_blocks_count - (superblock.s_blocks_per_group * (total_groups - 1));
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
        fprintf(stderr, "Unable to parse arguments \n");
        exit(1);
    }

    struct option options[] = {
        {0, 0, 0, 0}};

    if (getopt_long(argc, argv, "", options, NULL) != -1)
    {
        fprintf(stderr, "Unable to parse arguments \n");
        exit(1);
    }

    if ((img = open(argv[1], O_RDONLY)) == -1)
    {
        fprintf(stderr, "Unable to open file \n");
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
