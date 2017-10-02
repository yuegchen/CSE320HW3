#include <criterion/criterion.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "sfmm.h"
#include "helper.h"

/**
 *  HERE ARE OUR TEST CASES NOT ALL SHOULD BE GIVEN STUDENTS
 *  REMINDER MAX ALLOCATIONS MAY NOT EXCEED 4 * 4096 or 16384 or 128KB
 */

Test(sf_memsuite, Malloc_an_Integer, .init = sf_mem_init, .fini = sf_mem_fini) {
  int *x = sf_malloc(sizeof(int));
  *x = 4;
  cr_assert(*x == 4, "Failed to properly sf_malloc space for an integer!");
}

Test(sf_memsuite, Free_block_check_header_footer_values, .init = sf_mem_init, .fini = sf_mem_fini) {
  void *pointer = sf_malloc(sizeof(short));
  sf_free(pointer);
  pointer = (char*)pointer - 8;
  sf_header *sfHeader = (sf_header *) pointer;
  cr_assert(sfHeader->alloc == 0, "Alloc bit in header is not 0!\n");
  sf_footer *sfFooter = (sf_footer *) ((char*)pointer + (sfHeader->block_size << 4) - 8);
  cr_assert(sfFooter->alloc == 0, "Alloc bit in the footer is not 0!\n");
}

Test(sf_memsuite, SplinterSize_Check_char, .init = sf_mem_init, .fini = sf_mem_fini){
  void* x = sf_malloc(32);
  void* y = sf_malloc(32);
  (void)y;

  sf_free(x);

  x = sf_malloc(16);

  sf_header *sfHeader = (sf_header *)((char*)x - 8);
  cr_assert(sfHeader->splinter == 1, "Splinter bit in header is not 1!");
  cr_assert(sfHeader->splinter_size == 16, "Splinter size is not 16");

  sf_footer *sfFooter = (sf_footer *)((char*)sfHeader + (sfHeader->block_size << 4) - 8);
  cr_assert(sfFooter->splinter == 1, "Splinter bit in header is not 1!");
}

Test(sf_memsuite, Check_next_prev_pointers_of_free_block_at_head_of_list, .init = sf_mem_init, .fini = sf_mem_fini) {
  int *x = sf_malloc(4);
  memset(x, 0, 0);
  cr_assert(freelist_head->next == NULL);
  cr_assert(freelist_head->prev == NULL);
}

Test(sf_memsuite, Coalesce_no_coalescing, .init = sf_mem_init, .fini = sf_mem_fini) {
  int *x = sf_malloc(4);
  int *y = sf_malloc(4);
  memset(y, 0, 0);
  sf_free(x);

  //just simply checking there are more than two things in list
  //and that they point to each other
  cr_assert(freelist_head->next != NULL);
  cr_assert(freelist_head->next->prev != NULL);
}

//#
//STUDENT UNIT TESTS SHOULD BE WRITTEN BELOW
//DO NOT DELETE THESE COMMENTS
//#
Test(sf_memsuite, free_case1, .init = sf_mem_init, .fini = sf_mem_fini) {
  int *a= sf_malloc(4);
  int *x=sf_malloc(5);
  int *y=sf_malloc(555);
  (void)a;(void)y;
  sf_free(x);
  sf_free_header *fp = (void*)((char*)getFooter((char*)a-wsize)+wsize);
  sf_footer* fpFooter = (void*)((char*)getFooter((void*)fp));

  cr_assert(fp->header.block_size==fpFooter->block_size,"header and footer not same");
  cr_assert(fp==freelist_head,"the new free blcok is not freelist_head");
  cr_assert(freelist_head->next!=NULL,"the new free blcok is not inserted into freelist");
  cr_assert(freelist_head->next->prev==freelist_head,"the new free blcok is not inserted into freelist");
  cr_assert(fp->header.alloc==0,"the new free blcok doesn't coalesce with next block");
  cr_assert(getSize(*(sf_header*)fp)==32,"the new free blcok doesn't coalesce with next block");
  cr_assert(fpFooter->alloc==0,"the footer of free block is not same as header");
  cr_assert(getSize(*(sf_header*)fpFooter)==32,"the footer of free block is not same as header");
}

