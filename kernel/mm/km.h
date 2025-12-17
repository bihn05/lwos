// kernel memory allocatin

#ifndef _KM_H_
#define _KM_H_

#include <stdint.h>
#include <mm/vmm.h>
#include <mm/pmm.h>

#define KHEAP_START 0xc0000000
#define KHEAP_INITIAL_SIZE 0x100000

typedef struct chunk_header {
    struct chunk_header* next;  // next chunk
    uint32_t size;              // current chunk size
    bool is_free;               // chunk status
} chunk_header_t;

// global heap ptr
chunk_header_t* kheap_first_chunk = 0;

void kheap_init();
void* kmalloc(uint32_t size);
void kfree(void* ptr);

void kheap_init() {
    // map pm to khp vm range
    uint32_t curr_addr = KHEAP_START;
    uint32_t end_addr = KHEAP_START + KHEAP_INITIAL_SIZE;

    while (curr_addr < end_addr) {
        uint32_t phys_page = pmm_alloc_page();
        map_page(curr_addr, phys_page, 0x3);
        curr_addr += 4096;
    }

    // init 1st chunk
    // ptr ptr to heap head
    kheap_first_chunk = (chunk_header_t*)KHEAP_START;

    // cover whole heap
    kheap_first_chunk->next = 0;
    kheap_first_chunk->size = KHEAP_INITIAL_SIZE - sizeof(chunk_header_t);
    kheap_first_chunk->is_free = true;
}
void* kmalloc(uint32_t size) {
    if (size == 0)return 0; // r u kid me

    // align to 8
    uint32_t aligned_size = (size + 3) & ~3;

    chunk_header_t* curr = kheap_first_chunk;

    while (curr != 0) {
        // find free chunk size enough
        if (curr->is_free && curr->size >= aligned_size) {
            // splitable or not
            if (curr->size > aligned_size + sizeof(chunk_header_t) + 4) {
                // where split
                chunk_header_t* new_chunk = (chunk_header_t*)((uint8_t*)curr + sizeof(chunk_header_t) + aligned_size);

                // set new chunk properties
                new_chunk->is_free = true;
                new_chunk->size = curr->size - aligned_size - sizeof(chunk_header_t);
                new_chunk->next = curr->next;

                // update current chunk properties
                curr->size = aligned_size;
                curr->next = new_chunk;
            }

            // occupied
            curr->is_free = false;

            return (void*)((uint8_t*)curr + sizeof(chunk_header_t));
        }
        curr = curr->next;
    }
    // to do
    // map again ?
    return 0;
}
void kfree(void* ptr) {
    if (ptr == 0)return; // ugnhhhhhhhhhh

    chunk_header_t* header = (chunk_header_t*)((uint8_t*)ptr - sizeof(chunk_header_t));

    header->is_free = true;

    // merge
    chunk_header_t* curr = kheap_first_chunk;

    while (curr != 0 && curr->next != 0) {
        // if curr + next all free
        if (curr->is_free && curr->next->is_free) {
            // then merge them
            curr->size += sizeof(chunk_header_t) + curr->next->size;

            // skip
            curr->next = curr->next->next;
        } else {
            curr = curr->next;
        }
    }
}

#endif