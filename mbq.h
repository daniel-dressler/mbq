#pragma once

struct mbq_accounting {
	size_t stale_threshhold;
	int stale_item_count;

	int begin_index;
	size_t size;

	size_t item_size;
	void *array;
};

void *mbq_init(struct mbq_accounting *accounts, size_t item_size, size_t item_count);
void *mbq_expand(struct mbq_accounting *accounts, size_t extra_item_capacity);
size_t mbq_drop_pages(struct mbq_accounting *accounts, size_t start_index, size_t dead_items);
void mbq_delete_head(struct mbq_accounting *accounts, size_t items_to_delete);

// Note: ARRAY must not be void *
// the array[0] ensures this.
// Note: The , operator in C
// is not common but is useful.
#define mbq_get_first_item(accounting, array) \
	(array[0], array + accounting->stale_item_count)

#define mbq_get_item_from_index(accounting, array, index) \
	(array[0], array + (accounting->stale_item_count + index - account->begin_index))
