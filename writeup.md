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
<!-- Buddy memory, coalesce on joins, search in larger and larger buckets and free slack, etc. -->
<!-- Don't forget to talk about why we took log(size), etc. for speed -->

#Data Structures
The free lists are represented as doubly linked lists, which allows for O(1) insertion and deletion. 

Each memory block maintains a footer, which is a single size_t that is set to zero when a block of memory is in use, and set to the size of usable memory (same as the free list node's size member). This footer was included to allow for very fast coalescing, as described in [coalescing](#algorithm) and in code in [join_blocks()](#join_blocks).

#Key Methods

###get_bucket()

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

#Failed optimizations
<!-- For malloc requests, experimented with always checking the next bucket instead of the 'appropriately sized' bucket.  -->

#Individual Contributions
###Shawn
 - coalescing implemention and unit tests
 - footer to speed up coalescing
 - realloc revision and unit tests targeting class 9 traces
 - check for coalescing opportunities in my_free
 - 'next largest bucket' strategy (reverted)

###Robi
 - Doubly linked list for free lists
 - Variable sized buckets
<Fill in more>

###Joint
 - Validator