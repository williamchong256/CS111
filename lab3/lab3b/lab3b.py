import sys, csv, math, numpy

sblock_list = None
g_list = None
inode_list = []
direct_list = []
indirect_list = []
free_block_list = []
free_inode_list = []
alloc_list = {}  
dup_list = set()  

# global variable
MAX_BLOCK_COUNT = 0
FIRST_DATA_BLOCK_NUM = 0

class Superblock:
    def __init__(self, line):
        self.total_blocks = int(line[1])
        self.total_inodes = int(line[2])
        self.block_size = int(line[3])
        self.super_inode_size = int(line[4])
        self.first_unres_inode = int(line[7])

# total number of i-nodes in this group (decimal)
# block number of first block of i-nodes in this group (decimal)
class Group:
    def __init__(self, line):
        self.num_inodes = int(line[3])
        self.first_inode_block = int(line[8])


# inode number (decimal)
# file type
# link count (decimal)
# file size (decimal)
# number of (512 byte) blocks of disk space (decimal) taken up by this file
class Inode:
    def __init__(self, line):
        self.inode_num = int(line[1])
        self.file_type = line[2]
        self.link_count = int(line[6])
        self.file_size = int(line[10])
        # TO DO: ADD FIELDS FOR LIST OF POINTERS AND LIST OF BLOCKS
        # MAY NEED ADDITIONAL CLASS FUNCTIONS
        if line[2] == 's':
            self.dirent_blocks = []
        else:
            self.dirent_blocks = [int(block_num) for block_num in line[12:24]]
        if line[2] == 's':
            self.indirect_blocks = []
        else:
            self.indirect_blocks = [int(block_num) for block_num in line[24:27]]

# parent inode number (decimal) ... the I-node number of the directory that contains this entry
# inode number of the referenced file (decimal)
# name (string, surrounded by single-quotes).
class DirectoryEntries:
    def __init__(self, line):
        self.parent_inode = int(line[1])
        self.ref_inode_num = int(line[3])
        self.name = line[6]


# Inode number of the owning file (decimal)
# (decimal) level of indirection for the block being scanned
# logical block offset (decimal) represented by the referenced block.
# block number of the referenced block (decimal)
class Indirect:
    def __init__(self, line):
        self.inode_num = int(line[1])
        self.indir_level = int(line[2])
        self.logical_block_offset = int(line[3])
        self.ref_block_num = int(line[5])


def inode_allocation_audit(first_inode_pos, inode_count):
    alloc_inode_list = []

    # check if allocated inode is in the free inode list
    for alloc_inode in inode_list:
        if alloc_inode.inode_num != 0:
            alloc_inode_list.append(alloc_inode.inode_num)
            # erroneous allocated inode  in free list
            if alloc_inode.inode_num in free_inode_list:
                print("ALLOCATED INODE %d ON FREELIST" % alloc_inode.inode_num)

    # check unallocated inodes in freelist, type is 0
    for unalloc_inode in inode_list:
        if unalloc_inode.file_type == '0' and unalloc_inode.inode_num not in free_inode_list:
            print("UNALLOCATED INODE %d NOT ON FREELIST" % (unalloc_inode.inode_num))

     # check if unallocated inode is in freelist
    for inode in range(first_inode_pos, inode_count):
        if inode not in alloc_inode_list and inode not in free_inode_list:
            print("UNALLOCATED INODE %d NOT ON FREELIST" % inode)

    return alloc_inode_list

