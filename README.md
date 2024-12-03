Dynamic Memory Allocator Project
Overview
This project involves designing and implementing a dynamic storage allocator for C programs. The allocator includes custom implementations of the malloc, free, and realloc functions, focusing on achieving efficiency, reliability, and performance.

A significant aspect of the project was developing a heap consistency checker to validate the integrity of the memory heap. This debugging tool identifies potential issues and ensures the allocator operates correctly under various scenarios.

The core functionality is implemented in mm.c, with additional helper functions and data structures to maintain modularity, clarity, and scalability.

Key Features and Design Principles
Memory Management
The allocator is designed to interface with a simulated memory system, enabling controlled heap extensions and memory allocations. This abstraction provides a foundation for safe and efficient memory operations.

Alignment
To ensure compatibility and performance, all allocated memory blocks are aligned to modern system architecture requirements. Proper alignment minimizes overhead and supports optimized memory access.

Efficiency
The design incorporates constraints to optimize memory usage and avoid excessive global storage. Most tracking data is stored within the allocated heap, maximizing available memory for applications.

Code Quality
The implementation prioritizes readability and maintainability, using static functions over macros to enhance code clarity and allow compiler optimizations. The modular structure enables future extensions and improvements.

This broader explanation conveys the project's goals and principles without delving too deeply into technical specifics, making it suitable for diverse audiences.
