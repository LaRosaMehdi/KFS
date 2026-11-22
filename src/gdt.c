/*
** Mandatory KFS-2.
** The GDT tells the CPU what each segment selector means. Ours is flat:
** every segment starts at 0 and covers the whole 4 GiB, so an address is
** just a linear address. Ring 0 is the kernel, ring 3 is user mode.
** Stacks are ordinary data segments; only their selector differs.
**
** bonus/src/gdt.c replaces this file, so the bonus keeps its own copy.
*/

#include "gdt.h"
#include "vga.h"

extern uint32_t stack_top[];            /* boot.s: high end of the 16 KiB stack */

static struct gdt_entry *const g_gdt = (struct gdt_entry *)GDT_ADDRESS;

static void gdt_set_gate(int num, uint32_t base, uint32_t limit,
        uint8_t access, uint8_t gran)
{
    g_gdt[num].base_low = (uint16_t)(base & 0xFFFF);
    g_gdt[num].base_mid = (uint8_t)((base >> 16) & 0xFF);
    g_gdt[num].base_high = (uint8_t)((base >> 24) & 0xFF);
    g_gdt[num].limit_low = (uint16_t)(limit & 0xFFFF);
    g_gdt[num].granularity = (uint8_t)(((limit >> 16) & 0x0F) | (gran & 0xF0));
    g_gdt[num].access = access;
}

/* lgdt alone keeps the old selectors in ds/ss/cs. Reload them, and reload
** cs with a far jump (a normal jump cannot change cs). */
static void gdt_flush(struct gdt_ptr *gp)
{
    __asm__ volatile (
        "lgdt %0\n"
        "movw $0x10, %%ax\n"
        "movw %%ax, %%ds\n"
        "movw %%ax, %%es\n"
        "movw %%ax, %%fs\n"
        "movw %%ax, %%gs\n"
        "movw $0x18, %%ax\n"
        "movw %%ax, %%ss\n"
        "ljmp $0x08, $1f\n"
        "1:\n"
        :
        : "m"(*gp)
        : "eax", "memory");
}

void gdt_init(void)
{
    struct gdt_ptr gp;

    gp.limit = (uint16_t)(GDT_ENTRY_COUNT * sizeof(struct gdt_entry) - 1);
    gp.base = GDT_ADDRESS;
    gdt_set_gate(0, 0, 0, 0, 0);                    /* required null entry */
    /* 0x9A / 0xFA: present, ring 0 / 3, readable code.
    ** 0x92 / 0xF2: present, ring 0 / 3, writable data.
    ** 0xCF: 32-bit segment, limit counted in 4 KiB pages. */
    gdt_set_gate(1, 0, 0xFFFFFFFF, 0x9A, 0xCF);     /* 0x08 kernel code */
    gdt_set_gate(2, 0, 0xFFFFFFFF, 0x92, 0xCF);     /* 0x10 kernel data */
    gdt_set_gate(3, 0, 0xFFFFFFFF, 0x92, 0xCF);     /* 0x18 kernel stack */
    gdt_set_gate(4, 0, 0xFFFFFFFF, 0xFA, 0xCF);     /* 0x20 user code */
    gdt_set_gate(5, 0, 0xFFFFFFFF, 0xF2, 0xCF);     /* 0x28 user data */
    gdt_set_gate(6, 0, 0xFFFFFFFF, 0xF2, 0xCF);     /* 0x30 user stack */
    gdt_flush(&gp);
}

static void vga_write_hex(uint32_t value, size_t digits)
{
    static const char symbols[] = "0123456789abcdef";
    char text[9];
    size_t i;

    i = digits;
    text[i] = '\0';
    while (i > 0)
    {
        i--;
        text[i] = symbols[value & 0xF];
        value >>= 4;
    }
    vga_write(text);
}

static void vga_write_uint(uint32_t value)
{
    char buffer[11];
    char *digit;

    digit = buffer + 10;
    *digit = '\0';
    if (value == 0)
        *--digit = '0';
    while (value > 0)
    {
        *--digit = (char)('0' + (value % 10));
        value /= 10;
    }
    vga_write(digit);
}

/* Ring is bits 5-6 of the access byte. Printed from memory, not from the
** values we just wrote, so the line shows what really sits at 0x800. */
static void print_segment(uint32_t selector, const char *name)
{
    const struct gdt_entry *entry;
    uint8_t ring;

    entry = &g_gdt[selector / sizeof(struct gdt_entry)];
    ring = (entry->access >> 5) & 3;
    vga_write("  0x");
    vga_write_hex(selector, 2);
    vga_write("  ");
    vga_write(name);
    vga_write("  ring ");
    vga_putchar((char)('0' + ring));
    vga_write("\n");
}

void gdt_print(void)
{
    vga_write("\nGDT 0x00000800, flat, base 0, 4 GiB\n");
    vga_write("  0x00  null          (unused)\n");
    print_segment(0x08, "kernel code ");
    print_segment(0x10, "kernel data ");
    print_segment(0x18, "kernel stack");
    print_segment(0x20, "user code   ");
    print_segment(0x28, "user data   ");
    print_segment(0x30, "user stack  ");
}

void print_k_stack(void)
{
    uint32_t *stack_pointer;
    uint32_t *word;
    size_t words_on_line;

    __asm__ volatile ("mov %%esp, %0" : "=r"(stack_pointer));
    vga_write("kernel stack  esp=0x");
    vga_write_hex((uint32_t)stack_pointer, 8);
    vga_write("  top=0x");
    vga_write_hex((uint32_t)stack_top, 8);
    vga_write("  ");
    vga_write_uint((uint32_t)stack_top - (uint32_t)stack_pointer);
    vga_write(" bytes, newest first\n");
    word = stack_pointer;
    words_on_line = 0;
    while (word < stack_top)
    {
        if (words_on_line == 0)
        {
            vga_write_hex((uint32_t)word, 8);
            vga_write(":");
        }
        vga_write(" ");
        vga_write_hex(*word, 8);
        word++;
        words_on_line++;
        if (words_on_line == 4 || word == stack_top)
        {
            vga_write("\n");
            words_on_line = 0;
        }
    }
}