def directory_consistency_audit(first_inode_pos, inode_count, alloc_inode_list, link_count, parent):
    for directory in direct_list:
        # if out of bounds (invalid inode)
        if directory.ref_inode_num < 1 or directory.ref_inode_num > inode_count:
            print("DIRECTORY INODE %d NAME %s INVALID INODE %d" %
                  (directory.parent_inode, directory.name, directory.ref_inode_num))
        # if inode is not in allocated inode list
        elif directory.ref_inode_num not in alloc_inode_list:
            print("DIRECTORY INODE %d NAME %s UNALLOCATED INODE %d" %
                  (directory.parent_inode, directory.name, directory.ref_inode_num))
        else:
            link_count[directory.ref_inode_num] += 1

    # check if link counts match up
    for inode in inode_list:
        if link_count[inode.inode_num] != inode.link_count:
            print("INODE %d HAS %d LINKS BUT LINKCOUNT IS %d" %
                  (inode.inode_num, link_count[inode.inode_num], inode.link_count))

    for directory in direct_list:
        if directory.name != "'.'" and directory.name != "'..'":
            parent[directory.ref_inode_num] = directory.parent_inode

    for directory in direct_list:
        if directory.name == "'.'" and directory.ref_inode_num != directory.parent_inode:
            print("DIRECTORY INODE %d NAME '.' LINK TO INODE %d SHOULD BE %d" %
                  (directory.parent_inode, directory.ref_inode_num, directory.parent_inode))
        if directory.name == "'..'" and parent[directory.parent_inode] != directory.ref_inode_num:
            print("DIRECTORY INODE %d NAME '..' LINK TO INODE %d SHOULD BE %d" %
                  (directory.parent_inode, directory.ref_inode_num, parent[directory.parent_inode]))

    # added this
    directories_filtered = list(filter(lambda x: x.parent_inode != 2 and x.name == "'..'", direct_list))
    for directory in list(filter(lambda x: x.name == "'..'", directories_filtered)):
        # print("Looking at inode# {} with name {} and number {}".format(direntInQuestion.parent_inode_num, direntInQuestion.name, direntInQuestion.inode_num), file=sys.stdout)
        par_candidate = list(filter(lambda x:
                                    x.ref_inode_num == directory.parent_inode and x.parent_inode != directory.parent_inode, direct_list))
        # print(len(possibleParents))
        for parent_directory in par_candidate:
            if directory.ref_inode_num != parent_directory.parent_inode:
                print('DIRECTORY INODE %d NAME %s LINK TO INODE %d SHOULD BE %d' %
                      (directory.parent_inode, directory.name, directory.ref_inode_num, parent_directory.parent_inode),
                      file=sys.stdout)


def alloc_inode_and_directory_consistency_audits(first_inode_pos, inode_count):
    link_count = numpy.zeros(inode_count + first_inode_pos)
    parent = numpy.zeros(inode_count + first_inode_pos)
    parent[2] = 2

    alloc_inode_list = inode_allocation_audit(first_inode_pos,inode_count)
    directory_consistency_audit(first_inode_pos, inode_count, alloc_inode_list, link_count, parent)


def check_inode_table(MAX_BLOCK_COUNT, FIRST_DATA_BLOCK_NUM, inode):
    # check the inode tables to find invalid/reserved blocks
    offset = 0
    level = 0
    for block_num in inode.dirent_blocks:
        if block_num < 0 or block_num >= MAX_BLOCK_COUNT:
            print("INVALID BLOCK " + str(block_num) + " IN INODE " + str(inode.inode_num) + " AT OFFSET " + str(offset))
        elif 0 < block_num < FIRST_DATA_BLOCK_NUM:
            print(
                "RESERVED BLOCK " + str(block_num) + " IN INODE " + str(inode.inode_num) + " AT OFFSET " + str(offset))
        elif block_num != level:
            # check for duplicate blocks
            if block_num not in alloc_list:
                alloc_list[block_num] = [[inode.inode_num, offset, level]]
            else:
                dup_list.add(block_num)
                alloc_list[block_num].append([inode.inode_num, offset, level])
        offset += 1


def check_singly_indirect_block(MAX_BLOCK_COUNT, FIRST_DATA_BLOCK_NUM, inode):
    # check the singly indirect blocks for invalid/reserved blocks
    block_num = inode.indirect_blocks[0]
    offset = 12
    level = 1
    if inode.indirect_blocks[0] < 0 or inode.indirect_blocks[0] >= MAX_BLOCK_COUNT:
        print("INVALID INDIRECT BLOCK " + str(block_num) + " IN INODE " + str(inode.inode_num) + " AT OFFSET " + str(
            offset))
    elif 0 < block_num < FIRST_DATA_BLOCK_NUM:
        print("RESERVED INDIRECT BLOCK " + str(block_num) + " IN INODE " + str(inode.inode_num) + " AT OFFSET " + str(
            offset))
    elif block_num != level - 1:
        # check for duplicate blocks
        if block_num not in alloc_list:
            alloc_list[block_num] = [[inode.inode_num, offset, level]]
        else:
            dup_list.add(block_num)
            alloc_list[block_num].append([inode.inode_num, offset, level])


