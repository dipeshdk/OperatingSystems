// #include<ulib.h>

// int fn_2(int i);
// int fn_1(int i);
// int fn_2(int i)
// {
// 	if( i == 0) return 0;
// 	printf("In fn2 i = %d\n", i);	
// 	return fn_1(i-1);
// }

// int fn_1(int i)
// {
// 	if(i ==0) return 0;
// 	printf("In fn1 i = %d\n",i);	
// 	return fn_2(i-1);
// }


// int main(u64 arg1, u64 arg2, u64 arg3, u64 arg4, u64 arg5)
// {
// 	int cpid;
// 	long ret = 0;
// 	int i, bt_count;
// 	unsigned long long bt_info[MAX_BACKTRACE];
	
// 	ret = become_debugger();
	
// 	cpid = fork();
// 	// printk("I am exiting wait and continue\n");
// 	if(cpid < 0){
// 		printf("Error in fork\n");
// 	}
// 	else if(cpid == 0){
// 		printf("fn_1 : %x\n", fn_1);
// 		printf("fn_2 : %x\n", fn_2);
// 		fn_1(0);
// 		fn_2(0);
// 		fn_1(4);
// 	}
// 	else{
// 		ret = set_breakpoint(fn_1);
// 		ret = set_breakpoint(fn_2);
		
// 		// fn_1 
// 		ret = wait_and_continue();
// 		printf("Breakpoint hit at : %x\n", ret);
// 		printf("BACKTRACE INFORMATION\n");
// 		bt_count = backtrace((void*)&bt_info);
		
// 		printf("Backtrace count: %d\n", bt_count);
// 		for(int i = 0; i < bt_count; i++)
// 		{
// 			printf("#%d %x\n", i, bt_info[i]);
// 		}

// 		// fn_2
// 		ret = wait_and_continue();
// 		printf("Breakpoint hit at : %x\n", ret);
// 		printf("BACKTRACE INFORMATION\n");
// 		bt_count = backtrace((void*)&bt_info);
		
// 		printf("Backtrace count: %d\n", bt_count);
// 		for(int i = 0; i < bt_count; i++)
// 		{
// 			printf("#%d %x\n", i, bt_info[i]);
// 		}

// 		// fn_1 4
// 		ret = wait_and_continue();
// 		printf("Breakpoint hit at : %x\n", ret);
// 		printf("BACKTRACE INFORMATION\n");
// 		bt_count = backtrace((void*)&bt_info);
		
// 		printf("Backtrace count: %d\n", bt_count);
// 		for(int i = 0; i < bt_count; i++)
// 		{
// 			printf("#%d %x\n", i, bt_info[i]);
// 		}

// 		// fn_2 3
// 		ret = wait_and_continue();
// 		printf("Breakpoint hit at : %x\n", ret);
// 		printf("BACKTRACE INFORMATION\n");
// 		bt_count = backtrace((void*)&bt_info);
		
// 		printf("Backtrace count: %d\n", bt_count);
// 		for(int i = 0; i < bt_count; i++)
// 		{
// 			printf("#%d %x\n", i, bt_info[i]);
// 		}

// 		// fn_1 2
// 		ret = wait_and_continue();
// 		// printf("I came here\n");
// 		printf("Breakpoint hit at : %x\n", ret);
// 		printf("BACKTRACE INFORMATION\n");
// 		bt_count = backtrace((void*)&bt_info);
		
// 		printf("Backtrace count: %d\n", bt_count);
// 		for(int i = 0; i < bt_count; i++)
// 		{
// 			printf("#%d %x\n", i, bt_info[i]);
// 		}
// 		// printf("I came here now exiting\n");

// 		// fn_2 1
// 		ret = wait_and_continue();
// 		printf("Breakpoint hit at : %x\n", ret);
// 		printf("BACKTRACE INFORMATION\n");
// 		bt_count = backtrace((void*)&bt_info);
		
// 		printf("Backtrace count: %d\n", bt_count);
// 		for(int i = 0; i < bt_count; i++)
// 		{
// 			printf("#%d %x\n", i, bt_info[i]);
// 		}

