/**
 * Do not submit your assignment with a main function in this file.
 * If you submit with a main function in this file, you will get a zero.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "debug.h"
#include "sfmm.h"
#include "errno.h"

// write value to address
#define PUT(p, val) (*(size_t *)(p) = (val))

//Read 8 bytes
#define GET(p) (*(size_t *)(p))

// read the size in the header
#define GET_SIZE(p) (GET(p) & ~0x7)
//get pre-allocated field
#define GET_PREALLOC(p) (GET(p) & 0x2)
//get allocated field
#define GET_ALLOC(p) (GET(p) & 0x1)
//get in-quicklist field
#define GET_INQUICKLIST(p) (GET(p) & 0x4)
//get footer from header address
#define GET_FOOTER_FROM_HEADER(p) ((sf_block *)((void *)(p) + GET_SIZE(p) - sizeof(sf_header)))
//get header from footer address
#define GET_HEADER_FROM_FOOTER(p) ((sf_block *)((void *)(p) - GET_SIZE(p) + sizeof(sf_header)))
//get header from payload
#define GET_HEADER_FROM_PAYLOAD(p) ((sf_block *)((void *)(p) - sizeof(sf_header)))

void *sf_malloc(size_t size)
{
	if (size == 0)
		return NULL;

	if (sf_mem_start() == sf_mem_end())
	{
		if (sf_mem_grow())
		{
			//set header in prologue
			// ((sf_block *)(sf_mem_start()))->header = 0x20 | THIS_BLOCK_ALLOCATED;
			PUT(sf_mem_start(), 0x20 | THIS_BLOCK_ALLOCATED);
			//set header in epologue
			// ((sf_block *)(sf_mem_end() - sizeof(sf_header)))->header = THIS_BLOCK_ALLOCATED;
			PUT(sf_mem_end() - sizeof(sf_header), THIS_BLOCK_ALLOCATED);
			//set header in first big free block
			// ((sf_block *)(sf_mem_start() + 32))->header =  (sf_mem_end() - sizeof(sf_header) - (sf_mem_start() + 32)) | PREV_BLOCK_ALLOCATED;
			PUT(sf_mem_start() + 32, (sf_mem_end() - sizeof(sf_header) - (sf_mem_start() + 32)) | PREV_BLOCK_ALLOCATED);
			//set footer
			// ((sf_block *)(sf_mem_start() + 32 + GET_SIZE((sf_block *)(sf_mem_start() + 32)) - 8))->header = ((sf_block *)(sf_mem_start() + 32))->header;
			PUT(GET_FOOTER_FROM_HEADER(sf_mem_start() + 32), GET(sf_mem_start() + 32));
			// debug("%zu", GET(sf_mem_start() + 32));
			// debug("%zu", (sf_mem_end() - sizeof(sf_header) - (sf_mem_start() + 32)) | PREV_BLOCK_ALLOCATED);
			//M size is 32
			size_t M = 32;
			//size of free block
			size_t sizeOfInitialFreeBlock = GET_SIZE((sf_block *)(sf_mem_start() + 32));
			// if (sizeOfInitialFreeBlock < 32) {
			// 	sf_errno = ENOMEM;
			// 	return NULL;
			// }
			//initialize each linked list in free list and add the free block in the right index
			for(int i = 0; i < NUM_FREE_LISTS; i++) {
				//initailize each free list
				sf_free_list_heads[i].body.links.next = &sf_free_list_heads[i];
				sf_free_list_heads[i].body.links.prev = &sf_free_list_heads[i];

				//add first free block to the right index
				// if (i == 0) {
				// 	//range if size == M
				// 	if (sizeOfInitialFreeBlock == M) {
				// 		sf_free_list_heads[i].body.links.next = (sf_block *)(sf_mem_start() + 32);
				// 		sf_free_list_heads[i].body.links.prev = (sf_block *)(sf_mem_start() + 32);
				// 		((sf_block *)(sf_mem_start() + 32))->body.links.next = &sf_free_list_heads[i];
				// 		((sf_block *)(sf_mem_start() + 32))->body.links.prev = &sf_free_list_heads[i];
				// 	}
				// } else {
				// 	//range of (M, 2M]
				// 	if (sizeOfInitialFreeBlock > M && sizeOfInitialFreeBlock <= (M << 1)) {
				// 		sf_free_list_heads[i].body.links.next = (sf_block *)(sf_mem_start() + 32);
				// 		sf_free_list_heads[i].body.links.prev = (sf_block *)(sf_mem_start() + 32);
				// 		((sf_block *)(sf_mem_start() + 32))->body.links.next = &sf_free_list_heads[i];
				// 		((sf_block *)(sf_mem_start() + 32))->body.links.prev = &sf_free_list_heads[i];
				// 	}
				// 	M = M << 1;
				// }


				//size always going to be 4056
				if (sizeOfInitialFreeBlock > M && sizeOfInitialFreeBlock <= (M << 1)) {
					sf_free_list_heads[i].body.links.next = (sf_block *)(sf_mem_start() + 32);
					sf_free_list_heads[i].body.links.prev = (sf_block *)(sf_mem_start() + 32);
					((sf_block *)(sf_mem_start() + 32))->body.links.next = &sf_free_list_heads[i];
					((sf_block *)(sf_mem_start() + 32))->body.links.prev = &sf_free_list_heads[i];
				} else if(i == NUM_FREE_LISTS - 1){
					if(sizeOfInitialFreeBlock > (M << 1)) {
						//last list
						sf_free_list_heads[i].body.links.next = (sf_block *)(sf_mem_start() + 32);
						sf_free_list_heads[i].body.links.prev = (sf_block *)(sf_mem_start() + 32);
						((sf_block *)(sf_mem_start() + 32))->body.links.next = &sf_free_list_heads[i];
						((sf_block *)(sf_mem_start() + 32))->body.links.prev = &sf_free_list_heads[i];
					}
				}
				if (i != 0) {
					M = M << 1;
				}
			}
			// sf_show_heap();
		}
		else
		{
			sf_errno = ENOMEM;
			return NULL;
		}
	}
	// debug("%p", sf_mem_start() + 4096);
	// debug("%p", sf_mem_end());

	// debug("%p", &(((sf_block *)(sf_mem_start()))->header));
	//find free block in quicklist

	//pointer to space allocated successfully
	sf_block *toBeAllocated = NULL;
	//search through quicklist
	//actual size needed
	size_t sizeNeeded = size + 8;
	size_t sizeFreeNeeded;
	//size of free block needed
	if (sizeNeeded < 32) {
		sizeFreeNeeded = 32;
	} else {
		// debug("%zu", sizeNeeded);
		if(sizeNeeded % 8 != 0) {
			sizeFreeNeeded = 8 - (sizeNeeded % 8) + sizeNeeded;
		} else {
			sizeFreeNeeded = sizeNeeded;
		}
	}
	// debug("%zu", sizeFreeNeeded);
	//get quicklist index needed
	size_t sizeMatchQuickIndex = sizeFreeNeeded / 8;
	//if there is a block avaliable, use it
	if(sizeMatchQuickIndex < NUM_QUICK_LISTS) {
		//check if there's free blocks
		if (sf_quick_lists[sizeMatchQuickIndex].length) {
			//get the first free block
			toBeAllocated = sf_quick_lists[sizeMatchQuickIndex].first;
			//set first pointer to next 
			sf_quick_lists[sizeMatchQuickIndex].first = sf_quick_lists[sizeMatchQuickIndex].first->body.links.next;
			//decrease length of quicklist
			sf_quick_lists[sizeMatchQuickIndex].length -= 1;
			//set in-quicklist to 0
			toBeAllocated->header = GET_SIZE(toBeAllocated) || GET_ALLOC(toBeAllocated) || GET_PREV_ALLOC(toBeAllocated);
		}
	}

	//if not found in quicklist, search through main free list
	if(!toBeAllocated) {
		size_t M = 32;
		//pointer to free block in main free list
		sf_block *freeBlockPtr = NULL;
		//search main free list for free space
		for (int i = 0; i < NUM_FREE_LISTS; i++) {
			//check if there's free blocks and if sizeNeeded <= M
			if ((sf_free_list_heads[i].body.links.next != &sf_free_list_heads[i] && sizeFreeNeeded <= M) || (i == 9 && sizeFreeNeeded > (M >> 1))) {
				freeBlockPtr = sf_free_list_heads[i].body.links.next;
				//go through list to find suitable free block
				while(freeBlockPtr != &sf_free_list_heads[i]) {
					//comapre size of free block and sizeFreeNeeded
					if (GET_SIZE(freeBlockPtr) >= sizeFreeNeeded) {
						toBeAllocated = freeBlockPtr;
						freeBlockPtr->body.links.prev->body.links.next = freeBlockPtr->body.links.next;
						freeBlockPtr->body.links.next->body.links.prev = freeBlockPtr->body.links.prev;
						break;
					} else {
					//go to next free block
						freeBlockPtr = sf_free_list_heads[i].body.links.next;
					}
				}
				//if a blocked found, break
				if(toBeAllocated) {
					break;
				}
			}
			M = M << 1;
		}
	}

	//use for calling second mem_grow if needed
	// void *currSpacePtr = sf_mem_end();
	while(!toBeAllocated) {
		// void *oldEnd = sf_mem_end();
		void *newSpacePtr;
		if((newSpacePtr = sf_mem_grow())) {
			// debug("Old: %p", oldEnd);
			// debug("New: %p", newSpacePtr);
			//coalescing free block before new space
			//set new epilogue
			PUT(sf_mem_end() - sizeof(sf_header), THIS_BLOCK_ALLOCATED);
			//set old epilogue to header with new free size
			PUT(GET_HEADER_FROM_PAYLOAD(newSpacePtr), (sf_mem_end() - newSpacePtr) | GET_PREALLOC(GET_HEADER_FROM_PAYLOAD(newSpacePtr)));
			//if previous block is free
			//newSpacePtr at the end of the heap
			if(GET_PREALLOC(GET_HEADER_FROM_PAYLOAD(newSpacePtr)) == 0) {
				//get previous block
				//pointer at header
				sf_block *prevBlockPtr = GET_HEADER_FROM_FOOTER((newSpacePtr - 16));
				// debug("PrevFreeBlock: %zu", GET_SIZE(prevBlockPtr));
				//set header of new free block
				PUT(prevBlockPtr, (GET_SIZE(prevBlockPtr) + GET_SIZE(GET_HEADER_FROM_PAYLOAD(newSpacePtr))) | GET_PREALLOC(prevBlockPtr));
				// debug("%zu", (sf_mem_end() - newSpacePtr));
				// debug("%p", newSpacePtr);
				// debug("%p", sf_mem_end());
				//set footer of new free block
				PUT(GET_FOOTER_FROM_HEADER(prevBlockPtr), GET(prevBlockPtr));
				//set toBeAllocated to new free block if size is enough
				// debug("NewFreeBlock: %zu", GET_SIZE(prevBlockPtr));
				if(GET_SIZE(prevBlockPtr) >= sizeFreeNeeded) {
					// debug("%zu", sizeFreeNeeded);
					//remove free block from main free list if using this free block
					// sf_block *temp = prevBlockPtr->body.links.prev;
					prevBlockPtr->body.links.prev->body.links.next = prevBlockPtr->body.links.next;
					prevBlockPtr->body.links.next->body.links.prev = prevBlockPtr->body.links.prev;
					toBeAllocated = prevBlockPtr;
					// sf_show_heap();
					// debug("%p", toBeAllocated);
				}
			} else {
				if(GET_SIZE(GET_HEADER_FROM_PAYLOAD(newSpacePtr)) >= sizeFreeNeeded) {
					toBeAllocated = GET_HEADER_FROM_PAYLOAD(newSpacePtr);
				}
			}
		} else {
			//no more space
			sf_errno = ENOMEM;
			return NULL;
		}
	}




	//if free block found, allocate space
	if(toBeAllocated) {
		//original block size
		size_t toBeAllocatedBlockSize = GET_SIZE(toBeAllocated);
		// debug("%zu", GET_SIZE(toBeAllocated));

		//choose to split or not
		//choose split else don't
		if(toBeAllocatedBlockSize - sizeFreeNeeded >= 32) {
			//change header of allocated block
			toBeAllocated->header = sizeFreeNeeded | GET_PREALLOC(toBeAllocated) | THIS_BLOCK_ALLOCATED;


			//new free block after allocation
			// debug("%zu", sizeFreeNeeded);
			sf_block *newFreeBlockPtr = ((void *)toBeAllocated + sizeFreeNeeded);
			// debug("%p", newFreeBlockPtr);
			// debug("%p", toBeAllocated);
			//set header for new free block
			newFreeBlockPtr->header = (toBeAllocatedBlockSize - sizeFreeNeeded) | PREV_BLOCK_ALLOCATED;
			//set footer to header values
			((sf_block *)GET_FOOTER_FROM_HEADER(newFreeBlockPtr))->header = newFreeBlockPtr->header;

			//add new free block to main free list
			size_t M = 32;
			sf_block *temp;
			for(int i = 0; i < NUM_FREE_LISTS; i++) {
					//add first free block to the right index
					if (i == 0) {
						//range if size == M
						//FIX THIS FOR ADDING TO FREE LIST ---------------------------------------------------------------------------------------------------------
						if ((toBeAllocatedBlockSize - sizeFreeNeeded) == M) {
							// 1->3, 1 is dummy, 3 is next block, 2 is inserting block
							//preserve 3
							temp = sf_free_list_heads[i].body.links.next;
							//set 1->2
							sf_free_list_heads[i].body.links.next = newFreeBlockPtr;
							//set 2->3
							newFreeBlockPtr->body.links.next = temp;
							//set 2->1
							newFreeBlockPtr->body.links.prev = &sf_free_list_heads[i];
							//set 3->2
							temp->body.links.prev = newFreeBlockPtr;
						}
					} else if ((toBeAllocatedBlockSize - sizeFreeNeeded) > M && (toBeAllocatedBlockSize - sizeFreeNeeded) <= (M << 1)) {
							// 1->3, 1 is dummy, 3 is next block, 2 is inserting block
							//preserve 3
							temp = sf_free_list_heads[i].body.links.next;
							//set 1->2
							sf_free_list_heads[i].body.links.next = newFreeBlockPtr;
							//set 2->3
							newFreeBlockPtr->body.links.next = temp;
							//set 2->1
							newFreeBlockPtr->body.links.prev = &sf_free_list_heads[i];
							//set 3->2
							temp->body.links.prev = newFreeBlockPtr;
					} else if(i == NUM_FREE_LISTS - 1) {
						// 1->3, 1 is dummy, 3 is next block, 2 is inserting block
						if((toBeAllocatedBlockSize - sizeFreeNeeded) > (M << 1)) {
							//preserve 3
							temp = sf_free_list_heads[i].body.links.next;
							//set 1->2
							sf_free_list_heads[i].body.links.next = newFreeBlockPtr;
							//set 2->3
							newFreeBlockPtr->body.links.next = temp;
							//set 2->1
							newFreeBlockPtr->body.links.prev = &sf_free_list_heads[i];
							//set 3->2
							temp->body.links.prev = newFreeBlockPtr;
						}
					}
					if (i != 0) {
						M = M << 1;
					}
			} 
		} else {
			// not splitting
			//change header of allocated block
			toBeAllocated->header = toBeAllocatedBlockSize | GET_PREALLOC(toBeAllocated) | THIS_BLOCK_ALLOCATED;
		}


		// debug("%zu", GET_SIZE(toBeAllocated));
		// sf_show_heap();
		return toBeAllocated->body.payload;
	}

	//if no free block found, return NULL
	sf_errno = ENOMEM;
	return toBeAllocated;
}

void sf_free(void *pp)
{
	if(
	!pp || //if pp is NULL
	!((size_t)pp & 7) || //if not 8-byte aligned
	(GET_SIZE(GET_HEADER_FROM_PAYLOAD(pp)) < 32 ) || //if size is < 32
	!(GET_SIZE(GET_HEADER_FROM_PAYLOAD(pp)) % 8) || //if size is not a multiple of 8
	(GET_HEADER_FROM_PAYLOAD(pp) < (sf_block *)(sf_mem_start() + 32)) || //if header is before end of prologue
	(((void *)(GET_FOOTER_FROM_HEADER(GET_HEADER_FROM_PAYLOAD(pp))) + 8) > (sf_mem_end() - 8)) ||//if footer end is after epilogue
	!(GET_ALLOC(GET_HEADER_FROM_PAYLOAD(pp))) || //if block is already free
	GET_INQUICKLIST(GET_HEADER_FROM_PAYLOAD(pp)) || //block is in quicklist
	(!(GET_PREALLOC(GET_HEADER_FROM_PAYLOAD(pp))) && GET_ALLOC((void *)(GET_HEADER_FROM_PAYLOAD(pp)) - 8)) //prev_alloc of current is 0 but previous block is allocated
	) {
		abort();
	}
	abort();
}

void *sf_realloc(void *pp, size_t rsize)
{
	// TO BE IMPLEMENTED
	abort();
}

void *sf_memalign(size_t size, size_t align)
{
	// TO BE IMPLEMENTED
	abort();
}
