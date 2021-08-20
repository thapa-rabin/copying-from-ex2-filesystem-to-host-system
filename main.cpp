#include <iostream>
#include <unistd.h>
#include <fcntl.h>
#include <iomanip>
#include <cmath>
#include <cstring>
#include<string>
#include <cstddef>
#include <libgen.h>

using namespace std;


void primaryDisplay();

//initializing 5 basic VDI I/O functions
struct VDIFile *vdiOpen (char *fn);
void vdiClose (struct VDIFile *f);
ssize_t vdiRead (struct VDIFile *f,void *buf,size_t count);
ssize_t  vdiWrite (struct VDIFile *f,void *buf,size_t count);
off_t vdiSeek (VDIFile *f,off_t offset, int anchor);

//Inititalizing 5 basic partition I/O functions
struct PartitionFile *partitionOpen(struct VDIFile *vdiFile, struct PartitionEntry partitionEntry);
void partitionClose(struct PartitionFile *f);
ssize_t partitionRead(struct PartitionFile *partitionFile,void *buf,size_t count);
ssize_t partitionWrite(struct PartitionFile *f,void *buf,size_t count);
off_t partitionSeek(struct PartitionFile *f,off_t offset,int anchor);
void dumpPartitionEntry(struct PartitionEntry partitionEntry);
void readPartitionEntry( struct VDIFile* vdiFile,PartitionEntry partitionEntry);

//Initializing I/O functions for Ext2File access
struct Ext2File *ext2Open (char *fn, int32_t pNum);
void ext2Close (struct Ext2File *f);
int32_t fetchBlock(struct Ext2File *f,uint32_t blockNum, void *buf);
int32_t writeBlock(struct Ext2File *f,uint32_t blockNum, void *buf);
int32_t fetchSuperblock(struct Ext2File *f,uint32_t blockNum, struct Ext2Superblock *sb);
int32_t writeSuperblock(struct Ext2File *f,uint32_t blockNum, struct Ext2Superblock *sb);
void dumpSuperBlock(Ext2Superblock *sb);
int32_t fetchBGDT(struct Ext2File *f,uint32_t blockNum,struct Ext2BlockGroupDescriptor *bgdt);
int32_t writeBGDT(struct Ext2File *f,uint32_t blockNum,struct Ext2BlockGroupDescriptor *bgdt);
void dumpBlockGroupDescriptorTable (struct Ext2File *f);
void dumpBlockGroupDescriptorTableEntry (Ext2BlockGroupDescriptor *blockGroupDescriptor);

//I/O functions to access inodes
int32_t fetchInode(struct Ext2File *f,uint32_t iNum, struct Inode *buf);
int32_t writeInode(struct Ext2File *f,uint32_t iNum, struct Inode *buf);
int32_t inodeInUse(struct Ext2File *f,uint32_t iNum);
uint32_t allocateInode(struct Ext2File *f,int32_t group);
int32_t freeInode(struct Ext2File *f,uint32_t iNum);
void dumpInode(Inode *inode);

//Functions to access data blocks in an ext2file 
int32_t fetchBlockFromFile(struct Ext2File *f,struct Inode *i,uint32_t bNum, void *buf);
int32_t writeBlockToFile(struct Ext2File *f,struct Inode *i,uint32_t bNum, void *buf,uint32_t iNum);
int32_t allocateBlock(struct Ext2File *f, uint32_t groupNumber);
int32_t blockInUse (struct Ext2File *f,uint32_t blockGroupNumber,uint32_t bNum);

//functions to access directories in an ext2 file system 
struct Directory *openDir(struct  Ext2File *f, uint32_t iNum);
bool getNextDirent(struct Directory *d,uint32_t &iNum,char *name);
void rewindDir(struct Directory *d);
void closeDir(struct Directory *d);

//function to navigate within directories in an ext2 file system 
uint32_t searchDir(struct Ext2File *f,uint32_t iNum,char *target);
uint32_t traversePath(Ext2File *f);

//function to list files/ directories in an ext2file systme 
void displayAllFilesInVDI (struct  Ext2File *f,uint32_t dirNum);
void displayFilesWithInfo(Ext2File *f, uint32_t dirNum);

void programEngine();
string returnPermission(string permission,uint16_t i_mode);
uint32_t copyFilesFromHost (Ext2File *f, char *hostFilePath);

//Contains of VDI header structure
struct HeaderStructure{
    /** Just text info about image type, for eyes only. */
    char szFileInfo[64];
    /** The image signature (VDI_IMAGE_SIGNATURE). */
    uint32_t u32Signature;
    /** The image version (VDI_IMAGE_VERSION). */
    uint32_t u32Version;
    /** Size of this structure in bytes. */
    uint32_t cbHeader;
    /** The image type (VDI_IMAGE_TYPE_*). */
    uint32_t u32Type;
    /** Image flags (VDI_IMAGE_FLAGS_*). */
    uint32_t fFlags;
    /** Image comment. (UTF-8) */
    char szComment[256];
    /** Offset of blocks array from the beginning of image file.
     * Should be sector-aligned for HDD access optimization. */
    uint32_t offBlocks;
    /** Offset of image data from the beginning of image file.
     * Should be sector-aligned for HDD access optimization. */
    uint32_t offData;
    /** Cylinders. */
    uint32_t    cCylinders;
    /** Heads. */
    uint32_t    cHeads;
    /** Sectors per track. */
    uint32_t    cSectors;
    /** Sector size. (bytes per sector) */
    uint32_t    cbSector;
    /** Was BIOS HDD translation mode, now unused. */
    uint32_t u32Dummy;
    /** Size of disk (in bytes). */
    uint64_t cbDisk;
    /** Block size. (For instance VDI_IMAGE_BLOCK_SIZE.) Should be a power of 2! */
    uint32_t cbBlock;
    /** Size of additional service information of every data block.
     * Prepended before block data. May be 0.
     * Should be a power of 2 and sector-aligned for optimization reasons. */
    uint32_t cbBlockExtra;
    /** Number of blocks. */
    uint32_t cBlocks;
    /** Number of allocated blocks. */
    uint32_t cBlocksAllocated;
    /** UUID of image. */
    char uuidCreate[16];
    /** UUID of image's last modification. */
    char uuidModify[16];
    /** Only for secondary images - UUID of previous image. */
    char uuidLinkage[16];
    /** Only for secondary images - UUID of previous image's last modification. */
    char uuidParentModify[16];
}__attribute__((packed));
//structure that contains data necessary to run 5 basic VDI I/O function
struct VDIFile {
    int fd;
    HeaderStructure header;
    ssize_t cursor;
}__attribute__((packed));

/*Each Partition Table entry is 16 bytes long,
making a maximum of four entries available in a partition table.This is a struct for a partition entry of a partition
 table (features taken from https://thestarman.pcministry.com/asm/mbr/PartTables.htm)*/
