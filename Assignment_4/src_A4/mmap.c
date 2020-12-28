#include<types.h>
#include<mmap.h>
#define NORMAL_PAGE_SIZE 4096

// #define HUGE_PAGE_SIZE (1<<21)
// #define PGD_SHIFT 39
// #define PUD_SHIFT 30
// #define PMD_SHIFT 21
// #define PTE_SHIFT 12

// #define PGD_MASK 0xff8000000000UL 
// #define PUD_MASK 0x7fc0000000UL
// #define PMD_MASK 0x3fe00000UL
// #define PTE_MASK 0x1ff000UL

// Helper function to create a new vm_area
struct vm_area* create_vm_area(u64 start_addr, u64 end_addr, u32 flags, u32 mapping_type)
{
	struct vm_area *new_vm_area = alloc_vm_area();
	new_vm_area-> vm_start = start_addr;
	new_vm_area-> vm_end = end_addr;
	new_vm_area-> access_flags = flags;
	new_vm_area->mapping_type = mapping_type;
	return new_vm_area;
}

// Helper function to flush TLB
void flush_tlb(u64 addr){
	asm volatile (
		"invlpg (%0);" 
		:: "r"(addr) 
		: "memory"
	); 
}

// ***************************************************** END OF PREDEFINED HELPER FUNCTIONS **********************************************************************************

/**
 * This function merges the vma based on the merge conditions given in the pdf
*/
void merge_vmas_based_on_type(struct exec_context *current){
	struct vm_area* cur = current->vm_area;
	struct vm_area* prev = NULL;
	while (cur){
		if(prev && prev->vm_end == cur->vm_start && prev->access_flags == cur->access_flags && prev->mapping_type == cur->mapping_type){
			// will merge to left
			prev->vm_next = cur->vm_next;
			prev->vm_end = cur->vm_end;
			struct vm_area* cur_next = cur->vm_next;
			dealloc_vm_area(cur);
			cur = cur_next;
			continue;
		}
		prev = cur;
		cur = cur->vm_next;
	}
}


// *********************************************************** PAGEFAULT IMPLEMENTATION **************************************************************

int addr_in_vm(struct exec_context *current, u64 addr){
	struct vm_area* cur = current->vm_area;
	while(cur){
		if(addr < cur->vm_end && addr >=cur->vm_start){
			return cur->mapping_type;
		}
		cur = cur->vm_next;
	}
	return 0;
}


void install_pt(struct exec_context *ctx, u64 addr, u64 error_code, int map_type) {
	// get base addr of pgdir
	u64 *vaddr_base = (u64 *)osmap(ctx->pgd);
	u64 *entry;
	u64 pfn;
	// set User and Present flags
	// set Write flag if specified in error_code
	u64 ac_flags = 0x5 | (error_code & 0x2);
	
	// find the entry in page directory
	entry = vaddr_base + ((addr & PGD_MASK) >> PGD_SHIFT);
	if(*entry & 0x1) {
		// PGD->PUD Present, access it
		pfn = (*entry >> PTE_SHIFT) & 0xFFFFFFFF;
		vaddr_base = (u64 *)osmap(pfn);
	}else{
		// allocate PUD
		pfn = os_pfn_alloc(OS_PT_REG);
		*entry = (pfn << PTE_SHIFT) | ac_flags;
		vaddr_base = osmap(pfn);
	}

	entry = vaddr_base + ((addr & PUD_MASK) >> PUD_SHIFT);
	if(*entry & 0x1) {
		// PUD->PMD Present, access it
		pfn = (*entry >> PTE_SHIFT) & 0xFFFFFFFF;
		vaddr_base = (u64 *)osmap(pfn);
	}else{
		// allocate PMD
		pfn = os_pfn_alloc(OS_PT_REG);
		*entry = (pfn << PTE_SHIFT) | ac_flags;
		vaddr_base = osmap(pfn);
	}

	entry = vaddr_base + ((addr & PMD_MASK) >> PMD_SHIFT); //pmd_entry
	if(*entry & 0x1) {
		// PMD->PTE Present, access it
		pfn = (*entry >> PTE_SHIFT) & 0xFFFFFFFF;
		vaddr_base = (u64 *)osmap(pfn);
	}else{
		// allocate PLD 
		if(map_type == HUGE_PAGE_MAPPING){
			void *os_page_vaddr = os_hugepage_alloc();
			pfn = get_hugepage_pfn(os_page_vaddr);
			*entry = (pfn << PMD_SHIFT) | ac_flags;
			*entry = *entry | (1<<7);
			// printk("huge-page in install pt= %x\n", *entry);
			return;
		}else{
			pfn = os_pfn_alloc(OS_PT_REG);
			*entry = (pfn << PTE_SHIFT) | ac_flags;
			vaddr_base = osmap(pfn);
			// printk("normal-page in install pt = %x\n", *entry);
		}
	}

	entry = vaddr_base + ((addr & PTE_MASK) >> PTE_SHIFT);
	// since this fault occured as frame was not present, we don't need present check here
	pfn = os_pfn_alloc(USER_REG);
	*entry = (pfn << PTE_SHIFT) | ac_flags;
	*entry = *entry | 0x1;
}