Test(sf_memsuite, free_case2, .init = sf_mem_init, .fini = sf_mem_fini) {
  int *a= sf_malloc(4);
  int *x=sf_malloc(5);
  int *y=sf_malloc(4);
  int *z=sf_malloc(1222);
  (void)a;(void)y;(void)z;
  sf_free(y);
  sf_free(x);
  sf_free_header *fp = (void*)((char*)getFooter((char*)a-wsize)+wsize);
  sf_footer* fpFooter = (void*)((char*)getFooter((void*)fp));

  cr_assert(fp->header.block_size==fpFooter->block_size,"header and footer not same");
  cr_assert(fp==freelist_head,"the new free blcok is not freelist_head");
  cr_assert(freelist_head->next!=NULL,"the new free blcok is not inserted into freelist");
  cr_assert(freelist_head->next->prev==freelist_head,"the new free blcok is not inserted into freelist");
  cr_assert(fp->header.alloc==0,"the new free blcok doesn't coalesce with next block");
  cr_assert(getSize(*(sf_header*)fp)==64,"the new free blcok doesn't coalesce with next block");
  cr_assert(fpFooter->alloc==0,"the footer of free block is not same as header");
  cr_assert(getSize(*(sf_header*)fpFooter)==64,"the footer of free block is not same as header");
}

Test(sf_memsuite, free_case3, .init = sf_mem_init, .fini = sf_mem_fini) {
  int *a= sf_malloc(4);
  int *x=sf_malloc(5);
  int *y=sf_malloc(4);
  int *z=sf_malloc(1222);
  (void)a;(void)y;(void)z;
  sf_free(x);
  sf_free(y);
  sf_free_header *fp = (void*)((char*)getFooter((char*)a-wsize)+wsize);
  sf_footer* fpFooter = (void*)((char*)getFooter((void*)fp));

  cr_assert(fp->header.block_size==fpFooter->block_size,"header and footer not same");
  cr_assert(fp==freelist_head,"the new free blcok is not freelist_head");
  cr_assert(freelist_head->next!=NULL,"the new free blcok is not inserted into freelist");
  cr_assert(freelist_head->next->prev==freelist_head,"the new free blcok is not inserted into freelist");
  cr_assert(fp->header.alloc==0,"the new free blcok doesn't coalesce with next block");
  cr_assert(getSize(*(sf_header*)fp)==64,"the new free blcok doesn't coalesce with next block");
  cr_assert(fpFooter->alloc==0,"the footer of free block is not same as header");
  cr_assert(getSize(*(sf_header*)fpFooter)==64,"the footer of free block is not same as header");
}

Test(sf_memsuite, free_case4, .init = sf_mem_init, .fini = sf_mem_fini) {
  int *a= sf_malloc(4);
  int *x=sf_malloc(5);
  int *y=sf_malloc(4);
  int *z=sf_malloc(1222);
  (void)a;(void)y;(void)z;
  sf_free(a);
  sf_free(y);
  sf_free(x);
  sf_free_header *fp = getHeapHead();
  sf_footer* fpFooter = (void*)((char*)getFooter((void*)fp));

  cr_assert(fp->header.block_size==fpFooter->block_size,"header and footer not same");
  cr_assert(fp==freelist_head,"the new free blcok is not freelist_head");
  cr_assert(freelist_head->next!=NULL,"the new free blcok is not inserted into freelist");
  cr_assert(freelist_head->next->prev==freelist_head,"the new free blcok is not inserted into freelist");
  cr_assert(fp->header.alloc==0,"the new free blcok doesn't coalesce with next block");
  cr_assert(getSize(*(sf_header*)fp)==96,"the new free blcok doesn't coalesce with next block");
  cr_assert(fpFooter->alloc==0,"the footer of free block is not same as header");
  cr_assert(getSize(*(sf_header*)fpFooter)==96,"the footer of free block is not same as header");
}

Test(sf_memsuite, realloc_smaller_size_1_splinter, .init = sf_mem_init, .fini = sf_mem_fini) {
  int *x = sf_malloc(4);
  int *y = sf_malloc(17);
  int *z  = sf_malloc(4);
  (void)z;
  memset(x, 0, 0);
  y=sf_realloc(y,2);
  //sf_varprint(y);//take a look
  //sf_snapshot(true);
  sf_header *sfHeader = (void*)((char *) y-wsize);
  //check the splinter in header
  cr_assert(sfHeader->splinter==1,"splinter bit in header is not 1");
  cr_assert(sfHeader->splinter_size==16,"splinter_size in header is not 16");
}