// 		// fn_1 0
// 		ret = wait_and_continue();
// 		printf("Breakpoint hit at : %x\n", ret);
// 		printf("BACKTRACE INFORMATION\n");
// 		bt_count = backtrace((void*)&bt_info);
		
// 		printf("Backtrace count: %d\n", bt_count);
// 		for(int i = 0; i < bt_count; i++)
// 		{
// 			printf("#%d %x\n", i, bt_info[i]);
// 		}
		
// 		// for exit
// 		ret = wait_and_continue();	
// 		printf("Child exit return : %x\n", ret);
// 	}
	
// 	return 0;
// }
#include <ulib.h>


void fn_1(int x)
{
    printf("x = %d\n", x);
    if (x == 0)
        return;
    fn_1(x - 1);
}

int fib_merge(int);

int fib(int i){
    if(i <= 1) return i;
    return fib_merge(i);
}

int fib_merge(int i){
    return fib(i-1) + fib(i-2);
}

int fib_rec(int i){
    if(i <= 1) return i;
    return fib_rec(i-1) + fib_rec(i-2);
}

void print_regs(struct registers reg_info){
    printf("Registers:\n");
    printf("r15: %x\n", reg_info.r15);
    printf("r14: %x\n", reg_info.r14);
    printf("r13: %x\n", reg_info.r13);
    printf("r12: %x\n", reg_info.r12);
    printf("r11: %x\n", reg_info.r11);
    printf("r10: %x\n", reg_info.r10);
    printf("r9: %x\n", reg_info.r9);
    printf("r8: %x\n", reg_info.r8);
    printf("rbp: %x\n", reg_info.rbp);
    printf("rdi: %x\n", reg_info.rdi);
    printf("rsi: %x\n", reg_info.rsi);
    printf("rdx: %x\n", reg_info.rdx);
    printf("rcx: %x\n", reg_info.rcx);
    printf("rbx: %x\n", reg_info.rbx);
    printf("rax: %x\n", reg_info.rax);
    printf("entry_rip: %x\n", reg_info.entry_rip);
    printf("entry_cs: %x\n", reg_info.entry_cs);
    printf("entry_rflags: %x\n", reg_info.entry_rflags);
    printf("entry_rsp: %x\n", reg_info.entry_rsp);
    printf("entry_ss: %x\n", reg_info.entry_ss);
}

int main(u64 arg1, u64 arg2, u64 arg3, u64 arg4, u64 arg5)
{
    int cpid;
    long ret = 0;
    int i, bt_count;
    unsigned long long bt_info[MAX_BACKTRACE];

    ret = become_debugger();

    cpid = fork();

    if (cpid < 0)
    {
        printf("Error in fork\n");
    }
    else if (cpid == 0)
    {
        printf("fib = %d\n", fib(4));
        // printf("fib_rec = %d\n", fib_rec(4));
    }
    else
    {
        int ret_fib = set_breakpoint(fib);
        int ret_fib_merg = set_breakpoint(fib_merge);
        // int ret_fib_rec = set_breakpoint(fib_rec);
        struct breakpoint bp[100];
        int bp_num = info_breakpoints(bp);
        for(int i=0; i < bp_num; i++){
            printf("num = %d addr = %x\n", bp[i].num, bp[i].addr);
        }
        s64 ret_wait;
        int cnt = 0;
        while(ret_wait = wait_and_continue()){
            printf("breakpoint addr %x\n", ret_wait);
            if(1){
                remove_breakpoint((void*)ret_wait);
                if((void*)ret_wait == fib && cnt != 0){
                    set_breakpoint(fib_merge);
                }else if(cnt != 0){
                    set_breakpoint(fib);
                }
            }
            // struct registers reg;
            // info_registers(&reg);
            // print_regs(reg);
            bp_num = info_breakpoints(bp);
            for(int i=0; i < bp_num; i++){
                printf("num = %d addr = %x\n", bp[i].num, bp[i].addr);
            }
            cnt++;
        }
    }

    return 0;
}




