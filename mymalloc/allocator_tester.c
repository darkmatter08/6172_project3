#include "./allocator_interface.h"

// from allocator.c
#ifndef ALIGNMENT
#define ALIGNMENT 8
#define SIZELIMIT 500
#endif
#define ALIGN(size) (((size) + (ALIGNMENT-1)) & ~(ALIGNMENT-1))
#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))
// endfrom

void big_join_test() {
	mem_reset_brk();
	my_init();

	// BEGIN TEST
	void* p1 = my_malloc(640);
	assert(p1);
	void* p2 = my_malloc(640);
	assert(p2);

	my_free(p1);
	assert(big_free_list);
	assert(big_free_list->size == 640);
	// TODO: write to end
	
	my_free(p2);
	// check p1 actually still exists.
	assert(big_free_list->next);
	assert(big_free_list->next->size == 640);
	// check p2 was actually added.
	assert(big_free_list);
	assert(big_free_list->size == 640);
	assert(small_free_list == NULL);
	// TODO: write to end

	join_blocks(big_free_list, big_free_list->next);
	assert(small_free_list == NULL);
	assert(big_free_list);
	assert(big_free_list->size > 1280);
	assert(big_free_list->size == 1280+SIZE_T_SIZE);
	assert(!(big_free_list->next));

	p2 = my_malloc(1280+SIZE_T_SIZE);
	// TODO: write to end
	// checked that we got from freed joined block
	assert(big_free_list == NULL);
	assert(small_free_list == NULL);

	my_free(p2);
	assert(big_free_list);
	assert(big_free_list->size == 1280+SIZE_T_SIZE);
	// mem_deinit();
}

void small_join_test() {
	mem_reset_brk();
	my_init();

	assert(!big_free_list); assert(!small_free_list);

	// BEGIN TEST
	void* p1 = my_malloc(100);
	assert(p1);
	void* p2 = my_malloc(100);
	assert(p2);

	my_free(p1);
	assert(small_free_list);
	assert(small_free_list->size == 100);
	// TODO: write to end

	my_free(p2);
	assert(small_free_list->next);
	assert(small_free_list->next->size == 100);
	assert(big_free_list == NULL);
	// TODO: write to end

	join_blocks(small_free_list, small_free_list->next);
	assert(big_free_list == NULL);
	assert(small_free_list);
	assert(small_free_list->size > 200);
	assert(small_free_list->size == 200+SIZE_T_SIZE);
	assert(!(small_free_list->next));

	p2 = my_malloc(200+SIZE_T_SIZE);
	// TODO: write to end
	// checked that we got from freed joined block
	assert(small_free_list == NULL);
	assert(big_free_list == NULL);

	my_free(p2);
	assert(small_free_list);
	assert(small_free_list->size == 200+SIZE_T_SIZE);
}

#ifdef TEST
int main() {
	mem_init();
	big_join_test();
	printf("PASSED: big_join_test\n");
	small_join_test();
	printf("PASSED: small_join_test\n");
	mem_deinit();
	printf("ALL TESTS PASSED\n");
	return 1;
}
#endif
