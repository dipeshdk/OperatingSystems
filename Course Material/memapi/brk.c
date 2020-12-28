#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

extern char etext, edata, end; /*see man(3) end*/

int main()
{
   void *ptr = sbrk(0);
   // printf("%s: End of text %p\n", __FILE__, &etext); // edn of code segment
   // printf("%s: End of initialized data %p\n", __FILE__, &edata); // end of initialized data
   printf("%s: End of uninitialized data (at load time) %p\n", __FILE__, &end); // end of uninitialised data
   printf("%s: End of uninitialized data (at main) = %p now = %p\n", __FILE__, ptr, sbrk(0));
   
   if(sbrk(4096 * 16) == (void *)-1){ // allocating a memory of 64KB
        printf("sbrk failed\n");
   }
   printf("%s: End of uninitialized data after expand = %p\n", __FILE__, sbrk(0)); 
}
