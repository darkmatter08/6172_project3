#include "./allocator_interface.h"
#include "./stdint.h"

// from allocator.c
#ifndef ALIGNMENT
#define ALIGNMENT 8
#define SIZELIMIT 500
#endif
#define ALIGN(size) (((size) + (ALIGNMENT-1)) & ~(ALIGNMENT-1))
#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))
// endfrom

#define BIGCONST 640
#define SMALLCONST 100

void big_join_test() {
	mem_reset_brk();
	my_init();
	do_coalesce = 0;

	// BEGIN TEST
	void* p1 = my_malloc(BIGCONST);
	assert(p1);
	void* p2 = my_malloc(BIGCONST);
	assert(p2);

	my_free(p1);
	assert(big_free_list);
	assert(big_free_list->size == BIGCONST);
	// TODO: write to end
	
	my_free(p2);
	// check p1 actually still exists.
	assert(big_free_list->next);
	assert(big_free_list->next->size == BIGCONST);
	// check p2 was actually added.
	assert(big_free_list);
	assert(big_free_list->size == BIGCONST);
	assert(small_free_list == NULL);
	// TODO: write to end

	join_blocks(big_free_list, big_free_list->next);
	assert(small_free_list == NULL);
	assert(big_free_list);
	assert(big_free_list->size > 2*BIGCONST);
	assert(big_free_list->size == 2*BIGCONST+SIZE_T_SIZE);
	assert(!(big_free_list->next));

	p2 = my_malloc(2*BIGCONST+SIZE_T_SIZE);
	// TODO: write to end
	// checked that we got from freed joined block
	assert(big_free_list == NULL);
	assert(small_free_list == NULL);

	my_free(p2);
	assert(big_free_list);
	assert(big_free_list->size == 2*BIGCONST+SIZE_T_SIZE);
	// mem_deinit();
}

void small_join_test() {
	mem_reset_brk();
	my_init();
	do_coalesce = 0;

	assert(!big_free_list); assert(!small_free_list);

	// BEGIN TEST
	void* p1 = my_malloc(SMALLCONST);
	assert(p1);
	void* p2 = my_malloc(SMALLCONST);
	assert(p2);

	my_free(p1);
	assert(small_free_list);
	assert(small_free_list->size == ALIGN(SMALLCONST));
	// TODO: write to end

	my_free(p2);
	// check p1 actually still exists.
	assert(small_free_list->next);
	assert(small_free_list->next->size == ALIGN(SMALLCONST));
	// check p2 was actually added.
	assert(small_free_list);
	assert(small_free_list->size == ALIGN(SMALLCONST));
	assert(big_free_list == NULL);
	// TODO: write to end

	join_blocks(small_free_list, small_free_list->next);
	assert(big_free_list == NULL);
	assert(small_free_list);
	assert(small_free_list->size > ALIGN(2*SMALLCONST));
	assert(small_free_list->size == 2*ALIGN(SMALLCONST)+SIZE_T_SIZE);
	assert(!(small_free_list->next));

	p2 = my_malloc(2*SMALLCONST+SIZE_T_SIZE);
	// TODO: write to end
	// checked that we got from freed joined block
	assert(small_free_list == NULL);
	assert(big_free_list == NULL);

	my_free(p2);
	assert(small_free_list);
	assert(small_free_list->size == 2*ALIGN(SMALLCONST)+SIZE_T_SIZE);
}

void small_big_join_test() {
	mem_reset_brk();
	my_init();
	do_coalesce = 0;

	assert(!big_free_list); assert(!small_free_list);

	// BEGIN TEST
	void* p1 = my_malloc(SMALLCONST);
	assert(p1);
	void* p2 = my_malloc(BIGCONST);
	assert(p2);

	my_free(p1);
	assert(small_free_list);
	assert(small_free_list->size == ALIGN(SMALLCONST));
	// TODO: write to end

	my_free(p2);
	// check p1 actually still exists.
	assert(small_free_list);
	assert(small_free_list->size == ALIGN(SMALLCONST));
	assert(small_free_list->next == NULL);
	// check p2 was actually added.
	assert(big_free_list);
	assert(big_free_list->size == BIGCONST);
	assert(big_free_list->next == NULL);
	// TODO: write to end

	join_blocks(small_free_list, big_free_list);
	assert(small_free_list == NULL);
	assert(big_free_list);
	assert(big_free_list->size > ALIGN(SMALLCONST+BIGCONST));
	assert(big_free_list->size == ALIGN(SMALLCONST)+BIGCONST+SIZE_T_SIZE);
	assert(!(big_free_list->next));

	p2 = my_malloc(SMALLCONST+BIGCONST+SIZE_T_SIZE);
	// TODO: write to end
	// checked that we got from freed joined block
	assert(small_free_list == NULL);
	assert(big_free_list == NULL); // fails because of alignment differences.

	my_free(p2);
	assert(big_free_list);
	assert(big_free_list->size == ALIGN(SMALLCONST)+BIGCONST+SIZE_T_SIZE);
}