// #include <ulib.h>


// void fn_1(int x)
// {
//     printf("x = %d\n", x);
//     if (x == 0)
//         return;
//     fn_1(x - 1);
// }

// int fib_merge(int);

// int fib(int i){
//     if(i <= 1) return i;
//     return fib_merge(i);
// }

// int fib_merge(int i){
//     return fib(i-1) + fib(i-2);
// }

// int fib_rec(int i){
//     if(i <= 1) return i;
//     return fib_rec(i-1) + fib_rec(i-2);
// }

// void print_regs(struct registers reg_info){
//     printf("Registers:\n");
//     printf("r15: %x\n", reg_info.r15);
//     printf("r14: %x\n", reg_info.r14);
//     printf("r13: %x\n", reg_info.r13);
//     printf("r12: %x\n", reg_info.r12);
//     printf("r11: %x\n", reg_info.r11);
//     printf("r10: %x\n", reg_info.r10);
//     printf("r9: %x\n", reg_info.r9);
//     printf("r8: %x\n", reg_info.r8);
//     printf("rbp: %x\n", reg_info.rbp);
//     printf("rdi: %x\n", reg_info.rdi);
//     printf("rsi: %x\n", reg_info.rsi);
//     printf("rdx: %x\n", reg_info.rdx);
//     printf("rcx: %x\n", reg_info.rcx);
//     printf("rbx: %x\n", reg_info.rbx);
//     printf("rax: %x\n", reg_info.rax);
//     printf("entry_rip: %x\n", reg_info.entry_rip);
//     printf("entry_cs: %x\n", reg_info.entry_cs);
//     printf("entry_rflags: %x\n", reg_info.entry_rflags);
//     printf("entry_rsp: %x\n", reg_info.entry_rsp);
//     printf("entry_ss: %x\n", reg_info.entry_ss);
// }

// int main(u64 arg1, u64 arg2, u64 arg3, u64 arg4, u64 arg5)
// {
//     int cpid;
//     long ret = 0;
//     int i, bt_count;
//     unsigned long long bt_info[MAX_BACKTRACE];

//     ret = become_debugger();

//     cpid = fork();

//     if (cpid < 0)
//     {
//         printf("Error in fork\n");
//     }
//     else if (cpid == 0)
//     {
//         printf("fib = %d\n", fib(4));
//         // printf("fib_rec = %d\n", fib_rec(4));
//     }
//     else
//     {
//         int ret_fib = set_breakpoint(fib);
//         int ret_fib_merg = set_breakpoint(fib_merge);
//         // int ret_fib_rec = set_breakpoint(fib_rec);
//         struct breakpoint bp[100];
//         int bp_num = info_breakpoints(bp);
//         for(int i=0; i < bp_num; i++){
//             printf("num = %d addr = %x\n", bp[i].num, bp[i].addr);
//         }
//         s64 ret_wait;
//         int cnt = 0;
//         while(ret_wait = wait_and_continue()){
//             printf("breakpoint addr %x\n", ret_wait);
//             if(1){
//                 remove_breakpoint((void*)ret_wait);
//                 if((void*)ret_wait == fib && cnt != 0){
//                     set_breakpoint(fib_merge);
//                 }else if(cnt != 0){
//                     set_breakpoint(fib);
//                 }
//             }
//             // struct registers reg;
//             // info_registers(&reg);
//             // print_regs(reg);
//             bp_num = info_breakpoints(bp);
//             for(int i=0; i < bp_num; i++){
//                 printf("num = %d addr = %x\n", bp[i].num, bp[i].addr);
//             }
//             cnt++;
//         }
//     }
// 	// printf(" I am exiting with pid = %d\n", getpid());
//     return 0;
// }

// #include<ulib.h>

// int fn_2()
// {
// 	printf("In fn2\n");	
// 	return 0;
// }

// int fn_1()
// {
// 	printf("In fn1\n");	
// 	return 0;
// }