struct PartitionEntry{
    //status of a partition entry (Active or bootable), 1 byte
    uint8_t status;
    //Cylinder head sector of first absolute sector in partition,3 bytes
    uint8_t chsFirstSector[3];
    //Type of partition based on methods used to access them,1 byte
    uint8_t type;
    //Cylinder head sector address of last absolute sector in partition, 3 bytes
    uint8_t chsLastSector[3];
    /*Logical Block Address of first absolute sector in the partition, 4 bytes*/
    uint32_t LBAFirstSector;
    //Number of sectors in partition, sector stores a fixed amount of data in a partition,4 bytes
    uint32_t numSectors;
} __attribute__((packed));

/*The partition table describes the partitions of a storage device.
This is a struct that holds the data necessary to work with partition.*/
struct PartitionFile{
    struct VDIFile *f;
    //max of 4 partition entries in a partition
    PartitionEntry partitionEntry;
}__attribute__((packed));

//main data structure of a UNIX file
struct Ext2Superblock {
    uint32_t s_inodes_count;
    uint32_t s_blocks_count;
    uint32_t s_r_blocks_count;
    uint32_t s_free_blocks_count;
    uint32_t s_free_inodes_count;
    uint32_t s_first_data_block;
    uint32_t s_log_block_size;
    uint32_t s_log_frag_size ;
    uint32_t s_blocks_per_group;
    uint32_t s_frags_per_group;
    uint32_t s_inodes_per_group;
    time_t s_mtime;
    time_t s_wtime;
    uint16_t s_mnt_count;
    uint16_t s_max_mnt_count;
    uint16_t s_magic;
    uint16_t s_state;
    uint16_t s_errors;
    uint16_t s_minor_rev_level;
    time_t s_lastcheck;
    time_t s_checkinterval;
    uint32_t s_creator_os;
    uint32_t s_rev_level;
    uint16_t s_def_resuid;
    uint16_t s_def_resgid;
    uint32_t s_first_ino;
    uint16_t s_inode_size;
    uint16_t s_block_group_nr;
    uint32_t s_feature_compat;
    uint32_t s_feature_incompat;
    uint32_t s_feature_ro_compat;
    char        s_uuid[16];
    char s_volume_name[16];
    char        s_last_mounted[64];
    uint32_t s_algo_bitmap;
    unsigned char s_prealloc_blocks;
    unsigned char_prealloc_dir_blocks;
    unsigned short s_padding_1;
    unsigned  char s_journal_uuid[16];
    uint32_t s_journal_inum;
    uint32_t        s_journal_dev;
    uint32_t s_last_orphan;
    uint32_t        s_hash_seed[4];
    uint8_t s_def_hash_version;
    unsigned char s_reserved_char_pad;
    unsigned short s_reserved_word_pad;
    uint32_t        s_default_mount_options;
    uint32_t s_first_meta_bg;
    unsigned int s_reserved[190];
}__attribute__((packed));

//descriptions of group of blocks in a partition
struct Ext2BlockGroupDescriptor{
    uint32_t 	bg_block_bitmap;	/* Blocks bitmap block */
    uint32_t 	bg_inode_bitmap;	/* Inodes bitmap block */
    uint32_t 	bg_inode_table;		/* Inodes table block */
    uint16_t 	bg_free_blocks_count;	/* Free blocks count */
    uint16_t 	bg_free_inodes_count;	/* Free inodes count */
    uint16_t 	bg_used_dirs_count;	/* Directories count */
    uint16_t 	bg_pad;
    uint32_t 	bg_reserved[3];
}__attribute__((packed));

//Ext2File with partitions, superblock, and blockgroupDescriptors table for different block groups in  a partition
struct Ext2File{
    PartitionFile *partitionFile;
    Ext2Superblock superBlocks;
    ssize_t numberofBlockGroups;
    //description of every block group
    Ext2BlockGroupDescriptor* blockGroupDescriptorstable;
}__attribute__((packed));


//inode holds all the information about a file
//this struct was provided by Dr.Kramer on discord
struct Inode {
    uint16_t
            i_mode,
            i_uid;
    uint32_t
            i_size,
            i_atime,
            i_ctime,
            i_mtime,
            i_dtime;
    uint16_t
            i_gid,
            i_links_count;
    uint32_t
            i_blocks,
            i_flags,
#pragma clang diagnostic push
#pragma ide diagnostic ignored "OCUnusedGlobalDeclarationInspection"
            i_osd1,
#pragma clang diagnostic pop
            i_block[15],
            i_generation,
            i_file_acl,
            i_sizeHigh,
#pragma clang diagnostic push
#pragma ide diagnostic ignored "OCUnusedGlobalDeclarationInspection"
            i_faddr;
#pragma clang diagnostic pop
    uint16_t
            i_blocksHigh,
#pragma clang diagnostic push
#pragma ide diagnostic ignored "OCUnusedGlobalDeclarationInspection"
            reserved16,
#pragma clang diagnostic pop
            i_uidHigh,
            i_gidHigh;
#pragma clang diagnostic push
#pragma ide diagnostic ignored "OCUnusedGlobalDeclarationInspection"
    uint32_t
            reserved32;
#pragma clang diagnostic pop
}__attribute__((packed));

struct Dirent {
    uint32_t iNum;
    uint16_t recLen;
    uint8_t nameLen, fileType,  name[1];  }__attribute__((packed));

struct Directory{
    ssize_t  cursor;
    uint32_t iNum;
    Inode inode;
    uint8_t *ptr;
    Ext2File *ext2File;
    Dirent *dirent;
};

uint32_t copyFileToHost(Ext2File*f, char *vdiFileName, char *hostFilePath);


int main(){
   programEngine();
}

void primaryDisplay(){
    cout << "1. Copy A File From a VDI File " << endl;
    cout << "2. List all the directories and files in a VDI File" << endl;
    cout << "3. Show properties of all the files and directories in a VDI File" << endl;
    cout << "Please enter 1,2, or 3 based on your choice:  " << endl;
}

