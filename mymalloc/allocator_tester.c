#include "./allocator_interface.h"

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

#ifdef TEST
int main() {
	mem_init();
	big_join_test();
	printf("PASSED: big_join_test\n");
	small_join_test();
	printf("PASSED: small_join_test\n");
	small_big_join_test();
	printf("PASSED: small_big_join_test\n");
	mem_deinit();
	printf("ALL TESTS PASSED\n");
	return 1;
}
#endif
