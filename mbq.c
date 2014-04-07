#define _GNU_SOURCE
#include <sys/mman.h>
#include <unistd.h>
#include <assert.h>

#include "mbq.h"

static size_t MBQ_PAGESIZE = 0;

void *mbq_init(struct mbq_accounting *accounts, size_t item_size, size_t item_count)
{
	MBQ_PAGESIZE = sysconf(_SC_PAGESIZE);
	assert(item_size < MBQ_PAGESIZE);

	// Bulk up item_count until we fill whole pages
	size_t items_per_page = MBQ_PAGESIZE / item_size;
	item_count = ((item_count / items_per_page) + 1) * items_per_page;

	accounts->item_size = item_size;
	accounts->begin_index = 0;
	accounts->size = item_count;

	// Note: threshhold must be page sized
	// Default: One page worth
	// We only mremem() after N or more item deletions
	accounts->stale_threshhold = (MBQ_PAGESIZE / item_size) + 1;
	accounts->stale_item_count = 0;

	// Note: Non-portable arguments to mmap
	void *mapped_array = mmap(0, item_size * item_count,
	                          PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

	if (MAP_FAILED == mapped_array) {
		// Error: No room in virtual memory space
		accounts->array = NULL;
		return NULL;
	}

	accounts->array = mapped_array;
	return mapped_array;
}

void *mbq_expand(struct mbq_accounting *accounts, size_t extra_item_capacity)
{
	assert(MBQ_PAGESIZE != 0);

	size_t data_size = (accounts->stale_item_count
	                    + accounts->size
	                    - accounts->begin_index) * accounts->item_size;
	size_t extra_size = extra_item_capacity * accounts->item_size;

	void *new_map = mremap(accounts->array, data_size, data_size + extra_size, MREMAP_MAYMOVE);
	if (MAP_FAILED == new_map) {
		// Error: No room in virtual memory space
		accounts->array = NULL;
		return NULL;
	}

	accounts->array = new_map;
	return new_map;
}

size_t mbq_wipe_pages(struct mbq_accounting *accounts, size_t start_index, size_t dead_items)
{
	assert(MBQ_PAGESIZE != 0);

	assert(start_index >= accounts->begin_index);
	assert(start_index + dead_items <= accounts->size);

	// NOTE: We must find a subset of dead items which
	// fit in whole pages only.
	void *start_addr = accounts->array +
	                    (accounts->begin_index - start_index) * accounts->item_size;
	void *end_addr = start_addr + dead_items * accounts->item_size;
	size_t dead_mem = end_addr - start_addr;
	size_t skipped_mem = MBQ_PAGESIZE % (size_t)start_addr;
	void *aligned_start = start_addr + skipped_mem;
	if (aligned_start >= end_addr) {
		return 0;
	}

	void *aligned_end = (void *)(((size_t)end_addr / MBQ_PAGESIZE) * MBQ_PAGESIZE);
	if (aligned_end <= aligned_start) {
		return 0;
	}

	// We drop the pages and address space
	size_t aligned_dead_mem = aligned_end - aligned_start;
	int status = munmap(aligned_start, aligned_dead_mem);
	assert(status != -1);

	// Now we take the address space back
	// Note: This exploits Zero-Fill-On-Demand
	void *address = mmap(aligned_start, aligned_dead_mem,
	                          PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
	assert(address != MAP_FAILED);


	size_t items_worth_dropped = aligned_dead_mem / accounts->item_size;
	return items_worth_dropped;
}

void mbq_delete_head(struct mbq_accounting *accounts, size_t items_to_delete)
{
	assert(MBQ_PAGESIZE != 0);

	size_t stale_count = accounts->stale_item_count += items_to_delete;
	if (stale_count < accounts->stale_threshhold) {
		return;
	}

	// Note: We assume ITEM_SIZE is less than PAGESIZE
	size_t items_per_page = MBQ_PAGESIZE / accounts->item_size;

	// Note: munmap works on whole pages
	size_t spare_stales = accounts->stale_item_count = stale_count % items_per_page;
	size_t items_to_del = stale_count - spare_stales;
	size_t bytes_to_del = items_to_del * accounts->item_size;
	assert(bytes_to_del % MBQ_PAGESIZE == 0);

	if (bytes_to_del == 0) {
		// Stales do not fill atleast one page
		return;
	}

	int status = munmap(accounts->array, bytes_to_del);
	assert(status != -1); // Note: Should only fail if address was unaligned

	accounts->array += bytes_to_del;
	accounts->begin_index += items_to_del;
	return;
}