/**
 * Function will invoked whenever there is page fault. (Lazy allocation)
 * 
 * For valid access. Map the physical page 
 * Return 1
 * 
 * For invalid access, i.e Access which is not matching the vm_area access rights (Writing on ReadOnly pages)
 * return -1. 
 */
int vm_area_pagefault(struct exec_context *current, u64 addr, int error_code)
{
	int map_type = addr_in_vm(current, addr);
	if(!map_type) return  -1;
	if(error_code == 0x4 || error_code  == 0x6){
		install_pt(current, addr, error_code,map_type);
		return 1;
	}
	return -1;
}


// ****************************************************** MMAP IMPLEMENTATION ***********************************************************************
/** 
 * This helper function takes the addr and tries to allocate the given memory at this addr. 
 * It returns the addr if the vma is allocated
 * else it return -1
 */	
u64 vm_area_map_at_addr(u64 addr, struct exec_context *current, u64 mm_area_size, u32 prot){
	
	if(addr % NORMAL_PAGE_SIZE) return EINVAL;

	// check if address can be allocated or not
	struct vm_area* cur = current->vm_area;
	struct vm_area* prev = NULL; 
	while(cur){
		if(addr < cur->vm_end && addr >= cur->vm_start){
			return -1;
		}
		if( prev && addr >= prev->vm_end && addr + mm_area_size <= cur->vm_start){
				// will not merge anywhere, thus create a new node and insert it here
				struct vm_area* new_vma = create_vm_area(addr, addr + mm_area_size, prot, NORMAL_PAGE_MAPPING); 
				new_vma->vm_next = cur;
				prev->vm_next = new_vma;
				merge_vmas_based_on_type(current);
				return addr;
		}
		prev = cur;
		cur = cur->vm_next;
	}
	// the address lies outside the last vma
	if(addr + mm_area_size <= MMAP_AREA_END){
			struct vm_area* new_vma = create_vm_area(addr, addr + mm_area_size, prot, NORMAL_PAGE_MAPPING);
			new_vma -> vm_next = NULL;
			prev->vm_next = new_vma;
			merge_vmas_based_on_type(current); 
			return addr;
	}else{
		return -1;
	}
}

/** 
 * This helper function doesn't take the addr but tries to allocate the given memory anywhere. 
 * It returns the addr if the vma is allocated
 * else it return -1
 */	
u64 vm_area_map_anywhere(struct exec_context *current, u64 mm_area_size, u32 prot){
	struct vm_area* cur = current->vm_area;
	struct vm_area* prev = NULL;
	while (cur){
		if(prev && prev->vm_end+mm_area_size <= cur->vm_start){
			// gap found
			if(prev->access_flags == prot && prev->mapping_type == NORMAL_PAGE_MAPPING){
				// will merge to left

				// check if merge to right also has to be done
				if( prev->vm_end + mm_area_size == cur->vm_start && prot == cur->access_flags && cur->mapping_type == NORMAL_PAGE_MAPPING){
					// will merge to right also
					u64 ret_addr = prev->vm_end;
					prev->vm_next = cur->vm_next;
					prev->vm_end = cur->vm_end;
					dealloc_vm_area(cur);
					return ret_addr;
				}else{
					// will not merge to right, so just merge it to left
					u64 ret_addr = prev->vm_end;
					prev->vm_end = prev->vm_end + mm_area_size;
					return ret_addr;
				}
			}else if(prev->vm_end + mm_area_size == cur->vm_start && prot == cur->access_flags && cur->mapping_type == NORMAL_PAGE_MAPPING){
				// will merge to right
				cur->vm_start = prev->vm_end;
				// printk("will merge to right\n node created = [%x , %x]\n",cur->vm_start, cur->vm_end);
				return prev->vm_end;
			}else{
				// will not merge anywhere, thus create a new node and insert it here
				struct vm_area* new_vma = create_vm_area(prev->vm_end, prev->vm_end + mm_area_size, prot, NORMAL_PAGE_MAPPING); 
				new_vma->vm_next = cur;
				prev->vm_next = new_vma;
				return new_vma->vm_start;
			}
		}
		prev = cur;
		cur = cur->vm_next;
	}

	// if no gap found then check if the vma can be inserted at the last
	if(prev->vm_end+ mm_area_size <= MMAP_AREA_END){
		if(prev->access_flags == prot && prev->mapping_type == NORMAL_PAGE_MAPPING){
			// merge with last vma
			u64 ret_addr = prev->vm_end;
			prev->vm_end += mm_area_size;
			return ret_addr;
		}else{
			// no possibility of merge, thus create a new vma and insert it in the list
			u64 ret_addr = prev->vm_end;
			struct vm_area* new_vma = create_vm_area(ret_addr, ret_addr + mm_area_size, prot, NORMAL_PAGE_MAPPING);
			new_vma -> vm_next = NULL;
			prev->vm_next = new_vma; 
			return ret_addr;
		}
	}else{
		return -1;
	}
}


