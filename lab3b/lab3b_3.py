import sys,csv,math,numpy

superblock_element=None
group_element=None
inode_element=[]
dirent_element=[]
indirect_element=[]
bfree_element=[]
ifree_element=[]
balloc_element={} # a dictionary mapping allocated block to list of inodes that point to this point\
bdup_element=set()  # a list of all detected duplicate blocks


# constants
MAX_BLOCK_COUNT = 0
FIRST_DATA_BLOCK_NUM = 0

def print_error(error_message):
    sys.stderr.write(error_message)
    sys.exit(1)
#class for superblock's information read from the csv file
class superblock:
    def __init__(self, line):
        self.total_blocks = int(line[1])
        self.total_inodes = int(line[2])
        self.block_size = int(line[3])
        self.super_inode_size = int(line[4])
        self.blocks_per_group = int(line[5])
        self.inodes_per_group = int(line[6])
        self.first_unres_inode = int(line[7])
    # def __init__(self, element):
    #     self.s_blocks_count = int(element[1])
    #     self.s_inodes_count = int(element[2])
    #     self.block_size = int(element[3])
    #     self.s_inode_size = int(element[4])
    #     self.s_blocks_per_group = int(element[5])
    #     self.s_inodes_per_group = int(element[6])
    #     self.s_first_ino = int(element[7])

#class for superblock's inode read from the csv file
class inode:
    # def __init__(self,element):
    #     self.inode_num = int(element[1])
    #     self.file_type = element[2]
    #     self.mode = int(element[3])
    #     self.i_uid = int(element[4])
    #     self.i_gid = int(element[5])
    #     self.i_links_count = int(element[6])
    #     self.ctime_str = element[7]
    #     self.mtime_str = element[8]
    #     self.atime_str = element[9]
    #     self.i_size = int(element[10])
    #     self.i_blocks = int(element[11])
    #     #NOTE: THESE SHOULD BE BETTER CHECKED
    #     self.dirent_blocks = [int(block_num) for block_num in element[12:24]]
    #     self.indirect_blocks = [int(block_num) for block_num in element[24:27]]
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
        #TO DO: ADD FIELDS FOR LIST OF POINTERS AND LIST OF BLOCKS
        #MAY NEED ADDITIONAL CLASS FUNCTIONS
        self.dirent_blocks = [int(block_num) for block_num in line[12:24]]
        self.indirect_blocks = [int(block_num) for block_num in line[24:27]]

#class for group's information read from the csv file
class group:
    # def __init__(self,element):
    #     self.s_inodes_per_group = int(element[3])
    #     self.bg_inode_table = int(element[-1])
    def __init__(self, line):
        self.group_num = int(line[1])
        self.num_blocks = int(line[2])
        self.num_inodes = int(line[3])
        self.num_free_blocks = int(line[4])
        self.num_free_inodes = int(line[5])
        self.block_bitmap = int(line[6])
        self.inode_bitmap = int(line[7])
        self.first_inode_block = int(line[8])

#class for dirent's information read from the csv file
class dirent:
    # def __init__(self,element):
    #     self.inode_num          =int(element[1])
    #     self.logical_offset     =int(element[2])
    #     self.inode              =int(element[3])
    #     self.rec_len            =int(element[4])
    #     self.name_len           =int(element[5])
    #     self.name               =element[6]
    def __init__(self, line):
        self.parent_inode = int(line[1])
        self.logical_byte_offset = int(line[2])
        self.ref_inode_num = int(line[3])
        self.entry_length = int(line[4])
        self.name_length = int(line[5])
        self.name = line[6]

#class for indirect's information read from the csv file
class indirect:
    # def __init__(self,element):
    #     self.inode_num          =int(element[1])
    #     self.level              =int(element[2])
    #     self.offset             =int(element[3])
    #     self.block_number       =int(element[4])
    #     self.element            =int(element[5])
    def __init__(self, line):
        self.inode_num = int(line[1])
        self.indir_level = int(line[2])
        self.logical_block_offset = int(line[3])
        self.indir_block_num = int(line[4])
        self.ref_block_num = int(line[5])