//main engine of the program that processes users choice 
void programEngine(){
    primaryDisplay();
    int choice;
    cin >> choice;

    while (choice <1 || choice > 3 ){
        cout << endl;
        cout << "Please make a choice between 1 and 3" << endl;
        primaryDisplay();
        cin >> choice;
        if (choice > 0 && choice <=3 ){
            break;
        }
    }

    if (choice ==1){
        cout << "Type the path to the vdiFile System which you would like to copy from (Example : ../Test-fixed-4k.vdi) " << endl;
        char *vdiFileName = new char [256];
        cin >> vdiFileName;
        cout << "Type the path to the file in the vdiFileSystem which you would like to copy to your host system (Example: /examples/08.Strings/StringCaseChanges/StringCaseChanges.txt)" << endl;
        char *vdiFile = new char [256];
        cin >> vdiFile;
        cout << "Type the path to the name of the file in your host which you would like to copy over to" << endl;
        char *hostFilePath = new char [256];
        cin >> hostFilePath;
        Ext2File *ext2File= ext2Open(vdiFileName,0);
        copyFileToHost(ext2File,vdiFile,hostFilePath);
    }else if (choice ==2){
        cout << "Type the path to the vdiFile System whose files and directories you want to look at(Example : ../Test-fixed-4k.vdi) " << endl;
        char *vdiFileName = new char [256];
        cin >> vdiFileName;
        cout << endl;
        Ext2File *ext2File= ext2Open(vdiFileName,0);
        displayAllFilesInVDI (ext2File,2);
    }else{
        cout << "Type the path to the vdiFile System whose files and directories information you want to look at(Example : ../Test-fixed-4k.vdi) " << endl;
        char *vdiFileName = new char [256];
        cin >> vdiFileName;
        cout << endl;
        Ext2File *ext2File= ext2Open(vdiFileName,0);
        displayFilesWithInfo(ext2File, 2);
    }

    cout << "Do you wish to end the software? Type Y or N " << endl;
    char decision;
    cin >> decision;
    if (decision =='N') programEngine();
    else{
        cout << "Thank you for using the software!";
    }
}

//start of step 1
//open the passed vdi file
struct VDIFile *vdiOpen (char *fn){
    int fd = open(fn,O_RDWR|O_BINARY);
    //if file opened
    if (fd>1){
        //create a vdi header structure
        HeaderStructure header ={};
        //load the header bytes to the struture
        VDIFile *vdiFileStruct = new VDIFile;
        read(fd,&(vdiFileStruct->header),sizeof(HeaderStructure));
        //create a VDI file structure with that header,set the cursor to 0 and file descriptor to whatever was returned from open
        //set the pointer to have the address of the VDIFileStructure we created
        vdiFileStruct->fd = fd; vdiFileStruct->cursor = 0;
        //return the  address of the VDI File structure we created
        return vdiFileStruct;
    }

    else {
        //return a null pointer if the vdi file did not open
        return nullptr;
    }
}

//read the vdi file
ssize_t vdiRead (struct VDIFile *f,void *buf,size_t count){
    lseek(f->fd, f->cursor+f->header.offData, SEEK_SET);
    read(f->fd,buf,count);
    return 0;
}

//write on a vdi file
ssize_t  vdiWrite (struct VDIFile *f,void *buf, size_t count){
    //seek the first position to be written at by adding the cursor to the start of the VDI file's data space
    lseek(f->fd,f->cursor + f->header.offData, SEEK_SET);
    //write count number of bytes from the file with file descriptor f to the buffer "buf"
    write(f->fd,buf,count);
    return 0;
}

//seek to a position within a vdi file
off_t vdiSeek (VDIFile *f, off_t offset, int anchor){
    //create a new location within the file
    off_t location;

    //if seeking data from the beginning of the file (not the beginning of the data in the file)
    if (anchor == SEEK_SET){
        location = offset;
    }

    //if seeking from the current position of the cursos
    else if (anchor == SEEK_CUR){
        location = f->cursor+offset;
    }

    //if seeking from the end of the file
    else if (anchor == SEEK_END){
        location = offset + f->header.cbDisk;
    }

    if (location >= 0 && location < f->header.cbDisk) f->cursor = location;
    return f->cursor;
}

void vdiClose (struct VDIFile *f){
    //close the file passed
    close((*f).fd);
    //delete any dynamically created memory regions
    delete f;
}
//end of step 1

//start of step 2
/*Open the partition file of the given VDI */
struct PartitionFile *partitionOpen(struct VDIFile* vdiFile, struct PartitionEntry partitionEntry){
    //creating a new partition file
    PartitionFile *ptr= new PartitionFile;
    //partition is of the following vdi file
    ptr->f = vdiFile;
    ptr->partitionEntry=partitionEntry;
    //returning a partition file of the given VDI file with the passes partition entry
    return ptr;
}

void partitionClose(struct PartitionFile *f){
    //closing the vdi file containing that partition
    close( (*f).f->fd);
}

ssize_t partitionRead(struct PartitionFile *partitionFile,void *buf,size_t count){
    /* inding the beginning of partition:
     LBA is basically sector number. MBR is the first sector with LBA 0.
     The MBR is not located in a partition; it is located at a first sector of the device (physical offset 0), preceding the first partition.
     After first 446 bytes in a VDI header, the next 64 bytes contain 4 partition entries of size 16 bytes each.
     Last 4 bytes of each partition entry contains LBA.LBA inside first partition entry tells you the number of
     sectors from first sector (mbr) to given partition. So, lba of first partition*512 gives total number of bytes from the beginning of the disk
     the first partition.*/


    // If cursor is outside the partition, read only the contents reduce our count to read the contents only within our partition
    if ( (partitionFile->f->cursor + count) >  (512 * (partitionFile->partitionEntry.LBAFirstSector + partitionFile->partitionEntry.numSectors)) ){
        //remove the part of the count outside the partition
        count = (partitionFile->partitionEntry.LBAFirstSector+ partitionFile->partitionEntry.numSectors)*512- partitionFile->f->cursor;
    }

    return vdiRead(partitionFile->f,buf,count);
}

ssize_t partitionWrite(struct PartitionFile *partitionFile,void *buf,size_t count){
    if ( (partitionFile->f->cursor + count) >  (512 * (partitionFile->partitionEntry.LBAFirstSector + partitionFile->partitionEntry.numSectors)) ){
        //remove the part of the count outside the partition
        count = (partitionFile->partitionEntry.LBAFirstSector+ partitionFile->partitionEntry.numSectors)*512- partitionFile->f->cursor;
    }

    return vdiWrite(partitionFile->f,buf,count);
}

off_t partitionSeek(struct PartitionFile *f,off_t offset,int anchor){
    off_t location;
    //store the current position of the cursor
    int64_t cur = f->f->cursor;

    //new location of the cursor to seek to
    int64_t newCur;

    //seek the cursor from the beginning of the partition to the given offset
    newCur = vdiSeek(f->f,offset+f->partitionEntry.LBAFirstSector*512,anchor);

    //do not change the cursor to the sought position if it is outside our partition
    if (newCur < f->partitionEntry.LBAFirstSector*512 ||
        newCur >= 512 *(f->partitionEntry.LBAFirstSector+f->partitionEntry.numSectors) )
        f->f->cursor = cur;

    //return out current cursor position in the partition
    return f->f->cursor - f->partitionEntry.LBAFirstSector*512;
}

//this prints a partition entry is easy to read format
void dumpPartitionEntry(PartitionEntry partitionEntry){
    cout << endl;
    cout << "LBAFirstSector : " << partitionEntry.LBAFirstSector << endl;
    cout << "LBA Sector Count : " << partitionEntry.numSectors << endl;
};

void readPartitionEntry( struct VDIFile* vdiFile,PartitionEntry *partitionEntry){
    // Use vdiSeek to seek to the location of the partition table
    vdiSeek(vdiFile,446,SEEK_SET);
    // Use vdiRead to read the data of the partition table into the array of entries
    vdiRead(vdiFile,partitionEntry,64);}
