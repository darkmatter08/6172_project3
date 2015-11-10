/**
 * Copyright (c) 2015 MIT License by 6.172 Staff
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 **/

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "./allocator_interface.h"
#include "./memlib.h"

// Don't call libc malloc!
#define malloc(...) (USE_MY_MALLOC)
#define free(...) (USE_MY_FREE)
#define realloc(...) (USE_MY_REALLOC)

// All blocks must have a specified minimum alignment.
// The alignment requirement (from config.h) is >= 8 bytes.

// Rounds up to the nearest multiple of ALIGNMENT.
#define ALIGN(size) (((size) + (ALIGNMENT-1)) & ~(ALIGNMENT-1))

// DEPRECATED WTIH BUCKETS
// #define IS_SMALL(size) ((size) <= SIZELIMIT)

// The smallest aligned size that will hold a size_t value.
#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))

#define FOOTER_SIZE SIZE_T_SIZE

// check - This checks our invariant that the size_t header before every
// block points to either the beginning of the next block, or the end of the
// heap.
int my_check() {
  char *p;
  char *lo = (char*)mem_heap_lo();
  char *hi = (char*)mem_heap_hi() + 1;
  size_t size = 0;

  p = lo;
  while (lo <= p && p < hi) {
    size_t unaligned_size = *(size_t*)p;
    assert((ALIGN(unaligned_size)) ==  unaligned_size); // all stored sizes should be aligned
    size = ALIGN(*(size_t*)p + SIZE_T_SIZE);
    p += size + FOOTER_SIZE;
  }

  if (p != hi) {
    printf("Bad headers did not end at heap_hi!\n");
    printf("heap_lo: %p, heap_hi: %p, size: %lu, p: %p\n", lo, hi, size, p);
    return -1;
  }

  return 0;
}

// FOR BUCKETS
// figures out which bucket to use, takes aligned sizes
// currently returns a bucket that potentially works
unsigned int get_bucket(unsigned int aligned_size){
  unsigned int exponent = 31 - __builtin_clz(aligned_size);
  if (exponent < BASESIZE)
    return 0;
  unsigned int bucket =  exponent - BASESIZE;
  //assert(bucket == other_bucket);
  if (bucket + 1 >= NUMBUCKETS)
    return NUMBUCKETS - 1;
  else
    return bucket+1;
}

static inline int remove_from_free_list_given(free_list_t * block,
                                              free_list_t ** list) {
  free_list_t * next = *list;
  free_list_t * prev = NULL;

  while (next) {
    if (next == block) {
      // pop from linked list
      if (prev) {
        prev->next = next->next;
      } else { // first node removed
        *list = next->next;
      }
      return 1; //break;
    }

    if(!(next->next)){
      break; // no match found
    }
    prev = next;
    next = next->next;
  }

  return 0;
}

// TODO: Update for buckets
static inline int remove_from_free_list(free_list_t * block) {
  // FOR BUCKETS:
  for (int i = 0; i < NUMBUCKETS; i++) {
    if (remove_from_free_list_given(block, &free_list_array[i]))
      return 1;
  }
  return 0;
}

// coalesce two blocks in a free list 
void join_blocks(free_list_t * b1, free_list_t * b2) {
  // figure out which block comes first, before coalescing.
  // free_list_t * first = (uint64_t) b1 < (uint64_t) b2 ? b1: b2;
  free_list_t * first; free_list_t * last;
  if (b1 < b2) {
    first = b1;
    last = b2;
  } else {
    first = b2;
    last = b1;
  }


  // FOR BUCKETS:
  // remove both from free list.
  int i1 = remove_from_free_list(first);
  int i2 = remove_from_free_list(last);
  assert(i1);
  assert(i2);

  // set up a new combined block in the appropriate free list
  // add 2*SIZE_T_SIZE to get the total size of the memory.
  size_t size_block = first->size + last->size + (2*SIZE_T_SIZE) + (2*FOOTER_SIZE);
  // choose free list to add based on size_block
  free_list_t ** bin = &free_list_array[get_bucket(size_block)];//IS_SMALL(size_block) ? &small_free_list: &big_free_list;

  assert(size_block >= sizeof(free_list_t));
  // size_t aligned_size = ALIGN(size_block); // use in .size?
  assert(ALIGN(size_block) == size_block);
  *first = (free_list_t) {.next = *bin, .prev = NULL, .size = size_block - SIZE_T_SIZE - FOOTER_SIZE};
  assert(ALIGN(first->size) == first->size);
  *bin = first;
  void * footer = (void *) ((uint8_t*) first + size_block - FOOTER_SIZE);
  // set footer on joins
  *(size_t*) footer = size_block - SIZE_T_SIZE - FOOTER_SIZE;

}


