# NAME: Jongyun Park, William Chong
# EMAIL: jp101200@g.ucla.edu, williamchong256@gmail.com
# ID: 505096047, 205114665

import sys, csv, math, numpy

superblock_element = None
group_element = None
inode_element = []
dirent_element = []
indirect_element = []
bfree_element = []
ifree_element = []
balloc_element = {}  # a dictionary mapping allocated block to list of inodes that point to this point\
bdup_element = set()  # a list of all detected duplicate blocks

# constants
MAX_BLOCK_COUNT = 0
FIRST_DATA_BLOCK_NUM = 0


def print_error(error_message):
    sys.stderr.write(error_message)
    sys.exit(1)


class Superblock:
    def __init__(self, line):
        self.total_blocks = int(line[1])
        self.total_inodes = int(line[2])
        self.block_size = int(line[3])
        self.super_inode_size = int(line[4])
        self.blocks_per_group = int(line[5])
        self.inodes_per_group = int(line[6])
        self.first_unres_inode = int(line[7])


# group number (decimal, starting from zero)
# total number of blocks in this group (decimal)
# total number of i-nodes in this group (decimal)
# number of free blocks (decimal)
# number of free i-nodes (decimal)
# block number of free block bitmap for this group (decimal)
# block number of free i-node bitmap for this group (decimal)
# block number of first block of i-nodes in this group (decimal)
class Group:
    def __init__(self, line):
        self.group_num = int(line[1])
        self.num_blocks = int(line[2])
        self.num_inodes = int(line[3])
        self.num_free_blocks = int(line[4])
        self.num_free_inodes = int(line[5])
        self.block_bitmap = int(line[6])
        self.inode_bitmap = int(line[7])
        self.first_inode_block = int(line[8])


# inode number (decimal)
# file type
# mode (low order 12-bits, octal ... suggested format "%o")
# owner (decimal)
# group (decimal)
# link count (decimal)
# time of last I-node change (mm/dd/yy hh:mm:ss, GMT)
# modification time (mm/dd/yy hh:mm:ss, GMT)
# time of last access (mm/dd/yy hh:mm:ss, GMT)
# file size (decimal)
# number of (512 byte) blocks of disk space (decimal) taken up by this file
class Inode:
    def __init__(self, line):
        self.inode_num = int(line[1])
        self.file_type = line[2]
        self.mode = line[3]
        self.owner = int(line[4])
        self.group = int(line[5])
        self.link_count = int(line[6])
        self.change_time = line[7]
        self.mod_time = line[8]
        self.access_time = line[9]
        self.file_size = int(line[10])
        self.num_blocks = int(line[11])
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
# logical byte offset (decimal) of this entry within the directory
# inode number of the referenced file (decimal)
# entry length (decimal)
# name length (decimal)
# name (string, surrounded by single-quotes).
class DirectoryEntries:
    def __init__(self, line):
        self.parent_inode = int(line[1])
        self.logical_byte_offset = int(line[2])
        self.ref_inode_num = int(line[3])
        self.entry_length = int(line[4])
        self.name_length = int(line[5])
        self.name = line[6]


# Inode number of the owning file (decimal)
# (decimal) level of indirection for the block being scanned
# logical block offset (decimal) represented by the referenced block.
# block number of the (1, 2, 3) indirect block being scanned (decimal).
# block number of the referenced block (decimal)
class Indirect:
    def __init__(self, line):
        self.inode_num = int(line[1])
        self.indir_level = int(line[2])
        self.logical_block_offset = int(line[3])
        self.indir_block_num = int(line[4])
        self.ref_block_num = int(line[5])


