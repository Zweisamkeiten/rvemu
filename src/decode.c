/**
 * @file decode.c
 * @brief
 */

#include "common.h"
#include "rvemu.h"

#define QUADRANT(data) (((data) >> 0) & 0x3)

/*
 * normal types
 */
#define OPCODE(data) (((data) >> 2) & 0x1f)
#define RD(data) (((data) >> 7) & 0x1f)
#define RS1(data) (((data) >> 15) & 0x1f)
#define RS2(data) (((data) >> 20) & 0x1f)
#define RS3(data) (((data) >> 27) & 0x1f)
#define FUNCT2(data) (((data) >> 25) & 0x3)
#define FUNCT3(data) (((data) >> 12) & 0x7)
#define FUNCT7(data) (((data) >> 25) & 0x7f)
#define IMM116(data) (((data) >> 26) & 0x3f)

static inline inst_t inst_rtype_read(uint32_t data) {
  return (inst_t){
      .rs1 = RS1(data),
      .rs2 = RS2(data),
      .rd = RD(data),
  };
}

static inline inst_t inst_itype_read(uint32_t data) {
  return (inst_t){
      .imm = (int32_t)data >> 20,
      .rs1 = RS1(data),
      .rd = RD(data),
  };
}

static inline inst_t inst_stype_read(uint32_t data) {
  uint32_t imm115 = (data >> 25) & 0x7f;
  uint32_t imm40 = (data >> 7) & 0x1f;

  // imm always signed extend
  int32_t imm = (imm115 << 5) | imm40;
  imm = (imm << (32 - (11 - 0 + 1))) >> (32 - (11 - 0 + 1));

  return (inst_t){
      .imm = imm,
      .rs1 = RS1(data),
      .rs2 = RS2(data),
  };
}

static inline inst_t inst_btype_read(uint32_t data) {
  uint32_t imm12 = (data >> 31) & 0x1;
  uint32_t imm105 = (data >> 25) & 0x3f;
  uint32_t imm41 = (data >> 8) & 0xf;
  uint32_t imm11 = (data >> 7) & 0x1;

  int32_t imm = (imm12 << 12) | (imm11 << 11) | (imm105 << 5) | (imm41 << 1);
  imm = (imm << (32 - (12 - 0 + 1))) >> (32 - (12 - 0 + 1));

  return (inst_t){
      .imm = imm,
      .rs1 = RS1(data),
      .rs2 = RS2(data),
  };
}

static inline inst_t inst_utype_read(uint32_t data) {
  return (inst_t){
      .imm = (int32_t)data & 0xfffff000,
      .rd = RD(data),
  };
}

static inline inst_t inst_jtype_read(uint32_t data) {
  uint32_t imm20 = (data >> 31) & 0x1;
  uint32_t imm101 = (data >> 20) & 0x3ff;
  uint32_t imm11 = (data >> 19) & 0x1;
  uint32_t imm1912 = (data >> 12) & 0xff;

  int32_t imm = (imm20 << 20) | (imm1912 << 12) | (imm11 << 11) | (imm101 << 1);
  imm = (imm << (32 - (20 - 0 + 1))) >> (32 - (20 - 0 + 1));
  return (inst_t){
      .imm = (int32_t)data & 0xfffff000,
      .rd = RD(data),
  };
}

static inline inst_t inst_csrtype_read(uint32_t data) {
  return (inst_t){
      .csr = data >> 20,
      .rs1 = RS1(data),
      .rd = RD(data),
  };
}

/**
 * @brief decode the inst
 *
 * @param inst
 * @param data
 */
