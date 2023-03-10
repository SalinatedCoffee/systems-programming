/**
 * Ideal Indirection Lab
 * CS 241 - Spring 2019
 */
 
#include "mmu.h"
#include <assert.h>
#include <dirent.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#define PD_MASK 0xFFC00000
#define PT_MASK 0x3FF000
#define OF_MASK 0xFFF

// 32 - 12	: Base
// 6		: Dirty
// 5		: Accessed
// 2		: User / Supervisor
// 1		: Read / Write
// 0		: Present

mmu *mmu_create() {
    mmu *my_mmu = calloc(1, sizeof(mmu));
    my_mmu->tlb = tlb_create();
    return my_mmu;
}

void mmu_read_from_virtual_address(mmu *this, addr32 virtual_address,
                                   size_t pid, void *buffer, size_t num_bytes) {
    assert(this);
    assert(pid < MAX_PROCESS_ID);
    assert(num_bytes + (virtual_address % PAGE_SIZE) <= PAGE_SIZE);
	addr32 pd_base = (virtual_address & PD_MASK) >> 22;
	addr32 pt_base = (virtual_address & PT_MASK) >> 12;
	addr32 base = (virtual_address & ~OF_MASK) >> 12;
	addr32 offset = virtual_address & OF_MASK;

	vm_segmentations *proc_seg = this->segmentations[pid];
	page_directory *proc_pd = this->page_directories[pid];

	int flushed = 0;

	if(!address_in_segmentations(proc_seg, virtual_address)) {
		mmu_raise_segmentation_fault(this);
		return;
	}

	if(this->curr_pid != pid) { // different process, flush TLB for context switch
		mmu_tlb_miss(this);
		tlb_flush(&this->tlb);
		this->curr_pid = pid;
		flushed = 1;
	}

	page_table_entry *target = NULL;
	if(!(target = tlb_get_pte(&this->tlb, base))) { // tlb miss
		if(!flushed) { mmu_tlb_miss(this); } // only increment tlb miss if we didn't flush
		page_directory_entry *proc_pde = &proc_pd->entries[pd_base];
		if(!proc_pde->present) { // page table not in memory
			mmu_raise_page_fault(this);
			return;
		}
		proc_pde->accessed = true;
		page_table *proc_pt = (page_table*) get_system_pointer_from_pde(proc_pde);
		page_table_entry *proc_pte = &proc_pt->entries[pt_base];
		if(!proc_pte->present) { // physical frame not in memory
			mmu_raise_page_fault(this);
			return;
		}
		proc_pte->accessed = true;
		void *proc_pf = get_system_pointer_from_pte(proc_pte);
		proc_pf = (char*) proc_pf + offset;
		memcpy(buffer, proc_pf, num_bytes);
		tlb_add_pte(&this->tlb, base, proc_pte);
		return;
	}

	if(!target->present) {
		mmu_raise_page_fault(this);
		target->present = true;
		target->base_addr = ask_kernel_for_frame(NULL) >> 12;
		read_page_from_disk(target);
	}
	target->accessed = true;
	void *proc_pf = get_system_pointer_from_pte(target);
	proc_pf = (char*) proc_pf + offset;
	memcpy(buffer, proc_pf, num_bytes);
	return;	
}

void mmu_write_to_virtual_address(mmu *this, addr32 virtual_address, size_t pid,
                                  const void *buffer, size_t num_bytes) {
    assert(this);
    assert(pid < MAX_PROCESS_ID);
    assert(num_bytes + (virtual_address % PAGE_SIZE) <= PAGE_SIZE);
	addr32 pd_base = (virtual_address & PD_MASK) >> 22;
	addr32 pt_base = (virtual_address & PT_MASK) >> 12;
	addr32 base = (virtual_address & ~OF_MASK) >> 12;
	addr32 offset = virtual_address & OF_MASK;
	
	vm_segmentations *proc_segs = this->segmentations[pid];
	page_directory *proc_pd = this->page_directories[pid];
	vm_segmentation *proc_seg = NULL;

	int flushed = 0;

	if(!(address_in_segmentations(proc_segs, virtual_address))) {
		mmu_raise_segmentation_fault(this);
		return;
	}
	proc_seg = find_segment(proc_segs, virtual_address);
	if(!(proc_seg->permissions & 0x2)) { // no write permission
		mmu_raise_segmentation_fault(this);
		return;
	}

	if(this->curr_pid != pid) { // different process, flush TLB for context switch
		flushed = 1;
		mmu_tlb_miss(this);
		tlb_flush(&this->tlb);
		this->curr_pid = pid;
	}

	page_table_entry *target = NULL;
	if(!(target = tlb_get_pte(&this->tlb, base))) { // tlb miss
		if(!flushed) { mmu_tlb_miss(this); }
		page_directory_entry *proc_pde = &proc_pd->entries[pd_base];
		if(!proc_pde->present) { // writable page table not in memory
			mmu_raise_page_fault(this);
			proc_pde->base_addr = ask_kernel_for_frame(NULL) >> 12;
			proc_pde->read_write = true;
			proc_pde->accessed = true;
			proc_pde->present = true;
		}
		proc_pde->accessed = true;
		page_table *proc_pt = (page_table*) get_system_pointer_from_pde(proc_pde);
		page_table_entry *proc_pte = &proc_pt->entries[pt_base];
		if(!proc_pte->present) { // physical frame not in memory
			mmu_raise_page_fault(this);
			proc_pte->base_addr = ask_kernel_for_frame(NULL) >> 12;
			proc_pte->read_write = true;
			proc_pte->accessed = true;
			proc_pte->present = true;
			read_page_from_disk(proc_pte);
		}
		proc_pte->accessed = true;
		void *proc_pf = get_system_pointer_from_pte(proc_pte);
		proc_pf = (char*) proc_pf + offset;
		memcpy(proc_pf, buffer, num_bytes);
		tlb_add_pte(&this->tlb, base, proc_pte);
		return;
	}
	target->accessed = true;
	void *proc_pf = get_system_pointer_from_pte(target);
	proc_pf = (char*) proc_pf + offset;
	memcpy(proc_pf, buffer, num_bytes);
}

