/**
 * All functions you make for the assignment must be implemented in this file.
 * Do not submit your assignment with a main function in this file.
 * If you submit with a main function in this file, you will get a zero.
 */
#include "sfmm.h"
#include "helper.h"
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
/**
 * You should store the head of your free list in this variable.
 * Doing so will make it accessible via the extern statement in sfmm.h
 * which will allow you to pass the address to sf_snapshot in a different file.
 */
sf_free_header* freelist_head = NULL;
static size_t allocatedBlocks=0;
static size_t splinterBlocks=0;
static size_t padding_bytes=0;
static size_t splinter_bytes=0;
static size_t coalesces_times=0;
static double heap_size=0;
static double current_payload=0;
static double max_payload=0;
static double peakMemoryUtilization=0;
static char* heap_head=0;

void *sf_malloc(size_t size) {
	if(size==0){
		errno =EINVAL;
		return NULL;
	}

	size_t blockSize;
	if (size <= dsize)
		blockSize = 2*dsize;
	else
		blockSize = dsize *  ((size + (dsize)  + (dsize-1)) /  dsize);

	if(blockSize>4096*4){
		errno=ENOMEM;
		return NULL;
	}

	if(freelist_head==NULL){
		if(heap_size==0){
			heap_head=sf_sbrk(0);
			freelist_head=sf_sbrk(4096);
		}
		else if(heap_size>=4096*4){
			errno=ENOMEM;
			return NULL;
		}
		else
			freelist_head=(void*)((char*)sf_sbrk(4096));
		heap_size+=4096;
		peakMemoryUtilization=max_payload/heap_size;
		freelist_head->header.block_size=(4096>>4);
		freelist_head->header.alloc=0;
		freelist_head->header.splinter=0;
		freelist_head->prev=NULL;
		freelist_head->next=NULL;
		makeFooter((char*)&freelist_head->header);
	}
	char* bp;
	while(heap_size<=4096*4){
		if((bp=find_best_fit(blockSize))!=NULL){
			place_block(bp,blockSize,size);
			allocatedBlocks+=1;
			splinterBlocks+=(*(sf_header*)bp).splinter;
			padding_bytes+=(*(sf_header*)bp).padding_size;
			splinter_bytes+=(*(sf_header*)bp).splinter_size;
			current_payload+=(*(sf_header*)bp).requested_size;
			if(current_payload>max_payload){
				max_payload=current_payload;
				peakMemoryUtilization=max_payload/heap_size;
			}
			bp=bp+wsize;
			return bp;
		}
		if(heap_size>=4096*4)
			break;
		char* newF=(char*)sf_sbrk(4096);
		((sf_header*)newF)->block_size=(4096>>4);
		((sf_header*)newF)->alloc=0;
		((sf_header*)newF)->splinter=0;
		makeFooter((char*)newF);
		newF=coalesces(newF);
		heap_size+=4096;
		peakMemoryUtilization=max_payload/heap_size;
		insertFreeList(newF);
	}
	errno=ENOMEM;
	return NULL;
}
/*
 * Resizes the memory pointed to by ptr to size bytes.
 * @param ptr Address of the memory region to resize.
 * @param size The minimum size to resize the memory to.
 * @return If successful, the pointer to a valid region of memory is
 * returned, else NULL is returned and ERRNO is set accordingly.
 *
 * If sf_realloc is called with a valid pointer and a size of 0 it should free the
 * allocated block and return NULL.
 */
