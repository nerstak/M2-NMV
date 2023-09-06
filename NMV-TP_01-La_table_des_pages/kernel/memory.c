#include <memory.h>
#include <printk.h>
#include <string.h>
#include <x86.h>


#define PHYSICAL_POOL_PAGES  64
#define PHYSICAL_POOL_BYTES  (PHYSICAL_POOL_PAGES << 12)
#define BITSET_SIZE          (PHYSICAL_POOL_PAGES >> 6)

#define PAGE_SIZE 4096

#define PGT_VALID_MASK 0x1
#define PGT_WRITABLE_MASK 0x2 // ou 10
#define PGT_USER_MASK 0x4 // ou 100
#define PGT_ADDRESS_MASK 0x7FFFFFFFFF000
#define PGT_HUGEPAGE_MASK 0x80

#define PGT_IS_VALID(p) (p&PGT_VALID_MASK)
#define PGT_IS_HUGEPAGE(p) (p&PGT_HUGEPAGE_MASK)
#define PGT_ADDRESS(p) (p&PGT_ADDRESS_MASK)

#define PGT_PML4_INDEX_MASK 0xFF8000000000
#define PGT_PML3_INDEX_MASK 0x7FC0000000
#define PGT_PML2_INDEX_MASK 0x3FE00000
#define PGT_PML1_INDEX_MASK 0x1FF000

#define PGT_PML4_INDEX(v) ((v & PGT_PML4_INDEX_MASK) >> 39)
#define PGT_PML3_INDEX(v) ((v & PGT_PML3_INDEX_MASK) >> 30)
#define PGT_PML2_INDEX(v) ((v & PGT_PML2_INDEX_MASK) >> 21)
#define PGT_PML1_INDEX(v) ((v & PGT_PML1_INDEX_MASK) >> 12)


extern __attribute__((noreturn)) void die(void);

static uint64_t bitset[BITSET_SIZE];

// Pool of physical bytes?
// Aligned on a byte
static uint8_t pool[PHYSICAL_POOL_BYTES] __attribute__((aligned(0x1000)));


paddr_t alloc_page(void) {
    size_t i, j;
    uint64_t v;

    for (i = 0; i < BITSET_SIZE; i++) {
        // If its 1 only, we skip (1 means occupied)
        // Memory full, so no use of checking this line
        if (bitset[i] == 0xffffffffffffffff)
            continue;

        for (j = 0; j < 64; j++) {
            v = 1ul << j;
            // Check if space at j is free or occupied
            if (bitset[i] & v)
                continue;

            // Now it occupied
            bitset[i] |= v;
            // MSBs are determined by position in bitset
            // LSBs (12 ones) are determined by pool physical memory
            // Note: this way of allocating means that, by default we only have 64 identity pages??
            return (((64 * i) + j) << 12) + ((paddr_t) &pool);
        }
    }

    printk("[error] Not enough identity free page\n");
    return 0;
}

void free_page(paddr_t addr) {
    paddr_t tmp = addr;
    size_t i, j;
    uint64_t v;

    tmp = tmp - ((paddr_t) &pool);
    tmp = tmp >> 12;

    i = tmp / 64;
    j = tmp % 64;
    v = 1ul << j;

    if ((bitset[i] & v) == 0) {
        printk("[error] Invalid page free %p\n", addr);
        die();
    }
    // memset here?

    bitset[i] &= ~v;
}


