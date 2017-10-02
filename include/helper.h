
#ifndef HELPER_H
#define HELPER_H
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#define wsize 8
#define dsize 16


size_t getSize(sf_header header);

void* makeFooter(char* header);

void* getFooter(char* header);

void* find_best_fit(size_t size);

void place_block(char* bp,size_t b_size,size_t r_size);

void* coalesces(char* bp);

void* getHeader(char* footer);

void insertFreeList(char* bp);

void* getHeapHead();
#endif