/**
 * mmap system call implementation.
 */
// vma = [vm_start, vm_end) => size = vm_end - vm_start
long vm_area_map(struct exec_context *current, u64 addr, int length, int prot, int flags)
{
	if(!current || length <=0) return -1;
	if(flags == MAP_FIXED && !addr) return -1;
	if(!(prot & PROT_READ) && !(prot & PROT_WRITE)) return -1;
	
	// Calculate the area to be allocated in multiple of pages
	u64 count_normal_pages = ((u64)length / (u64)NORMAL_PAGE_SIZE);
	if(length % NORMAL_PAGE_SIZE) count_normal_pages++;
	u64 mm_area_size = count_normal_pages * NORMAL_PAGE_SIZE;

	if(current -> vm_area == NULL){
		// Currently vm_area is empty, thus create a dummy node
		current->vm_area = create_vm_area(MMAP_AREA_START, NORMAL_PAGE_SIZE + MMAP_AREA_START, 0x4, NORMAL_PAGE_MAPPING);
		current->vm_area->vm_next = NULL;
	}	
	
	if(flags == MAP_FIXED){
		return vm_area_map_at_addr(addr, current, mm_area_size, prot);
	}else{
		if(addr == 0){
			// no hint address
			u64 alloc_addr = vm_area_map_anywhere(current, mm_area_size, prot);
			return alloc_addr;
		}else{
			// hint address

			// align the address
			u64 alg_addr = (addr % NORMAL_PAGE_SIZE == 0) ? addr: addr + NORMAL_PAGE_SIZE - (addr % NORMAL_PAGE_SIZE);  
			
			// check if the vma can be allocated at that address
			u64 alloc_addr = vm_area_map_at_addr(alg_addr, current, mm_area_size, prot);
			if(alloc_addr != -1 && alloc_addr != EINVAL){
				return alloc_addr;
			}else{
				return vm_area_map_anywhere(current, mm_area_size, prot);
			}
		}
	}
	return 0;
}



// *************************************************** MUNMAP IMPLEMENTATION **************************************************************************

void remove_pma_normal(struct exec_context *current, struct vm_area* node){
	u64 st_addr = node->vm_start;
	u64 end_addr = node->vm_end;
	// u64 page_count = (end_addr - st_addr) % NORMAL_PAGE_SIZE;
	for(u64 offset = 0; offset < end_addr; offset+= NORMAL_PAGE_SIZE){
		u64 addr = st_addr + offset;

		u64 *pud_vaddr_base;u64 *pmd_vaddr_base;u64 *pte_vaddr_base; u64 *pfn_vaddr_base;
		u64 *pgd_entry;u64 *pud_entry;u64 *pmd_entry;u64 *pte_entry; u64* entry;
		u64 pud_pfn;u64 pmd_pfn;u64 pte_pfn;u64 pfn;

		u64 *pgd_vaddr_base = (u64 *)osmap(current->pgd);
		pgd_entry = pgd_vaddr_base + ((addr & PGD_MASK) >> PGD_SHIFT);
		if(*pgd_entry & 0x1) {
			// PGD->PUD Present, access it
			pud_pfn = (*pgd_entry >> PTE_SHIFT) & 0xFFFFFFFF;
			pud_vaddr_base = (u64 *)osmap(pud_pfn);
		}else{
			continue;
		}

		pud_entry = pud_vaddr_base + ((addr & PUD_MASK) >> PUD_SHIFT);
		if(*pud_entry & 0x1) {
			// PUD->PMD Present, access it
			pmd_pfn = (*pud_entry >> PTE_SHIFT) & 0xFFFFFFFF;
			pmd_vaddr_base = (u64 *)osmap(pmd_pfn);
		}else{
			continue;
		}

		pmd_entry = pmd_vaddr_base + ((addr & PMD_MASK) >> PMD_SHIFT);
		if(*pmd_entry & 0x1) {
			// PMD->PTE Present, access it
			pte_pfn = (*pmd_entry >> PTE_SHIFT) & 0xFFFFFFFF;
			pte_vaddr_base = (u64 *)osmap(pte_pfn);
		}else{
			// allocate PLD 
			continue;
		}

		pte_entry = pte_vaddr_base + ((addr & PTE_MASK) >> PTE_SHIFT);
		if(*pte_entry & 0x1) {
			// PTE->PFN Present, access it
			pfn = (*pte_entry >> PTE_SHIFT) & 0xFFFFFFFF;
			// vaddr_base = (u64 *)osmap(pfn); 
			// remove the pfn
			os_pfn_free(USER_REG, pfn);
			*pte_entry = *pte_entry & 0x0; // not present now
			flush_tlb(addr);
			int pte = 0;
			for(u64* ptei= pte_vaddr_base; ptei < pte_vaddr_base + (1<<9); ptei++){
				if(*ptei & 0x1){
					pte = 1; // PTE will not be deleted
					break; 
				}
			} 
			if(!pte){
				//PTE will be deleted
				//find pmd
				os_pfn_free(OS_PT_REG,pte_pfn);
				*pmd_entry = *pmd_entry & 0x0;
				//delete pte page and put its present bit as 0 in pmd
				
				int pmd = 0;
				for(u64* pmdi= pmd_vaddr_base; pmdi < pmd_vaddr_base + (1<<9); pmdi++){
					if(*pmdi & 0x1){
						pmd = 1; // PTE will not be deleted
						break; 
					}
				}
				// check pmd now
				if(!pmd){
					os_pfn_free(OS_PT_REG, pmd_pfn);
					*pud_entry = *pud_entry & 0x0;
					
					int pud = 0;
					for(u64* pudi= pud_vaddr_base; pudi < pud_vaddr_base + (1<<9); pudi++){
						if(*pudi & 0x1){
							pud = 1; // PTE will not be deleted
							break; 
						}
					}

					if(!pud){
						os_pfn_free(OS_PT_REG, pud_pfn);
						*pgd_entry = *pgd_entry & 0x0;
						// now no checking as pgd can never be empty as dummy node is always present	
					}	
				}
			}
		}else{
			continue;
		}

	}
}


