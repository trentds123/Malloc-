/*
 * mm.c
 *
 * Name: Trent Seidel
 *
* Thid project implements a dynamic memory allocator, which from my design was by using a segregated list, The allocator is desighned to provide efficiant memory allocation and freeing for varios types of memeory requests. It does this by usiing mechinisms like heap initialazation, dynamic extension, block coalescing which reduces fragmentation and both allocation and deallocation are optimised using best fit.he design emphasizes minimizing fragmentation and improving allocation efficiency by categorizing free blocks into size-specific lists and ensuring heap integrity through consistency checks. 
 */
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>
#include <stdbool.h>

#include "mm.h"
#include "memlib.h"

/*
 *
 */
// #define DEBUG

#ifdef DEBUG
// When debugging is enabled, the underlying functions get called
#define dbg_printf(...) printf(__VA_ARGS__)
#define dbg_assert(...) assert(__VA_ARGS__)
#else
// When debugging is disabled, no code gets generated
#define dbg_printf(...)
#define dbg_assert(...)
#endif // DEBUG

// do not change the following!
#ifdef DRIVER
// create aliases for driver tests
#define malloc mm_malloc
#define free mm_free
#define realloc mm_realloc
#define calloc mm_calloc
#define memset mm_memset
#define memcpy mm_memcpy
#endif // DRIVER
#define ALIGNMENT 16

static const size_t WSIZE =  8;
static const size_t DSIZE =  16;
//static const size_t CHUNKSIZE =  (1<<12);
static void *heap_listp;  // Pointer to first block
void *Head_of_linked_list = NULL; // head stores address of the 1st node in linked list.



//These were macros that were insipred by Computer  Systems A programmers Prospective page 893
//Converted macros into functions becuase project wont allow macros.
static void *extend_heap(size_t words);
static void place(void *bp, size_t asize);
static void *find_best_fit_in_freeList(size_t asize);
static void *coalesce(void *bp);
bool check_heap_consistency(void);
static inline uint64_t *prev_linkedlist_pointer(void* bp);
static inline uint64_t *next_linkedlist_pointer(void* bp);
static void set_pointer(void *bp, void *node);
static void add_block_to_free_list(void *bp);
static void remove_block_from_free_list(void *bp);

//Book has a max function but I am not famailiar with the syntax so I made a simple one aswell as a MIN assumting I may need it at some point
static uint64_t max(uint64_t x, uint64_t y) {
    return (x > y) ? x : y;
}

static uint64_t min(uint64_t x, uint64_t y) {
    if (x <= y) {
        return x;
    } else {
        return y;
    }
}

// Pack a size and allocated bit into a word
static uint64_t PACK(uint64_t size, uint64_t alloc) {
    return size | alloc;
}


// read at the address where blockpointer is
static uint64_t GET(void *bp) {
    return *(uint64_t *)bp;
}
//Write at Adress where the blockPointer is
static void PUT(void *bp, uint64_t val) {
    
    (*(uint64_t *)bp) = val;
   
}


/* Read the size and allocated fields from address p */
static uint64_t GET_SIZE(void *bp) {
    return GET(bp) & ~0x7;
}

static uint64_t GET_ALLOC(void *bp) {
    return GET(bp) & 0x1;
}


// Get the header and footer pointer when given the block pointer.
static uint64_t *HDRP(void *bp) {
    return ((uint64_t *)(bp - 8));
}


static uint64_t *FTRP(void *bp) {
    return (uint64_t*)(bp + GET_SIZE(HDRP(bp)) - DSIZE);
}

// Get the prevois and next block pointer
static inline uint64_t *PREV_BLKP(void* bp){
    return (uint64_t *)((bp) - GET_SIZE((bp) - DSIZE));
}

static inline uint64_t *NEXT_BLKP(void* bp){
    return (uint64_t *)((bp) + GET_SIZE(HDRP(bp)));
}


//chuck size is what will extend the heap when it doesnt have enouph memory. 
// rounds up to the nearest multiple of ALIGNMENT
static uint64_t align(uint64_t x)
{
    return ALIGNMENT * ((x+ALIGNMENT-1)/ALIGNMENT);
}



//these two class functions are where the free blocks were keep track of the prev and next pointers in the linked list.

