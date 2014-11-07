; MIPS DSP ASE Rev2 Revision 0.02 5/19/2006
(define_insn "mips_absq_s_qb"
  [(parallel
    [(set (match_operand:V4QI 0 "register_operand" "=d")
	  (unspec:V4QI [(match_operand:V4QI 1 "reg_or_0_operand" "dY")]
		       UNSPEC_ABSQ_S_QB))
     (set (reg:CCDSP CCDSP_OU_REGNUM)
	  (unspec:CCDSP [(match_dup 1)] UNSPEC_ABSQ_S_QB))])]
  "TARGET_MIPS_DSP32R2 && !TARGET_MIPS16" 
  "absq_s.qb\t%0,%z1"
  [(set_attr "type"	"dspalusat")
   (set_attr "mode"	"SI")])

(define_insn "mips_addu_ph"
  [(parallel
    [(set (match_operand:V2HI 0 "register_operand" "=d")
	  (plus:V2HI (match_operand:V2HI 1 "reg_or_0_operand" "dY")
		     (match_operand:V2HI 2 "reg_or_0_operand" "dY")))
     (set (reg:CCDSP CCDSP_OU_REGNUM)
	  (unspec:CCDSP [(match_dup 1) (match_dup 2)] UNSPEC_ADDU_PH))])]
  "TARGET_MIPS_DSP32R2 && !TARGET_MIPS16"
  "addu.ph\t%0,%z1,%z2"
  [(set_attr "type"	"dspalu")
   (set_attr "mode"	"SI")])

(define_insn "mips_addu_s_ph"
  [(parallel
    [(set (match_operand:V2HI 0 "register_operand" "=d")
	  (unspec:V2HI [(match_operand:V2HI 1 "reg_or_0_operand" "dY")
			(match_operand:V2HI 2 "reg_or_0_operand" "dY")]
		       UNSPEC_ADDU_S_PH))
     (set (reg:CCDSP CCDSP_OU_REGNUM)
	  (unspec:CCDSP [(match_dup 1) (match_dup 2)] UNSPEC_ADDU_S_PH))])]
  "TARGET_MIPS_DSP32R2 && !TARGET_MIPS16"
  "addu_s.ph\t%0,%z1,%z2"
  [(set_attr "type"	"dspalusat")
   (set_attr "mode"	"SI")])

(define_insn "mips_adduh_qb"
  [(set (match_operand:V4QI 0 "register_operand" "=d")
	(unspec:V4QI [(match_operand:V4QI 1 "reg_or_0_operand" "dY")
		      (match_operand:V4QI 2 "reg_or_0_operand" "dY")]
		     UNSPEC_ADDUH_QB))]
  "TARGET_MIPS_DSP32R2 && !TARGET_MIPS16"
  "adduh.qb\t%0,%z1,%z2"
  [(set_attr "type"	"dspalu")
   (set_attr "mode"	"SI")])

(define_insn "mips_adduh_r_qb"
  [(set (match_operand:V4QI 0 "register_operand" "=d")
	(unspec:V4QI [(match_operand:V4QI 1 "reg_or_0_operand" "dY")
		      (match_operand:V4QI 2 "reg_or_0_operand" "dY")]
		     UNSPEC_ADDUH_R_QB))]
  "TARGET_MIPS_DSP32R2 && !TARGET_MIPS16"
  "adduh_r.qb\t%0,%z1,%z2"
  [(set_attr "type"	"dspalu")
   (set_attr "mode"	"SI")])

(define_insn "mips_append"
  [(set (match_operand:SI 0 "register_operand" "=d")
	(unspec:SI [(match_operand:SI 1 "register_operand" "0")
		    (match_operand:SI 2 "reg_or_0_operand" "dJ")
		    (match_operand:SI 3 "immediate_operand" "i")]
		   UNSPEC_APPEND))]
  "TARGET_MIPS_DSP32R2 && !TARGET_MIPS16"
  "append\t%0,%z2,%3"
  [(set_attr "type"	"dspalu")
   (set_attr "mode"	"SI")])

(define_insn "mips_balign"
  [(set (match_operand:SI 0 "register_operand" "=d")
	(unspec:SI [(match_operand:SI 1 "register_operand" "0")
		    (match_operand:SI 2 "reg_or_0_operand" "dJ")
		    (match_operand:SI 3 "immediate_operand" "i")]
		   UNSPEC_BALIGN))]
  "TARGET_MIPS_DSP32R2 && !TARGET_MIPS16"
  "balign\t%0,%z2,%3"
  [(set_attr "type"	"dspalu")
   (set_attr "mode"	"SI")])

(define_insn "mips_cmpgdu_eq_qb"
  [(parallel
    [(set (match_operand:SI 0 "register_operand" "=d")
	  (unspec:SI [(match_operand:V4QI 1 "reg_or_0_operand" "dY")
		      (match_operand:V4QI 2 "reg_or_0_operand" "dY")]
		     UNSPEC_CMPGDU_EQ_QB))
     (set (reg:CCDSP CCDSP_CC_REGNUM)
	  (unspec:CCDSP [(match_dup 1) (match_dup 2)
			 (reg:CCDSP CCDSP_CC_REGNUM)]
			UNSPEC_CMPGDU_EQ_QB))])]
  "TARGET_MIPS_DSP32R2 && !TARGET_MIPS16"
  "cmpgdu.eq.qb\t%0,%z1,%z2"
  [(set_attr "type"	"dspalu")
   (set_attr "mode"	"SI")])

