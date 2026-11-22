/*
** `regs` shell command. Shows the selectors actually loaded in the CPU,
** so we can check that the GDT is in use: 0x08 code, 0x10 data, 0x18 stack.
*/

#include "regs.h"
#include "printk.h"
#include "types.h"

static const char *segment_name(uint32_t selector)
{
    if (selector == 0x08)
        return ("kernel code");
    if (selector == 0x10)
        return ("kernel data");
    if (selector == 0x18)
        return ("kernel stack");
    if (selector == 0x20)
        return ("user code");
    if (selector == 0x28)
        return ("user data");
    if (selector == 0x30)
        return ("user stack");
    return ("other");
}

static void print_seg(const char *name, uint32_t selector)
{
    printk("%s  0x%08x  %s\n", name, selector, segment_name(selector));
}

void regs_print(void)
{
    uint32_t cs;
    uint32_t ds;
    uint32_t ss;
    uint32_t esp;
    uint32_t ebp;
    uint32_t eflags;

    __asm__ volatile ("mov %%cs, %0" : "=r"(cs));
    __asm__ volatile ("mov %%ds, %0" : "=r"(ds));
    __asm__ volatile ("mov %%ss, %0" : "=r"(ss));
    __asm__ volatile ("mov %%esp, %0" : "=r"(esp));
    __asm__ volatile ("mov %%ebp, %0" : "=r"(ebp));
    __asm__ volatile ("pushfl; pop %0" : "=r"(eflags));
    print_seg("cs    ", cs);
    print_seg("ds    ", ds);
    print_seg("ss    ", ss);
    printk("esp   0x%08x\n", esp);
    printk("ebp   0x%08x\n", ebp);
    printk("eflags 0x%08x\n", eflags);
}