def check_doubly_indirect_block(MAX_BLOCK_COUNT, FIRST_DATA_BLOCK_NUM, inode):
    # check the doubly indirect blocks for invalid/reserved blocks
    block_num = inode.indirect_blocks[1]
    offset = 268
    level = 2
    if inode.indirect_blocks[1] < 0 or inode.indirect_blocks[1] >= MAX_BLOCK_COUNT:
        print("INVALID DOUBLE INDIRECT BLOCK " + str(block_num) + " IN INODE " + str(
            inode.inode_num) + " AT OFFSET " + str(offset))
    elif 0 < block_num < FIRST_DATA_BLOCK_NUM:
        print("RESERVED DOUBLE INDIRECT BLOCK " + str(block_num) + " IN INODE " + str(
            inode.inode_num) + " AT OFFSET " + str(offset))
    elif block_num != 0:
        # check for duplicate blocks
        if block_num not in alloc_list:
            alloc_list[block_num] = [[inode.inode_num, offset, level]]
        else:
            dup_list.add(block_num)
            alloc_list[block_num].append([inode.inode_num, offset, level])


def check_triply_indirect_block(MAX_BLOCK_COUNT, FIRST_DATA_BLOCK_NUM, inode):
    # check the triply indirect blocks for invalid.reserved blocks
    block_num = inode.indirect_blocks[2]
    offset = 65804
    level = 3
    if  block_num >= MAX_BLOCK_COUNT or block_num < 0:
        print("INVALID TRIPLE INDIRECT BLOCK " + str(block_num) + " IN INODE " + str(
            inode.inode_num) + " AT OFFSET " + str(offset))
    elif 0 < block_num < FIRST_DATA_BLOCK_NUM:
        print("RESERVED TRIPLE INDIRECT BLOCK " + str(block_num) + " IN INODE " + str(
            inode.inode_num) + " AT OFFSET " + str(offset))
    elif block_num != 0:
        # check for duplicate blocks
        if block_num not in alloc_list:
            alloc_list[block_num] = [[inode.inode_num, offset, level]]
        else:
            dup_list.add(block_num)
            alloc_list[block_num].append([inode.inode_num, offset, level])


def check_inner_indirect_block_size(MAX_BLOCK_COUNT, FIRST_DATA_BLOCK_NUM):
    # check inner indirect blocks that may be indierct, double indirect, or triple indirect
    for indirect in indirect_list:
        block_num = indirect.ref_block_num
        slevel = ""
        offset = 0
        level = 0
        if indirect.indir_level == 1:
            slevel = "INDIRECT"
            offset = 12
            level = 1
        elif indirect.indir_level == 2:
            slevel = "DOUBLE INDIRECT"
            offset = 268
            level = 2
        elif indirect.indir_level == 3:
            slevel = "TRIPLE INDIRECT"
            offset = 65804
            level = 3

        # check the blocks to for invalid/reserved blocks
        if block_num < 0 or block_num >= MAX_BLOCK_COUNT:
            print("INVALID " + str(slevel) + " BLOCK " + str(block_num) + " IN INODE " + str(
                indirect.inode_num) + " AT OFFSET " + str(offset))
        elif 0 < block_num < FIRST_DATA_BLOCK_NUM:
            print("RESERVED " + str(slevel) + " BLOCK " + str(block_num) + " IN INODE " + str(
                indirect.inode_num) + " AT OFFSET " + str(offset))
        elif block_num != 0:  
            # check for duplicate blocks
            if block_num in alloc_list:
                dup_list.add(block_num)
                alloc_list[block_num].append([indirect.inode_num, indirect.logical_block_offset, indirect.indir_level])
            else:
                alloc_list[block_num] = [[indirect.inode_num, indirect.logical_block_offset, indirect.indir_level]]