(define_insn "mips_cmpgdu_lt_qb"
  [(parallel
    [(set (match_operand:SI 0 "register_operand" "=d")
	  (unspec:SI [(match_operand:V4QI 1 "reg_or_0_operand" "dY")
		      (match_operand:V4QI 2 "reg_or_0_operand" "dY")]
		     UNSPEC_CMPGDU_LT_QB))
     (set (reg:CCDSP CCDSP_CC_REGNUM)
	  (unspec:CCDSP [(match_dup 1) (match_dup 2)
			 (reg:CCDSP CCDSP_CC_REGNUM)]
			UNSPEC_CMPGDU_LT_QB))])]
  "TARGET_MIPS_DSP32R2 && !TARGET_MIPS16"
  "cmpgdu.lt.qb\t%0,%z1,%z2"
  [(set_attr "type"	"dspalu")
   (set_attr "mode"	"SI")])

(define_insn "mips_cmpgdu_le_qb"
  [(parallel
    [(set (match_operand:SI 0 "register_operand" "=d")
	  (unspec:SI [(match_operand:V4QI 1 "reg_or_0_operand" "dY")
		      (match_operand:V4QI 2 "reg_or_0_operand" "dY")]
		     UNSPEC_CMPGDU_LE_QB))
     (set (reg:CCDSP CCDSP_CC_REGNUM)
	  (unspec:CCDSP [(match_dup 1) (match_dup 2)
			 (reg:CCDSP CCDSP_CC_REGNUM)]
			UNSPEC_CMPGDU_LE_QB))])]
  "TARGET_MIPS_DSP32R2 && !TARGET_MIPS16"
  "cmpgdu.le.qb\t%0,%z1,%z2"
  [(set_attr "type"	"dspalu")
   (set_attr "mode"	"SI")])

(define_insn "mips_dpa_w_ph"
  [(set (match_operand:DI 0 "register_operand" "=a")
	(unspec:DI [(match_operand:DI 1 "register_operand" "0")
		    (match_operand:V2HI 2 "reg_or_0_operand" "dY")
		    (match_operand:V2HI 3 "reg_or_0_operand" "dY")]
		   UNSPEC_DPA_W_PH))]
  "TARGET_MIPS_DSP32R2 && !TARGET_64BIT && !TARGET_MIPS16"
  "dpa.w.ph\t%q0,%z2,%z3"
  [(set_attr "type"	"dspmac")
   (set_attr "mode"	"SI")])

(define_insn "mips_dpa_w_ph_64"
  [(set (match_operand:TI 0 "register_operand" "=a")
	(unspec:TI [(match_operand:TI 1 "register_operand" "0")
		    (match_operand:V2HI 2 "reg_or_0_operand" "dY")
		    (match_operand:V2HI 3 "reg_or_0_operand" "dY")]
		   UNSPEC_DPA_W_PH_64))]
  "TARGET_MIPS_DSP64R2 && !TARGET_MIPS16"
  "dpa.w.ph\t%q0,%z2,%z3"
  [(set_attr "type"	"dspmac")
   (set_attr "mode"	"DI")])

(define_insn "mips_dps_w_ph"
  [(set (match_operand:DI 0 "register_operand" "=a")
	(unspec:DI [(match_operand:DI 1 "register_operand" "0")
		    (match_operand:V2HI 2 "reg_or_0_operand" "dY")
		    (match_operand:V2HI 3 "reg_or_0_operand" "dY")]
		   UNSPEC_DPS_W_PH))]
  "TARGET_MIPS_DSP32R2 && !TARGET_64BIT && !TARGET_MIPS16"
  "dps.w.ph\t%q0,%z2,%z3"
  [(set_attr "type"	"dspmac")
   (set_attr "mode"	"SI")])

(define_insn "mips_dps_w_ph_64"
  [(set (match_operand:TI 0 "register_operand" "=a")
	(unspec:TI [(match_operand:TI 1 "register_operand" "0")
		    (match_operand:V2HI 2 "reg_or_0_operand" "dY")
		    (match_operand:V2HI 3 "reg_or_0_operand" "dY")]
		   UNSPEC_DPS_W_PH_64))]
  "TARGET_MIPS_DSP64R2 && !TARGET_MIPS16"
  "dps.w.ph\t%q0,%z2,%z3"
  [(set_attr "type"	"dspmac")
   (set_attr "mode"	"DI")])