// int fib(int n){
// 	printf("In fib\n");
// 	if(n == 1 || n == 0) return 1;
// 	return fib(n-1) + fib(n-2);
// }

// int main(u64 arg1, u64 arg2, u64 arg3, u64 arg4, u64 arg5)
// {
// 	int cpid;
// 	long ret = 0;
// 	int i, bt_count;
// 	unsigned long long bt_info[MAX_BACKTRACE];
	
// 	ret = become_debugger();
	
// 	cpid = fork();
	
// 	if(cpid < 0){
// 		printf("Error in fork\n");
// 	}
// 	else if(cpid == 0){
// 		printf("fn_1 : %x\n", fn_1);
// 		printf("fn_2 : %x\n", fn_2);
// 		printf("fib : %x\n", fib);
// 		fn_1();
// 		fn_2();
// 		fib(2);
// 	}
// 	else{
// 		ret = set_breakpoint(fn_1);
// 		ret = set_breakpoint(fn_2);
// 		ret = set_breakpoint(fib);
// 		// fn_1 
// 		ret = wait_and_continue();
// 		printf("Breakpoint hit at : %x\n", ret);
// 		printf("BACKTRACE INFORMATION\n");
// 		bt_count = backtrace((void*)&bt_info);
		
// 		printf("Backtrace count: %d\n", bt_count);
// 		for(int i = 0; i < bt_count; i++)
// 		{
// 			printf("#%d %x\n", i, bt_info[i]);
// 		}

// 		// fn_2
// 		ret = wait_and_continue();
// 		printf("Breakpoint hit at : %x\n", ret);
// 		printf("BACKTRACE INFORMATION\n");
// 		bt_count = backtrace((void*)&bt_info);
		
// 		printf("Backtrace count: %d\n", bt_count);
// 		for(int i = 0; i < bt_count; i++)
// 		{
// 			printf("#%d %x\n", i, bt_info[i]);
// 		}

// 		// fib
// 		ret = wait_and_continue();
// 		printf("Breakpoint hit at : %x\n", ret);
// 		printf("BACKTRACE INFORMATION\n");
// 		bt_count = backtrace((void*)&bt_info);
		
// 		printf("Backtrace count: %d\n", bt_count);
// 		for(int i = 0; i < bt_count; i++)
// 		{
// 			printf("#%d %x\n", i, bt_info[i]);
// 		}

// 		// for exit
// 		ret = wait_and_continue();
// 		printf("Breakpoint hit at : %x\n", ret);
// 		printf("BACKTRACE INFORMATION\n");
// 		bt_count = backtrace((void*)&bt_info);
		
// 		printf("Backtrace count: %d\n", bt_count);
// 		for(int i = 0; i < bt_count; i++)
// 		{
// 			printf("#%d %x\n", i, bt_info[i]);
// 		}
		
// 		ret = wait_and_continue();
// 		printf("Breakpoint hit at : %x\n", ret);
// 		printf("BACKTRACE INFORMATION\n");
// 		bt_count = backtrace((void*)&bt_info);
		
// 		printf("Backtrace count: %d\n", bt_count);
// 		for(int i = 0; i < bt_count; i++)
// 		{
// 			printf("#%d %x\n", i, bt_info[i]);
// 		}

// 		//exit
// 		ret = wait_and_continue();
// 		// ret = wait_and_continue();	
// 	}
// 	printf(" %d I am exiting\n", getpid());
// 	return 0;
// }



 
// #include<ulib.h>

// int fn_2(int i);
// int fn_1(int i);
// int fn_2(int i)
// {
// 	if( i == 0) return 0;
// 	printf("In fn2 i = %d\n", i);	
// 	return fn_1(i-1);
// }

// int fn_1(int i)
// {
// 	if(i == 0) return 0;
// 	printf("In fn1 i = %d\n",i);	
// 	return fn_2(i-1);
// }