int my_init() {
  // FOR BUCKETS
  for(int i = 0; i < NUMBUCKETS; i++){
    free_list_array[i] = NULL;
  }
  return 0;
}

//  malloc - Allocate a block by incrementing the brk pointer.
//  Always allocate a block whose size is a multiple of the alignment.
void * my_malloc(size_t size) {
  // We allocate a little bit of extra memory so that we can store the
  // size of the block we've allocated.  Take a look at realloc to see
  // one example of a place where this can come in handy.
  
  // FOR BUCKETS

  assert(get_bucket(1) == 0);
  //unsigned int base_size = 1 << BASESIZE; 
  //assert(get_bucket(base_size) == 1);

  int aligned_size = ALIGN(size + SIZE_T_SIZE);
  //assert(aligned_size >= sizeof(free_list_t));

  void *p = NULL;
  // Expands the heap by the given number of bytes and returns a pointer to
  // the newly-allocated area.  This is a slow call, so you will want to
  // make sure you don't wind up calling it on every malloc.

  free_list_t * next;

  // FOR BUCKETS
  unsigned int free_list_array_index = get_bucket(aligned_size);
  assert(free_list_array_index < NUMBUCKETS);
  int need_resizing = 0;
  for(unsigned int iterate = free_list_array_index; iterate < NUMBUCKETS; iterate++){
    next = free_list_array[iterate];
    free_list_t * prev = NULL;

    while (next) {
      // FOR BUCKETS
      // possible issue: other codebase aligns the next->size + SIZE_T_SIZE
      assert(next->size == ALIGN(next->size));
      if(next->size + SIZE_T_SIZE >= aligned_size) {
        // pop from linked list
        if(prev) {
          prev->next = next->next;
        } else { // first node removed
          // FOR BUCKETS
          free_list_array[iterate] = next->next;
        }
        p = next;
        // TODO: free slack?
        size_t slack = 0;
        if (next->size + SIZE_T_SIZE > aligned_size + FOOTER_SIZE) {
          slack = next->size + SIZE_T_SIZE - aligned_size - FOOTER_SIZE;
        }
        if (slack > sizeof(free_list_t) + FOOTER_SIZE) {
          // create a new element in the free list
          void* next_block = (void *) ((uint8_t*) p + aligned_size+FOOTER_SIZE);
          my_free_with_size(next_block, slack);
          // set footer of slack block
          *((size_t*) ((uint8_t*) next_block + slack)) = slack-SIZE_T_SIZE;
          need_resizing = 1;
        }
        break;
      }

      if(!(next->next)){
        break; // no match found
      }
      prev = next;
      next = next->next;
    }
    if(p)
      break;
  }
  // next is valid and the last element in the linked list
  // checks whether we need a mem_sbrk
  int need_mem_sbrk = !p;
  if (need_mem_sbrk){
    p = mem_sbrk(aligned_size+FOOTER_SIZE);
  }


  if (p == (void *)-1) { // TODO: move check as part of mem_sbrk only
    // Whoops, an error of some sort occurred.  We return NULL to let
    // the client code know that we weren't able to allocate memory.
    return NULL;
  } else {
    // We store the size of the block we've allocated in the first
    // SIZE_T_SIZE bytes.
    //if (need_update)
    //*(size_t*)p = size; //aligned_size-SIZE_T_SIZE;W

    // Then, we return a pointer to the rest of the block of memory,
    // which is at least size bytes long.  We have to cast to uint8_t
    // before we try any pointer arithmetic because voids have no size
    // and so the compiler doesn't know how far to move the pointer.
    // Since a uint8_t is always one byte, adding SIZE_T_SIZE after
    // casting advances the pointer by SIZE_T_SIZE bytes.
    if (need_mem_sbrk || need_resizing) { // TODO: Need to set headers/footers again when blocks are split.
      assert(ALIGN(aligned_size - SIZE_T_SIZE) == aligned_size - SIZE_T_SIZE);
      *(size_t*)p = aligned_size-SIZE_T_SIZE;
      // set footer size to be 0. When freed, set to be same as header size
      *((size_t*) ((uint8_t*) p + aligned_size)) = 0;//aligned_size-SIZE_T_SIZE;
    } else {
      void* footer = (uint8_t*) p + *(size_t*)p + SIZE_T_SIZE;
      *((size_t*) ((uint8_t*) footer)) = 0;//aligned_size-SIZE_T_SIZE;
    }
    return (void *)((char *)p + SIZE_T_SIZE);
  }
}