(define_insn "mips_madd"
  [(set (match_operand:DI 0 "register_operand" "=a")
	(plus:DI
	 (mult:DI (sign_extend:DI
		   (match_operand:SI 2 "reg_or_0_operand" "dJ"))
		  (sign_extend:DI
		   (match_operand:SI 3 "reg_or_0_operand" "dJ")))
	 (match_operand:DI 1 "register_operand" "0")))]
  "TARGET_MIPS_DSP32R2 && !TARGET_64BIT && !TARGET_MIPS16"
  "madd\t%q0,%z2,%z3"
  [(set_attr "type"	"dspmac")
   (set_attr "mode"	"SI")])

(define_insn "mips_maddu"
  [(set (match_operand:DI 0 "register_operand" "=a")
	(plus:DI
	 (mult:DI (zero_extend:DI
		   (match_operand:SI 2 "reg_or_0_operand" "dJ"))
		  (zero_extend:DI
		   (match_operand:SI 3 "reg_or_0_operand" "dJ")))
	 (match_operand:DI 1 "register_operand" "0")))]
  "TARGET_MIPS_DSP32R2 && !TARGET_64BIT && !TARGET_MIPS16"
  "maddu\t%q0,%z2,%z3"
  [(set_attr "type"	"dspmac")
   (set_attr "mode"	"SI")])

(define_insn "mips_msub"
  [(set (match_operand:DI 0 "register_operand" "=a")
	(minus:DI
	 (match_operand:DI 1 "register_operand" "0")
	 (mult:DI (sign_extend:DI
		   (match_operand:SI 2 "reg_or_0_operand" "dJ"))
		  (sign_extend:DI
		   (match_operand:SI 3 "reg_or_0_operand" "dJ")))))]
  "TARGET_MIPS_DSP32R2 && !TARGET_64BIT && !TARGET_MIPS16"
  "msub\t%q0,%z2,%z3"
  [(set_attr "type"	"dspmac")
   (set_attr "mode"	"SI")])

(define_insn "mips_msubu"
  [(set (match_operand:DI 0 "register_operand" "=a")
	(minus:DI
	 (match_operand:DI 1 "register_operand" "0")
	 (mult:DI (zero_extend:DI
		   (match_operand:SI 2 "reg_or_0_operand" "dJ"))
		  (zero_extend:DI
		   (match_operand:SI 3 "reg_or_0_operand" "dJ")))))]
  "TARGET_MIPS_DSP32R2 && !TARGET_64BIT && !TARGET_MIPS16"
  "msubu\t%q0,%z2,%z3"
  [(set_attr "type"	"dspmac")
   (set_attr "mode"	"SI")])

(define_insn "mulv2hi3"
  [(parallel
    [(set (match_operand:V2HI 0 "register_operand" "=d")
	  (mult:V2HI (match_operand:V2HI 1 "reg_or_0_operand" "dY")
		     (match_operand:V2HI 2 "reg_or_0_operand" "dY")))
     (set (reg:CCDSP CCDSP_OU_REGNUM)
	  (unspec:CCDSP [(match_dup 1) (match_dup 2)] UNSPEC_MUL_PH))
     (clobber (match_scratch:DI 3 "=x"))])]
  "TARGET_MIPS_DSP32R2 && !TARGET_MIPS16"
  "mul.ph\t%0,%z1,%z2"
  [(set_attr "type"	"imul3")
   (set_attr "mode"	"SI")])

(define_insn "mips_mul_s_ph"
  [(parallel
    [(set (match_operand:V2HI 0 "register_operand" "=d")
	  (unspec:V2HI [(match_operand:V2HI 1 "reg_or_0_operand" "dY")
			(match_operand:V2HI 2 "reg_or_0_operand" "dY")]
		       UNSPEC_MUL_S_PH))
     (set (reg:CCDSP CCDSP_OU_REGNUM)
	  (unspec:CCDSP [(match_dup 1) (match_dup 2)] UNSPEC_MUL_S_PH))
     (clobber (match_scratch:DI 3 "=x"))])]
  "TARGET_MIPS_DSP32R2 && !TARGET_MIPS16"
  "mul_s.ph\t%0,%z1,%z2"
  [(set_attr "type"	"imul3")
   (set_attr "mode"	"SI")])

(define_insn "mips_mulq_rs_w"
  [(parallel
    [(set (match_operand:SI 0 "register_operand" "=d")
	  (unspec:SI [(match_operand:SI 1 "reg_or_0_operand" "dJ")
		      (match_operand:SI 2 "reg_or_0_operand" "dJ")]
		     UNSPEC_MULQ_RS_W))
     (set (reg:CCDSP CCDSP_OU_REGNUM)
	  (unspec:CCDSP [(match_dup 1) (match_dup 2)] UNSPEC_MULQ_RS_W))
     (clobber (match_scratch:DI 3 "=x"))])]
  "TARGET_MIPS_DSP32R2 && !TARGET_MIPS16"
  "mulq_rs.w\t%0,%z1,%z2"
  [(set_attr "type"	"imul3")
   (set_attr "mode"	"SI")])