/*
 * Memory model for Rackdoll OS
 *
 * +----------------------+ 0xffffffffffffffff
 * | Higher half          |
 * | (unused)             |
 * +----------------------+ 0xffff800000000000
 * | (impossible address) |
 * +----------------------+ 0x00007fffffffffff
 * | User                 |
 * | (text + data + heap) |
 * +----------------------+ 0x2000000000
 * | User                 | 0x1ffffffff8 --> Seems like a legit page fault: we are asking memory from stack
 * | (stack)              | 0x1FFFFF3000
 * +----------------------+ 0x40000000 -> Lazy allocation: only allocating given mapped addresses at use. Useful for glibc for example: it has a pool of addresses but maybe all won't be used
 * | Kernel               |
 * | (valloc)             |
 * +----------------------+ 0x201000
 * | Kernel               |
 * | (APIC)               |
 * +----------------------+ 0x200000
 * | Kernel               |
 * | (text + data)        |
 * +----------------------+ 0x100000
 * | Kernel               |
 * | (BIOS + VGA)         |
 * +----------------------+ 0x0
 *
 * This is the memory model for Rackdoll OS: the kernel is located in low
 * addresses. The first 2 MiB are identity mapped and not cached.
 * Between 2 MiB and 1 GiB, there are kernel addresses which are not mapped
 * with an identity table.
 * Between 1 GiB and 128 GiB is the stack addresses for user processes growing
 * down from 128 GiB.
 * The user processes expect these addresses are always available and that
 * there is no need to map them explicitely.
 * Between 128 GiB and 128 TiB is the heap addresses for user processes.
 * The user processes have to explicitely map them in order to use them.
 */

void map_page(struct task *ctx, vaddr_t vaddr, paddr_t paddr) {
    uint64_t *p = (uint64_t *) ctx->pgt;
    paddr_t tmp;

    // PML4
    if (!PGT_IS_VALID(p[PGT_PML4_INDEX(vaddr)])) {
//        printk("Entry is invalid\n");
        tmp = alloc_page();

        memset((void *) tmp, 0, PAGE_SIZE);
        p[PGT_PML4_INDEX(vaddr)] |= (tmp | PGT_VALID_MASK | PGT_WRITABLE_MASK | PGT_USER_MASK);
    } else {
//        printk("Address is already valid\n");
    }
    p = (uint64_t *) PGT_ADDRESS(p[PGT_PML4_INDEX(vaddr)]);
//    printk("0x%lx\n", p);


    // PML3
    if (!PGT_IS_VALID(p[PGT_PML3_INDEX(vaddr)])) {
//        printk("Entry is invalid\n");
        tmp = alloc_page();

        memset((void *) tmp, 0, PAGE_SIZE);
        p[PGT_PML3_INDEX(vaddr)] |= (tmp | PGT_VALID_MASK | PGT_WRITABLE_MASK | PGT_USER_MASK);
    } else {
//        printk("Address is already valid\n");
    }
    p = (uint64_t *) PGT_ADDRESS(p[PGT_PML3_INDEX(vaddr)]);
//    printk("0x%lx\n", p);


    //PML2
    if (!PGT_IS_VALID(p[PGT_PML2_INDEX(vaddr)])) {
//        printk("Entry is invalid\n");
        tmp = alloc_page();

        memset((void *) tmp, 0, PAGE_SIZE);
        p[PGT_PML2_INDEX(vaddr)] |= (tmp | PGT_VALID_MASK | PGT_WRITABLE_MASK | PGT_USER_MASK);
    } else {
//        printk("Address is already valid\n");
    }
    p = (uint64_t *) PGT_ADDRESS(p[PGT_PML2_INDEX(vaddr)]);
//    printk("0x%lx\n", p);


    // PML1
    if (!PGT_IS_VALID(p[PGT_PML1_INDEX(vaddr)])) {
        // First we clear the space in the page
        memset((void *) p[PGT_PML1_INDEX(vaddr)], 0, PAGE_SIZE);
        // THEN we save the physical address in the page
        p[PGT_PML1_INDEX(vaddr)] |= (paddr | PGT_VALID_MASK | PGT_WRITABLE_MASK | PGT_USER_MASK);
    } else {
//        printk("[error] virtual address 0x%lx already mapped\n", vaddr);
    }
}