// free given size to free
void my_free_with_size(void *ptr_header, size_t aligned_size) {
  unsigned int free_list_array_index = get_bucket(aligned_size);
  if(free_list_array[free_list_array_index])
    free_list_array[free_list_array_index]->prev = (free_list_t *) ptr_header;
  *((free_list_t *) ptr_header) = (free_list_t) {.next = free_list_array[free_list_array_index], .prev = NULL, .size = aligned_size - SIZE_T_SIZE};
  free_list_array[free_list_array_index] = ptr_header;
}

// free - Freeing a block does nothing.
void my_free(void *ptr) {
  void* ptr_header = ((char*)ptr) - SIZE_T_SIZE;
  size_t size_block = *((size_t*) (ptr_header)) + SIZE_T_SIZE;
  if (size_block >= sizeof(free_list_t)){

    assert(size_block + FOOTER_SIZE >= sizeof(free_list_t) + FOOTER_SIZE);
    size_t aligned_size = ALIGN(size_block);
    assert(aligned_size == size_block);

    // FOR BUCKETS:
    my_free_with_size(ptr_header, aligned_size);

    // set footer
    *((size_t*) ((uint8_t*) ptr_header + aligned_size)) = aligned_size-SIZE_T_SIZE;
    //&free_list to get location of free memory


    // check header of next memory to compute footer.
    void * next_block = (void *) ((uint8_t*) ptr_header + aligned_size + FOOTER_SIZE);
    if (next_block < my_heap_hi()) {
      size_t size_next_block = ((free_list_t*) next_block)->size;
      void * footer_next_block = (void *) ((uint8_t*) next_block + SIZE_T_SIZE + size_next_block);
      size_t value_footer_next_block = *(size_t*) footer_next_block;
      if (value_footer_next_block) {
        join_blocks(next_block, ptr_header);
      }
    }

    // check address of prev block's footer is greater than my_heap_lo().
    void * prev_block = (uint8_t*) ptr_header - FOOTER_SIZE;
    // check footer of previous memory.
    size_t size_prev_block = *((size_t*) prev_block); // possible invalid memory access
    if (prev_block > my_heap_lo() && size_prev_block) { // can coalesce back
      join_blocks(
          (free_list_t*) ((uint8_t*) ptr_header - FOOTER_SIZE - size_prev_block - SIZE_T_SIZE),
          ptr_header
                  );
    }
  }
}

// realloc - Implemented simply in terms of malloc and free
// if change in size is not too big, just change size of header?
void * my_realloc(void *ptr, size_t size) {
  void *newptr;
  size_t copy_size;

  // if ptr is the last allocated block on the stack, just mem_sbrk() 
  // size-block_size instead. No need to copy data.
  void* ptr_header = ((char*)ptr) - SIZE_T_SIZE;
  size_t size_block = *((size_t*) (ptr_header)) + SIZE_T_SIZE;
  // assert(size_block + FOOTER_SIZE >= sizeof(free_list_t) + FOOTER_SIZE);
  void* end_of_block_pointer = (uint8_t*) ptr_header + size_block + FOOTER_SIZE;
  ssize_t extraspace = size - (size_block - SIZE_T_SIZE);
  if (extraspace <=0)
    return ptr;
  else if (end_of_block_pointer - 1 == my_heap_hi()) {
    extraspace = ALIGN(extraspace);
    mem_sbrk(extraspace);
    // reset header
    *(size_t*) ptr_header += extraspace;
    // reset footer
    void* footer_pointer = (uint8_t*) ptr_header + *(size_t*) ptr_header + SIZE_T_SIZE;
    *(size_t*) footer_pointer = 0; // used block
    return ptr;
  } else { // check if following block is free and big enough to accomodate
  // extraspace. If so, 'coalesce' it into this block and use it. avoids copies
  // and mem_sbrks.
    ;
  }

  // Allocate a new chunk of memory, and fail if that allocation fails.
  newptr = my_malloc(size);
  if (NULL == newptr)
    return NULL;

  // Get the size of the old block of memory.  Take a peek at my_malloc(),
  // where we stashed this in the SIZE_T_SIZE bytes directly before the
  // address we returned.  Now we can back up by that many bytes and read
  // the size.
  copy_size = *(size_t*)((uint8_t*)ptr - SIZE_T_SIZE);

  // If the new block is smaller than the old one, we have to stop copying
  // early so that we don't write off the end of the new block of memory.
  if (size < copy_size)
    copy_size = size;

  // This is a standard library call that performs a simple memory copy.
  memcpy(newptr, ptr, copy_size);

  // Release the old block.
  my_free(ptr);

  // Return a pointer to the new block.
  return newptr;
}

// call mem_reset_brk.
void my_reset_brk() {
  mem_reset_brk();
}

// call mem_heap_lo
void * my_heap_lo() {
  return mem_heap_lo();
}

// call mem_heap_hi
void * my_heap_hi() {
  return mem_heap_hi();
}
