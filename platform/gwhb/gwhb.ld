/*
 * Cupcake GWHB image — generic Universal Homebrew Header binary for
 * game-and-watch-retro-go-sd gwhb-generic-homebrew-loader. Any filename
 * under /roms/homebrew/ works; dispatch is by the 512-byte header.
 */
OUTPUT_FORMAT("elf32-littlearm", "elf32-littlearm", "elf32-littlearm")
OUTPUT_ARCH(arm)

__RAM_EMU_START__ = 0x2404B000;
__RAM_EMU_LENGTH__ = 724K;
__RAM_EMU_END__ = __RAM_EMU_START__ + __RAM_EMU_LENGTH__;

MEMORY
{
  RAM_EMU (rwx) : ORIGIN = __RAM_EMU_START__, LENGTH = __RAM_EMU_LENGTH__
}

SECTIONS
{
  .gwhb_header : {
    __gwhb_image_start__ = .;
    KEEP(*(.gwhb_header))
  } > RAM_EMU

  __gwhb_header_size_chk__ = ASSERT(SIZEOF(.gwhb_header) == 512, "Error: gwhb_header_t must be exactly 512 bytes");

  .gwhb_entry : {
    KEEP(*(.gwhb_entry))
  } > RAM_EMU

  __gwhb_entry_offset_chk__ = ASSERT(ADDR(.gwhb_entry) - __gwhb_image_start__ == 512,
         "Error: GWHB entry point must sit exactly at offset 512");

  .gwhb_load : {
    *(.text .text*)
    *(.rodata .rodata*)
    *(.data .data*)
    . = ALIGN(4);
    _GWHB_LOAD_END = .;
  } > RAM_EMU

  _GWHB_LOAD_START = LOADADDR(.gwhb_header);
  _GWHB_LOAD_SIZE = _GWHB_LOAD_END - __gwhb_image_start__;

  .gwhb_bss _GWHB_LOAD_END : {
    _GWHB_BSS_START = .;
    *(.bss .bss*)
    *(COMMON)
    . = ALIGN(4);
    _GWHB_BSS_END = .;
    ASSERT(ABSOLUTE(_GWHB_BSS_END) < __RAM_EMU_END__, "Error: cupcake BSS overflow");
  } > RAM_EMU

  _GWHB_BSS_SIZE = SIZEOF(.gwhb_bss);

  /DISCARD/ : {
    *(.comment)
    *(.note*)
  }
}