static inline uint64_t *prev_linkedlist_pointer(void* bp){
    return (uint64_t *)((bp));
}

//postion of next in block pointer will be 8 more than prev becuase they are next to eachother.
static inline uint64_t *next_linkedlist_pointer(void* bp){
    return (uint64_t *)((8  + (bp)));
}

// This is an essential function in my adding and probably removing becuase I was trying to put the pointers in the linked list and move them isng PUT
//this function is the same as PUT but works with pointers for the linked list.


static void set_pointer(void *bp, void *node ){
    //assigning the address of the node to the first size_t of memory thats pointed by bp
    *(size_t *)(bp) = (size_t)(node);
}



// helper function for free list
static void add_block_to_free_list(void *bp){
    //printf("entering in add block\n");

//when im adding go line by line,, print the content of the prev next pointer   make sure its not garbige

    if(Head_of_linked_list == NULL){
        //store address of first block.
        Head_of_linked_list = bp;
        //set the current blocks Prev and Next to null
        set_pointer(prev_linkedlist_pointer(bp),NULL);
        set_pointer(next_linkedlist_pointer(bp),NULL);

    }
    else{

        
        //when there is already a node, and you add a new node...
        //1) current block next pointer to next block in the list
        //2) current previous pointer will be set to NULL
        void* ptr = Head_of_linked_list;
        Head_of_linked_list = bp;

        set_pointer(next_linkedlist_pointer(bp), ptr);
        set_pointer(prev_linkedlist_pointer(bp),NULL);

        //3) pointer will have been updated from setp 1) then
        // Prevois pointer in the NEXT block will be set to current bp

        set_pointer(prev_linkedlist_pointer(ptr), bp);
        //4) update the head of the linked list to the new added block.
    } 
    //printf("After add %p\n",bp);
   
}
// helper function for free list

static void remove_block_from_free_list(void *bp){
    // printf("i am removing");
    //     printf("bp = %p\n", bp);

    // printf("Head of LL %p\n", Head_of_linked_list);

    if (bp == NULL) {
        //error checker incase of a null pointer trying to be removed.
        fprintf(stderr, "Error removing a NULL block from the free list.\n");
        return; 
    }    

    //pointer that points to another pointer. then derefrence for the address to actually get the prev and next block in the free

    size_t *next_block_in_freeList = (uint64_t *)(*(next_linkedlist_pointer(bp)));
    size_t *prev_block_in_freeList = (uint64_t *)(*(prev_linkedlist_pointer(bp)));

    //rewrite remove into 4 cases:

    //Case 1) Theres only 1 thing in the list
    if(prev_block_in_freeList == NULL && next_block_in_freeList == NULL){
        Head_of_linked_list = NULL;
    }

    //case 2) Your at the beggining of the block, but there are blocks after
    else if(prev_block_in_freeList == NULL && next_block_in_freeList != NULL){
        set_pointer(prev_linkedlist_pointer(next_block_in_freeList), NULL);
        Head_of_linked_list = next_block_in_freeList;
    }
    //case 3) In the middle of the linked list meaning both are not null
    else if(prev_block_in_freeList != NULL && next_block_in_freeList != NULL){
        set_pointer(prev_linkedlist_pointer(next_block_in_freeList),prev_block_in_freeList);
        set_pointer(next_linkedlist_pointer(prev_block_in_freeList),next_block_in_freeList);
    }
    //case 4) If your at the last block in the free list, then the prev block pointer is set to null
    else {
            if(prev_block_in_freeList != NULL && next_block_in_freeList == NULL){
                set_pointer(next_linkedlist_pointer(prev_block_in_freeList), NULL);

            }
    }
}
static void *coalesce(void *bp){
    size_t prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(bp)));
    size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
    size_t size = GET_SIZE(HDRP(bp));
    //Before merdging the free blocks, Must remove the block from the free list
    //becuase then if it was in the free list after merdging it would not longer exist as seperate blocks.  

    if(prev_alloc && next_alloc) {//CASE 1: no coalesce needed, but I still need to add the current free block to the free list
        // add_block_to_free_list(bp);
        return bp;// exit function 
    } 
    else if(prev_alloc && !next_alloc) // CASE 2: merdge the next block with the current, remove the next block and the current
    {
        size += GET_SIZE(HDRP(NEXT_BLKP(bp)));
        remove_block_from_free_list(bp);
        remove_block_from_free_list(NEXT_BLKP(bp));
        PUT(HDRP(bp), PACK(size, 0));
        PUT(FTRP(bp), PACK(size, 0));
       
    }
    else if (!prev_alloc && next_alloc) // CASE 3: merdge the prev block with the current, remove the prev block and current from the free list
    {
        size += GET_SIZE(HDRP(PREV_BLKP(bp)));
                remove_block_from_free_list(bp);
        remove_block_from_free_list(PREV_BLKP(bp));
        PUT(FTRP(bp), PACK(size, 0));
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
        bp = PREV_BLKP(bp);
    }
    else // CASE 4
    {
        //combine both prev current and next block, remove both the next and prev block from the list*
        size += GET_SIZE(HDRP(PREV_BLKP(bp))) + GET_SIZE(FTRP(NEXT_BLKP(bp)));
        remove_block_from_free_list(NEXT_BLKP(bp));
        remove_block_from_free_list(PREV_BLKP(bp));
        remove_block_from_free_list(bp);

        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
        PUT(FTRP(NEXT_BLKP(bp)), PACK(size, 0));
        bp = PREV_BLKP(bp);

    }
    //after everythng has been merdged together, need to add this back to the free list as one block
    add_block_to_free_list(bp);
    return bp;


}

