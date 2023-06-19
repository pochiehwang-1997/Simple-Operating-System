/*
 File: vm_pool.C
 
 Author:
 Date  :
 
 */

/*--------------------------------------------------------------------------*/
/* DEFINES */
/*--------------------------------------------------------------------------*/

/* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* INCLUDES */
/*--------------------------------------------------------------------------*/

#include "vm_pool.H"
#include "console.H"
#include "utils.H"
#include "assert.H"
#include "simple_keyboard.H"

/*--------------------------------------------------------------------------*/
/* DATA STRUCTURES */
/*--------------------------------------------------------------------------*/

/* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* CONSTANTS */
/*--------------------------------------------------------------------------*/

/* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* FORWARDS */
/*--------------------------------------------------------------------------*/

/* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* METHODS FOR CLASS   V M P o o l */
/*--------------------------------------------------------------------------*/

VMPool::VMPool(unsigned long  _base_address,
               unsigned long  _size,
               ContFramePool *_frame_pool,
               PageTable     *_page_table) {
    base_address = _base_address;
    size = _size;
    frame_pool = _frame_pool;
    page_table = _page_table;
    page_table->register_pool(this);
    allocated_region_array = (AllocatedRegion *) _base_address;
    region_number = 0;
}

unsigned long VMPool::allocate(unsigned long _size) {
    int frames = (_size / Machine::PAGE_SIZE);
    if((_size % Machine::PAGE_SIZE) != 0){
        frames += 1;
    }

    int num = region_number;
    if(region_number == 0){
        allocated_region_array[0].base_address = base_address + Machine::PAGE_SIZE;
        allocated_region_array[0].size = (unsigned long) (frames * Machine::PAGE_SIZE);
    } else {
        for(int i = 1; i < region_number; i++){
            if(allocated_region_array[i].base_address - (allocated_region_array[i-1].base_address + allocated_region_array[i-1].size) > frames * Machine::PAGE_SIZE){
                num = i;
                break;
            }
        }
        if(num == region_number){
            if((base_address + size) < (allocated_region_array[num].base_address + allocated_region_array[num].size)){ // Check if it is valid or not, if it is invalid, return 0
                return 0;
            }
        }else {
            for(int i = region_number; i > num; i--){ // Update the allocated region array
                allocated_region_array[i] = allocated_region_array[i-1];
            }
        }
        allocated_region_array[num].base_address = allocated_region_array[num-1].base_address + allocated_region_array[num-1].size;
        allocated_region_array[num].size = (unsigned long) (frames * Machine::PAGE_SIZE);
        
    }
    region_number ++;
    
    return allocated_region_array[num].base_address;
}

void VMPool::release(unsigned long _start_address) {
    // Find the region number
    
    int num = -1;
    for(int i = 0; i < region_number; i++){
        if(allocated_region_array[i].base_address == _start_address){
            num = i;
            break;
        }
    }
    if(num == -1){
        return;
    }

    // Free pages
    for(int i = 0; i < allocated_region_array[num].size/Machine::PAGE_SIZE; i++){
        page_table->free_page(allocated_region_array[num].base_address + (i * Machine::PAGE_SIZE));
    }

    // Update allocated region array
    for(int i = num; i < region_number - 1; i++){
        allocated_region_array[i] = allocated_region_array[i+1];
    }
    region_number --;
}

bool VMPool::is_legitimate(unsigned long _address) {
    if((_address >= int(base_address)) && (_address < int(base_address + size))){
        return true;
    } else {
        return false;
    }
}

