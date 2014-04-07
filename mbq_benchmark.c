#include <stdio.h>
#include <sys/time.h>

#include "mbq.h"

void run_test(char *test_title, void (*test_body)())
{
	struct timeval start_time;
	gettimeofday(&start_time, NULL);

	test_body();

	struct timeval end_time;
	gettimeofday(&end_time, NULL);

	struct timeval taken_time;
	taken_time.tv_sec = end_time.tv_sec - start_time.tv_sec;
	taken_time.tv_usec = end_time.tv_usec - start_time.tv_usec;

	printf("%ld, %s\n", 
			taken_time.tv_sec * 1000 * 1000 + taken_time.tv_usec, test_title);
	return;

}

#define LARGENUM (1000 * 1000 * 1000)
#define MBQ_INTS \
	struct mbq_accounting ints_tag; \
	int *ints = mbq_init(&ints_tag, sizeof(*ints), LARGENUM);


void test_write()
{
	MBQ_INTS;

	volatile int *ints_iter = mbq_get_first_item((&ints_tag), ints);
	size_t len = ints_tag.size;
	while (len--) {
		*ints_iter++ = len;
	}
}

void test_write_read()
{
	MBQ_INTS;

	int *ints_iter = mbq_get_first_item((&ints_tag), ints);
	size_t len = ints_tag.size;
	while (len--) {
		*ints_iter++ = len;
	}

	ints_iter = mbq_get_first_item((&ints_tag), ints);
	len = ints_tag.size;
	long long sum = 0;
	while (len--) {
		sum += *ints_iter++;
	}
	fprintf(stderr, "Sum of writen = %lld\n", sum);
}

void test_write_wipe_read()
{
	MBQ_INTS;

	// Write
	int *ints_iter = mbq_get_first_item((&ints_tag), ints);
	size_t len = ints_tag.size;
	while (len--) {
		*ints_iter++ = len;
	}

	// Wipe
	mbq_wipe_pages(&ints_tag, ints_tag.begin_index, ints_tag.size);

	// Read
	ints_iter = mbq_get_first_item((&ints_tag), ints);
	len = ints_tag.size;
	long long sum = 0;
	while (len--) {
		sum += *ints_iter++;
	}
	fprintf(stderr, "Sum of wiped = %lld\n", sum);
}

void test_read()
{
	MBQ_INTS;

	int *ints_iter = mbq_get_first_item((&ints_tag), ints);
	size_t len = ints_tag.size;
	long long sum = 0;
	while (len--) {
		sum += *ints_iter++;
	}
	fprintf(stderr, "Sum of empty = %lld\n", sum);
}

void test_alloc()
{
	MBQ_INTS;
}

void test_alloc_wipe()
{
	MBQ_INTS;
	mbq_wipe_pages(&ints_tag, ints_tag.begin_index, ints_tag.size);
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