//end of step 2

//start of step 3
struct Ext2File *ext2Open (char *fn, int32_t pNum){
    //creating a pointer to a new Ext2File
    Ext2File *ptr= new Ext2File;
    //open the vdi file
    VDIFile *vdiFile = vdiOpen(fn);
    PartitionEntry *partitionEntry= NULL;
    partitionEntry = new PartitionEntry[4];
    readPartitionEntry(vdiFile,partitionEntry);
    //open the partition which the user wants to open
    ptr->partitionFile= partitionOpen(vdiFile,partitionEntry[pNum]);

    //seek to the super block of the partition and read into the  superblock of new Ext2File it
    partitionSeek(ptr->partitionFile,1024 ,SEEK_SET);
    partitionRead(ptr->partitionFile,&ptr->superBlocks, 1024);


    //update two fields in the superblock
    ptr->superBlocks.s_log_block_size= 1024 << ptr->superBlocks.s_log_block_size;
    ptr->numberofBlockGroups = (ptr->superBlocks.s_blocks_count/ptr->superBlocks.s_blocks_per_group);

    //set the size of the block group descriptor table based on the number of block groups in the file
    ptr->blockGroupDescriptorstable = new Ext2BlockGroupDescriptor[ptr->numberofBlockGroups+1];

    //reading block descriptor table of each block group
    for (int i = 0; i <= ptr->numberofBlockGroups; ++i) {
        //seek to the second block of the block group
        partitionSeek(ptr->partitionFile,(ptr->superBlocks.s_first_data_block+1) *ptr->superBlocks.s_log_block_size+32*i,SEEK_SET);
        //read into the block group descriptor of that block group so that each block group descriptor entries fill the block descriptor table
        partitionRead(ptr->partitionFile,&ptr->blockGroupDescriptorstable[i],32);
    }

    return ptr;
}

void ext2Close (struct Ext2File *f){
    close (f->partitionFile->f->fd);
    delete(f);
}

//fetch the block with a given block number from partition in an Ext2File
int32_t fetchBlock(struct Ext2File *f,uint32_t blockNum, void *buf){
    //seek to the block whose block number is given
    partitionSeek(f->partitionFile,blockNum*f->superBlocks.s_log_block_size ,SEEK_SET);
    //read that entire block into the buffer
    int32_t readSuccess = partitionRead(f->partitionFile,buf, f->superBlocks.s_log_block_size);
    if (readSuccess==0){
        return 0;
    }else{
        return -1;
    }
}

//write from a buffer to a given block in a partition in an Ext2File
int32_t writeBlock(struct Ext2File *f,uint32_t blockNum, void *buf){
    //seek to the block whose block number is given
    partitionSeek(f->partitionFile,blockNum*f->superBlocks.s_log_block_size ,SEEK_SET);
    //write the buffer to the given block
    int32_t writeSuccess = partitionWrite(f->partitionFile,buf, f->superBlocks.s_log_block_size);
    if (writeSuccess==0){
        return 0;
    }else{
        return -1;
    }
}

//fetch the copy of the superblock in the block group of the given block Num
int32_t fetchSuperblock(struct Ext2File *f,uint32_t blockNum, struct Ext2Superblock *sb){
    //find the group in which the given block is contained.
    int groupNumber = round(blockNum/ f->superBlocks.s_blocks_per_group);
    //if accessing the main super block in block group 0, check to see if we need to  skip a block before accessing  superblock
    //Block group zero ends at 8191 in a 1k vdi file.
    if (groupNumber==0){
        //seek to the end of the block before superblock
        partitionSeek(f->partitionFile,f->superBlocks.s_first_data_block *f->superBlocks.s_log_block_size,SEEK_SET);
        //read 1024 bytes from there into the passed superblock
        partitionRead(f->partitionFile,sb,1024);
        (sb->s_log_block_size) = 1024 << sb->s_log_block_size;
    }
        //else if accessing the copies of super block in other blocks,
        // find the position where the superblock begins based on the number of block groups before that superblock
    else{
        //create a buffer of the size of a block in the partition
        void *buff = malloc(f->superBlocks.s_log_block_size);

        //find the number of block number to escape before accessing the copy of superblock in the block group of the passed blockNum
        int blockNumber = groupNumber * f->superBlocks.s_blocks_per_group ;

        //fetch the superblock into the buffer
        fetchBlock(f,blockNumber,buff);

        //copy 1024 bytes from the buffer to the superblock
        memcpy(sb,buff,1024);

        //update the size of the block in the  superblock
        (sb->s_log_block_size) = 1024 << sb->s_log_block_size;
    }
    cout << "The superblock has been fetched from the block group of the given block." << endl;
    return 0;
}

//write the given superblock to the blockNum
int32_t writeSuperblock(struct Ext2File *f,uint32_t blockNum, struct Ext2Superblock *sb){
    //find the group in which the given block is contained.
    int groupNumber = round(blockNum/ f->superBlocks.s_blocks_per_group);

    //if copying to the main superblock in block group 0, seek to the main superblock and write from the buffer to the maing superblock
    if (groupNumber==0){
        partitionSeek(f->partitionFile,f->superBlocks.s_first_data_block *f->superBlocks.s_log_block_size,SEEK_SET);
        partitionWrite(f->partitionFile,sb,1024);
    }
        //If copying to the copies of superblock in other block groups, seek to the first block in those block groups and write to them
    else{
        void *buff = malloc(f->superBlocks.s_log_block_size);
        int blockNumber = groupNumber * f->superBlocks.s_blocks_per_group ;
        writeBlock(f,blockNumber,buff);
        memcpy(sb,buff,1024);
    }
    return 0;
}

//function to display contains of superblock structure
void dumpSuperBlock(Ext2Superblock *sb){
    cout << endl;
    cout << "~~~~~~~~~~The Given Superblock~~~~~~~~~~"<< endl;
    cout << "Number of inodes : " << (int) (uint32_t ) sb->s_inodes_count << endl;
    cout << "Number of blocks : " << (int) sb->s_blocks_count << endl;
    cout << "Number of reserved blocks : " << (int) (uint32_t ) sb->s_r_blocks_count << endl;
    cout << "Number of free blocks : " << (int) sb->s_free_blocks_count <<endl;
    cout << "Number of inodes count : " << (int) sb->s_free_inodes_count << endl;
    cout << "First data block : " << (int) sb->s_first_data_block << endl;
    cout << "Block size : " <<   (sb->s_log_block_size) <<endl;
    cout << "Log fragment size : " << sb->s_log_frag_size << endl;
    cout << "Blocks per group : " << sb->s_blocks_per_group << endl;
    cout << "Fragments per group : " << sb->s_frags_per_group << endl;
    cout << "Inodes per group : " << sb->s_inodes_per_group << endl;
    cout << "Mount count : " << sb->s_mnt_count << endl;
    cout << " Max mount countr : " << sb->s_max_mnt_count << endl;
    cout << "Magic number : 0x" << (hex) << sb->s_magic << endl;
}

