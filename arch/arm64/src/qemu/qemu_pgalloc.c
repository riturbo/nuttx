/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <sys/types.h>
#include <assert.h>
#include <debug.h>

#include <nuttx/arch.h>
#include <nuttx/addrenv.h>
#include <nuttx/pgalloc.h>

#include "chip.h"
#include "arm64_mmu.h"

#include "qemu_memmap.h"

#ifdef CONFIG_MM_PGALLOC

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* Currently, page cache memory must be allocated in DRAM.  There are other
 * possibilities, but the logic in this file will have to extended in order
 * handle any other possibility.
 */
#if 0
#ifndef CONFIG_ARCH_PGPOOL_MAPPING
#  error CONFIG_ARCH_PGPOOL_MAPPING must be selected
#endif

#ifndef CONFIG_A133_DDRCS_PGHEAP
#  error CONFIG_A133_DDRCS_PGHEAP must be selected
#endif

#ifndef CONFIG_A133_DDRCS_PGHEAP_OFFSET
#  error CONFIG_A133_DDRCS_PGHEAP_OFFSET must be specified
#endif

#if (CONFIG_A133_DDRCS_PGHEAP_OFFSET & MM_PGMASK) != 0
#  warning CONFIG_A133_DDRCS_PGHEAP_OFFSET is not aligned to a page boundary
#endif

#ifndef CONFIG_A133_DDRCS_PGHEAP_SIZE
#  error CONFIG_A133_DDRCS_PGHEAP_SIZE must be specified
#endif

#if (CONFIG_A133_DDRCS_PGHEAP_SIZE & MM_PGMASK) != 0
#  warning CONFIG_A133_DDRCS_PGHEAP_SIZE is not aligned to a page boundary
#endif
#endif
/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: up_allocate_pgheap
 *
 * Description:
 *   If there is a page allocator in the configuration, then this function
 *   must be provided by the platform-specific code.  The OS initialization
 *   logic will call this function early in the initialization sequence to
 *   get the page heap information needed to configure the page allocator.
 *
 * Input parameters:
 *   heap_start - A double pointer to the start address of the pgheap
 *   heap_size - A pointer to the size of the pgheap
 *
 * Returned Value:
 *   None
 *
 ****************************************************************************/

void up_allocate_pgheap(void **heap_start, size_t *heap_size)
{
  DEBUGASSERT(heap_start && heap_size);

  *heap_start = (void *)((uintptr_t)SUNXI_DRAM_BASE +
                             0x4000000);
  *heap_size  = 0x4000000;
}

#endif /* CONFIG_MM_PGALLOC */
