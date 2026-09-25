/* Stub kernel — Limine module (ET_DYN / PIE, loaded by ZealBooter) */
OUTPUT_FORMAT(elf64-littleaarch64)
ENTRY(kernel_entry)

PHDRS
{
    text PT_LOAD FLAGS(0x05);
    rodata PT_LOAD FLAGS(0x04);
    data PT_LOAD FLAGS(0x06);
    dynamic PT_DYNAMIC FLAGS(0x06);
    /* RWX page for freestanding JIT smoke (Limine honours PF_X|PF_W). */
    jit PT_LOAD FLAGS(0x07);
}

SECTIONS
{
    . = 0;

    .text : {
        . = ALIGN(0x800);
        KEEP(*(.text.vectors))
        *(.text .text.*)
    } :text

    . = ALIGN(CONSTANT(MAXPAGESIZE));

    .rodata : {
        *(.rodata .rodata.*)
    } :rodata

    . = ALIGN(CONSTANT(MAXPAGESIZE));

    .data : {
        *(.data .data.*)
    } :data

    .dynamic : {
        *(.dynamic)
    } :data :dynamic

    .rela.dyn : {
        *(.rela*)
    } :data

    .bss : {
        *(.bss .bss.*)
        *(COMMON)
    } :data

    . = ALIGN(CONSTANT(MAXPAGESIZE));

    .jit : {
        KEEP(*(.jit))
        . = ALIGN(CONSTANT(MAXPAGESIZE));
    } :jit

    /DISCARD/ : {
        *(.eh_frame*)
        *(.note .note.*)
        *(.ARM.attributes)
        *(.comment)
    }
}