Test(sf_memsuite, realloc_smaller_size_2_no_splinter_free_block, .init = sf_mem_init, .fini = sf_mem_fini) {
  int *x = sf_malloc(4);
  int *y = sf_malloc(48);
  int *z  = sf_malloc(4);
  (void)z;
  memset(x, 0, 0);
  y=sf_realloc(y,2);
  //sf_varprint(y);//take a look
  //sf_snapshot(true);
  sf_header *sfHeader = (void*)((char *) y-wsize);
  sf_free_header *fp = (void*)((char*)getFooter((void*)sfHeader)+wsize);
  //check the splinter in header
  cr_assert(sfHeader->splinter==0,"splinter bit in header is not 0");
  cr_assert(sfHeader->splinter_size==0,"splinter_size in header is not 0");
  cr_assert(fp==freelist_head,"the new free blcok is not freelist_head");
  cr_assert(freelist_head->next!=NULL,"the new free blcok is not inserted into freelist");
  cr_assert(freelist_head->next->prev==freelist_head,"the new free blcok is not inserted into freelist");
}

Test(sf_memsuite, realloc_smaller_size_3_freeblock_coalesces_next_free_and_info, .init = sf_mem_init, .fini = sf_mem_fini) {
  int *x = sf_malloc(4);
  int *y = sf_malloc(48);
  int *z = sf_malloc(4);
  int* a = sf_malloc(4);
  info p1,p2;
  sf_info(&p1);
  (void)z;(void)a;
  memset(x, 0, 0);
  sf_free(z);
  y=sf_realloc(y,2);
  sf_info(&p2);
  //sf_varprint(y);//take a look
  //sf_snapshot(true);
  sf_header *sfHeader = (void*)((char *) y-wsize);
  sf_free_header *fp = (void*)((char*)getFooter((void*)sfHeader)+wsize);
  sf_footer* fpFooter = (void*)((char*)getFooter((void*)fp));
  //check the splinter in header
  cr_assert(sfHeader->splinter==0,"splinter bit in header is not 0");
  cr_assert(sfHeader->splinter_size==0,"splinter_size in header is not 0");
  cr_assert(fp==freelist_head,"the new free blcok is not freelist_head");
  cr_assert(freelist_head->next!=NULL,"the new free blcok is not inserted into freelist");
  cr_assert(freelist_head->next->prev==freelist_head,"the new free blcok is not inserted into freelist");
  cr_assert(fp->header.alloc==0,"the new free blcok doesn't coalesce with next block");
  cr_assert(getSize(*(sf_header*)fp)==64,"the new free blcok doesn't coalesce with next block");
  cr_assert(fpFooter->alloc==0,"the footer of free block is not same as header");
  cr_assert(getSize(*(sf_header*)fpFooter)==64,"the footer of free block is not same as header");
  cr_assert(p2.coalesces-p1.coalesces==1,"the coalesces statics doesn't increase by 1");
}

Test(sf_memsuite, realloc_smaller_size_4_splinter_coalesces_next_free_and_info, .init = sf_mem_init, .fini = sf_mem_fini) {
  int *x = sf_malloc(4);
  int *y = sf_malloc(32);
  int *z = sf_malloc(4);
  int* a = sf_malloc(4);
  info p1,p2;
  sf_info(&p1);
  (void)z;(void)a;
  memset(x, 0, 0);
  sf_free(z);
  y=sf_realloc(y,2);
  sf_info(&p2);
  //sf_varprint(y);//take a look
  //sf_snapshot(true);
  sf_header *sfHeader = (void*)((char *) y-wsize);
  sf_free_header *fp = (void*)((char*)getFooter((void*)sfHeader)+wsize);
  sf_footer* fpFooter = (void*)((char*)getFooter((void*)fp));

  cr_assert(sfHeader->splinter==0,"splinter bit in header is not 0");
  cr_assert(sfHeader->splinter_size==0,"splinter_size in header is not 0");
  cr_assert(fp==freelist_head,"the new free blcok is not freelist_head");
  cr_assert(freelist_head->next!=NULL,"the new free blcok is not inserted into freelist");
  cr_assert(freelist_head->next->prev==freelist_head,"the new free blcok is not inserted into freelist");
  cr_assert(fp->header.alloc==0,"the new free blcok doesn't coalesce with next block");
  cr_assert(getSize(*(sf_header*)fp)==48,"the new free blcok doesn't coalesce with next block");
  cr_assert(fpFooter->alloc==0,"the footer of free block is not same as header");
  cr_assert(getSize(*(sf_header*)fpFooter)==48,"the footer of free block is not same as header");
  cr_assert(p2.coalesces-p1.coalesces==1,"the coalesces statics doesn't increase by 1");
  cr_assert(p2.splinterBlocks-p1.splinterBlocks==0,"splinteringblcoks shold increase by 0");
  cr_assert(p2.splintering-p1.splintering==0,"splintering shold increase by 0");
}