//the next pointers, I was not returning the actaul next address but where the next pointer was.

static void *extend_heap(size_t words){
    char *bp;
    size_t size;

    //allocate an even number of words to maintian allighnment
    size =  (words % 2) ? (words +1) * WSIZE : words * WSIZE;
    if ((long)(bp = mem_sbrk(size)) == -1){
        return NULL;
    }

    
    //begin to create 
    // initialize free block header and footer and the epilioge header
    PUT(HDRP(bp),PACK(size,0));// free block header
    PUT(FTRP(bp),PACK(size,0)); // free block footer
    PUT(HDRP(NEXT_BLKP(bp)), PACK(0,1)); //New epilouge header
    //size_t currentSize = GET_SIZE(HDRP(bp));
    // printf("currentSize in extend heap = %ld\n", currentSize);
    add_block_to_free_list(bp);
    // coalesce if the prev blovk was free
    return coalesce(bp);
}

//changed from first fit to best fit.
static void *find_best_fit_in_freeList(size_t requiredSize) {

    void *best_fit = NULL;
    size_t best_fit_size = SIZE_MAX; // Begin with max possiblee size

    if (Head_of_linked_list != NULL) {  //first check the head is not null, otherwise segfualt. 

        void *bp = Head_of_linked_list;

        while (bp != NULL) {
            // keep track of current size
            size_t currentSize = GET_SIZE(HDRP(bp));

            //find a block that will fit
            if (currentSize >= requiredSize) { 
                // Check to see if this block is better than the fit that was found previosly
                if (currentSize < best_fit_size) {
                    best_fit = bp;
                    best_fit_size = currentSize;
                    //if its exact fit then stop searching
                    if (currentSize == requiredSize) break;
                }
            }

            // Move to the next block in the free list
            bp = (uint64_t *)(*(next_linkedlist_pointer(bp)));
        }

        // remove a block from the free list when best fit is successful
        if (best_fit != NULL) {
            remove_block_from_free_list(best_fit);
        }
    }

    // Return the best fit found or NULL if no suitable block was found
    return best_fit;
}


//code orginally inspired from the book, page 920 A systems programmer perpective.

/*the place function actaully is more like a splice. It will take a free block and a requested size to be allocated
Then it will be split up into one allocated block and 1 shorter free block. This free block is the difference between
original free block given and the requested size to be allocated. 
*/