def block_error(MAX_BLOCK_COUNT, FIRST_DATA_BLOCK_NUM):
    # check for unreferenced/allocated blocks
    for num in range(FIRST_DATA_BLOCK_NUM, MAX_BLOCK_COUNT):
        if num not in free_block_list and num not in alloc_list:
            print("UNREFERENCED BLOCK " + str(num))
        if num in free_block_list and num in alloc_list:
            print("ALLOCATED BLOCK " + str(num) + " ON FREELIST")

    # check for duplicate blocks and classify them
    for num in dup_list:
        for ref in alloc_list[num]:
            slevel = ""
            if ref[-1] == 1:
                fref = ref[0]
                slevel = "INDIRECT"
            elif ref[-1] == 2:
                fref = ref[0]
                slevel = "DOUBLE INDIRECT"
            elif ref[-1] == 3:
                fref = ref[0]
                slevel = "TRIPLE INDIRECT"
            if slevel == "":
                fref = ref[0]
                print("DUPLICATE BLOCK " + str(num) + " IN INODE " + str(fref) + " AT OFFSET " + str(ref[1]))
            else:
                print("DUPLICATE " + str(slevel) + " BLOCK " + str(num) + " IN INODE " + str(
                    fref) + " AT OFFSET " + str(ref[1]))


def calc_max_block_count(element):
    return element.total_blocks


def calc_first_data_block_num(gp, sblock):
    return int(gp.first_inode_block + math.ceil(gp.num_inodes * sblock.super_inode_size / sblock.block_size))


def main():
    global sblock_list
    global g_list
    global free_block_list
    global free_inode_list
    global inode_list
    global direct_list
    global indirect_list

    # check for valid arg
    try:
        input_file = open(sys.argv[1], 'r')
    except:
        sys.stderr.write("Unable to open csv file \n")
        exit(1)

    rfile = csv.reader(input_file)

    # assign blocks
    for element in rfile:
        identity = element[0]
        if identity == "SUPERBLOCK":
            sblock_list = Superblock(element)
        elif identity == "GROUP":
            g_list = Group(element)
        elif identity == "BFREE":
            free_block_list.append(int(element[1]))
        elif identity == "IFREE":
            free_inode_list.append(int(element[1]))
        elif identity == "INODE":
            inode_list.append(Inode(element))
        elif identity == "DIRENT":
            direct_list.append(DirectoryEntries(element))
        elif identity == "INDIRECT":
            indirect_list.append(Indirect(element))
        else:
            sys.stderr.write("Unable to parse csv file \n")
            exit(1)

    # calcualte max block to verify blocks
    MAX_BLOCK_COUNT = calc_max_block_count(sblock_list)
    # calcualte first data to call functions
    FIRST_DATA_BLOCK_NUM = calc_first_data_block_num(g_list, sblock_list)

    for inode in inode_list:
        if inode.file_size <= MAX_BLOCK_COUNT and inode.file_type == 's':
            continue
        check_inode_table(MAX_BLOCK_COUNT, FIRST_DATA_BLOCK_NUM, inode)
        check_singly_indirect_block(MAX_BLOCK_COUNT, FIRST_DATA_BLOCK_NUM, inode)
        check_doubly_indirect_block(MAX_BLOCK_COUNT, FIRST_DATA_BLOCK_NUM, inode)
        check_triply_indirect_block(MAX_BLOCK_COUNT, FIRST_DATA_BLOCK_NUM, inode)
    check_inner_indirect_block_size(MAX_BLOCK_COUNT, FIRST_DATA_BLOCK_NUM)
    block_error(MAX_BLOCK_COUNT, FIRST_DATA_BLOCK_NUM)

    alloc_inode_and_directory_consistency_audits(sblock_list.first_unres_inode, sblock_list.total_inodes)

if __name__ == '__main__':
    main()
