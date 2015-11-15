Serial Dynamic Memory Allocation
================================
###6.172 Fall 2015
Shantanu Jain
[<jains@mit.edu>](mailto:jains@mit.edu)

Robi Bhattacharjee
[<robibhatt@gmail.com>](mailto:robibhatt@gmail.com)
------------

#Overview
The project was developed in three main phases. The first phase implemented a simple two-bucket free list, where all blocks larger than a threshold automatically were placed into a larger bucket. The second phase modified this with a powers-of-two based bucketing system. Each free list in this implementation roughly held blocks in the range [2^x, 2^(x-1)), where x is the bucket. The number of buckets was a parameterized variable. The final phase finished the implementation of the basic buddy memory allocator, by adding logic for coalescing adjacent blocks and borrowing blocks from upper buckets if a lower bucket is exhausted. This implementation was tweaked with [opentuner](#autotuning), [profiled with gperf](#profiling), further optimized on bottlenecking code, and revised to improve performance on certain classes of traces. 

#Algorithm
Our algorithm is a standard free list alogirthm. In short, we maintain a list of free blocks, and given a malloc we scan the list to see if any of the free blocks can be used. Upon freeing a block, we add it to the free list. There are three things we do to speed this up. First, we partition the free_list into several doubly linked lists each of which corresponds to certain range of sizes. This way we don't spend too much time looking through blocks that are clearly too small. Second, upon assigning a free block to a malloc call, if the free block is clearly larger than the malloc, we free the "slack", which is just the remaining portion of the free block that isn't used by the malloc. Third, every time we free a block, we check the adjacent blocks to see if they are free, and if this is the case we coalesce the 2 adjacent free blocks into one larger free blocks. 


#Data Structures
The free lists are represented as doubly linked lists, which allows for O(1) insertion and deletion. 

Each memory block maintains a footer, which is a single size_t that is set to zero when a block of memory is in use, and set to the size of usable memory (same as the free list node's size member). This footer was included to allow for very fast coalescing, as described in [coalescing](#algorithm) and in code in [join_blocks()](#join_blocks).

#Key Methods

###get_bucket(size)
simply returns the floor of log(size/base_size). Implemented using a gcc built in

###my_free()
calls join_blocks() every time since this is the main place where the free list is updated.

###my_malloc()

###my_realloc()
Checks to see if the block is being shrunk. If so, the pointer is simply returned with no change to the state of the allocator. The unused space of the block just stays with that block. 

If additional space is being requested, then the 

Otherwise falls back to the staff's naieve malloc-copy-free strategy.

###join_blocks()

#Autotuning
There were two major parameters in this implemenation. The `NUMBUCKETS` parameter varied the number of buckets (free lists) that were initialized and maintained. Adjusting the `NUMBUCKETS` parameter upwards would just create buckets for larger blocks that would have been otherwise been put into the largest bucket. 

#Profiling
We learned from profiling that our join_buckets function was taking a massive amount of time. This was because we weren't using doubly linked lists, and deletions were O(n) instead of O(1). Changing this to a doubly linked list got rid of the problem and boosted our throughput to 99.

#Failed optimizations
 - checking the next bucket as opposed to searching the current bucket to find a more optimally sized free block

#Individual Contributions
###Shawn
 - coalescing implemention and unit tests
 - footer to speed up coalescing
 - realloc revision and unit tests targeting class 9 traces
 - check for coalescing opportunities in my_free
 - 'next largest bucket' strategy (reverted)
 - set up testing suite

###Robi
 - Wrote the initial my_malloc and my_free function for 2 bucketed lists
 - Expanded this into multiple bucket free_lists
 - Added implementation of "freeing the slack" upon mallocing from free_list
 - Changed free_list to doubly linked list, and rewrote all necessary functions to integrate change
 - Debugged a lot of the code (see git logs)


###Joint
 - Validator