#ifndef _INCLUDE_MEMORY_H_
#define _INCLUDE_MEMORY_H_


#include <idt.h>
#include <task.h>
#include <types.h>

/**
 * Allocate a physical page identity mapped
 * Returns an address that can be used for doing whatever we want for our page
 * Content is undefined
 * @return Address of physical page
 */
paddr_t alloc_page(void);

/**
 * Mark physical address as usable for allocation
 * @param addr Address of physical page
 */
void free_page(paddr_t addr);  /* Release a page allocated with alloc_page() */

/**
 * Map a physical address to a virtual one inside the PML pages
 * Creates intermediate PML pages if missing
 * @param ctx Context of task
 * @param vaddr Virtual address
 * @param paddr Physical address
 */
void map_page(struct task *ctx, vaddr_t vaddr, paddr_t paddr);

/**
 * Load task, allocate PMLs and pages for payload and bss
 * Also maps the PML for kernel addresses
 * @param ctx Ctx of task
 */
void load_task(struct task *ctx);

/**
 * Change the CR3 to the address of the PML4
 * @param ctx  Ctx of task
 */
void set_task(struct task *ctx);

void duplicate_task(struct task *ctx);

/**
 * Create a new mapping for a given virtual address and given context
 * Allocate PMLs, new page and memory space (cleared)
 * @param ctx Ctx of task
 * @param vaddr Virtual address
 */
void mmap(struct task *ctx, vaddr_t vaddr);

/**
 * Delete mapping for a given virtual address and a given context
 * Clear, free and invalidate
 * @param ctx Ctx of task
 * @param vaddr Virtual address
 */
void munmap(struct task *ctx, vaddr_t vaddr);

/**
 * Handling of page fault interruptions
 * Terminate current task or lazy allocate stack memory
 * @param ctx Ctx of interruption
 */
void pgfault(struct interrupt_context *ctx);


#endif