void remove_pma_huge(struct exec_context *current, struct vm_area* node){
	u64 st_addr = node->vm_start;
	u64 end_addr = node->vm_end;
	// u64 page_count = (end_addr - st_addr) % HUGE_PAGE_SIZE;
	for(u64 offset = 0; offset < end_addr; offset += HUGE_PAGE_SIZE){
		u64 addr = st_addr + offset;
		u64 *pgd_vaddr_base = (u64 *)osmap(current->pgd);
		u64 *huge_vaddr_base; u64 *pud_vaddr_base;u64 *pmd_vaddr_base;
		u64 *entry;u64 *pgd_entry; u64 *pud_entry;u64 *pmd_entry;
		u64 huge_pfn; u64 pud_pfn; u64 pmd_pfn;
		u64 ac_flags = 0x5 | (node->access_flags & 0x2);

		pgd_entry = pgd_vaddr_base + ((addr & PGD_MASK) >> PGD_SHIFT);
		if(*pgd_entry & 0x1) {
			// PGD->PUD Present, access it
			pud_pfn = (*pgd_entry >> PTE_SHIFT) & 0xFFFFFFFF;
			pud_vaddr_base = (u64 *)osmap(pud_pfn);
		}else{
			continue;
		}

		pud_entry = pud_vaddr_base + ((addr & PUD_MASK) >> PUD_SHIFT);
		if(*pud_entry & 0x1) {
			// PUD->PMD Present, access it
			pmd_pfn = (*pud_entry >> PTE_SHIFT) & 0xFFFFFFFF;
			pmd_vaddr_base = (u64 *)osmap(pmd_pfn);
		}else{
			continue;
		}

		pmd_entry = pmd_vaddr_base + ((addr & PMD_MASK) >> PMD_SHIFT);
		if(*pmd_entry & 0x1) {
			// PTE->PFN Present, access it
			huge_pfn = (*pmd_entry >> PMD_SHIFT) & 0xFFFFFFFF;
			huge_vaddr_base = (u64 *)(huge_pfn*HUGE_PAGE_SIZE); 
			// remove the pfn
			// os_pfn_free(USER_REG, pfn);
			os_hugepage_free((void*)huge_vaddr_base);
			flush_tlb(addr);
			*pmd_entry = *pmd_entry & 0x0; // not present now			
			//delete pte page and put its present bit as 0 in pmd
			int pmd = 0;
			for(u64* pmdi= pmd_vaddr_base; pmdi < pmd_vaddr_base + (1<<9); pmdi++){
				if(*pmdi & 0x1){
					pmd = 1; // PTE will not be deleted
					break; 
				}
			}
			// check pmd now
			if(!pmd){
				*pud_entry = *pud_entry & 0x0;
				os_pfn_free(OS_PT_REG, pmd_pfn);
				int pud = 0;
				for(u64* pudi= pud_vaddr_base; pudi < pud_vaddr_base + (1<<9); pudi++){
					if(*pudi & 0x1){
						pud = 1; // PTE will not be deleted
						break; 
					}
				}
				if(!pud){
					*pgd_entry = *pgd_entry & 0x0;
					os_pfn_free(OS_PT_REG, pud_pfn);
					// now no checking as pgd can never be empty as dummy node is always present	
				}	
			}
			
		}else{
			// allocate PLD 
			continue;
		}
	}
}

/**
 * munmap system call implemenations
 * return 0 on success and -1 on failure
 */
