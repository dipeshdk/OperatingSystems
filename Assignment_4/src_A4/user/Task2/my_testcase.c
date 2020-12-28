#include<ulib.h>



int main(u64 arg1, u64 arg2, u64 arg3, u64 arg4, u64 arg5)
{

	int pageSize = 4096;
	u32 MB = 1 << 20;
	// allocate 8M region
	char *addr = mmap(NULL, 8*MB, PROT_READ|PROT_WRITE, 0);
	pmap(1);
	printf("mmap start_addr = %x\n", addr); // addr is not 2MB aligned
	
	// find next 2M aligned 
	u64 aligned = (u64)addr;
	// if(aligned & 0x1fffff){
	// 	aligned >>= 21;
	// 	++aligned;
	// 	aligned <<= 21;
	// }
	
	int printing = 50;
	for (int i = 0; i < 8*MB/1024 ; i++) { // Number of page faults = 2048
		*(((char *)aligned) + i*1024) = 'A' + (i)%26;
		if(printing > 0) printf("%c ", 'A'+(i)%26);
		printing--;
	}
	printf("\n\n");
    char *addr_aligned = (char *) make_hugepage(addr, 8*MB , PROT_READ|PROT_WRITE, 0);
    /*
	  Find next huge page boundary address (aligned)
      Make as many huge pages as possible in the address range (aligned, addr+8M)
      The starting of the huge page should be from a 2MB aligned address, aligned
    */

    pmap(1); //2048

    int diff = 0;
	printing = 50;
	for (int i = 0; i < 8*MB/1024; i++) {
		if(printing > 0) printf("%c ", addr[i*1024]);
		printing--;
        // The content of the pages that get converted to huge pages are preserved.
		if (addr[i*1024] != ('A'+ ((i)%26))) {
			++diff;	
		}
	}

	pmap(1); //2048
	printf("%d\n", diff);
    if(diff)
		printf("Test failed\n");
	else
		printf("Test passed\n");
    

    break_hugepage(addr_aligned, 4*MB);
    pmap(1);

    diff = 0;
	printing = 50;
	for (int i = 0; i < 8*MB/1024; i++) {
		if(printing > 0) printf("%c ", addr[i*1024]);
		printing--;
        // The content of the pages that get converted to huge pages are preserved.
		if (addr[i*1024] != ('A'+ ((i)%26))) {
			++diff;	
		}
	}
    printf("\n\n");
    // printf("diff = %x\n", diff);
	if(diff)
		printf("Test failed\n");
	else
		printf("Test passed\n");
    
    pmap(1);

    munmap(addr,1*MB);
    pmap(1);

    diff = 0;
	printing = 50;
	for (int i = (1*MB/1024 + 1); i < 8*MB/1024; i++) {
		if(printing > 0) printf("%c ", addr[i*1024]);
		printing--;
        // The content of the pages that get converted to huge pages are preserved.
		if (addr[i*1024] != ('A'+ ((i)%26))) {
			++diff;	
		}
	}
    printf("\n\n");
    // printf("diff = %x\n", diff);
	if(diff)
		printf("Test failed\n");
	else
		printf("Test passed\n");
    
    pmap(1);

    munmap(addr+2*MB,2*MB);
    pmap(1);

    diff = 0;
	printing = 50;
	for (int i = (1*MB/1024 + 1); i < 2*MB/1024; i++) {
		if(printing > 0) printf("%c ", addr[i*1024]);
		printing--;
        // The content of the pages that get converted to huge pages are preserved.
		if (addr[i*1024] != ('A'+ ((i)%26))) {
			++diff;	
		}
	}
    printing = 50;
	for (int i = (4*MB/1024 + 1); i < 8*MB/1024; i++) {
		if(printing > 0) printf("%c ", addr[i*1024]);
		printing--;
        // The content of the pages that get converted to huge pages are preserved.
		if (addr[i*1024] != ('A'+ ((i)%26))) {
			++diff;	
		}
	}
    printf("\n\n");
    // printf("diff = %x\n", diff);
	if(diff)
		printf("Test failed\n");
	else
		printf("Test passed\n");
    
    pmap(1);

    
}