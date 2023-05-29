/**
 * @file decode.c
 * @brief
 */

#include "common.h"
#include "rvemu.h"
#include "utils.h"

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
  uint32_t imm101 = (data >> 21) & 0x3ff;
  uint32_t imm11 = (data >> 20) & 0x1;
  uint32_t imm1912 = (data >> 12) & 0xff;

  int32_t imm = (imm20 << 20) | (imm1912 << 12) | (imm11 << 11) | (imm101 << 1);
  imm = (imm << (32 - (20 - 0 + 1))) >> (32 - (20 - 0 + 1));
  return (inst_t){
      .imm = imm,
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

static inline inst_t inst_fprtype_read(uint32_t data) {
  return (inst_t){
      .rs1 = RS1(data),
      .rs2 = RS2(data),
      .rs3 = RS3(data),
      .rd = RD(data),
  };
}

/**
 * compressed types
 */
#define COPCODE(data) (((data) >> 13) & 0x7)
#define CFUNCT1(data) (((data) >> 12) & 0x1)
#define CFUNCT2LOW(data) (((data) >> 5) & 0x3)
#define CFUNCT2HIGH(data) (((data) >> 10) & 0x3)
#define RP1(data) (((data) >> 7) & 0x7)
#define RP2(data) (((data) >> 2) & 0x7)
#define RC1(data) (((data) >> 7) & 0x1f)
#define RC2(data) (((data) >> 2) & 0x1f)

static inline inst_t inst_catype_read(uint16_t data) {
  return (inst_t){
      .rd = RP1(data) + 8,
      .rs2 = RP2(data) + 8,
      .rvc = true,
  };
}

static inline inst_t inst_crtype_read(uint16_t data) {
  return (inst_t){
      .rs1 = RC1(data),
      .rs2 = RC2(data),
      .rvc = true,
  };
}

static inline inst_t inst_citype_read(uint16_t data) {
  uint32_t imm40 = (data >> 2) & 0x1f;
  uint32_t imm5 = (data >> 12) & 0x1;
  int32_t imm = (imm5 << 5) | imm40;
  imm = (imm << 26) >> 26;

  return (inst_t){
      .imm = imm,
      .rd = RC1(data),
      .rvc = true,
  };
}

static inline inst_t inst_citype_read2(uint16_t data) {
  uint32_t imm86 = (data >> 2) & 0x7;
  uint32_t imm43 = (data >> 5) & 0x3;
  uint32_t imm5 = (data >> 12) & 0x1;

  int32_t imm = (imm86 << 6) | (imm43 << 3) | (imm5 << 5);

  return (inst_t){
      .imm = imm,
      .rd = RC1(data),
      .rvc = true,
  };
}

static inline inst_t inst_citype_read3(uint16_t data) {
  uint32_t imm5 = (data >> 2) & 0x1;
  uint32_t imm87 = (data >> 3) & 0x3;
  uint32_t imm6 = (data >> 5) & 0x1;
  uint32_t imm4 = (data >> 6) & 0x1;
  uint32_t imm9 = (data >> 12) & 0x1;

  int32_t imm =
      (imm5 << 5) | (imm87 << 7) | (imm6 << 6) | (imm4 << 4) | (imm9 << 9);
  imm = (imm << 22) >> 22;

  return (inst_t){
      .imm = imm,
      .rd = RC1(data),
      .rvc = true,
  };
}

static inline inst_t inst_citype_read4(uint16_t data) {
  uint32_t imm5 = (data >> 12) & 0x1;
  uint32_t imm42 = (data >> 4) & 0x7;
  uint32_t imm76 = (data >> 2) & 0x3;

  int32_t imm = (imm5 << 5) | (imm42 << 2) | (imm76 << 6);

  return (inst_t){
      .imm = imm,
      .rd = RC1(data),
      .rvc = true,
  };
}

static inline inst_t inst_citype_read5(uint16_t data) {
  uint32_t imm1612 = (data >> 2) & 0x1f;
  uint32_t imm17 = (data >> 12) & 0x1;

  int32_t imm = (imm1612 << 12) | (imm17 << 17);
  imm = (imm << 14) >> 14;
  return (inst_t){
      .imm = imm,
      .rd = RC1(data),
      .rvc = true,
  };
}

static inline inst_t inst_cbtype_read(uint16_t data) {
  uint32_t imm5 = (data >> 2) & 0x1;
  uint32_t imm21 = (data >> 3) & 0x3;
  uint32_t imm76 = (data >> 5) & 0x3;
  uint32_t imm43 = (data >> 10) & 0x3;
  uint32_t imm8 = (data >> 12) & 0x1;

  int32_t imm =
      (imm8 << 8) | (imm76 << 6) | (imm5 << 5) | (imm43 << 3) | (imm21 << 1);
  imm = (imm << 23) >> 23;

  return (inst_t){
      .imm = imm,
      .rs1 = RP1(data) + 8,
      .rvc = true,
  };
}

static inline inst_t inst_cbtype_read2(uint16_t data) {
  uint32_t imm40 = (data >> 2) & 0x1f;
  uint32_t imm5 = (data >> 12) & 0x1;
  int32_t imm = (imm5 << 5) | imm40;
  imm = (imm << 26) >> 26;

  return (inst_t){
      .imm = imm,
      .rd = RP1(data) + 8,
      .rvc = true,
  };
}

static inline inst_t inst_cstype_read(uint16_t data) {
  uint32_t imm76 = (data >> 5) & 0x3;
  uint32_t imm53 = (data >> 10) & 0x7;

  int32_t imm = ((imm76 << 6) | (imm53 << 3));

  return (inst_t){
      .imm = imm,
      .rs1 = RP1(data) + 8,
      .rs2 = RP2(data) + 8,
      .rvc = true,
  };
}

static inline inst_t inst_cstype_read2(uint16_t data) {
  uint32_t imm6 = (data >> 5) & 0x1;
  uint32_t imm2 = (data >> 6) & 0x1;
  uint32_t imm53 = (data >> 10) & 0x7;

  int32_t imm = ((imm6 << 6) | (imm2 << 2) | (imm53 << 3));

  return (inst_t){
      .imm = imm,
      .rs1 = RP1(data) + 8,
      .rs2 = RP2(data) + 8,
      .rvc = true,
  };
}

static inline inst_t inst_cjtype_read(uint16_t data) {
  uint32_t imm5 = (data >> 2) & 0x1;
  uint32_t imm31 = (data >> 3) & 0x7;
  uint32_t imm7 = (data >> 6) & 0x1;
  uint32_t imm6 = (data >> 7) & 0x1;
  uint32_t imm10 = (data >> 8) & 0x1;
  uint32_t imm98 = (data >> 9) & 0x3;
  uint32_t imm4 = (data >> 11) & 0x1;
  uint32_t imm11 = (data >> 12) & 0x1;

  int32_t imm = ((imm5 << 5) | (imm31 << 1) | (imm7 << 7) | (imm6 << 6) |
                 (imm10 << 10) | (imm98 << 8) | (imm4 << 4) | (imm11 << 11));
  imm = (imm << 20) >> 20;
  return (inst_t){
      .imm = imm,
      .rvc = true,
  };
}

static inline inst_t inst_cltype_read(uint16_t data) {
  uint32_t imm6 = (data >> 5) & 0x1;
  uint32_t imm2 = (data >> 6) & 0x1;
  uint32_t imm53 = (data >> 10) & 0x7;

  int32_t imm = (imm6 << 6) | (imm2 << 2) | (imm53 << 3);

  return (inst_t){
      .imm = imm,
      .rs1 = RP1(data) + 8,
      .rd = RP2(data) + 8,
      .rvc = true,
  };
}

static inline inst_t inst_cltype_read2(uint16_t data) {
  uint32_t imm76 = (data >> 5) & 0x3;
  uint32_t imm53 = (data >> 10) & 0x7;

  int32_t imm = (imm76 << 6) | (imm53 << 3);

  return (inst_t){
      .imm = imm,
      .rs1 = RP1(data) + 8,
      .rd = RP2(data) + 8,
      .rvc = true,
  };
}

static inline inst_t inst_csstype_read(uint16_t data) {
  uint32_t imm86 = (data >> 7) & 0x7;
  uint32_t imm53 = (data >> 10) & 0x7;

  int32_t imm = (imm86 << 6) | (imm53 << 3);

  return (inst_t){
      .imm = imm,
      .rs2 = RC2(data),
      .rvc = true,
  };
}

static inline inst_t inst_csstype_read2(uint16_t data) {
  uint32_t imm76 = (data >> 7) & 0x3;
  uint32_t imm52 = (data >> 9) & 0xf;

  int32_t imm = (imm76 << 6) | (imm52 << 2);

  return (inst_t){
      .imm = imm,
      .rs2 = RC2(data),
      .rvc = true,
  };
}

static inline inst_t inst_ciwtype_read(uint16_t data) {
  uint32_t imm3 = (data >> 5) & 0x1;
  uint32_t imm2 = (data >> 6) & 0x1;
  uint32_t imm96 = (data >> 7) & 0xf;
  uint32_t imm54 = (data >> 11) & 0x3;

  int32_t imm = (imm3 << 3) | (imm2 << 2) | (imm96 << 6) | (imm54 << 4);

  return (inst_t){
      .imm = imm,
      .rd = RP2(data) + 8,
      .rvc = true,
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
  // printf("inst_data: %08x\n", data);
  switch (quadrand) {
  case 0b00: {
    uint32_t copcode = COPCODE(data);

    switch (copcode) {
    case 0b000: { /* C.ADDI4SPN */
                  /* addi rd, sp, uimm */
      *inst = inst_ciwtype_read(data);
      inst->rs1 = sp;
      inst->type = inst_addi;
      Assert(inst->imm != 0, "c.addi4spn uimm=0时非法, %08x", data);
      printf(ANSI_FMT("c.addi4spn to addi\n", ANSI_FG_BLUE));
      return;
    }
    case 0b001: { /* C.FLD */
                  /* fld rd, uimm(x2) */
      *inst = inst_cltype_read2(data);
      inst->type = inst_fld;
      printf(ANSI_FMT("c.fld to fld\n", ANSI_FG_BLUE));
      return;
    }
    case 0b010: { /* C.LW */
      /* lw rd, uimm(rs1) */
      *inst = inst_cltype_read(data);
      inst->type = inst_lw;
      printf(ANSI_FMT("c.lw to lw\n", ANSI_FG_BLUE));
      return;
    }
    case 0b011: { /* C.LD */
      *inst = inst_cltype_read2(data);
      inst->type = inst_ld;
      printf(ANSI_FMT("c.ld to ld\n", ANSI_FG_BLUE));
      return;
    }
    case 0b100: { /* reserverd */
      panic("reserverd");
      return;
    }
    case 0b101: { /* C.FSD */
                  /* fsd rs2, uimm(rs1) */
      *inst = inst_cstype_read(data);
      inst->type = inst_fsd;
      printf(ANSI_FMT("c.fsd to fsd\n", ANSI_FG_BLUE));
      return;
    }
    case 0b110: { /* C.SW */
                  /* sw rs2, uimm(rs1) */
      *inst = inst_cstype_read2(data);
      inst->type = inst_sw;
      printf(ANSI_FMT("c.sw to sw\n", ANSI_FG_BLUE));
      return;
    }
    case 0b111: { /* C.SD */
                  /* sd rs2, uimm(rs1) */
      *inst = inst_cstype_read(data);
      inst->type = inst_sd;
      printf(ANSI_FMT("c.sd to sd\n", ANSI_FG_BLUE));
      return;
    }
    default:
      panic("data: %x\n unimplemented", data);
    }
  }
    unreachable();
  case 0b01: {
    uint32_t copcode = COPCODE(data);

    switch (copcode) {
    case 0b000: { /* C.ADDI */
                  /* addi rd, rd, imm */
      *inst = inst_citype_read(data);
      inst->rs1 = inst->rd;
      inst->type = inst_addi;
      printf(ANSI_FMT("c.addi to addi\n", ANSI_FG_BLUE));
      return;
    }
    case 0b001: { /* C.ADDIW */
                  /* addiw rd, rd, imm */
      *inst = inst_citype_read(data);
      Assert(inst->rd != zero, "c.addiw rd=x0时非法");
      inst->rs1 = inst->rd;
      inst->type = inst_addiw;
      return;
    }
    case 0b010: { /* C.LI */
                  /* addi rd, zero, imm */
      *inst = inst_citype_read(data);
      inst->rs1 = zero;
      inst->type = inst_addi;
      printf(ANSI_FMT("c.li to addi\n", ANSI_FG_BLUE));
      return;
    }
    case 0b011: { /* C.LUI / C.ADDI16SP */
      int32_t rd = RC1(data);
      if (rd == 2) { /* C.ADDI16SP */
                     /* addi x2, x2, imm */
        *inst = inst_citype_read3(data);
        Assert(inst->imm != 0, "c.addi16sp imm = 0时非法");
        inst->type = inst_addi;
        printf(ANSI_FMT("c.addi16sp to addi\n", ANSI_FG_BLUE));
      } else {
        *inst = inst_citype_read5(data);
        Assert(inst->imm != 0, "c.lui rd=x2或imm = 0时非法");
        Assert(inst->rd != inst->rs2, "c.lui rd=x2或imm = 0时非法");
        inst->type = inst_lui;
        printf(ANSI_FMT("c.lui to lui\n", ANSI_FG_BLUE));
      }
      return;
    }
    case 0b100: { /* MISC-ALU */
      uint32_t cfunct2high = CFUNCT2HIGH(data);

      switch (cfunct2high) {
      case 0b00: { /* C.SRLI */
                   /* srli rd, rd, uimm */
        *inst = inst_cbtype_read2(data);
        inst->rs1 = inst->rd;
        inst->type = inst_srli;
        printf(ANSI_FMT("c.srli to srli\n", ANSI_FG_BLUE));
        return;
      }
      case 0b01: { /* C.SRAI */
                   /* srai rd, rd, uimm */
        *inst = inst_cbtype_read2(data);
        inst->rs1 = inst->rd;
        inst->type = inst_srai;
        printf(ANSI_FMT("c.srai to srai\n", ANSI_FG_BLUE));
        return;
      }
      case 0b10: { /* C.ANDI */
                   /* andi rd, rd, uimm */
        *inst = inst_cbtype_read2(data);
        inst->rs1 = inst->rd;
        inst->type = inst_andi;
        printf(ANSI_FMT("c.andi to andi\n", ANSI_FG_BLUE));
        return;
      }
      case 0b11: { /* C.SUB C.XOR C.OR C.AND C.SUBW C.ADDW */
        uint32_t cfunct1 = CFUNCT1(data);

        switch (cfunct1) {
        case 0b0: { /* C.SUB C.XOR C.oR C.AND */
          uint32_t cfunct2low = CFUNCT2LOW(data);

          *inst = inst_catype_read(data);
          inst->rs1 = inst->rd;

          switch (cfunct2low) {
          case 0b00: { /* C.SUB */
                       /* sub rd, rd, rs2 */
            inst->type = inst_sub;
            printf(ANSI_FMT("c.sub to sub\n", ANSI_FG_BLUE));
            return;
          }
          case 0b01: { /* C.XOR */
                       /* xor rd, rd, rs2 */
            inst->type = inst_xor;
            printf(ANSI_FMT("c.xor to xor\n", ANSI_FG_BLUE));
            return;
          }
          case 0b10: { /* C.OR */
                       /* or rd, rd, rs2 */
            inst->type = inst_or;
            printf(ANSI_FMT("c.or to or\n", ANSI_FG_BLUE));
            return;
          }
          case 0b11: { /* C.AND */
                       /* and rd, rd, rs2 */
            inst->type = inst_and;
            printf(ANSI_FMT("c.and to and\n", ANSI_FG_BLUE));
            return;
          }
          default:
            unreachable();
          }
          return;
        }
        case 0b1: { /* C.SUBW C.ADDW */
          uint32_t cfunct2low = CFUNCT2LOW(data);

          *inst = inst_catype_read(data);
          inst->rs1 = inst->rd;

          switch (cfunct2low) {
          case 0b00: { /* C.SUBW */
            inst->type = inst_subw;
            printf(ANSI_FMT("c.subw to subw\n", ANSI_FG_BLUE));
            return;
          }
          case 0b01: { /* C.ADDW */
            inst->type = inst_addw;
            printf(ANSI_FMT("c.addw to addw\n", ANSI_FG_BLUE));
            return;
          }
          default:
            unreachable();
          }
        }
        default:
          unreachable();
        }
        return;
      }
      }
      return;
    }
    case 0b101: { /* C.J */
                  /* jal x0, offset */
      *inst = inst_cjtype_read(data);
      inst->rd = zero;
      inst->type = inst_jal;
      inst->cont = true;
      printf(ANSI_FMT("c.j to jal\n", ANSI_FG_BLUE));
      return;
    }
    case 0b110: { /* C.BEQZ */
      /* beq rs1, x0, offset */
      *inst = inst_cbtype_read(data);
      inst->rs2 = zero;
      inst->type = inst_beq;
      printf(ANSI_FMT("c.beqz to beq\n", ANSI_FG_BLUE));
      return;
    }
    case 0b111: { /* C.BNEZ */
      /* bne rs1, x0, offset */
      *inst = inst_cbtype_read(data);
      inst->rs2 = zero;
      inst->type = inst_bne;
      printf(ANSI_FMT("c.bnez to bne\n", ANSI_FG_BLUE));
      return;
    }
    default:
      panic("unrecognized copcode");
    }
  }
    unreachable();
  case 0b10: {
    uint32_t copcode = COPCODE(data);

    switch (copcode) {
    case 0b000: { /* C.SLLI */
      /* slli rd, rd, uimm */
      *inst = inst_citype_read(data);
      inst->rs1 = inst->rd;
      inst->type = inst_slli;
      printf(ANSI_FMT("c.slli to slli\n", ANSI_FG_BLUE));
      return;
    }
    case 0b001: { /* C.FLDSP */
                  /* fld rd, uimm(x2) */
      *inst = inst_citype_read2(data);
      inst->rs1 = sp;
      inst->type = inst_fld;
      printf(ANSI_FMT("c.fldsp to fld\n", ANSI_FG_BLUE));
      return;
    }
    case 0b010: { /* C.LWSP */
                  /* lw rd, uimm(sp) rd=x0非法 */
      *inst = inst_citype_read4(data);
      Assert(inst->rd != zero, "c.lwsp rd = x0 时非法");
      inst->rs1 = sp;
      inst->type = inst_lw;
      printf(ANSI_FMT("c.lwsp to lw\n", ANSI_FG_BLUE));
      return;
    }
    case 0b011: { /* C.LDSP */
                  /* ld rd, uimm(x2) rd=x0时非法 */
      *inst = inst_citype_read2(data);
      Assert(inst->rd != zero, "c.ldsp rd = x0 时非法");
      inst->rs1 = sp;
      inst->type = inst_ld;
      printf(ANSI_FMT("c.ldsp to ld\n", ANSI_FG_BLUE));
      return;
    }
    case 0b100: { /* C.JR C.MV C.EBREAK C.JALR C.ADD */
      uint32_t cfunct1 = CFUNCT1(data);
      switch (cfunct1) {
      case 0b0: { /* C.JR C.MV */
        *inst = inst_crtype_read(data);
        if (inst->rs2 == zero) { /* C.JR */
          /* jalr x0, 0(rs1) */
          inst->type = inst_jalr;
          printf(ANSI_FMT("c.jr to jalr\n", ANSI_FG_BLUE));
          Assert(inst->rs1 != zero, "c.jr rs1=x0时非法");
          return;
        } else { /* C.MV */
                 /* add rd, x0, rs2 */
          inst->rd = inst->rs1;
          inst->rs1 = zero;
          inst->type = inst_add;
          printf(ANSI_FMT("c.mv to add\n", ANSI_FG_BLUE));
          Assert(inst->rs2 != zero, "c.mv rs2=x0时非法");
          return;
        }
        return;
      }
      case 0b1: { /* C.EBREAK C.JALR C.ADD */
        *inst = inst_crtype_read(data);
        if (inst->rs1 == zero && inst->rs2 == zero) { /* C.EBREAK */
          panic("unimplemented");
        } else if (inst->rs2 == zero) { /* C.JALR */
                                        /* jalr x1, 0(rs1) */
          inst->rd = ra;
          inst->type = inst_jalr;
          inst->cont = true;
          printf(ANSI_FMT("c.jalr to jalr\n", ANSI_FG_BLUE));
          Assert(inst->rd != zero || inst->rs2 == zero, "c.mv rs2=x0时非法");
        } else { /* C.ADD */
                 /* add rd, rd, rs2 rd = x0 或 rs2 = x0 时非法 */
          inst->rd = inst->rs1;
          inst->type = inst_add;
          printf(ANSI_FMT("c.add to add\n", ANSI_FG_BLUE));
          Assert(inst->rd != zero || inst->rs2 == zero, "c.mv rs2=x0时非法");
        }
        return;
      }
      default:
        unreachable();
      }
      unreachable();
    }
      unreachable();
    case 0b101: { /*C.FSDSP */
                  /* fsd rs2, uimm(x2) */
      *inst = inst_csstype_read(data);
      inst->rs1 = sp;
      inst->type = inst_fsd;
      printf(ANSI_FMT("c.fsdsp to fsd\n", ANSI_FG_BLUE));
      return;
    }
      unreachable();
    case 0b110: { /*C.SWSP */
                  /* sw rs2, uimm(x2) */
      *inst = inst_csstype_read2(data);
      inst->rs1 = sp;
      inst->type = inst_sw;
      printf(ANSI_FMT("c.swsp to sw\n", ANSI_FG_BLUE));
      return;
    }
    case 0b111: { /*C.SDSP */
      /* sd rs2, uimm(x2) */
      *inst = inst_csstype_read(data);
      inst->rs1 = sp;
      inst->type = inst_sd;
      printf(ANSI_FMT("c.sdsp to sd\n", ANSI_FG_BLUE));
      return;
    }
    default:
      panic("unrecognized copcode");
    }
  }
    unreachable();
  case 0b11: {
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
      uint32_t funct2 = FUNCT2(data);

      *inst = inst_fprtype_read(data);
      switch (funct2) {
      case 0x0: /* FMADD.S */
        inst->type = inst_fmadd_s;
        return;
      case 0x1: /* FMADD.D */
        inst->type = inst_fmadd_d;
        return;
      default:
        unreachable();
      }
    }
      unreachable();
    case 0x11: {
      uint32_t funct2 = FUNCT2(data);

      *inst = inst_fprtype_read(data);
      switch (funct2) {
      case 0x0: /* FMSUB.S */
        inst->type = inst_fmsub_s;
        return;
      case 0x1: /* FMSUB.D */
        inst->type = inst_fmsub_d;
        return;
      default:
        unreachable();
      }
    }
      unreachable();
    case 0x12: {
      uint32_t funct2 = FUNCT2(data);

      *inst = inst_fprtype_read(data);
      switch (funct2) {
      case 0x0: /* FNMSUB.S */
        inst->type = inst_fnmsub_s;
        return;
      case 0x1: /* FNMSUB.D */
        inst->type = inst_fnmsub_d;
        return;
      default:
        unreachable();
      }
    }
      unreachable();
    case 0x13: {
      uint32_t funct2 = FUNCT2(data);

      *inst = inst_fprtype_read(data);
      switch (funct2) {
      case 0x0: /* FNMADD.S */
        inst->type = inst_fnmadd_s;
        return;
      case 0x1: /* FNMADD.D */
        inst->type = inst_fnmadd_d;
        return;
      default:
        unreachable();
      }
    }
      unreachable();
    case 0x14: {
      uint32_t funct7 = FUNCT7(data);

      *inst = inst_rtype_read(data);
      switch (funct7) {
      case 0x0: /* FADD.S */
        inst->type = inst_fadd_s;
        return;
      case 0x1: /* FADD.D */
        inst->type = inst_fadd_d;
        return;
      case 0x4: /* FSUB.S */
        inst->type = inst_fsub_s;
        return;
      case 0x5: /* FSUB.D */
        inst->type = inst_fsub_d;
        return;
      case 0x8: /* FMUL.S */
        inst->type = inst_fmul_s;
        return;
      case 0x9: /* FMUL.D */
        inst->type = inst_fmul_d;
        return;
      case 0xc: /* FDIV.S */
        inst->type = inst_fdiv_s;
        return;
      case 0xd: /* FDIV.D */
        inst->type = inst_fdiv_d;
        return;
      case 0x10: {
        uint32_t funct3 = FUNCT3(data);

        switch (funct3) {
        case 0x0: /* FSGNJ.S */
          inst->type = inst_fsgnj_s;
          return;
        case 0x1: /* FSGNJN.S */
          inst->type = inst_fsgnjn_s;
          return;
        case 0x2: /* FSGNJX.S */
          inst->type = inst_fsgnjx_s;
          return;
        default:
          unreachable();
        }
      }
        unreachable();
      case 0x11: {
        uint32_t funct3 = FUNCT3(data);

        switch (funct3) {
        case 0x0: /* FSGNJ.D */
          inst->type = inst_fsgnj_d;
          return;
        case 0x1: /* FSGNJN.D */
          inst->type = inst_fsgnjn_d;
          return;
        case 0x2: /* FSGNJX.D */
          inst->type = inst_fsgnjx_d;
          return;
        default:
          unreachable();
        }
      }
        unreachable();
      case 0x14: {
        uint32_t funct3 = FUNCT3(data);

        switch (funct3) {
        case 0x0: /* FMIN.S */
          inst->type = inst_fmin_s;
          return;
        case 0x1: /* FMAX.S */
          inst->type = inst_fmax_s;
          return;
        default:
          unreachable();
        }
      }
        unreachable();
      case 0x15: {
        uint32_t funct3 = FUNCT3(data);

        switch (funct3) {
        case 0x0: /* FMIN.D */
          inst->type = inst_fmin_d;
          return;
        case 0x1: /* FMAX.D */
          inst->type = inst_fmax_d;
          return;
        default:
          unreachable();
        }
      }
        unreachable();
      case 0x20: /* FCVT.S.D */
        Assert(RS2(data) == 1, "fcvt.s.d");
        inst->type = inst_fcvt_s_d;
        return;
      case 0x21: /* FCVT.D.S */
        Assert(RS2(data) == 0, "fcvt.d.s");
        inst->type = inst_fcvt_d_s;
        return;
      case 0x2c: /* FSQRT.S */
        Assert(inst->rs2 == 0, "fsqrt.s");
        inst->type = inst_fsqrt_s;
        return;
      case 0x2d: /* FSQRT.D */
        Assert(inst->rs2 == 0, "fsqrt.d");
        inst->type = inst_fsqrt_d;
        return;
      case 0x50: {
        uint32_t funct3 = FUNCT3(data);

        switch (funct3) {
        case 0x0: /* FLE.S */
          inst->type = inst_fle_s;
          return;
        case 0x1: /* FLT.S */
          inst->type = inst_flt_s;
          return;
        case 0x2: /* FEQ.S */
          inst->type = inst_feq_s;
          return;
        default:
          unreachable();
        }
      }
        unreachable();
      case 0x51: {
        uint32_t funct3 = FUNCT3(data);

        switch (funct3) {
        case 0x0: /* FLE.D */
          inst->type = inst_fle_d;
          return;
        case 0x1: /* FLT.D */
          inst->type = inst_flt_d;
          return;
        case 0x2: /* FEQ.D */
          inst->type = inst_feq_d;
          return;
        default:
          unreachable();
        }
      }
        unreachable();
      case 0x60: {
        uint32_t rs2 = RS2(data);

        switch (rs2) {
        case 0x0: /* FCVT.W.S */
          inst->type = inst_fcvt_w_s;
          return;
        case 0x1: /* FCVT.WU.S */
          inst->type = inst_fcvt_wu_s;
          return;
        case 0x2: /* FCVT.L.S */
          inst->type = inst_fcvt_l_s;
          return;
        case 0x3: /* FCVT.LU.S */
          inst->type = inst_fcvt_lu_s;
          return;
        default:
          unreachable();
        }
      }
        unreachable();
      case 0x61: {
        uint32_t rs2 = RS2(data);

        switch (rs2) {
        case 0x0: /* FCVT.W.D */
          inst->type = inst_fcvt_w_d;
          return;
        case 0x1: /* FCVT.WU.D */
          inst->type = inst_fcvt_wu_d;
          return;
        case 0x2: /* FCVT.L.D */
          inst->type = inst_fcvt_l_d;
          return;
        case 0x3: /* FCVT.LU.D */
          inst->type = inst_fcvt_lu_d;
          return;
        default:
          unreachable();
        }
      }
        unreachable();
      case 0x68: {
        uint32_t rs2 = RS2(data);

        switch (rs2) {
        case 0x0: /* FCVT.S.W */
          inst->type = inst_fcvt_s_w;
          return;
        case 0x1: /* FCVT.S.WU */
          inst->type = inst_fcvt_s_wu;
          return;
        case 0x2: /* FCVT.S.L */
          inst->type = inst_fcvt_s_l;
          return;
        case 0x3: /* FCVT.S.LU */
          inst->type = inst_fcvt_s_lu;
          return;
        default:
          unreachable();
        }
      }
        unreachable();
      case 0x69: {
        uint32_t rs2 = RS2(data);

        switch (rs2) {
        case 0x0: /* FCVT.D.W */
          inst->type = inst_fcvt_d_w;
          return;
        case 0x1: /* FCVT.D.WU */
          inst->type = inst_fcvt_d_wu;
          return;
        case 0x2: /* FCVT.D.L */
          inst->type = inst_fcvt_d_l;
          return;
        case 0x3: /* FCVT.D.LU */
          inst->type = inst_fcvt_d_lu;
          return;
        default:
          unreachable();
        }
      }
        unreachable();
      case 0x70: {
        Assert(RS2(data) == 0, "fmv.x.w/fclass.s");
        uint32_t funct3 = FUNCT3(data);

        switch (funct3) {
        case 0x0: /* FMV.X.W */
          inst->type = inst_fmv_x_w;
          return;
        case 0x1: /* FCLASS.S */
          inst->type = inst_fclass_s;
          return;
        default:
          unreachable();
        }
      }
        unreachable();
      case 0x71: {
        Assert(RS2(data) == 0, "fmv.x.d/fclass.d");
        uint32_t funct3 = FUNCT3(data);

        switch (funct3) {
        case 0x0: /* FMV.X.D */
          inst->type = inst_fmv_x_d;
          return;
        case 0x1: /* FCLASS.D */
          inst->type = inst_fclass_d;
          return;
        default:
          unreachable();
        }
      }
        unreachable();
      case 0x78: /* FMV_W_X */
        Assert(RS2(data) == 0 && FUNCT3(data) == 0, "fmv_w_x");
        inst->type = inst_fmv_w_x;
        return;
      case 0x79: /* FMV_D_X */
        Assert(RS2(data) == 0 && FUNCT3(data) == 0, "fmv_d_x");
        inst->type = inst_fmv_d_x;
        return;
      default:
        unreachable();
      }
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