(define_insn "mips_mulq_s_ph"
  [(parallel
    [(set (match_operand:V2HI 0 "register_operand" "=d")
	  (unspec:V2HI [(match_operand:V2HI 1 "reg_or_0_operand" "dY")
			(match_operand:V2HI 2 "reg_or_0_operand" "dY")]
		       UNSPEC_MULQ_S_PH))
     (set (reg:CCDSP CCDSP_OU_REGNUM)
	  (unspec:CCDSP [(match_dup 1) (match_dup 2)] UNSPEC_MULQ_S_PH))
     (clobber (match_scratch:DI 3 "=x"))])]
  "TARGET_MIPS_DSP32R2 && !TARGET_MIPS16"
  "mulq_s.ph\t%0,%z1,%z2"
  [(set_attr "type"	"imul3")
   (set_attr "mode"	"SI")])

(define_insn "mips_mulq_s_w"
  [(parallel
    [(set (match_operand:SI 0 "register_operand" "=d")
	  (unspec:SI [(match_operand:SI 1 "reg_or_0_operand" "dJ")
		      (match_operand:SI 2 "reg_or_0_operand" "dJ")]
		     UNSPEC_MULQ_S_W))
     (set (reg:CCDSP CCDSP_OU_REGNUM)
	  (unspec:CCDSP [(match_dup 1) (match_dup 2)] UNSPEC_MULQ_S_W))
     (clobber (match_scratch:DI 3 "=x"))])]
  "TARGET_MIPS_DSP32R2 && !TARGET_MIPS16"
  "mulq_s.w\t%0,%z1,%z2"
  [(set_attr "type"	"imul3")
   (set_attr "mode"	"SI")])

(define_insn "mips_mulsa_w_ph"
  [(set (match_operand:DI 0 "register_operand" "=a")
	(unspec:DI [(match_operand:DI 1 "register_operand" "0")
		    (match_operand:V2HI 2 "reg_or_0_operand" "dY")
		    (match_operand:V2HI 3 "reg_or_0_operand" "dY")]
		   UNSPEC_MULSA_W_PH))]
  "TARGET_MIPS_DSP32R2 && !TARGET_64BIT && !TARGET_MIPS16"
  "mulsa.w.ph\t%q0,%z2,%z3"
  [(set_attr "type"	"dspmac")
   (set_attr "mode"	"SI")])

(define_insn "mips_mulsa_w_ph_64"
  [(set (match_operand:TI 0 "register_operand" "=a")
	(unspec:TI [(match_operand:TI 1 "register_operand" "0")
		    (match_operand:V2HI 2 "reg_or_0_operand" "dY")
		    (match_operand:V2HI 3 "reg_or_0_operand" "dY")]
		   UNSPEC_MULSA_W_PH_64))]
  "TARGET_MIPS_DSP64R2 && !TARGET_MIPS16"
  "mulsa.w.ph\t%q0,%z2,%z3"
  [(set_attr "type"	"dspmac")
   (set_attr "mode"	"DI")])

(define_insn "mips_mult"
  [(set (match_operand:DI 0 "register_operand" "=a")
	(mult:DI
	 (sign_extend:DI (match_operand:SI 1 "reg_or_0_operand" "dJ"))
	 (sign_extend:DI (match_operand:SI 2 "reg_or_0_operand" "dJ"))))]
  "TARGET_MIPS_DSP32R2 && !TARGET_64BIT && !TARGET_MIPS16"
  "mult\t%q0,%z1,%z2"
  [(set_attr "type"	"dspmac")
   (set_attr "mode"	"SI")])

(define_insn "mips_multu"
  [(set (match_operand:DI 0 "register_operand" "=a")
	(mult:DI
	 (zero_extend:DI (match_operand:SI 1 "reg_or_0_operand" "dJ"))
	 (zero_extend:DI (match_operand:SI 2 "reg_or_0_operand" "dJ"))))]
  "TARGET_MIPS_DSP32R2 && !TARGET_64BIT && !TARGET_MIPS16"
  "multu\t%q0,%z1,%z2"
  [(set_attr "type"	"dspmac")
   (set_attr "mode"	"SI")])

(define_insn "mips_precr_qb_ph"
  [(set (match_operand:V4QI 0 "register_operand" "=d")
	(unspec:V4QI [(match_operand:V2HI 1 "reg_or_0_operand" "dY")
		      (match_operand:V2HI 2 "reg_or_0_operand" "dY")]
		     UNSPEC_PRECR_QB_PH))]
  "TARGET_MIPS_DSP32R2 && !TARGET_MIPS16"
  "precr.qb.ph\t%0,%z1,%z2"
  [(set_attr "type"	"dspalu")
   (set_attr "mode"	"SI")])

(define_insn "mips_precr_sra_ph_w"
  [(set (match_operand:V2HI 0 "register_operand" "=d")
	(unspec:V2HI [(match_operand:SI 1 "register_operand" "0")
		      (match_operand:SI 2 "reg_or_0_operand" "dJ")
		      (match_operand:SI 3 "immediate_operand" "i")]
		     UNSPEC_PRECR_SRA_PH_W))]
  "TARGET_MIPS_DSP32R2 && !TARGET_MIPS16"
  "precr_sra.ph.w\t%0,%z2,%3"
  [(set_attr "type"	"dspalu")
   (set_attr "mode"	"SI")])