void load_task(struct task *ctx) {
    // PML4
    paddr_t paddr = alloc_page();
    memset((void *) paddr, 0, PAGE_SIZE);
    ctx->pgt = paddr;

    // PML3
    uint64_t *p = (uint64_t *) ctx->pgt;
    paddr = alloc_page();
    memset((void *) paddr, 0, PAGE_SIZE);
    p[0] |= (paddr | PGT_VALID_MASK | PGT_WRITABLE_MASK | PGT_USER_MASK);

    // Linking first entry of PML3 to address of PML2 dedicated to kernel addresses
    p = (uint64_t *) store_cr3(); // Addr of PML4
    paddr = PGT_ADDRESS(*(uint64_t *) PGT_ADDRESS(*p)); // Addr of PML3 = *(addr PML4); Addr of PML2 = *(addr PML3)
    p = (uint64_t *) PGT_ADDRESS(*((uint64_t *) ctx->pgt));
    p[0] |= (paddr | PGT_VALID_MASK | PGT_WRITABLE_MASK | PGT_USER_MASK);


//    printk("load_paddr 0x%lx\n", ctx->load_paddr);
//    printk("load_end_paddr 0x%lx\n", ctx->load_end_paddr);



    // Allocating payload
    vaddr_t vaddr;
    for (paddr = ctx->load_paddr; paddr < ctx->load_end_paddr; paddr += PAGE_SIZE) { // Moving forward paddr by 4KiB
        vaddr = ctx->load_vaddr + paddr - ctx->load_paddr; // Moving forward
//        printk("payload: vaddr x0%lx to paddr 0x%lx\n", vaddr, paddr);
        map_page(ctx, vaddr, paddr); // Aaaand we map (it will go down to PML1 by itself)
    }

    vaddr_t bss_start_vaddr = ctx->load_vaddr + ctx->load_end_paddr - ctx->load_paddr;
    // Allocating bss
    for (vaddr = bss_start_vaddr; vaddr < ctx->bss_end_vaddr; vaddr += PAGE_SIZE) {
        paddr = alloc_page();
        // Note that we allocate and set to 0 for the bss
//        printk("bss: vaddr x0%lx to paddr 0x%lx\n", vaddr, paddr);
        memset((void *) paddr, 0, PAGE_SIZE);
        map_page(ctx, vaddr, paddr);
    }
}

void set_task(struct task *ctx) {
    load_cr3(ctx->pgt);
}

void mmap(struct task *ctx, vaddr_t vaddr) {
    paddr_t paddr = alloc_page();
    // Important to clear memory space: we don't want task to access previous data
    memset((void *) paddr, 0, PAGE_SIZE);
    map_page(ctx, vaddr, paddr);
}

void munmap(struct task *ctx, vaddr_t vaddr) {
    // From ctx->prg, get CR3 and convert vaddr to physical
    // Then we free
    uint64_t *p = (uint64_t *) ctx->pgt; // PML4 addr
    p = (uint64_t *) PGT_ADDRESS(p[PGT_PML4_INDEX(vaddr)]); // PML3 addr
    p = (uint64_t *) PGT_ADDRESS(p[PGT_PML3_INDEX(vaddr)]); // PML2 addr
    p = (uint64_t *) PGT_ADDRESS(p[PGT_PML2_INDEX(vaddr)]); // PML1 addr
    p = (uint64_t *) PGT_ADDRESS(p[PGT_PML1_INDEX(vaddr)]); // page addr

    // Cleaning memory to prevent access in the future
    memset(p, 0, PAGE_SIZE);
    // Marking physical address as usable
    free_page((paddr_t) p);
    // Invalidating TLB for this address now that it was unmapped
    invlpg(vaddr);
}

void pgfault(struct interrupt_context *ctx) {
    // ctx is NOT the context of the task
    // It is the context of the kernel during interruptions, not the process context (and not associated with a task)
    //
    // To recover the context of the interrupted context process, we need to call the macro current()
    //
    // See: http://books.gigatux.nl/mirror/kerneldevelopment/0672327201/ch06lev1sec5.html


    uint64_t addr = store_cr2();
    // From start of stack to end of stack
    if (addr > 0x40000000 && addr <= 0x2000000000) {
        // Lazy allocation
        mmap((void *) current(), addr);
    } else {
        // Legitimate page fault
        printk("Page fault at %p\n", ctx->rip);
        printk("  cr2 = %p\n", store_cr2());
        exit_task(ctx);
    }
}

void duplicate_task(struct task *ctx) {
}
