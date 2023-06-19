#include "assert.H"
#include "exceptions.H"
#include "console.H"
#include "paging_low.H"
#include "page_table.H"

PageTable * PageTable::current_page_table = NULL;
unsigned int PageTable::paging_enabled = 0;
ContFramePool * PageTable::kernel_mem_pool = NULL;
ContFramePool * PageTable::process_mem_pool = NULL;
unsigned long PageTable::shared_size = 0;
VMPool * PageTable::vm_pool_head = NULL;


void PageTable::init_paging(ContFramePool * _kernel_mem_pool,
                            ContFramePool * _process_mem_pool,
                            const unsigned long _shared_size)
{
    kernel_mem_pool = _kernel_mem_pool;
    process_mem_pool = _process_mem_pool;
    shared_size = _shared_size;
}

PageTable::PageTable()
{
    page_directory = (unsigned long *) (process_mem_pool->get_frames(1)*PAGE_SIZE);
    unsigned long * first_page_table = (unsigned long *) (process_mem_pool->get_frames(1)*PAGE_SIZE);
    unsigned long address = 0;

    for (int i = 0; i < 1024; i++)
    {
        first_page_table[i] = address | 3;
        address += PAGE_SIZE;
    }
    page_directory[0] = (unsigned long) first_page_table;
    page_directory[0] = page_directory[0] | 3;

    for(int i = 1; i < 1024; i++)
    {
        if(i == 1023){
            page_directory[i] = (unsigned long) page_directory | 3;
        } else {
            page_directory[i] = 0 | 2; 
        }
    }

    paging_enabled = 0;
}


void PageTable::load()
{
    current_page_table = this;
    write_cr3((unsigned long) current_page_table->page_directory);
}

void PageTable::enable_paging()
{
    write_cr0(read_cr0() | 0x80000000);
    paging_enabled = 1;
}

void PageTable::handle_fault(REGS * _r)
{
    unsigned long faulty_address = read_cr2();
    unsigned long faulty_address_dir = faulty_address >> 22;
    unsigned long faulty_address_pte = (faulty_address >> 12) & 1023;
    unsigned long * PDE_address = (unsigned long *) 0xFFFFF000; // Recursive page directory lookup
    unsigned long * PTE_address =(unsigned long *) (0xFFC00000 | (faulty_address_dir << 12)); // Recursive page table lookup
    VMPool * cur_vm_pool = vm_pool_head;
    int is_valid = 0;

    while (cur_vm_pool != NULL){
        if(cur_vm_pool->is_legitimate(faulty_address) == true){
            is_valid = 1;
            break;
        }
        cur_vm_pool = cur_vm_pool->next;
    }
    if(is_valid == 0 && cur_vm_pool != NULL){
        assert(false);
    }

    if ((PDE_address[faulty_address_dir] & 1) == 0) // Page fault in PDE
    {
        unsigned long * new_page_table_physical_address = (unsigned long *) (process_mem_pool->get_frames(1)*PAGE_SIZE);
        PDE_address[faulty_address_dir] = (unsigned long) new_page_table_physical_address | 3;
        for (int i = 0; i < 1024; i++)
        {
                PTE_address[i] = 2; // Supervisor, write and not present
        }
    } else // Page fault in PTE
    {
        PTE_address[faulty_address_pte] = (process_mem_pool->get_frames(1)*PAGE_SIZE) | 3;
    }
}

void PageTable::register_pool(VMPool * _vm_pool)
{
    if(vm_pool_head == NULL){
        vm_pool_head = _vm_pool;
    } else {
        VMPool * temp = vm_pool_head;
        while(temp->next != NULL){
            temp = temp -> next;
        }
        temp -> next = _vm_pool;
    }
    Console::puts("Register VM pool successfully!\n");
}

void PageTable::free_page(unsigned long _page_no) {
    unsigned long released_pde_addr = _page_no >> 22;
    unsigned long released_pte_addr = (_page_no << 10) >> 22;
    unsigned long * PTE_address = (unsigned long *) (0xFFC00000 | (released_pde_addr << 12));
    if((PTE_address[released_pte_addr] & 1) == 1){
        process_mem_pool->release_frames(PTE_address[released_pte_addr] / PAGE_SIZE);
        PTE_address[released_pte_addr] = 2;
        load();
    }
}