int vm_area_unmap(struct exec_context *current, u64 addr, int length)
{
	if(addr % NORMAL_PAGE_SIZE || length <= 0) return -1;
	// align the length according to the pages considering hugepages and normalpages both
	// This will be done using travesing the whole vma list and checking the type of vma
	// Normal Page alignment of length
	u64 alg_len = ((u64)length % NORMAL_PAGE_SIZE == 0) ? (u64)length: (u64)length + NORMAL_PAGE_SIZE - ((u64)length % NORMAL_PAGE_SIZE);
	
	u64 st_addr = addr;
	u64 end_addr = addr + alg_len;
	
	// Huge page alignment of length
	struct vm_area* cur = current -> vm_area;
	while(cur){
		if(end_addr > cur -> vm_start && end_addr <= cur->vm_end){
			if(cur->mapping_type == HUGE_PAGE_MAPPING){
				alg_len = (alg_len % HUGE_PAGE_SIZE == 0) ? alg_len: alg_len + HUGE_PAGE_SIZE - (alg_len % HUGE_PAGE_SIZE);
				end_addr = addr + alg_len;
				break;
			}
		}
		cur = cur->vm_next;
	}
	// printk("st_addr = %x , end_addr = %x\n",st_addr, end_addr);
	// split the vma according to the unmapped area length
	cur = current->vm_area;
	struct vm_area* prev = NULL;

	struct vm_area* prev_vma = NULL;
	struct vm_area* st_vma = NULL;
	struct vm_area* end_vma = NULL;
	while(cur){
		if(cur->vm_start >= end_addr) break;
		if(cur->vm_start < st_addr && cur->vm_end > st_addr){
			// left split
			struct vm_area* new_vma = create_vm_area(st_addr,cur->vm_end, cur->access_flags, cur->mapping_type);
			new_vma->vm_next = cur->vm_next;
			cur->vm_next = new_vma;
			cur->vm_end = st_addr;
		}else if(cur->vm_start < end_addr && cur->vm_end > end_addr){
			// right split
			struct vm_area* new_vma = create_vm_area(end_addr, cur->vm_end, cur->access_flags, cur->mapping_type);
			new_vma->vm_next = cur->vm_next;
			cur->vm_next = new_vma;
			cur->vm_end = end_addr;
		}
		cur = cur->vm_next;
	}
	cur = current->vm_area;
	prev = NULL;
	int do_free = 0;
	while(cur){
		if(cur->vm_start >= end_addr) break;
		if(cur->vm_start == st_addr){
			prev_vma = prev;
			do_free = 1;
		}
		if(do_free){
			if(cur->mapping_type == NORMAL_PAGE_MAPPING){
				remove_pma_normal(current,cur);
			}else{
				remove_pma_huge(current,cur);
			}
			prev = cur;
			cur= cur->vm_next;
			if(prev->vm_end == end_addr){
				do_free = 0;
				end_vma = prev->vm_next;
			}
			dealloc_vm_area(prev);
			if(!do_free) break;
		}else{
			prev = cur;
			cur= cur->vm_next;
		}
	// printk("I am inside pma loop\n");
	}
	prev_vma->vm_next = end_vma;
	return 0;
}


// ********************************************** MAKE HUGEPAGE IMPLEMENTATION ***************************************************************************
/** 
 * This function checks if there is any unmapped(virtual to virtual) region in between st_addr to end_addr
*/
int vma_mapped(struct exec_context *current, u64 st_addr, u64 end_addr){
	struct vm_area* cur = current->vm_area;
	struct vm_area* prev = NULL;
	int range_running = 0;
	int permission_check = 0;
	while(cur){
		if( cur->vm_start <=st_addr && cur->vm_end >= end_addr){
			permission_check = 1;
		}
		if(!range_running && (cur->vm_end > st_addr && cur->vm_end <= end_addr)){
			range_running = 1;
		}
		if(range_running && (cur->vm_start >= end_addr || cur->vm_end == end_addr)){
			range_running = 0;
			break;
		}
		if(range_running && cur->vm_end < end_addr){
			if(cur->vm_end != cur->vm_next->vm_start){
				return -ENOMAPPING;
			}
		}
		if(range_running && cur->mapping_type == HUGE_PAGE_MAPPING){
			return -EVMAOCCUPIED;
		}
		prev = cur;
		cur = cur->vm_next;
	}
	if(range_running){
		return -ENOMAPPING;
	}
	if(permission_check) return 1; // same permissions and all mapped
	else return 0; // diff permissions, but all mapped
}


/**
 * This function handles the physical memory in make_hugepages()
  */