(define_insn "mips_precr_sra_r_ph_w"
  [(set (match_operand:V2HI 0 "register_operand" "=d")
	(unspec:V2HI [(match_operand:SI 1 "register_operand" "0")
		      (match_operand:SI 2 "reg_or_0_operand" "dJ")
		      (match_operand:SI 3 "immediate_operand" "i")]
		     UNSPEC_PRECR_SRA_R_PH_W))]
  "TARGET_MIPS_DSP32R2 && !TARGET_MIPS16"
  "precr_sra_r.ph.w\t%0,%z2,%3"
  [(set_attr "type"	"dspalu")
   (set_attr "mode"	"SI")])

(define_insn "mips_prepend"
  [(set (match_operand:SI 0 "register_operand" "=d")
	(unspec:SI [(match_operand:SI 1 "register_operand" "0")
		    (match_operand:SI 2 "reg_or_0_operand" "dJ")
		    (match_operand:SI 3 "immediate_operand" "i")]
		   UNSPEC_PREPEND))]
  "TARGET_MIPS_DSP32R2 && !TARGET_MIPS16"
  "prepend\t%0,%z2,%3"
  [(set_attr "type"	"dspalu")
   (set_attr "mode"	"SI")])

(define_insn "mips_shra_qb"
  [(set (match_operand:V4QI 0 "register_operand" "=d,d")
	(unspec:V4QI [(match_operand:V4QI 1 "reg_or_0_operand" "dY,dY")
		      (match_operand:SI 2 "arith_operand" "I,d")]
		     UNSPEC_SHRA_QB))]
  "TARGET_MIPS_DSP32R2 && !TARGET_MIPS16"
  "@
   shra.qb\t%0,%z1,%2
   shrav.qb\t%0,%z1,%2"
  [(set_attr "type"	"dspalu")
   (set_attr "mode"	"SI")])

(define_insn "mips_shra_r_qb"
  [(set (match_operand:V4QI 0 "register_operand" "=d,d")
	(unspec:V4QI [(match_operand:V4QI 1 "reg_or_0_operand" "dY,dY")
		      (match_operand:SI 2 "arith_operand" "I,d")]
		     UNSPEC_SHRA_R_QB))]
  "TARGET_MIPS_DSP32R2 && !TARGET_MIPS16"
  "@
   shra_r.qb\t%0,%z1,%2
   shrav_r.qb\t%0,%z1,%2"
  [(set_attr "type"	"dspalu")
   (set_attr "mode"	"SI")])

(define_insn "mips_shrl_ph"
  [(set (match_operand:V2HI 0 "register_operand" "=d,d")
	(unspec:V2HI [(match_operand:V2HI 1 "reg_or_0_operand" "dY,dY")
		      (match_operand:SI 2 "arith_operand" "I,d")]
		     UNSPEC_SHRL_PH))]
  "TARGET_MIPS_DSP32R2 && !TARGET_MIPS16"
  "@
   shrl.ph\t%0,%z1,%2
   shrlv.ph\t%0,%z1,%2"
  [(set_attr "type"	"dspalu")
   (set_attr "mode"	"SI")])

(define_insn "mips_subu_ph"
  [(parallel
    [(set (match_operand:V2HI 0 "register_operand" "=d")
	  (unspec:V2HI [(match_operand:V2HI 1 "reg_or_0_operand" "dY")
			(match_operand:V2HI 2 "reg_or_0_operand" "dY")]
		       UNSPEC_SUBU_PH))
     (set (reg:CCDSP CCDSP_OU_REGNUM)
	  (unspec:CCDSP [(match_dup 1) (match_dup 2)] UNSPEC_SUBU_PH))])]
  "TARGET_MIPS_DSP32R2 && !TARGET_MIPS16"
  "subu.ph\t%0,%z1,%z2"
  [(set_attr "type"	"dspalu")
   (set_attr "mode"	"SI")])

(define_insn "mips_subu_s_ph"
  [(parallel
    [(set (match_operand:V2HI 0 "register_operand" "=d")
	  (unspec:V2HI [(match_operand:V2HI 1 "reg_or_0_operand" "dY")
			(match_operand:V2HI 2 "reg_or_0_operand" "dY")]
		       UNSPEC_SUBU_S_PH))
     (set (reg:CCDSP CCDSP_OU_REGNUM)
	  (unspec:CCDSP [(match_dup 1) (match_dup 2)] UNSPEC_SUBU_S_PH))])]
  "TARGET_MIPS_DSP32R2 && !TARGET_MIPS16"
  "subu_s.ph\t%0,%z1,%z2"
  [(set_attr "type"	"dspalusat")
   (set_attr "mode"	"SI")])

(define_insn "mips_subuh_qb"
  [(set (match_operand:V4QI 0 "register_operand" "=d")
	(unspec:V4QI [(match_operand:V4QI 1 "reg_or_0_operand" "dY")
		      (match_operand:V4QI 2 "reg_or_0_operand" "dY")]
		     UNSPEC_SUBUH_QB))]
  "TARGET_MIPS_DSP32R2 && !TARGET_MIPS16"
  "subuh.qb\t%0,%z1,%z2"
  [(set_attr "type"	"dspalu")
   (set_attr "mode"	"SI")])