//Reminder to self: the block has already been removed from the free list.
int a = 0;
static void place(void *bp, size_t requestedSize){
    size_t currentSize = GET_SIZE(HDRP(bp)); //get current size of the freeblock from pb
    //If the remainderof the block after splitting would be greater than or equal 
    //to the minimum blocksize, then we go ahead and split the block
    // check if the free block is big enouph to be split
    // printf("currentSize = %ld\n", currentSize);
    // printf("bp = %p\n", bp);
    remove_block_from_free_list(bp);
    if((currentSize - requestedSize) >= (2*DSIZE)){
        //SPLITTING BLOCK:
        size_t adjusted_Free_Block_Size = currentSize - requestedSize;


        //mark the current block as allocated,
        PUT(HDRP(bp), PACK(requestedSize, 1));
        PUT(FTRP(bp), PACK(requestedSize, 1));
        
        //move bp so that the new free block will start here
        bp = NEXT_BLKP(bp);
        // printf("bp = %p\n", bp);
        
        //  establish the new free block
        PUT(HDRP(bp), PACK(adjusted_Free_Block_Size, 0));
        PUT(FTRP(bp), PACK(adjusted_Free_Block_Size, 0));
        add_block_to_free_list(bp);//I think this will be correct becuase the block will move to the next..

    }
    else{
        //when the block cannot be split, allocate it the original size back and dont add anything.
        PUT(HDRP(bp), PACK(currentSize, 1));
        PUT(FTRP(bp), PACK(currentSize, 1));
        // printf("test place function\n");
    }
}
/* mm_init - Initialize the memory manager */
bool mm_init(void)
{  

    // create intial empty heap
    if ((heap_listp = mm_sbrk(4*WSIZE)) == (void *)-1) {
        return false;
    }

    PUT(heap_listp, 0); // padding
    PUT(heap_listp + (1*WSIZE), PACK(DSIZE, 1)); // prologue head
    PUT(heap_listp + (2*WSIZE), PACK(DSIZE, 1)); // prologue footer
    PUT(heap_listp + (3*WSIZE), PACK(0, 1)); // epilogue header
    heap_listp += (2*WSIZE);

    //put heap check here



    // extend the empty heap wtotj free block of chunksize bytes
    Head_of_linked_list = NULL;

    //if (extend_heap(CHUNKSIZE/WSIZE) == NULL) {return false;}
   

    //put heap check here




    return true;

//check that header and footer are both equal.
}


//code inspired from the book chapter 9 page 896
//mm_free frees a block and uses boundary-tag coalescing to merge it
//with any adjacent free blocks in constant time
void mm_free(void *bp){
    // printf("begginig of free\n");
    if(bp == NULL){
        return;
    }

    //put heap check here


    size_t size = GET_SIZE(HDRP(bp));

    // printf("this is the bp being freed %p\n",bp);
    PUT(HDRP(bp),PACK(size,0));
    PUT(FTRP(bp),PACK(size,0));

    //put heap check here

    add_block_to_free_list(bp);
    coalesce(bp);

    //put heap check here


}



//code inspired from the book chapter 9 page 896

void *mm_malloc(size_t size)
{
    // printf("beggining of malloc\n");
    //put heap check here

    size_t asize;/* Adjusted block size */
    size_t extendsize; /* Amount to extend heap if no fit */
    char *bp;

    /* Ignore spurious requests */
    if (size == 0){
        return NULL;
    }
    /* Adjust block size to include overhead and alignment reqs. */
    if (size <= DSIZE){
        asize = 2*DSIZE;
    }   
    else{
        asize = DSIZE * ((size + (DSIZE) + (DSIZE-1)) / DSIZE);
    }

    // printf("before Find ff\n");

    /* Search the free list for a fit */
    // if function finds a first fit (not null), and Not at the beggining of the Linked list:
    //first fit will set the bp to the first
    if ((bp = find_best_fit_in_freeList(asize)) != NULL) {
        
        //if(Head_of_linked_list!= NULL){
            //printf("line 496 bp %p", bp);

            place(bp, asize);
            return bp;
        //}
        
        //take the free block and splice it in place
    
        //put heap check here
        
    }
    /* No fit found. Get more memory and place the block */
    extendsize = asize;
    if ((bp = extend_heap(extendsize/WSIZE)) == NULL){
        return NULL;
    }

    //printf(" end of malloc  before place bp = %p\n", bp);
    
    place(bp,asize);
    
    //put heap check here

    return bp;
}




/*
 * realloc
 */
