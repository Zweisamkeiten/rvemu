  .text
  .global	_start

_start:
  addi    a0, zero, 1
  addi    sp, sp, -4
  lb      a0, 0(sp)
  lh      a0, 0(sp)
  lw      a0, 0(sp)
  ld      a0, 0(sp)
  lbu     a0, 0(sp)
  lhu     a0, 0(sp)
  lwu     a0, 0(sp)
  flw     ft0, 0(a0)
  fld     ft0, 0(a0)
  fence   rw, rw
  fence.i
  addi    a0, a0, 1
  slli    a0, a0, 1
  slti    a0, a0, 1
  sltiu   a0, a0, 1
  xori    a0, a0, 1
  srli    a0, a0, 1
  srai    a0, a0, 1
  ori     a0, a0, 1
  andi    a0, a0, 1
  auipc   sp, 0x0
  addiw   a0, a0, 1
  slliw   a0, a0, 1
  srliw   a0, a0, 1
  sraiw   a0, a0, 1
  sb      a4, 0(a5)
  sh      a4, 0(a5)
  sw      a4, 0(a5)
  sd      a4, 0(a5)
  fsw     ft0, 0(a5)
  fsd     ft0, 0(a5)
  add     a0, a0, a0
  sll     a0, a0, a0
  slt     a0, a0, a0
  sltu    a0, a0, a0
  xor     a0, a0, a0
  srl     a0, a0, a0
  or      a0, a0, a0
  and     a0, a0, a0
  mul     a0, a0, a0
  mulh    a0, a0, a0
  mulhsu  a0, a0, a0
  mulhu   a0, a0, a0
  div     a0, a0, a0
  divu    a0, a0, a0
  rem     a0, a0, a0
  remu    a0, a0, a0
  sub     a0, a0, a0
  sra     a0, a0, a0
  lui     a0, 0x4
  addw    a0, a0, a0
  sllw    a0, a0, a0
  srlw    a0, a0, a0
  mulw    a0, a0, a0
  divw    a0, a0, a0
  divuw   a0, a0, a0
  remw    a0, a0, a0
  remuw   a0, a0, a0
  subw    a0, a0, a0
  sraw    a0, a0, a0
  jalr    zero,0(ra)
  jal     zero,0
  ecall
  ebreak
  csrrw   zero,mtvec,a4
  csrrs   t0,mcause,zero
  csrrc   a4, mcause, a0
  csrrwi  a0, mcause, 0x1
  csrrsi  a0, mcause, 0x1
  csrrci  a0, mcause, 0x1
  beq     a0,zero,_start
  bne     a0,a1,_start
  blt     a0,a1,_start
  bge     a0,a1,_start
  bltu    a0,a1,_start
  bgeu    a0,a1,_start

  .end