// int main(u64 arg1, u64 arg2, u64 arg3, u64 arg4, u64 arg5)
// {
// 	int cpid;
// 	long ret = 0;
// 	int i, bt_count;
// 	unsigned long long bt_info[MAX_BACKTRACE];
	
// 	ret = become_debugger();
	
// 	cpid = fork();
	
// 	if(cpid < 0){
// 		printf("Error in fork\n");
// 	}
// 	else if(cpid == 0){
// 		printf("fn_1 : %x\n", fn_1);
// 		printf("fn_2 : %x\n", fn_2);
// 		fn_1(0);
// 		fn_2(0);
// 		fn_1(4);
// 	}
// 	else{
// 		ret = set_breakpoint(fn_1);
// 		ret = set_breakpoint(fn_2);
		
// 		// fn_1 
// 		ret = wait_and_continue();
// 		printf("Breakpoint hit at : %x\n", ret);
// 		printf("BACKTRACE INFORMATION\n");
// 		bt_count = backtrace((void*)&bt_info);
		
// 		printf("Backtrace count: %d\n", bt_count);
// 		for(int i = 0; i < bt_count; i++)
// 		{
// 			printf("#%d %x\n", i, bt_info[i]);
// 		}

// 		// fn_2
// 		ret = wait_and_continue();
// 		printf("Breakpoint hit at : %x\n", ret);
// 		printf("BACKTRACE INFORMATION\n");
// 		bt_count = backtrace((void*)&bt_info);
		
// 		printf("Backtrace count: %d\n", bt_count);
// 		for(int i = 0; i < bt_count; i++)
// 		{
// 			printf("#%d %x\n", i, bt_info[i]);
// 		}

// 		// fn_1 4
// 		ret = wait_and_continue();
// 		printf("Breakpoint hit at : %x\n", ret);
// 		printf("BACKTRACE INFORMATION\n");
// 		bt_count = backtrace((void*)&bt_info);
		
// 		printf("Backtrace count: %d\n", bt_count);
// 		for(int i = 0; i < bt_count; i++)
// 		{
// 			printf("#%d %x\n", i, bt_info[i]);
// 		}

// 		// fn_2 3
// 		ret = wait_and_continue();
// 		printf("Breakpoint hit at : %x\n", ret);
// 		printf("BACKTRACE INFORMATION\n");
// 		bt_count = backtrace((void*)&bt_info);
		
// 		printf("Backtrace count: %d\n", bt_count);
// 		for(int i = 0; i < bt_count; i++)
// 		{
// 			printf("#%d %x\n", i, bt_info[i]);
// 		}

// 		// fn_1 2
// 		ret = wait_and_continue();
// 		printf("Breakpoint hit at : %x\n", ret);
// 		printf("BACKTRACE INFORMATION\n");
// 		bt_count = backtrace((void*)&bt_info);
		
// 		printf("Backtrace count: %d\n", bt_count);
// 		for(int i = 0; i < bt_count; i++)
// 		{
// 			printf("#%d %x\n", i, bt_info[i]);
// 		}

// 		// fn_2 1
// 		ret = wait_and_continue();
// 		printf("Breakpoint hit at : %x\n", ret);
// 		printf("BACKTRACE INFORMATION\n");
// 		bt_count = backtrace((void*)&bt_info);
		
// 		printf("Backtrace count: %d\n", bt_count);
// 		for(int i = 0; i < bt_count; i++)
// 		{
// 			printf("#%d %x\n", i, bt_info[i]);
// 		}

// 		// fn_1 0
// 		ret = wait_and_continue();
// 		printf("Breakpoint hit at : %x\n", ret);
// 		printf("BACKTRACE INFORMATION\n");
// 		bt_count = backtrace((void*)&bt_info);
		
// 		printf("Backtrace count: %d\n", bt_count);
// 		for(int i = 0; i < bt_count; i++)
// 		{
// 			printf("#%d %x\n", i, bt_info[i]);
// 		}
		
// 		// for exit
// 		ret = wait_and_continue();	
// 		printf("Child exit return : %x\n", ret);
// 	}
	
// 	return 0;
// }