Test(sf_memsuite, realloc_larger_size_1_no_splinter_adjacent_fb_enough_space, .init = sf_mem_init, .fini = sf_mem_fini) {
  int *x = sf_malloc(4);
  int *y = sf_malloc(2);
  int *z = sf_malloc(4);
  int* a = sf_malloc(4);
  info p1,p2;
  sf_info(&p1);
  (void)z;(void)a;
  memset(x, 0, 0);
  sf_free(z);
  y=sf_realloc(y,33);
  sf_info(&p2);
  //sf_varprint(y);//take a look
  //sf_snapshot(true);
  sf_header *sfHeader = (void*)((char *) y-wsize);
  sf_free_header *fp = (void*)((char*)getFooter((char*)a-wsize)+wsize);
  sf_footer* fpFooter = (void*)((char*)getFooter((void*)fp));
  //check the splinter in header
  cr_assert(sfHeader->splinter==0,"splinter bit in header is not 0");
  cr_assert(sfHeader->splinter_size==0,"splinter_size in header is not 0");
  cr_assert(getSize(*sfHeader)==64,"block_size in header is not 64");
  cr_assert(sfHeader->padding_size==15,"padding_size in header is not 15");
  cr_assert(fp==freelist_head," freelist_head is not in right place");
  cr_assert(freelist_head->next==NULL,"freelist is not correct");
  cr_assert(fp->header.alloc==0,"alloc bit in freelist_head is not 0");
  cr_assert(getSize(*(sf_header*)fp)==getSize(*(sf_header*)fpFooter),"the block_size on freelist_head and footer is not same");
  cr_assert(fpFooter->alloc==0,"the alloc of footer of freelist_head is not 0");
  cr_assert(p2.coalesces-p1.coalesces==0,"this shold not count for coalesces");
}

Test(sf_memsuite, realloc_larger_size_2_has_splinter_adjacent_fb_enough_space, .init = sf_mem_init, .fini = sf_mem_fini) {
  int *x = sf_malloc(4);
  int *y = sf_malloc(17);
  int *z = sf_malloc(4);
  int* a = sf_malloc(4);
  info p1,p2;
  sf_info(&p1);
  (void)z;(void)a;
  memset(x, 0, 0);
  sf_free(z);
  y=sf_realloc(y,33);
  sf_info(&p2);
  //sf_varprint(y);//take a look
  // sf_snapshot(true);
  sf_header *sfHeader = (void*)((char *) y-wsize);
  sf_footer* sfFooter = (void*)((char*)getFooter((void*)sfHeader));
  sf_free_header *fp = (void*)((char*)getFooter((char*)a-wsize)+wsize);
  sf_footer* fpFooter = (void*)((char*)getFooter((void*)fp));
  //check the splinter in header
  cr_assert(sfHeader->splinter==1,"splinter bit in header is not 1");
  cr_assert(sfFooter->splinter==1,"splinter bit in footer is not 1");
  cr_assert(sfHeader->splinter_size==16,"splinter_size in header is not 16");
  cr_assert(getSize(*sfHeader)==80,"block_size in header is not 80");
  cr_assert(getSize(*(sf_header*)sfFooter)==80,"block_size in footer is not 80");
  cr_assert(sfHeader->padding_size==15,"padding_size in header is not 15");
  cr_assert(fp==freelist_head," freelist_head is not in right place");
  cr_assert(freelist_head->next==NULL,"freelist is not correct");
  cr_assert(fp->header.alloc==0,"alloc bit in freelist_head is not 0");
  cr_assert(getSize(*(sf_header*)fp)==getSize(*(sf_header*)fpFooter),"the block_size on freelist_head and footer is not same");
  cr_assert(fpFooter->alloc==0,"the alloc of footer of freelist_head is not 0");
  cr_assert(p2.coalesces-p1.coalesces==0,"this shold not count for coalesces");
  cr_assert(p2.splinterBlocks-p1.splinterBlocks==1,"splinteringblocks shold increase by 16");
  cr_assert(p2.splintering-p1.splintering==16,"splintering shold increase by 16");
}