void make_hugepage_pma_handler(struct exec_context* current, u64 st_addr, u32 prot){
	// check if any page is allocated
	u64* huge_pfn_vaddr_base = NULL;
	u64 huge_pfn = 0; u64 pte_pfn = 0;
	int flag = 0;
	u64 addr = st_addr;
	u64 *vaddr_base = (u64 *)osmap(current->pgd);
	u64 *entry;
	u64 pfn;
	u64 ac_flags = 0x5 | (prot & 0x2);
	entry = vaddr_base + ((addr & PGD_MASK) >> PGD_SHIFT);
	if(*entry & 0x1) {
		// PGD->PUD Present, access it
		pfn = (*entry >> PTE_SHIFT) & 0xFFFFFFFF;
		vaddr_base = (u64 *)osmap(pfn);
	}else{
		// allocate PUD
		return;
	}

	entry = vaddr_base + ((addr & PUD_MASK) >> PUD_SHIFT);
	if(*entry & 0x1) {
		// PUD->PMD Present, access it
		pfn = (*entry >> PTE_SHIFT) & 0xFFFFFFFF;
		vaddr_base = (u64 *)osmap(pfn);
	}else{
		// allocate PMD
		return;
	}

	entry = vaddr_base + ((addr & PMD_MASK) >> PMD_SHIFT);
	u64* pmd_entry = entry;
	u64* pte_vaddr_base = NULL;
	if(*entry & 0x1) {
		// PMD->PTE Present, access it
		pfn = (*entry >> PTE_SHIFT) & 0xFFFFFFFF;
		pte_pfn = pfn;
		pte_vaddr_base = (u64 *)osmap(pfn);
	}else{
		// allocate PLD
		return;
	}
	// printk("juts before for loop\n");
	for( u64 offset = 0 ; offset < HUGE_PAGE_SIZE; offset += NORMAL_PAGE_SIZE){
		
		entry = pte_vaddr_base + (((addr + offset) & PTE_MASK) >> PTE_SHIFT);
		// entry = pte_vaddr_base++;
		if(*entry & 0x1) {
			// PTE->PFN Present, access it
			// printk("entry = %x\n", *entry);
			if(!flag){
				huge_pfn_vaddr_base = os_hugepage_alloc();
				huge_pfn = get_hugepage_pfn(huge_pfn_vaddr_base);
				flag = 1;
				*pmd_entry = (huge_pfn << PMD_SHIFT) | ac_flags;
				*pmd_entry = *pmd_entry | (1<<7);
			}
			pfn = (*entry >> PTE_SHIFT) & 0xFFFFFFFF;
			// printk("pfn = %x\n",pfn);flush_tlb(addr);
			vaddr_base = (u64 *)osmap(pfn);
			// printk("vaddr_base = %x\n",vaddr_base);
			memcpy((char*)((u64)huge_pfn_vaddr_base + offset),(char*)vaddr_base,NORMAL_PAGE_SIZE);
			
			os_pfn_free(USER_REG,pfn);
			*entry = *entry ^ (0x1);
		}else{
			// allocate PLD
			continue;
		}
		flush_tlb(addr+offset);
	}
	if(flag) os_pfn_free(OS_PT_REG, pte_pfn);
}	

/**
 * This function merges the vma for creating the hugepage without checking the permissions of the vma. 
 */
void merge_vma_into_huge(struct exec_context *current, u32 prot, u64 st_addr, u64 end_addr){
	struct vm_area* cur = current->vm_area;
	struct vm_area* prev = NULL;
	struct vm_area* prev_vma = NULL;
	struct vm_area* end_vma = NULL;

	while(cur){
		if(cur->vm_start >= end_addr) break;
		if(cur->vm_start < st_addr && cur->vm_end > st_addr){
			// left split
			struct vm_area* new_vma = create_vm_area(st_addr,cur->vm_end, cur->access_flags, cur->mapping_type);
			new_vma->vm_next = cur->vm_next;
			cur->vm_next = new_vma;
			cur->vm_end = st_addr;
		}else if(cur->vm_start < end_addr && cur->vm_end > end_addr){
			// right split
			struct vm_area* new_vma = create_vm_area(end_addr, cur->vm_end, cur->access_flags, cur->mapping_type);
			new_vma->vm_next = cur->vm_next;
			cur->vm_next = new_vma;
			cur->vm_end = end_addr;
		}
		cur = cur->vm_next;
	}

	cur = current->vm_area;
	prev = NULL;
	int do_free = 0;
	while(cur){
		if(cur->vm_start >= end_addr) break;
		if(cur->vm_start == st_addr){
			prev_vma = prev;
			do_free = 1;
		}
		if(do_free){
			prev = cur;
			cur= cur->vm_next;
			if(prev->vm_end == end_addr){
				do_free = 0;
				end_vma = prev->vm_next;
			}
			dealloc_vm_area(prev);
			if(!do_free) break;
		}else{
			prev = cur;
			cur= cur->vm_next;
		}
	}
	struct vm_area* new_vma = create_vm_area(st_addr, end_addr, prot, HUGE_PAGE_MAPPING);
	new_vma->vm_next = end_vma;
	prev_vma -> vm_next = new_vma;
}


/**
 * make_hugepage system call implemenation
 */
long vm_area_make_hugepage(struct exec_context *current, void *addr, u32 length, u32 prot, u32 force_prot)
{
	if(!addr || length<=0) return -EINVAL;
	u64 st_addr = (u64)addr;
	u64 end_addr = (u64)addr + length;
	st_addr = (st_addr % HUGE_PAGE_SIZE == 0) ? st_addr : st_addr + HUGE_PAGE_SIZE - (st_addr % HUGE_PAGE_SIZE);
	end_addr = (end_addr % HUGE_PAGE_SIZE == 0) ? end_addr : end_addr - (end_addr % HUGE_PAGE_SIZE);
	// printk("range = [%x, %x]\n", st_addr, end_addr);
	// zero huge pages cases
	if(end_addr - st_addr <= 0) return -1;
	// huge pages are not zero
	int ret_vma_mapped = vma_mapped(current, st_addr, end_addr);
	// check if all the vmas are mapped in [st_addr, end_addr]
	if( ret_vma_mapped == -ENOMAPPING) return -ENOMAPPING;
	// check if already any huge page vma is present or not
	if( ret_vma_mapped == -EVMAOCCUPIED) return -EVMAOCCUPIED;
	if( ret_vma_mapped == 0 && !force_prot ) return -EDIFFPROT;
	

	// u64
	for(u64 arg_addr = st_addr; arg_addr < end_addr ; arg_addr += HUGE_PAGE_SIZE){
		make_hugepage_pma_handler(current, arg_addr, prot);
	}
	merge_vma_into_huge(current, prot, st_addr, end_addr);
	// handle_pma(current, st_addr, end_addr, prot);
	merge_vmas_based_on_type(current);
	return st_addr;
}