(define_insn "mips_subuh_r_qb"
  [(set (match_operand:V4QI 0 "register_operand" "=d")
	(unspec:V4QI [(match_operand:V4QI 1 "reg_or_0_operand" "dY")
		      (match_operand:V4QI 2 "reg_or_0_operand" "dY")]
		     UNSPEC_SUBUH_R_QB))]
  "TARGET_MIPS_DSP32R2 && !TARGET_MIPS16"
  "subuh_r.qb\t%0,%z1,%z2"
  [(set_attr "type"	"dspalu")
   (set_attr "mode"	"SI")])

(define_insn "mips_addqh_ph"
  [(set (match_operand:V2HI 0 "register_operand" "=d")
	(unspec:V2HI [(match_operand:V2HI 1 "reg_or_0_operand" "dY")
		      (match_operand:V2HI 2 "reg_or_0_operand" "dY")]
		     UNSPEC_ADDQH_PH))]
  "TARGET_MIPS_DSP32R2 && !TARGET_MIPS16"
  "addqh.ph\t%0,%z1,%z2"
  [(set_attr "type"	"dspalu")
   (set_attr "mode"	"SI")])

(define_insn "mips_addqh_r_ph"
  [(set (match_operand:V2HI 0 "register_operand" "=d")
	(unspec:V2HI [(match_operand:V2HI 1 "reg_or_0_operand" "dY")
		      (match_operand:V2HI 2 "reg_or_0_operand" "dY")]
		     UNSPEC_ADDQH_R_PH))]
  "TARGET_MIPS_DSP32R2 && !TARGET_MIPS16"
  "addqh_r.ph\t%0,%z1,%z2"
  [(set_attr "type"	"dspalu")
   (set_attr "mode"	"SI")])

(define_insn "mips_addqh_w"
  [(set (match_operand:SI 0 "register_operand" "=d")
	(unspec:SI [(match_operand:SI 1 "reg_or_0_operand" "dJ")
		    (match_operand:SI 2 "reg_or_0_operand" "dJ")]
		   UNSPEC_ADDQH_W))]
  "TARGET_MIPS_DSP32R2 && !TARGET_MIPS16"
  "addqh.w\t%0,%z1,%z2"
  [(set_attr "type"	"dspalu")
   (set_attr "mode"	"SI")])

(define_insn "mips_addqh_r_w"
  [(set (match_operand:SI 0 "register_operand" "=d")
	(unspec:SI [(match_operand:SI 1 "reg_or_0_operand" "dJ")
		    (match_operand:SI 2 "reg_or_0_operand" "dJ")]
		   UNSPEC_ADDQH_R_W))]
  "TARGET_MIPS_DSP32R2 && !TARGET_MIPS16"
  "addqh_r.w\t%0,%z1,%z2"
  [(set_attr "type"	"dspalu")
   (set_attr "mode"	"SI")])

(define_insn "mips_subqh_ph"
  [(set (match_operand:V2HI 0 "register_operand" "=d")
	(unspec:V2HI [(match_operand:V2HI 1 "reg_or_0_operand" "dY")
		      (match_operand:V2HI 2 "reg_or_0_operand" "dY")]
		     UNSPEC_SUBQH_PH))]
  "TARGET_MIPS_DSP32R2 && !TARGET_MIPS16"
  "subqh.ph\t%0,%z1,%z2"
  [(set_attr "type"	"dspalu")
   (set_attr "mode"	"SI")])

(define_insn "mips_subqh_r_ph"
  [(set (match_operand:V2HI 0 "register_operand" "=d")
	(unspec:V2HI [(match_operand:V2HI 1 "reg_or_0_operand" "dY")
		      (match_operand:V2HI 2 "reg_or_0_operand" "dY")]
		     UNSPEC_SUBQH_R_PH))]
  "TARGET_MIPS_DSP32R2 && !TARGET_MIPS16"
  "subqh_r.ph\t%0,%z1,%z2"
  [(set_attr "type"	"dspalu")
   (set_attr "mode"	"SI")])

(define_insn "mips_subqh_w"
  [(set (match_operand:SI 0 "register_operand" "=d")
	(unspec:SI [(match_operand:SI 1 "reg_or_0_operand" "dJ")
		    (match_operand:SI 2 "reg_or_0_operand" "dJ")]
		   UNSPEC_SUBQH_W))]
  "TARGET_MIPS_DSP32R2 && !TARGET_MIPS16"
  "subqh.w\t%0,%z1,%z2"
  [(set_attr "type"	"dspalu")
   (set_attr "mode"	"SI")])

(define_insn "mips_subqh_r_w"
  [(set (match_operand:SI 0 "register_operand" "=d")
	(unspec:SI [(match_operand:SI 1 "reg_or_0_operand" "dJ")
		    (match_operand:SI 2 "reg_or_0_operand" "dJ")]
		   UNSPEC_SUBQH_R_W))]
  "TARGET_MIPS_DSP32R2 && !TARGET_MIPS16"
  "subqh_r.w\t%0,%z1,%z2"
  [(set_attr "type"	"dspalu")
   (set_attr "mode"	"SI")])