void* realloc(void* oldptr, uint64_t size)
{
    //check to see if the old pointer is null if so call malloc
    if(oldptr == NULL){
        return malloc(size);
    }
    //check to see if size is 0 if so free the block, return null
    if(size == 0){
        free(oldptr);
        return NULL;
    }
    
    size_t oldsize = GET_SIZE(HDRP(oldptr));
    //size will need to be adjusted for overhead alihnment
    size_t asize = 0;

    if(size <= DSIZE){
        //accomadate for small overhead if its too small
        asize = 2 * DSIZE;
    }
    else{
        //requested size + overhead, and then alighn it
        asize = DSIZE * ((size + (DSIZE) + (DSIZE - 1)) / DSIZE);
    }
    //check that new size is <= old size
    if(asize <= oldsize){
        return oldptr;
    }
    //copy old data, free the old block and allovate a new one.
    void* newBP = malloc(size);
    if(newBP == NULL){
        //failed to allocate
        return NULL;
    }
    //old data get copied
    size_t copy = oldsize - WSIZE;
    if(size < copy){
        copy = size;
    }
    memcpy(newBP,oldptr,copy);
    //PRevent memory leaks by freeing the old block becuase its data has been copiedd to a new location
    free(oldptr);
    //return new pointer to new allocated mem
    return newBP;

}

/*
 * calloc
 * This function is not tested by mdriver, and has been implemented for you.
 */


void* calloc(uint64_t nmemb, uint64_t size)
{
    void* ptr;
    size *= nmemb;
    ptr = malloc(size);
    if (ptr) {
        memset(ptr, 0, size);
    }
    return ptr;
}

/*
 * Returns whether the pointer is in the heap.
 * May be useful for debugging.
 */
static bool in_heap(const void* p)
{
    return p <= mm_heap_hi() && p >= mm_heap_lo();

}

/*
 * Returns whether the pointer is aligned.
 * May be useful for debugging.
 */
static bool aligned(const void* p)
{
    uint64_t ip = (uint64_t) p;
    return align(ip) == ip;
}


/*
 * mm_checkheap
 * You call the function via mm_checkheap(__LINE__)
 * The line number can be used to print the line number of the calling
 * function where there was an invalid heap.
 */




/////////////////////////////////
//HEAP CHECKER HELPER FUNCTIONS
/////////////////////////////////

//returns true if the block is allocated and false if not.
bool is_block_free (void *bp) {
    return !GET_ALLOC(HDRP(bp));
}



/*1) ensure the heap is set up correctly from the start
    2) after operations that modify the heap, call this function 
    to find where the inconsistoncies lie */

bool check_heap_consistency() {
    char *current_block_ptr = heap_listp;

    // Check prologue block
    if (GET_SIZE(HDRP(heap_listp)) != DSIZE || GET_ALLOC(HDRP(heap_listp)) != 1) {
        printf("Prologue block size or allocation status incorrect\n");
        return false; // Indicate inconsistency
    }

    // Iterate through every block in the heap starting from prologue all the way to epilogue
    for (current_block_ptr = (char*)NEXT_BLKP(heap_listp); GET_SIZE(HDRP(current_block_ptr)) > 0; current_block_ptr = (char*)NEXT_BLKP(current_block_ptr)) {
        // Check the address alignment
        if (((size_t)current_block_ptr % 8) != 0) {
            printf("Block at %p is not aligned to the 8-byte boundary\n", current_block_ptr);
            return false; // Indicate inconsistency
        }

        // Check if the block is within heap bounds
        if (!in_heap(current_block_ptr)) {
            printf("Block at %p is outside of the heap bounds\n", current_block_ptr);
            return false; // Indicate inconsistency
        }

        // Check header and footer match for each block
        if (GET_SIZE(HDRP(current_block_ptr)) != GET_SIZE(FTRP(current_block_ptr)) ||
            GET_ALLOC(HDRP(current_block_ptr)) != GET_ALLOC(FTRP(current_block_ptr))) {
            printf("Header and footer do not match for block at %p\n", current_block_ptr);
            return false; // Indicate inconsistency
        }
    }

    // After iteration, ensure the epilogue block size is 0, and it should be marked as allocated
    if (GET_SIZE(HDRP(current_block_ptr)) != 0 || GET_ALLOC(HDRP(current_block_ptr)) != 1) {
        printf("Epilogue block size or allocation status incorrect\n");
        return false; // Indicate inconsistency
    }

    return true; // Heap is consistent
}




bool mm_checkheap(int line_number)

{
//#ifdef DEBUG
     

//#endif // DEBUG
    return true;
}
