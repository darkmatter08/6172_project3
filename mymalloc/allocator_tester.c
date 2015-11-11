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

// helper function for tests that checks that coalescing works optimally in certain cases
void assert_count(int check_total_size) {
	int count = 0;
	for (int i = 0; i < NUMBUCKETS; i++) {
		if (free_list_array[i]){
			count++;
			if (check_total_size)
				assert(free_list_array[i]->size >= 114604);
		}
	}
	assert(count == 1);
}

// runs part of trace c7
void c7_test() {
	mem_reset_brk();
	my_init();


	void * p0 =  my_malloc(2150);
	void * p1 =  my_malloc(3843);
	void * p2 =  my_malloc(4331);
	void * p3 =  my_malloc(894);
	void * p4 =  my_malloc(8359);
	void * p5 =  my_malloc(7788);
	void * p6 =  my_malloc(33252);
	void * p7 =  my_malloc(902);
	void * p8 =  my_malloc(16679);
	void * p9 =  my_malloc(677);
	void * p10 = my_malloc(33196);
	void * p11 = my_malloc(870);
	void * p12 = my_malloc(1012);
	void * p13 = my_malloc(651);
	void * heap_top = my_heap_hi();
	printf("my_heap_hi: %p\n", heap_top);

	my_free(p0); assert_count(0);
	my_free(p1); assert_count(0);
	my_free(p2); assert_count(0);
	my_free(p3); assert_count(0);
	my_free(p4); assert_count(0);
	my_free(p5); assert_count(0);
	my_free(p6); assert_count(0);
	my_free(p7); assert_count(0);
	my_free(p8); assert_count(0);
	my_free(p9); assert_count(0);
	my_free(p10); assert_count(0);
	my_free(p11); assert_count(0);
	my_free(p12); assert_count(0);
	my_free(p13); assert_count(0);
	// only one bucket is occupied, size >= 114604
	assert_count(1);

	void * p14 = my_malloc(17356);
	void * p15 = my_malloc(5993);
	void * p16 = my_malloc(5225);
	void * p17 = my_malloc(34066);
	void * p18 = my_malloc(16147);
	void * p19 = my_malloc(1663);
	void * p20 = my_malloc(34154);
	// all free lists are empty.
	printf("my_heap_hi: %p, heap_top: %p\n", my_heap_hi(), heap_top);
	assert(my_heap_hi() == heap_top);
}

// runs part of trace c7
void c7_test_2() {
	mem_reset_brk();
	my_init();


	void * p0 =  my_malloc(2150);
	void * p1 =  my_malloc(3843);
	void * p2 =  my_malloc(4331);
	void * p3 =  my_malloc(894);
	void * p4 =  my_malloc(8359);
	void * p5 =  my_malloc(7788);
	void * p6 =  my_malloc(33252);
	void * p7 =  my_malloc(902);
	void * p8 =  my_malloc(16679);
	void * p9 =  my_malloc(677);
	void * p10 = my_malloc(33196);
	void * p11 = my_malloc(870);
	void * p12 = my_malloc(1012);
	void * p13 = my_malloc(651);
	my_free(p0); assert_count(0);
	my_free(p1); assert_count(0);
	my_free(p2); assert_count(0);
	my_free(p3); assert_count(0);
	my_free(p4); assert_count(0);
	my_free(p5); assert_count(0);
	my_free(p6); assert_count(0);
	my_free(p7); assert_count(0);
	my_free(p8); assert_count(0);
	my_free(p9); assert_count(0);
	my_free(p10); assert_count(0);
	my_free(p11); assert_count(0);
	my_free(p12); assert_count(0);
	my_free(p13); assert_count(0);
	void * heap_top = my_heap_hi();
	// only one bucket is occupied, size >= 114604
	assert_count(1);

	void * p14 = my_malloc(17356);
	void * p15 = my_malloc(5993);
	void * p16 = my_malloc(5225);
	void * p17 = my_malloc(34066);
	void * p18 = my_malloc(16147);
	void * p19 = my_malloc(1663);
	void * p20 = my_malloc(34154);
	// TODO: check all free lists are empty.
	assert(my_heap_hi() == heap_top);

	my_free(p14);
	my_free(p15);
	my_free(p16);
	my_free(p17);
	my_free(p18);
	my_free(p19);
	my_free(p20);
	// only one bucket is occupied, size >= 114604
	assert_count(1);

	void * p21 = my_malloc(4569);
	void * p22 = my_malloc(673);
	void * p23 = my_malloc(8275);
	void * p24 = my_malloc(672);
	// allocated 14,189
	assert(my_heap_hi() == heap_top);
	
	my_free(p21);
	my_free(p22);
	my_free(p23);
	my_free(p24);
	// all allocated memory should be free
	assert_count(1);

	void * p25 = my_malloc(8947);
	void * p26 = my_malloc(5242);
	assert(my_heap_hi() == heap_top);

	my_free(p25);
	my_free(p26);
	// all allocated memory should be free
	assert_count(1);
	assert(my_heap_hi() == heap_top);
}

