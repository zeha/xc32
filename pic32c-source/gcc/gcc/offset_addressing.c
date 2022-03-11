/*
  Optimization pass to reduce the immediate offset value for a sequence of load
  / store instructions that use the [areg, #offset] addressing mode.

  The optimization tries to reduce the immediate offset to be:
  * in range [0..31]  for load / store of 8bit data size
  * in range [0..62]  for load / store of 16bit data size
  * in range [0..124] for load / store of 32bit data size

  When reducing the offset we have to insert a pair of add/sub instructions
  around the sequence of load / store instructions to be optimized to adjust
  the address in the areg register. There are two possible scenarios:
   1. the sequence to be optimized is formed of consecutive instructions that
  have no uses of areg other than [areg, #offset]

      Before:

      ld dreg, [areg, 104]
      ld dreg, [areg, 106]
      . . .
      ld dreg, [areg, 105]

      After:

      add areg,#104
      ld dreg, [areg, 0]
      ld dreg, [areg, 2]
      . . .
      ld dreg, [areg, 1]
      sub areg,#104

      In this case the overhead is _16b_ADD_SUB or _16b_ADD if the areg has no
  other uses after ld dreg, [areg, 1]

  2. the sequence to be optimized is formed of non-consecutive instructions

      Before:

      ld dreg, [areg, 104]
      ld dreg, [areg, 10]
      ld dreg, [areg, 106]
      . . .
      ld dreg, [areg, 105]

      After:

      add new_areg,areg, #104
      ld dreg, [new_areg, 0]
      ld dreg, [areg,    10]
      ld dreg, [new_areg, 2]
      . . .
      ld dreg, [new_areg, 1]
      sub new_areg,areg, #104

      In this case the overhead is _32b_ADD_SUB or _32b_ADD if the new_areg has
  no other uses after ld dreg, [new_areg, 1]

  The current implementation works at basic-block level, but it can be extended
  to work on low-pressure-regions (regions formed by multiple basic blocks were 
  a specific reg is not used)
*/

#ifdef TARGET_MCHP_PIC32C

#include "config.h"
#include "system.h"
#include "coretypes.h"
#include "target.h"
#include "tree.h"
#include "function.h"
#include "basic-block.h"
#include "tree-pass.h"
#include "rtl.h"
#include "memmodel.h"
#include "emit-rtl.h"
#include "bitmap.h"
#include "cfg.h"
#include "cfgrtl.h"
#include "df.h"
#include "predict.h"

#define N_LOW_REGS 7
#define BMAP_STACK_SIZE 5
#if 0
#define dump_file stderr
#endif

extern void df_simulate_find_defs (rtx_insn *, bitmap);
extern void df_simulate_find_uses (rtx_insn *, bitmap);

typedef int32_t offset_t;
typedef int32_t gain_t;
typedef int8_t reg_t;

// [areg, #offset] - reg-imm addressing mode
typedef struct ri_am
{
  // used to link all ri_am (from the same BB) that share the same areg
  int32_t prev_insn_index;
  // used to determine the size of the memory access
  machine_mode mode;
  // address of the imm (offset) operand
  rtx *off_loc;
  // address of the address-reg (areg) operand
  rtx *areg_loc;
  // data reg to be loaded / stored at [areg, #offset] address
  rtx dreg;
  rtx_insn *insn;
} ri_am;

// address-reg operand
typedef struct ri_areg
{
  int32_t tail;
  gain_t possible_gain;
} ri_areg;

// overhead that has to be added / subtracted around a ri_seq
typedef enum overhead
{
  _16b_ADD,
  _16b_ADD_SUB,
  _32b_ADD,
  _32b_ADD_SUB,
  _UNSET_,
} overhead;

// sequence of ri_am instructions that can be optimized (all sharing the same
// areg)
typedef struct ri_seq
{
  reg_t areg;
  reg_t base_reg;
  offset_t base_offset;
  overhead okind;
  gain_t gain;
  int8_t no_gaps;
  bitmap insn_id;
} ri_seq;

