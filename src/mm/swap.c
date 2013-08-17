/*
 *  linux/mm/swap.c
 *
 *  (C) 1991  Linus Torvalds
 */

/*
 * This file should contain most things doing the swapping from/to disk.
 * Started 18.12.91
 */

#include <string.h>
#include <errno.h>

#include <linux/mm.h>
#include <sys/stat.h>
#include <linux/sched.h>
#include <linux/head.h>
#include <linux/kernel.h>

#define SWAP_BITS (4096<<3)

#define bitop(name,op) \
static inline int name(char * addr,unsigned int nr) \
{ \
	int __res; \
	__asm__ __volatile__("bt" op " %1,%2; adcl $0,%0" \
		:"=g" (__res) \
		:"r" (nr),"m" (*(addr)),"0" (0)); \
	return __res; \
}

bitop(bit,"")
bitop(setbit,"s")
bitop(clrbit,"r")

static char * swap_bitmap = NULL;
unsigned int swap_device = 0;
struct m_inode * swap_file = NULL;
int SWAP_DEV = 0;
void rw_swap_page(int rw, unsigned int nr, char * buf)
{
	unsigned int zones[4];
	int i;

	if (swap_device) {
		ll_rw_page(rw,swap_device,nr,buf);
		return;
	}
	if (swap_file) {
		nr <<= 2;
		for (i = 0; i < 4; i++)
			if (!(zones[i] = bmap(swap_file,nr++))) {
				printk("rw_swap_page: bad swap file\n");
				return;
			}
		ll_rw_swap_file(rw,swap_file->i_dev, zones,4,buf);
		return;
	}
	printk("ll_swap_page: no swap file or device\n");
}

/*
 * We never page the pages in task[0] - kernel memory.
 * We page all other pages.
 */
#define FIRST_VM_PAGE (TASK_SIZE>>12)
#define LAST_VM_PAGE (1024*1024)
#define VM_PAGES (LAST_VM_PAGE - FIRST_VM_PAGE)

static int get_swap_page(void)
{
	int nr;

	if (!swap_bitmap)
		return 0;
	for (nr = 1; nr < SWAP_BITS ; nr++)
		if (clrbit(swap_bitmap,nr))
			return nr;
	return 0;
}

void swap_free(int swap_nr)
{
	if (!swap_nr)
		return;
	if (swap_bitmap && swap_nr < SWAP_BITS)
		if (!setbit(swap_bitmap,swap_nr))
			return;
	printk("swap_free: swap-space bitmap bad\n");
	return;
}

void swap_in(unsigned long *table_ptr)
{
	int swap_nr;
	unsigned long page;

	if (!swap_bitmap) {
		printk("Trying to swap in without swap bit-map");
		return;
	}
	if (1 & *table_ptr) {
		printk("trying to swap in present page\n\r");
		return;
	}
	swap_nr = *table_ptr >> 1;
	if (!swap_nr) {
		printk("No swap page in swap_in\n\r");
		return;
	}
	if (!(page = get_free_page()))
		oom();
	read_swap_page(swap_nr, (char *) page);
	if (setbit(swap_bitmap,swap_nr))
		printk("swapping in multiply from same page\n\r");
	*table_ptr = page | (PAGE_DIRTY | 7);
}

int try_to_swap_out(unsigned long * table_ptr)
{
	unsigned long page;
	unsigned long swap_nr;

	page = *table_ptr;
	if (!(PAGE_PRESENT & page))
		return 0;
	if (page - LOW_MEM > PAGING_MEMORY)
		return 0;
	if (PAGE_DIRTY & page) {
		page &= 0xfffff000;
		if (mem_map[MAP_NR(page)] != 1)
			return 0;
		if (!(swap_nr = get_swap_page()))
			return 0;
		*table_ptr = swap_nr<<1;
		invalidate();
		write_swap_page(swap_nr, (char *) page);
		free_page(page);
		return 1;
	}
	*table_ptr = 0;
	invalidate();
	free_page(page);
	return 1;
}