Test(sf_memsuite, realloc_larger_size_3_adjacent_fb_enough_space_split, .init = sf_mem_init, .fini = sf_mem_fini) {
  int *x = sf_malloc(4);
  int *y = sf_malloc(32);
  info p1,p2;
  sf_info(&p1);
  memset(x, 0, 0);
  y=sf_realloc(y,33);
  sf_info(&p2);
  //sf_varprint(y);//take a look
  //sf_snapshot(true);
  sf_header *sfHeader = (void*)((char *) y-wsize);
  sf_footer* sfFooter = (void*)((char*)getFooter((void*)sfHeader));
  sf_free_header *fp = (void*)((char*)sfFooter+wsize);
  sf_footer* fpFooter = (void*)((char*)getFooter((void*)fp));

  cr_assert(getSize(*sfHeader)==64,"block_size in header is not 64");
  cr_assert(getSize(*(sf_header*)sfFooter)==64,"block_size in footer is not 64");
  cr_assert(sfHeader->padding_size==15,"padding_size in header is not 15");
  cr_assert(fp==freelist_head," freelist_head is not in right place");
  cr_assert(freelist_head->next==NULL,"freelist is not correct");
  cr_assert(fp->header.alloc==0,"alloc bit in freelist_head is not 0");
  cr_assert(getSize(*(sf_header*)fp)==getSize(*(sf_header*)fpFooter),"the block_size on freelist_head and footer is not same");
  cr_assert(fpFooter->alloc==0,"the alloc of footer of freelist_head is not 0");
  cr_assert(p2.padding-p1.padding==15,"padding shold increase by 15");
  cr_assert(p1.peakMemoryUtilization==36.0/4096.0,"peakMemoryUtilization of p1 is not correct");
  cr_assert(p2.peakMemoryUtilization==37.0/4096.0,"peakMemoryUtilization of p2 is not correct");
}