#define FOR_EACH_RI_AM(_it_, _regno_tail_)                                    \
  for (int32_t _it_ = _regno_tail_; _it_ != -1;                               \
       _it_ = maccess[_it_].prev_insn_index)

static ri_am *maccess;
static bitmap sbmap[BMAP_STACK_SIZE];
static int8_t bmap_idx;

static inline uint32_t
get_dreg (ri_am ri)
{
  return REGNO (ri.dreg);
}

static inline offset_t
get_offset (ri_am ri)
{
  return (offset_t)INTVAL (*(ri.off_loc));
}

static inline rtx_insn *
emit_set_insn (rtx x, rtx y)
{
  return emit_insn (gen_rtx_SET (x, y));
}

static inline rtx_insn *
first_insn_in_seq (ri_seq const *seq)
{
  unsigned idx = bitmap_first_set_bit (seq->insn_id);
  return maccess[idx].insn;
}

static inline rtx_insn *
last_insn_in_seq (ri_seq const *seq)
{
  unsigned idx = bitmap_last_set_bit (seq->insn_id);
  return maccess[idx].insn;
}

static void
ri_seq_copy (ri_seq *dest, ri_seq const *src)
{
  dest->base_offset = src->base_offset;
  dest->no_gaps = src->no_gaps;
  dest->gain = src->gain;

  bitmap_copy (dest->insn_id, src->insn_id);
}

static void
print_seq (ri_seq const *seq, const int32_t regno_tail)
{
  if (dump_file)
    {
      fprintf (dump_file, "base-offset=%-8d: {", seq->base_offset);

      FOR_EACH_RI_AM (i, regno_tail)
      {
        if (bitmap_bit_p (seq->insn_id, i))
          fprintf (dump_file, "%d, ", get_offset (maccess[i]));
      }

      fprintf (dump_file, "}\n");
      fprintf (dump_file, "gain=%8d\n", seq->gain);
      fprintf (dump_file, "no_gaps=%5d\n\n", seq->no_gaps);
    }
}

// to avoid calling BITMAP_ALLOC & BITMAP_FREE too often, we use a preallocated
// stack of bitmap
//  objects
static inline bitmap
get_bmap ()
{
  gcc_assert ((bmap_idx + 1) < BMAP_STACK_SIZE);
  gcc_assert ((bmap_idx + 1) >= 0);

  bmap_idx += 1;
  bitmap_clear (sbmap[bmap_idx]);

  return sbmap[bmap_idx];
}

// we have to pay special attention to release the bitmap objects in exactly
// the same order as when allocated
static inline void
release_bmap (bitmap bmap)
{
  if (bmap_idx < 0)
    return;

  gcc_assert (sbmap[bmap_idx] == bmap);
  bmap_idx -= 1;
}

// the cost represents the number of bytes needed to encode the offset constant
static uint32_t
ri_am_cost (const offset_t offset, const machine_mode mode)
{
  // 32-bit word
  if (mode == SImode)
    return ((0 <= offset) && (offset <= 124)) ? 1 : 2;
  // 16-bit halfword
  else if (mode == HImode)
    return ((0 <= offset) && (offset <= 62)) ? 1 : 2;
  // 8-bit byte
  else if (mode == QImode)
    return ((0 <= offset) && (offset <= 31)) ? 1 : 2;

  gcc_assert (0 && "Invalid memory access size");
}

// the cost represents the number of bytes needed to encode the instruction(s)
//  that have to be inserted around a ri_seq
static uint32_t
get_overhead_cost (const overhead okind)
{
  switch (okind)
    {
    case _16b_ADD: // one 16b add instruction
      return 1;

    case _16b_ADD_SUB: // two 16b instructions
    case _32b_ADD:     // one 32b add instruction
      return 2;

    case _32b_ADD_SUB: // two 32b instructions
      return 4;

    case _UNSET_:
      return 0xFFFFFFFF;

    default:
      gcc_assert (0 && "Invalid overhead value");
    }
  gcc_assert (0 && "Should not reach this line");
}

