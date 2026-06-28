/*
 * Cupcake overlay — same RAM slot as game-and-watch-retro-go-sd-cupcake (SD layout).
 * Load image (.overlay_cupcake) is extracted to cupcake.bin; BSS is zeroed by firmware at runtime.
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
  .overlay_cupcake : {
    __ram_emu_cupcake_start__ = .;
    KEEP(*(.cupcake_entry))
    KEEP(*(.cupcake_entry*))
    KEEP(*(.text .text*))
    *(.text .text*)
    *(.rodata .rodata*)
    *(.data .data*)
    . = ALIGN(4);
    _OVERLAY_CUPCAKE_LOAD_END = .;
  } > RAM_EMU

  _OVERLAY_CUPCAKE_LOAD_START = LOADADDR(.overlay_cupcake);
  _OVERLAY_CUPCAKE_SIZE = SIZEOF(.overlay_cupcake);

  .overlay_cupcake_bss _OVERLAY_CUPCAKE_LOAD_END : {
    _OVERLAY_CUPCAKE_BSS_START = .;
    *(.bss .bss*)
    *(COMMON)
    . = ALIGN(4);
    __ram_emu_cupcake_end__ = .;
    _OVERLAY_CUPCAKE_BSS_END = .;
    ASSERT(ABSOLUTE(_OVERLAY_CUPCAKE_BSS_END) < __RAM_EMU_END__, "Error: CUPCAKE BSS overflow");
  } > RAM_EMU

  _OVERLAY_CUPCAKE_BSS_SIZE = SIZEOF(.overlay_cupcake_bss);

  /DISCARD/ : {
    *(.comment)
    *(.note*)
  }
}