def I_node_and_Directory_Allocation_Audits(first_inode_pos,inode_count):

    allocated_inode=[]
    links=numpy.zeros(inode_count + first_inode_pos)
    parentof=numpy.zeros(inode_count + first_inode_pos)
    parentof[2]=2

    #scan the inode and print the inode numnber when it is a valid number and also in the FREELIST
    for allocated_inode_num in inode_element:
        if allocated_inode_num.inode_num != 0:
           allocated_inode.append(allocated_inode_num.inode_num)
           if allocated_inode_num.inode_num in ifree_element:
               print("ALLOCATED INODE %d ON FREELIST" %(allocated_inode_num.inode_num))

    #scan all the valid inode number to detect the UNALLOCATED inode and not in the free list
    for inode_num in range(first_inode_pos,inode_count):
        if inode_num not in allocated_inode and inode_num not in ifree_element:
            print("UNALLOCATED INODE %d NOT ON FREELIST" %(inode_num))

    #if inode's file type is zero then the it should in the freelist
    for allocated_inode_num in inode_element:
        if allocated_inode_num.inode_num not in allocated_inode and allocated_inode_num.inode_num not in ifree_element:
            emptylist="empty"
        elif allocated_inode_num.file_type == '0' and allocated_inode_num.inode_num not in ifree_element:
            print("UNALLOCATED INODE %d NOT ON FREELIST" %(allocated_inode_num.inode_num))


    #if the inode number of the directory is out of boundary or not in the allocated inode set, output error_message
    #else count the corresponding inode number in the links list
    for dir in dirent_element:
        if dir.ref_inode_num > inode_count or dir.ref_inode_num < 1:
            print("DIRECTORY INODE %d NAME %s INVALID INODE %d"     %(dir.parent_inode, dir.name, dir.ref_inode_num))
        elif dir.ref_inode_num not in allocated_inode:
            print("DIRECTORY INODE %d NAME %s UNALLOCATED INODE %d" %(dir.parent_inode, dir.name, dir.ref_inode_num))
        else:
            links[dir.ref_inode_num] += 1

    #scan the all the inode for incorrect link_count
    for inode in inode_element:
        if links[inode.inode_num] != inode.link_count:
            print("INODE %d HAS %d LINKS BUT LINKCOUNT IS %d" %(inode.inode_num, links[inode.inode_num], inode.i_links_count ))

    #scan the directory element and record their parent's inode number
    for dir in dirent_element:
        if dir.name != "'.'" and dir.name != "'..'" :
            parentof[dir.ref_inode_num]=dir.parent_inode

    #detect inconsistency for '.' and '..'
    for dir_element in dirent_element:
        if dir_element.name == "'.'" and dir_element.ref_inode_num != dir_element.parent_inode:
            print("DIRECTORY INODE %d NAME '.' LINK TO INODE %d SHOULD BE %d"  %(dir_element.parent_inode, dir_element.ref_inode_num, dir_element.parent_inode))
        if dir_element.name == "'..'" and parentof[dir_element.parent_inode] != dir_element.ref_inode_num:
            print("DIRECTORY INODE %d NAME '..' LINK TO INODE %d SHOULD BE %d" %(dir_element.parent_inode, dir_element.ref_inode_num, parentof[dir_element.parent_inode]))

def check_inode_table(MAX_BLOCK_COUNT, FIRST_DATA_BLOCK_NUM, inode):
    offset = 0
    level = 0
    for block_num in inode.dirent_blocks:
        if block_num < 0 or block_num >= MAX_BLOCK_COUNT:
            print("INVALID BLOCK " + str(block_num) + " IN INODE " + str(inode.inode_num) + " AT OFFSET " + str(offset)) 
        elif block_num > 0 and block_num < FIRST_DATA_BLOCK_NUM:
            print("RESERVED BLOCK " + str(block_num) + " IN INODE " + str(inode.inode_num) + " AT OFFSET " + str(offset))
        elif block_num != level: 
            if block_num not in balloc_element:
                balloc_element[block_num] = [[inode.inode_num, offset, level]]
            else:
                bdup_element.add(block_num)
                balloc_element[block_num].append([inode.inode_num, offset, level])
        offset += 1

def check_singly_indirect_block(MAX_BLOCK_COUNT, FIRST_DATA_BLOCK_NUM, inode):
    block_num=inode.indirect_blocks[0]
    offset = 12
    level = 1
    if inode.indirect_blocks[0] < 0 or inode.indirect_blocks[0] >= MAX_BLOCK_COUNT:
        print("INVALID INDIRECT BLOCK " + str(block_num) + " IN INODE " + str(inode.inode_num) + " AT OFFSET " + str(offset))
    elif block_num > 0 and block_num < FIRST_DATA_BLOCK_NUM:
        print("RESERVED INDIRECT BLOCK " + str(block_num) + " IN INODE " + str(inode.inode_num) + " AT OFFSET " + str(offset))
    elif block_num != level-1:
        if block_num not in balloc_element:
            balloc_element[block_num] = [[inode.inode_num, offset, level]]
        else:
            bdup_element.add(block_num)
            balloc_element[block_num].append([inode.inode_num, offset, level])

def check_doubly_indirect_block(MAX_BLOCK_COUNT,FIRST_DATA_BLOCK_NUM, inode):
    block_num=inode.indirect_blocks[1]
    offset = 268
    level = 2
    if inode.indirect_blocks[1] < 0 or inode.indirect_blocks[1] >= MAX_BLOCK_COUNT:
        print("INVALID DOUBLE INDIRECT BLOCK " + str(block_num) + " IN INODE " + str(inode.inode_num) + " AT OFFSET " + str(offset))
    elif block_num > 0 and block_num < FIRST_DATA_BLOCK_NUM:
        print("RESERVED DOUBLE INDIRECT BLOCK " + str(block_num) + " IN INODE " + str(inode.inode_num) + " AT OFFSET " + str(offset))
    elif block_num != 0:
        if block_num not in balloc_element:
            balloc_element[block_num] = [[inode.inode_num, offset, level]]
        else:
            bdup_element.add(block_num)
            balloc_element[block_num].append([inode.inode_num, offset, level])

