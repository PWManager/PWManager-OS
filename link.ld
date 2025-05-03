ENTRY(_start)

SECTIONS
{
    . = 1M; /* загружаем ядро по адресу 1MB */
    
    .multiboot : {
        KEEP(*(.multiboot))
    }

    .text : {
        *(.text)
    }

    .data : {
        *(.data)
    }

    .bss : {
        *(.bss)
    }
}