void mmu_tlb_miss(mmu *this) {
    this->num_tlb_misses++;
}

void mmu_raise_page_fault(mmu *this) {
    this->num_page_faults++;
}

void mmu_raise_segmentation_fault(mmu *this) {
    this->num_segmentation_faults++;
}

void mmu_add_process(mmu *this, size_t pid) {
    assert(pid < MAX_PROCESS_ID);
    addr32 page_directory_address = ask_kernel_for_frame(NULL);
    this->page_directories[pid] =
        (page_directory *)get_system_pointer_from_address(
            page_directory_address);
    page_directory *pd = this->page_directories[pid];
    this->segmentations[pid] = calloc(1, sizeof(vm_segmentations));
    vm_segmentations *segmentations = this->segmentations[pid];

    // Note you can see this information in a memory map by using
    // cat /proc/self/maps
    segmentations->segments[STACK] =
        (vm_segmentation){.start = 0xBFFFE000,
                          .end = 0xC07FE000, // 8mb stack
                          .permissions = READ | WRITE,
                          .grows_down = true};

    segmentations->segments[MMAP] =
        (vm_segmentation){.start = 0xC07FE000,
                          .end = 0xC07FE000,
                          // making this writeable to simplify the next lab.
                          // todo make this not writeable by default
                          .permissions = READ | EXEC | WRITE,
                          .grows_down = true};

    segmentations->segments[HEAP] =
        (vm_segmentation){.start = 0x08072000,
                          .end = 0x08072000,
                          .permissions = READ | WRITE,
                          .grows_down = false};

    segmentations->segments[BSS] =
        (vm_segmentation){.start = 0x0805A000,
                          .end = 0x08072000,
                          .permissions = READ | WRITE,
                          .grows_down = false};

    segmentations->segments[DATA] =
        (vm_segmentation){.start = 0x08052000,
                          .end = 0x0805A000,
                          .permissions = READ | WRITE,
                          .grows_down = false};

    segmentations->segments[TEXT] =
        (vm_segmentation){.start = 0x08048000,
                          .end = 0x08052000,
                          .permissions = READ | EXEC,
                          .grows_down = false};

    // creating a few mappings so we have something to play with (made up)
    // this segment is made up for testing purposes
    segmentations->segments[TESTING] =
        (vm_segmentation){.start = PAGE_SIZE,
                          .end = 3 * PAGE_SIZE,
                          .permissions = READ | WRITE,
                          .grows_down = false};
    // first 4 mb is bookkept by the first page directory entry
    page_directory_entry *pde = &(pd->entries[0]);
    // assigning it a page table and some basic permissions
    pde->base_addr = (ask_kernel_for_frame(NULL) >> NUM_OFFSET_BITS);
    pde->present = true;
    pde->read_write = true;
    pde->user_supervisor = true;

    // setting entries 1 and 2 (since each entry points to a 4kb page)
    // of the page table to point to our 8kb of testing memory defined earlier
    for (int i = 1; i < 3; i++) {
        page_table *pt = (page_table *)get_system_pointer_from_pde(pde);
        page_table_entry *pte = &(pt->entries[i]);
        pte->base_addr = (ask_kernel_for_frame(pte) >> NUM_OFFSET_BITS);
        pte->present = true;
        pte->read_write = true;
        pte->user_supervisor = true;
    }
}

void mmu_remove_process(mmu *this, size_t pid) {
    assert(pid < MAX_PROCESS_ID);
    // example of how to BFS through page table tree for those to read code.
    page_directory *pd = this->page_directories[pid];
    if (pd) {
        for (size_t vpn1 = 0; vpn1 < NUM_ENTRIES; vpn1++) {
            page_directory_entry *pde = &(pd->entries[vpn1]);
            if (pde->present) {
                page_table *pt = (page_table *)get_system_pointer_from_pde(pde);
                for (size_t vpn2 = 0; vpn2 < NUM_ENTRIES; vpn2++) {
                    page_table_entry *pte = &(pt->entries[vpn2]);
                    if (pte->present) {
                        void *frame = (void *)get_system_pointer_from_pte(pte);
                        return_frame_to_kernel(frame);
                    }
                    remove_swap_file(pte);
                }
                return_frame_to_kernel(pt);
            }
        }
        return_frame_to_kernel(pd);
    }

    this->page_directories[pid] = NULL;
    free(this->segmentations[pid]);
    this->segmentations[pid] = NULL;

    if (this->curr_pid == pid) {
        tlb_flush(&(this->tlb));
    }
}

void mmu_delete(mmu *this) {
    for (size_t pid = 0; pid < MAX_PROCESS_ID; pid++) {
        mmu_remove_process(this, pid);
    }

    tlb_delete(this->tlb);
    free(this);
    remove_swap_files();
}