// ************************************************* BREAK HUGEPAGE IMPLEMENTATION *************************************************************************
void change_pm_map(struct exec_context *current, struct vm_area* cur){
	u64 st_addr = cur->vm_start;
	u64 end_addr = cur->vm_end;
	u64 prot = cur->access_flags;
	// u64 count_pages = (cur->vm_end - cur->vm_start) % HUGE_PAGE_SIZE;
	for(u64 offset = 0; offset < end_addr ; offset+=HUGE_PAGE_SIZE){
		u64 addr = st_addr + offset;
		u64 *pgd_vaddr_base = (u64*)osmap(current->pgd);
		u64 *pud_vaddr_base; u64 *pmd_vaddr_base; u64 *pte_vaddr_base;u64 *pfn_vaddr_base; u64 *small_pfn_vaddr_base;
		u64 *pgd_entry; u64 *pud_entry; u64 *pte_entry; u64 *pmd_entry;
		u64 pud_pfn;u64 pte_pfn; u64 pmd_pfn;u64 huge_pfn;u64 normal_pfn;
		u64 ac_flags = 0x5 | (prot);

		pgd_entry = pgd_vaddr_base + ((addr & PGD_MASK) >> PGD_SHIFT);
		if(*pgd_entry & 0x1) {
			// PGD->PUD Present, access it
			pud_pfn = (*pgd_entry >> PTE_SHIFT) & 0xFFFFFFFF;
			pud_vaddr_base = (u64 *)osmap(pud_pfn);
		}else{
			continue;
		}

		pud_entry = pud_vaddr_base + ((addr & PUD_MASK) >> PUD_SHIFT);
		if(*pud_entry & 0x1) {
			// PUD->PMD Present, access it
			pmd_pfn = (*pud_entry >> PTE_SHIFT) & 0xFFFFFFFF;
			pmd_vaddr_base = (u64 *)osmap(pmd_pfn);
		}else{
			continue;
		}

		pmd_entry = pmd_vaddr_base + ((addr & PMD_MASK) >> PMD_SHIFT);
		if(*pmd_entry & 0x1){
			// PTE->PFN present, access it
			huge_pfn = (*pmd_entry >> PMD_SHIFT) & 0xFFFFFFFF;
			pfn_vaddr_base = (u64 *)(huge_pfn*HUGE_PAGE_SIZE);
		}else{
			continue;
		}
		pte_pfn = os_pfn_alloc(OS_PT_REG);
		*pmd_entry = (pte_pfn << PTE_SHIFT) | ac_flags;
		pte_vaddr_base = (u64*)osmap(pte_pfn);
		
		for(u64 page_addr = 0 ; page_addr < HUGE_PAGE_SIZE; page_addr += NORMAL_PAGE_SIZE){
			pte_entry = pte_vaddr_base + (((addr+ page_addr) & PTE_MASK) >> PTE_SHIFT);
			u64 normal_pfn =  os_pfn_alloc(USER_REG);
			*pte_entry = (normal_pfn << PTE_SHIFT) | ac_flags;
			small_pfn_vaddr_base = (u64*)osmap(normal_pfn);
			memcpy((char*)small_pfn_vaddr_base, (char*)((u64)pfn_vaddr_base + page_addr) , NORMAL_PAGE_SIZE);
			flush_tlb(addr + page_addr);
			
		}
		flush_tlb(addr);
		os_hugepage_free(pfn_vaddr_base);
	}
}



