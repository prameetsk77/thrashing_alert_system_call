#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/sched.h>
#include <linux/tty.h>
#include <linux/slab.h> 
#include <linux/string.h>
#include <linux/list.h>
#include <linux/delay.h>
#include <linux/kthread.h>
#include <linux/spinlock.h>  
#include <linux/moduleparam.h>
#include <linux/mm.h>
#include <stdbool.h>
#include <linux/highmem.h>
#include <linux/delay.h>
#include <linux/kthread.h>

#define DEBUG 0

struct task_struct *kernelThread = NULL;

int translate_n_check_access_bit(unsigned long add_user, struct mm_struct *mm_task){

	pgd_t *pgd_entry;
	pud_t *pud_entry;
	pmd_t *pmd_entry;
	pte_t *pte_entry;

	//pte_t vfn;

	spin_lock(&mm_task->page_table_lock);
	//get pgd from mm_struct 
	pgd_entry = pgd_offset(mm_task,add_user);

	if(DEBUG)
		printk(KERN_ALERT "PGD FOUND %llu\n",(unsigned long long)pgd_val(*pgd_entry));
	if(pgd_none(*pgd_entry) || pgd_bad (*pgd_entry)){
		if(DEBUG)printk(KERN_ALERT "PUD is NULL\n");
		spin_unlock(&mm_task->page_table_lock);
		return 0;
	}

	pud_entry = pud_offset(pgd_entry,add_user);
	if(pud_none(*pud_entry) || pud_bad (*pud_entry)){	
		if(DEBUG)printk(KERN_ALERT "PUD is NULL\n");
		spin_unlock(&mm_task->page_table_lock);
		return 0;
	}
	if(DEBUG)
		printk(KERN_ALERT "PUD FOUND %llu\n",(unsigned long long)pud_val(*pud_entry));

	pmd_entry = pmd_offset(pud_entry,add_user);
	if(pmd_none(*pmd_entry) || pmd_bad (*pmd_entry))
	{	
		if(DEBUG)printk(KERN_ALERT "PMD is NULL\n");
		spin_unlock(&mm_task->page_table_lock);
		return 0;
	}

	if(DEBUG)
		printk(KERN_ALERT "PMD FOUND %llu\n",(unsigned long long)pmd_val(*pmd_entry));
	
	
	pte_entry = pte_offset_map(pmd_entry,add_user);
	
	if(pte_none(*pte_entry) || !pte_entry ){
		if(DEBUG)
			printk(KERN_ALERT "PTE is NULL\n");
		pte_unmap(pte_entry);
		spin_unlock(&mm_task->page_table_lock);	
		return 0;
	}
	
	if(pte_present(*pte_entry)){
		//vfn = ((unsigned long long)pte_val(*pte_entry));

		if(pte_young(*pte_entry)){
			pte_mkold(*pte_entry);
			pte_unmap(pte_entry);
			spin_unlock(&mm_task->page_table_lock);
			return 1;
		}
		else{
			pte_mkold(*pte_entry);
			pte_unmap(pte_entry);
			spin_unlock(&mm_task->page_table_lock);
			return 0;
		}
	}
	else{
		pte_unmap(pte_entry);
		if(DEBUG)
			printk(KERN_ALERT "Swapped\n");
		// add code to check access bit of swap page here //
		spin_unlock(&mm_task->page_table_lock);
		return 0;
	}
	
	pte_unmap(pte_entry);
	spin_unlock(&mm_task->page_table_lock);
	return 0;
}

int getwss_count(unsigned long start,unsigned long end, struct mm_struct *mm_task){
	int count=0;
	unsigned long i;
	for(i=start; i<end; i=i+4096){
		if(translate_n_check_access_bit(i, mm_task))
			count++;
	}
	return count;
}