// the <reg> register is considered dead after <after> insn if:
//
//  (1) there is a definition of <reg> that follows <after> insn:
//    no uses of <reg> are between <after> insn and this definition of <reg>
//
//  (2) there is no definition of <reg> that follows <after> insn:
//    no uses of <reg> are between <after> and the last insn in current BB
//    the <reg> is not live_out in current BB (has uses outside of this BB)
static bool
is_reg_dead (const reg_t reg, rtx_insn const *after)
{

  rtx_insn *it = NULL;
  rtx_insn *end = NULL;
  rtx_insn *last_def = NULL;
  basic_block bb = BLOCK_FOR_INSN (after);
  bitmap bmap = get_bmap ();
  int reg_used_between;

  df_ref x = df_bb_regno_last_def_find (bb, reg);

  if (x && DF_INSN_LUID (DF_REF_INSN (x)) > DF_INSN_LUID (after))
    last_def = DF_REF_INSN (x);
  end = last_def ? NEXT_INSN (last_def) : NEXT_INSN (BB_END (bb));

  for (it = NEXT_INSN (after); it != end; it = NEXT_INSN (it))
    df_simulate_find_uses (it, bmap);

  reg_used_between = bitmap_bit_p (bmap, reg);
  release_bmap (bmap);

  // reg is used between after and last_def/end => reg is not dead
  if (reg_used_between)
    return 0;

  // reg is not used between after and last_def => reg dead
  if (last_def)
    return 1;

  // reg has no other uses in this bb, but might be used in other incoming bb
  return (!bitmap_bit_p (df_get_live_out (bb), reg));
}

// find the overhead (insn) that will have to be inserted around ri_seq
static overhead
get_overhead (const uint8_t thumb16_insn, const reg_t regno,
              rtx_insn const *last_insn_in_seq)
{
  bool is_dead_after = is_reg_dead (regno, last_insn_in_seq);

  if (thumb16_insn)
    return is_dead_after ? _16b_ADD : _16b_ADD_SUB;

  return is_dead_after ? _32b_ADD : _32b_ADD_SUB;
}

static bool
is_ri_mem_pattern (const_rtx mpat, machine_mode *mode, rtx **off_loc,
                   rtx **areg_loc)
{
  if ((GET_CODE (XEXP (mpat, 0)) == PLUS))
    {
      rtx plus = XEXP (mpat, 0);
      if (REG_P (XEXP (plus, 0)) && CONST_INT_P (XEXP (plus, 1)))
        {
          *off_loc = &(XEXP (plus, 1));
          *areg_loc = &(XEXP (plus, 0));
          *mode = GET_MODE (mpat);
          return true;
        }
    }
  return false;
}

static bool
is_r_pattern (rtx pat, rtx *rloc)
{
  if (REG_P (pat))
    {
      *rloc = pat;
      return true;
    }
  else if (GET_CODE (pat) == UNSPEC)
    {
      uint8_t unalligned_ld_st = false;
      unalligned_ld_st |= (XINT (pat, 1) == UNSPEC_UNALIGNED_LOAD);
      unalligned_ld_st |= (XINT (pat, 1) == UNSPEC_UNALIGNED_STORE);

      if (!unalligned_ld_st)
        return false;

      gcc_assert (XVECLEN (pat, 0) == 1);
      gcc_assert (REG_P (XVECEXP (pat, 0, 0)));
      *rloc = XVECEXP (pat, 0, 0);
      return true;
    }
  return false;
}

