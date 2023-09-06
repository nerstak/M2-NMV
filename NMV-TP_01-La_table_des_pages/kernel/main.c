#include <idt.h>                            /* see there for interrupt names */
#include <memory.h>                               /* physical page allocator */
#include <printk.h>                      /* provides printk() and snprintk() */
#include <string.h>                                     /* provides memset() */
#include <syscall.h>                         /* setup system calls for tasks */
#include <task.h>                             /* load the task from mb2 info */
#include <types.h>              /* provides stdint and general purpose types */
#include <vga.h>                                         /* provides clear() */
#include <x86.h>                                    /* access to cr3 and cr2 */

#define PGT_VALID_MASK 0x1
#define PGT_ADDRESS_MASK 0xFFFFFFFFFF000
#define PGT_HUGEPAGE_MASK 0x80


#define PGT_IS_VALID(p) (p&PGT_VALID_MASK)
#define PGT_IS_HUGEPAGE(p) (p&PGT_HUGEPAGE_MASK)
#define PGT_ADDRESS(p) (p&PGT_ADDRESS_MASK)

void print_pgt(paddr_t pml, uint8_t lvl) {
    uint64_t *p = (uint64_t *) pml;

    if (lvl == 4) {
        printk("cr3: 0x%lx\n", pml);
    }

    if (lvl < 1) {
        return;
    }

    for (int i = 0; i < 512; ++i) {
        if (PGT_IS_VALID(p[i])) {
            printk("pml%d %d: 0x%lx\n", lvl, i, PGT_ADDRESS(p[i]));
            if (!PGT_IS_HUGEPAGE(p[i])) {
                print_pgt((paddr_t) PGT_ADDRESS(p[i]), lvl - 1);
            }
        }
    }
}

__attribute__((noreturn))
void die(void) {
    /* Stop fetching instructions and go low power mode */
    asm volatile ("hlt");

    /* This while loop is dead code, but it makes gcc happy */
    while (1);
}

__attribute__((noreturn))
void main_multiboot2(void *mb2) {
    clear();                                     /* clear the VGA screen */
    printk("Rackdoll OS\n-----------\n\n");                 /* greetings */

    setup_interrupts();                           /* setup a 64-bits IDT */
    setup_tss();                                  /* setup a 64-bits TSS */
    interrupt_vector[INT_PF] = pgfault;      /* setup page fault handler */

    remap_pic();               /* remap PIC to avoid spurious interrupts */
    disable_pic();                         /* disable anoying legacy PIC */
    sti();                                          /* enable interrupts */

//
//    print_pgt(store_cr3(), 4);
//    struct task fake;
//    paddr_t new;
//    fake.pgt = store_cr3();
//    new = alloc_page();
//    map_page(&fake, 0x201000, new);


    load_tasks(mb2);                         /* load the tasks in memory */
    run_tasks();                                 /* run the loaded tasks */

    printk("\nGoodbye!\n");                                 /* fairewell */
    die();                        /* the work is done, we can die now... */
}