void *sf_realloc(void *ptr, size_t size) {
	char* bp=(char*)ptr-wsize;//get block ptr
	if(bp<heap_head||bp>=(char*)sf_sbrk(0)-wsize){
		errno =EINVAL;
		return NULL;
	}
	if(((sf_footer*)getFooter(bp))->block_size!=(*(sf_header*)bp).block_size||
		(*(sf_header*)bp).alloc==0||(((sf_footer*)getFooter(bp))->splinter!=(*(sf_header*)bp).splinter)||
		((sf_footer*)getFooter(bp))->alloc==0){
		errno =EINVAL;
		return NULL;
	}
	if(size==0){
		sf_free(ptr);
		return NULL;
	}
	size_t old_r_size=(*(sf_header*)bp).requested_size;
	if(old_r_size==size&&(*(sf_header*)bp).splinter==0){
		return ptr;
	}
	size_t blockSize;
	if (size <= dsize)
		blockSize = 2*dsize;
	else
		blockSize = dsize *  ((size + (dsize)  + (dsize-1)) /  dsize);
	if(blockSize>4096*4){
		errno=ENOMEM;
		return NULL;
	}
	size_t old_b_size=getSize((*(sf_header*)bp));
	allocatedBlocks-=1;
	splinterBlocks-=(*(sf_header*)bp).splinter;
	padding_bytes-=(*(sf_header*)bp).padding_size;
	splinter_bytes-=(*(sf_header*)bp).splinter_size;
	current_payload-=(*(sf_header*)bp).requested_size;// i guess i should put it outside if block.
	if(blockSize<=old_b_size){
		if(old_b_size-blockSize>=32){
			(*(sf_header*)bp).block_size=(blockSize>>4);
			(*(sf_header*)bp).splinter=0;
			(*(sf_header*)bp).splinter_size=0;
			(*(sf_header*)bp).alloc=1;
			(*(sf_header*)bp).requested_size=size;
			(*(sf_header*)bp).padding_size=blockSize-size;
			makeFooter(bp);
			char* fpNew=bp+blockSize;// make a new free block
			(*(sf_header*)fpNew).block_size=((old_b_size-blockSize)>>4);
			(*(sf_header*)fpNew).splinter=0;
			(*(sf_header*)fpNew).alloc=0;
			makeFooter(fpNew);
			coalesces(fpNew);// i assume this is right, who knows?
			insertFreeList(fpNew);
		}
		else{// old_b_size-blockSize < 32, there is a splinter
			if( ((sf_header*)((char*)getFooter(bp)+wsize))->alloc==0 ){//there is an adjcent free block
				if(old_b_size-blockSize>0){//a splinter
					sf_free_header* prev=((sf_free_header*)((char*)getFooter(bp)+wsize))->prev;
					sf_free_header* next=((sf_free_header*)((char*)getFooter(bp)+wsize))->next;

					size_t next_size=getSize(*(sf_header*)((char*)getFooter(bp)+wsize));
					char* fpNew=bp+blockSize;// make a new free block with the splinter
					(*(sf_header*)fpNew).block_size=((old_b_size-blockSize+next_size)>>4);
					(*(sf_header*)fpNew).splinter=0;
					(*(sf_header*)fpNew).alloc=0;
					makeFooter(fpNew);
					((sf_free_header*)fpNew)->next=next;
					((sf_free_header*)fpNew)->prev=prev;
					if(next!=NULL)
						next->prev=(void*)fpNew;
					if(prev!=NULL)
						prev->next=(void*)fpNew;
					if( ((sf_free_header*)((char*)getFooter(bp)+wsize))==freelist_head ){
						freelist_head=(void*)fpNew;
					}
					coalesces_times+=1;// coalesces with a splinter!
				}
				(*(sf_header*)bp).block_size=(blockSize>>4);
				(*(sf_header*)bp).splinter=0;
				(*(sf_header*)bp).splinter_size=0;
				(*(sf_header*)bp).alloc=1;
				(*(sf_header*)bp).requested_size=size;
				(*(sf_header*)bp).padding_size=blockSize-size;
				makeFooter(bp);
			}
			else{// there is no adjcent free block
				(*(sf_header*)bp).block_size=(old_b_size>>4);
				(*(sf_header*)bp).splinter=1;
				(*(sf_header*)bp).splinter_size=old_b_size-blockSize;
				(*(sf_header*)bp).alloc=1;
				(*(sf_header*)bp).requested_size=size;
				(*(sf_header*)bp).padding_size=blockSize-size;
				makeFooter(bp);
			}
		}
	}
	else{//blockSize>old_b_size, need to find a suitable free block, then memcpy,
		//then free old block.bp becomes the address of new suitable block.If none,extend heap properly.
		int extra_size=-1, adajcent_free_b_to_heap_end=0,fit_in_list=0;

		if((find_best_fit(blockSize))!=NULL)
			fit_in_list=1;

		sf_free_header* adajcent_fb=(void*)((char*)getFooter(bp)+wsize);
		if(((sf_header*)(adajcent_fb))->alloc==0&& getFooter((void*)adajcent_fb)
			==((char*)sf_sbrk(0)-wsize))
			adajcent_free_b_to_heap_end=1;
		// do{}
		if( ((sf_header*)adajcent_fb)->alloc==0)
			extra_size=getSize(*(sf_header*)adajcent_fb)+old_b_size-blockSize;

		if( (((sf_header*)adajcent_fb)->alloc==0&&extra_size>=0)||(adajcent_free_b_to_heap_end&&!fit_in_list) ){
		//there is an adjcent free block
			do{
				extra_size=getSize(*(sf_header*)adajcent_fb)+old_b_size-blockSize;
			if(extra_size>=32){//there is enough space. and will generate a new free block.
				int needMoveFreeHead=0;
				if(adajcent_fb==freelist_head)
						needMoveFreeHead=1;// if next free block is freelist head.
				sf_free_header* prev=adajcent_fb->prev;
				sf_free_header* next=adajcent_fb->next;
				(*(sf_header*)bp).block_size=(blockSize>>4);
				(*(sf_header*)bp).splinter=0;
				(*(sf_header*)bp).splinter_size=0;
				(*(sf_header*)bp).alloc=1;
				(*(sf_header*)bp).requested_size=size;
				(*(sf_header*)bp).padding_size=blockSize-size;
				makeFooter(bp);
				char* fpNew=bp+blockSize;// make a new free block
				(*(sf_header*)fpNew).block_size=((extra_size)>>4);
				(*(sf_header*)fpNew).splinter=0;
				(*(sf_header*)fpNew).alloc=0;
				makeFooter(fpNew);
				((sf_free_header*)fpNew)->next=next;
				((sf_free_header*)fpNew)->prev=prev;
				if(needMoveFreeHead)
					freelist_head=(void*)fpNew;
				break;
			}
			else if(extra_size<32&&extra_size>=0){//enough space, but a splinter.
				size_t total_size=getSize(*(sf_header*)adajcent_fb)+old_b_size;
				sf_free_header* old_fp=(void*)adajcent_fb;
				//need to remove that old free block from freelist.
				if(old_fp==freelist_head){
					freelist_head=freelist_head->next;
					if(freelist_head!=NULL)
						freelist_head->prev=NULL;
				}
				else{
					if(old_fp->prev!=NULL)
						old_fp->prev->next=old_fp->next;
					if(old_fp->next!=NULL){
						old_fp->next->prev=old_fp->prev;
					}
				}
				(*(sf_header*)bp).block_size=(total_size>>4);
				if(extra_size==0)//exactly enough space and no splinter.
					(*(sf_header*)bp).splinter=0;
				else//splinter
					(*(sf_header*)bp).splinter=1;
				(*(sf_header*)bp).splinter_size=extra_size;
				(*(sf_header*)bp).alloc=1;
				(*(sf_header*)bp).requested_size=size;
				(*(sf_header*)bp).padding_size=blockSize-size;
				makeFooter(bp);
				break;
			}
			if(heap_size>=4096*4){
				errno=ENOMEM;
				allocatedBlocks+=1;
				splinterBlocks+=(*(sf_header*)bp).splinter;
				padding_bytes+=(*(sf_header*)bp).padding_size;
				splinter_bytes+=(*(sf_header*)bp).splinter_size;
				current_payload+=(*(sf_header*)bp).requested_size;
				if(current_payload>max_payload){
					max_payload=current_payload;
					peakMemoryUtilization=max_payload/heap_size;
				}
				return NULL;
			}
			char* newF=(char*)sf_sbrk(4096);
			((sf_header*)newF)->block_size=(4096>>4);
			((sf_header*)newF)->alloc=0;
			((sf_header*)newF)->splinter=0;
			makeFooter((char*)newF);
			adajcent_fb=coalesces(newF);
			heap_size+=4096;
			peakMemoryUtilization=max_payload/heap_size;
			insertFreeList((void*)adajcent_fb);
			}while(adajcent_free_b_to_heap_end);
		}
		else{//there is no adjcent free block, or not enough space.
			char* newBp;
			int enoughSpace=0,find_in_list=0,from_heap=0;
			if((newBp=find_best_fit(blockSize))!=NULL){
				enoughSpace=1;find_in_list=1;// there is a suitable free block in list
			}
			else if((newBp=((char*)sf_malloc(size)-wsize))!=NULL){// there is no enough space in free list, extend heap.
				enoughSpace=1;from_heap=1;// heap is able to allocate more space.
			}
			if(enoughSpace){//not sure if it is correct, need to test.
				size_t free_b_size=getSize(*(sf_header*)newBp);
				int needMoveFreeHead=0;
				if(newBp==(char*)freelist_head)
						needMoveFreeHead=1;// if newBp block is freelist head.
				sf_free_header* prev=((sf_free_header*)(newBp))->prev;
				sf_free_header* next=((sf_free_header*)(newBp))->next;
				int splinter_size=(*(sf_header*)newBp).splinter_size;
				int padding_size=(*(sf_header*)newBp).padding_size;
				int block_size=(*(sf_header*)newBp).block_size;//record block info if its from heap
				newBp=memcpy(newBp,bp,old_b_size);
				if(free_b_size-blockSize>=32&&find_in_list){//generate a new free block
					(*(sf_header*)newBp).block_size=(blockSize>>4);
					(*(sf_header*)newBp).splinter=0;
					(*(sf_header*)newBp).splinter_size=0;
					(*(sf_header*)newBp).alloc=1;
					(*(sf_header*)newBp).requested_size=size;
					(*(sf_header*)newBp).padding_size=blockSize-size;
					makeFooter(newBp);
					char* fpNew=newBp+blockSize;// make a new free block
					(*(sf_header*)fpNew).block_size=((free_b_size-blockSize)>>4);
					(*(sf_header*)fpNew).splinter=0;
					(*(sf_header*)fpNew).alloc=0;
					makeFooter(fpNew);
					((sf_free_header*)fpNew)->next=next;
					((sf_free_header*)fpNew)->prev=prev;
					if(next!=NULL)
						next->prev=(void*)fpNew;
					if(prev!=NULL)
						prev->next=(void*)fpNew;
					if(needMoveFreeHead){
						freelist_head=(void*)fpNew;
					}
				}
				else if(from_heap){
					(*(sf_header*)newBp).block_size=block_size;// dont need to >>4 here
					(*(sf_header*)newBp).splinter_size=splinter_size;
					(*(sf_header*)newBp).padding_size=padding_size;
					(*(sf_header*)newBp).requested_size=size;
					(*(sf_header*)newBp).alloc=1;
					if(splinter_size==0)
						(*(sf_header*)newBp).splinter=0;
					else
						(*(sf_header*)newBp).splinter=1;
					makeFooter(newBp);
				}
				else{//splinter! and remove the free block.
					//need to remove that old free block from freelist.
					if(needMoveFreeHead){
						freelist_head=freelist_head->next;
						if(freelist_head!=NULL)
							freelist_head->prev=NULL;
					}
					else{
						if(((sf_free_header*)newBp)->prev!=NULL)
						((sf_free_header*)newBp)->prev->next=((sf_free_header*)newBp)->next;
						if(((sf_free_header*)newBp)->next!=NULL){
							((sf_free_header*)newBp)->next->prev=((sf_free_header*)newBp)->prev;
						}
					}
					(*(sf_header*)newBp).block_size=(free_b_size>>4);
					if(free_b_size-blockSize==0)
						(*(sf_header*)newBp).splinter=0;
					else
						(*(sf_header*)newBp).splinter=1;
					(*(sf_header*)newBp).splinter_size=free_b_size-blockSize;
					(*(sf_header*)newBp).alloc=1;
					(*(sf_header*)newBp).requested_size=size;
					(*(sf_header*)newBp).padding_size=blockSize-size;
					makeFooter(newBp);
				}
				sf_free(ptr);
				bp=newBp;//remember we return bp,not newBp.
			}
			else{
				{// no space available on heap and no suitable free block in list.
					errno=ENOMEM;
					allocatedBlocks+=1;
					splinterBlocks+=(*(sf_header*)bp).splinter;
					padding_bytes+=(*(sf_header*)bp).padding_size;
					splinter_bytes+=(*(sf_header*)bp).splinter_size;
					current_payload+=(*(sf_header*)bp).requested_size;
					if(current_payload>max_payload){
						max_payload=current_payload;
						peakMemoryUtilization=max_payload/heap_size;
					}
					return NULL;
				}
			}
		}
	}
	allocatedBlocks+=1;
	splinterBlocks+=(*(sf_header*)bp).splinter;
	padding_bytes+=(*(sf_header*)bp).padding_size;
	splinter_bytes+=(*(sf_header*)bp).splinter_size;
	current_payload+=(*(sf_header*)bp).requested_size;
	if(current_payload>max_payload){
		max_payload=current_payload;
		peakMemoryUtilization=max_payload/heap_size;
	}
	bp=bp+wsize;
	return bp;
}

