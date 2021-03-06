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
    // all stored sizes should be aligned
    assert((ALIGN(unaligned_size)) ==  unaligned_size);
    size = ALIGN(*(size_t*)p + SIZE_T_SIZE);
    p += size + FOOTER_SIZE; //modified to include footers
  }

  if (p != hi) {
    printf("Bad headers did not end at heap_hi!\n");
    printf("heap_lo: %p, heap_hi: %p, size: %lu, p: %p\n", lo, hi, size, p);
    return -1;
  }

  return 0;
}

// Returns which bucket to use given an aligned_size
unsigned int get_bucket(unsigned int aligned_size){
  unsigned int exponent = 31 - __builtin_clz(aligned_size);
  if (exponent < BASESIZE)
    return 0;
  unsigned int bucket =  exponent - BASESIZE;
  if (bucket + 1 >= NUMBUCKETS)
    return NUMBUCKETS - 1;
  else
    return bucket+1;
}

static inline void remove_from_free_list(free_list_t * block){
  if(block->prev){
    (block->prev)->next = block->next;
    if(block->next){
      (block->next)->prev = block->prev;
    }
  }
  else{
    if(block->next)
      (block->next)->prev = NULL;
    unsigned int bucket = get_bucket(block->size + SIZE_T_SIZE);
    free_list_array[bucket] = block->next;
  }
}

// coalesce two adjacent blocks and add result into free list
void join_blocks(free_list_t * b1, free_list_t * b2) {
  // figure out which block comes first, before coalescing.
  free_list_t * first; free_list_t * last;
  if (b1 < b2) {
    first = b1;
    last = b2;
  } else {
    first = b2;
    last = b1;
  }

  // remove both from free list.
  remove_from_free_list(first);
  remove_from_free_list(last);

  size_t new_size = first->size + last->size + 2*SIZE_T_SIZE + FOOTER_SIZE; //includes header
  unsigned int free_list_array_index = get_bucket(new_size);

  // update our block
  if(free_list_array[free_list_array_index])
    free_list_array[free_list_array_index]->prev = first;

  *first = (free_list_t) {.next = free_list_array[free_list_array_index],
                          .prev = NULL, .size = new_size - SIZE_T_SIZE};
  free_list_array[free_list_array_index] = first;

  // set footer correctly
  void * footer = (void *) ((uint8_t*) first + new_size);
  *(size_t*) footer = new_size - SIZE_T_SIZE;
}

// Call before using the memory system. Initalizes global variables.
int my_init() {
  for(int i = 0; i < NUMBUCKETS; i++){
    free_list_array[i] = NULL;
  }
  return 0;
}

