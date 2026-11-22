/*
** Mandatory KFS-2: flat GDT at 0x00000800, then a readable kernel stack dump.
** bonus/src/gdt.c replaces this file when BONUS=1, so the bonus keeps its
** own GDT and its own print_k_stack.
*/

#include "gdt.h"
#include "vga.h"

extern uint32_t stack_top[];

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
    gdt_set_gate(0, 0, 0, 0, 0);
    gdt_set_gate(1, 0, 0xFFFFFFFF, 0x9A, 0xCF);
    gdt_set_gate(2, 0, 0xFFFFFFFF, 0x92, 0xCF);
    gdt_set_gate(3, 0, 0xFFFFFFFF, 0x92, 0xCF);
    gdt_set_gate(4, 0, 0xFFFFFFFF, 0xFA, 0xCF);
    gdt_set_gate(5, 0, 0xFFFFFFFF, 0xF2, 0xCF);
    gdt_set_gate(6, 0, 0xFFFFFFFF, 0xF2, 0xCF);
    gdt_flush(&gp);
    vga_write("\nGDT loaded at 0x00000800\n");
}

static void vga_write_hex(uint32_t value)
{
    static const char digits[] = "0123456789abcdef";
    char text[9];
    int i;

    i = 8;
    text[i] = '\0';
    while (i > 0)
    {
        i--;
        text[i] = digits[value & 0xF];
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

void print_k_stack(void)
{
    uint32_t *stack_pointer;
    uint32_t *word;
    size_t words_on_line;

    __asm__ volatile ("mov %%esp, %0" : "=r"(stack_pointer));
    vga_write("kernel stack: esp=0x");
    vga_write_hex((uint32_t)stack_pointer);
    vga_write(" top=0x");
    vga_write_hex((uint32_t)stack_top);
    vga_write(" (");
    vga_write_uint((uint32_t)stack_top - (uint32_t)stack_pointer);
    vga_write(" bytes used)\n");
    word = stack_pointer;
    words_on_line = 0;
    while (word < stack_top)
    {
        if (words_on_line == 0)
        {
            vga_write_hex((uint32_t)word);
            vga_write(":");
        }
        vga_write(" ");
        vga_write_hex(*word++);
        if (++words_on_line == 4 || word == stack_top)
        {
            vga_write("\n");
            words_on_line = 0;
        }
    }
}