Test(sf_memsuite, realloc_larger_size_4_adjacent_fb_not_enough_space, .init = sf_mem_init, .fini = sf_mem_fini) {
  int *x = sf_malloc(4);
  int *y = sf_malloc(16);
  int *z = sf_malloc(4);
  int* a = sf_malloc(4);
  int* b = sf_malloc(96);
  int* c = sf_malloc(16);
  info p1,p2;
  sf_info(&p1);
  (void)z;(void)a;(void)c;
  memset(x, 0, 0);
  sf_free(z);
  sf_free(b);
  y=sf_realloc(y,64);
  sf_info(&p2);
  // sf_varprint(y);//take a look
  // sf_snapshot(true);
  sf_header *sfHeader = (void*)((char *) y-wsize);
  sf_footer* sfFooter = (void*)((char*)getFooter((void*)sfHeader));
  sf_free_header *fp = (void*)((char*)getFooter((char*)x-wsize)+wsize);
  sf_footer* fpFooter = (void*)((char*)getFooter((void*)fp));
  sf_free_header *fp2=(void*)((char*)sfFooter+wsize);
  sf_footer* fp2Footer = (void*)((char*)getFooter((void*)fp2));
  sf_free_header *fpLast=(void*)((char*)getFooter((char*)c-wsize)+wsize);
  sf_footer* fpLastFooter = (void*)((char*)getFooter((void*)fpLast));

  cr_assert(getSize(*sfHeader)==80,"block_size in header is not 80");
  cr_assert(getSize(*(sf_header*)sfFooter)==80,"block_size in footer is not 80");
  cr_assert(fp==freelist_head," freelist_head is not in right place");
  cr_assert(fp->prev==NULL," freelist_head is not in right place");
  cr_assert(fp->next== fp2,"freelist is not correct");
  cr_assert(freelist_head== fp2->prev,"freelist is not correct");
  cr_assert(freelist_head->next->prev==fp,"the new free blcok is not inserted into freelist");
  cr_assert(fp2->next==fpLast,"freelist is not correct");
  cr_assert(fpLast->prev== fp2,"freelist is not correct");
  cr_assert(fpLast->next== NULL,"freelist is not correct");
  cr_assert(fp->header.alloc==0,"alloc bit in freelist_head is not 0");
  cr_assert(fp2->header.alloc==0,"alloc bit in fp2 is not 0");
  cr_assert(fpLast->header.alloc==0,"alloc bit in fpLast is not 0");
  cr_assert(getSize(*(sf_header*)fp)==getSize(*(sf_header*)fpFooter),"the block_size on freelist_head and footer is not same");
  cr_assert(getSize(*(sf_header*)fp2)==getSize(*(sf_header*)fp2Footer),"the block_size on freelist_head and footer is not same");
  cr_assert(getSize(*(sf_header*)fpLast)==getSize(*(sf_header*)fpLastFooter),"the block_size on freelist_head and footer is not same");
  cr_assert(fpFooter->alloc==0,"the alloc of footer of freelist_head is not 0");
  cr_assert(fp2Footer->alloc==0,"the alloc of footer of fp2 is not 0");
  cr_assert(fpLastFooter->alloc==0,"the alloc of footer of fpLast is not 0");
  cr_assert(p1.coalesces==0,"no coalesces yet");
  cr_assert(p2.coalesces==1,"this shold count for coalesces");
}

Test(sf_memsuite, realloc_larger_size_5_adjacent_fb_not_enough_space_extends_heap, .init = sf_mem_init, .fini = sf_mem_fini) {
  int *x = sf_malloc(4);
  int *y = sf_malloc(16);
  int *z = sf_malloc(4);
  int* a = sf_malloc(4);
  int* b = sf_malloc(96);
  int* c = sf_malloc(16);
  info p1,p2;
  sf_info(&p1);
  (void)z;(void)a;(void)c;
  memset(x, 0, 0);
  sf_free(x);
  sf_free(b);
  sf_free(z);
  y=sf_realloc(y,4096*3);
  sf_info(&p2);
  // sf_varprint(y);//take a look
  // sf_snapshot(true);
  sf_header *sfHeader = (void*)((char *) y-wsize);
  sf_footer* sfFooter = (void*)((char*)getFooter((void*)sfHeader));
  sf_free_header *fp = getHeapHead();
  sf_footer* fpFooter = (void*)((char*)getFooter((void*)fp));
  sf_free_header *fp2=(void*)((char*)getFooter((char*)a-wsize)+wsize);
  sf_footer* fp2Footer = (void*)((char*)getFooter((void*)fp2));
  sf_free_header *fpLast=(void*)((char*)getFooter((char*)y-wsize)+wsize);
  sf_footer* fpLastFooter = (void*)((char*)getFooter((void*)fpLast));

  cr_assert(getSize(*sfHeader)==4096*3+16,"block_size in header is not right");
  cr_assert(getSize(*(sf_header*)sfFooter)==4096*3+16,"block_size in footer is not right");
  cr_assert(fp==freelist_head," freelist_head is not in right place");
  cr_assert(fp->prev==NULL," freelist_head is not in right place");
  cr_assert(fp->next== fp2,"freelist is not correct");
  cr_assert(freelist_head== fp2->prev,"freelist is not correct");
  cr_assert(freelist_head->next->prev==fp,"the new free blcok is not inserted into freelist");
  cr_assert(fp2->next==fpLast,"freelist is not correct");
  cr_assert(fpLast->prev== fp2,"freelist is not correct");
  cr_assert(fpLast->next== NULL,"freelist is not correct");
  cr_assert(fp->header.alloc==0,"alloc bit in freelist_head is not 0");
  cr_assert(fp2->header.alloc==0,"alloc bit in fp2 is not 0");
  cr_assert(fpLast->header.alloc==0,"alloc bit in fpLast is not 0");
  cr_assert(getSize(*(sf_header*)fp)==getSize(*(sf_header*)fpFooter),"the block_size on freelist_head and footer is not same");
  cr_assert(getSize(*(sf_header*)fp2)==getSize(*(sf_header*)fp2Footer),"the block_size on freelist_head and footer is not same");
  cr_assert(getSize(*(sf_header*)fpLast)==getSize(*(sf_header*)fpLastFooter),"the block_size on freelist_head and footer is not same");
  cr_assert(fpFooter->alloc==0,"the alloc of footer of freelist_head is not 0");
  cr_assert(fp2Footer->alloc==0,"the alloc of footer of fp2 is not 0");
  cr_assert(fpLastFooter->alloc==0,"the alloc of footer of fpLast is not 0");
  cr_assert(fpLastFooter==(void*)((char*)sf_sbrk(0)-wsize),"the last free block doesnt place at end of heap");
  cr_assert(p1.coalesces==0,"no coalesces yet");
  cr_assert(p2.coalesces==4,"this shold count for 4 coalesces, 3 heap_extendings, 1 free case4");
}