// runs part of trace c7
void c7_test_3() {
    c7_test_2();

	void * p27 = my_malloc(4148);
	void * p28 = my_malloc(7890);
	void * p29 = my_malloc(4270);
	void * p30 = my_malloc(15962);
	void * p31 = my_malloc(4142);
	void * p32 = my_malloc(946);
	void * p33 = my_malloc(844);
	void * p34 = my_malloc(3991);
	void * p35 = my_malloc(33091);
	void * p36 = my_malloc(15965);

	my_free(p27);
	my_free(p28);
	my_free(p29);
	my_free(p30);
	my_free(p31);
	my_free(p32);
	my_free(p33);
	my_free(p34);
	my_free(p35);
	my_free(p36);
	assert_count(0);

	void * p37 = my_malloc(20232);
	void * p38 = my_malloc(12038);
	void * p39 = my_malloc(49056);
	void * p40 = my_malloc(5088);
	void * p41 = my_malloc(4835);

	my_free(p37);
	my_free(p38);
	my_free(p39);
	my_free(p40);
	my_free(p41);
	assert_count(0);

	void * p42 = my_malloc(33013);
	void * p43 = my_malloc(16291);
	void * p44 = my_malloc(2187);
	void * p45 = my_malloc(1688);
	void * p46 = my_malloc(1198);
	void * p47 = my_malloc(16229);
	void * p48 = my_malloc(1022);
	void * p49 = my_malloc(4086);
	void * p50 = my_malloc(4171);
	void * p51 = my_malloc(645);
	void * p52 = my_malloc(8411);
	void * p53 = my_malloc(3809);
	void * p54 = my_malloc(4148);
	void * p55 = my_malloc(15989);

	my_free(p42);
	my_free(p43);
	my_free(p44);
	my_free(p45);
	my_free(p46);
	my_free(p47);
	my_free(p48);
	my_free(p49);
	my_free(p50);
	my_free(p51);
	my_free(p52);
	my_free(p53);
	my_free(p54);
	my_free(p55);
	assert_count(0);

	void * p56 = my_malloc(49304);
	void * p57 = my_malloc(12220);
	void * p58 = my_malloc(4816);
	void * p59 = my_malloc(3875);
	void * p60 = my_malloc(5108);
	void * p61 = my_malloc(20137);
	void * p62 = my_malloc(17427);

	my_free(p56);
	my_free(p57);
	my_free(p58);
	my_free(p59);
	my_free(p60);
	my_free(p61);
	my_free(p62);
	assert_count(0);

	void * p63 = my_malloc(1113);
	void * p64 = my_malloc(15893);
	void * p65 = my_malloc(16537);
	void * p66 = my_malloc(88);
	void * p67 = my_malloc(33157);
	void * p68 = my_malloc(15885);
	void * p69 = my_malloc(4573);
	void * p70 = my_malloc(984);
	void * p71 = my_malloc(16742);
	void * p72 = my_malloc(16317);
	void * p73 = my_malloc(2349);
	void * p74 = my_malloc(343);
	void * p75 = my_malloc(8672);
	void * p76 = my_malloc(16180);
	void * p77 = my_malloc(2311);
	void * p78 = my_malloc(15874);
	void * p79 = my_malloc(986);
	void * p80 = my_malloc(16360);
	void * p81 = my_malloc(1165);
	void * p82 = my_malloc(940);

	my_free(p63);
	my_free(p64);
	my_free(p65);
	my_free(p66);
	my_free(p67);
	my_free(p68);
	my_free(p69);
	my_free(p70);
	my_free(p71);
	my_free(p72);
	my_free(p73);
	my_free(p74);
	my_free(p75);
	my_free(p76);
	my_free(p77);
	my_free(p78);
	my_free(p79);
	my_free(p80);
	my_free(p81);
	my_free(p82);
	assert_count(0);
}