//  <seq> is valid if the seq->areg register is not defined between the
//  [first_insn_in_seq..last_insn_in_seq]
uint8_t
seq_is_valid (ri_seq const *seq)
{
  rtx_insn *start = first_insn_in_seq (seq);
  rtx_insn *end = NEXT_INSN (last_insn_in_seq (seq));
  bitmap bmap = get_bmap ();
  int reg_is_used_between;

  for (rtx_insn *insn = start; insn != end; insn = NEXT_INSN (insn))
    df_simulate_find_defs (insn, bmap);

  reg_is_used_between = bitmap_bit_p (bmap, seq->areg);
  release_bmap (bmap);

  return (!reg_is_used_between);
}

//  For the accesses below
//
//    ld dreg, [areg, 102]
//    ld dreg, [areg, 100]
//    ld dreg, [areg, 104]
//    ld dreg, [areg, 105]
//    ld dreg, [areg, 106]
//    ld dreg, [areg, 101]
//
//  find_longest_seq_of_consecutive_accesses will return the sequence [104,
//  105, 106]
//  find_longest_seq will return the sequence [102, 100, 104, 105, 106, 101]
// TODO: this can be extended to find multiple sequences worth being optimized
static void
find_longest_seq (ri_seq *seq, const int32_t regno_tail)
{
  int8_t h_correction_needed = 0;
  int8_t w_correction_needed = 0;
  gain_t gain = 0;
  bitmap bmap = get_bmap ();

#define INIT_NEW_SEQ                                                          \
  h_correction_needed = w_correction_needed = gain = 0;                       \
  bitmap_clear (bmap);

#define CHECK_FOR_NEW_SEQ                                                     \
  if (gain > seq->gain)                                                       \
    {                                                                         \
      bitmap_copy (seq->insn_id, bmap);                                       \
      if (seq_is_valid (seq))                                                 \
        {                                                                     \
          seq->base_offset = get_offset (maccess[i]);                         \
          seq->gain = gain;                                                   \
          if (w_correction_needed)                                            \
            seq->base_offset = seq->base_offset & ~(0x3);                     \
          else if (h_correction_needed)                                       \
            seq->base_offset = seq->base_offset & ~(0x1);                     \
        }                                                                     \
    }

  FOR_EACH_RI_AM (i, regno_tail)
  { 
    INIT_NEW_SEQ
    FOR_EACH_RI_AM (j, regno_tail)
    {
      int32_t new_off = get_offset (maccess[j]) - get_offset (maccess[i]);

      if ((new_off < 0) && seq->no_gaps)
        {
          CHECK_FOR_NEW_SEQ
          INIT_NEW_SEQ
          continue;
        }

      uint32_t old_cost = ri_am_cost (get_offset (maccess[j]), maccess[j].mode);
      uint32_t new_cost = ri_am_cost (new_off, maccess[j].mode);

      if ((old_cost == 2 && new_cost == 1))
        {
          gain += 1;
          bitmap_set_bit (bmap, j);

          w_correction_needed |= ((maccess[j].mode == SImode) && (new_off & 0x3));
          h_correction_needed |= ((maccess[j].mode == HImode) && (new_off & 0x1));
        }
    }
    CHECK_FOR_NEW_SEQ
  }

  release_bmap (bmap);
}

// Find a constant value to be added/substracted from seq.areg in such a way
// that
//  as many as possible of the ri_am memory accesses will use a smaller offset
static void
find_base_offset (ri_seq *seq, const int32_t regno_tail)
{
  ri_seq s1;
  ri_seq s2;

  s1.insn_id = get_bmap ();
  s2.insn_id = get_bmap ();
  s1.no_gaps = 1;
  s2.no_gaps = 0;
  s1.gain = s2.gain = 0;
  s1.areg = s2.areg = seq->areg;

  find_longest_seq (&s1, regno_tail);
  find_longest_seq (&s2, regno_tail);

  print_seq (&s2, regno_tail);
  print_seq (&s1, regno_tail);

  if (s1.gain >= s2.gain)
    ri_seq_copy (seq, &s1);
  else
    ri_seq_copy (seq, &s2);

  release_bmap (s2.insn_id);
  release_bmap (s1.insn_id);
}