/*
 * Go through the page tables, searching for a user page that
 * we can swap out.
 */
int swap_out(void)
{
	static int dir_entry = FIRST_VM_PAGE>>10;
	static int page_entry = -1;
	int counter = VM_PAGES;
	int pg_table;

	while (counter>0) {
		pg_table = pg_dir[dir_entry];
		if (pg_table & 1)
			break;
		counter -= 1024;
		dir_entry++;
		if (dir_entry >= 1024)
			dir_entry = FIRST_VM_PAGE>>10;
	}
	pg_table &= 0xfffff000;
	while (counter-- > 0) {
		page_entry++;
		if (page_entry >= 1024) {
			page_entry = 0;
		repeat:
			dir_entry++;
			if (dir_entry >= 1024)
				dir_entry = FIRST_VM_PAGE>>10;
			pg_table = pg_dir[dir_entry];
			if (!(pg_table&1))
				if ((counter -= 1024) > 0)
					goto repeat;
				else
					break;
			pg_table &= 0xfffff000;
		}
		if (try_to_swap_out(page_entry + (unsigned long *) pg_table))
			return 1;
	}
	printk("Out of swap-memory\n\r");
	return 0;
}

/*
 * Get physical address of first (actually last :-) free page, and mark it
 * used. If no free pages left, return 0.
 */
unsigned long get_free_page(void)
{
	register unsigned long __res asm("ax");

	do {
		__asm__(
			"std\n\t"
			"repnz scasb\n\t"
			"jne 1f\n\t"
			"movb $1, 1(%%edi)\n\t"
			"sal $12, %%ecx\n\t"
			"add %2, %%ecx\n\t"
			"mov %%ecx, %%edx\n\t"
			"mov $1024, %%ecx\n\t"
			"mov %%edx, %%edi\n\t"
			"cld\n\t"
			"rep stosl\n\t"
			"mov %%edx,%%eax\n"
			"1:"
			: "=a" (__res)
			: "0"(0), "i"(LOW_MEM), "c"(PAGING_PAGES), "D"(mem_map+PAGING_PAGES-1));
	} while (__res >= HIGH_MEMORY || !__res&&swap_out());
	return __res;
}

void init_swapping(void)
{
	extern int *blk_size[];
	int swap_size,i,j;

	if (!SWAP_DEV)
		return;
	if (!blk_size[MAJOR(SWAP_DEV)]) {
		printk("Unable to get size of swap device\n\r");
		return;
	}
	swap_size = blk_size[MAJOR(SWAP_DEV)][MINOR(SWAP_DEV)];
	if (!swap_size)
		return;
	if (swap_size < 100) {
		printk("Swap device too small (%d blocks)\n\r",swap_size);
		return;
	}
	swap_size >>= 2;
	if (swap_size > SWAP_BITS)
		swap_size = SWAP_BITS;
	swap_bitmap = (char *) get_free_page();
	if (!swap_bitmap) {
		printk("Unable to start swapping: out of memory :-)\n\r");
		return;
	}
	read_swap_page(0,swap_bitmap);
	if (strncmp("SWAP-SPACE",swap_bitmap+4086,10)) {
		printk("Unable to find swap-space signature\n\r");
		free_page((long) swap_bitmap);
		swap_bitmap = NULL;
		return;
	}
	memset(swap_bitmap+4086,0,10);
	for (i = 0 ; i < SWAP_BITS ; i++) {
		if (i == 1)
			i = swap_size;
		if (bit(swap_bitmap,i)) {
			printk("Bad swap-space bit-map\n\r");
			free_page((long) swap_bitmap);
			swap_bitmap = NULL;
			return;
		}
	}
	j = 0;
	for (i = 1 ; i < swap_size ; i++)
		if (bit(swap_bitmap,i))
			j++;
	if (!j) {
		free_page((long) swap_bitmap);
		swap_bitmap = NULL;
		return;
	}
	printk("Swap device ok: %d pages (%d bytes) swap-space\n\r",j,j*4096);
}