def check_triply_indirect_block(MAX_BLOCK_COUNT,FIRST_DATA_BLOCK_NUM, inode):
    offset = 65804
    level = 3
    block_num=inode.indirect_blocks[2]
    if inode.indirect_blocks[2] < 0 or inode.indirect_blocks[2] >= MAX_BLOCK_COUNT:
        print("INVALID TRIPLE INDIRECT BLOCK " + str(block_num) + " IN INODE " + str(inode.inode_num) + " AT OFFSET " + str(offset))
    elif block_num > 0 and block_num < FIRST_DATA_BLOCK_NUM:
        print("RESERVED TRIPLE INDIRECT BLOCK " + str(block_num) + " IN INODE " + str(inode.inode_num) + " AT OFFSET " + str(offset))
    elif block_num != 0:
        if block_num not in balloc_element:
            balloc_element[block_num] = [[inode.inode_num, offset, level]]
        else:
            bdup_element.add(block_num)
            balloc_element[block_num].append([inode.inode_num, offset, level])

def check_inner_indirect_block_size(MAX_BLOCK_COUNT,FIRST_DATA_BLOCK_NUM):
    # check the inner indirect block_size
    for indirect in indirect_element:
        block_num = indirect.element
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
            print("INVALID " + str(slevel) + " BLOCK " + str(block_num) + " IN INODE " + str(indirect.inode_num) + " AT OFFSET " + str(offset))
        elif block_num > 0 and block_num < FIRST_DATA_BLOCK_NUM:
            print("RESERVED " + str(slevel) + " BLOCK " + str(block_num) + " IN INODE " + str(indirect.inode_num) + " AT OFFSET " + str(offset))
        elif block_num != 0: # find used blocks
            if block_num not in balloc_element:
                balloc_element[block_num] = [[indirect.inode_num, indirect.logical_block_offset, indirect.indir_level]]
            else:
                bdup_element.add(block_num)
                balloc_element[block_num].append([indirect.inode_num, indirect.logical_block_offset, indirect.indir_level])

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
                print("DUPLICATE " + str(slevel) + " BLOCK " + str(num) + " IN INODE " + str(ref[0]) + " AT OFFSET " + str(ref[1]))

def Block_Consistency_Audits(MAX_BLOCK_COUNT, FIRST_DATA_BLOCK_NUM):
    for inode in inode_element:
        if inode.file_type == 's' and inode.file_size <= MAX_BLOCK_COUNT:
            continue
        check_inode_table(MAX_BLOCK_COUNT,FIRST_DATA_BLOCK_NUM, inode)
        check_singly_indirect_block(MAX_BLOCK_COUNT,FIRST_DATA_BLOCK_NUM, inode)
        check_doubly_indirect_block(MAX_BLOCK_COUNT, FIRST_DATA_BLOCK_NUM, inode)
        check_triply_indirect_block(MAX_BLOCK_COUNT, FIRST_DATA_BLOCK_NUM, inode)
    check_inner_indirect_block_size(MAX_BLOCK_COUNT,FIRST_DATA_BLOCK_NUM)
    check_blocks(MAX_BLOCK_COUNT,FIRST_DATA_BLOCK_NUM)

def Calc_max_block_count(element):
    return element.total_blocks

def Calc_first_data_block_num(gp, sblock):
    return int(gp.first_inode_block + math.ceil(gp.inodes_per_group * sblock.super_inode_size / sblock.block_size))



def main ():

    if len(sys.argv) != 2:
        print_error("Error: the input argument is not two \n")

    #open the input csv file
    try:
        input_csvfile=open(sys.argv[1], 'r')
    except IOError:
        print_error("Error: cannot open the input csv file \n")

    #read the csv infromation and stored them to corresponding class element
    filereader=csv.reader(input_csvfile)

    for element in filereader:
        if element[0] == "SUPERBLOCK":
            superblock_element = superblock(element)
        elif element[0] == "GROUP":
            group_element = group(element)
        elif element[0] == "BFREE":
            bfree_element.append(int(element[1]))
        elif element[0] == "IFREE":
            ifree_element.append(int(element[1]))
        elif element[0] == "INODE":
            inode_element.append(inode(element))
        elif element[0] == "DIRENT":
            dirent_element.append(dirent(element))
        elif element[0] == "INDIRECT":
            indirect_element.append(indirect(element))
        else:
            print_error("Error: the input csv file contains invild element: \n")

    #the maximum number of block
    MAX_BLOCK_COUNT = Calc_max_block_count(superblock_element)
    #first block position index
    FIRST_DATA_BLOCK_NUM = Calc_first_data_block_num(group_element, superblock_element)

    #scan the blocks and print error_message
    Block_Consistency_Audits(MAX_BLOCK_COUNT, FIRST_DATA_BLOCK_NUM)
    #scan the inode and directory, and print error_message
    I_node_and_Directory_Allocation_Audits(superblock_element.first_unres_inode, superblock_element.total_inodes)


if __name__ == '__main__':
    main()
