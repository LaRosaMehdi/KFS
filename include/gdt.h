#ifndef GDT_H
# define GDT_H

# include "types.h"

/*
** Mandatory KFS-2. Seven descriptors at 0x00000800: the null entry, then
** code, data and stack for the kernel and the same three for user mode.
** bonus/include/gdt.h replaces this header when BONUS=1.
*/

# define GDT_ADDRESS 0x00000800
# define GDT_ENTRY_COUNT 7

/* Hardware layout. Base and limit are split across several bytes. */
struct gdt_entry
{
    uint16_t limit_low;
    uint16_t base_low;
    uint8_t base_mid;
    uint8_t access;
    uint8_t granularity;
    uint8_t base_high;
} __attribute__((packed));

/* What lgdt reads: size minus one, then the table address. */
struct gdt_ptr
{
    uint16_t limit;
    uint32_t base;
} __attribute__((packed));

/* Writes the table and loads it. GRUB's GDT is replaced on purpose. */
void gdt_init(void);
/* Reads the table back and prints one plain line per segment. */
void gdt_print(void);
/* Prints the live kernel stack, from esp (newest) up to stack_top. */
void print_k_stack(void);

#endif
