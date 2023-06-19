/*
     File        : file.C

     Author      : Riccardo Bettati
     Modified    : 2021/11/28

     Description : Implementation of simple File class, with support for
                   sequential read/write operations.
*/

/*--------------------------------------------------------------------------*/
/* DEFINES */
/*--------------------------------------------------------------------------*/

/* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* INCLUDES */
/*--------------------------------------------------------------------------*/

#include "assert.H"
#include "console.H"
#include "file.H"

/*--------------------------------------------------------------------------*/
/* CONSTRUCTOR/DESTRUCTOR */
/*--------------------------------------------------------------------------*/

File::File(FileSystem *_fs, int _id) {
    // Set information
    Console::puts("Opening file.\n");
    cur_pos = 0;
    fs = _fs;
    id = _id;
    inode = fs->LookupFile(id);
    size = inode->size;
    fs->disk->read(inode->block_id, block_ids);
}

File::~File() {
    Console::puts("Closing file.\n");
    /* Make sure that you write any cached data to disk. */
    /* Also make sure that the inode in the inode list is updated. */

    // Write the last block
    // Get a free block
    if((char)(block_ids[size/SimpleDisk::BLOCK_SIZE]) == 0){
        block_ids[size/SimpleDisk::BLOCK_SIZE] = fs->GetFreeBlock();
        fs->disk->write(inode->block_id,block_ids);
    }
    fs->disk->write(block_ids[size/SimpleDisk::BLOCK_SIZE], block_cache);
    inode->size = size;
}

/*--------------------------------------------------------------------------*/
/* FILE FUNCTIONS */
/*--------------------------------------------------------------------------*/

int File::Read(unsigned int _n, char *_buf) {
    Console::puts("reading from file\n");
    for(int i = 0; i < _n; i++){
        if(cur_pos < size){
            // Get next block
            if(cur_pos%SimpleDisk::BLOCK_SIZE == 0){
                fs->disk->read(block_ids[cur_pos/SimpleDisk::BLOCK_SIZE], block_cache); 
            }
            _buf[i] = block_cache[cur_pos%SimpleDisk::BLOCK_SIZE];
            cur_pos ++;
        }
        else{
            Reset();
            return i;
        }
    }
    return _n;
}

int File::Write(unsigned int _n, const char *_buf) {
    Console::puts("writing to file\n");
    for(int i = 0; i < _n; i++){
        if(EoF()==false){
            // If block cache is full, write back to disk
            if(cur_pos%SimpleDisk::BLOCK_SIZE == 0){
                // Get free block if it is not enough
                if((block_ids[cur_pos/SimpleDisk::BLOCK_SIZE] == 0) && (cur_pos != 0)){
                    block_ids[(cur_pos/SimpleDisk::BLOCK_SIZE)-1] = fs->GetFreeBlock();
                    fs->disk->write(inode->block_id,block_ids);
                }
                fs->disk->write(block_ids[(cur_pos/SimpleDisk::BLOCK_SIZE)-1], block_cache);
                inode->size = size;
            }
            block_cache[cur_pos%SimpleDisk::BLOCK_SIZE] = _buf[i];
            cur_pos++;
            size = cur_pos;
        }
        else{
            Reset();
            return i;
        }
    }
    return _n;
}

void File::Reset() {
    Console::puts("resetting file\n");
    if(block_ids[size/SimpleDisk::BLOCK_SIZE] == 0){
        block_ids[size/SimpleDisk::BLOCK_SIZE] = fs->GetFreeBlock();
        fs->disk->write(inode->block_id,block_ids);
    }
    fs->disk->write(block_ids[size/SimpleDisk::BLOCK_SIZE], block_cache);
    cur_pos = 0;
    inode->size = size;
}

bool File::EoF() {
    return cur_pos == (SimpleDisk::BLOCK_SIZE*128);
}