// checks behavior of realloc on last allocated block
void realloc_expand_final_block() {
	mem_reset_brk();
	my_init();

	void* p1 = my_malloc(BIGCONST);

	void* p2 = my_malloc(SMALLCONST);

	void* heap_top_old = my_heap_hi();

	void* p3 = my_realloc(p2, SMALLCONST + BIGCONST);

	// check pointer stays the same.
	assert (p2 == p3);
	
	// check top of heap only moved by at most BIGCONST
	void* heap_top_new = my_heap_hi();
	assert((uint8_t*) heap_top_new - (uint8_t*) heap_top_old <= BIGCONST);

	// check header is updated
	void* header_pointer = (uint8_t*) p3 - SIZE_T_SIZE;
	assert(*(size_t*) header_pointer == ALIGN(SMALLCONST + BIGCONST));

	// check footer is updated
	void* footer_pointer = (uint8_t*) p3 + *(uint8_t*) ((uint8_t*) p3 - SIZE_T_SIZE);
	assert(*(size_t*) footer_pointer == 0);
}

// test behvior of realloc on something in the middle
void realloc_shrink_middle_block() {
	mem_reset_brk();
	my_init();

	void* p1 = my_malloc(BIGCONST);

	void* p2 = my_malloc(BIGCONST);

	void* p3 = my_malloc(SMALLCONST);

	void* heap_top_old = my_heap_hi();

	void* p4 = my_realloc(p2, SMALLCONST);

	// check pointer stays the same.
	assert (p2 == p4);
	
	// check top of heap didn't move.
	void* heap_top_new = my_heap_hi();
	assert(heap_top_new == heap_top_old);

	// check header and footer stayed the same.
	void* header_pointer = (uint8_t*) p4 - SIZE_T_SIZE;
	assert(*(size_t*) header_pointer == ALIGN(BIGCONST));

	void* footer_pointer = (uint8_t*) p4 + *(uint8_t*) ((uint8_t*) p4 - SIZE_T_SIZE);
	assert(*(size_t*) footer_pointer == 0);

}

// runs all tests
#ifdef TEST
int main() {

	mem_init();
	c7_test();
	mem_deinit();
	printf("PASSED: c7_test\n");

	mem_init();
	c7_test_2();
	mem_deinit();
	printf("PASSED: c7_test_2\n");

	mem_init();
	c7_test_3();
	mem_deinit();
	printf("PASSED: c7_test_3\n");

	mem_init();
	realloc_expand_final_block();
	mem_deinit();
	printf("PASSED: realloc_expand_final_block\n");

	mem_init();
	realloc_shrink_middle_block();
	mem_deinit();
	printf("PASSED: realloc_shrink_middle_block\n");
	
	printf("ALL TESTS PASSED\n");
	return 1;
}
#endif