// Find a register to be used as new base-register
// First check if the current seq.areg register can be used because this leads
// to a smaller overhead (seq.areg register can be used as new base-reg if all
// insns between [first_insn_in_seq..last_insn_in_seq]
//  are consecutive)
static void
find_base_reg (ri_seq *seq)
{
  rtx_insn *start = first_insn_in_seq (seq);
  rtx_insn *end = NEXT_INSN (last_insn_in_seq (seq));
  bitmap bmap = get_bmap ();

  if (seq->no_gaps)
    {
      df_ref def, use;

      bitmap_clear (bmap);
      for (rtx_insn *insn = start; insn != end; insn = NEXT_INSN (insn))
        {
          FOR_EACH_INSN_DEF (def, insn)
          {
            gcc_assert (DF_REF_TYPE (def) == DF_REF_REG_DEF
                        && "DF_REF_REG_DEF expected");
            bitmap_set_bit (bmap, DF_REF_REGNO (def));
          }

          struct df_insn_info *insn_info = DF_INSN_INFO_GET (insn);

          FOR_EACH_INSN_INFO_DEF (def, insn_info)
          if (DF_REF_FLAGS (def) & (DF_REF_PARTIAL | DF_REF_CONDITIONAL))
            bitmap_set_bit (bmap, DF_REF_REGNO (def));

          FOR_EACH_INSN_INFO_USE (use, insn_info)
          {
            if (DF_REF_TYPE (use) == DF_REF_REG_MEM_LOAD)
              continue;
            if (DF_REF_TYPE (use) == DF_REF_REG_MEM_STORE)
              continue;

            bitmap_set_bit (bmap, DF_REF_REGNO (use));
            gcc_assert (DF_REF_TYPE (use) == DF_REF_REG_USE
                        && "DF_REF_REG_USE expected");
          }
        }

      if (!bitmap_bit_p (bmap, seq->areg))
        {
          seq->base_reg = seq->areg;
          seq->okind = get_overhead (true, seq->areg, last_insn_in_seq (seq));
          release_bmap (bmap);
          return;
        }
    }

  bitmap_clear (bmap);
  for (rtx_insn *insn = start; insn != end; insn = NEXT_INSN (insn))
    {
      df_simulate_find_defs (insn, bmap);
      df_simulate_find_uses (insn, bmap);
    }

  seq->okind = _UNSET_;
  seq->base_reg = -1;
  for (unsigned regno = 0; regno <= N_LOW_REGS; ++regno)
    {
      if (bitmap_bit_p (bmap, regno))
        continue;
      overhead tmp = get_overhead (false, regno, last_insn_in_seq (seq));
      if (get_overhead_cost (tmp) < get_overhead_cost (seq->okind))
        {
          seq->okind = tmp;
          seq->base_reg = regno;
        }
    }

  release_bmap (bmap);
}