//  malloc returns a pointer to a heap location with size bytes free for use.
//  Always allocate a block whose size is a multiple of the alignment.
void * my_malloc(size_t size) {
  
  int aligned_size = ALIGN(size + SIZE_T_SIZE);
  void *p = NULL;
  free_list_t * next;

  unsigned int free_list_array_index = get_bucket(aligned_size);
  int need_resizing = 0; //indicator variable for vailidity of the header

  //searches every bucket with appropriate size for a free block that works
  for(unsigned int iterate = free_list_array_index; iterate < NUMBUCKETS; iterate++){
    next = free_list_array[iterate];
    free_list_t * prev = NULL;
    while (next) {
      if(next->size + SIZE_T_SIZE >= aligned_size) { 
        if(prev) {
          prev->next = next->next; //pop from doubly linked list
          if(prev->next)
            (prev->next)->prev = prev;
        } else { // first node removed
          free_list_array[iterate] = next->next;
          if(free_list_array[iterate])
            free_list_array[iterate]->prev = NULL;
        }
        p = next;

        // free the rest of the block
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
          need_resizing = 1; //header of allocated block is no longer valid
        }
        break;
      }

      if(!(next->next)){
        break; // no match found
      }
      prev = next;
      next = next->next;
    }
    if(p) // found a fit
      break;
  }
  // next is valid and the last element in the linked list
  // checks whether we need a mem_sbrk
  int need_mem_sbrk = !p;
  if (need_mem_sbrk){
    // Expands the heap by the given number of bytes and returns a pointer to
    // the newly-allocated area. 
    p = mem_sbrk(aligned_size+FOOTER_SIZE);
  }

  if (p == (void *)-1) {
    // Whoops, an error of some sort occurred.  We return NULL to let
    // the client code know that we weren't able to allocate memory.
    return NULL;
  } else {
    if (need_mem_sbrk || need_resizing) {
      // We store the size of the block we've allocated in the first
      // SIZE_T_SIZE bytes.
      *(size_t*)p = aligned_size-SIZE_T_SIZE;
      // set footer size to be 0. When freed, set to be same as header size
      *((size_t*) ((uint8_t*) p + aligned_size)) = 0;
    } else {
      void* footer = (uint8_t*) p + *(size_t*)p + SIZE_T_SIZE;
      *((size_t*) ((uint8_t*) footer)) = 0;
    }
    // Then, we return a pointer to the rest of the block of memory,
    // which is at least size bytes long.  We have to cast to uint8_t
    // before we try any pointer arithmetic because voids have no size
    // and so the compiler doesn't know how far to move the pointer.
    // Since a uint8_t is always one byte, adding SIZE_T_SIZE after
    // casting advances the pointer by SIZE_T_SIZE bytes.
    return (void *)((char *)p + SIZE_T_SIZE);
  }
}

// free block at ptr_header given it's aligned_size
void my_free_with_size(void *ptr_header, size_t aligned_size) {
  unsigned int free_list_array_index = get_bucket(aligned_size);
  if(free_list_array[free_list_array_index])
    free_list_array[free_list_array_index]->prev = (free_list_t *) ptr_header;
  // set up free_list_t at ptr_header
  *((free_list_t *) ptr_header) = (free_list_t)
                                {.next = free_list_array[free_list_array_index],
                                 .prev = NULL, .size = aligned_size - SIZE_T_SIZE};
  free_list_array[free_list_array_index] = ptr_header;
}

// free - Freeing a block does nothing.
void my_free(void *ptr) {
  void* ptr_header = ((char*)ptr) - SIZE_T_SIZE;
  size_t size_block = *((size_t*) (ptr_header)) + SIZE_T_SIZE;
  if (size_block >= sizeof(free_list_t)) {

    assert(size_block + FOOTER_SIZE >= sizeof(free_list_t) + FOOTER_SIZE);
    size_t aligned_size = ALIGN(size_block);
    assert(aligned_size == size_block);

    my_free_with_size(ptr_header, aligned_size);

    // set footer
    *((size_t*) ((uint8_t*) ptr_header + aligned_size)) = aligned_size-SIZE_T_SIZE;

    // check header of next memory to compute footer.
    void * next_block = (void *) ((uint8_t*) ptr_header + aligned_size + FOOTER_SIZE);
    if (next_block < my_heap_hi()) {
      size_t size_next_block = ((free_list_t*) next_block)->size;
      void * footer_next_block = (void *) ((uint8_t*) next_block
                                           + SIZE_T_SIZE + size_next_block);
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
        (free_list_t*) ((uint8_t*) prev_block - size_prev_block - SIZE_T_SIZE),
        ptr_header
      );
    }
  }
}

// Returns a new pointer to the resized block with the data preserved.
// The returned pointer may be the same as *ptr when size < ptr's size, or when
// *ptr was the last allocated block on the heap.
// Otherwise just uses with a malloc, memcpy, and free.
void * my_realloc(void *ptr, size_t size) {
  void *newptr;
  size_t copy_size;

  // if ptr is the last allocated block on the stack, just mem_sbrk() 
  // size-block_size instead. No need to copy data.
  void* ptr_header = ((char*)ptr) - SIZE_T_SIZE;
  size_t size_block = *((size_t*) (ptr_header)) + SIZE_T_SIZE;
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
  } else {
    // check if following block is free and big enough to accomodate
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