//function to fetch a block group descriptor table of the block group in which block blockNum is contained
int32_t fetchBGDT(struct Ext2File *f,uint32_t blockNum,Ext2BlockGroupDescriptor *bgdt) {

    //find the group number of the block
    int groupNumber = round(blockNum/ f->superBlocks.s_blocks_per_group);

    //assign the values group of descriptor entry of the calculated group based on the values in the main block group descriptor table
    bgdt->bg_inode_bitmap= f->blockGroupDescriptorstable[groupNumber].bg_inode_bitmap;
    bgdt->bg_block_bitmap=f->blockGroupDescriptorstable[groupNumber].bg_block_bitmap;
    bgdt->bg_free_blocks_count=f->blockGroupDescriptorstable[groupNumber].bg_free_blocks_count;
    bgdt->bg_free_inodes_count=f->blockGroupDescriptorstable[groupNumber].bg_free_inodes_count;
    bgdt->bg_used_dirs_count=f->blockGroupDescriptorstable[groupNumber].bg_used_dirs_count;
    bgdt->bg_pad=f->blockGroupDescriptorstable[groupNumber].bg_pad;
    bgdt->bg_inode_table=f->blockGroupDescriptorstable[groupNumber].bg_inode_table;

    for (int i = 0; i <3 ; ++i) {
        bgdt->bg_reserved[i]= f->blockGroupDescriptorstable[groupNumber].bg_reserved[i];
    }

    return 0;
}

int32_t writeBGDT(struct Ext2File *f,uint32_t blockNum,Ext2BlockGroupDescriptor *bgdt) {

    //find the group number of the group in which blockNum exists
    int groupNumber = round(blockNum/ f->superBlocks.s_blocks_per_group);

    //write contents of bgdt to the block group descriptor of the block group groupNumber
    f->blockGroupDescriptorstable[groupNumber].bg_inode_bitmap= bgdt->bg_inode_bitmap;
    f->blockGroupDescriptorstable[groupNumber].bg_block_bitmap= bgdt->bg_block_bitmap;
    f->blockGroupDescriptorstable[groupNumber].bg_free_blocks_count=bgdt->bg_free_blocks_count;
    f->blockGroupDescriptorstable[groupNumber].bg_free_inodes_count= bgdt->bg_free_inodes_count;
    f->blockGroupDescriptorstable[groupNumber].bg_used_dirs_count=bgdt->bg_used_dirs_count;
    f->blockGroupDescriptorstable[groupNumber].bg_pad=bgdt->bg_pad;
    f->blockGroupDescriptorstable[groupNumber].bg_inode_table=bgdt->bg_inode_table;

    for (int i = 0; i <3 ; ++i) {
        f->blockGroupDescriptorstable[groupNumber].bg_reserved[i]=bgdt->bg_reserved[i];
    }

    return 0;
}

//displaying the main block group descriptor table
void dumpBlockGroupDescriptorTable (struct Ext2File *f){
    cout << endl;
    cout << "~~~~~~~~~~The Main Group Descriptor Table Entry~~~~~~~~~~" << endl;
    for (int i = 0; i <= f->numberofBlockGroups ; ++i) {
        cout << "Block Number : " << i <<endl;
        cout << "Block Bitmap : " << f->blockGroupDescriptorstable[i].bg_block_bitmap << endl;
        cout << "Inode Bitmap : " << f->blockGroupDescriptorstable[i].bg_inode_bitmap << endl;
        cout << "Inode Table : " << f->blockGroupDescriptorstable[i].bg_inode_table << endl;
        //cout << "Free Blocks : " << f->blockGroupDescriptorstable[i].bg_free_blocks_count <<endl;
        //cout << "Free Inodes : " <<f->blockGroupDescriptorstable[i].bg_free_inodes_count <<endl;
        cout << endl;
    }
}

//displaying one block descriptor entry
void dumpBlockGroupDescriptorTableEntry (Ext2BlockGroupDescriptor *blockGroupDescriptor){
    cout << endl;
    cout << "~~~~~~~~~~The Given Group Descriptor Table Entry~~~~~~~~~~" << endl;
    cout << "Block Bitmap : " << blockGroupDescriptor->bg_block_bitmap << endl;
    cout << "Inode Bitmap : " << blockGroupDescriptor->bg_inode_bitmap << endl;
    cout << "Inode Table : " << blockGroupDescriptor->bg_inode_table << endl;

}
//end of step 3

//start of step 4
int32_t fetchInode(struct Ext2File *f,uint32_t iNum, struct Inode *buf){
    //find the block group in which the iNode is contained
    int blockGroupNumber = (iNum-1)/f->superBlocks.s_inodes_per_group;
    iNum = (iNum - 1) % f->superBlocks.s_inodes_per_group;
    //find the number of inodes per block
    int num_inodes_per_block= f->superBlocks.s_log_block_size/f->superBlocks.s_inode_size;
    int  inode_block_num = (iNum / num_inodes_per_block) + f->blockGroupDescriptorstable[blockGroupNumber].bg_inode_table;
    int offset_of_inode_in_block= iNum % num_inodes_per_block;
    uint8_t * tempBlock = NULL;
    tempBlock= new uint8_t [f->superBlocks.s_log_block_size];
    fetchBlock(f,inode_block_num,tempBlock);
    *buf = ( (Inode *) tempBlock)[offset_of_inode_in_block];
    return 0;
}

//write inode buf into the given inode number
int32_t writeInode(struct Ext2File *f,uint32_t iNum, struct Inode *buf) {
//find the block group in which the iNode is contained
    int blockGroupNumber = (iNum-1)/f->superBlocks.s_inodes_per_group;
    iNum = (iNum - 1) % f->superBlocks.s_inodes_per_group;
    //find the number of inodes per block
    int num_inodes_per_block= f->superBlocks.s_log_block_size/f->superBlocks.s_inode_size;
    int  inode_block_num = (iNum / num_inodes_per_block) + f->blockGroupDescriptorstable[blockGroupNumber].bg_inode_table;
    int offset_of_inode_in_block= iNum % num_inodes_per_block;
    uint8_t * tempBlock = NULL;
    tempBlock= new uint8_t [f->superBlocks.s_log_block_size];
    writeBlock(f,inode_block_num,tempBlock);
    *buf = ( (Inode *) tempBlock)[offset_of_inode_in_block];
    return 0;
}