void insertFreeList(char* bp){
	char* fp=(void*)freelist_head;
	if(freelist_head==NULL){
		freelist_head=(void*)bp;
		freelist_head->next=NULL;
		freelist_head->prev=NULL;
		return;
	}
	while(fp!=NULL){
		if(fp==bp)
			break;
		else if(fp-bp>0){
			((sf_free_header*)bp)->prev=((sf_free_header*)fp)->prev;
			((sf_free_header*)bp)->next=(void*)fp;
			if((void*)fp==(void*)freelist_head)
				freelist_head=(void*)bp;
			else if(((sf_free_header*)fp)->prev!=NULL)
				((sf_free_header*)fp)->prev->next=((void*)bp);
			((sf_free_header*)fp)->prev=(void*)bp;
			break;
		}
		else if(((sf_free_header*)fp)->next==NULL){
			((sf_free_header*)fp)->next=(void*)bp;
			((sf_free_header*)bp)->next=NULL;
			((sf_free_header*)bp)->prev=(void*)fp;
			break;
		}
		fp=(void*)((sf_free_header*)fp)->next;
	}
}

void sf_free(void* ptr) {
	char* bp=(char*)ptr-wsize;//get block ptr
	if(bp<heap_head||bp>=(char*)sf_sbrk(0)-wsize){
		errno =EINVAL;
		return;
	}
	if(((sf_footer*)getFooter(bp))->block_size!=(*(sf_header*)bp).block_size||
		(*(sf_header*)bp).alloc==0||(((sf_footer*)getFooter(bp))->splinter!=(*(sf_header*)bp).splinter)||
		((sf_footer*)getFooter(bp))->alloc==0){
		errno =EINVAL;
		return;
	}
	allocatedBlocks-=1;
	splinterBlocks-=(*(sf_header*)bp).splinter;
	padding_bytes-=(*(sf_header*)bp).padding_size;
	splinter_bytes-=(*(sf_header*)bp).splinter_size;
	current_payload-=(*(sf_header*)bp).requested_size;

	(*(sf_header*)bp).alloc=0;
	(*(sf_header*)bp).splinter=0;
	((sf_footer*)getFooter(bp))->alloc=0;
	((sf_footer*)getFooter(bp))->splinter=0;

	bp=coalesces(bp);

	insertFreeList(bp);// insert bp into freelist if necessary
	return;
}

