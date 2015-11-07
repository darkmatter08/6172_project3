#include "./allocator_interface.h"
#include "./stdint.h"

// from allocator.c
#ifndef ALIGNMENT
#define ALIGNMENT 8
#define BASESIZE 500
#define NUMBUCKETS 7
#endif
#define ALIGN(size) (((size) + (ALIGNMENT-1)) & ~(ALIGNMENT-1))
#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))
// endfrom


void bucket_0() {
	mem_reset_brk();
	my_init();
    void * ptr1 = my_malloc(30);
    assert(ptr1);
    my_free(ptr1);
    void * ptr2 = get_free_list_pointer(0);
    void * ptr3 = ((char *) ptr1) - SIZE_T_SIZE;
    assert(ptr2 == ptr3);
}

void bucket_1() {
	mem_reset_brk();
	my_init();
    void * ptr1 = my_malloc(600);
    assert(ptr1);
    my_free(ptr1);
    void * ptr2 = get_free_list_pointer(1);
    void * ptr3 = ((char *) ptr1) - SIZE_T_SIZE;
    assert(ptr2 == ptr3);
}

void use_free_list_0() {
	mem_reset_brk();
	my_init();
    void * ptr1 = my_malloc(600);
    assert(ptr1);
    my_free(ptr1);
    void * ptr2 = my_malloc(588);
    assert(ptr2 == ptr1);
}

#ifdef TEST
int main() {
	
	mem_init();
    bucket_0();
	mem_deinit();
	printf("PASSED: Bucket 0\n");
    mem_init();
    bucket_1();
	mem_deinit();
	printf("PASSED: Bucket 1\n");
    mem_init();
    use_free_list_0();
	mem_deinit();
	printf("PASSED: Use Free List 0\n");
	printf("ALL TESTS PASSED\n");
	return 1;
}
#endif
