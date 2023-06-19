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



void PageTable::init_paging(ContFramePool * _kernel_mem_pool,
                            ContFramePool * _process_mem_pool,
                            const unsigned long _shared_size)
{
   kernel_mem_pool = _kernel_mem_pool;
   process_mem_pool = _process_mem_pool;
   shared_size = _shared_size;
   // assert(false);
   // Console::puts("Initialized Paging System\n");
}

PageTable::PageTable()
{
   page_directory = (unsigned long *) (kernel_mem_pool->get_frames(1)*PAGE_SIZE);
   unsigned long * page_table = (unsigned long *) (kernel_mem_pool->get_frames(1)*PAGE_SIZE);
   unsigned long address = 0;
   unsigned int i;
   for (i = 0; i < 1024; i++)
   {
      page_table[i] = address | 3;
      address += PAGE_SIZE;
   }
   page_directory[0] = (unsigned long) page_table;
   page_directory[0] = page_directory[0] | 3;

   for(i = 1; i < 1024; i++)
   {
      page_directory[i] = 0 | 2;
   }

   paging_enabled = 0;
   // assert(false);
   // Console::puts("Constructed Page Table object\n");
}


void PageTable::load()
{
   current_page_table = this;
   write_cr3((unsigned long) current_page_table->page_directory);
   
   // assert(false);
   // Console::puts("Loaded page table\n");
}

void PageTable::enable_paging()
{
   write_cr0(read_cr0() | 0x80000000);
   paging_enabled = 1;
   // assert(false);
   // Console::puts("Enabled paging\n");
}

void PageTable::handle_fault(REGS * _r)
{
   unsigned long faulty_address = read_cr2();
   unsigned long faulty_address_dir = faulty_address >> 22;
   unsigned long faulty_address_pte = (faulty_address >> 12) & 1023;

   if ((current_page_table->page_directory[faulty_address_dir] & 1) == 0)
   {
      unsigned long * new_page_table = (unsigned long *) (kernel_mem_pool->get_frames(1)*PAGE_SIZE);
      current_page_table->page_directory[faulty_address_dir] = (unsigned long) new_page_table;
      current_page_table->page_directory[faulty_address_dir] |= 3;
      for (int i = 0; i < 1024; i++)
      {
         new_page_table[i] = 2;
      }
   } else
   {
      unsigned long * page_table = (unsigned long *) (current_page_table->page_directory[faulty_address_dir] >> 12 << 12);
      page_table[faulty_address_pte] = (process_mem_pool->get_frames(1)*PAGE_SIZE);
      page_table[faulty_address_pte] |= 3;
   }

}