int32_t inodeInUse(struct Ext2File *f,uint32_t iNum){
    iNum--;
    uint8_t * block_with_inode_bitmap = NULL;
    block_with_inode_bitmap= new uint8_t [f->superBlocks.s_log_block_size];
    //find the block group in which the given iNum  is contained
    uint8_t blockGroupNumber = iNum/f->superBlocks.s_inodes_per_group;
    //fetch the only block in the group with inode bitmap
    fetchBlock(f, f->blockGroupDescriptorstable[blockGroupNumber].bg_inode_bitmap,block_with_inode_bitmap);
    //find the byte that has the bitmap of the given inode
    uint8_t byte_Position = iNum/ 8;
    //check for the inode bitmap within that byte
    return ( (block_with_inode_bitmap[byte_Position] & (1 << iNum %8)) !=0 );
}

uint32_t allocateInode(struct Ext2File *f,int32_t group) {
    uint32_t  iNumfinal=0;
    if (group ==-1){
        group= 0;
    }

    //find the inode number of the first inode in the group
    uint32_t first_inode= 1+group*f->superBlocks.s_inodes_per_group;
    //find the inode number of the last inode in the group
    uint32_t last_inode= (first_inode-1) + f->superBlocks.s_inodes_per_group;

    while(first_inode <= last_inode){
        int32_t avail = inodeInUse(f,first_inode);
        if (avail !=1 ){
            iNumfinal = first_inode;
            break;
        }
        first_inode++;
    }


    //iNumfinal is the free inode
    //following code is for marking the free inode as allocated
    uint32_t iNum= iNumfinal-1;
    uint8_t * block_with_inode_bitmap = NULL;
    block_with_inode_bitmap= new uint8_t [f->superBlocks.s_log_block_size];
    //find the block group in which the given iNum  is contained
    uint8_t blockGroupNumber = iNum/f->superBlocks.s_inodes_per_group;
    //fetch the only block in the group with inode bitmap
    fetchBlock(f, f->blockGroupDescriptorstable[blockGroupNumber].bg_inode_bitmap,block_with_inode_bitmap);
    //find the byte that has the bitmap of the given inode
    uint8_t byte_Position = iNum/ 8;
    //find the bit position of the inode in that byte
    uint8_t  bit_Position = iNum % 8  ;

    //updating the byte with allocated bit
    block_with_inode_bitmap[byte_Position] |= 1UL << bit_Position ;

    //writing the block back to the vdifile
    writeBlock(f,f->blockGroupDescriptorstable[blockGroupNumber].bg_inode_bitmap,block_with_inode_bitmap);


    f->blockGroupDescriptorstable[blockGroupNumber].bg_free_inodes_count--;
    f->superBlocks.s_free_inodes_count--;
    return iNumfinal;
}

int32_t freeInode(struct Ext2File *f,uint32_t iNum){
    uint32_t e = inodeInUse(f,iNum);
    //iNum is the allocted inode
    iNum--;
    uint8_t * block_with_inode_bitmap = NULL;
    block_with_inode_bitmap= new uint8_t [f->superBlocks.s_log_block_size];
    //find the block group in which the given iNum  is contained
    uint8_t blockGroupNumber = iNum/f->superBlocks.s_inodes_per_group;
    //fetch the only block in the group with inode bitmap
    fetchBlock(f, f->blockGroupDescriptorstable[blockGroupNumber].bg_inode_bitmap,block_with_inode_bitmap);
    //find the byte that has the bitmap of the given inode
    uint8_t byte_Position = iNum/ 8;
    //find the bit position of the inode in that byte
    uint8_t  bit_Position = iNum % 8  ;
    //updating the byte with allocated bit
    block_with_inode_bitmap[byte_Position] &= ~(1UL << bit_Position);
    //writing the block back to the vdifile
    writeBlock(f,f->blockGroupDescriptorstable[blockGroupNumber].bg_inode_bitmap,block_with_inode_bitmap);
    uint32_t d = inodeInUse(f,iNum);
    return iNum;
}

void dumpInode(Inode *inode){
    cout << "Size " << inode->i_size << endl;
    cout << "Blocks " << inode->i_blocks << endl;
    cout << "Links " << inode->i_links_count << endl;
    cout << "Flags "  << inode->i_flags <<endl;
    cout << "SIB " << inode->i_block[12] << endl;
    cout << "DIB " << inode->i_block[13] << endl;
    cout << "TIB " << inode->i_block[14] << endl;
    cout << "Entry 0" << inode->i_block[0] << endl;
    cout << "Entry 1 " << inode->i_block[1] << endl;
}

//end of step 4

//start of step 5
int32_t fetchBlockFromFile(struct Ext2File *f,struct Inode *i,uint32_t bNum, void *buf){
    uint32_t k = f->superBlocks.s_log_block_size/4;
    uint32_t * blockList= NULL;
    blockList= new uint32_t [sizeof(buf)];
    uint32_t index;
    //if block is in the i_block array
    if (bNum < 12){
        blockList = i->i_block;
        if (blockList[bNum]==0){
            return false;
        }
        fetchBlock(f,blockList[bNum],buf);
        return true;
    }
    else if (bNum < 12+k){
        if (i->i_block[12]==0){
            return false;
        }
        fetchBlock(f,i->i_block[12],buf);
        blockList= static_cast<uint32_t *>(buf);
        bNum= bNum-12;
        if (blockList[bNum]==0){
            return false;
        }
        fetchBlock(f,blockList[bNum],buf);
        return true;
    }
    else if (bNum < (12+k+ (k*k)) ){
        if (i->i_block[13]==0){
            return false;
        }
        fetchBlock(f,i->i_block[13],buf);
        blockList = static_cast<uint32_t *>(buf);
        bNum= bNum-12-k;
        goto single;
    }
    else{
        if (i->i_block[14]==0){
            return false;
        }
        fetchBlock(f,i->i_block[14],buf);
        blockList = static_cast<uint32_t *>(buf);
        bNum = bNum-12-k-(k*k);

        //go to double
        index= (bNum) / (k*k);
        bNum = (bNum) % (k*k);
        if (blockList[index]==0){
            return false;
        }

        fetchBlock(f,blockList[index],buf);
        blockList= static_cast<uint32_t *>(buf);
        goto single;
    }

    single:
    index = (bNum)/k;
    bNum= (bNum) % k;
    if (blockList[index] ==0){
        return false;
    }

    fetchBlock(f,blockList[index],buf);
    blockList= static_cast<uint32_t *>(buf);

    if (blockList[bNum]==0){
        return false;
    }

    fetchBlock(f,blockList[bNum],buf);
    return true;

}


