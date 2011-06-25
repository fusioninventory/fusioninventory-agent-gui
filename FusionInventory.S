.section .rodata
.global _fusinv
_fusinv:
.incbin "FusionInventory.exe"
/*.byte 0*/
.equ fusinv_len, . - _fusinv
.align 4 /* TODO: adjust for 64-bit platforms */
.global _fusinv_size
_fusinv_size:
.int fusinv_len