int sf_info(info* ptr) {
	if(ptr==NULL||heap_size==0)
		return -1;
	ptr->allocatedBlocks=allocatedBlocks;
	ptr->splinterBlocks=splinterBlocks;
	ptr->padding=padding_bytes;
	ptr->splintering=splinter_bytes;
	ptr->coalesces=coalesces_times;
	ptr->peakMemoryUtilization=peakMemoryUtilization;
	return 0;
}
void* getHeapHead(){
	return heap_head;
}
size_t getSize(sf_header header){
	return header.block_size<<4;
}

void* coalesces(char* bp){
	int prev_alloc, next_alloc;
	if(bp==heap_head)
		prev_alloc=1;
	else
		prev_alloc = ((sf_footer*)(bp-wsize))->alloc;
	if(getFooter(bp)==(char*)sf_sbrk(0)-wsize)
		next_alloc=1;
	else
		next_alloc =((sf_header*)((char*)getFooter(bp)+wsize))->alloc;

	size_t size= getSize(*(sf_header*)(bp));
	if (prev_alloc && next_alloc) {
		return bp;
	}
	else if (prev_alloc &&  !next_alloc) {  /* Case 2 */
		sf_free_header* prev=((sf_free_header*)((char*)getFooter(bp)+wsize))->prev;
		sf_free_header* next=((sf_free_header*)((char*)getFooter(bp)+wsize))->next;
		((sf_free_header*)bp)->prev=prev;
		((sf_free_header*)bp)->next=next;
		if(prev!=NULL)
			prev->next=(void*)bp;
		if(next!=NULL)
			next->prev=(void*)bp;
		if( ((sf_free_header*)((char*)getFooter(bp)+wsize))==freelist_head ){
			freelist_head=(void*)bp;
		}
		size+= getSize(*((sf_header*)((char*)getFooter(bp)+wsize)));
		((sf_header*)bp)->block_size=(size>>4);
		((sf_footer*)getFooter(bp))->block_size=(size>>4);
	}
	else if (!prev_alloc && next_alloc) {  /* Case 3 */
		size+= getSize(*((sf_header*)(bp-wsize)));
	//size+= GET_SIZE(HDRP(PREV_BLKP(bp)));
		((sf_footer*)getFooter(bp))->block_size=(size>>4);
	//PUT(FTRP(bp), PACK(size, 0));
		((sf_header*)getHeader(bp-wsize))->block_size=(size>>4);
	//PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
		bp=getHeader(bp-wsize);
	//bp = PREV_BLKP(bp);
	}
	else {
		if(((sf_free_header*)((char*)getFooter(bp)+wsize))->next!=NULL
			&&(char*)getFooter(((char*)getFooter(bp)+wsize))<(char*)sf_sbrk(0)-wsize){
			((sf_free_header*)getHeader(bp-wsize))->next=((sf_free_header*)((char*)getFooter(bp)+wsize))->next;
			((sf_free_header*)((char*)getFooter(bp)+wsize))->next->prev=((sf_free_header*)getHeader(bp-wsize));
		}
		else{
			((sf_free_header*)getHeader(bp-wsize))->next=NULL;
		}
  	//size += GET_SIZE(HDRP(PREV_BLKP(bp))) +GET_SIZE(FTRP(NEXT_BLKP(bp)));
		size+=getSize(*((sf_header*)(bp-wsize)))+getSize(*((sf_header*)((char*)getFooter(bp)+wsize)));
 	//PUT(HDRP(PREV_BLKP(bp)), PACK(size, O));
		((sf_header*)getHeader(bp-wsize))->block_size=(size>>4);
 	//PUT(FTRP(NEXT_BLKP(bp)), PACK(size, O));
		((sf_footer*)getFooter((char*)getFooter(bp)+wsize))->block_size=(size>>4);
  	//bp = PREV_BLKP(bp);
		bp=getHeader(bp-wsize);
	}
	coalesces_times+=1;
	return bp;
}