def inode_and_directory_allocation_audits(first_inode_pos, inode_count):
    allocated_inode = []
    links = numpy.zeros(inode_count + first_inode_pos)
    parentof = numpy.zeros(inode_count + first_inode_pos)
    parentof[2] = 2

    # scan the inode and print the inode numnber when it is a valid number and also in the FREELIST
    for allocated_inode_num in inode_element:
        if allocated_inode_num.inode_num != 0:
            allocated_inode.append(allocated_inode_num.inode_num)
            if allocated_inode_num.inode_num in ifree_element:
                print("ALLOCATED INODE %d ON FREELIST" % (allocated_inode_num.inode_num))

    # scan all the valid inode number to detect the UNALLOCATED inode and not in the free list
    for inode_num in range(first_inode_pos, inode_count):
        if inode_num not in allocated_inode and inode_num not in ifree_element:
            print("UNALLOCATED INODE %d NOT ON FREELIST" % (inode_num))

    # if inode's file type is zero then the it should in the freelist
    for allocated_inode_num in inode_element:
        if allocated_inode_num.inode_num not in allocated_inode and allocated_inode_num.inode_num not in ifree_element:
            emptylist = "empty"
        elif allocated_inode_num.file_type == '0' and allocated_inode_num.inode_num not in ifree_element:
            print("UNALLOCATED INODE %d NOT ON FREELIST" % (allocated_inode_num.inode_num))



    # if the inode number of the directory is out of boundary or not in the allocated inode set, output error_message
    # else count the corresponding inode number in the links list
    for dir in dirent_element:
        if dir.ref_inode_num > inode_count or dir.ref_inode_num < 1:
            print("DIRECTORY INODE %d NAME %s INVALID INODE %d" % (dir.parent_inode, dir.name, dir.ref_inode_num))
        elif dir.ref_inode_num not in allocated_inode:
            print("DIRECTORY INODE %d NAME %s UNALLOCATED INODE %d" % (dir.parent_inode, dir.name, dir.ref_inode_num))
        else:
            links[dir.ref_inode_num] += 1

    # scan the all the inode for incorrect link_count
    for inode in inode_element:
        if links[inode.inode_num] != inode.link_count:
            print("INODE %d HAS %d LINKS BUT LINKCOUNT IS %d" % (
            inode.inode_num, links[inode.inode_num], inode.link_count))

    # scan the directory element and record their parent's inode number
    for dir in dirent_element:
        if dir.name != "'.'" and dir.name != "'..'":
            parentof[dir.ref_inode_num] = dir.parent_inode

    # detect inconsistency for '.' and '..'
    for dir_element in dirent_element:
        if dir_element.name == "'.'" and dir_element.ref_inode_num != dir_element.parent_inode:
            print("DIRECTORY INODE %d NAME '.' LINK TO INODE %d SHOULD BE %d" %
                  (dir_element.parent_inode, dir_element.ref_inode_num, dir_element.parent_inode))
        if dir_element.name == "'..'" and parentof[dir_element.parent_inode] != dir_element.ref_inode_num:
            print("DIRECTORY INODE %d NAME '..' LINK TO INODE %d SHOULD BE %d" %
                  (dir_element.parent_inode, dir_element.ref_inode_num, parentof[dir_element.parent_inode]))

    filteredDirEnt = list(filter(lambda x: x.parent_inode != 2 and x.name == "'..'", dirent_element))
    for direntInQuestion in list(filter(lambda x: x.name == "'..'", filteredDirEnt)):
        # print("Looking at inode# {} with name {} and number {}".format(direntInQuestion.parent_inode_num, direntInQuestion.name, direntInQuestion.inode_num), file=sys.stdout)
        possibleParents = list(filter(lambda
                                          x: x.ref_inode_num == direntInQuestion.parent_inode and x.parent_inode != direntInQuestion.parent_inode,
                                      dirent_element))
        # print(len(possibleParents))
        for parentDirEnt in possibleParents:
            if direntInQuestion.ref_inode_num != parentDirEnt.parent_inode:
                # print('check against pnum {} inode target {} name {}'.format(parentDirEnt.parent_inode_num, parentDirEnt.inode_num, parentDirEnt.name), file= sys.stdout)
                print(
                    'DIRECTORY INODE {} NAME {} LINK TO INODE {} SHOULD BE {}'.format(direntInQuestion.parent_inode,
                                                                                      direntInQuestion.name,
                                                                                      direntInQuestion.ref_inode_num,
                                                                                      parentDirEnt.parent_inode),
                    file=sys.stdout)