(define_insn "mips_dpax_w_ph"
  [(set (match_operand:DI 0 "register_operand" "=a")
	(unspec:DI [(match_operand:DI 1 "register_operand" "0")
		    (match_operand:V2HI 2 "reg_or_0_operand" "dY")
		    (match_operand:V2HI 3 "reg_or_0_operand" "dY")]
		   UNSPEC_DPAX_W_PH))]
  "TARGET_MIPS_DSP32R2 && !TARGET_64BIT && !TARGET_MIPS16"
  "dpax.w.ph\t%q0,%z2,%z3"
  [(set_attr "type"	"dspmac")
   (set_attr "mode"	"SI")])

(define_insn "mips_dpsx_w_ph"
  [(set (match_operand:DI 0 "register_operand" "=a")
	(unspec:DI [(match_operand:DI 1 "register_operand" "0")
		    (match_operand:V2HI 2 "reg_or_0_operand" "dY")
		    (match_operand:V2HI 3 "reg_or_0_operand" "dY")]
		   UNSPEC_DPSX_W_PH))]
  "TARGET_MIPS_DSP32R2 && !TARGET_64BIT && !TARGET_MIPS16"
  "dpsx.w.ph\t%q0,%z2,%z3"
  [(set_attr "type"	"dspmac")
   (set_attr "mode"	"SI")])

(define_insn "mips_dpaqx_s_w_ph"
  [(parallel
    [(set (match_operand:DI 0 "register_operand" "=a")
	  (unspec:DI [(match_operand:DI 1 "register_operand" "0")
		      (match_operand:V2HI 2 "reg_or_0_operand" "dY")
		      (match_operand:V2HI 3 "reg_or_0_operand" "dY")]
		     UNSPEC_DPAQX_S_W_PH))
     (set (reg:CCDSP CCDSP_OU_REGNUM)
	  (unspec:CCDSP [(match_dup 1) (match_dup 2) (match_dup 3)]
			UNSPEC_DPAQX_S_W_PH))])]
  "TARGET_MIPS_DSP32R2 && !TARGET_64BIT && !TARGET_MIPS16"
  "dpaqx_s.w.ph\t%q0,%z2,%z3"
  [(set_attr "type"	"dspmac")
   (set_attr "mode"	"SI")])

(define_insn "mips_dpaqx_sa_w_ph"
  [(parallel
    [(set (match_operand:DI 0 "register_operand" "=a")
	  (unspec:DI [(match_operand:DI 1 "register_operand" "0")
		      (match_operand:V2HI 2 "reg_or_0_operand" "dY")
		      (match_operand:V2HI 3 "reg_or_0_operand" "dY")]
		     UNSPEC_DPAQX_SA_W_PH))
     (set (reg:CCDSP CCDSP_OU_REGNUM)
	  (unspec:CCDSP [(match_dup 1) (match_dup 2) (match_dup 3)]
			UNSPEC_DPAQX_SA_W_PH))])]
  "TARGET_MIPS_DSP32R2 && !TARGET_64BIT && !TARGET_MIPS16"
  "dpaqx_sa.w.ph\t%q0,%z2,%z3"
  [(set_attr "type"	"dspmacsat")
   (set_attr "mode"	"SI")])

(define_insn "mips_dpsqx_s_w_ph"
  [(parallel
    [(set (match_operand:DI 0 "register_operand" "=a")
	  (unspec:DI [(match_operand:DI 1 "register_operand" "0")
		      (match_operand:V2HI 2 "reg_or_0_operand" "dY")
		      (match_operand:V2HI 3 "reg_or_0_operand" "dY")]
		     UNSPEC_DPSQX_S_W_PH))
     (set (reg:CCDSP CCDSP_OU_REGNUM)
	  (unspec:CCDSP [(match_dup 1) (match_dup 2) (match_dup 3)]
			UNSPEC_DPSQX_S_W_PH))])]
  "TARGET_MIPS_DSP32R2 && !TARGET_64BIT && !TARGET_MIPS16"
  "dpsqx_s.w.ph\t%q0,%z2,%z3"
  [(set_attr "type"	"dspmac")
   (set_attr "mode"	"SI")])

(define_insn "mips_dpsqx_sa_w_ph"
  [(parallel
    [(set (match_operand:DI 0 "register_operand" "=a")
	  (unspec:DI [(match_operand:DI 1 "register_operand" "0")
		      (match_operand:V2HI 2 "reg_or_0_operand" "dY")
		      (match_operand:V2HI 3 "reg_or_0_operand" "dY")]
		     UNSPEC_DPSQX_SA_W_PH))
     (set (reg:CCDSP CCDSP_OU_REGNUM)
	  (unspec:CCDSP [(match_dup 1) (match_dup 2) (match_dup 3)]
			UNSPEC_DPSQX_SA_W_PH))])]
  "TARGET_MIPS_DSP32R2 && !TARGET_64BIT && !TARGET_MIPS16"
  "dpsqx_sa.w.ph\t%q0,%z2,%z3"
  [(set_attr "type"	"dspmacsat")
   (set_attr "mode"	"SI")])

