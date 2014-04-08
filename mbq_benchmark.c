#include <stdio.h>
#include <sys/time.h>

#include "mbq.h"

void run_test(char *test_title, struct mbq_accounting (*test_body)())
{
	struct timeval start_time;
	gettimeofday(&start_time, NULL);

	struct mbq_accounting tag = test_body();

	struct timeval end_time;
	gettimeofday(&end_time, NULL);

	struct timeval taken_time;
	taken_time.tv_sec = end_time.tv_sec - start_time.tv_sec;
	taken_time.tv_usec = end_time.tv_usec - start_time.tv_usec;

	printf("%ld, %s\n", 
			taken_time.tv_sec * 1000 * 1000 + taken_time.tv_usec, test_title);
	mbq_destroy(&tag);
	return;

}

#define TEST(x) struct mbq_accounting x()
#define LARGENUM (100 * 1000 * 1000)
#define test_type long long
#define MBQ_INTS \
	struct mbq_accounting tag; \
	test_type *array = mbq_init(&tag, sizeof(*array), LARGENUM);


TEST(test_write)
{
	MBQ_INTS;

	volatile test_type *iter = mbq_get_first_item((&tag), array);
	size_t len = tag.size;
	while (len--) {
		*iter++ = len;
	}
	return tag;
}

TEST(test_write_read)
{
	MBQ_INTS;

	volatile test_type *iter = mbq_get_first_item((&tag), array);
	size_t len = tag.size;
	while (len--) {
		*iter++ = len;
	}

	iter = mbq_get_first_item((&tag), array);
	len = tag.size;
	long long sum = 0;
	while (len--) {
		sum += *iter++;
	}
	fprintf(stderr, "Sum of writen = %lld\n", sum);
	return tag;
}

TEST(test_write_wipe_read)
{
	MBQ_INTS;

	// Write
	volatile test_type *iter = mbq_get_first_item((&tag), array);
	size_t len = tag.size;
	while (len--) {
		*iter++ = len;
	}

	// Wipe
	mbq_wipe_pages(&tag, tag.begin_index, tag.size);

	// Read
	iter = mbq_get_first_item((&tag), array);
	len = tag.size;
	long long sum = 0;
	while (len--) {
		sum += *iter++;
	}
	fprintf(stderr, "Sum of wiped = %lld\n", sum);
	return tag;
}

TEST(test_read)
{
	MBQ_INTS;

	volatile test_type *iter = mbq_get_first_item((&tag), array);
	size_t len = tag.size;
	long long sum = 0;
	while (len--) {
		sum += *iter++;
	}
	fprintf(stderr, "Sum of empty = %lld\n", sum);
	return tag;
}

TEST(test_alloc)
{
	MBQ_INTS;
	return tag;
}

TEST(test_alloc_wipe)
{
	MBQ_INTS;
	mbq_wipe_pages(&tag, tag.begin_index, tag.size);
	return tag;
}

int main(void)
{
	puts("Time elapsed in microseconds, Test title");
	run_test("Aloc", &test_alloc);
	run_test("Aloc then Wipe", &test_alloc_wipe);
	run_test("Read", &test_read);
	run_test("Write then Read", &test_write_read);
	run_test("Write, Wipe, Read", &test_write_wipe_read);
	run_test("Write", &test_write);
}
