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
#define BASESIZE 500
#define NUMBUCKETS 7
#endif

// Rounds up to the nearest multiple of ALIGNMENT.
#define ALIGN(size) (((size) + (ALIGNMENT-1)) & ~(ALIGNMENT-1))

// The smallest aligned size that will hold a size_t value.
#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))

typedef struct free_list_t {
  size_t size; 
  struct free_list_t* next;
} free_list_t;

free_list_t * free_list_array[NUMBUCKETS];

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
    p += size;
  }

  if (p != hi) {
    printf("Bad headers did not end at heap_hi!\n");
    printf("heap_lo: %p, heap_hi: %p, size: %lu, p: %p\n", lo, hi, size, p);
    return -1;
  }

  return 0;
}

// init - Initialize the malloc package.  Called once before any other
// calls are made.  Since this is a very simple implementation, we just
// return success.
int my_init() {
  for(int i = 0; i < NUMBUCKETS; i++){
    free_list_array[i] = NULL;
  }
  return 0;
}

// figures out which bucket to use, takes aligned sizes
// currently returns a bucket that potentially works

unsigned int get_bucket(unsigned int aligned_size){
  if (aligned_size < BASESIZE)
    return 0;
  unsigned int rounded_down = aligned_size / BASESIZE;
  unsigned int bucket = 0;
  while(rounded_down >>= 1){
    ++bucket;
  }
  assert((BASESIZE << bucket) <= aligned_size);
  assert((BASESIZE << (bucket+1)) > aligned_size);
  if (bucket + 1 >= NUMBUCKETS)
    return NUMBUCKETS - 1;
  else
    return bucket+1;
}

//useful for debugging
void detect_cycle(free_list_t * start){
  free_list_t * next;
  free_list_t * double_next;
  next = start;
  double_next = start;
  while (next){
    next = next->next;
    double_next = double_next->next;
    if (double_next)
      double_next = double_next->next;
    else
      break;
    if (!double_next)
      break;
    assert(double_next != next);
  }
}

//  malloc - Allocate a block by incrementing the brk pointer.
//  Always allocate a block whose size is a multiple of the alignment.
void * my_malloc(size_t size) {
  // We allocate a little bit of extra memory so that we can store the
  // size of the block we've allocated.  Take a look at realloc to see
  // one example of a place where this can come in handy.
  assert(get_bucket(1) == 0);
  assert(get_bucket(BASESIZE) == 1);
  unsigned int aligned_size = ALIGN(size + SIZE_T_SIZE);

  void *p = NULL;
  // Expands the heap by the given number of bytes and returns a pointer to
  // the newly-allocated area.  This is a slow call, so you will want to
  // make sure you don't wind up calling it on every malloc.

  free_list_t * next;
  unsigned int free_list_array_index = get_bucket(aligned_size);
  assert(free_list_array_index < NUMBUCKETS);
  next = free_list_array[free_list_array_index];
  free_list_t * prev = NULL;
  //detect_cycle(next);
  /* variables slack, ptr_heaader, need_clearing are used in the case where we use an
     from the free list that has a size significantly greater than the size we just
     stored. */
  /*size_t slack;
  void* ptr_header;
  int need_clearing = 0; */
  while (next) {
    size_t next_size = ALIGN(next->size + SIZE_T_SIZE);
    if(next_size >= aligned_size) {
      //means we potentially need to free memory for the linked list
      //need_clearing = 1;
      //slack = next_size - aligned_size;

      // pop from linked list
      if(prev) {
        prev->next = next->next;
      } else { // first node removed
        free_list_array[free_list_array_index] = next->next;
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

  //ptr_header = ((char *) p) + aligned_size;

  //code to make sure we do change the size stored at p if we are going to add the slack to the free list
  /*int need_resizing = 1;
  if(need_clearing == 1){
    need_resizing = 0; //set to 0 if too slack is too small
    // this is the code that frees the rest of the already free device
    if (slack >= BASESIZE){
      need_resizing = 1;
      my_free_with_size(ptr_header, slack);
    }
    }*/
  // checks whether we need a mem_sbrk
  unsigned int need_mem_sbrk = !p;
  if (need_mem_sbrk)
    p = mem_sbrk(aligned_size);


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
    if (need_mem_sbrk)
      *(size_t*)p = aligned_size-SIZE_T_SIZE;
    return (void *)((char *)p + SIZE_T_SIZE);
  }
}

// free given size to free
void my_free_with_size(void *ptr_header, size_t aligned_size) {
  unsigned int free_list_array_index = get_bucket(aligned_size);
  *((free_list_t *) ptr_header) = (free_list_t) {.next = free_list_array[free_list_array_index], .size = aligned_size - SIZE_T_SIZE};
  free_list_array[free_list_array_index] = ptr_header;

  //&free_list to get location of free memory
}

// free - Freeing a block does nothing.
void my_free(void *ptr) {
  void* ptr_header = ((char*)ptr) - SIZE_T_SIZE;
  size_t size_block = *((size_t*) (ptr_header)) + SIZE_T_SIZE;
  assert(size_block >= sizeof(free_list_t));
  size_t aligned_size = ALIGN(size_block);
  my_free_with_size(ptr_header, aligned_size);
  /*unsigned int free_list_array_index = get_bucket(aligned_size);
  *((free_list_t *) ptr_header) = (free_list_t) {.next = free_list_array[free_list_array_index], .size = aligned_size - SIZE_T_SIZE};
  free_list_array[free_list_array_index] = ptr_header; */

  //&free_list to get location of free memory
}

// realloc - Implemented simply in terms of malloc and free
void * my_realloc(void *ptr, size_t size) {
  void *newptr;
  size_t copy_size;

  // Allocate a new chunk of memory, and fail if that allocation fails.
  copy_size = *(size_t*)((uint8_t*)ptr - SIZE_T_SIZE);
  if (size < copy_size){
    return ptr;
  }

  // If the new block is smaller than the old one, we have to stop copying
  // early so that we don't write off the end of the new block of memory.
  newptr = my_malloc(size);
  if (NULL == newptr)
    return NULL;

  // Get the size of the old block of memory.  Take a peek at my_malloc(),
  // where we stashed this in the SIZE_T_SIZE bytes directly before the
  // address we returned.  Now we can back up by that many bytes and read
  // the size.

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
