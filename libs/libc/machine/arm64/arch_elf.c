/****************************************************************************
 * libs/libc/machine/arm/armv7-a/arch_elf.c
 *
 * Licensed to the Apache Software Foundation (ASF) under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.  The
 * ASF licenses this file to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance with the
 * License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  See the
 * License for the specific language governing permissions and limitations
 * under the License.
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <inttypes.h>
#include <stdlib.h>
#include <errno.h>
#include <debug.h>
#include <limits.h>

#include <nuttx/elf.h>

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: up_checkarch
 *
 * Description:
 *   Given the ELF header in 'hdr', verify that the ELF file is appropriate
 *   for the current, configured architecture.  Every architecture that uses
 *   the ELF loader must provide this function.
 *
 * Input Parameters:
 *   hdr - The ELF header read from the ELF file.
 *
 * Returned Value:
 *   True if the architecture supports this ELF file.
 *
 ****************************************************************************/

bool up_checkarch(const Elf_Ehdr *ehdr)
{
  /* Make sure it's an ARM executable */

  if (ehdr->e_machine != EM_AARCH64)
    {
      berr("ERROR: Not for ARM: e_machine=%04x\n", ehdr->e_machine);
      return false;
    }

  /* Make sure that 32-bit objects are supported */

  if (ehdr->e_ident[EI_CLASS] != ELFCLASS64)
    {
      berr("ERROR: Need 32-bit objects: e_ident[EI_CLASS]=%02x\n",
           ehdr->e_ident[EI_CLASS]);
      return false;
    }

  /* Verify endian-ness */

#ifdef CONFIG_ENDIAN_BIG
  if (ehdr->e_ident[EI_DATA] != ELFDATA2MSB)
#else
  if (ehdr->e_ident[EI_DATA] != ELFDATA2LSB)
#endif
    {
      berr("ERROR: Wrong endian-ness: e_ident[EI_DATA]=%02x\n",
           ehdr->e_ident[EI_DATA]);
      return false;
    }

  /* Make sure the entry point address is properly aligned */

  if ((ehdr->e_entry & 3) != 0)
    {
      berr("ERROR: Entry point is not properly aligned: %08" PRIx32 "\n",
           ehdr->e_entry);
      return false;
    }

  /* TODO:  Check ABI here. */

  return true;
}

/****************************************************************************
 * Name: up_relocate and up_relocateadd
 *
 * Description:
 *   Perform an architecture-specific ELF relocation.  Every architecture
 *   that uses the ELF loader must provide this function.
 *
 * Input Parameters:
 *   rel - The relocation type
 *   sym - The ELF symbol structure containing the fully resolved value.
 *         There are a few relocation types for a few architectures that do
 *         not require symbol information.  For those, this value will be
 *         NULL.  Implementations of these functions must be able to handle
 *         that case.
 *   addr - The address that requires the relocation.
 *
 * Returned Value:
 *   Zero (OK) if the relocation was successful.  Otherwise, a negated errno
 *   value indicating the cause of the relocation failure.
 *
 ****************************************************************************/

int up_relocate(const Elf_Rel *rel, const Elf_Sym *sym, uintptr_t addr)
{

  berr("ERROR: Unsupported relocation up_relocate\n");

  return -EPERM;
}

enum aarch64_insn_imm_type {
	AARCH64_INSN_IMM_ADR,
	AARCH64_INSN_IMM_26,
	AARCH64_INSN_IMM_19,
	AARCH64_INSN_IMM_16,
	AARCH64_INSN_IMM_14,
	AARCH64_INSN_IMM_12,
	AARCH64_INSN_IMM_9,
	AARCH64_INSN_IMM_7,
	AARCH64_INSN_IMM_6,
	AARCH64_INSN_IMM_S,
	AARCH64_INSN_IMM_R,
	AARCH64_INSN_IMM_N,
	AARCH64_INSN_IMM_MAX
};

enum aarch64_reloc_op {
	RELOC_OP_NONE,
	RELOC_OP_ABS,
	RELOC_OP_PREL,
	RELOC_OP_PAGE,
};

#define BIT(nr)            (1ul << (nr))