static int memmap(void* null){

	struct task_struct *task;
	struct mm_struct *mm_task=NULL;

	struct vm_area_struct* vm_list ;
	unsigned long long wss;
	unsigned long long  twss;
	int vm_count;
	rwlock_t rd_lock;
	rwlock_init(&rd_lock);
    printk(KERN_ALERT "\n\n--------Function Starts Here-----\n");
	
	while(!kthread_should_stop()){
		twss=0;
		
		read_lock(&rd_lock);
		for_each_process(task){
			mm_task = task->mm;	
			
			if(mm_task == NULL){
				if(DEBUG)printk(KERN_INFO "mm_task is null for pid %d with name %s \n", task->pid, task->comm);
				continue;
			}
			
			wss =0;
			vm_list = mm_task->mmap;
			if(vm_list == NULL){
				if(DEBUG)
					printk(KERN_INFO "vm_list head is null for pid %d with name %s \n", task->pid, task->comm);
				continue;
			}

			vm_count = 0;
			while(vm_list != NULL){
				unsigned long start = vm_list->vm_start;
				unsigned long end = vm_list->vm_end;
				if(DEBUG)printk(KERN_INFO "vm_size: %lu\n", end - start);

				wss += getwss_count(start,end, mm_task);

				vm_list = vm_list->vm_next;
				vm_count++;
			}
			if(DEBUG)printk(KERN_INFO "pid: %d with name %s \t wss count: %llu \t vm_count: %d\n", task->pid, task->comm, wss, vm_count);
			twss += wss;
		}
		read_unlock(&rd_lock);

		printk(KERN_INFO "twss %llu\n", twss);
		msleep(1 * 1000);
	}
	return 0;
}

static int __init init_func(void){

    //memmap();
    kernelThread = kthread_run(&memmap, NULL, "Project 3 Part 2 Thread");
    return 0;   
}

static void __exit exit_func(void){
	kthread_stop(kernelThread);
    return;
}

module_init(init_func);
module_exit(exit_func);


MODULE_LICENSE("GPL");
MODULE_AUTHOR("Amsal Naeem (amsal.naeem@asu.edu)");
MODULE_DESCRIPTION("OS Project 3 step_1");




/////////////////////////////////////////////////////////////////////////////////////////////////
/*
	pgd_t *pgd_entry;
	pud_t *pud_entry;
	pmd_t *pmd_entry;
	pte_t *pte_entry;

	unsigned long vfn;

	int swap;

	pgd_entry = pgd_offset(mm_task,add_user);
	printk(KERN_ALERT "PGD FOUND %ld\n",(unsigned long)pgd_val(*pgd_entry));

	pud_entry = pud_offset(pgd_entry,add_user);
	if(pud_none(*pud_entry) || pud_bad(*pud_entry))
	{	printk(KERN_ALERT "PUD is NONE\n");
		return 0;
	}
	printk(KERN_ALERT "PUD FOUND %ld\n",(unsigned long)pud_val(*pud_entry));

	pmd_entry = pmd_offset(pud_entry,add_user);
	if(pmd_none(*pmd_entry) || pmd_bad(*pmd_entry))
	{	printk(KERN_ALERT "PMD is NULL OR BAD\n");
		return 0;
	}

	printk(KERN_ALERT "PMD FOUND %ld\n",(unsigned long)pmd_val(*pmd_entry));

    swap = 0;

	pte_entry = pte_offset_kernel(pmd_entry,add_user);

	if(!pte_present(*pte_entry)){
		if(pte_none(*pte_entry) || pte_entry == NULL){
			printk(KERN_ALERT "PTE is NULL\n");
			return 0;
		}
		swap = 1;	
	}
	
	if(swap)
		printk(KERN_ALERT "Swapped\n");
	else{
		printk(KERN_ALERT "Not Swapped\n");
		
		vfn = pte_val(*pte_entry);
		
		printk(KERN_ALERT "PTE VALUE %ld\n",vfn);
	    printk(KERN_ALERT "TRANSLATION:\n"); 
		printk(KERN_ALERT "Virtual Address: 0x%lx,	VFN: %ld,	 PFN: %ld,	 phys_addr: 0x%lx \n", add_user & 0xFFFFF000, add_user>>12, vfn, vfn); 

		if(pte_young(vfn)){
			pte_unmap(pte_entry);
			return 1;
		}
		else{
			pte_unmap(pte_entry);
			return 0;
		}
		pte_mkold(vfn);
	}
	pte_unmap(pte_entry);
	return 0;

	*/