def check_inode_table(MAX_BLOCK_COUNT, FIRST_DATA_BLOCK_NUM, inode):
    offset = 0
    level = 0
    for block_num in inode.dirent_blocks:
        if block_num < 0 or block_num >= MAX_BLOCK_COUNT:
            print("INVALID BLOCK " + str(block_num) + " IN INODE " + str(inode.inode_num) + " AT OFFSET " + str(offset))
        elif 0 < block_num < FIRST_DATA_BLOCK_NUM:
            print(
                "RESERVED BLOCK " + str(block_num) + " IN INODE " + str(inode.inode_num) + " AT OFFSET " + str(offset))
        elif block_num != level:
            if block_num not in balloc_element:
                balloc_element[block_num] = [[inode.inode_num, offset, level]]
            else:
                bdup_element.add(block_num)
                balloc_element[block_num].append([inode.inode_num, offset, level])
        offset += 1


def check_singly_indirect_block(MAX_BLOCK_COUNT, FIRST_DATA_BLOCK_NUM, inode):
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
        if block_num not in balloc_element:
            balloc_element[block_num] = [[inode.inode_num, offset, level]]
        else:
            bdup_element.add(block_num)
            balloc_element[block_num].append([inode.inode_num, offset, level])


def check_doubly_indirect_block(MAX_BLOCK_COUNT, FIRST_DATA_BLOCK_NUM, inode):
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
        if block_num not in balloc_element:
            balloc_element[block_num] = [[inode.inode_num, offset, level]]
        else:
            bdup_element.add(block_num)
            balloc_element[block_num].append([inode.inode_num, offset, level])


def check_triply_indirect_block(MAX_BLOCK_COUNT, FIRST_DATA_BLOCK_NUM, inode):
    offset = 65804
    level = 3
    block_num = inode.indirect_blocks[2]
    if inode.indirect_blocks[2] < 0 or inode.indirect_blocks[2] >= MAX_BLOCK_COUNT:
        print("INVALID TRIPLE INDIRECT BLOCK " + str(block_num) + " IN INODE " + str(
            inode.inode_num) + " AT OFFSET " + str(offset))
    elif 0 < block_num < FIRST_DATA_BLOCK_NUM:
        print("RESERVED TRIPLE INDIRECT BLOCK " + str(block_num) + " IN INODE " + str(
            inode.inode_num) + " AT OFFSET " + str(offset))
    elif block_num != 0:
        if block_num not in balloc_element:
            balloc_element[block_num] = [[inode.inode_num, offset, level]]
        else:
            bdup_element.add(block_num)
            balloc_element[block_num].append([inode.inode_num, offset, level])


def check_inner_indirect_block_size(MAX_BLOCK_COUNT, FIRST_DATA_BLOCK_NUM):
    # check the inner indirect block_size
    for indirect in indirect_element:
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

        if block_num < 0 or block_num >= MAX_BLOCK_COUNT:
            print("INVALID " + str(slevel) + " BLOCK " + str(block_num) + " IN INODE " + str(
                indirect.inode_num) + " AT OFFSET " + str(offset))
        elif 0 < block_num < FIRST_DATA_BLOCK_NUM:
            print("RESERVED " + str(slevel) + " BLOCK " + str(block_num) + " IN INODE " + str(
                indirect.inode_num) + " AT OFFSET " + str(offset))
        elif block_num != 0:  # find used blocks
            if block_num not in balloc_element:
                balloc_element[block_num] = [[indirect.inode_num, indirect.logical_block_offset, indirect.indir_level]]
            else:
                bdup_element.add(block_num)
                balloc_element[block_num].append(
                    [indirect.inode_num, indirect.logical_block_offset, indirect.indir_level])