//works if you try copying within a vdifile
int32_t writeBlockToFile(struct Ext2File *f,struct Inode *i,uint32_t bNum, void *buf,uint32_t iNum){
    int groupNumber = round(iNum-1)/f->superBlocks.s_inodes_per_group;
    uint32_t  k = f->superBlocks.s_log_block_size/4;
    char tmp[f->superBlocks.s_log_block_size];
    char * blockList= NULL;
    blockList= new char [sizeof(tmp)];

    //index is in the direct array
    if (bNum < 12){
        if (i->i_block[bNum]==0){
            i->i_block[bNum]=allocateBlock(f,groupNumber);
            writeInode(f,iNum,i);
        }
        writeBlock(f,i->i_block[bNum],buf);
        return true;
    }

        //index is in the first single indirect block
    else if (bNum< 12+k){
        if (i->i_block[12]==0){
            i->i_block[12] = allocateBlock(f,groupNumber);
            writeInode(f,iNum,i);
        }

        fetchBlock(f,i->i_block[12],tmp);
        uint32_t  ibNum = i->i_block[12];
        blockList= tmp;
        bNum =bNum-12;
        //go to direct
        if (blockList[bNum]==0){
            blockList[bNum] = allocateBlock(f,groupNumber);
            writeBlock(f,ibNum, blockList);
        }
        writeBlock(f,blockList[bNum],buf);
        return true;
    }

        //index is in DIB
    else if (bNum <12+k+k*k) {
        if (i->i_block[13] == 0) {
            i->i_block[13] = allocateBlock(f, groupNumber);
            writeInode(f, iNum, i);
        }
        fetchBlock(f, i->i_block[13], tmp);
        blockList = tmp;
        bNum = bNum - 12 - k;
        uint32_t ibNum = i->i_block[13];

        //go to single
        uint32_t index = bNum/k;
        bNum = bNum % k;
        if (blockList[index]==0){
            blockList[index] = allocateBlock(f,groupNumber);
            writeBlock(f,ibNum,blockList);
        }
        ibNum= blockList[index];
        fetchBlock(f,blockList[index],tmp);
        blockList=tmp;

        if (blockList[bNum]==0){
            blockList[bNum] = allocateBlock(f,groupNumber);
            writeBlock(f,ibNum, blockList);
        }
        writeBlock(f,blockList[bNum],buf);
        return true;
    }
    else {
        if (i->i_block[14]==0){
            i->i_block[14]=allocateBlock(f,groupNumber);
            writeInode(f,iNum,i);
        }

        fetchBlock(f,i->i_block[14],tmp);

        uint32_t ibNum= i->i_block[14];
        bNum = bNum-12-k-(k*k);

        //go to dib
        uint32_t index= bNum/(k*k);
        bNum =(bNum) % (k*k);
        if (blockList[index]==0){
            blockList[index] =allocateBlock(f,groupNumber);
            writeBlock(f,ibNum,blockList);
        }
        ibNum= blockList[index];
        fetchBlock(f,blockList[index],tmp);
        blockList=tmp;

        //go to single
        index = bNum/k;
        bNum = bNum % k;
        if (blockList[index]==0){
            blockList[index] = allocateBlock(f,groupNumber);
            writeBlock(f,ibNum,blockList);
        }
        ibNum= blockList[index];
        fetchBlock(f,blockList[index],tmp);
        blockList=tmp;

        //go to direct
        if (blockList[bNum]==0){
            blockList[bNum] = allocateBlock(f,groupNumber);
            writeBlock(f,ibNum, blockList);
        }
        writeBlock(f,blockList[bNum],buf);
        return true;
    }
}

int32_t allocateBlock(struct Ext2File *f,uint32_t groupNumber){
    //find the first block in the group
    uint32_t first_block= groupNumber*f->superBlocks.s_blocks_per_group;
    //find the last block in the group
    uint32_t last_block= first_block+ f->superBlocks.s_blocks_per_group;

    uint32_t blockFinal;

    while(first_block <= last_block){
        //find the first available block in the group
        int32_t avail = blockInUse(f,groupNumber,first_block);
        if (avail !=1 ){
            blockFinal = first_block;
            break;
        }
        first_block++;
    }

    uint8_t * block_with_block_bitmap = NULL;
    block_with_block_bitmap= new uint8_t [f->superBlocks.s_log_block_size];
    //find the block group in which the given bNum  is contained
    uint8_t blockGroupNumber = blockFinal/f->superBlocks.s_blocks_per_group;
    //fetch the only block in the group with block bitmap
    fetchBlock(f, f->blockGroupDescriptorstable[blockGroupNumber].bg_block_bitmap,block_with_block_bitmap);
    //find the byte that has the bitmap of the given block
    uint8_t byte_Position = blockFinal/ 8;
    //find the bit position of the block in that byte
    uint8_t  bit_Position = blockFinal % 8  ;
    //updating the byte with allocated bit
    block_with_block_bitmap[byte_Position] |= 1UL << bit_Position ;
    //writing block back to the vdifile after marking that block as allocated
    writeBlock(f,f->blockGroupDescriptorstable[blockGroupNumber].bg_block_bitmap,block_with_block_bitmap);

    Ext2Superblock superblock;
    Ext2BlockGroupDescriptor ext2BlockGroupDescriptor;
    fetchSuperblock(f,blockFinal,&superblock);
    fetchBGDT(f,blockFinal,&ext2BlockGroupDescriptor);

    cout << "Updating the free block count in the group's superblock " <<endl;
    superblock.s_free_blocks_count--;
    cout << "Updating the free block count in the group's bgdt " <<endl;
    ext2BlockGroupDescriptor.bg_free_blocks_count--;
    cout << "Writing the group's superblock to the main superblock" <<endl;

    writeSuperblock(f,0,&superblock);
    cout << "Writing the group's bgdt to the main bgdt" <<endl;
    writeBGDT(f,0,&ext2BlockGroupDescriptor);
    cout << "Block allocated successfully" << endl;

    return blockFinal;

}

//check whether a block is in use
int32_t blockInUse (struct Ext2File *f,uint32_t blockGroupNumber,uint32_t bNum){
    uint8_t * block_with_block_bitmap = NULL;
    block_with_block_bitmap= new uint8_t [f->superBlocks.s_log_block_size];
    //fetch the only block in the group with block bitmap
    fetchBlock(f, f->blockGroupDescriptorstable[blockGroupNumber].bg_block_bitmap,block_with_block_bitmap);
    //find the byte that has the bitmap of the given inode
    uint8_t byte_Position = bNum/ 8;
    //find the bit position of the inode in that byte
    uint8_t  bit_Position = bNum % 8  ;
    return ( (block_with_block_bitmap[byte_Position] & (1 << bNum %8)) !=0 );
}

//start of step 6

//open the directory with the given iNum
struct Directory *openDir(struct  Ext2File *f, uint32_t iNum){
    Directory *directory=new Directory;
    directory->cursor=0;
    directory->iNum=iNum;
    fetchInode(f,iNum,&directory->inode);
    directory->ptr = new uint8_t[f->superBlocks.s_log_block_size];
    directory->ext2File=f;
    return directory;
}

