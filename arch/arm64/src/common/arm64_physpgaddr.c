/****************************************************************************
 * arch/arm/src/armv8-a/arm64_physpgaddr.c
 *
 * Licensed to the Apache Software Foundation (ASF) under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.  The
 * ASF licenses this file to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance with the
 * License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  See the
 * License for the specific language governing permissions and limitations
 * under the License.
 *
 ****************************************************************************/

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

#include "arm64_pgalloc.h"

#ifdef CONFIG_MM_PGALLOC

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: arm64_physpgaddr
 *
 * Description:
 *   Check if the virtual address lies in the user data area and, if so
 *   get the mapping to the physical address in the page pool.
 *
 ****************************************************************************/

uintptr_t arm64_physpgaddr(uintptr_t vaddr)
{
  uint32_t *l2table;
  uintptr_t paddr;
  uint32_t l1entry;
  int index;

  /* Check if this address is within the range of one of the virtualized user
   * address regions.
   */

  if (arm64_uservaddr(vaddr))
    {
      /* Yes.. Get Level 1 page table entry corresponding to this virtual
       * address.
       */

      l1entry = 0;//mmu_l1_getentry(vaddr);
      if (0)
        {
          /* Get the physical address of the level 2 page table from the
           * level 1 page table entry.
           */

          paddr = ((uintptr_t)l1entry & PMD_PTE_PADDR_MASK);

          /* Get the virtual address of the base of level 2 page table */

          l2table = (uint32_t *)arm64_pgvaddr(paddr);

          if (l2table)
            {
              /* Invalidate D-Cache line containing this virtual address so
               * that we re-read from physical memory
               */

              index = (vaddr & SECTION_MASK) >> MM_PGSHIFT;
              up_invalidate_dcache((uintptr_t)&l2table[index],
                                   (uintptr_t)&l2table[index] +
                                    sizeof(uint32_t));

              /* Get the Level 2 page table entry corresponding to this
               * virtual address.  Extract the physical address of the page
               * containing the mapping of the virtual address.
               */

              paddr = ((uintptr_t)l2table[index] & PTE_SMALL_PADDR_MASK);

              /* Add the correct offset and return the physical address
               * corresponding to the virtual address.
               */

              return paddr + (vaddr & MM_PGMASK);
            }
        }
    }

  /* No mapping available */

  return 0;
}

#endif /* CONFIG_MM_PGALLOC */