static int aarch64_get_imm_shift_mask(enum aarch64_insn_imm_type type,
						uint32_t *maskp, int *shiftp)
{
	uint32_t mask;
	int shift;

	switch (type) {
	case AARCH64_INSN_IMM_26:
		mask = BIT(26) - 1;
		shift = 0;
		break;
	case AARCH64_INSN_IMM_19:
		mask = BIT(19) - 1;
		shift = 5;
		break;
	case AARCH64_INSN_IMM_16:
		mask = BIT(16) - 1;
		shift = 5;
		break;
	case AARCH64_INSN_IMM_14:
		mask = BIT(14) - 1;
		shift = 5;
		break;
	case AARCH64_INSN_IMM_12:
		mask = BIT(12) - 1;
		shift = 10;
		break;
	case AARCH64_INSN_IMM_9:
		mask = BIT(9) - 1;
		shift = 12;
		break;
	case AARCH64_INSN_IMM_7:
		mask = BIT(7) - 1;
		shift = 15;
		break;
	case AARCH64_INSN_IMM_6:
	case AARCH64_INSN_IMM_S:
		mask = BIT(6) - 1;
		shift = 10;
		break;
	case AARCH64_INSN_IMM_R:
		mask = BIT(6) - 1;
		shift = 16;
		break;
	case AARCH64_INSN_IMM_N:
		mask = 1;
		shift = 22;
		break;
	default:
		return -EINVAL;
	}

	*maskp = mask;
	*shiftp = shift;

	return 0;
}

#define SZ_2M (0x200000)
#define ADR_IMM_HILOSPLIT	2
#define ADR_IMM_SIZE		SZ_2M
#define ADR_IMM_LOMASK		((1 << ADR_IMM_HILOSPLIT) - 1)
#define ADR_IMM_HIMASK		((ADR_IMM_SIZE >> ADR_IMM_HILOSPLIT) - 1)
#define ADR_IMM_LOSHIFT		29
#define ADR_IMM_HISHIFT		5

uint32_t aarch64_insn_encode_immediate(enum aarch64_insn_imm_type type,
				  uint32_t insn, uint64_t imm)
{
	uint32_t immlo, immhi, mask;
	int shift;

	switch (type) {
	case AARCH64_INSN_IMM_ADR:
		shift = 0;
		immlo = (imm & ADR_IMM_LOMASK) << ADR_IMM_LOSHIFT;
		imm >>= ADR_IMM_HILOSPLIT;
		immhi = (imm & ADR_IMM_HIMASK) << ADR_IMM_HISHIFT;
		imm = immlo | immhi;
		mask = ((ADR_IMM_LOMASK << ADR_IMM_LOSHIFT) |
			(ADR_IMM_HIMASK << ADR_IMM_HISHIFT));
		break;
	default:
		if (aarch64_get_imm_shift_mask(type, &mask, &shift) < 0) {
			berr("%s: unknown immediate encoding %d\n", __func__,
			       type);
			return 0xd4200000;
		}
	}

	/* Update the immediate field. */
	insn &= ~(mask << shift);
	insn |= (imm & mask) << shift;

	return insn;
}

static uint64_t do_reloc(enum aarch64_reloc_op reloc_op, void *place, uint64_t val)
{
	switch (reloc_op) {
	case RELOC_OP_ABS:
		return val;
	case RELOC_OP_PREL:
		return val - (uint64_t)place;
	case RELOC_OP_PAGE:
		return (val & ~0xfff) - ((uint64_t)place & ~0xfff);
	case RELOC_OP_NONE:
		return 0;
	}

	berr("do_reloc: unknown relocation operation %d\n", reloc_op);
	return 0;
}