Test(sf_memsuite, realloc_larger_size_6_adjacent_fb_not_enough_space_extends_heap_add_to_adjacent_fb, .init = sf_mem_init, .fini = sf_mem_fini) {
  int *x = sf_malloc(4);
  info p1,p2;
  sf_info(&p1);
  memset(x, 0, 0);
  x=sf_realloc(x,4096*3);
  sf_info(&p2);
  // sf_varprint(x);//take a look
  // sf_snapshot(true);
  sf_header *sfHeader = (void*)((char *) x-wsize);
  sf_footer* sfFooter = (void*)((char*)getFooter((void*)sfHeader));
  sf_free_header *fp = (void*)((char*)sfFooter+wsize);
  sf_footer* fpFooter = (void*)((char*)getFooter((void*)fp));

  cr_assert(sfHeader==getHeapHead(),"the header of the block should not move");
  cr_assert(getSize(*sfHeader)==4096*3+16,"block_size in header is not 80");
  cr_assert(getSize(*(sf_header*)sfFooter)==4096*3+16,"block_size in footer is not 80");
  cr_assert(fp==freelist_head," freelist_head is not in right place");
  cr_assert(fp->prev==NULL," freelist_head is not in right place");
  cr_assert(fp->next==NULL," freelist_head is not in right place");
  cr_assert(getSize(*(sf_header*)fp)==getSize(*(sf_header*)fpFooter),"the block_size on freelist_head and footer is not same");
  cr_assert(fp->header.alloc==0,"alloc bit in freelist_head is not 0");
  cr_assert(fpFooter->alloc==0,"the alloc of footer of freelist_head is not 0");
  cr_assert(p1.coalesces==0,"no coalesces yet");
  cr_assert(p2.coalesces==3,"this shold count for 3 coalesces, 3 heap_extendings");
  cr_assert(p1.peakMemoryUtilization==4.0/4096.0,"peakMemoryUtilization of p1 is not correct");
  cr_assert(p2.peakMemoryUtilization==4096.0*3.0/4096.0/4.0,"peakMemoryUtilization of p2 is not correct");
}

Test(sf_memsuite, realloc_larger_size_7_extends_heap_is_last_option_search_list_first, .init = sf_mem_init, .fini = sf_mem_fini) {
  int *a= sf_malloc(4016);
  int *x = sf_malloc(4);
  info p1,p2;
  sf_info(&p1);
  memset(x, 0, 0);
  sf_free(a);
  x=sf_realloc(x,144);
  sf_info(&p2);
  // sf_varprint(x);//take a look
  // sf_snapshot(true);
  sf_header *sfHeader = getHeapHead();
  sf_footer* sfFooter = (void*)((char*)getFooter((void*)sfHeader));
  sf_free_header *fp = (void*)((char*)getFooter(getHeapHead())+wsize);
  sf_footer* fpFooter = (void*)((char*)getFooter((void*)fp));

  cr_assert(sfHeader==getHeapHead(),"the header of the block should not move");
  cr_assert(getSize(*sfHeader)==160,"block_size in header is not 160");
  cr_assert(getSize(*(sf_header*)sfFooter)==160,"block_size in footer is not 160");
  cr_assert(fp==freelist_head," freelist_head is not in right place");
  cr_assert(fp->prev==NULL," freelist_head is not in right place");
  cr_assert(fp->next==NULL," freelist_head is not in right place");
  cr_assert(getSize(*(sf_header*)fp)==getSize(*(sf_header*)fpFooter),"the block_size on freelist_head and footer is not same");
  cr_assert(fp->header.alloc==0,"alloc bit in freelist_head is not 0");
  cr_assert(fpFooter->alloc==0,"the alloc of footer of freelist_head is not 0");
  cr_assert(p1.coalesces==0,"no coalesces yet");
  cr_assert(p2.coalesces==1,"this shold count for 1 coalesces");
  cr_assert(p1.peakMemoryUtilization==4020.0/4096.0,"peakMemoryUtilization of p1 is not correct");
  cr_assert(p2.peakMemoryUtilization==4020.0/4096.0,"peakMemoryUtilization of p2 is not correct");
}

