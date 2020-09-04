#ifndef __MEMORY_PADDR_H__
#define __MEMORY_PADDR_H__

#include <common.h>

#define PMEM_SIZE (256 * 1024 * 1024)

/* convert the guest physical address in the guest program to host virtual address in NEMU */
void* guest_to_host(paddr_t addr);
/* convert the host virtual address in NEMU to guest physical address in the guest program */
paddr_t host_to_guest(void *addr);

word_t paddr_read(paddr_t addr, int len);
void paddr_write(paddr_t addr, word_t data, int len);

#endif