static int reloc_data(enum aarch64_reloc_op op, void *place, uint64_t val, int len)
{
	int64_t sval = do_reloc(op, place, val);

	/*
	 * The ELF psABI for AArch64 documents the 16-bit and 32-bit place
	 * relative and absolute relocations as having a range of [-2^15, 2^16)
	 * or [-2^31, 2^32), respectively. However, in order to be able to
	 * detect overflows reliably, we have to choose whether we interpret
	 * such quantities as signed or as unsigned, and stick with it.
	 * The way we organize our address space requires a signed
	 * interpretation of 32-bit relative references, so let's use that
	 * for all R_AARCH64_PRELxx relocations. This means our upper
	 * bound for overflow detection should be Sxx_MAX rather than Uxx_MAX.
	 */

	switch (len) {
	case 16:
		*(int16_t *)place = sval;
		switch (op) {
		case RELOC_OP_ABS:
			if (sval < 0 || sval > UINT16_MAX)
				return -ERANGE;
			break;
		case RELOC_OP_PREL:
			if (sval < INT16_MIN || sval > INT16_MAX)
				return -ERANGE;
			break;
		default:
			berr("Invalid 16-bit data relocation (%d)\n", op);
			return 0;
		}
		break;
	case 32:
		*(int32_t *)place = sval;
		switch (op) {
		case RELOC_OP_ABS:
			if (sval < 0 || sval > UINT32_MAX)
				return -ERANGE;
			break;
		case RELOC_OP_PREL:
			if (sval < INT32_MIN || sval > INT32_MAX)
				return -ERANGE;
			break;
		default:
			berr("Invalid 32-bit data relocation (%d)\n", op);
			return 0;
		}
		break;
	case 64:
		*(int64_t *)place = sval;
		break;
	default:
		berr("Invalid length (%d) for data relocation\n", len);
		return 0;
	}
	return 0;
}

enum aarch64_insn_movw_imm_type {
	AARCH64_INSN_IMM_MOVNZ,
	AARCH64_INSN_IMM_MOVKZ,
};

static int reloc_insn_movw(enum aarch64_reloc_op op, uint32_t *place, uint64_t val,
			   int lsb, enum aarch64_insn_movw_imm_type imm_type)
{
	uint64_t imm;
	int64_t sval;
	uint32_t insn = *place;

	sval = do_reloc(op, place, val);
	imm = sval >> lsb;

	if (imm_type == AARCH64_INSN_IMM_MOVNZ) {
		/*
		 * For signed MOVW relocations, we have to manipulate the
		 * instruction encoding depending on whether or not the
		 * immediate is less than zero.
		 */
		insn &= ~(3 << 29);
		if (sval >= 0) {
			/* >=0: Set the instruction to MOVZ (opcode 10b). */
			insn |= 2 << 29;
		} else {
			/*
			 * <0: Set the instruction to MOVN (opcode 00b).
			 *     Since we've masked the opcode already, we
			 *     don't need to do anything other than
			 *     inverting the new immediate field.
			 */
			imm = ~imm;
		}
	}

	/* Update the instruction with the new encoding. */
	insn = aarch64_insn_encode_immediate(AARCH64_INSN_IMM_16, insn, imm);
	*place = insn;

	if (imm > UINT16_MAX)
		return -ERANGE;

	return 0;
}

