Dynamic Memory Allocator Project
Overview
This project involves building a dynamic storage allocator for C programs, implementing custom versions of the malloc, free, and realloc functions. The goal is to create an allocator that is efficient, fast, and reliable while adhering to strict design and testing requirements.

As part of this project, I wrote a heap consistency checker to ensure the correctness and integrity of the heap. This tool serves as a debugging aid to identify and resolve potential issues in the allocator implementation.

The code I developed is in mm.c, where I implemented five key functions: mm_init, malloc, free, realloc, and mm_checkheap. To structure the code efficiently, I added helper functions and defined structures as needed, maintaining modularity and clarity throughout.

Function Descriptions
1. Initialization (mm_init)
This function sets up the memory allocator by performing any necessary initialization, like creating the initial heap area. It’s called before any other allocator function and ensures the allocator is ready to handle allocation requests.

2. Allocation (malloc)
The malloc function returns a pointer to a block of memory with a payload of at least size bytes. It guarantees:

The block lies within the heap boundary.
The pointer is aligned to 16 bytes for compatibility.
Returns NULL if it runs out of memory.
3. Freeing Memory (free)
The free function releases a previously allocated block. It safely handles null pointers and ensures no double freeing of the same block occurs.

4. Reallocation (realloc)
The realloc function adjusts the size of an existing block while preserving its contents:

If ptr is NULL, it behaves like malloc.
If size is 0, it behaves like free.
If resizing is necessary, it returns a new block, copying the old block’s data.
5. Heap Consistency Checker (mm_checkheap)
This function validates the heap structure to ensure it adheres to expected rules:

Verifies that free blocks are correctly linked in the free list.
Ensures no overlapping allocated blocks exist.
Checks that all pointers are valid and within the heap boundary.
The checker not only serves as a tool for debugging but also highlights my commitment to delivering reliable and error-free code.

Design Principles and Rules
The allocator is built under the following constraints:

Memory Management: The project relies on a provided memory library (memlib.c) to simulate memory allocation. Instead of using system calls like sbrk, my implementation uses mm_sbrk to extend the heap.
64-bit Alignment: All payload pointers are aligned to 16 bytes for optimal performance and compatibility.
Global Space: The use of global memory is limited to 128 bytes, ensuring that most tracking data is managed within the heap.
Code Quality: The design avoids macros and instead uses static functions for better readability and compiler optimizations.
Testing and Validation
Testing focused on:

Ensuring no memory leaks or segmentation faults.
Validating heap consistency using the mm_checkheap function.
Passing rigorous trace-driven tests to measure the allocator's performance and correctness.
Challenges and Key Takeaways
This project sharpened my ability to work with low-level memory operations and debug complex pointer-based code. Writing the heap consistency checker reinforced the importance of building robust debugging tools to identify subtle issues efficiently. By focusing on both design and implementation, I delivered an allocator that is both reliable and maintainable.