void footer_test() {
	mem_reset_brk();
	my_init();
	do_coalesce = 0;

	assert(!big_free_list); assert(!small_free_list);

	// BEGIN TEST
	void* p1 = my_malloc(SMALLCONST);
	assert(p1);
	// check footer is zeroed.
	size_t p1_size = *(size_t*)(((uint8_t*) p1));
	assert(*(size_t*)((uint8_t*) p1 + p1_size) == 0);

	void* p2 = my_malloc(BIGCONST);
	assert(p2);
	// check footer is zeroed.
	size_t p2_size = *(size_t*)(((uint8_t*) p2));
	assert(*(size_t*)((uint8_t*) p2 + p2_size) == 0);

	my_free(p1);
	// Assume p1 in the small_free_list
	// check the footer is set to the size in the free list
	// since we are starting from the small_free_list, we must add
	// SIZE_T_SIZE to jump to the same point as p1.
	assert(p1 == (uint8_t*) small_free_list + SIZE_T_SIZE);

	assert(*(size_t*)((uint8_t*) small_free_list + (small_free_list->size) + SIZE_T_SIZE) == small_free_list->size);
	
	my_free(p2);
	// Assume p2 in the big_free_list
	// check the footer is set to the size in the free list
	assert(p2 == (uint8_t*) big_free_list + SIZE_T_SIZE);
	assert(*(size_t*)((uint8_t*) big_free_list + (big_free_list->size) + SIZE_T_SIZE) == big_free_list->size);
}

void coalesce_test_previous() {
	mem_reset_brk();
	my_init();
	do_coalesce = 1;

	assert(!big_free_list); assert(!small_free_list);

	// BEGIN TEST
	void* p1 = my_malloc(SMALLCONST);
	assert(p1);

	void* p2 = my_malloc(BIGCONST);
	assert(p2);

	// checks only prev memory case, not next memory case.
	my_free(p1);
	my_free(p2);

	// check for auto-joining.
	assert(small_free_list == NULL);
	assert(big_free_list);
	assert(big_free_list->size > ALIGN(SMALLCONST+BIGCONST));
	assert(big_free_list->size == ALIGN(SMALLCONST)+BIGCONST+SIZE_T_SIZE); // don't include footer size
	assert(!(big_free_list->next));

	p2 = my_malloc(SMALLCONST+BIGCONST+SIZE_T_SIZE);
	// checked that we got from freed joined block
	assert(small_free_list == NULL);
	assert(big_free_list == NULL);
	// TODO: check size from footer?

	my_free(p2);
	// check block is returned to big_free_list
	assert(big_free_list);
	assert(big_free_list->size == ALIGN(SMALLCONST)+BIGCONST+SIZE_T_SIZE);
}

// not actually testing forward coalescing.
void coalesce_test_forward() {
	mem_reset_brk();
	my_init();
	do_coalesce = 1;

	assert(!big_free_list); assert(!small_free_list);

	// BEGIN TEST
	void* p1 = my_malloc(SMALLCONST);
	assert(p1);

	void* p2 = my_malloc(BIGCONST);
	assert(p2);

	// checks only next memory case, not prev memory case.
	my_free(p2); // for some reason, doesn't get added to small_free_list
	assert(big_free_list);
	my_free(p1);

	// check for auto-joining.
	assert(small_free_list == NULL);
	assert(big_free_list);
	assert(big_free_list->size > ALIGN(SMALLCONST+BIGCONST));
	assert(big_free_list->size == ALIGN(SMALLCONST)+BIGCONST+SIZE_T_SIZE); // don't include footer size
	assert(!(big_free_list->next));

	p2 = my_malloc(SMALLCONST+BIGCONST+SIZE_T_SIZE);
	// checked that we got from freed joined block
	assert(small_free_list == NULL);
	assert(big_free_list == NULL);
	// TODO: check size from footer?

	my_free(p2);
	// check block is returned to big_free_list
	assert(big_free_list);
	assert(big_free_list->size == ALIGN(SMALLCONST)+BIGCONST+SIZE_T_SIZE);
}

void coalesce_test_forward_real() {
	mem_reset_brk();
	my_init();
	do_coalesce = 1;

	assert(!big_free_list); assert(!small_free_list);

	// BEGIN TEST
	void* p1 = my_malloc(SMALLCONST);
	assert(p1);

	void* p2 = my_malloc(BIGCONST);
	assert(p2);

	void* p3 = my_malloc(BIGCONST);
	assert(p3);

	my_free(p3);
	my_free(p2);
	// check foward
	// my_free();
}

#ifdef TEST
int main() {
	
	mem_init();
	big_join_test();
	mem_deinit();
	printf("PASSED: big_join_test\n");
	
	mem_init();
	small_join_test();
	mem_deinit();
	printf("PASSED: small_join_test\n");
	
	mem_init();
	small_big_join_test();
	mem_deinit();
	printf("PASSED: small_big_join_test\n");

	mem_init();
	footer_test();
	mem_deinit();
	printf("PASSED: footer_test\n");

	mem_init();
	coalesce_test_previous();
	mem_deinit();
	printf("PASSED: coalesce_test_previous\n");

	mem_init();
	coalesce_test_forward();
	mem_deinit();
	printf("PASSED: coalesce_test_forward\n");

	// mem_init();
	// coalesce_test_forward_real();
	// mem_deinit();
	// printf("PASSED: coalesce_test_forward_real\n");
	
	printf("ALL TESTS PASSED\n");
	return 1;
}
#endif
