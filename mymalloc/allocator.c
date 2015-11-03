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
#ifndef ALIGNMENT
#define ALIGNMENT 8
#define SIZELIMIT 500
#endif

// Rounds up to the nearest multiple of ALIGNMENT.
#define ALIGN(size) (((size) + (ALIGNMENT-1)) & ~(ALIGNMENT-1))
#define IS_SMALL(size) ((size) <= SIZELIMIT)

// The smallest aligned size that will hold a size_t value.
#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))

#define FOOTER_SIZE SIZE_T_SIZE

// // size is the size of memory without the header
// typedef struct free_list_t {
//   size_t size; 
//   struct free_list_t* next;
// } free_list_t;

// extern free_list_t * small_free_list;
// extern free_list_t * big_free_list;

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

static inline int remove_from_free_list_given(free_list_t * block, free_list_t ** list) {
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


static inline int remove_from_free_list(free_list_t * block) {
  if (remove_from_free_list_given(block, &small_free_list))
    return 1;
  else 
    return remove_from_free_list_given(block, &big_free_list);
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

  // if the first block is in the big_free_list, just adjust the size. 
  // delete last from free_list
  if (!IS_SMALL(first->size)) {
    first->size += (last->size + SIZE_T_SIZE); // what about header?
    assert(remove_from_free_list(last));
  } else { // if first is not in the large free list
    // remove both blocks from the free list
    assert(remove_from_free_list(first));
    assert(remove_from_free_list(last));
    
    // set up a new combined block in either the small or big free_list
    // add 2*SIZE_T_SIZE to get the total size of the memory.
    size_t size_block = first->size + last->size + (2*SIZE_T_SIZE);
    // choose free list to add based on size_block
    free_list_t ** bin = IS_SMALL(size_block) ? &small_free_list: &big_free_list;

    assert(size_block >= sizeof(free_list_t));
    // size_t aligned_size = ALIGN(size_block); // use in .size?
    *first = (free_list_t) {.next = *bin, .size = size_block - SIZE_T_SIZE};
    *bin = first;

  }
}

// init - Initialize the malloc package.  Called once before any other
// calls are made.  Since this is a very simple implementation, we just
// return success.
int my_init() {
  small_free_list = NULL;
  big_free_list = NULL;
  return 0;
}

//  malloc - Allocate a block by incrementing the brk pointer.
//  Always allocate a block whose size is a multiple of the alignment.
void * my_malloc(size_t size) {
  // We allocate a little bit of extra memory so that we can store the
  // size of the block we've allocated.  Take a look at realloc to see
  // one example of a place where this can come in handy.
  int aligned_size = ALIGN(size + SIZE_T_SIZE);

  void *p = NULL;
  // Expands the heap by the given number of bytes and returns a pointer to
  // the newly-allocated area.  This is a slow call, so you will want to
  // make sure you don't wind up calling it on every malloc.

  free_list_t * next;
  int is_small = (aligned_size <= SIZELIMIT);
  if (is_small)
    next = small_free_list;
  else
    next = big_free_list;
  free_list_t * prev = NULL;

  while (next) {
    if(next->size + SIZE_T_SIZE >= aligned_size) {
      // pop from linked list
      if(prev) {
        prev->next = next->next;
      } else { // first node removed
        if (is_small)
          small_free_list = next->next;
        else
          big_free_list = next->next;
      }
      p = next;
      break;
    }

    if(!(next->next)){
      break; // no match found
    }
    prev = next;
    next = next->next;
  } // next is valid and the last element in the linked list
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
    if (need_mem_sbrk) { // TODO: Need to set headers/footers again when blocks are split.
      *(size_t*)p = aligned_size-SIZE_T_SIZE;
      // set footer size to be 0. When freed, set to be same as header size
      *((size_t*) ((uint8_t*) p + aligned_size+SIZE_T_SIZE)) = 0;//aligned_size-SIZE_T_SIZE;
    }

    return (void *)((char *)p + SIZE_T_SIZE);
  }
}

// free - Freeing a block does nothing.
void my_free(void *ptr) {
  void* ptr_header = ((char*)ptr) - SIZE_T_SIZE;
  size_t size_block = *((size_t*) (ptr_header)) + SIZE_T_SIZE;
  assert(size_block >= sizeof(free_list_t));

  size_t aligned_size = ALIGN(size_block);
  if (aligned_size <= SIZELIMIT){
    *((free_list_t *) ptr_header) = (free_list_t) {.next = small_free_list, .size = aligned_size - SIZE_T_SIZE};
    small_free_list = ptr_header;
  }
  else {
    *((free_list_t *) ptr_header) = (free_list_t) {.next = big_free_list, .size = aligned_size - SIZE_T_SIZE};
    big_free_list = ptr_header;
  }
  // set footer
  *((size_t*) ((uint8_t*) ptr_header + aligned_size)) = aligned_size-SIZE_T_SIZE;
  //&free_list to get location of free memory
  // check footer of previous memory.

  // check header of next memory to compute footer.
}

// realloc - Implemented simply in terms of malloc and free
// if change in size is not too big, just change size of header?
void * my_realloc(void *ptr, size_t size) {
  void *newptr;
  size_t copy_size;

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