Test(sf_memsuite, sf_malloc_too_larger_size, .init = sf_mem_init, .fini = sf_mem_fini) {
  int *a= sf_malloc(4096*4);
  cr_assert(a==NULL,"it shold return NULL");
  cr_assert(freelist_head==NULL,"no free list yet");
}

Test(sf_memsuite, too_many_sf_malloc_calls_size_excess_16384, .init = sf_mem_init, .fini = sf_mem_fini) {
  int *x = sf_malloc(4096);
  int *y = sf_malloc(4096);
  int *z = sf_malloc(4096);
  int* a = sf_malloc(4096);

  info p1,p2;
  sf_info(&p1);
  (void)z;(void)a;
  memset(x, 0, 0);
  int *yt=y;
  y=sf_realloc(y,4096);
  sf_info(&p2);
  // sf_varprint(y);//take a look
  // sf_snapshot(true);

  cr_assert(a==NULL,"no space large enough");
  cr_assert(y==yt,"y should not change");
  cr_assert(a==NULL,"no space large enough");
  cr_assert(freelist_head->next==NULL," freelist_head is not in right place");
  cr_assert(getFooter((void*)freelist_head)==(void*)((char*)sf_sbrk(0)-wsize)," freelist_head is not in right place");
 }

Test(sf_memsuite, sf_malloc_0_and_realloc_0, .init = sf_mem_init, .fini = sf_mem_fini) {
  int *a= sf_malloc(4);
  int *x=sf_malloc(0);
  a=sf_realloc(a,0);
  cr_assert(x==NULL,"it shold return NULL");
  cr_assert(a==NULL,"it should return NULL");
  cr_assert(freelist_head==(getHeapHead()),"all free");
  }

Test(sf_memsuite, the_data_after_sf_realloc_should_not_change, .init = sf_mem_init, .fini = sf_mem_fini) {
  int *a= sf_malloc(4);
  int *x=sf_malloc(5);
  *a=7;(void)x;
  a=sf_realloc(a,60);
  cr_assert(*a==7,"data shold remain same");
}

Test(sf_memsuite, realloc_same_r_size_but_a_splinter_and_adajcent_fb, .init = sf_mem_init, .fini = sf_mem_fini) {
  int *x = sf_malloc(4);
  int *y = sf_malloc(17);
  int *z  = sf_malloc(4);
  info p1,p2;
  (void)z;
  memset(x, 0, 0);
  y=sf_realloc(y,2);
  sf_free(z);
  sf_info(&p1);
  y=sf_realloc(y,2);
  sf_info(&p2);
  //sf_varprint(y);//take a look
  //sf_snapshot(true);
  sf_header *sfHeader = (void*)((char *) y-wsize);
  sf_free_header* fp=(void*)((char*)getFooter((void*)sfHeader)+wsize);
  //check the splinter in header
  cr_assert(sfHeader->splinter==0,"splinter bit in header is not 0");
  cr_assert(sfHeader->splinter_size==0,"splinter_size in header is not 0");
  cr_assert(fp==freelist_head,"didnt coalesces");
  cr_assert(fp->next==NULL,"didnt coalesces");
  cr_assert(p1.coalesces==1,"coalesces is incorrect");
  cr_assert(p2.coalesces==2,"coalesces is incorrect");
}