def check_blocks(MAX_BLOCK_COUNT, FIRST_DATA_BLOCK_NUM):
    for num in range(FIRST_DATA_BLOCK_NUM, MAX_BLOCK_COUNT):
        if num not in bfree_element and num not in balloc_element:
            print("UNREFERENCED BLOCK " + str(num))
        if num in bfree_element and num in balloc_element:
            print("ALLOCATED BLOCK " + str(num) + " ON FREELIST")

    # print information of all duplicated blocks
    for num in bdup_element:
        for ref in balloc_element[num]:
            slevel = ""
            if ref[-1] == 1:
                slevel = "INDIRECT"
            elif ref[-1] == 2:
                slevel = "DOUBLE INDIRECT"
            elif ref[-1] == 3:
                slevel = "TRIPLE INDIRECT"
            if slevel == "":
                print("DUPLICATE BLOCK " + str(num) + " IN INODE " + str(ref[0]) + " AT OFFSET " + str(ref[1]))
            else:
                print("DUPLICATE " + str(slevel) + " BLOCK " + str(num) + " IN INODE " + str(
                    ref[0]) + " AT OFFSET " + str(ref[1]))


def block_consistency_audits(MAX_BLOCK_COUNT, FIRST_DATA_BLOCK_NUM):
    for inode in inode_element:
        if inode.file_type == 's' and inode.file_size <= MAX_BLOCK_COUNT:
            continue
        check_inode_table(MAX_BLOCK_COUNT, FIRST_DATA_BLOCK_NUM, inode)
        check_singly_indirect_block(MAX_BLOCK_COUNT, FIRST_DATA_BLOCK_NUM, inode)
        check_doubly_indirect_block(MAX_BLOCK_COUNT, FIRST_DATA_BLOCK_NUM, inode)
        check_triply_indirect_block(MAX_BLOCK_COUNT, FIRST_DATA_BLOCK_NUM, inode)
    check_inner_indirect_block_size(MAX_BLOCK_COUNT, FIRST_DATA_BLOCK_NUM)
    check_blocks(MAX_BLOCK_COUNT, FIRST_DATA_BLOCK_NUM)


def calc_max_block_count(element):
    return element.total_blocks


def calc_first_data_block_num(gp, sblock):
    return int(gp.first_inode_block + math.ceil(gp.num_inodes * sblock.super_inode_size / sblock.block_size))


def main():
    if len(sys.argv) != 2:
        print_error("Error: the input argument is not two \n")

    # open the input csv file
    try:
        input_csvfile = open(sys.argv[1], 'r')
    except IOError:
        print_error("Error: cannot open the input csv file \n")

    # read the csv infromation and stored them to corresponding class element
    read_file = csv.reader(input_csvfile)

    for element in read_file:
        if element[0] == "SUPERBLOCK":
            superblock_element = Superblock(element)
        elif element[0] == "GROUP":
            group_element = Group(element)
        elif element[0] == "BFREE":
            bfree_element.append(int(element[1]))
        elif element[0] == "IFREE":
            ifree_element.append(int(element[1]))
        elif element[0] == "INODE":
            inode_element.append(Inode(element))
        elif element[0] == "DIRENT":
            dirent_element.append(DirectoryEntries(element))
        elif element[0] == "INDIRECT":
            indirect_element.append(Indirect(element))
        else:
            print_error("Error: the input csv file contains invild element: \n")

    # the maximum number of block
    MAX_BLOCK_COUNT = calc_max_block_count(superblock_element)
    # first block position index
    FIRST_DATA_BLOCK_NUM = calc_first_data_block_num(group_element, superblock_element)

    # scan the blocks and print error_message
    block_consistency_audits(MAX_BLOCK_COUNT, FIRST_DATA_BLOCK_NUM)
    # scan the inode and directory, and print error_message
    inode_and_directory_allocation_audits(superblock_element.first_unres_inode, superblock_element.total_inodes)


if __name__ == '__main__':
    main()
