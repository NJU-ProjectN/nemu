#include <isa.h>
#include <memory/paddr.h>
#include <memory/vaddr.h>

paddr_t isa_mmu_translate(vaddr_t addr, int type, int len) {
  return MEM_RET_FAIL;
}