void inst_decode(inst_t *inst, uint32_t data) {
  uint32_t quadrand = QUADRANT(data);
  switch (quadrand) {
  case 0x0:
    panic("unimplemented");
  case 0x1:
    panic("unimplemented");
  case 0x2:
    panic("unimplemented");
  case 0x3: {
    uint32_t opcode = OPCODE(data);
    switch (opcode) {
    case 0x0: {
      uint32_t funct3 = FUNCT3(data);

      *inst = inst_itype_read(data);
      switch (funct3) {
      case 0x0: /* LB */
        inst->type = inst_lb;
        return;
      case 0x1: /* LH */
        inst->type = inst_lh;
        return;
      case 0x2: /* LW */
        inst->type = inst_lw;
        return;
      case 0x3: /* LD */
        inst->type = inst_ld;
        return;
      case 0x4: /* LBU */
        inst->type = inst_lbu;
        return;
      case 0x5: /* LHU */
        inst->type = inst_lhu;
        return;
      case 0x6: /* LWU */
        inst->type = inst_lwu;
        return;
      default:
        unreachable();
      }
      unreachable();
    }
    case 0x1: {
      uint32_t funct3 = FUNCT3(data);

      *inst = inst_itype_read(data);
      switch (funct3) {
      case 0x2: /* FLW */
        inst->type = inst_flw;
        return;
      case 0x3: /* FLD */
        inst->type = inst_fld;
        return;
      default:
        unreachable();
      }
      unreachable();
    }
    case 0x3: {
      uint32_t funct3 = FUNCT3(data);

      switch (funct3) {
      case 0x0: { /* FENCE */
        inst_t _inst = {0};
        *inst = _inst;
        inst->type = inst_fence;
        return;
      }
      case 0x1: { /* FENCE.I */
        inst_t _inst = {0};
        *inst = _inst;
        inst->type = inst_fence_i;
        return;
      }
      default:
        unreachable();
      }
      unreachable();
    }
    case 0x4: {
      uint32_t funct3 = FUNCT3(data);

      *inst = inst_itype_read(data);

      switch (funct3) {
      case 0x0: /* ADDI */
        inst->type = inst_addi;
        return;
      case 0x1: {
        uint32_t imm116 = IMM116(data);
        if (imm116 == 0) { /* SLLI */
          inst->type = inst_slli;
        } else {
          unreachable();
        }
        return;
      }
        unreachable();
      case 0x2: /* SLTI */
        inst->type = inst_slti;
        return;
      case 0x3: /* SLTIU */
        inst->type = inst_sltiu;
        return;
      case 0x4: /* XORI */
        inst->type = inst_xori;
        return;
      case 0x5: {
        uint32_t imm116 = IMM116(data);
        if (imm116 == 0) { /* SRLI */
          inst->type = inst_srli;
        } else if (imm116 == 0x10) { /* SRAI */
          inst->type = inst_srai;
        } else {
          unreachable();
        }
        return;
      }
        unreachable();
      case 0x6: /* ORI */
        inst->type = inst_ori;
        return;
      case 0x7: /* ANDI */
        inst->type = inst_andi;
        return;
      default:
        panic("unrecognized funct3");
      }
    }
      unreachable();
    case 0x5: { /* AUIPC */
      *inst = inst_utype_read(data);
      inst->type = inst_auipc;
      return;
    }
    case 0x6: {
      uint32_t funct3 = FUNCT3(data);
      uint32_t funct7 = FUNCT7(data);

      *inst = inst_itype_read(data);
      switch (funct3) {
      case 0x0: /* ADDIW */
        inst->type = inst_addiw;
        return;
      case 0x1: /* SLLIW */
        Assert(funct7 == 0, "the data is not match ADDIW funct7 00000000");
        inst->type = inst_slliw;
        return;
      case 0x5: {
        switch (funct7) {
        case 0x0: /* SRLIW */
          inst->type = inst_srliw;
          return;
        case 0x20: /* SRAIW */
          inst->type = inst_sraiw;
          return;
        default:
          unreachable();
        }
      }
        unreachable();
      default:
        panic("unimplemented");
      }
    }
      unreachable();
    case 0x8: {
      uint32_t funct3 = FUNCT3(data);

      *inst = inst_stype_read(data);
      switch (funct3) {
      case 0x0: /* SB */
        inst->type = inst_sb;
        return;
      case 0x1: /* SH */
        inst->type = inst_sh;
        return;
      case 0x2: /* SW */
        inst->type = inst_sw;
        return;
      case 0x3: /* SD */
        inst->type = inst_sd;
        return;
      default:
        unreachable();
      }
    }
      unreachable();
    case 0x9: {
      uint32_t funct3 = FUNCT3(data);

      *inst = inst_stype_read(data);
      switch (funct3) {
      case 0x2: /* FSW */
        inst->type = inst_fsw;
        return;
      case 0x3: /* FSD */
        inst->type = inst_fsd;
        return;
      default:
        unreachable();
      }
    }
      unreachable();
    case 0xc: {
      uint32_t funct3 = FUNCT3(data);
      uint32_t funct7 = FUNCT7(data);

      *inst = inst_rtype_read(data);
      switch (funct7) {
      case 0x0: {
        switch (funct3) {
        case 0x0: /* ADD */
          inst->type = inst_add;
          return;
        case 0x1: /* SLL */
          inst->type = inst_sll;
          return;
        case 0x2: /* SLT */
          inst->type = inst_slt;
          return;
        case 0x3: /* SLTU */
          inst->type = inst_sltu;
          return;
        case 0x4: /* XOR */
          inst->type = inst_xor;
          return;
        case 0x5: /* SRL */
          inst->type = inst_srl;
          return;
        case 0x6: /* OR */
          inst->type = inst_or;
          return;
        case 0x7: /* AND */
          inst->type = inst_and;
          return;
        default:
          unreachable();
        }
      }
        unreachable();
      case 0x1: {
        switch (funct3) {
        case 0x0: /* MUL */
          inst->type = inst_mul;
          return;
        case 0x1: /* MULH */
          inst->type = inst_mulh;
          return;
        case 0x2: /* MULHSU */
          inst->type = inst_mulhsu;
          return;
        case 0x3: /* MULHU */
          inst->type = inst_mulhu;
          return;
        case 0x4: /* DIV */
          inst->type = inst_div;
          return;
        case 0x5: /* DIVU */
          inst->type = inst_divu;
          return;
        case 0x6: /* REM */
          inst->type = inst_rem;
          return;
        case 0x7: /* REMU */
          inst->type = inst_remu;
          return;
        default:
          unreachable();
        }
      }
        unreachable();
      case 0x20: {
        switch (funct3) {
        case 0x0: /* SUB */
          inst->type = inst_sub;
          return;
        case 0x5: /* SRA */
          inst->type = inst_sra;
          return;
        default:
          unreachable();
        }
      }
        unreachable();
      default:
        unreachable();
      }
    }
    case 0xd: { /* LUI */
      *inst = inst_utype_read(data);
      inst->type = inst_lui;
      return;
    }
    case 0xe: {
      *inst = inst_rtype_read(data);

      uint32_t funct3 = FUNCT3(data);
      uint32_t funct7 = FUNCT7(data);

      switch (funct7) {
      case 0x0: {
        switch (funct3) {
        case 0x0: /* ADDW */
          inst->type = inst_addw;
          return;
        case 0x1: /* SLLW */
          inst->type = inst_sllw;
          return;
        case 0x5: /* SRLW */
          inst->type = inst_srlw;
          return;
        default:
          unreachable();
        }
      }
        unreachable();
      case 0x1: {
        switch (funct3) {
        case 0x0: /* MULW */
          inst->type = inst_mulw;
          return;
        case 0x4: /* DIVW */
          inst->type = inst_divw;
          return;
        case 0x5: /* DIVUW */
          inst->type = inst_divuw;
          return;
        case 0x6: /* REMW */
          inst->type = inst_remw;
          return;
        case 0x7: /* REMUW */
          inst->type = inst_remuw;
          return;
        default:
          unreachable();
        }
      }
        unreachable();
      case 0x20: {
        switch (funct3) {
        case 0x0: /* SUBW */
          inst->type = inst_subw;
          return;
        case 0x5: /* SRAW */
          inst->type = inst_sraw;
          return;
        default:
          unreachable();
        }
      }
        unreachable();
      default:
        unreachable();
      }
    }
    case 0x10: {
    }
      unreachable();
    case 0x11: {
    }
      unreachable();
    case 0x12: {
    }
      unreachable();
    case 0x18: {
      *inst = inst_btype_read(data);

      uint32_t funct3 = FUNCT3(data);
      switch (funct3) {
      case 0x0: /* BEQ */
        inst->type = inst_beq;
        return;
      case 0x1: /* BNE */
        inst->type = inst_bne;
        return;
      case 0x4: /* BLT */
        inst->type = inst_blt;
        return;
      case 0x5: /* BGE */
        inst->type = inst_bge;
        return;
      case 0x6: /* BLTU */
        inst->type = inst_bltu;
        return;
      case 0x7: /* BGEU */
        inst->type = inst_bgeu;
        return;
      default:
        unreachable();
      }
    }
      unreachable();
    case 0x19: { /* JALR */
      *inst = inst_itype_read(data);
      inst->type = inst_jalr;
      inst->cont = true;
      return;
    }
      unreachable();
    case 0x1b: { /* JAL */
      *inst = inst_jtype_read(data);
      inst->type = inst_jal;
      inst->cont = true;
      return;
    }
      unreachable();
    case 0x1c: {
      if (data == 0x73) { /* ECALL */
        inst->type = inst_ecall;
        inst->cont = true;
        return;
      } else if (data == 0x00100073) { /* EBREAK */
        inst->type = inst_ebreak;
        inst->cont = true;
        return;
      } else {
        uint32_t funct3 = FUNCT3(data);
        *inst = inst_csrtype_read(data);
        switch (funct3) {
        case 0x1: /* CSRRW */
          inst->type = inst_csrrw;
          return;
        case 0x2: /* CSRRS */
          inst->type = inst_csrrs;
          return;
        case 0x3: /* CSRRC */
          inst->type = inst_csrrc;
          return;
        case 0x5: /* CSRRWI */
          inst->type = inst_csrrwi;
          return;
        case 0x6: /* CSRRSI */
          inst->type = inst_csrrsi;
          return;
        case 0x7: /* CSRRCI */
          inst->type = inst_csrrci;
          return;
        default:
          unreachable();
        }
      }
      unreachable();
    }
    default:
      unreachable();
    }
  }
    unreachable();
  default:
    unreachable();
  }
}