//fetch the directory entries in the given directory
bool getNextDirent(struct Directory *d,uint32_t &iNum,char *name){
    //while we are within the directory file
    while (d->cursor < d->inode.i_size){
        //find the block in the file which contains the directory
        int bNum = d->cursor/d->ext2File->superBlocks.s_log_block_size;

        //fetch the directory
        fetchBlockFromFile(d->ext2File,&d->inode,bNum,d->ptr);

        //find the offset of the directory entry from the start of the directory
        int offset = d->cursor % d->ext2File->superBlocks.s_log_block_size;

        //point to the directory entry we want to access
        d->dirent = (Dirent *) (d->ptr+offset);

        //advance our cursor to the next directory entry
        d->cursor += d->dirent->recLen;

        //copy the name to the passed array if the inode is not zero
        if (d->dirent->iNum !=0 ){
            iNum= d->dirent->iNum;

            for (int i = 0; i < d->dirent->nameLen; i++) {
                name[i]= d->dirent->name[i];
            }

            name[d->dirent->nameLen]=0;
            return true;
        }
    }
    return false;
}

void rewindDir(struct Directory *d){
    d->cursor=0;
}

void closeDir(struct Directory *d){
    close(d->ext2File->partitionFile->f->fd);
}

//end of step 6

//start of step 7
/*search the sequence of directories for the inode number
 corresponding to the file at the end of the path*/
uint32_t searchDir(struct Ext2File *f,uint32_t iNum,char *target){
    char name[256];
    Directory *directory;
    //open the root directory
    directory= openDir(f,iNum);
    uint32_t diNum;
    //loop through the directory until the passed directory is found
    while (getNextDirent(directory,diNum,name)) {
        if (strcmp(target,name) ==0){
            return diNum;
        }
    }
    return 0;
}

uint32_t traversePath(Ext2File *f,char *path){
    uint32_t  start =1;
    uint32_t  len =strlen(path);
    uint32_t iNum = 2;
    while (start < len && iNum != 0 ){
        uint32_t end = start+1;
        while (path[end] != 0 and path[end] != '/') {
            end++;
        }

        path[end] =0;
        iNum  =searchDir(f,iNum,path+start);
        start = end+1;
    }
    return iNum;
}
//end of step 7

//start of step 8
//copy file from an ext2file system to a host system 
uint32_t copyFileToHost(Ext2File*f, char *vdiFileName, char *hostFilePath){
    cout << "Copying to the host system... "<< endl;
    char *vdiName = new char [256];
    vdiName = vdiFileName;

    uint8_t * dataBlock = NULL;
    dataBlock= new uint8_t [f->superBlocks.s_log_block_size];

    //find the inode of the file that needs to be copied out
    uint32_t iNum = traversePath(f,vdiFileName);
    Inode inode;
    fetchInode(f,iNum,&inode);

    //open the file that needs to be copied into
    int fd = open(hostFilePath,O_WRONLY|O_CREAT|O_BINARY|O_TRUNC,0666);

    //get the number of bytes we need to copy
    int bytesLeft = inode.i_size,bNum=0;


    while (bytesLeft > 0) {
        fetchBlockFromFile(f,&inode,bNum++,dataBlock);
        if (bytesLeft < f->superBlocks.s_log_block_size){
            write(fd,dataBlock,bytesLeft);
            bytesLeft -= bytesLeft;
        }
        else{
            write(fd,dataBlock,f->superBlocks.s_log_block_size);
            bytesLeft -= f->superBlocks.s_log_block_size;
        }
    }

    cout << "File has been copied to "  << hostFilePath << endl;

    close (fd);
    delete [] dataBlock;
}

void displayAllFilesInVDI (Ext2File *f, uint32_t dirNum) {
    Directory *directory;
    directory = openDir(f, dirNum);
    char name[256];
    uint32_t iNum;
    uint32_t count = 0;

    while (getNextDirent(directory, iNum, name)) {
        if (count >= 2) {
            Inode inode;
            fetchInode(f, iNum, &inode);
            cout << name << endl;
            if (inode.i_mode == 16877) {
                displayAllFilesInVDI(f,iNum);
            }
        }
        count ++;
    }
    cout << endl;
}


//end of step 8

string filepath;

//looked at this website to develop strategy for printing the file permission
//https://wiki.archlinux.org/index.php/File_permissions_and_attributes#Numeric_method
string returnPermission(string permission,uint16_t i_mode){

    int binary[100];
    int a[100];

    int i =0;

    //change imode to binary
    while (i_mode > 0) {
        binary[i] = i_mode % 2;
        i_mode= i_mode / 2;
        i++;
    }


    int count = 0;

    //reverse the binary to get the final binary
    for (int j = i - 1; j >= 0; j--) {
            a[count] = binary[j];
            count++;
    }

    //set permission for group user and everyone else
    int group [i-11];
    int user[i-10];
    int eElse[i-9];

    group[0] =a[i-9];
    group[1] = a[i-8];
    group[2] = a[i-7];

    user[0] =a[i-6];
    user[1] = a[i-5];
    user[2] = a[i-4];

    eElse[0] =a[i-3];
    eElse[1] = a[i-2];
    eElse[2] = a[i-1];


    if (group[0] ==1) permission.append("r");
    else permission.append("-");
    if (group[1] ==1) permission.append("w");
    else permission.append("-");
    if (group[2] ==1) permission.append("x");
    else permission.append("-");
    if (user[0] ==1) permission.append("r");
    else permission.append("-");
    if (user[1] ==1) permission.append("w");
    else permission.append("-");
    if (user[2] ==1) permission.append("x");
    else permission.append("-");
    if (eElse[0] ==1) permission.append("r");
    else permission.append("-");
    if (eElse[1] ==1) permission.append("w");
    else permission.append("-");
    if (eElse[2] ==1) permission.append("x");
    else permission.append("-");

    return permission;

}


//display files in an ext2file system with file info
void displayFilesWithInfo(Ext2File *f, uint32_t dirNum){
    Directory *directory;
    directory = openDir(f, dirNum);
    char name[256];
    uint32_t iNum;
    uint32_t count = 0;
    string newfilepath = filepath;

    while (getNextDirent(directory, iNum, name)) {
        if (count >= 2) {
            Inode inode;
            fetchInode(f, iNum, &inode);

            string finalPermission;

            if (directory->dirent->fileType ==2){
                finalPermission.append("d");
            }else{
                finalPermission.append("-");
            }

            finalPermission = returnPermission(finalPermission,inode.i_mode);

            //getting file path of the file
            filepath.append("/");
            filepath.append(name);

            //getting last access time in readable format
            time_t accessTime = inode.i_mtime;
            string readableTime = ctime(&accessTime);


            //printing file info
            cout << "File path : " << filepath << endl;
            cout << "File Permission : "  << finalPermission << endl;
            cout << "UID : " << inode.i_uid << endl;
            cout << "GID : " << inode.i_gid << endl;
            cout << "Links Count : " << inode.i_links_count << endl;
            cout << "File Size : " << inode.i_size << endl;
            cout << "Inode number : " << iNum << endl;
            cout << "Last Access Time : " << readableTime << endl;

            //call the function again if the given file is a directory
            if (inode.i_mode == 16877) {
                displayFilesWithInfo(f,iNum);
            }
            filepath = newfilepath;
        }
        count ++;
    }
    cout << endl;
}