void
offset_addressing ()
{
  const int n_insn = get_max_insn_count ();
  basic_block bb;
  ri_areg regs_idx[N_LOW_REGS + 1];
  ri_seq seq;

  maccess = XNEWVEC (struct ri_am, n_insn + 1);
  gcc_assert (maccess);

  seq.insn_id = BITMAP_ALLOC (NULL);
  for (unsigned idx = 0; idx < BMAP_STACK_SIZE; idx++)
    sbmap[idx] = BITMAP_ALLOC (NULL);
  bmap_idx = -1;

  FOR_EACH_BB_FN (bb, cfun)
  {
    int32_t midx = 0;
    rtx_insn *insn;

    for (reg_t regno = 0; regno <= N_LOW_REGS; regno++)
      {
        regs_idx[regno].tail = -1;
        regs_idx[regno].possible_gain = -1;
      }

    FOR_BB_INSNS (bb, insn)
    {
      rtx pat;
      rtx dreg = NULL;
      rtx *areg_loc = NULL;
      rtx *off_loc = NULL;
      machine_mode mode;

      if (!NONDEBUG_INSN_P (insn))
        continue;

      // what about PARALLEL instruction ???

      pat = PATTERN (insn);

      if (GET_CODE (pat) != SET)
        continue;

      if (MEM_P (XEXP (pat, 1)))
        {
          if (!is_ri_mem_pattern (XEXP (pat, 1), &mode, &off_loc, &areg_loc))
            continue;
          if (!is_r_pattern (XEXP (pat, 0), &dreg))
            continue;
        }
      else if (MEM_P (XEXP (pat, 0)))
        {
          if (!is_ri_mem_pattern (XEXP (pat, 0), &mode, &off_loc, &areg_loc))
            continue;
          if (!is_r_pattern (XEXP (pat, 1), &dreg))
            continue;
        }
      else
        continue;

      // [areg, #offset] cannot be encoded on 16b unless areg is [r0..r7]
      if (REGNO (*areg_loc) > 7)
        continue;

      // optimize ".b", ".h", ".w" accesses only
      // !!!
      // what about double-integers       ??
      //
      // what about multiple load/stores  ??
      // !!!
      if (!((mode == QImode) || (mode == HImode) || (mode == SImode)))
        continue;

      gcc_assert (midx < (n_insn + 1));

      maccess[midx].mode = mode;
      maccess[midx].off_loc = off_loc;
      maccess[midx].areg_loc = areg_loc;
      maccess[midx].dreg = dreg;
      maccess[midx].insn = insn;
      maccess[midx].prev_insn_index = regs_idx[REGNO (*areg_loc)].tail;

      if ((ri_am_cost (get_offset (maccess[midx]), maccess[midx].mode) - 1))
        {
          if (get_dreg (maccess[midx]) <= 7)
            regs_idx[REGNO (*areg_loc)].possible_gain += 1;
        }
      regs_idx[REGNO (*areg_loc)].tail = midx;

      midx++;
    }

    for (reg_t regno = 0; regno <= N_LOW_REGS; regno++)
      {
        if (regs_idx[regno].possible_gain <= 0)
          continue;

        if (dump_file)
          {
            fprintf (dump_file,
                     "\n*** [%s][BB=%d] [regno=%d] [possible gain=%d]\n\n",
                     current_function_name (), bb->index, regno,
                     regs_idx[regno].possible_gain);

            FOR_EACH_RI_AM (idx, regs_idx[regno].tail)
            {
              char access_size;
              char star = ' ';

              if (maccess[idx].mode == QImode)
                access_size = 'b';
              else if (maccess[idx].mode == HImode)
                access_size = 'h';
              else if (maccess[idx].mode == SImode)
                access_size = 'w';
              else
                gcc_assert (0);

              if ((ri_am_cost (get_offset (maccess[idx]), maccess[idx].mode)
                   - 1))
                {
                  if (get_dreg (maccess[idx]) <= 7)
                    star = '*';
                  else
                    star = '-';
                }
              fprintf (dump_file, "[.%c] offset=%-4d %c [luid=%d]\n",
                       access_size, get_offset (maccess[idx]), star,
                       INSN_UID (maccess[idx].insn));
            }
            fprintf (dump_file, "\n");
          }

        seq.areg = regno;
        find_base_offset (&seq, regs_idx[regno].tail);
        if (!seq.gain)
          continue;
        find_base_reg (&seq);
        if (seq.base_reg == -1)
          continue;
        seq.gain -= get_overhead_cost (seq.okind);
        if (seq.gain <= 0)
          continue;

        if (dump_file)
          {
            fprintf (dump_file, "Base offset        = %d\n", seq.base_offset);
            fprintf (dump_file, "Base register      = r%d\n", seq.base_reg);
            fprintf (dump_file, "Overhead-cost      = %d\n",
                     get_overhead_cost (seq.okind));
            fprintf (dump_file, "Gain               = %d\n", seq.gain);
            fprintf (dump_file, "Seq                = [luid=%d..luid=%d]\n",
                     INSN_UID (first_insn_in_seq (&seq)),
                     INSN_UID (last_insn_in_seq (&seq)));
            fprintf (dump_file, "\n");
          }

        rtx_insn *insns;

        rtx rtx_old_base_reg = gen_rtx_REG (SImode, regno);
        rtx rtx_new_base_reg = gen_rtx_REG (SImode, seq.base_reg);
        rtx rtx_offset;

        // !!!
        //  what if instead of the add/sub pairs, for those cases where
        //  we can replace all old-base-reg references (and the
        //  old-base-reg reference is loaded thru a load pc relative, so
        //  an addresses known at link time is loaded) we do emit a
        //  relocation with addend as seq.base_offset ???
        //  !!!

        rtx_offset = gen_int_mode (seq.base_offset, SImode);
        start_sequence ();
        emit_set_insn (rtx_new_base_reg,
                       gen_rtx_PLUS (SImode, rtx_old_base_reg, rtx_offset));
        insns = get_insns ();
        end_sequence ();
        emit_insn_before (insns, first_insn_in_seq (&seq));

        // Let's have the sub instruction generated no matter what and
        // leave
        //  the dce alg (that follows after) to eliminate this
        //  instruction ( for those cases where the base-reg has no other
        //  uses after)
        //
        // Another solution is to consider/check the seq.okind value
        rtx_offset = gen_int_mode (-seq.base_offset, SImode);
        start_sequence ();
        emit_set_insn (rtx_new_base_reg,
                       gen_rtx_PLUS (SImode, rtx_old_base_reg, rtx_offset));
        insns = get_insns ();
        end_sequence ();
        emit_insn_after (insns, last_insn_in_seq (&seq));

        FOR_EACH_RI_AM (idx, regs_idx[regno].tail)
        {
          if (!bitmap_bit_p (seq.insn_id, idx))
            continue;
          // if not all accesses have been optimized it might be
          // possible that the
          //  seq.base_offset is larger than the current offset
          if ((get_offset (maccess[idx]) - seq.base_offset) >= 0)
            {
              rtx_offset = gen_int_mode (
                  get_offset (maccess[idx]) - seq.base_offset, SImode);
              *maccess[idx].off_loc = rtx_offset;
              *maccess[idx].areg_loc = rtx_new_base_reg;
            }
        }
      }
  }

  df_finish_pass (true);
  df_scan_alloc (NULL);
  df_scan_blocks ();

  free (maccess);
  for (unsigned idx = 0; idx < BMAP_STACK_SIZE; idx++)
    BITMAP_FREE (sbmap[idx]);
  BITMAP_FREE (seq.insn_id);
}