(define_insn "mips_dpax_w_ph_64"
  [(set (match_operand:TI 0 "register_operand" "=a")
	(unspec:TI [(match_operand:TI 1 "register_operand" "0")
		    (match_operand:V2HI 2 "reg_or_0_operand" "dY")
		    (match_operand:V2HI 3 "reg_or_0_operand" "dY")]
		   UNSPEC_DPAX_W_PH_64))]
  "TARGET_MIPS_DSP64R2 && !TARGET_MIPS16"
  "dpax.w.ph\t%q0,%z2,%z3"
  [(set_attr "type"	"dspmac")
   (set_attr "mode"	"DI")])

(define_insn "mips_dpsx_w_ph_64"
  [(set (match_operand:TI 0 "register_operand" "=a")
	(unspec:TI [(match_operand:TI 1 "register_operand" "0")
		    (match_operand:V2HI 2 "reg_or_0_operand" "dY")
		    (match_operand:V2HI 3 "reg_or_0_operand" "dY")]
		   UNSPEC_DPSX_W_PH_64))]
  "TARGET_MIPS_DSP64R2 && !TARGET_MIPS16"
  "dpsx.w.ph\t%q0,%z2,%z3"
  [(set_attr "type"	"dspmac")
   (set_attr "mode"	"DI")])

(define_insn "mips_dpaqx_s_w_ph_64"
  [(parallel
    [(set (match_operand:TI 0 "register_operand" "=a")
	  (unspec:TI [(match_operand:TI 1 "register_operand" "0")
		      (match_operand:V2HI 2 "reg_or_0_operand" "dY")
		      (match_operand:V2HI 3 "reg_or_0_operand" "dY")]
		     UNSPEC_DPAQX_S_W_PH_64))
     (set (reg:CCDSP CCDSP_OU_REGNUM)
	  (unspec:CCDSP [(match_dup 1) (match_dup 2) (match_dup 3)]
			UNSPEC_DPAQX_S_W_PH_64))])]
  "TARGET_MIPS_DSP64R2 && !TARGET_MIPS16"
  "dpaqx_s.w.ph\t%q0,%z2,%z3"
  [(set_attr "type"	"dspmac")
   (set_attr "mode"	"DI")])

(define_insn "mips_dpaqx_sa_w_ph_64"
  [(parallel
    [(set (match_operand:TI 0 "register_operand" "=a")
	  (unspec:TI [(match_operand:TI 1 "register_operand" "0")
		      (match_operand:V2HI 2 "reg_or_0_operand" "dY")
		      (match_operand:V2HI 3 "reg_or_0_operand" "dY")]
		     UNSPEC_DPAQX_SA_W_PH_64))
     (set (reg:CCDSP CCDSP_OU_REGNUM)
	  (unspec:CCDSP [(match_dup 1) (match_dup 2) (match_dup 3)]
			UNSPEC_DPAQX_SA_W_PH_64))])]
  "TARGET_MIPS_DSP64R2 && !TARGET_MIPS16"
  "dpaqx_sa.w.ph\t%q0,%z2,%z3"
  [(set_attr "type"	"dspmacsat")
   (set_attr "mode"	"DI")])

(define_insn "mips_dpsqx_s_w_ph_64"
  [(parallel
    [(set (match_operand:TI 0 "register_operand" "=a")
	  (unspec:TI [(match_operand:TI 1 "register_operand" "0")
		      (match_operand:V2HI 2 "reg_or_0_operand" "dY")
		      (match_operand:V2HI 3 "reg_or_0_operand" "dY")]
		     UNSPEC_DPSQX_S_W_PH_64))
     (set (reg:CCDSP CCDSP_OU_REGNUM)
	  (unspec:CCDSP [(match_dup 1) (match_dup 2) (match_dup 3)]
			UNSPEC_DPSQX_S_W_PH_64))])]
  "TARGET_MIPS_DSP64R2 && !TARGET_MIPS16"
  "dpsqx_s.w.ph\t%q0,%z2,%z3"
  [(set_attr "type"	"dspmac")
   (set_attr "mode"	"DI")])

(define_insn "mips_dpsqx_sa_w_ph_64"
  [(parallel
    [(set (match_operand:TI 0 "register_operand" "=a")
	  (unspec:TI [(match_operand:TI 1 "register_operand" "0")
		      (match_operand:V2HI 2 "reg_or_0_operand" "dY")
		      (match_operand:V2HI 3 "reg_or_0_operand" "dY")]
		     UNSPEC_DPSQX_SA_W_PH_64))
     (set (reg:CCDSP CCDSP_OU_REGNUM)
	  (unspec:CCDSP [(match_dup 1) (match_dup 2) (match_dup 3)]
			UNSPEC_DPSQX_SA_W_PH_64))])]
  "TARGET_MIPS_DSP64R2 && !TARGET_MIPS16"
  "dpsqx_sa.w.ph\t%q0,%z2,%z3"
  [(set_attr "type"	"dspmacsat")
   (set_attr "mode"	"DI")])