static int reloc_insn_imm(enum aarch64_reloc_op op, uint32_t *place, uint64_t val,
			  int lsb, int len, enum aarch64_insn_imm_type imm_type)
{
	uint64_t imm, imm_mask;
	int64_t sval;
	uint32_t insn = *place;

	/* Calculate the relocation value. */
	sval = do_reloc(op, place, val);
	sval >>= lsb;

	/* Extract the value bits and shift them to bit 0. */
	imm_mask = (BIT(lsb + len) - 1) >> lsb;
	imm = sval & imm_mask;

	/* Update the instruction's immediate field. */
	insn = aarch64_insn_encode_immediate(imm_type, insn, imm);
	*place = insn;

	/*
	 * Extract the upper value bits (including the sign bit) and
	 * shift them to bit 0.
	 */
	sval = (int64_t)(sval & ~(imm_mask >> 1)) >> (len - 1);

	/*
	 * Overflow has occurred if the upper bits are not all equal to
	 * the sign bit of the value.
	 */
	//if ((uint64_t)(sval + 1) >= 2)
	//	return -ERANGE;

	return 0;
}
#if 1
static int reloc_insn_adrp(uint32_t *place, uint64_t val)
{
	//uint32_t insn;


		return reloc_insn_imm(RELOC_OP_PAGE, place, val, 12, 21,
				      AARCH64_INSN_IMM_ADR);

	/* patch ADRP to ADR if it is in range */
	//if (!reloc_insn_imm(RELOC_OP_PREL, place, val & ~0xfff, 0, 21,
	//		    AARCH64_INSN_IMM_ADR)) {
	//	insn = le32_to_cpu(*place);
	//	insn &= ~BIT(31);
	//} else {
	//	/* out of range for ADR -> emit a veneer */
	//	val = module_emit_veneer_for_adrp(mod, sechdrs, place, val & ~0xfff);
	//	if (!val)
	//		return -ENOEXEC;
	//	insn = aarch64_insn_gen_branch_imm((uint64_t)place, val,
	//					   AARCH64_INSN_BRANCH_NOLINK);
	//}
	//*place = cpu_to_le32(insn);
	return 0;
}
#endif
int up_relocateadd(const Elf_Rela *rel, const Elf_Sym *sym, uintptr_t addr)
{
  unsigned int relotype;
  uint64_t val;
  int ovf = -EINVAL;

  relotype = ELF_R_TYPE(rel->r_info);
  /* Handle the relocation by relocation type */
  val = sym->st_value + rel->r_addend;

  switch (relotype){
  /* Null relocations. */
  case R_AARCH64_NONE:
    ovf = 0;
  	break;
  /* Data relocations. */
  case R_AARCH64_ABS64:
  	ovf = reloc_data(RELOC_OP_ABS, (void *)addr, val, 64);
  	break;
  case R_AARCH64_ABS32:
  	ovf = reloc_data(RELOC_OP_ABS, (void *)addr, val, 32);
  	break;
  case R_AARCH64_ABS16:
  	ovf = reloc_data(RELOC_OP_ABS, (void *)addr, val, 16);
  	break;
  case R_AARCH64_PREL64:
  	ovf = reloc_data(RELOC_OP_PREL, (void *)addr, val, 64);
  	break;
  case R_AARCH64_PREL32:
  	ovf = reloc_data(RELOC_OP_PREL, (void *)addr, val, 32);
  	break;
  case R_AARCH64_PREL16:
  	ovf = reloc_data(RELOC_OP_PREL, (void *)addr, val, 16);
  	break;  
  /* MOVW instruction relocations. */
  case R_AARCH64_MOVW_UABS_G0_NC:
  case R_AARCH64_MOVW_UABS_G0:
  	ovf = reloc_insn_movw(RELOC_OP_ABS, (void *)addr, val, 0,
  			      AARCH64_INSN_IMM_MOVKZ);
  	break;
  case R_AARCH64_MOVW_UABS_G1_NC:
  case R_AARCH64_MOVW_UABS_G1:
  	ovf = reloc_insn_movw(RELOC_OP_ABS, (void *)addr, val, 16,
  			      AARCH64_INSN_IMM_MOVKZ);
  	break;
  case R_AARCH64_MOVW_UABS_G2_NC:
  case R_AARCH64_MOVW_UABS_G2:
  	ovf = reloc_insn_movw(RELOC_OP_ABS, (void *)addr, val, 32,
  			      AARCH64_INSN_IMM_MOVKZ);
  	break;
  case R_AARCH64_MOVW_UABS_G3:
  	ovf = reloc_insn_movw(RELOC_OP_ABS, (void *)addr, val, 48,
  			      AARCH64_INSN_IMM_MOVKZ);
  	break;
  case R_AARCH64_MOVW_SABS_G0:
  	ovf = reloc_insn_movw(RELOC_OP_ABS, (void *)addr, val, 0,
  			      AARCH64_INSN_IMM_MOVNZ);
  	break;
  case R_AARCH64_MOVW_SABS_G1:
  	ovf = reloc_insn_movw(RELOC_OP_ABS, (void *)addr, val, 16,
  			      AARCH64_INSN_IMM_MOVNZ);
  	break;
  case R_AARCH64_MOVW_SABS_G2:
  	ovf = reloc_insn_movw(RELOC_OP_ABS, (void *)addr, val, 32,
  			      AARCH64_INSN_IMM_MOVNZ);
  	break;
  case R_AARCH64_MOVW_PREL_G0_NC:
  	ovf = reloc_insn_movw(RELOC_OP_PREL, (void *)addr, val, 0,
  			      AARCH64_INSN_IMM_MOVKZ);
  	break;
  case R_AARCH64_MOVW_PREL_G0:
  	ovf = reloc_insn_movw(RELOC_OP_PREL, (void *)addr, val, 0,
  			      AARCH64_INSN_IMM_MOVNZ);
  	break;
  case R_AARCH64_MOVW_PREL_G1_NC:
  	ovf = reloc_insn_movw(RELOC_OP_PREL, (void *)addr, val, 16,
  			      AARCH64_INSN_IMM_MOVKZ);
  	break;
  case R_AARCH64_MOVW_PREL_G1:
  	ovf = reloc_insn_movw(RELOC_OP_PREL, (void *)addr, val, 16,
  			      AARCH64_INSN_IMM_MOVNZ);
  	break;
  case R_AARCH64_MOVW_PREL_G2_NC:
  	ovf = reloc_insn_movw(RELOC_OP_PREL, (void *)addr, val, 32,
  			      AARCH64_INSN_IMM_MOVKZ);
  	break;
  case R_AARCH64_MOVW_PREL_G2:
  	ovf = reloc_insn_movw(RELOC_OP_PREL, (void *)addr, val, 32,
  			      AARCH64_INSN_IMM_MOVNZ);
  	break;
  case R_AARCH64_MOVW_PREL_G3:
  	/* We're using the top bits so we can't overflow. */
  	ovf = reloc_insn_movw(RELOC_OP_PREL, (void *)addr, val, 48,
  			      AARCH64_INSN_IMM_MOVNZ);
  	break;  
  /* Immediate instruction relocations. */
  case R_AARCH64_LD_PREL_LO19:
  	ovf = reloc_insn_imm(RELOC_OP_PREL, (void *)addr, val, 2, 19,
  			     AARCH64_INSN_IMM_19);
  	break;
  case R_AARCH64_ADR_PREL_LO21:
  	ovf = reloc_insn_imm(RELOC_OP_PREL, (void *)addr, val, 0, 21,
  			     AARCH64_INSN_IMM_ADR);
  	break;
  case R_AARCH64_ADR_PREL_PG_HI21_NC:
  case R_AARCH64_ADR_PREL_PG_HI21:
  	ovf = reloc_insn_adrp((void *)addr, val);
  	if (ovf && ovf != -ERANGE)
    ovf = 0;
  	return ovf;
  	break;
  case R_AARCH64_ADD_ABS_LO12_NC:
  case R_AARCH64_LDST8_ABS_LO12_NC:
  	ovf = reloc_insn_imm(RELOC_OP_ABS, (void *)addr, val, 0, 12,
  			     AARCH64_INSN_IMM_12);
  	break;
  case R_AARCH64_LDST16_ABS_LO12_NC:
  	ovf = reloc_insn_imm(RELOC_OP_ABS, (void *)addr, val, 1, 11,
  			     AARCH64_INSN_IMM_12);
  	break;
  case R_AARCH64_LDST32_ABS_LO12_NC:
  	ovf = reloc_insn_imm(RELOC_OP_ABS, (void *)addr, val, 2, 10,
  			     AARCH64_INSN_IMM_12);
  	break;
  case R_AARCH64_LDST64_ABS_LO12_NC:
  	ovf = reloc_insn_imm(RELOC_OP_ABS, (void *)addr, val, 3, 9,
  			     AARCH64_INSN_IMM_12);
  	break;
  case R_AARCH64_LDST128_ABS_LO12_NC:
  	ovf = reloc_insn_imm(RELOC_OP_ABS, (void *)addr, val, 4, 8,
  			     AARCH64_INSN_IMM_12);
  	break;
  case R_AARCH64_TSTBR14:
  	ovf = reloc_insn_imm(RELOC_OP_PREL, (void *)addr, val, 2, 14,
  			     AARCH64_INSN_IMM_14);
  	break;
  case R_AARCH64_CONDBR19:
  	ovf = reloc_insn_imm(RELOC_OP_PREL, (void *)addr, val, 2, 19,
  			     AARCH64_INSN_IMM_19);
  	break;
  case R_AARCH64_JUMP26:
  case R_AARCH64_CALL26:
  	ovf = reloc_insn_imm(RELOC_OP_PREL, (void *)addr, val, 2, 26,
  			     AARCH64_INSN_IMM_26);
  	//if (ovf == -ERANGE) {
  	//	val = module_emit_plt_entry(me, sechdrs, addr, &rel[i], sym);
  	//	if (!val)
  	//		return -ENOEXEC;
  	//	ovf = reloc_insn_imm(RELOC_OP_PREL, (void *)addr, val, 2,
  	//			     26, AARCH64_INSN_IMM_26);
  	//}
  	break;
  default:
      berr("ERROR: Unsupported relocation: %" PRId32 "\n",
           ELF_R_TYPE(rel->r_info));
      return -EINVAL;
  }

  return ovf;
}