void change_hp(struct exec_context *current, u64 addr, u32 prot) {
	u64* pud_vaddr_base; u64 *pmd_vaddr_base; u64 *pte_vaddr_base; u64 *huge_vaddr_base;
	u64* pgd_vaddr_base = (u64 *)osmap(current->pgd);
	u64 *pgd_entry; u64* pud_entry; u64* pmd_entry; u64* pte_entry;
	u64 pud_pfn; u64 pmd_pfn; u64 pte_pfn; u64 normal_page_pfn; u64 huge_page_pfn;
	u64 ac_flags = 0x5 | prot;
	// u64 count_pages = (cur->vm_end - cur->vm_start) % HUGE_PAGE_SIZE;
	pgd_entry = pgd_vaddr_base + ((addr & PGD_MASK) >> PGD_SHIFT);
	if(*pgd_entry & 0x1) {
		// PGD->PUD Present, access it
		pud_pfn = (*pgd_entry >> PTE_SHIFT) & 0xFFFFFFFF;
		pud_vaddr_base = (u64 *)osmap(pud_pfn);
	}else{
		return;
	}

	pud_entry = pud_vaddr_base + ((addr & PUD_MASK) >> PUD_SHIFT);
	if(*pud_entry & 0x1) {
		// PUD->PMD Present, access it
		pmd_pfn = (*pud_entry >> PTE_SHIFT) & 0xFFFFFFFF;
		pmd_vaddr_base = (u64 *)osmap(pmd_pfn);
	}else{
		return;
	}

	pmd_entry = pmd_vaddr_base + ((addr & PMD_MASK) >> PMD_SHIFT);
	if(*pmd_entry & 0x1) {
		huge_page_pfn = (u64)((*pmd_entry >> PMD_SHIFT) & 0xFFFFFFFF);
		huge_vaddr_base = (u64 *)((huge_page_pfn * HUGE_PAGE_SIZE));
	}else{
		return;
	}
	
	pte_pfn = os_pfn_alloc(OS_PT_REG);
	pte_vaddr_base = osmap(pte_pfn);
	*pmd_entry = (pte_pfn << PTE_SHIFT) | ac_flags;
	
	u64 pte_addr = addr;
	while(pte_addr < addr + HUGE_PAGE_SIZE) {
		u64 pfn_pte_entry = os_pfn_alloc(USER_REG);
		pte_entry = pte_vaddr_base + ((pte_addr & PTE_MASK) >> PTE_SHIFT);
		*pte_entry = (pfn_pte_entry << PTE_SHIFT) | ac_flags;
		memcpy((char *)osmap(pfn_pte_entry), (char *)((u64)huge_vaddr_base + ((pte_addr - addr))), NORMAL_PAGE_SIZE);
		flush_tlb(pte_addr);
		pte_addr += NORMAL_PAGE_SIZE;
	}
	
	flush_tlb(addr);
	os_hugepage_free(huge_vaddr_base);
}

void change_pm_brk_hp(struct exec_context *current, struct vm_area *cur) {
	u64 st_addr = cur->vm_start;
	u64 end_addr = cur->vm_end;
	for(u64 addr = st_addr; addr < end_addr; addr+= HUGE_PAGE_SIZE){
		change_hp(current, addr, cur->access_flags);
	}
}


/**
 * break_system call implemenation
 */
int vm_area_break_hugepage(struct exec_context *current, void *addr, u32 length)
{
	if(!addr || length<=0 ) return -1;
	if((u64)addr % HUGE_PAGE_SIZE || length % HUGE_PAGE_SIZE) return -EINVAL;
	u64 st_addr = (u64)addr;
	u64 end_addr = (u64)addr + length;
	// check if any hugepage mapping is present or not
	// split the vma if it is partially included(split only if huge page mapping)
	struct vm_area* cur = current->vm_area;
	int huge_page_exists = 0;
	// printk("split vmas\n");
	while(cur){
		if(cur->mapping_type == HUGE_PAGE_MAPPING && ((cur->vm_end >= st_addr && cur->vm_end <= end_addr) || (cur->vm_start >= st_addr && cur->vm_start <= end_addr))) huge_page_exists = 1;
		if(cur->vm_start >= end_addr) break;
		if(cur->mapping_type == HUGE_PAGE_MAPPING && cur->vm_start < st_addr && cur->vm_end > st_addr){
			// left split
			struct vm_area* new_vma = create_vm_area(st_addr,cur->vm_end, cur->access_flags, cur->mapping_type);
			new_vma->vm_next = cur->vm_next;
			cur->vm_next = new_vma;
			cur->vm_end = st_addr;
		}else if( cur->mapping_type == HUGE_PAGE_MAPPING && cur->vm_start < end_addr && cur->vm_end > end_addr){
			// right split
			struct vm_area* new_vma = create_vm_area(end_addr, cur->vm_end, cur->access_flags, cur->mapping_type);
			new_vma->vm_next = cur->vm_next;
			cur->vm_next = new_vma;
			cur->vm_end = end_addr;
		}
		cur = cur->vm_next;
		// printk("Splitting the vmas\n");
	}
	// printk("convert the mappings to normal\n");
	if(!huge_page_exists) return 0;
	cur = current->vm_area;
	while(cur){
		if(cur->vm_start >= end_addr) break;
		// if(cur->mapping_type == HUGE_PAGE_MAPPING && ((cur->vm_end >= st_addr && cur->vm_end <= end_addr) || (cur->vm_start >= st_addr && cur->vm_start <= end_addr))){
		if(cur->mapping_type == HUGE_PAGE_MAPPING && cur->vm_start >= st_addr ){	
			// handle physical memory
			// change_pm_map(current,cur);
			change_pm_brk_hp(current, cur);
			cur->mapping_type = NORMAL_PAGE_MAPPING;
		}
		cur= cur->vm_next;
		// printk("changing the mappings\n");
	}
	// printk(" merge vmas now\n");
	// merge operation
	merge_vmas_based_on_type(current);
	return 0;
}