namespace
{

const pass_data pass_data_opt_base_reg_offset = {
  RTL_PASS,              /* type */
  "opt_base_reg_offset", /* name */
  OPTGROUP_NONE,         /* optinfo_flags */
  TV_FWPROP,             /* tv_id */
  0,                     /* properties_required */
  0,                     /* properties_provided */
  0,                     /* properties_destroyed */
  0,                     /* todo_flags_start */
  TODO_df_finish,        /* todo_flags_finish */
};

class pass_opt_base_reg_offset : public rtl_opt_pass
{
public:
  pass_opt_base_reg_offset (gcc::context *ctxt)
      : rtl_opt_pass (pass_data_opt_base_reg_offset, ctxt)
  {
  }

  /* opt_pass methods: */
  virtual bool
  gate (function *)
  {
    return optimize_imm_offset_addressing
           && optimize_function_for_size_p (cfun);
  }

  virtual unsigned int
  execute (function *)
  {

    df_note_add_problem ();
    df_analyze ();
    offset_addressing ();

    return false;
  }

}; // class pass_opt_base_reg_offset

} // anon namespace

rtl_opt_pass *
make_pass_opt_base_reg_offset (gcc::context *ctxt)
{
  return new pass_opt_base_reg_offset (ctxt);
}

#endif // TARGET_MCHP_PIC32C