void* makeFooter(char* header){
	sf_footer* footer=(sf_footer*)(header+getSize(*(sf_header*)header)-wsize);
	footer->block_size=((sf_header*)header)->block_size;
	footer->alloc=((sf_header*)header)->alloc;
	footer->splinter=((sf_header*)header)->splinter;
	return footer;
}

void* getHeader(char* footer){
	return (footer-getSize(*(sf_header*)footer)+wsize);
}

void* getFooter(char* header){
	return (header+getSize(*(sf_header*)header)-wsize);
}

void* find_best_fit(size_t size){
	sf_free_header* bp=freelist_head;
	sf_free_header* best_fit=NULL;
	int smallestD=4096*4;
	while(bp!=NULL){
		if(getSize((*(sf_header*)bp))>=size && smallestD>(getSize((*(sf_header*)bp))-size)){
			smallestD=getSize((*(sf_header*)bp))-size;
			best_fit=bp;
		}
		bp=bp->next;
	}

	return best_fit;
}

void place_block(char* bp,size_t b_size,size_t r_size){
	size_t oldBlockSize=getSize(*(sf_header*)bp);
	sf_free_header *next=((sf_free_header *)bp)->next;
    sf_free_header *prev=((sf_free_header *)bp)->prev;
	if(oldBlockSize-b_size>=32){ // create a new free block, insert it to free list.
		(*(sf_header*)bp).splinter=0;
		(*(sf_header*)bp).alloc=1;
		(*(sf_header*)bp).block_size=(b_size>>4);
		(*(sf_header*)bp).requested_size=r_size;
		(*(sf_header*)bp).splinter_size=0;
		(*(sf_header*)bp).padding_size=b_size-r_size-dsize;
		makeFooter(bp);
		if((void*)bp==(void*)freelist_head){
			bp=bp+b_size;
			(*(sf_header*)bp).block_size=((oldBlockSize-b_size)>>4);
			(*(sf_header*)bp).splinter=0;
			(*(sf_header*)bp).alloc=0;
			((sf_footer*)getFooter(bp))->block_size=(*(sf_header*)bp).block_size;
			freelist_head=(void*)bp;
			freelist_head->next=next;
			freelist_head->prev=prev;
			return;
		}
		bp=bp+b_size;
		(*(sf_header*)bp).block_size=((oldBlockSize-b_size)>>4);
		(*(sf_header*)bp).splinter=0;
		(*(sf_header*)bp).alloc=0;
		((sf_footer*)getFooter(bp))->block_size=(*(sf_header*)bp).block_size;
		if(prev!=NULL)
			prev->next=(sf_free_header*)bp;
		if(next!=NULL)
			next->prev=(sf_free_header*)bp;
		((sf_free_header*)bp)->prev=prev;
		((sf_free_header*)bp)->next=next;
		return;
	}
	//oldBlockSize-b_size<32, create a splinter. Delete the free block from free list.
	if(prev!=NULL)
		prev->next=next;
	if(next!=NULL)
		next->prev=prev;
	if((void*)bp==(void*)freelist_head)
		freelist_head=next;
	(*(sf_header*)bp).splinter=1;
	(*(sf_header*)bp).alloc=1;
	(*(sf_header*)bp).block_size=(oldBlockSize>>4);
	(*(sf_header*)bp).requested_size=r_size;
	(*(sf_header*)bp).splinter_size=oldBlockSize-b_size;
	(*(sf_header*)bp).padding_size=b_size-r_size-dsize;
	((sf_footer*)getFooter(bp))->splinter=1;
	((sf_footer*)getFooter(bp))->alloc=1;
	return;
}
