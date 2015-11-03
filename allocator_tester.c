#include "./allocator.c"

#ifdef TEST
int main() {
	my_init();
	void* p1 = my_malloc(640);
	void* p2 = my_malloc(640);

	my_free(p1);
	assert(big_free_list);
	assert(big_free_list->size == 640);
	
	my_free(p2);
	assert(big_free_list->next);
	assert(big_free_list->next->size == 640);

}
#endif