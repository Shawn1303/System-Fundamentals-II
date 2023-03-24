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
	size_t sizeMatchQuickIndex = ((sizeFreeNeeded - 32) / 8);
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
			toBeAllocated->header = GET_SIZE(toBeAllocated) || GET_ALLOC(toBeAllocated) || GET_PREALLOC(toBeAllocated);
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
		sf_block *prevBlockPtr;
		if((newSpacePtr = sf_mem_grow())) {
			// debug("Old: %p", oldEnd);
			// debug("New: %p", newSpacePtr);
			//coalescing free block before new space
			//set new epilogue
			PUT(sf_mem_end() - sizeof(sf_header), THIS_BLOCK_ALLOCATED);
			//set old epilogue to header with new free size
			PUT(GET_HEADER_FROM_PAYLOAD(newSpacePtr), (sf_mem_end() - newSpacePtr) | GET_PREALLOC(GET_HEADER_FROM_PAYLOAD(newSpacePtr)));
			//set footer of old epilogue
			PUT(GET_FOOTER_FROM_HEADER(GET_HEADER_FROM_PAYLOAD(newSpacePtr)), GET(GET_HEADER_FROM_PAYLOAD(newSpacePtr)));
			//if previous block is free
			//newSpacePtr at the end of the heap
			if(GET_PREALLOC(GET_HEADER_FROM_PAYLOAD(newSpacePtr)) == 0) {
				//get previous block
				//pointer at header
				prevBlockPtr = GET_HEADER_FROM_FOOTER((newSpacePtr - 16));
				//remove free block from main free list if using this free block
				prevBlockPtr->body.links.prev->body.links.next = prevBlockPtr->body.links.next;
				prevBlockPtr->body.links.next->body.links.prev = prevBlockPtr->body.links.prev;
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
					// prevBlockPtr->body.links.prev->body.links.next = prevBlockPtr->body.links.next;
					// prevBlockPtr->body.links.next->body.links.prev = prevBlockPtr->body.links.prev;
					toBeAllocated = prevBlockPtr;
					prevBlockPtr = NULL;
					// sf_show_heap();
					// debug("%p", toBeAllocated);
				}
			} else {
				if(GET_SIZE(GET_HEADER_FROM_PAYLOAD(newSpacePtr)) >= sizeFreeNeeded) {
					toBeAllocated = GET_HEADER_FROM_PAYLOAD(newSpacePtr);
				}
			}
		} else {
			//add new free block to main free list
			if(prevBlockPtr) {
				size_t M = 32;
				sf_block *temp;
				for(int i = 0; i < NUM_FREE_LISTS; i++) {
						//add first free block to the right index
						if (i == 0) {
							//range if size == M
							if (GET_SIZE(prevBlockPtr) == M) {
								// 1->3, 1 is dummy, 3 is next block, 2 is inserting block
								//preserve 3
								temp = sf_free_list_heads[i].body.links.next;
								//set 1->2
								sf_free_list_heads[i].body.links.next = prevBlockPtr;
								//set 2->3
								prevBlockPtr->body.links.next = temp;
								//set 2->1
								prevBlockPtr->body.links.prev = &sf_free_list_heads[i];
								//set 3->2
								temp->body.links.prev = prevBlockPtr;
							}
						} else if (GET_SIZE(prevBlockPtr) > M && GET_SIZE(prevBlockPtr) <= (M << 1)) {
								// 1->3, 1 is dummy, 3 is next block, 2 is inserting block
								//preserve 3
								temp = sf_free_list_heads[i].body.links.next;
								//set 1->2
								sf_free_list_heads[i].body.links.next = prevBlockPtr;
								//set 2->3
								prevBlockPtr->body.links.next = temp;
								//set 2->1
								prevBlockPtr->body.links.prev = &sf_free_list_heads[i];
								//set 3->2
								temp->body.links.prev = prevBlockPtr;
						} else if(i == NUM_FREE_LISTS - 1) {
							// 1->3, 1 is dummy, 3 is next block, 2 is inserting block
							if(GET_SIZE(prevBlockPtr) > (M << 1)) {
								// 1->3, 1 is dummy, 3 is next block, 2 is inserting block
								//preserve 3
								temp = sf_free_list_heads[i].body.links.next;
								//set 1->2
								sf_free_list_heads[i].body.links.next = prevBlockPtr;
								//set 2->3
								prevBlockPtr->body.links.next = temp;
								//set 2->1
								prevBlockPtr->body.links.prev = &sf_free_list_heads[i];
								//set 3->2
								temp->body.links.prev = prevBlockPtr;
							}
						}
						if (i != 0) {
							M = M << 1;
						}
				} 
				//reset prevBlockPtr
				prevBlockPtr = NULL;
			}
			//no more space
			// sf_show_heap();
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
			((sf_block *)GET_FOOTER_FROM_HEADER(newFreeBlockPtr))->header = GET(newFreeBlockPtr);

			//add new free block to main free list
			size_t M = 32;
			sf_block *temp;
			for(int i = 0; i < NUM_FREE_LISTS; i++) {
					//add first free block to the right index
					if (i == 0) {
						//range if size == M
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
		// debug("malloc");
		// sf_show_heap();
		return toBeAllocated->body.payload;
	}

	//if no free block found, return NULL
	sf_errno = ENOMEM;
	return toBeAllocated;
}

void sf_free(void *pp)
{
	//see if ptr valid
	if(
	!pp || //if pp is NULL
	((size_t)pp & 7) || //if not 8-byte aligned
	(GET_SIZE(GET_HEADER_FROM_PAYLOAD(pp)) < 32 ) || //if size is < 32
	(GET_SIZE(GET_HEADER_FROM_PAYLOAD(pp)) % 8) || //if size is not a multiple of 8
	(GET_HEADER_FROM_PAYLOAD(pp) < (sf_block *)(sf_mem_start() + 32)) || //if header is before end of prologue
	(((void *)(GET_FOOTER_FROM_HEADER(GET_HEADER_FROM_PAYLOAD(pp))) + 8) > (sf_mem_end() - 8)) ||//if footer end is after epilogue
	!(GET_ALLOC(GET_HEADER_FROM_PAYLOAD(pp))) || //if block is already free
	GET_INQUICKLIST(GET_HEADER_FROM_PAYLOAD(pp)) || //block is in quicklist
	(!(GET_PREALLOC(GET_HEADER_FROM_PAYLOAD(pp))) && GET_ALLOC((void *)(GET_HEADER_FROM_PAYLOAD(pp)) - 8)) //prev_alloc of current is 0 but previous block is allocated
	) {
		abort();
	}
	//header of block to be freed
	sf_block *freeBlock = GET_HEADER_FROM_PAYLOAD(pp);

	//size of block to be freed
	size_t toBeFreeBlockSize = GET_SIZE(freeBlock);
	//if block fit in quick list
	if(toBeFreeBlockSize <= (32 + (NUM_QUICK_LISTS - 1) * 8)) {
		size_t quickListIndex = (toBeFreeBlockSize - 32) / 8;
		//check if there's free blocks
		if (sf_quick_lists[quickListIndex].length == QUICK_LIST_MAX) {
			//flush list
			sf_block *flushBlock = sf_quick_lists[quickListIndex].first;
			while(flushBlock) {
				//remove from quick list
				sf_quick_lists[quickListIndex].first = flushBlock->body.links.next;
				sf_quick_lists[quickListIndex].length -= 1;

				//set inQuickList to 0 and alloc to 0
				PUT(flushBlock, GET_SIZE(flushBlock) | GET_PREALLOC(flushBlock));
				//set footer
				PUT(GET_FOOTER_FROM_HEADER(flushBlock), GET(flushBlock));
				//set next block's prev alloc to 0
				PUT(
					((void *)GET_FOOTER_FROM_HEADER(flushBlock) + 8), 
					GET_SIZE(((void *)GET_FOOTER_FROM_HEADER(flushBlock) + 8)) | 
					GET_ALLOC(((void *)GET_FOOTER_FROM_HEADER(flushBlock) + 8)) | 
					GET_INQUICKLIST(((void *)GET_FOOTER_FROM_HEADER(flushBlock) + 8))
					);

				//coalesce flushblock with previous block
				if(!GET_PREALLOC(flushBlock)) {
					//get header of previous block
					sf_block *prevBlock = GET_HEADER_FROM_FOOTER((void *)flushBlock - 8);

					//remove prevBlock from main free
					prevBlock->body.links.prev->body.links.next = prevBlock->body.links.next;
					prevBlock->body.links.next->body.links.prev = prevBlock->body.links.prev;

					//merge prevBlock and flushBlock
					PUT(prevBlock, (GET_SIZE(prevBlock) + GET_SIZE(flushBlock)) | GET_PREALLOC(prevBlock));
					//set footer
					PUT(GET_FOOTER_FROM_HEADER(prevBlock), GET(prevBlock));
					//set next block's prev alloc to 0
					PUT(
						((void *)GET_FOOTER_FROM_HEADER(flushBlock) + 8), 
						GET_SIZE(((void *)GET_FOOTER_FROM_HEADER(flushBlock) + 8)) | 
						GET_ALLOC(((void *)GET_FOOTER_FROM_HEADER(flushBlock) + 8)) | 
						GET_INQUICKLIST(((void *)GET_FOOTER_FROM_HEADER(flushBlock) + 8))
						);
					flushBlock = prevBlock;
				}
				//coalesce flushblock with next block
				sf_block *nextBlock; 
				if(!GET_ALLOC(nextBlock = (sf_block *)((void *)(GET_FOOTER_FROM_HEADER(flushBlock)) + 8))) {
					//remove nextBlock from main free
					nextBlock->body.links.prev->body.links.next = nextBlock->body.links.next;
					nextBlock->body.links.next->body.links.prev = nextBlock->body.links.prev;

					//merge flushBlock and nextBlock
					PUT(flushBlock, (GET_SIZE(flushBlock) + GET_SIZE(nextBlock)) | GET_PREALLOC(flushBlock));
					//set footer
					PUT(GET_FOOTER_FROM_HEADER(flushBlock), GET(flushBlock));
					//set next block's prev alloc to 0
					PUT(
						((void *)GET_FOOTER_FROM_HEADER(flushBlock) + 8), 
						GET_SIZE(((void *)GET_FOOTER_FROM_HEADER(flushBlock) + 8)) | 
						GET_ALLOC(((void *)GET_FOOTER_FROM_HEADER(flushBlock) + 8)) | 
						GET_INQUICKLIST(((void *)GET_FOOTER_FROM_HEADER(flushBlock) + 8))
						);
				}

				//add to main free list
				size_t M = 32;
				sf_block *temp;
				for(int i = 0; i < NUM_FREE_LISTS; i++) {
						//add first free block to the right index
						if (i == 0) {
							//range if size == M
							if (GET_SIZE(flushBlock) == M) {
								// 1->3, 1 is dummy, 3 is next block, 2 is inserting block
								//preserve 3
								temp = sf_free_list_heads[i].body.links.next;
								//set 1->2
								sf_free_list_heads[i].body.links.next = flushBlock;
								//set 2->3
								flushBlock->body.links.next = temp;
								//set 2->1
								flushBlock->body.links.prev = &sf_free_list_heads[i];
								//set 3->2
								temp->body.links.prev = flushBlock;
							}
						} else if (GET_SIZE(flushBlock) > M && GET_SIZE(flushBlock) <= (M << 1)) {
								// 1->3, 1 is dummy, 3 is next block, 2 is inserting block
								//preserve 3
								temp = sf_free_list_heads[i].body.links.next;
								//set 1->2
								sf_free_list_heads[i].body.links.next = flushBlock;
								//set 2->3
								flushBlock->body.links.next = temp;
								//set 2->1
								flushBlock->body.links.prev = &sf_free_list_heads[i];
								//set 3->2
								temp->body.links.prev = flushBlock;
						} else if(i == NUM_FREE_LISTS - 1) {
							// 1->3, 1 is dummy, 3 is next block, 2 is inserting block
							if(GET_SIZE(flushBlock) > (M << 1)) {
								// 1->3, 1 is dummy, 3 is next block, 2 is inserting block
								//preserve 3
								temp = sf_free_list_heads[i].body.links.next;
								//set 1->2
								sf_free_list_heads[i].body.links.next = flushBlock;
								//set 2->3
								flushBlock->body.links.next = temp;
								//set 2->1
								flushBlock->body.links.prev = &sf_free_list_heads[i];
								//set 3->2
								temp->body.links.prev = flushBlock;
							}
						}
						if (i != 0) {
							M = M << 1;
						}
				}
				//get next flushblock 
				flushBlock = sf_quick_lists[quickListIndex].first;
			}
		}
		//add to quick list
		freeBlock->body.links.next = sf_quick_lists[quickListIndex].first;
		sf_quick_lists[quickListIndex].first = freeBlock;
		sf_quick_lists[quickListIndex].length += 1;

		//update header to in quick list
		PUT(freeBlock, toBeFreeBlockSize | GET_ALLOC(freeBlock) | GET_PREALLOC(freeBlock) | IN_QUICK_LIST);
	} else {
		//free block is the block being freed if can't fit in quick list

		//set alloc to 0
		PUT(freeBlock, GET_SIZE(freeBlock) | GET_PREALLOC(freeBlock));
		//set footer
		PUT(GET_FOOTER_FROM_HEADER(freeBlock), GET(freeBlock));
		//set next block's prev alloc to 0
		PUT(
			((void *)GET_FOOTER_FROM_HEADER(freeBlock) + 8), 
			GET_SIZE(((void *)GET_FOOTER_FROM_HEADER(freeBlock) + 8)) | 
			GET_ALLOC(((void *)GET_FOOTER_FROM_HEADER(freeBlock) + 8)) | 
			GET_INQUICKLIST(((void *)GET_FOOTER_FROM_HEADER(freeBlock) + 8))
			);

		//coalesce freeBlock with prev
		if(!GET_PREALLOC(freeBlock)) {
			//get header of previous block
			sf_block *prevBlockOfFree = GET_HEADER_FROM_FOOTER((void *)freeBlock - 8);

			//remove prevBlock from main free
			prevBlockOfFree->body.links.prev->body.links.next = prevBlockOfFree->body.links.next;
			prevBlockOfFree->body.links.next->body.links.prev = prevBlockOfFree->body.links.prev;

			//merge prevBlock and freeBlock
			PUT(prevBlockOfFree, (GET_SIZE(prevBlockOfFree) + GET_SIZE(freeBlock)) | GET_PREALLOC(prevBlockOfFree));
			//set footer
			PUT(GET_FOOTER_FROM_HEADER(prevBlockOfFree), GET(prevBlockOfFree));
			freeBlock = prevBlockOfFree;
			//set next block's prev alloc to 0
			PUT(
				((void *)GET_FOOTER_FROM_HEADER(freeBlock) + 8), 
				GET_SIZE(((void *)GET_FOOTER_FROM_HEADER(freeBlock) + 8)) | 
				GET_ALLOC(((void *)GET_FOOTER_FROM_HEADER(freeBlock) + 8)) | 
				GET_INQUICKLIST(((void *)GET_FOOTER_FROM_HEADER(freeBlock) + 8))
				);
		}
		//coalesce freeBlock with next 
		sf_block *nextBlockOfFree; 
		if(!GET_ALLOC(nextBlockOfFree = (sf_block *)((void *)(GET_FOOTER_FROM_HEADER(freeBlock)) + 8))) {
			//remove nextBlock from main free
			nextBlockOfFree->body.links.prev->body.links.next = nextBlockOfFree->body.links.next;
			nextBlockOfFree->body.links.next->body.links.prev = nextBlockOfFree->body.links.prev;

			//merge flushBlock and nextBlock
			PUT(freeBlock, (GET_SIZE(freeBlock) + GET_SIZE(nextBlockOfFree)) | GET_PREALLOC(freeBlock));
			//set footer
			PUT(GET_FOOTER_FROM_HEADER(freeBlock), GET(freeBlock));
			//set next block's prev alloc to 0
			PUT(
				((void *)GET_FOOTER_FROM_HEADER(freeBlock) + 8), 
				GET_SIZE(((void *)GET_FOOTER_FROM_HEADER(freeBlock) + 8)) | 
				GET_ALLOC(((void *)GET_FOOTER_FROM_HEADER(freeBlock) + 8)) | 
				GET_INQUICKLIST(((void *)GET_FOOTER_FROM_HEADER(freeBlock) + 8))
				);
		}

		//add to main free list
		size_t M = 32;
		sf_block *temp;
		for(int i = 0; i < NUM_FREE_LISTS; i++) {
				//add first free block to the right index
				if (i == 0) {
					//range if size == M
					if (GET_SIZE(freeBlock) == M) {
						// 1->3, 1 is dummy, 3 is next block, 2 is inserting block
						//preserve 3
						temp = sf_free_list_heads[i].body.links.next;
						//set 1->2
						sf_free_list_heads[i].body.links.next = freeBlock;
						//set 2->3
						freeBlock->body.links.next = temp;
						//set 2->1
						freeBlock->body.links.prev = &sf_free_list_heads[i];
						//set 3->2
						temp->body.links.prev = freeBlock;
					}
				} else if (GET_SIZE(freeBlock) > M && GET_SIZE(freeBlock) <= (M << 1)) {
						// 1->3, 1 is dummy, 3 is next block, 2 is inserting block
						//preserve 3
						temp = sf_free_list_heads[i].body.links.next;
						//set 1->2
						sf_free_list_heads[i].body.links.next = freeBlock;
						//set 2->3
						freeBlock->body.links.next = temp;
						//set 2->1
						freeBlock->body.links.prev = &sf_free_list_heads[i];
						//set 3->2
						temp->body.links.prev = freeBlock;
				} else if(i == NUM_FREE_LISTS - 1) {
					// 1->3, 1 is dummy, 3 is next block, 2 is inserting block
					if(GET_SIZE(freeBlock) > (M << 1)) {
						// 1->3, 1 is dummy, 3 is next block, 2 is inserting block
						//preserve 3
						temp = sf_free_list_heads[i].body.links.next;
						//set 1->2
						sf_free_list_heads[i].body.links.next = freeBlock;
						//set 2->3
						freeBlock->body.links.next = temp;
						//set 2->1
						freeBlock->body.links.prev = &sf_free_list_heads[i];
						//set 3->2
						temp->body.links.prev = freeBlock;
					}
				}
				if (i != 0) {
					M = M << 1;
				}
		}
	}
	// debug("Free");
	// sf_show_heap();
}

void *sf_realloc(void *pp, size_t rsize)
{
	if(
	!pp || //if pp is NULL
	((size_t)pp & 7) || //if not 8-byte aligned
	(GET_SIZE(GET_HEADER_FROM_PAYLOAD(pp)) < 32 ) || //if size is < 32
	(GET_SIZE(GET_HEADER_FROM_PAYLOAD(pp)) % 8) || //if size is not a multiple of 8
	(GET_HEADER_FROM_PAYLOAD(pp) < (sf_block *)(sf_mem_start() + 32)) || //if header is before end of prologue
	(((void *)(GET_FOOTER_FROM_HEADER(GET_HEADER_FROM_PAYLOAD(pp))) + 8) > (sf_mem_end() - 8)) ||//if footer end is after epilogue
	!(GET_ALLOC(GET_HEADER_FROM_PAYLOAD(pp))) || //if block is already free
	GET_INQUICKLIST(GET_HEADER_FROM_PAYLOAD(pp)) || //block is in quicklist
	(!(GET_PREALLOC(GET_HEADER_FROM_PAYLOAD(pp))) && GET_ALLOC((void *)(GET_HEADER_FROM_PAYLOAD(pp)) - 8)) //prev_alloc of current is 0 but previous block is allocated
	) {
		//invalid
		sf_errno = EINVAL;
		return NULL;
	}
	//pointer is valid
	if(rsize) {
		//pp payload size
		size_t payLoadSize = GET_SIZE(GET_HEADER_FROM_PAYLOAD(pp)) - 8;
		//pp block size
		size_t blockSize = payLoadSize + 8;
		if(payLoadSize == rsize) {
			//no change
			return pp;
		} else if(payLoadSize < rsize) {
			//larger 
			sf_block *largerBlock;
			if((largerBlock = sf_malloc(rsize))) {
				memcpy(largerBlock, pp, rsize);
				sf_free(pp);
				return largerBlock;
			} else {
				//malloc failed
				return NULL;
			}
		} else {
			//smaller
			//split
			size_t blockSizeNeed;
			//set size needed
			if(((rsize + 8) % 8)) {
				blockSizeNeed = rsize + 8 + (8 - ((rsize + 8) % 8));
			} else {
				//if 0
				blockSizeNeed = rsize + 8;
			}
			// debug("%zu", rsize);
			// debug("blockSizeNeed: %zu", blockSizeNeed);

			if(blockSizeNeed < 32) {
				blockSizeNeed = 32;
			}

			// debug("blockSizeNeed: %zu", blockSizeNeed);
			// debug("blockSize: %zu", blockSize);
			// debug("pp size: %zu", GET_SIZE(GET_HEADER_FROM_PAYLOAD(pp)));

			if((blockSize - blockSizeNeed) >= 32) {
				//split
				//copy memory
				// memcpy(pp, pp, blockSizeNeed);
				//change header of allocated block
				PUT(GET_HEADER_FROM_PAYLOAD(pp), blockSizeNeed | GET_ALLOC(GET_HEADER_FROM_PAYLOAD(pp)) | GET_PREALLOC(GET_HEADER_FROM_PAYLOAD(pp)));
				//split block
				sf_block *splitBlock = (void *)(GET_HEADER_FROM_PAYLOAD(pp)) + blockSizeNeed;
				//set header for split block
				PUT(splitBlock, (blockSize - blockSizeNeed) | PREV_BLOCK_ALLOCATED);
				//set footer for split block
				PUT(GET_FOOTER_FROM_HEADER(splitBlock), GET(splitBlock));

				//coallesce split block with next
				sf_block *nextBlockOfSplit; 
				if(!GET_ALLOC(nextBlockOfSplit = (sf_block *)((void *)(GET_FOOTER_FROM_HEADER(splitBlock)) + 8))) {
					//remove nextBlock from main free
					nextBlockOfSplit->body.links.prev->body.links.next = nextBlockOfSplit->body.links.next;
					nextBlockOfSplit->body.links.next->body.links.prev = nextBlockOfSplit->body.links.prev;

					//merge flushBlock and nextBlock
					PUT(splitBlock, (GET_SIZE(splitBlock) + GET_SIZE(nextBlockOfSplit)) | GET_PREALLOC(splitBlock));
					//set footer
					PUT(GET_FOOTER_FROM_HEADER(splitBlock), GET(splitBlock));
					//set next block's prev alloc to 0
					PUT(
						((void *)GET_FOOTER_FROM_HEADER(splitBlock) + 8), 
						GET_SIZE(((void *)GET_FOOTER_FROM_HEADER(splitBlock) + 8)) | 
						GET_ALLOC(((void *)GET_FOOTER_FROM_HEADER(splitBlock) + 8)) | 
						GET_INQUICKLIST(((void *)GET_FOOTER_FROM_HEADER(splitBlock) + 8))
						);
				}

				//add splitBlock to main free
				size_t M = 32;
				sf_block *temp;
				for(int i = 0; i < NUM_FREE_LISTS; i++) {
						//add first free block to the right index
						if (i == 0) {
							//range if size == M
							if (GET_SIZE(splitBlock) == M) {
								// 1->3, 1 is dummy, 3 is next block, 2 is inserting block
								//preserve 3
								temp = sf_free_list_heads[i].body.links.next;
								//set 1->2
								sf_free_list_heads[i].body.links.next = splitBlock;
								//set 2->3
								splitBlock->body.links.next = temp;
								//set 2->1
								splitBlock->body.links.prev = &sf_free_list_heads[i];
								//set 3->2
								temp->body.links.prev = splitBlock;
							}
						} else if (GET_SIZE(splitBlock) > M && GET_SIZE(splitBlock) <= (M << 1)) {
								// 1->3, 1 is dummy, 3 is next block, 2 is inserting block
								//preserve 3
								temp = sf_free_list_heads[i].body.links.next;
								//set 1->2
								sf_free_list_heads[i].body.links.next = splitBlock;
								//set 2->3
								splitBlock->body.links.next = temp;
								//set 2->1
								splitBlock->body.links.prev = &sf_free_list_heads[i];
								//set 3->2
								temp->body.links.prev = splitBlock;
						} else if(i == NUM_FREE_LISTS - 1) {
							// 1->3, 1 is dummy, 3 is next block, 2 is inserting block
							if(GET_SIZE(splitBlock) > (M << 1)) {
								// 1->3, 1 is dummy, 3 is next block, 2 is inserting block
								//preserve 3
								temp = sf_free_list_heads[i].body.links.next;
								//set 1->2
								sf_free_list_heads[i].body.links.next = splitBlock;
								//set 2->3
								splitBlock->body.links.next = temp;
								//set 2->1
								splitBlock->body.links.prev = &sf_free_list_heads[i];
								//set 3->2
								temp->body.links.prev = splitBlock;
							}
						}
						if (i != 0) {
							M = M << 1;
						}
				}
				return pp;
			} else {
				//don't split
				// memcpy(pp, pp, blockSizeNeed);
				// PUT(GET_HEADER_FROM_PAYLOAD(pp), blockSizeNeed | GET_ALLOC(GET_HEADER_FROM_PAYLOAD(pp)) | GET_PREALLOC(GET_HEADER_FROM_PAYLOAD(pp)));
				return pp;
			}
		}
	} 
	//size 0
	sf_free(pp);
	return NULL;
}

void *sf_memalign(size_t size, size_t align)
{
	// TO BE IMPLEMENTED
	abort();
}

