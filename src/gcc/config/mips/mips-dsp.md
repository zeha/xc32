(define_constants
  [(CCDSP_PO_REGNUM	182)
   (CCDSP_SC_REGNUM	183)
   (CCDSP_CA_REGNUM	184)
   (CCDSP_OU_REGNUM	185)
   (CCDSP_CC_REGNUM	186)
   (CCDSP_EF_REGNUM	187)])

; MIPS DSP ASE Revision 0.98 3/24/2005
; Table 2-1. MIPS DSP ASE Instructions: Arithmetic
; ADDQ*
(define_insn "addv2hi3"
  [(parallel
    [(set (match_operand:V2HI 0 "register_operand" "=d")
	  (plus:V2HI (match_operand:V2HI 1 "reg_or_0_operand" "dY")
		     (match_operand:V2HI 2 "reg_or_0_operand" "dY")))
     (set (reg:CCDSP CCDSP_OU_REGNUM)
	  (unspec:CCDSP [(match_dup 1) (match_dup 2)] UNSPEC_ADDQ_PH))])]
  "TARGET_MIPS_DSP32 && !TARGET_MIPS16"
  "addq.ph\t%0,%z1,%z2"
  [(set_attr "type"	"dspalu")
   (set_attr "mode"	"SI")])

(define_insn "mips_addq_s_ph"
  [(parallel
    [(set (match_operand:V2HI 0 "register_operand" "=d")
	  (unspec:V2HI [(match_operand:V2HI 1 "reg_or_0_operand" "dY")
			(match_operand:V2HI 2 "reg_or_0_operand" "dY")]
		       UNSPEC_ADDQ_S_PH))
     (set (reg:CCDSP CCDSP_OU_REGNUM)
	  (unspec:CCDSP [(match_dup 1) (match_dup 2)] UNSPEC_ADDQ_S_PH))])]
  "TARGET_MIPS_DSP32 && !TARGET_MIPS16"
  "addq_s.ph\t%0,%z1,%z2"
  [(set_attr "type"	"dspalusat")
   (set_attr "mode"	"SI")])

(define_insn "addv4hi3"
  [(parallel
    [(set (match_operand:V4HI 0 "register_operand" "=d")
	  (plus:V4HI (match_operand:V4HI 1 "reg_or_0_operand" "dY")
		     (match_operand:V4HI 2 "reg_or_0_operand" "dY")))
     (set (reg:CCDSP CCDSP_OU_REGNUM)
	  (unspec:CCDSP [(match_dup 1) (match_dup 2)] UNSPEC_ADDQ_QH))])]
  "TARGET_MIPS_DSP64 && !TARGET_MIPS16"
  "addq.qh\t%0,%z1,%z2"
  [(set_attr "type"	"dspalu")
   (set_attr "mode"	"DI")])

(define_insn "mips_addq_s_qh"
  [(parallel
    [(set (match_operand:V4HI 0 "register_operand" "=d")
	  (unspec:V4HI [(match_operand:V4HI 1 "reg_or_0_operand" "dY")
			(match_operand:V4HI 2 "reg_or_0_operand" "dY")]
		       UNSPEC_ADDQ_S_QH))
     (set (reg:CCDSP CCDSP_OU_REGNUM)
	  (unspec:CCDSP [(match_dup 1) (match_dup 2)] UNSPEC_ADDQ_S_QH))])]
  "TARGET_MIPS_DSP64 && !TARGET_MIPS16"
  "addq_s.qh\t%0,%z1,%z2"
  [(set_attr "type"	"dspalusat")
   (set_attr "mode"	"DI")])

(define_insn "mips_addq_s_w"
  [(parallel
    [(set (match_operand:SI 0 "register_operand" "=d")
	  (unspec:SI [(match_operand:SI 1 "reg_or_0_operand" "dJ")
		      (match_operand:SI 2 "reg_or_0_operand" "dJ")]
		     UNSPEC_ADDQ_S_W))
     (set (reg:CCDSP CCDSP_OU_REGNUM)
	  (unspec:CCDSP [(match_dup 1) (match_dup 2)] UNSPEC_ADDQ_S_W))])]
  "TARGET_MIPS_DSP32 && !TARGET_MIPS16"
  "addq_s.w\t%0,%z1,%z2"
  [(set_attr "type"	"dspalusat")
   (set_attr "mode"	"SI")])

(define_insn "addv2si3"
  [(parallel
    [(set (match_operand:V2SI 0 "register_operand" "=d")
	  (plus:V2SI (match_operand:V2SI 1 "reg_or_0_operand" "dY")
		     (match_operand:V2SI 2 "reg_or_0_operand" "dY")))
     (set (reg:CCDSP CCDSP_OU_REGNUM)
	  (unspec:CCDSP [(match_dup 1) (match_dup 2)] UNSPEC_ADDQ_PW))])]
  "TARGET_MIPS_DSP64 && !TARGET_MIPS16"
  "addq.pw\t%0,%z1,%z2"
  [(set_attr "type"	"dspalu")
   (set_attr "mode"	"DI")])

(define_insn "mips_addq_s_pw"
  [(parallel
    [(set (match_operand:V2SI 0 "register_operand" "=d")
	  (unspec:V2SI [(match_operand:V2SI 1 "reg_or_0_operand" "dY")
			(match_operand:V2SI 2 "reg_or_0_operand" "dY")]
		       UNSPEC_ADDQ_S_PW))
     (set (reg:CCDSP CCDSP_OU_REGNUM)
	  (unspec:CCDSP [(match_dup 1) (match_dup 2)] UNSPEC_ADDQ_S_PW))])]
  "TARGET_MIPS_DSP64 && !TARGET_MIPS16"
  "addq_s.pw\t%0,%z1,%z2"
  [(set_attr "type"	"dspalusat")
   (set_attr "mode"	"DI")])

; ADDU*
(define_insn "addv4qi3"
  [(parallel
    [(set (match_operand:V4QI 0 "register_operand" "=d")
	  (plus:V4QI (match_operand:V4QI 1 "reg_or_0_operand" "dY")
		     (match_operand:V4QI 2 "reg_or_0_operand" "dY")))
     (set (reg:CCDSP CCDSP_OU_REGNUM)
	  (unspec:CCDSP [(match_dup 1) (match_dup 2)] UNSPEC_ADDU_QB))])]
  "TARGET_MIPS_DSP32 && !TARGET_MIPS16"
  "addu.qb\t%0,%z1,%z2"
  [(set_attr "type"	"dspalu")
   (set_attr "mode"	"SI")])

(define_insn "mips_addu_s_qb"
  [(parallel
    [(set (match_operand:V4QI 0 "register_operand" "=d")
	  (unspec:V4QI [(match_operand:V4QI 1 "reg_or_0_operand" "dY")
			(match_operand:V4QI 2 "reg_or_0_operand" "dY")]
		       UNSPEC_ADDU_S_QB))
     (set (reg:CCDSP CCDSP_OU_REGNUM)
	  (unspec:CCDSP [(match_dup 1) (match_dup 2)] UNSPEC_ADDU_S_QB))])]
  "TARGET_MIPS_DSP32 && !TARGET_MIPS16"
  "addu_s.qb\t%0,%z1,%z2"
  [(set_attr "type"	"dspalusat")
   (set_attr "mode"	"SI")])

(define_insn "addv8qi3"
  [(parallel
    [(set (match_operand:V8QI 0 "register_operand" "=d")
	  (plus:V8QI (match_operand:V8QI 1 "reg_or_0_operand" "dY")
		     (match_operand:V8QI 2 "reg_or_0_operand" "dY")))
     (set (reg:CCDSP CCDSP_OU_REGNUM)
	  (unspec:CCDSP [(match_dup 1) (match_dup 2)] UNSPEC_ADDU_OB))])]
  "TARGET_MIPS_DSP64 && !TARGET_MIPS16"
  "addu.ob\t%0,%z1,%z2"
  [(set_attr "type"	"dspalu")
   (set_attr "mode"	"DI")])

(define_insn "mips_addu_s_ob"
  [(parallel
    [(set (match_operand:V8QI 0 "register_operand" "=d")
	  (unspec:V8QI [(match_operand:V8QI 1 "reg_or_0_operand" "dY")
			(match_operand:V8QI 2 "reg_or_0_operand" "dY")]
		       UNSPEC_ADDU_S_OB))
     (set (reg:CCDSP CCDSP_OU_REGNUM)
	  (unspec:CCDSP [(match_dup 1) (match_dup 2)] UNSPEC_ADDU_S_OB))])]
  "TARGET_MIPS_DSP64 && !TARGET_MIPS16"
  "addu_s.ob\t%0,%z1,%z2"
  [(set_attr "type"	"dspalusat")
   (set_attr "mode"	"DI")])

; SUBQ*
(define_insn "subv2hi3"
  [(parallel
    [(set (match_operand:V2HI 0 "register_operand" "=d")
	  (minus:V2HI (match_operand:V2HI 1 "reg_or_0_operand" "dY")
		      (match_operand:V2HI 2 "reg_or_0_operand" "dY")))
     (set (reg:CCDSP CCDSP_OU_REGNUM)
	  (unspec:CCDSP [(match_dup 1) (match_dup 2)] UNSPEC_SUBQ_PH))])]
  "TARGET_MIPS_DSP32 && !TARGET_MIPS16"
  "subq.ph\t%0,%z1,%z2"
  [(set_attr "type"	"dspalu")
   (set_attr "mode"	"SI")])

(define_insn "mips_subq_s_ph"
  [(parallel
    [(set (match_operand:V2HI 0 "register_operand" "=d")
	  (unspec:V2HI [(match_operand:V2HI 1 "reg_or_0_operand" "dY")
			(match_operand:V2HI 2 "reg_or_0_operand" "dY")]
		       UNSPEC_SUBQ_S_PH))
     (set (reg:CCDSP CCDSP_OU_REGNUM)
	  (unspec:CCDSP [(match_dup 1) (match_dup 2)] UNSPEC_SUBQ_S_PH))])]
  "TARGET_MIPS_DSP32 && !TARGET_MIPS16"
  "subq_s.ph\t%0,%z1,%z2"
  [(set_attr "type"	"dspalusat")
   (set_attr "mode"	"SI")])

(define_insn "subv4hi3"
  [(parallel
    [(set (match_operand:V4HI 0 "register_operand" "=d")
	  (minus:V4HI (match_operand:V4HI 1 "reg_or_0_operand" "dY")
		      (match_operand:V4HI 2 "reg_or_0_operand" "dY")))
     (set (reg:CCDSP CCDSP_OU_REGNUM)
	  (unspec:CCDSP [(match_dup 1) (match_dup 2)] UNSPEC_SUBQ_QH))])]
  "TARGET_MIPS_DSP64 && !TARGET_MIPS16"
  "subq.qh\t%0,%z1,%z2"
  [(set_attr "type"	"dspalu")
   (set_attr "mode"	"DI")])

(define_insn "mips_subq_s_qh"
  [(parallel
    [(set (match_operand:V4HI 0 "register_operand" "=d")
	  (unspec:V4HI [(match_operand:V4HI 1 "reg_or_0_operand" "dY")
			(match_operand:V4HI 2 "reg_or_0_operand" "dY")]
		       UNSPEC_SUBQ_S_QH))
     (set (reg:CCDSP CCDSP_OU_REGNUM)
	  (unspec:CCDSP [(match_dup 1) (match_dup 2)] UNSPEC_SUBQ_S_QH))])]
  "TARGET_MIPS_DSP64 && !TARGET_MIPS16"
  "subq_s.qh\t%0,%z1,%z2"
  [(set_attr "type"	"dspalusat")
   (set_attr "mode"	"DI")])

(define_insn "mips_subq_s_w"
  [(parallel
    [(set (match_operand:SI 0 "register_operand" "=d")
	  (unspec:SI [(match_operand:SI 1 "reg_or_0_operand" "dJ")
		      (match_operand:SI 2 "reg_or_0_operand" "dJ")]
		     UNSPEC_SUBQ_S_W))
     (set (reg:CCDSP CCDSP_OU_REGNUM)
	  (unspec:CCDSP [(match_dup 1) (match_dup 2)] UNSPEC_SUBQ_S_W))])]
  "TARGET_MIPS_DSP32 && !TARGET_MIPS16"
  "subq_s.w\t%0,%z1,%z2"
  [(set_attr "type"	"dspalusat")
   (set_attr "mode"	"SI")])

(define_insn "subv2si3"
  [(parallel
    [(set (match_operand:V2SI 0 "register_operand" "=d")
	  (minus:V2SI (match_operand:V2SI 1 "reg_or_0_operand" "dY")
		      (match_operand:V2SI 2 "reg_or_0_operand" "dY")))
     (set (reg:CCDSP CCDSP_OU_REGNUM)
	  (unspec:CCDSP [(match_dup 1) (match_dup 2)] UNSPEC_SUBQ_PW))])]
  "TARGET_MIPS_DSP64 && !TARGET_MIPS16"
  "subq.pw\t%0,%z1,%z2"
  [(set_attr "type"	"dspalu")
   (set_attr "mode"	"DI")])

(define_insn "mips_subq_s_pw"
  [(parallel
    [(set (match_operand:V2SI 0 "register_operand" "=d")
	  (unspec:V2SI [(match_operand:V2SI 1 "reg_or_0_operand" "dY")
			(match_operand:V2SI 2 "reg_or_0_operand" "dY")]
		       UNSPEC_SUBQ_S_PW))
     (set (reg:CCDSP CCDSP_OU_REGNUM)
	  (unspec:CCDSP [(match_dup 1) (match_dup 2)] UNSPEC_SUBQ_S_PW))])]
  "TARGET_MIPS_DSP64 && !TARGET_MIPS16"
  "subq_s.pw\t%0,%z1,%z2"
  [(set_attr "type"	"dspalusat")
   (set_attr "mode"	"DI")])

; SUBU* 
(define_insn "subv4qi3"
  [(parallel
    [(set (match_operand:V4QI 0 "register_operand" "=d")
	  (minus:V4QI (match_operand:V4QI 1 "reg_or_0_operand" "dY")
		      (match_operand:V4QI 2 "reg_or_0_operand" "dY")))
     (set (reg:CCDSP CCDSP_OU_REGNUM)
	  (unspec:CCDSP [(match_dup 1) (match_dup 2)] UNSPEC_SUBU_QB))])]
  "TARGET_MIPS_DSP32 && !TARGET_MIPS16"
  "subu.qb\t%0,%z1,%z2"
  [(set_attr "type"	"dspalu")
   (set_attr "mode"	"SI")])

(define_insn "mips_subu_s_qb"
  [(parallel
    [(set (match_operand:V4QI 0 "register_operand" "=d")
	  (unspec:V4QI [(match_operand:V4QI 1 "reg_or_0_operand" "dY")
			(match_operand:V4QI 2 "reg_or_0_operand" "dY")]
		       UNSPEC_SUBU_S_QB))
     (set (reg:CCDSP CCDSP_OU_REGNUM)
	  (unspec:CCDSP [(match_dup 1) (match_dup 2)] UNSPEC_SUBU_S_QB))])]
  "TARGET_MIPS_DSP32 && !TARGET_MIPS16"
  "subu_s.qb\t%0,%z1,%z2"
  [(set_attr "type"	"dspalusat")
   (set_attr "mode"	"SI")])

(define_insn "subv8qi3"
  [(parallel
    [(set (match_operand:V8QI 0 "register_operand" "=d")
	  (minus:V8QI (match_operand:V8QI 1 "reg_or_0_operand" "dY")
		      (match_operand:V8QI 2 "reg_or_0_operand" "dY")))
     (set (reg:CCDSP CCDSP_OU_REGNUM)
	  (unspec:CCDSP [(match_dup 1) (match_dup 2)] UNSPEC_SUBU_OB))])]
  "TARGET_MIPS_DSP64 && !TARGET_MIPS16"
  "subu.ob\t%0,%z1,%z2"
  [(set_attr "type"	"dspalu")
   (set_attr "mode"	"DI")])

(define_insn "mips_subu_s_ob"
  [(parallel
    [(set (match_operand:V8QI 0 "register_operand" "=d")
	  (unspec:V8QI [(match_operand:V8QI 1 "reg_or_0_operand" "dY")
			(match_operand:V8QI 2 "reg_or_0_operand" "dY")]
		       UNSPEC_SUBU_S_OB))
     (set (reg:CCDSP CCDSP_OU_REGNUM)
	  (unspec:CCDSP [(match_dup 1) (match_dup 2)] UNSPEC_SUBU_S_OB))])]
  "TARGET_MIPS_DSP64 && !TARGET_MIPS16"
  "subu_s.ob\t%0,%z1,%z2"
  [(set_attr "type"	"dspalusat")
   (set_attr "mode"	"DI")])

; ADDSC
(define_insn "mips_addsc"
  [(parallel
    [(set (match_operand:SI 0 "register_operand" "=d")
	  (unspec:SI [(match_operand:SI 1 "reg_or_0_operand" "dJ")
		      (match_operand:SI 2 "reg_or_0_operand" "dJ")]
		     UNSPEC_ADDSC))
     (set (reg:CCDSP CCDSP_CA_REGNUM)
	  (unspec:CCDSP [(match_dup 1) (match_dup 2)] UNSPEC_ADDSC))])]
  "TARGET_MIPS_DSP32 && !TARGET_MIPS16"
  "addsc\t%0,%z1,%z2"
  [(set_attr "type"	"dspalu")
   (set_attr "mode"	"SI")])

; ADDWC
(define_insn "mips_addwc"
  [(parallel
    [(set (match_operand:SI 0 "register_operand" "=d")
	  (unspec:SI [(match_operand:SI 1 "reg_or_0_operand" "dJ")
		      (match_operand:SI 2 "reg_or_0_operand" "dJ")
		    (reg:CCDSP CCDSP_CA_REGNUM)]
		     UNSPEC_ADDWC))
     (set (reg:CCDSP CCDSP_OU_REGNUM)
	  (unspec:CCDSP [(match_dup 1) (match_dup 2)] UNSPEC_ADDWC))])]
  "TARGET_MIPS_DSP32 && !TARGET_MIPS16"
  "addwc\t%0,%z1,%z2"
  [(set_attr "type"	"dspalu")
   (set_attr "mode"	"SI")])

; MODSUB 
(define_insn "mips_modsub"
  [(set (match_operand:SI 0 "register_operand" "=d")
	(unspec:SI [(match_operand:SI 1 "reg_or_0_operand" "dJ")
		    (match_operand:SI 2 "reg_or_0_operand" "dJ")]
		   UNSPEC_MODSUB))]
  "TARGET_MIPS_DSP32 && !TARGET_MIPS16"
  "modsub\t%0,%z1,%z2"
  [(set_attr "type"	"dspalu")
   (set_attr "mode"	"SI")])

; RADDU*
(define_insn "mips_raddu_w_qb"
  [(set (match_operand:SI 0 "register_operand" "=d")
	(unspec:SI [(match_operand:V4QI 1 "reg_or_0_operand" "dY")]
		   UNSPEC_RADDU_W_QB))]
  "TARGET_MIPS_DSP32 && !TARGET_MIPS16"
  "raddu.w.qb\t%0,%z1"
  [(set_attr "type"	"dspalu")
   (set_attr "mode"	"SI")])

(define_insn "mips_raddu_l_ob"
  [(set (match_operand:DI 0 "register_operand" "=d")
	(unspec:DI [(match_operand:V8QI 1 "reg_or_0_operand" "dY")]
		   UNSPEC_RADDU_L_OB))]
  "TARGET_MIPS_DSP64 && !TARGET_MIPS16"
  "raddu.l.ob\t%0,%z1"
  [(set_attr "type"	"dspalu")
   (set_attr "mode"	"DI")])

; ABSQ*
(define_insn "mips_absq_s_ph"
  [(parallel
    [(set (match_operand:V2HI 0 "register_operand" "=d")
	  (unspec:V2HI [(match_operand:V2HI 1 "reg_or_0_operand" "dY")]
		       UNSPEC_ABSQ_S_PH))
     (set (reg:CCDSP CCDSP_OU_REGNUM)
	  (unspec:CCDSP [(match_dup 1)] UNSPEC_ABSQ_S_PH))])]
  "TARGET_MIPS_DSP32 && !TARGET_MIPS16"
  "absq_s.ph\t%0,%z1"
  [(set_attr "type"	"dspalusat")
   (set_attr "mode"	"SI")])

(define_insn "mips_absq_s_w"
  [(parallel
    [(set (match_operand:SI 0 "register_operand" "=d")
	  (unspec:SI [(match_operand:SI 1 "reg_or_0_operand" "dJ")]
		     UNSPEC_ABSQ_S_W))
     (set (reg:CCDSP CCDSP_OU_REGNUM)
	  (unspec:CCDSP [(match_dup 1)] UNSPEC_ABSQ_S_W))])]
  "TARGET_MIPS_DSP32 && !TARGET_MIPS16"
  "absq_s.w\t%0,%z1"
  [(set_attr "type"	"dspalusat")
   (set_attr "mode"	"SI")])

(define_insn "mips_absq_qh"
  [(parallel
    [(set (match_operand:V4HI 0 "register_operand" "=d")
	  (unspec:V4HI [(match_operand:V4HI 1 "reg_or_0_operand" "dY")]
		       UNSPEC_ABSQ_QH))
     (set (reg:CCDSP CCDSP_OU_REGNUM)
	  (unspec:CCDSP [(match_dup 1)] UNSPEC_ABSQ_QH))])]
  "TARGET_MIPS_DSP64 && !TARGET_MIPS16"
  "absq.qh\t%0,%z1"
  [(set_attr "type"	"dspalu")
   (set_attr "mode"	"DI")])

(define_insn "mips_absq_s_qh"
  [(parallel
    [(set (match_operand:V4HI 0 "register_operand" "=d")
	  (unspec:V4HI [(match_operand:V4HI 1 "reg_or_0_operand" "dY")]
		       UNSPEC_ABSQ_S_QH))
     (set (reg:CCDSP CCDSP_OU_REGNUM)
	  (unspec:CCDSP [(match_dup 1)] UNSPEC_ABSQ_S_QH))])]
  "TARGET_MIPS_DSP64 && !TARGET_MIPS16"
  "absq_s.qh\t%0,%z1"
  [(set_attr "type"	"dspalusat")
   (set_attr "mode"	"DI")])

(define_insn "mips_absq_pw"
  [(parallel
    [(set (match_operand:V2SI 0 "register_operand" "=d")
	  (unspec:V2SI [(match_operand:V2SI 1 "reg_or_0_operand" "dY")]
		       UNSPEC_ABSQ_PW))
     (set (reg:CCDSP CCDSP_OU_REGNUM)
	  (unspec:CCDSP [(match_dup 1)] UNSPEC_ABSQ_PW))])]
  "TARGET_MIPS_DSP64 && !TARGET_MIPS16"
  "absq.pw\t%0,%z1"
  [(set_attr "type"	"dspalu")
   (set_attr "mode"	"DI")])

(define_insn "mips_absq_s_pw"
  [(parallel
    [(set (match_operand:V2SI 0 "register_operand" "=d")
	  (unspec:V2SI [(match_operand:V2SI 1 "reg_or_0_operand" "dY")]
		       UNSPEC_ABSQ_S_PW))
     (set (reg:CCDSP CCDSP_OU_REGNUM)
	  (unspec:CCDSP [(match_dup 1)] UNSPEC_ABSQ_S_PW))])]
  "TARGET_MIPS_DSP64 && !TARGET_MIPS16"
  "absq_s.pw\t%0,%z1"
  [(set_attr "type"	"dspalusat")
   (set_attr "mode"	"DI")])

; PRECRQ*
(define_insn "mips_precrq_qb_ph"
  [(set (match_operand:V4QI 0 "register_operand" "=d")
	(unspec:V4QI [(match_operand:V2HI 1 "reg_or_0_operand" "dY")
		      (match_operand:V2HI 2 "reg_or_0_operand" "dY")]
		     UNSPEC_PRECRQ_QB_PH))]
  "TARGET_MIPS_DSP32 && !TARGET_MIPS16"
  "precrq.qb.ph\t%0,%z1,%z2"
  [(set_attr "type"	"dspalu")
   (set_attr "mode"	"SI")])

(define_insn "mips_precrq_ph_w"
  [(set (match_operand:V2HI 0 "register_operand" "=d")
	(unspec:V2HI [(match_operand:SI 1 "reg_or_0_operand" "dJ")
		      (match_operand:SI 2 "reg_or_0_operand" "dJ")]
		     UNSPEC_PRECRQ_PH_W))]
  "TARGET_MIPS_DSP32 && !TARGET_MIPS16"
  "precrq.ph.w\t%0,%z1,%z2"
  [(set_attr "type"	"dspalu")
   (set_attr "mode"	"SI")])

(define_insn "mips_precrq_rs_ph_w"
  [(parallel
    [(set (match_operand:V2HI 0 "register_operand" "=d")
	  (unspec:V2HI [(match_operand:SI 1 "reg_or_0_operand" "dJ")
			(match_operand:SI 2 "reg_or_0_operand" "dJ")]
		       UNSPEC_PRECRQ_RS_PH_W))
     (set (reg:CCDSP CCDSP_OU_REGNUM)
	  (unspec:CCDSP [(match_dup 1) (match_dup 2)] UNSPEC_PRECRQ_RS_PH_W))])]
  "TARGET_MIPS_DSP32 && !TARGET_MIPS16"
  "precrq_rs.ph.w\t%0,%z1,%z2"
  [(set_attr "type"	"dspalusat")
   (set_attr "mode"	"SI")])

(define_insn "mips_precrq_ob_qh"
  [(set (match_operand:V8QI 0 "register_operand" "=d")
	(unspec:V8QI [(match_operand:V4HI 1 "reg_or_0_operand" "dY")
		      (match_operand:V4HI 2 "reg_or_0_operand" "dY")]
		     UNSPEC_PRECRQ_OB_QH))]
  "TARGET_MIPS_DSP64 && !TARGET_MIPS16"
  "precrq.ob.qh\t%0,%z1,%z2"
  [(set_attr "type"	"dspalu")
   (set_attr "mode"	"DI")])

(define_insn "mips_precrq_qh_pw"
  [(set (match_operand:V4HI 0 "register_operand" "=d")
	(unspec:V4HI [(match_operand:V2SI 1 "reg_or_0_operand" "dY")
		      (match_operand:V2SI 2 "reg_or_0_operand" "dY")]
		     UNSPEC_PRECRQ_QH_PW))]
  "TARGET_MIPS_DSP64 && !TARGET_MIPS16"
  "precrq.qh.pw\t%0,%z1,%z2"
  [(set_attr "type"	"dspalu")
   (set_attr "mode"	"DI")])

(define_insn "mips_precrq_rs_qh_pw"
  [(parallel
    [(set (match_operand:V4HI 0 "register_operand" "=d")
	  (unspec:V4HI [(match_operand:V2SI 1 "reg_or_0_operand" "dY")
			(match_operand:V2SI 2 "reg_or_0_operand" "dY")]
		       UNSPEC_PRECRQ_RS_QH_PW))
     (set (reg:CCDSP CCDSP_OU_REGNUM)
	  (unspec:CCDSP [(match_dup 1) (match_dup 2)] UNSPEC_PRECRQ_RS_QH_PW))])]
  "TARGET_MIPS_DSP64 && !TARGET_MIPS16"
  "precrq_rs.qh.pw\t%0,%z1,%z2"
  [(set_attr "type"	"dspalusat")
   (set_attr "mode"	"DI")])

(define_insn "mips_precrq_pw_l"
  [(set (match_operand:V2SI 0 "register_operand" "=d")
	(unspec:V2SI [(match_operand:DI 1 "reg_or_0_operand" "dJ")
		      (match_operand:DI 2 "reg_or_0_operand" "dJ")]
		     UNSPEC_PRECRQ_PW_L))]
  "TARGET_MIPS_DSP64 && !TARGET_MIPS16"
  "precrq.pw.l\t%0,%z1,%z2"
  [(set_attr "type"	"dspalu")
   (set_attr "mode"	"DI")])

; PRECRQU*
(define_insn "mips_precrqu_s_qb_ph"
  [(parallel
    [(set (match_operand:V4QI 0 "register_operand" "=d")
	  (unspec:V4QI [(match_operand:V2HI 1 "reg_or_0_operand" "dY")
			(match_operand:V2HI 2 "reg_or_0_operand" "dY")]
		       UNSPEC_PRECRQU_S_QB_PH))
     (set (reg:CCDSP CCDSP_OU_REGNUM)
	  (unspec:CCDSP [(match_dup 1) (match_dup 2)] UNSPEC_PRECRQU_S_QB_PH))])]
  "TARGET_MIPS_DSP32 && !TARGET_MIPS16"
  "precrqu_s.qb.ph\t%0,%z1,%z2"
  [(set_attr "type"	"dspalusat")
   (set_attr "mode"	"SI")])

(define_insn "mips_precrqu_s_ob_qh"
  [(parallel
    [(set (match_operand:V8QI 0 "register_operand" "=d")
	  (unspec:V8QI [(match_operand:V4HI 1 "reg_or_0_operand" "dY")
			(match_operand:V4HI 2 "reg_or_0_operand" "dY")]
		       UNSPEC_PRECRQU_S_OB_QH))
     (set (reg:CCDSP CCDSP_OU_REGNUM)
	  (unspec:CCDSP [(match_dup 1) (match_dup 2)] UNSPEC_PRECRQU_S_OB_QH))])]
  "TARGET_MIPS_DSP64 && !TARGET_MIPS16"
  "precrqu_s.ob.qh\t%0,%z1,%z2"
  [(set_attr "type"	"dspalusat")
   (set_attr "mode"	"DI")])

(define_insn "mips_precrqu_s_qh_pw"
  [(parallel
    [(set (match_operand:V4HI 0 "register_operand" "=d")
	  (unspec:V4HI [(match_operand:V2SI 1 "reg_or_0_operand" "dY")
			(match_operand:V2SI 2 "reg_or_0_operand" "dY")]
		       UNSPEC_PRECRQU_S_QH_PW))
     (set (reg:CCDSP CCDSP_OU_REGNUM)
	  (unspec:CCDSP [(match_dup 1) (match_dup 2)] UNSPEC_PRECRQU_S_QH_PW))])]
  "TARGET_MIPS_DSP64 && !TARGET_MIPS16"
  "precrqu_s.qh.pw\t%0,%z1,%z2"
  [(set_attr "type"	"dspalusat")
   (set_attr "mode"	"DI")])

(define_insn "mips_precrqu_s_pw_l"
  [(parallel
    [(set (match_operand:V2SI 0 "register_operand" "=d")
	  (unspec:V2SI [(match_operand:DI 1 "reg_or_0_operand" "dJ")
			(match_operand:DI 2 "reg_or_0_operand" "dJ")]
		       UNSPEC_PRECRQU_S_PW_L))
     (set (reg:CCDSP CCDSP_OU_REGNUM)
	  (unspec:CCDSP [(match_dup 1) (match_dup 2)] UNSPEC_PRECRQU_S_PW_L))])]
  "TARGET_MIPS_DSP64 && !TARGET_MIPS16"
  "precrqu_s.pw.l\t%0,%z1,%z2"
  [(set_attr "type"	"dspalusat")
   (set_attr "mode"	"DI")])

; PRECEQ*
(define_insn "mips_preceq_w_phl"
  [(set (match_operand:SI 0 "register_operand" "=d")
	(unspec:SI [(match_operand:V2HI 1 "reg_or_0_operand" "dY")]
		   UNSPEC_PRECEQ_W_PHL))]
  "TARGET_MIPS_DSP32 && !TARGET_MIPS16"
  "preceq.w.phl\t%0,%z1"
  [(set_attr "type"	"dspalu")
   (set_attr "mode"	"SI")])

(define_insn "mips_preceq_w_phr"
  [(set (match_operand:SI 0 "register_operand" "=d")
	(unspec:SI [(match_operand:V2HI 1 "reg_or_0_operand" "dY")]
		   UNSPEC_PRECEQ_W_PHR))]
  "TARGET_MIPS_DSP32 && !TARGET_MIPS16"
  "preceq.w.phr\t%0,%z1"
  [(set_attr "type"	"dspalu")
   (set_attr "mode"	"SI")])

(define_insn "mips_preceq_pw_qhl"
  [(set (match_operand:V2SI 0 "register_operand" "=d")
	(unspec:V2SI [(match_operand:V4HI 1 "reg_or_0_operand" "dY")]
		     UNSPEC_PRECEQ_PW_QHL))]
  "TARGET_MIPS_DSP64 && !TARGET_MIPS16"
  "preceq.pw.qhl\t%0,%z1"
  [(set_attr "type"	"dspalu")
   (set_attr "mode"	"DI")])

(define_insn "mips_preceq_pw_qhr"
  [(set (match_operand:V2SI 0 "register_operand" "=d")
	(unspec:V2SI [(match_operand:V4HI 1 "reg_or_0_operand" "dY")]
		     UNSPEC_PRECEQ_PW_QHR))]
  "TARGET_MIPS_DSP64 && !TARGET_MIPS16"
  "preceq.pw.qhr\t%0,%z1"
  [(set_attr "type"	"dspalu")
   (set_attr "mode"	"DI")])

(define_insn "mips_preceq_pw_qhla"
  [(set (match_operand:V2SI 0 "register_operand" "=d")
	(unspec:V2SI [(match_operand:V4HI 1 "reg_or_0_operand" "dY")]
		     UNSPEC_PRECEQ_PW_QHLA))]
  "TARGET_MIPS_DSP64 && !TARGET_MIPS16"
  "preceq.pw.qhla\t%0,%z1"
  [(set_attr "type"	"dspalu")
   (set_attr "mode"	"DI")])

(define_insn "mips_preceq_pw_qhra"
  [(set (match_operand:V2SI 0 "register_operand" "=d")
	(unspec:V2SI [(match_operand:V4HI 1 "reg_or_0_operand" "dY")]
		     UNSPEC_PRECEQ_PW_QHRA))]
  "TARGET_MIPS_DSP64 && !TARGET_MIPS16"
  "preceq.pw.qhra\t%0,%z1"
  [(set_attr "type"	"dspalu")
   (set_attr "mode"	"DI")])

(define_insn "mips_preceq_l_pwl"
  [(set (match_operand:DI 0 "register_operand" "=d")
	(unspec:DI [(match_operand:V2SI 1 "reg_or_0_operand" "dY")]
		   UNSPEC_PRECEQ_L_PWL))]
  "TARGET_MIPS_DSP64 && !TARGET_MIPS16"
  "preceq.l.pwl\t%0,%z1"
  [(set_attr "type"	"dspalu")
   (set_attr "mode"	"DI")])

(define_insn "mips_preceq_l_pwr"
  [(set (match_operand:DI 0 "register_operand" "=d")
	(unspec:DI [(match_operand:V2SI 1 "reg_or_0_operand" "dY")]
		   UNSPEC_PRECEQ_L_PWR))]
  "TARGET_MIPS_DSP64 && !TARGET_MIPS16"
  "preceq.l.pwr\t%0,%z1"
  [(set_attr "type"	"dspalu")
   (set_attr "mode"	"DI")])

; PRECEQU*
(define_insn "mips_precequ_ph_qbl"
  [(set (match_operand:V2HI 0 "register_operand" "=d")
	(unspec:V2HI [(match_operand:V4QI 1 "reg_or_0_operand" "dY")]
		     UNSPEC_PRECEQU_PH_QBL))]
  "TARGET_MIPS_DSP32 && !TARGET_MIPS16"
  "precequ.ph.qbl\t%0,%z1"
  [(set_attr "type"	"dspalu")
   (set_attr "mode"	"SI")])

(define_insn "mips_precequ_ph_qbr"
  [(set (match_operand:V2HI 0 "register_operand" "=d")
	(unspec:V2HI [(match_operand:V4QI 1 "reg_or_0_operand" "dY")]
		     UNSPEC_PRECEQU_PH_QBR))]
  "TARGET_MIPS_DSP32 && !TARGET_MIPS16"
  "precequ.ph.qbr\t%0,%z1"
  [(set_attr "type"	"dspalu")
   (set_attr "mode"	"SI")])

(define_insn "mips_precequ_ph_qbla"
  [(set (match_operand:V2HI 0 "register_operand" "=d")
	(unspec:V2HI [(match_operand:V4QI 1 "reg_or_0_operand" "dY")]
		     UNSPEC_PRECEQU_PH_QBLA))]
  "TARGET_MIPS_DSP32 && !TARGET_MIPS16"
  "precequ.ph.qbla\t%0,%z1"
  [(set_attr "type"	"dspalu")
   (set_attr "mode"	"SI")])

(define_insn "mips_precequ_ph_qbra"
  [(set (match_operand:V2HI 0 "register_operand" "=d")
	(unspec:V2HI [(match_operand:V4QI 1 "reg_or_0_operand" "dY")]
		     UNSPEC_PRECEQU_PH_QBRA))]
  "TARGET_MIPS_DSP32 && !TARGET_MIPS16"
  "precequ.ph.qbra\t%0,%z1"
  [(set_attr "type"	"dspalu")
   (set_attr "mode"	"SI")])

(define_insn "mips_precequ_qh_obl"
  [(set (match_operand:V4HI 0 "register_operand" "=d")
	(unspec:V4HI [(match_operand:V8QI 1 "reg_or_0_operand" "dY")]
		     UNSPEC_PRECEQU_QH_OBL))]
  "TARGET_MIPS_DSP64 && !TARGET_MIPS16"
  "precequ.qh.obl\t%0,%z1"
  [(set_attr "type"	"dspalu")
   (set_attr "mode"	"DI")])

(define_insn "mips_precequ_qh_obr"
  [(set (match_operand:V4HI 0 "register_operand" "=d")
	(unspec:V4HI [(match_operand:V8QI 1 "reg_or_0_operand" "dY")]
		     UNSPEC_PRECEQU_QH_OBR))]
  "TARGET_MIPS_DSP64 && !TARGET_MIPS16"
  "precequ.qh.obr\t%0,%z1"
  [(set_attr "type"	"dspalu")
   (set_attr "mode"	"DI")])

(define_insn "mips_precequ_qh_obla"
  [(set (match_operand:V4HI 0 "register_operand" "=d")
	(unspec:V4HI [(match_operand:V8QI 1 "reg_or_0_operand" "dY")]
		     UNSPEC_PRECEQU_QH_OBLA))]
  "TARGET_MIPS_DSP64 && !TARGET_MIPS16"
  "precequ.qh.obla\t%0,%z1"
  [(set_attr "type"	"dspalu")
   (set_attr "mode"	"DI")])

(define_insn "mips_precequ_qh_obra"
  [(set (match_operand:V4HI 0 "register_operand" "=d")
	(unspec:V4HI [(match_operand:V8QI 1 "reg_or_0_operand" "dY")]
		     UNSPEC_PRECEQU_QH_OBRA))]
  "TARGET_MIPS_DSP64 && !TARGET_MIPS16"
  "precequ.qh.obra\t%0,%z1"
  [(set_attr "type"	"dspalu")
   (set_attr "mode"	"DI")])

; PRECEU*
(define_insn "mips_preceu_ph_qbl"
  [(set (match_operand:V2HI 0 "register_operand" "=d")
	(unspec:V2HI [(match_operand:V4QI 1 "reg_or_0_operand" "dY")]
		     UNSPEC_PRECEU_PH_QBL))]
  "TARGET_MIPS_DSP32 && !TARGET_MIPS16"
  "preceu.ph.qbl\t%0,%z1"
  [(set_attr "type"	"dspalu")
   (set_attr "mode"	"SI")])

(define_insn "mips_preceu_ph_qbr"
  [(set (match_operand:V2HI 0 "register_operand" "=d")
	(unspec:V2HI [(match_operand:V4QI 1 "reg_or_0_operand" "dY")]
		     UNSPEC_PRECEU_PH_QBR))]
  "TARGET_MIPS_DSP32 && !TARGET_MIPS16"
  "preceu.ph.qbr\t%0,%z1"
  [(set_attr "type"	"dspalu")
   (set_attr "mode"	"SI")])

(define_insn "mips_preceu_ph_qbla"
  [(set (match_operand:V2HI 0 "register_operand" "=d")
	(unspec:V2HI [(match_operand:V4QI 1 "reg_or_0_operand" "dY")]
		     UNSPEC_PRECEU_PH_QBLA))]
  "TARGET_MIPS_DSP32 && !TARGET_MIPS16"
  "preceu.ph.qbla\t%0,%z1"
  [(set_attr "type"	"dspalu")
   (set_attr "mode"	"SI")])

(define_insn "mips_preceu_ph_qbra"
  [(set (match_operand:V2HI 0 "register_operand" "=d")
	(unspec:V2HI [(match_operand:V4QI 1 "reg_or_0_operand" "dY")]
		     UNSPEC_PRECEU_PH_QBRA))]
  "TARGET_MIPS_DSP32 && !TARGET_MIPS16"
  "preceu.ph.qbra\t%0,%z1"
  [(set_attr "type"	"dspalu")
   (set_attr "mode"	"SI")])

; Table 2-2. MIPS DSP ASE Instructions: Shift
; SHLL*
(define_insn "mips_shll_qb"
  [(parallel
    [(set (match_operand:V4QI 0 "register_operand" "=d,d")
	  (unspec:V4QI [(match_operand:V4QI 1 "reg_or_0_operand" "dY,dY")
			(match_operand:SI 2 "arith_operand" "I,d")]
		       UNSPEC_SHLL_QB))
     (set (reg:CCDSP CCDSP_OU_REGNUM)
	  (unspec:CCDSP [(match_dup 1) (match_dup 2)] UNSPEC_SHLL_QB))])]
  "TARGET_MIPS_DSP32 && !TARGET_MIPS16"
  "@
   shll.qb\t%0,%z1,%2
   shllv.qb\t%0,%z1,%2"
  [(set_attr "type"	"dspalu")
   (set_attr "mode"	"SI")])

(define_insn "mips_shll_ob"
  [(parallel
    [(set (match_operand:V8QI 0 "register_operand" "=d,d")
	  (unspec:V8QI [(match_operand:V8QI 1 "reg_or_0_operand" "dY,dY")
			(match_operand:DI 2 "arith_operand" "I,d")]
		       UNSPEC_SHLL_OB))
     (set (reg:CCDSP CCDSP_OU_REGNUM)
	  (unspec:CCDSP [(match_dup 1) (match_dup 2)] UNSPEC_SHLL_OB))])]
  "TARGET_MIPS_DSP64 && !TARGET_MIPS16"
  "@
   shll.ob\t%0,%z1,%2
   shllv.ob\t%0,%z1,%2"
  [(set_attr "type"	"dspalu")
   (set_attr "mode"	"DI")])

(define_insn "mips_shll_ph"
  [(parallel
    [(set (match_operand:V2HI 0 "register_operand" "=d,d")
	  (unspec:V2HI [(match_operand:V2HI 1 "reg_or_0_operand" "dY,dY")
			(match_operand:SI 2 "arith_operand" "I,d")]
		       UNSPEC_SHLL_PH))
     (set (reg:CCDSP CCDSP_OU_REGNUM)
	  (unspec:CCDSP [(match_dup 1) (match_dup 2)] UNSPEC_SHLL_PH))])]
  "TARGET_MIPS_DSP32 && !TARGET_MIPS16"
  "@
   shll.ph\t%0,%z1,%2
   shllv.ph\t%0,%z1,%2"
  [(set_attr "type"	"dspalu")
   (set_attr "mode"	"SI")])

(define_insn "mips_shll_s_ph"
  [(parallel
    [(set (match_operand:V2HI 0 "register_operand" "=d,d")
	  (unspec:V2HI [(match_operand:V2HI 1 "reg_or_0_operand" "dY,dY")
			(match_operand:SI 2 "arith_operand" "I,d")]
		       UNSPEC_SHLL_S_PH))
     (set (reg:CCDSP CCDSP_OU_REGNUM)
	  (unspec:CCDSP [(match_dup 1) (match_dup 2)] UNSPEC_SHLL_S_PH))])]
  "TARGET_MIPS_DSP32 && !TARGET_MIPS16"
  "@
   shll_s.ph\t%0,%z1,%2
   shllv_s.ph\t%0,%z1,%2"
  [(set_attr "type"	"dspalusat")
   (set_attr "mode"	"SI")])

(define_insn "mips_shll_qh"
  [(parallel
    [(set (match_operand:V4HI 0 "register_operand" "=d,d")
	  (unspec:V4HI [(match_operand:V4HI 1 "reg_or_0_operand" "dY,dY")
			(match_operand:DI 2 "arith_operand" "I,d")]
		       UNSPEC_SHLL_QH))
     (set (reg:CCDSP CCDSP_OU_REGNUM)
	  (unspec:CCDSP [(match_dup 1) (match_dup 2)] UNSPEC_SHLL_QH))])]
  "TARGET_MIPS_DSP64 && !TARGET_MIPS16"
  "@
   shll.qh\t%0,%z1,%2
   shllv.qh\t%0,%z1,%2"
  [(set_attr "type"	"dspalu")
   (set_attr "mode"	"DI")])

(define_insn "mips_shll_s_qh"
  [(parallel
    [(set (match_operand:V4HI 0 "register_operand" "=d,d")
	  (unspec:V4HI [(match_operand:V4HI 1 "reg_or_0_operand" "dY,dY")
			(match_operand:DI 2 "arith_operand" "I,d")]
		       UNSPEC_SHLL_S_QH))
     (set (reg:CCDSP CCDSP_OU_REGNUM)
	  (unspec:CCDSP [(match_dup 1) (match_dup 2)] UNSPEC_SHLL_S_QH))])]
  "TARGET_MIPS_DSP64 && !TARGET_MIPS16"
  "@
   shll_s.qh\t%0,%z1,%2
   shllv_s.qh\t%0,%z1,%2"
  [(set_attr "type"	"dspalusat")
   (set_attr "mode"	"DI")])

(define_insn "mips_shll_s_w"
  [(parallel
    [(set (match_operand:SI 0 "register_operand" "=d,d")
	  (unspec:SI [(match_operand:SI 1 "reg_or_0_operand" "dJ,dJ")
		      (match_operand:SI 2 "arith_operand" "I,d")]
		     UNSPEC_SHLL_S_W))
     (set (reg:CCDSP CCDSP_OU_REGNUM)
	  (unspec:CCDSP [(match_dup 1) (match_dup 2)] UNSPEC_SHLL_S_W))])]
  "TARGET_MIPS_DSP32 && !TARGET_MIPS16"
  "@
   shll_s.w\t%0,%z1,%2
   shllv_s.w\t%0,%z1,%2"
  [(set_attr "type"	"dspalusat")
   (set_attr "mode"	"SI")])

(define_insn "mips_shll_pw"
  [(parallel
    [(set (match_operand:V2SI 0 "register_operand" "=d,d")
	  (unspec:V2SI [(match_operand:V2SI 1 "reg_or_0_operand" "dY,dY")
			(match_operand:DI 2 "arith_operand" "I,d")]
		       UNSPEC_SHLL_PW))
     (set (reg:CCDSP CCDSP_OU_REGNUM)
	  (unspec:CCDSP [(match_dup 1) (match_dup 2)] UNSPEC_SHLL_PW))])]
  "TARGET_MIPS_DSP64 && !TARGET_MIPS16"
  "@
   shll.pw\t%0,%z1,%2
   shllv.pw\t%0,%z1,%2"
  [(set_attr "type"	"dspalu")
   (set_attr "mode"	"DI")])

(define_insn "mips_shll_s_pw"
  [(parallel
    [(set (match_operand:V2SI 0 "register_operand" "=d,d")
	  (unspec:V2SI [(match_operand:V2SI 1 "reg_or_0_operand" "dY,dY")
			(match_operand:DI 2 "arith_operand" "I,d")]
		       UNSPEC_SHLL_S_PW))
     (set (reg:CCDSP CCDSP_OU_REGNUM)
	  (unspec:CCDSP [(match_dup 1) (match_dup 2)] UNSPEC_SHLL_S_PW))])]
  "TARGET_MIPS_DSP64 && !TARGET_MIPS16"
  "@
   shll_s.pw\t%0,%z1,%2
   shllv_s.pw\t%0,%z1,%2"
  [(set_attr "type"	"dspalusat")
   (set_attr "mode"	"DI")])

; SHRL*
(define_insn "mips_shrl_qb"
  [(set (match_operand:V4QI 0 "register_operand" "=d,d")
	(unspec:V4QI [(match_operand:V4QI 1 "reg_or_0_operand" "dY,dY")
		      (match_operand:SI 2 "arith_operand" "I,d")]
		     UNSPEC_SHRL_QB))]
  "TARGET_MIPS_DSP32 && !TARGET_MIPS16"
  "@
   shrl.qb\t%0,%z1,%2
   shrlv.qb\t%0,%z1,%2"
  [(set_attr "type"	"dspalu")
   (set_attr "mode"	"SI")])

(define_insn "mips_shrl_ob"
  [(set (match_operand:V8QI 0 "register_operand" "=d,d")
	(unspec:V8QI [(match_operand:V8QI 1 "reg_or_0_operand" "dY,dY")
		      (match_operand:DI 2 "arith_operand" "I,d")]
		     UNSPEC_SHRL_OB))]
  "TARGET_MIPS_DSP64 && !TARGET_MIPS16"
  "@
   shrl.ob\t%0,%z1,%2
   shrlv.ob\t%0,%z1,%2"
  [(set_attr "type"	"dspalu")
   (set_attr "mode"	"DI")])

; SHRA*
(define_insn "mips_shra_ph"
  [(set (match_operand:V2HI 0 "register_operand" "=d,d")
	(unspec:V2HI [(match_operand:V2HI 1 "reg_or_0_operand" "dY,dY")
		      (match_operand:SI 2 "arith_operand" "I,d")]
		     UNSPEC_SHRA_PH))]
  "TARGET_MIPS_DSP32 && !TARGET_MIPS16"
  "@
   shra.ph\t%0,%z1,%2
   shrav.ph\t%0,%z1,%2"
  [(set_attr "type"	"dspalu")
   (set_attr "mode"	"SI")])

(define_insn "mips_shra_r_ph"
  [(set (match_operand:V2HI 0 "register_operand" "=d,d")
	(unspec:V2HI [(match_operand:V2HI 1 "reg_or_0_operand" "dY,dY")
		      (match_operand:SI 2 "arith_operand" "I,d")]
		     UNSPEC_SHRA_R_PH))]
  "TARGET_MIPS_DSP32 && !TARGET_MIPS16"
  "@
   shra_r.ph\t%0,%z1,%2
   shrav_r.ph\t%0,%z1,%2"
  [(set_attr "type"	"dspalu")
   (set_attr "mode"	"SI")])

(define_insn "mips_shra_r_w"
  [(set (match_operand:SI 0 "register_operand" "=d,d")
	(unspec:SI [(match_operand:SI 1 "reg_or_0_operand" "dJ,dJ")
		    (match_operand:SI 2 "arith_operand" "I,d")]
		   UNSPEC_SHRA_R_W))]
  "TARGET_MIPS_DSP32 && !TARGET_MIPS16"
  "@
   shra_r.w\t%0,%z1,%2
   shrav_r.w\t%0,%z1,%2"
  [(set_attr "type"	"dspalu")
   (set_attr "mode"	"SI")])

(define_insn "mips_shra_qh"
  [(set (match_operand:V4HI 0 "register_operand" "=d,d")
	(unspec:V4HI [(match_operand:V4HI 1 "reg_or_0_operand" "dY,dY")
		      (match_operand:DI 2 "arith_operand" "I,d")]
		     UNSPEC_SHRA_QH))]
  "TARGET_MIPS_DSP64 && !TARGET_MIPS16"
  "@
   shra.qh\t%0,%z1,%2
   shrav.qh\t%0,%z1,%2"
  [(set_attr "type"	"dspalu")
   (set_attr "mode"	"DI")])

(define_insn "mips_shra_r_qh"
  [(set (match_operand:V4HI 0 "register_operand" "=d,d")
	(unspec:V4HI [(match_operand:V4HI 1 "reg_or_0_operand" "dY,dY")
		      (match_operand:DI 2 "arith_operand" "I,d")]
		     UNSPEC_SHRA_R_QH))]
  "TARGET_MIPS_DSP64 && !TARGET_MIPS16"
  "@
   shra_r.qh\t%0,%z1,%2
   shrav_r.qh\t%0,%z1,%2"
  [(set_attr "type"	"dspalu")
   (set_attr "mode"	"DI")])

(define_insn "mips_shra_pw"
  [(set (match_operand:V2SI 0 "register_operand" "=d,d")
	(unspec:V2SI [(match_operand:V2SI 1 "reg_or_0_operand" "dY,dY")
		      (match_operand:DI 2 "arith_operand" "I,d")]
		     UNSPEC_SHRA_PW))]
  "TARGET_MIPS_DSP64 && !TARGET_MIPS16"
  "@
   shra.pw\t%0,%z1,%2
   shrav.pw\t%0,%z1,%2"
  [(set_attr "type"	"dspalu")
   (set_attr "mode"	"DI")])

(define_insn "mips_shra_r_pw"
  [(set (match_operand:V2SI 0 "register_operand" "=d,d")
	(unspec:V2SI [(match_operand:V2SI 1 "reg_or_0_operand" "dY,dY")
		      (match_operand:DI 2 "arith_operand" "I,d")]
		     UNSPEC_SHRA_R_PW))]
  "TARGET_MIPS_DSP64 && !TARGET_MIPS16"
  "@
   shra_r.pw\t%0,%z1,%2
   shrav_r.pw\t%0,%z1,%2"
  [(set_attr "type"	"dspalu")
   (set_attr "mode"	"DI")])

; Table 2-3. MIPS DSP ASE Instructions: Multiply
; MULEU*
(define_insn "mips_muleu_s_ph_qbl"
  [(parallel
    [(set (match_operand:V2HI 0 "register_operand" "=d")
	  (unspec:V2HI [(match_operand:V4QI 1 "reg_or_0_operand" "dY")
			(match_operand:V2HI 2 "reg_or_0_operand" "dY")]
		       UNSPEC_MULEU_S_PH_QBL))
     (set (reg:CCDSP CCDSP_OU_REGNUM)
	  (unspec:CCDSP [(match_dup 1) (match_dup 2)] UNSPEC_MULEU_S_PH_QBL))
     (clobber (match_scratch:DI 3 "=x"))])]
  "TARGET_MIPS_DSP32 && !TARGET_MIPS16"
  "muleu_s.ph.qbl\t%0,%z1,%z2"
  [(set_attr "type"	"imul3")
   (set_attr "mode"	"SI")])

(define_insn "mips_muleu_s_ph_qbr"
  [(parallel
    [(set (match_operand:V2HI 0 "register_operand" "=d")
	  (unspec:V2HI [(match_operand:V4QI 1 "reg_or_0_operand" "dY")
			(match_operand:V2HI 2 "reg_or_0_operand" "dY")]
		       UNSPEC_MULEU_S_PH_QBR))
     (set (reg:CCDSP CCDSP_OU_REGNUM)
	  (unspec:CCDSP [(match_dup 1) (match_dup 2)] UNSPEC_MULEU_S_PH_QBR))
     (clobber (match_scratch:DI 3 "=x"))])]
  "TARGET_MIPS_DSP32 && !TARGET_MIPS16"
  "muleu_s.ph.qbr\t%0,%z1,%z2"
  [(set_attr "type"	"imul3")
   (set_attr "mode"	"SI")])

; MULQ*
(define_insn "mips_mulq_rs_ph"
  [(parallel
    [(set (match_operand:V2HI 0 "register_operand" "=d")
	  (unspec:V2HI [(match_operand:V2HI 1 "reg_or_0_operand" "dY")
			(match_operand:V2HI 2 "reg_or_0_operand" "dY")]
		       UNSPEC_MULQ_RS_PH))
     (set (reg:CCDSP CCDSP_OU_REGNUM)
	  (unspec:CCDSP [(match_dup 1) (match_dup 2)] UNSPEC_MULQ_RS_PH))
     (clobber (match_scratch:DI 3 "=x"))])]
  "TARGET_MIPS_DSP32 && !TARGET_MIPS16"
  "mulq_rs.ph\t%0,%z1,%z2"
  [(set_attr "type"	"imul3")
   (set_attr "mode"	"SI")])

(define_insn "mips_mulq_qh"
  [(parallel
    [(set (match_operand:V4HI 0 "register_operand" "=d")
	  (unspec:V4HI [(match_operand:V4HI 1 "reg_or_0_operand" "dY")
			(match_operand:V4HI 2 "reg_or_0_operand" "dY")]
		       UNSPEC_MULQ_QH))
     (set (reg:CCDSP CCDSP_OU_REGNUM)
	  (unspec:CCDSP [(match_dup 1) (match_dup 2)] UNSPEC_MULQ_QH))
     (clobber (match_scratch:TI 3 "=x"))])]
  "TARGET_MIPS_DSP64 && !TARGET_MIPS16"
  "mulq.qh\t%0,%z1,%z2"
  [(set_attr "type"	"imul3")
   (set_attr "mode"	"DI")])

(define_insn "mips_mulq_rs_qh"
  [(parallel
    [(set (match_operand:V4HI 0 "register_operand" "=d")
	  (unspec:V4HI [(match_operand:V4HI 1 "reg_or_0_operand" "dY")
			(match_operand:V4HI 2 "reg_or_0_operand" "dY")]
		       UNSPEC_MULQ_RS_QH))
     (set (reg:CCDSP CCDSP_OU_REGNUM)
	  (unspec:CCDSP [(match_dup 1) (match_dup 2)] UNSPEC_MULQ_RS_QH))
     (clobber (match_scratch:TI 3 "=x"))])]
  "TARGET_MIPS_DSP64 && !TARGET_MIPS16"
  "mulq_rs.qh\t%0,%z1,%z2"
  [(set_attr "type"	"imul3")
   (set_attr "mode"	"DI")])

; MULEQ*
(define_insn "mips_muleq_s_w_phl"
  [(parallel
    [(set (match_operand:SI 0 "register_operand" "=d")
	  (unspec:SI [(match_operand:V2HI 1 "reg_or_0_operand" "dY")
		      (match_operand:V2HI 2 "reg_or_0_operand" "dY")]
		     UNSPEC_MULEQ_S_W_PHL))
     (set (reg:CCDSP CCDSP_OU_REGNUM)
	  (unspec:CCDSP [(match_dup 1) (match_dup 2)] UNSPEC_MULEQ_S_W_PHL))
     (clobber (match_scratch:DI 3 "=x"))])]
  "TARGET_MIPS_DSP32 && !TARGET_MIPS16"
  "muleq_s.w.phl\t%0,%z1,%z2"
  [(set_attr "type"	"imul3")
   (set_attr "mode"	"SI")])

(define_insn "mips_muleq_s_w_phr"
  [(parallel
    [(set (match_operand:SI 0 "register_operand" "=d")
	  (unspec:SI [(match_operand:V2HI 1 "reg_or_0_operand" "dY")
		      (match_operand:V2HI 2 "reg_or_0_operand" "dY")]
		     UNSPEC_MULEQ_S_W_PHR))
     (set (reg:CCDSP CCDSP_OU_REGNUM)
	  (unspec:CCDSP [(match_dup 1) (match_dup 2)] UNSPEC_MULEQ_S_W_PHR))
     (clobber (match_scratch:DI 3 "=x"))])]
  "TARGET_MIPS_DSP32 && !TARGET_MIPS16"
  "muleq_s.w.phr\t%0,%z1,%z2"
  [(set_attr "type"	"imul3")
   (set_attr "mode"	"SI")])

(define_insn "mips_muleq_pw_qhl"
  [(parallel
    [(set (match_operand:V2SI 0 "register_operand" "=d")
	  (unspec:V2SI [(match_operand:V4HI 1 "reg_or_0_operand" "dY")
			(match_operand:V4HI 2 "reg_or_0_operand" "dY")]
		       UNSPEC_MULEQ_PW_QHL))
     (set (reg:CCDSP CCDSP_OU_REGNUM)
	  (unspec:CCDSP [(match_dup 1) (match_dup 2)] UNSPEC_MULEQ_PW_QHL))
     (clobber (match_scratch:TI 3 "=x"))])]
  "TARGET_MIPS_DSP64 && !TARGET_MIPS16"
  "muleq.pw.qhl\t%0,%z1,%z2"
  [(set_attr "type"	"imul3")
   (set_attr "mode"	"DI")])

(define_insn "mips_muleq_pw_qhr"
  [(parallel
    [(set (match_operand:V2SI 0 "register_operand" "=d")
	  (unspec:V2SI [(match_operand:V4HI 1 "reg_or_0_operand" "dY")
			(match_operand:V4HI 2 "reg_or_0_operand" "dY")]
		       UNSPEC_MULEQ_PW_QHR))
     (set (reg:CCDSP CCDSP_OU_REGNUM)
	  (unspec:CCDSP [(match_dup 1) (match_dup 2)] UNSPEC_MULEQ_PW_QHR))
     (clobber (match_scratch:TI 3 "=x"))])]
  "TARGET_MIPS_DSP64 && !TARGET_MIPS16"
  "muleq.pw.qhr\t%0,%z1,%z2"
  [(set_attr "type"	"imul3")
   (set_attr "mode"	"DI")])

(define_insn "mips_muleq_s_pw_qhl"
  [(parallel
    [(set (match_operand:V2SI 0 "register_operand" "=d")
	  (unspec:V2SI [(match_operand:V4HI 1 "reg_or_0_operand" "dY")
			(match_operand:V4HI 2 "reg_or_0_operand" "dY")]
		       UNSPEC_MULEQ_S_PW_QHL))
     (set (reg:CCDSP CCDSP_OU_REGNUM)
	  (unspec:CCDSP [(match_dup 1) (match_dup 2)] UNSPEC_MULEQ_S_PW_QHL))
     (clobber (match_scratch:TI 3 "=x"))])]
  "TARGET_MIPS_DSP64 && !TARGET_MIPS16"
  "muleq_s.pw.qhl\t%0,%z1,%z2"
  [(set_attr "type"	"imul3")
   (set_attr "mode"	"DI")])

(define_insn "mips_muleq_s_pw_qhr"
  [(parallel
    [(set (match_operand:V2SI 0 "register_operand" "=d")
	  (unspec:V2SI [(match_operand:V4HI 1 "reg_or_0_operand" "dY")
			(match_operand:V4HI 2 "reg_or_0_operand" "dY")]
		       UNSPEC_MULEQ_S_PW_QHR))
     (set (reg:CCDSP CCDSP_OU_REGNUM)
	  (unspec:CCDSP [(match_dup 1) (match_dup 2)] UNSPEC_MULEQ_S_PW_QHR))
     (clobber (match_scratch:TI 3 "=x"))])]
  "TARGET_MIPS_DSP64 && !TARGET_MIPS16"
  "muleq_s.pw.qhr\t%0,%z1,%z2"
  [(set_attr "type"	"imul3")
   (set_attr "mode"	"DI")])

; DPAU*
(define_insn "mips_dpau_h_qbl"
  [(set (match_operand:DI 0 "register_operand" "=a")
	(unspec:DI [(match_operand:DI 1 "register_operand" "0")
		    (match_operand:V4QI 2 "reg_or_0_operand" "dY")
		    (match_operand:V4QI 3 "reg_or_0_operand" "dY")]
		   UNSPEC_DPAU_H_QBL))]
  "TARGET_MIPS_DSP32 && !TARGET_64BIT && !TARGET_MIPS16"
  "dpau.h.qbl\t%q0,%z2,%z3"
  [(set_attr "type"	"dspmac")
   (set_attr "mode"	"SI")])

(define_insn "mips_dpau_h_qbr"
  [(set (match_operand:DI 0 "register_operand" "=a")
	(unspec:DI [(match_operand:DI 1 "register_operand" "0")
		    (match_operand:V4QI 2 "reg_or_0_operand" "dY")
		    (match_operand:V4QI 3 "reg_or_0_operand" "dY")]
		   UNSPEC_DPAU_H_QBR))]
  "TARGET_MIPS_DSP32 && !TARGET_64BIT && !TARGET_MIPS16"
  "dpau.h.qbr\t%q0,%z2,%z3"
  [(set_attr "type"	"dspmac")
   (set_attr "mode"	"SI")])

(define_insn "mips_dpau_h_qbl_64"
  [(set (match_operand:TI 0 "register_operand" "=a")
	(unspec:TI [(match_operand:TI 1 "register_operand" "0")
		    (match_operand:V4QI 2 "reg_or_0_operand" "dY")
		    (match_operand:V4QI 3 "reg_or_0_operand" "dY")]
		   UNSPEC_DPAU_H_QBL_64))]
  "TARGET_MIPS_DSP64 && !TARGET_MIPS16"
  "dpau.h.qbl\t%q0,%z2,%z3"
  [(set_attr "type"	"dspmac")
   (set_attr "mode"	"DI")])

(define_insn "mips_dpau_h_qbr_64"
  [(set (match_operand:TI 0 "register_operand" "=a")
	(unspec:TI [(match_operand:TI 1 "register_operand" "0")
		    (match_operand:V4QI 2 "reg_or_0_operand" "dY")
		    (match_operand:V4QI 3 "reg_or_0_operand" "dY")]
		   UNSPEC_DPAU_H_QBR_64))]
  "TARGET_MIPS_DSP64 && !TARGET_MIPS16"
  "dpau.h.qbr\t%q0,%z2,%z3"
  [(set_attr "type"	"dspmac")
   (set_attr "mode"	"DI")])

; DPSU*
(define_insn "mips_dpsu_h_qbl"
  [(set (match_operand:DI 0 "register_operand" "=a")
	(unspec:DI [(match_operand:DI 1 "register_operand" "0")
		    (match_operand:V4QI 2 "reg_or_0_operand" "dY")
		    (match_operand:V4QI 3 "reg_or_0_operand" "dY")]
		   UNSPEC_DPSU_H_QBL))]
  "TARGET_MIPS_DSP32 && !TARGET_64BIT && !TARGET_MIPS16"
  "dpsu.h.qbl\t%q0,%z2,%z3"
  [(set_attr "type"	"dspmac")
   (set_attr "mode"	"SI")])

(define_insn "mips_dpsu_h_qbr"
  [(set (match_operand:DI 0 "register_operand" "=a")
	(unspec:DI [(match_operand:DI 1 "register_operand" "0")
		    (match_operand:V4QI 2 "reg_or_0_operand" "dY")
		    (match_operand:V4QI 3 "reg_or_0_operand" "dY")]
		   UNSPEC_DPSU_H_QBR))]
  "TARGET_MIPS_DSP32 && !TARGET_64BIT && !TARGET_MIPS16"
  "dpsu.h.qbr\t%q0,%z2,%z3"
  [(set_attr "type"	"dspmac")
   (set_attr "mode"	"SI")])

(define_insn "mips_dpsu_h_qbl_64"
  [(set (match_operand:TI 0 "register_operand" "=a")
	(unspec:TI [(match_operand:TI 1 "register_operand" "0")
		    (match_operand:V4QI 2 "reg_or_0_operand" "dY")
		    (match_operand:V4QI 3 "reg_or_0_operand" "dY")]
		   UNSPEC_DPSU_H_QBL_64))]
  "TARGET_MIPS_DSP64 && !TARGET_MIPS16"
  "dpsu.h.qbl\t%q0,%z2,%z3"
  [(set_attr "type"	"dspmac")
   (set_attr "mode"	"DI")])

(define_insn "mips_dpsu_h_qbr_64"
  [(set (match_operand:TI 0 "register_operand" "=a")
	(unspec:TI [(match_operand:TI 1 "register_operand" "0")
		    (match_operand:V4QI 2 "reg_or_0_operand" "dY")
		    (match_operand:V4QI 3 "reg_or_0_operand" "dY")]
		   UNSPEC_DPSU_H_QBR_64))]
  "TARGET_MIPS_DSP64 && !TARGET_MIPS16"
  "dpsu.h.qbr\t%q0,%z2,%z3"
  [(set_attr "type"	"dspmac")
   (set_attr "mode"	"DI")])

; DPAQ*
(define_insn "mips_dpaq_s_w_ph"
  [(parallel
    [(set (match_operand:DI 0 "register_operand" "=a")
	  (unspec:DI [(match_operand:DI 1 "register_operand" "0")
		      (match_operand:V2HI 2 "reg_or_0_operand" "dY")
		      (match_operand:V2HI 3 "reg_or_0_operand" "dY")]
		     UNSPEC_DPAQ_S_W_PH))
     (set (reg:CCDSP CCDSP_OU_REGNUM)
	  (unspec:CCDSP [(match_dup 1) (match_dup 2) (match_dup 3)]
			UNSPEC_DPAQ_S_W_PH))])]
  "TARGET_MIPS_DSP32 && !TARGET_64BIT && !TARGET_MIPS16"
  "dpaq_s.w.ph\t%q0,%z2,%z3"
  [(set_attr "type"	"dspmac")
   (set_attr "mode"	"SI")])

(define_insn "mips_dpaq_s_w_ph_64"
  [(parallel
    [(set (match_operand:TI 0 "register_operand" "=a")
	  (unspec:TI [(match_operand:TI 1 "register_operand" "0")
		      (match_operand:V2HI 2 "reg_or_0_operand" "dY")
		      (match_operand:V2HI 3 "reg_or_0_operand" "dY")]
		     UNSPEC_DPAQ_S_W_PH_64))
     (set (reg:CCDSP CCDSP_OU_REGNUM)
	  (unspec:CCDSP [(match_dup 1) (match_dup 2) (match_dup 3)]
			UNSPEC_DPAQ_S_W_PH_64))])]
  "TARGET_MIPS_DSP64 && !TARGET_MIPS16"
  "dpaq_s.w.ph\t%q0,%z2,%z3"
  [(set_attr "type"	"dspmac")
   (set_attr "mode"	"DI")])

(define_insn "mips_dpaq_w_qh"
  [(parallel
    [(set (match_operand:TI 0 "register_operand" "=a")
	  (unspec:TI [(match_operand:TI 1 "register_operand" "0")
		      (match_operand:V4HI 2 "reg_or_0_operand" "dY")
		      (match_operand:V4HI 3 "reg_or_0_operand" "dY")]
		     UNSPEC_DPAQ_W_QH))
     (set (reg:CCDSP CCDSP_OU_REGNUM)
	  (unspec:CCDSP [(match_dup 1) (match_dup 2) (match_dup 3)]
			UNSPEC_DPAQ_W_QH))])]
  "TARGET_MIPS_DSP64 && !TARGET_MIPS16"
  "dpaq.w.qh\t%q0,%z2,%z3"
  [(set_attr "type"	"dspmac")
   (set_attr "mode"	"DI")])

(define_insn "mips_dpaq_s_w_qh"
  [(parallel
    [(set (match_operand:TI 0 "register_operand" "=a")
	  (unspec:TI [(match_operand:TI 1 "register_operand" "0")
		      (match_operand:V4HI 2 "reg_or_0_operand" "dY")
		      (match_operand:V4HI 3 "reg_or_0_operand" "dY")]
		     UNSPEC_DPAQ_S_W_QH))
     (set (reg:CCDSP CCDSP_OU_REGNUM)
	  (unspec:CCDSP [(match_dup 1) (match_dup 2) (match_dup 3)]
			UNSPEC_DPAQ_S_W_QH))])]
  "TARGET_MIPS_DSP64 && !TARGET_MIPS16"
  "dpaq_s.w.qh\t%q0,%z2,%z3"
  [(set_attr "type"	"dspmac")
   (set_attr "mode"	"DI")])

; DPSQ*
(define_insn "mips_dpsq_s_w_ph"
  [(parallel
    [(set (match_operand:DI 0 "register_operand" "=a")
	  (unspec:DI [(match_operand:DI 1 "register_operand" "0")
		      (match_operand:V2HI 2 "reg_or_0_operand" "dY")
		      (match_operand:V2HI 3 "reg_or_0_operand" "dY")]
		     UNSPEC_DPSQ_S_W_PH))
     (set (reg:CCDSP CCDSP_OU_REGNUM)
	  (unspec:CCDSP [(match_dup 1) (match_dup 2) (match_dup 3)]
			UNSPEC_DPSQ_S_W_PH))])]
  "TARGET_MIPS_DSP32 && !TARGET_64BIT && !TARGET_MIPS16"
  "dpsq_s.w.ph\t%q0,%z2,%z3"
  [(set_attr "type"	"dspmac")
   (set_attr "mode"	"SI")])

(define_insn "mips_dpsq_s_w_ph_64"
  [(parallel
    [(set (match_operand:TI 0 "register_operand" "=a")
	  (unspec:TI [(match_operand:TI 1 "register_operand" "0")
		      (match_operand:V2HI 2 "reg_or_0_operand" "dY")
		      (match_operand:V2HI 3 "reg_or_0_operand" "dY")]
		     UNSPEC_DPSQ_S_W_PH_64))
     (set (reg:CCDSP CCDSP_OU_REGNUM)
	  (unspec:CCDSP [(match_dup 1) (match_dup 2) (match_dup 3)]
			UNSPEC_DPSQ_S_W_PH_64))])]
  "TARGET_MIPS_DSP64 && !TARGET_MIPS16"
  "dpsq_s.w.ph\t%q0,%z2,%z3"
  [(set_attr "type"	"dspmac")
   (set_attr "mode"	"DI")])

(define_insn "mips_dpsq_w_qh"
  [(parallel
    [(set (match_operand:TI 0 "register_operand" "=a")
	  (unspec:TI [(match_operand:TI 1 "register_operand" "0")
		      (match_operand:V4HI 2 "reg_or_0_operand" "dY")
		      (match_operand:V4HI 3 "reg_or_0_operand" "dY")]
		     UNSPEC_DPSQ_W_QH))
     (set (reg:CCDSP CCDSP_OU_REGNUM)
	  (unspec:CCDSP [(match_dup 1) (match_dup 2) (match_dup 3)]
			UNSPEC_DPSQ_W_QH))])]
  "TARGET_MIPS_DSP64 && !TARGET_MIPS16"
  "dpsq.w.qh\t%q0,%z2,%z3"
  [(set_attr "type"	"dspmac")
   (set_attr "mode"	"DI")])

(define_insn "mips_dpsq_s_w_qh"
  [(parallel
    [(set (match_operand:TI 0 "register_operand" "=a")
	  (unspec:TI [(match_operand:TI 1 "register_operand" "0")
		      (match_operand:V4HI 2 "reg_or_0_operand" "dY")
		      (match_operand:V4HI 3 "reg_or_0_operand" "dY")]
		     UNSPEC_DPSQ_S_W_QH))
     (set (reg:CCDSP CCDSP_OU_REGNUM)
	  (unspec:CCDSP [(match_dup 1) (match_dup 2) (match_dup 3)]
			UNSPEC_DPSQ_S_W_QH))])]
  "TARGET_MIPS_DSP64 && !TARGET_MIPS16"
  "dpsq_s.w.qh\t%q0,%z2,%z3"
  [(set_attr "type"	"dspmac")
   (set_attr "mode"	"DI")])

; MULSAQ*
(define_insn "mips_mulsaq_s_w_ph"
  [(parallel
    [(set (match_operand:DI 0 "register_operand" "=a")
	  (unspec:DI [(match_operand:DI 1 "register_operand" "0")
		      (match_operand:V2HI 2 "reg_or_0_operand" "dY")
		      (match_operand:V2HI 3 "reg_or_0_operand" "dY")]
		     UNSPEC_MULSAQ_S_W_PH))
     (set (reg:CCDSP CCDSP_OU_REGNUM)
	  (unspec:CCDSP [(match_dup 1) (match_dup 2) (match_dup 3)]
			UNSPEC_MULSAQ_S_W_PH))])]
  "TARGET_MIPS_DSP32 && !TARGET_64BIT && !TARGET_MIPS16"
  "mulsaq_s.w.ph\t%q0,%z2,%z3"
  [(set_attr "type"	"dspmac")
   (set_attr "mode"	"SI")])

(define_insn "mips_mulsaq_s_w_ph_64"
  [(parallel
    [(set (match_operand:TI 0 "register_operand" "=a")
	  (unspec:TI [(match_operand:TI 1 "register_operand" "0")
		      (match_operand:V2HI 2 "reg_or_0_operand" "dY")
		      (match_operand:V2HI 3 "reg_or_0_operand" "dY")]
		     UNSPEC_MULSAQ_S_W_PH_64))
     (set (reg:CCDSP CCDSP_OU_REGNUM)
	  (unspec:CCDSP [(match_dup 1) (match_dup 2) (match_dup 3)]
			UNSPEC_MULSAQ_S_W_PH_64))])]
  "TARGET_MIPS_DSP64 && !TARGET_MIPS16"
  "mulsaq_s.w.ph\t%q0,%z2,%z3"
  [(set_attr "type"	"dspmac")
   (set_attr "mode"	"DI")])

(define_insn "mips_mulsaq_w_qh"
  [(parallel
    [(set (match_operand:TI 0 "register_operand" "=a")
	  (unspec:TI [(match_operand:TI 1 "register_operand" "0")
		      (match_operand:V4HI 2 "reg_or_0_operand" "dY")
		      (match_operand:V4HI 3 "reg_or_0_operand" "dY")]
		     UNSPEC_MULSAQ_W_QH))
     (set (reg:CCDSP CCDSP_OU_REGNUM)
	  (unspec:CCDSP [(match_dup 1) (match_dup 2) (match_dup 3)]
			UNSPEC_MULSAQ_W_QH))])]
  "TARGET_MIPS_DSP64 && !TARGET_MIPS16"
  "mulsaq.w.qh\t%q0,%z2,%z3"
  [(set_attr "type"	"dspmac")
   (set_attr "mode"	"DI")])

(define_insn "mips_mulsaq_s_w_qh"
  [(parallel
    [(set (match_operand:TI 0 "register_operand" "=a")
	  (unspec:TI [(match_operand:TI 1 "register_operand" "0")
		      (match_operand:V4HI 2 "reg_or_0_operand" "dY")
		      (match_operand:V4HI 3 "reg_or_0_operand" "dY")]
		     UNSPEC_MULSAQ_S_W_QH))
     (set (reg:CCDSP CCDSP_OU_REGNUM)
	  (unspec:CCDSP [(match_dup 1) (match_dup 2) (match_dup 3)]
			UNSPEC_MULSAQ_S_W_QH))])]
  "TARGET_MIPS_DSP64 && !TARGET_MIPS16"
  "mulsaq_s.w.qh\t%q0,%z2,%z3"
  [(set_attr "type"	"dspmac")
   (set_attr "mode"	"DI")])

(define_insn "mips_mulsaq_l_pw"
  [(parallel
    [(set (match_operand:TI 0 "register_operand" "=a")
	  (unspec:TI [(match_operand:TI 1 "register_operand" "0")
		      (match_operand:V2SI 2 "reg_or_0_operand" "dY")
		      (match_operand:V2SI 3 "reg_or_0_operand" "dY")]
		     UNSPEC_MULSAQ_L_PW))
     (set (reg:CCDSP CCDSP_OU_REGNUM)
	  (unspec:CCDSP [(match_dup 1) (match_dup 2) (match_dup 3)]
			UNSPEC_MULSAQ_L_PW))])]
  "TARGET_MIPS_DSP64 && !TARGET_MIPS16"
  "mulsaq.l.pw\t%q0,%z2,%z3"
  [(set_attr "type"	"dspmac")
   (set_attr "mode"	"DI")])

(define_insn "mips_mulsaq_s_l_pw"
  [(parallel
    [(set (match_operand:TI 0 "register_operand" "=a")
	  (unspec:TI [(match_operand:TI 1 "register_operand" "0")
		      (match_operand:V2SI 2 "reg_or_0_operand" "dY")
		      (match_operand:V2SI 3 "reg_or_0_operand" "dY")]
		     UNSPEC_MULSAQ_S_L_PW))
     (set (reg:CCDSP CCDSP_OU_REGNUM)
	  (unspec:CCDSP [(match_dup 1) (match_dup 2) (match_dup 3)]
			UNSPEC_MULSAQ_S_L_PW))])]
  "TARGET_MIPS_DSP64 && !TARGET_MIPS16"
  "mulsaq_s.l.pw\t%q0,%z2,%z3"
  [(set_attr "type"	"dspmac")
   (set_attr "mode"	"DI")])

; DPAQ*
(define_insn "mips_dpaq_sa_l_w"
  [(parallel
    [(set (match_operand:DI 0 "register_operand" "=a")
	  (unspec:DI [(match_operand:DI 1 "register_operand" "0")
		      (match_operand:SI 2 "reg_or_0_operand" "dJ")
		      (match_operand:SI 3 "reg_or_0_operand" "dJ")]
		     UNSPEC_DPAQ_SA_L_W))
     (set (reg:CCDSP CCDSP_OU_REGNUM)
	  (unspec:CCDSP [(match_dup 1) (match_dup 2) (match_dup 3)]
			UNSPEC_DPAQ_SA_L_W))])]
  "TARGET_MIPS_DSP32 && !TARGET_64BIT && !TARGET_MIPS16"
  "dpaq_sa.l.w\t%q0,%z2,%z3"
  [(set_attr "type"	"dspmacsat")
   (set_attr "mode"	"SI")])

(define_insn "mips_dpaq_sa_l_w_64"
  [(parallel
    [(set (match_operand:TI 0 "register_operand" "=a")
	  (unspec:TI [(match_operand:TI 1 "register_operand" "0")
		      (match_operand:SI 2 "reg_or_0_operand" "dJ")
		      (match_operand:SI 3 "reg_or_0_operand" "dJ")]
		     UNSPEC_DPAQ_SA_L_W_64))
     (set (reg:CCDSP CCDSP_OU_REGNUM)
	  (unspec:CCDSP [(match_dup 1) (match_dup 2) (match_dup 3)]
			UNSPEC_DPAQ_SA_L_W_64))])]
  "TARGET_MIPS_DSP64 && !TARGET_MIPS16"
  "dpaq_sa.l.w\t%q0,%z2,%z3"
  [(set_attr "type"	"dspmacsat")
   (set_attr "mode"	"DI")])

(define_insn "mips_dpaq_l_pw"
  [(parallel
    [(set (match_operand:TI 0 "register_operand" "=a")
	  (unspec:TI [(match_operand:TI 1 "register_operand" "0")
		      (match_operand:V2SI 2 "reg_or_0_operand" "dY")
		      (match_operand:V2SI 3 "reg_or_0_operand" "dY")]
		     UNSPEC_DPAQ_L_PW))
     (set (reg:CCDSP CCDSP_OU_REGNUM)
	  (unspec:CCDSP [(match_dup 1) (match_dup 2) (match_dup 3)]
			UNSPEC_DPAQ_L_PW))])]
  "TARGET_MIPS_DSP64 && !TARGET_MIPS16"
  "dpaq.l.pw\t%q0,%z2,%z3"
  [(set_attr "type"	"dspmac")
   (set_attr "mode"	"DI")])

(define_insn "mips_dpaq_sa_l_pw"
  [(parallel
    [(set (match_operand:TI 0 "register_operand" "=a")
	  (unspec:TI [(match_operand:TI 1 "register_operand" "0")
		      (match_operand:V2SI 2 "reg_or_0_operand" "dY")
		      (match_operand:V2SI 3 "reg_or_0_operand" "dY")]
		     UNSPEC_DPAQ_SA_L_PW))
     (set (reg:CCDSP CCDSP_OU_REGNUM)
	  (unspec:CCDSP [(match_dup 1) (match_dup 2) (match_dup 3)]
			UNSPEC_DPAQ_SA_L_PW))])]
  "TARGET_MIPS_DSP64 && !TARGET_MIPS16"
  "dpaq_sa.l.pw\t%q0,%z2,%z3"
  [(set_attr "type"	"dspmacsat")
   (set_attr "mode"	"DI")])

; DPSQ*
(define_insn "mips_dpsq_sa_l_w"
  [(parallel
    [(set (match_operand:DI 0 "register_operand" "=a")
	  (unspec:DI [(match_operand:DI 1 "register_operand" "0")
		      (match_operand:SI 2 "reg_or_0_operand" "dJ")
		      (match_operand:SI 3 "reg_or_0_operand" "dJ")]
		     UNSPEC_DPSQ_SA_L_W))
     (set (reg:CCDSP CCDSP_OU_REGNUM)
	  (unspec:CCDSP [(match_dup 1) (match_dup 2) (match_dup 3)]
			UNSPEC_DPSQ_SA_L_W))])]
  "TARGET_MIPS_DSP32 && !TARGET_64BIT && !TARGET_MIPS16"
  "dpsq_sa.l.w\t%q0,%z2,%z3"
  [(set_attr "type"	"dspmacsat")
   (set_attr "mode"	"SI")])

(define_insn "mips_dpsq_sa_l_w_64"
  [(parallel
    [(set (match_operand:TI 0 "register_operand" "=a")
	  (unspec:TI [(match_operand:TI 1 "register_operand" "0")
		      (match_operand:SI 2 "reg_or_0_operand" "dJ")
		      (match_operand:SI 3 "reg_or_0_operand" "dJ")]
		     UNSPEC_DPSQ_SA_L_W_64))
     (set (reg:CCDSP CCDSP_OU_REGNUM)
	  (unspec:CCDSP [(match_dup 1) (match_dup 2) (match_dup 3)]
			UNSPEC_DPSQ_SA_L_W_64))])]
  "TARGET_MIPS_DSP64 && !TARGET_MIPS16"
  "dpsq_sa.l.w\t%q0,%z2,%z3"
  [(set_attr "type"	"dspmacsat")
   (set_attr "mode"	"DI")])

(define_insn "mips_dpsq_l_pw"
  [(parallel
    [(set (match_operand:TI 0 "register_operand" "=a")
	  (unspec:TI [(match_operand:TI 1 "register_operand" "0")
		      (match_operand:V2SI 2 "reg_or_0_operand" "dY")
		      (match_operand:V2SI 3 "reg_or_0_operand" "dY")]
		     UNSPEC_DPSQ_L_PW))
     (set (reg:CCDSP CCDSP_OU_REGNUM)
	  (unspec:CCDSP [(match_dup 1) (match_dup 2) (match_dup 3)]
			UNSPEC_DPSQ_L_PW))])]
  "TARGET_MIPS_DSP64 && !TARGET_MIPS16"
  "dpsq.l.pw\t%q0,%z2,%z3"
  [(set_attr "type"	"dspmac")
   (set_attr "mode"	"DI")])

(define_insn "mips_dpsq_sa_l_pw"
  [(parallel
    [(set (match_operand:TI 0 "register_operand" "=a")
	  (unspec:TI [(match_operand:TI 1 "register_operand" "0")
		      (match_operand:V2SI 2 "reg_or_0_operand" "dY")
		      (match_operand:V2SI 3 "reg_or_0_operand" "dY")]
		     UNSPEC_DPSQ_SA_L_PW))
     (set (reg:CCDSP CCDSP_OU_REGNUM)
	  (unspec:CCDSP [(match_dup 1) (match_dup 2) (match_dup 3)]
			UNSPEC_DPSQ_SA_L_PW))])]
  "TARGET_MIPS_DSP64 && !TARGET_MIPS16"
  "dpsq_sa.l.pw\t%q0,%z2,%z3"
  [(set_attr "type"	"dspmacsat")
   (set_attr "mode"	"DI")])

; MAQ*
(define_insn "mips_maq_s_w_phl"
  [(parallel
    [(set (match_operand:DI 0 "register_operand" "=a")
	  (unspec:DI [(match_operand:DI 1 "register_operand" "0")
		      (match_operand:V2HI 2 "reg_or_0_operand" "dY")
		      (match_operand:V2HI 3 "reg_or_0_operand" "dY")]
		     UNSPEC_MAQ_S_W_PHL))
     (set (reg:CCDSP CCDSP_OU_REGNUM)
	  (unspec:CCDSP [(match_dup 1) (match_dup 2) (match_dup 3)]
			UNSPEC_MAQ_S_W_PHL))])]
  "TARGET_MIPS_DSP32 && !TARGET_64BIT && !TARGET_MIPS16"
  "maq_s.w.phl\t%q0,%z2,%z3"
  [(set_attr "type"	"dspmac")
   (set_attr "mode"	"SI")])

(define_insn "mips_maq_s_w_phr"
  [(parallel
    [(set (match_operand:DI 0 "register_operand" "=a")
	  (unspec:DI [(match_operand:DI 1 "register_operand" "0")
		      (match_operand:V2HI 2 "reg_or_0_operand" "dY")
		      (match_operand:V2HI 3 "reg_or_0_operand" "dY")]
		     UNSPEC_MAQ_S_W_PHR))
     (set (reg:CCDSP CCDSP_OU_REGNUM)
	  (unspec:CCDSP [(match_dup 1) (match_dup 2) (match_dup 3)]
			UNSPEC_MAQ_S_W_PHR))])]
  "TARGET_MIPS_DSP32 && !TARGET_64BIT && !TARGET_MIPS16"
  "maq_s.w.phr\t%q0,%z2,%z3"
  [(set_attr "type"	"dspmac")
   (set_attr "mode"	"SI")])

(define_insn "mips_maq_s_w_phl_64"
  [(parallel
    [(set (match_operand:TI 0 "register_operand" "=a")
	  (unspec:TI [(match_operand:TI 1 "register_operand" "0")
		      (match_operand:V2HI 2 "reg_or_0_operand" "dY")
		      (match_operand:V2HI 3 "reg_or_0_operand" "dY")]
		     UNSPEC_MAQ_S_W_PHL_64))
     (set (reg:CCDSP CCDSP_OU_REGNUM)
	  (unspec:CCDSP [(match_dup 1) (match_dup 2) (match_dup 3)]
			UNSPEC_MAQ_S_W_PHL_64))])]
  "TARGET_MIPS_DSP64 && !TARGET_MIPS16"
  "maq_s.w.phl\t%q0,%z2,%z3"
  [(set_attr "type"	"dspmac")
   (set_attr "mode"	"DI")])

(define_insn "mips_maq_s_w_phr_64"
  [(parallel
    [(set (match_operand:TI 0 "register_operand" "=a")
	  (unspec:TI [(match_operand:TI 1 "register_operand" "0")
		      (match_operand:V2HI 2 "reg_or_0_operand" "dY")
		      (match_operand:V2HI 3 "reg_or_0_operand" "dY")]
		     UNSPEC_MAQ_S_W_PHR_64))
     (set (reg:CCDSP CCDSP_OU_REGNUM)
	  (unspec:CCDSP [(match_dup 1) (match_dup 2) (match_dup 3)]
			UNSPEC_MAQ_S_W_PHR_64))])]
  "TARGET_MIPS_DSP64 && !TARGET_MIPS16"
  "maq_s.w.phr\t%q0,%z2,%z3"
  [(set_attr "type"	"dspmac")
   (set_attr "mode"	"DI")])

(define_insn "mips_maq_w_qhll"
  [(parallel
    [(set (match_operand:TI 0 "register_operand" "=a")
	  (unspec:TI [(match_operand:TI 1 "register_operand" "0")
		      (match_operand:V4HI 2 "reg_or_0_operand" "dY")
		      (match_operand:V4HI 3 "reg_or_0_operand" "dY")]
		     UNSPEC_MAQ_W_QHLL))
     (set (reg:CCDSP CCDSP_OU_REGNUM)
	  (unspec:CCDSP [(match_dup 1) (match_dup 2) (match_dup 3)]
			UNSPEC_MAQ_W_QHLL))])]
  "TARGET_MIPS_DSP64 && !TARGET_MIPS16"
  "maq.w.qhll\t%q0,%z2,%z3"
  [(set_attr "type"	"dspmac")
   (set_attr "mode"	"DI")])

(define_insn "mips_maq_w_qhlr"
  [(parallel
    [(set (match_operand:TI 0 "register_operand" "=a")
	  (unspec:TI [(match_operand:TI 1 "register_operand" "0")
		      (match_operand:V4HI 2 "reg_or_0_operand" "dY")
		      (match_operand:V4HI 3 "reg_or_0_operand" "dY")]
		     UNSPEC_MAQ_W_QHLR))
     (set (reg:CCDSP CCDSP_OU_REGNUM)
	  (unspec:CCDSP [(match_dup 1) (match_dup 2) (match_dup 3)]
			UNSPEC_MAQ_W_QHLR))])]
  "TARGET_MIPS_DSP64 && !TARGET_MIPS16"
  "maq.w.qhlr\t%q0,%z2,%z3"
  [(set_attr "type"	"dspmac")
   (set_attr "mode"	"DI")])

(define_insn "mips_maq_w_qhrl"
  [(parallel
    [(set (match_operand:TI 0 "register_operand" "=a")
	  (unspec:TI [(match_operand:TI 1 "register_operand" "0")
		      (match_operand:V4HI 2 "reg_or_0_operand" "dY")
		      (match_operand:V4HI 3 "reg_or_0_operand" "dY")]
		     UNSPEC_MAQ_W_QHRL))
     (set (reg:CCDSP CCDSP_OU_REGNUM)
	  (unspec:CCDSP [(match_dup 1) (match_dup 2) (match_dup 3)]
			UNSPEC_MAQ_W_QHRL))])]
  "TARGET_MIPS_DSP64 && !TARGET_MIPS16"
  "maq.w.qhrl\t%q0,%z2,%z3"
  [(set_attr "type"	"dspmac")
   (set_attr "mode"	"DI")])

(define_insn "mips_maq_w_qhrr"
  [(parallel
    [(set (match_operand:TI 0 "register_operand" "=a")
	  (unspec:TI [(match_operand:TI 1 "register_operand" "0")
		      (match_operand:V4HI 2 "reg_or_0_operand" "dY")
		      (match_operand:V4HI 3 "reg_or_0_operand" "dY")]
		     UNSPEC_MAQ_W_QHRR))
     (set (reg:CCDSP CCDSP_OU_REGNUM)
	  (unspec:CCDSP [(match_dup 1) (match_dup 2) (match_dup 3)]
			UNSPEC_MAQ_W_QHRR))])]
  "TARGET_MIPS_DSP64 && !TARGET_MIPS16"
  "maq.w.qhrr\t%q0,%z2,%z3"
  [(set_attr "type"	"dspmac")
   (set_attr "mode"	"DI")])

(define_insn "mips_maq_s_w_qhll"
  [(parallel
    [(set (match_operand:TI 0 "register_operand" "=a")
	  (unspec:TI [(match_operand:TI 1 "register_operand" "0")
		      (match_operand:V4HI 2 "reg_or_0_operand" "dY")
		      (match_operand:V4HI 3 "reg_or_0_operand" "dY")]
		     UNSPEC_MAQ_S_W_QHLL))
     (set (reg:CCDSP CCDSP_OU_REGNUM)
	  (unspec:CCDSP [(match_dup 1) (match_dup 2) (match_dup 3)]
			UNSPEC_MAQ_S_W_QHLL))])]
  "TARGET_MIPS_DSP64 && !TARGET_MIPS16"
  "maq_s.w.qhll\t%q0,%z2,%z3"
  [(set_attr "type"	"dspmac")
   (set_attr "mode"	"DI")])

(define_insn "mips_maq_s_w_qhlr"
  [(parallel
    [(set (match_operand:TI 0 "register_operand" "=a")
	  (unspec:TI [(match_operand:TI 1 "register_operand" "0")
		      (match_operand:V4HI 2 "reg_or_0_operand" "dY")
		      (match_operand:V4HI 3 "reg_or_0_operand" "dY")]
		     UNSPEC_MAQ_S_W_QHLR))
     (set (reg:CCDSP CCDSP_OU_REGNUM)
	  (unspec:CCDSP [(match_dup 1) (match_dup 2) (match_dup 3)]
			UNSPEC_MAQ_S_W_QHLR))])]
  "TARGET_MIPS_DSP64 && !TARGET_MIPS16"
  "maq_s.w.qhlr\t%q0,%z2,%z3"
  [(set_attr "type"	"dspmac")
   (set_attr "mode"	"DI")])

(define_insn "mips_maq_s_w_qhrl"
  [(parallel
    [(set (match_operand:TI 0 "register_operand" "=a")
	  (unspec:TI [(match_operand:TI 1 "register_operand" "0")
		      (match_operand:V4HI 2 "reg_or_0_operand" "dY")
		      (match_operand:V4HI 3 "reg_or_0_operand" "dY")]
		     UNSPEC_MAQ_S_W_QHRL))
     (set (reg:CCDSP CCDSP_OU_REGNUM)
	  (unspec:CCDSP [(match_dup 1) (match_dup 2) (match_dup 3)]
			UNSPEC_MAQ_S_W_QHRL))])]
  "TARGET_MIPS_DSP64 && !TARGET_MIPS16"
  "maq_s.w.qhrl\t%q0,%z2,%z3"
  [(set_attr "type"	"dspmac")
   (set_attr "mode"	"DI")])

(define_insn "mips_maq_s_w_qhrr"
  [(parallel
    [(set (match_operand:TI 0 "register_operand" "=a")
	  (unspec:TI [(match_operand:TI 1 "register_operand" "0")
		      (match_operand:V4HI 2 "reg_or_0_operand" "dY")
		      (match_operand:V4HI 3 "reg_or_0_operand" "dY")]
		     UNSPEC_MAQ_S_W_QHRR))
     (set (reg:CCDSP CCDSP_OU_REGNUM)
	  (unspec:CCDSP [(match_dup 1) (match_dup 2) (match_dup 3)]
			UNSPEC_MAQ_S_W_QHRR))])]
  "TARGET_MIPS_DSP64 && !TARGET_MIPS16"
  "maq_s.w.qhrr\t%q0,%z2,%z3"
  [(set_attr "type"	"dspmac")
   (set_attr "mode"	"DI")])

(define_insn "mips_maq_l_pwl"
  [(parallel
    [(set (match_operand:TI 0 "register_operand" "=a")
	  (unspec:TI [(match_operand:TI 1 "register_operand" "0")
		      (match_operand:V2SI 2 "reg_or_0_operand" "dY")
		      (match_operand:V2SI 3 "reg_or_0_operand" "dY")]
		     UNSPEC_MAQ_L_PWL))
     (set (reg:CCDSP CCDSP_OU_REGNUM)
	  (unspec:CCDSP [(match_dup 1) (match_dup 2) (match_dup 3)]
			UNSPEC_MAQ_L_PWL))])]
  "TARGET_MIPS_DSP64 && !TARGET_MIPS16"
  "maq.l.pwl\t%q0,%z2,%z3"
  [(set_attr "type"	"dspmac")
   (set_attr "mode"	"DI")])

(define_insn "mips_maq_l_pwr"
  [(parallel
    [(set (match_operand:TI 0 "register_operand" "=a")
	  (unspec:TI [(match_operand:TI 1 "register_operand" "0")
		      (match_operand:V2SI 2 "reg_or_0_operand" "dY")
		      (match_operand:V2SI 3 "reg_or_0_operand" "dY")]
		     UNSPEC_MAQ_L_PWR))
     (set (reg:CCDSP CCDSP_OU_REGNUM)
	  (unspec:CCDSP [(match_dup 1) (match_dup 2) (match_dup 3)]
			UNSPEC_MAQ_L_PWR))])]
  "TARGET_MIPS_DSP64 && !TARGET_MIPS16"
  "maq.l.pwr\t%q0,%z2,%z3"
  [(set_attr "type"	"dspmac")
   (set_attr "mode"	"DI")])

(define_insn "mips_maq_s_l_pwl"
  [(parallel
    [(set (match_operand:TI 0 "register_operand" "=a")
	  (unspec:TI [(match_operand:TI 1 "register_operand" "0")
		      (match_operand:V2SI 2 "reg_or_0_operand" "dY")
		      (match_operand:V2SI 3 "reg_or_0_operand" "dY")]
		     UNSPEC_MAQ_S_L_PWL))
     (set (reg:CCDSP CCDSP_OU_REGNUM)
	  (unspec:CCDSP [(match_dup 1) (match_dup 2) (match_dup 3)]
			UNSPEC_MAQ_S_L_PWL))])]
  "TARGET_MIPS_DSP64 && !TARGET_MIPS16"
  "maq_s.l.pwl\t%q0,%z2,%z3"
  [(set_attr "type"	"dspmac")
   (set_attr "mode"	"DI")])

(define_insn "mips_maq_s_l_pwr"
  [(parallel
    [(set (match_operand:TI 0 "register_operand" "=a")
	  (unspec:TI [(match_operand:TI 1 "register_operand" "0")
		      (match_operand:V2SI 2 "reg_or_0_operand" "dY")
		      (match_operand:V2SI 3 "reg_or_0_operand" "dY")]
		     UNSPEC_MAQ_S_L_PWR))
     (set (reg:CCDSP CCDSP_OU_REGNUM)
	  (unspec:CCDSP [(match_dup 1) (match_dup 2) (match_dup 3)]
			UNSPEC_MAQ_S_L_PWR))])]
  "TARGET_MIPS_DSP64 && !TARGET_MIPS16"
  "maq_s.l.pwr\t%q0,%z2,%z3"
  [(set_attr "type"	"dspmac")
   (set_attr "mode"	"DI")])

; MAQ_SA*
(define_insn "mips_maq_sa_w_phl"
  [(parallel
    [(set (match_operand:DI 0 "register_operand" "=a")
	  (unspec:DI [(match_operand:DI 1 "register_operand" "0")
		      (match_operand:V2HI 2 "reg_or_0_operand" "dY")
		      (match_operand:V2HI 3 "reg_or_0_operand" "dY")]
		     UNSPEC_MAQ_SA_W_PHL))
     (set (reg:CCDSP CCDSP_OU_REGNUM)
	  (unspec:CCDSP [(match_dup 1) (match_dup 2) (match_dup 3)]
			UNSPEC_MAQ_SA_W_PHL))])]
  "TARGET_MIPS_DSP32 && !TARGET_64BIT && !TARGET_MIPS16"
  "maq_sa.w.phl\t%q0,%z2,%z3"
  [(set_attr "type"	"dspmacsat")
   (set_attr "mode"	"SI")])

(define_insn "mips_maq_sa_w_phr"
  [(parallel
    [(set (match_operand:DI 0 "register_operand" "=a")
	  (unspec:DI [(match_operand:DI 1 "register_operand" "0")
		      (match_operand:V2HI 2 "reg_or_0_operand" "dY")
		      (match_operand:V2HI 3 "reg_or_0_operand" "dY")]
		     UNSPEC_MAQ_SA_W_PHR))
     (set (reg:CCDSP CCDSP_OU_REGNUM)
	  (unspec:CCDSP [(match_dup 1) (match_dup 2) (match_dup 3)]
			UNSPEC_MAQ_SA_W_PHR))])]
  "TARGET_MIPS_DSP32 && !TARGET_64BIT && !TARGET_MIPS16"
  "maq_sa.w.phr\t%q0,%z2,%z3"
  [(set_attr "type"	"dspmacsat")
   (set_attr "mode"	"SI")])

(define_insn "mips_maq_sa_w_phl_64"
  [(parallel
    [(set (match_operand:TI 0 "register_operand" "=a")
	  (unspec:TI [(match_operand:TI 1 "register_operand" "0")
		      (match_operand:V2HI 2 "reg_or_0_operand" "dY")
		      (match_operand:V2HI 3 "reg_or_0_operand" "dY")]
		     UNSPEC_MAQ_SA_W_PHL_64))
     (set (reg:CCDSP CCDSP_OU_REGNUM)
	  (unspec:CCDSP [(match_dup 1) (match_dup 2) (match_dup 3)]
			UNSPEC_MAQ_SA_W_PHL_64))])]
  "TARGET_MIPS_DSP64 && !TARGET_MIPS16"
  "maq_sa.w.phl\t%q0,%z2,%z3"
  [(set_attr "type"	"dspmacsat")
   (set_attr "mode"	"DI")])

(define_insn "mips_maq_sa_w_phr_64"
  [(parallel
    [(set (match_operand:TI 0 "register_operand" "=a")
	  (unspec:TI [(match_operand:TI 1 "register_operand" "0")
		      (match_operand:V2HI 2 "reg_or_0_operand" "dY")
		      (match_operand:V2HI 3 "reg_or_0_operand" "dY")]
		     UNSPEC_MAQ_SA_W_PHR_64))
     (set (reg:CCDSP CCDSP_OU_REGNUM)
	  (unspec:CCDSP [(match_dup 1) (match_dup 2) (match_dup 3)]
			UNSPEC_MAQ_SA_W_PHR_64))])]
  "TARGET_MIPS_DSP64 && !TARGET_MIPS16"
  "maq_sa.w.phr\t%q0,%z2,%z3"
  [(set_attr "type"	"dspmacsat")
   (set_attr "mode"	"DI")])

; DMADD*
(define_insn "mips_dmadd"
  [(set (match_operand:TI 0 "register_operand" "=a")
	(unspec:TI [(match_operand:TI 1 "register_operand" "0")
		    (match_operand:DI 2 "reg_or_0_operand" "dJ")
		    (match_operand:DI 3 "reg_or_0_operand" "dJ")]
		   UNSPEC_DMADD))]
  "TARGET_MIPS_DSP64 && !TARGET_MIPS16"
  "dmadd\t%q0,%z2,%z3"
  [(set_attr "type"	"imadd")
   (set_attr "mode"	"DI")])

(define_insn "mips_dmaddu"
  [(set (match_operand:TI 0 "register_operand" "=a")
	(unspec:TI [(match_operand:TI 1 "register_operand" "0")
		    (match_operand:DI 2 "reg_or_0_operand" "dJ")
		    (match_operand:DI 3 "reg_or_0_operand" "dJ")]
		   UNSPEC_DMADDU))]
  "TARGET_MIPS_DSP64 && !TARGET_MIPS16"
  "dmaddu\t%q0,%z2,%z3"
  [(set_attr "type"	"imadd")
   (set_attr "mode"	"DI")])

; DMSUB*
(define_insn "mips_dmsub"
  [(set (match_operand:TI 0 "register_operand" "=a")
	(unspec:TI [(match_operand:TI 1 "register_operand" "0")
		    (match_operand:DI 2 "reg_or_0_operand" "dJ")
		    (match_operand:DI 3 "reg_or_0_operand" "dJ")]
		   UNSPEC_DMSUB))]
  "TARGET_MIPS_DSP64 && !TARGET_MIPS16"
  "dmsub\t%q0,%z2,%z3"
  [(set_attr "type"	"imadd")
   (set_attr "mode"	"DI")])

(define_insn "mips_dmsubu"
  [(set (match_operand:TI 0 "register_operand" "=a")
	(unspec:TI [(match_operand:TI 1 "register_operand" "0")
		    (match_operand:DI 2 "reg_or_0_operand" "dJ")
		    (match_operand:DI 3 "reg_or_0_operand" "dJ")]
		   UNSPEC_DMSUBU))]
  "TARGET_MIPS_DSP64 && !TARGET_MIPS16"
  "dmsubu\t%q0,%z2,%z3"
  [(set_attr "type"	"imadd")
   (set_attr "mode"	"DI")])

; Table 2-4. MIPS DSP ASE Instructions: General Bit/Manipulation
; BITREV
(define_insn "mips_bitrev"
  [(set (match_operand:SI 0 "register_operand" "=d")
	(unspec:SI [(match_operand:SI 1 "reg_or_0_operand" "dJ")]
		   UNSPEC_BITREV))]
  "TARGET_MIPS_DSP32 && !TARGET_MIPS16"
  "bitrev\t%0,%z1"
  [(set_attr "type"	"dspalu")
   (set_attr "mode"	"SI")])

; INSV
(define_insn "mips_insv"
  [(set (match_operand:SI 0 "register_operand" "=d")
	(unspec:SI [(match_operand:SI 1 "register_operand" "0")
		    (match_operand:SI 2 "reg_or_0_operand" "dJ")
		    (reg:CCDSP CCDSP_SC_REGNUM)
		    (reg:CCDSP CCDSP_PO_REGNUM)]
		   UNSPEC_INSV))]
  "TARGET_MIPS_DSP32 && !TARGET_MIPS16"
  "insv\t%0,%z2"
  [(set_attr "type"	"dspalu")
   (set_attr "mode"	"SI")])

(define_insn "mips_dinsv"
  [(set (match_operand:DI 0 "register_operand" "=d")
	(unspec:DI [(match_operand:DI 1 "register_operand" "0")
		    (match_operand:DI 2 "reg_or_0_operand" "dJ")
		    (reg:CCDSP CCDSP_SC_REGNUM)
		    (reg:CCDSP CCDSP_PO_REGNUM)]
		   UNSPEC_DINSV))]
  "TARGET_MIPS_DSP64 && !TARGET_MIPS16"
  "dinsv\t%0,%z2"
  [(set_attr "type"	"dspalu")
   (set_attr "mode"	"DI")])

; REPL*
(define_insn "mips_repl_qb"
  [(set (match_operand:V4QI 0 "register_operand" "=d,d")
	(unspec:V4QI [(match_operand:SI 1 "arith_operand" "I,d")]
		     UNSPEC_REPL_QB))]
  "TARGET_MIPS_DSP32 && !TARGET_MIPS16"
  "@
   repl.qb\t%0,%1
   replv.qb\t%0,%1"
  [(set_attr "type"	"dspalu")
   (set_attr "mode"	"SI")])

(define_insn "mips_repl_ob"
  [(set (match_operand:V8QI 0 "register_operand" "=d,d")
	(unspec:V8QI [(match_operand:DI 1 "arith_operand" "I,d")]
		     UNSPEC_REPL_OB))]
  "TARGET_MIPS_DSP64 && !TARGET_MIPS16"
  "@
   repl.ob\t%0,%1
   replv.ob\t%0,%1"
  [(set_attr "type"	"dspalu")
   (set_attr "mode"	"DI")])

(define_insn "mips_repl_ph"
  [(set (match_operand:V2HI 0 "register_operand" "=d,d")
	(unspec:V2HI [(match_operand:SI 1 "reg_simm10_operand" "I,d")]
		     UNSPEC_REPL_PH))]
  "TARGET_MIPS_DSP32 && !TARGET_MIPS16"
  "@
   repl.ph\t%0,%1
   replv.ph\t%0,%1"
  [(set_attr "type"	"dspalu")
   (set_attr "mode"	"SI")])

(define_insn "mips_repl_qh"
  [(set (match_operand:V4HI 0 "register_operand" "=d,d")
	(unspec:V4HI [(match_operand:DI 1 "reg_simm10_operand" "I,d")]
		     UNSPEC_REPL_QH))]
  "TARGET_MIPS_DSP64 && !TARGET_MIPS16"
  "@
   repl.qh\t%0,%1
   replv.qh\t%0,%1"
  [(set_attr "type"	"dspalu")
   (set_attr "mode"	"DI")])

(define_insn "mips_repl_pw"
  [(set (match_operand:V2SI 0 "register_operand" "=d,d")
	(unspec:V2SI [(match_operand:DI 1 "reg_simm10_operand" "I,d")]
		     UNSPEC_REPL_PW))]
  "TARGET_MIPS_DSP64 && !TARGET_MIPS16"
  "@
   repl.pw\t%0,%1
   replv.pw\t%0,%1"
  [(set_attr "type"	"dspalu")
   (set_attr "mode"	"DI")])

; Table 2-5. MIPS DSP ASE Instructions: Compare-Pick
; CMPU.* CMP.*
(define_insn "mips_cmpu_eq_qb"
  [(set (reg:CCDSP CCDSP_CC_REGNUM)
	(unspec:CCDSP [(match_operand:V4QI 0 "reg_or_0_operand" "dY")
		       (match_operand:V4QI 1 "reg_or_0_operand" "dY")
		       (reg:CCDSP CCDSP_CC_REGNUM)]
		      UNSPEC_CMPU_EQ_QB))]
  "TARGET_MIPS_DSP32 && !TARGET_MIPS16"
  "cmpu.eq.qb\t%z0,%z1"
  [(set_attr "type"	"dspalu")
   (set_attr "mode"	"SI")])

(define_insn "mips_cmpu_lt_qb"
  [(set (reg:CCDSP CCDSP_CC_REGNUM)
	(unspec:CCDSP [(match_operand:V4QI 0 "reg_or_0_operand" "dY")
		       (match_operand:V4QI 1 "reg_or_0_operand" "dY")
		       (reg:CCDSP CCDSP_CC_REGNUM)]
		      UNSPEC_CMPU_LT_QB))]
  "TARGET_MIPS_DSP32 && !TARGET_MIPS16"
  "cmpu.lt.qb\t%z0,%z1"
  [(set_attr "type"	"dspalu")
   (set_attr "mode"	"SI")])

(define_insn "mips_cmpu_le_qb"
  [(set (reg:CCDSP CCDSP_CC_REGNUM)
	(unspec:CCDSP [(match_operand:V4QI 0 "reg_or_0_operand" "dY")
		       (match_operand:V4QI 1 "reg_or_0_operand" "dY")
		       (reg:CCDSP CCDSP_CC_REGNUM)]
		      UNSPEC_CMPU_LE_QB))]
  "TARGET_MIPS_DSP32 && !TARGET_MIPS16"
  "cmpu.le.qb\t%z0,%z1"
  [(set_attr "type"	"dspalu")
   (set_attr "mode"	"SI")])

(define_insn "mips_cmpgu_eq_qb"
  [(set (match_operand:SI 0 "register_operand" "=d")
	(unspec:SI [(match_operand:V4QI 1 "reg_or_0_operand" "dY")
		    (match_operand:V4QI 2 "reg_or_0_operand" "dY")]
		   UNSPEC_CMPGU_EQ_QB))]
  "TARGET_MIPS_DSP32 && !TARGET_MIPS16"
  "cmpgu.eq.qb\t%0,%z1,%z2"
  [(set_attr "type"	"dspalu")
   (set_attr "mode"	"SI")])

(define_insn "mips_cmpgu_lt_qb"
  [(set (match_operand:SI 0 "register_operand" "=d")
	(unspec:SI [(match_operand:V4QI 1 "reg_or_0_operand" "dY")
		    (match_operand:V4QI 2 "reg_or_0_operand" "dY")]
		   UNSPEC_CMPGU_LT_QB))]
  "TARGET_MIPS_DSP32 && !TARGET_MIPS16"
  "cmpgu.lt.qb\t%0,%z1,%z2"
  [(set_attr "type"	"dspalu")
   (set_attr "mode"	"SI")])

(define_insn "mips_cmpgu_le_qb"
  [(set (match_operand:SI 0 "register_operand" "=d")
	(unspec:SI [(match_operand:V4QI 1 "reg_or_0_operand" "dY")
		    (match_operand:V4QI 2 "reg_or_0_operand" "dY")]
		   UNSPEC_CMPGU_LE_QB))]
  "TARGET_MIPS_DSP32 && !TARGET_MIPS16"
  "cmpgu.le.qb\t%0,%z1,%z2"
  [(set_attr "type"	"dspalu")
   (set_attr "mode"	"SI")])

(define_insn "mips_cmp_eq_ph"
  [(set (reg:CCDSP CCDSP_CC_REGNUM)
	(unspec:CCDSP [(match_operand:V2HI 0 "reg_or_0_operand" "dY")
		       (match_operand:V2HI 1 "reg_or_0_operand" "dY")
		       (reg:CCDSP CCDSP_CC_REGNUM)]
		      UNSPEC_CMP_EQ_PH))]
  "TARGET_MIPS_DSP32 && !TARGET_MIPS16"
  "cmp.eq.ph\t%z0,%z1"
  [(set_attr "type"	"dspalu")
   (set_attr "mode"	"SI")])

(define_insn "mips_cmp_lt_ph"
  [(set (reg:CCDSP CCDSP_CC_REGNUM)
	(unspec:CCDSP [(match_operand:V2HI 0 "reg_or_0_operand" "dY")
		       (match_operand:V2HI 1 "reg_or_0_operand" "dY")
		       (reg:CCDSP CCDSP_CC_REGNUM)]
		      UNSPEC_CMP_LT_PH))]
  "TARGET_MIPS_DSP32 && !TARGET_MIPS16"
  "cmp.lt.ph\t%z0,%z1"
  [(set_attr "type"	"dspalu")
   (set_attr "mode"	"SI")])

(define_insn "mips_cmp_le_ph"
  [(set (reg:CCDSP CCDSP_CC_REGNUM)
	(unspec:CCDSP [(match_operand:V2HI 0 "reg_or_0_operand" "dY")
		       (match_operand:V2HI 1 "reg_or_0_operand" "dY")
		       (reg:CCDSP CCDSP_CC_REGNUM)]
		      UNSPEC_CMP_LE_PH))]
  "TARGET_MIPS_DSP32 && !TARGET_MIPS16"
  "cmp.le.ph\t%z0,%z1"
  [(set_attr "type"	"dspalu")
   (set_attr "mode"	"SI")])

(define_insn "mips_cmpu_eq_ob"
  [(set (reg:CCDSP CCDSP_CC_REGNUM)
	(unspec:CCDSP [(match_operand:V8QI 0 "reg_or_0_operand" "dY")
		       (match_operand:V8QI 1 "reg_or_0_operand" "dY")
		       (reg:CCDSP CCDSP_CC_REGNUM)]
		      UNSPEC_CMPU_EQ_OB))]
  "TARGET_MIPS_DSP64 && !TARGET_MIPS16"
  "cmpu.eq.ob\t%z0,%z1"
  [(set_attr "type"	"dspalu")
   (set_attr "mode"	"DI")])

(define_insn "mips_cmpu_lt_ob"
  [(set (reg:CCDSP CCDSP_CC_REGNUM)
	(unspec:CCDSP [(match_operand:V8QI 0 "reg_or_0_operand" "dY")
		       (match_operand:V8QI 1 "reg_or_0_operand" "dY")
		       (reg:CCDSP CCDSP_CC_REGNUM)]
		      UNSPEC_CMPU_LT_OB))]
  "TARGET_MIPS_DSP64 && !TARGET_MIPS16"
  "cmpu.lt.ob\t%z0,%z1"
  [(set_attr "type"	"dspalu")
   (set_attr "mode"	"DI")])

(define_insn "mips_cmpu_le_ob"
  [(set (reg:CCDSP CCDSP_CC_REGNUM)
	(unspec:CCDSP [(match_operand:V8QI 0 "reg_or_0_operand" "dY")
		       (match_operand:V8QI 1 "reg_or_0_operand" "dY")
		       (reg:CCDSP CCDSP_CC_REGNUM)]
		      UNSPEC_CMPU_LE_OB))]
  "TARGET_MIPS_DSP64 && !TARGET_MIPS16"
  "cmpu.le.ob\t%z0,%z1"
  [(set_attr "type"	"dspalu")
   (set_attr "mode"	"DI")])

(define_insn "mips_cmp_eq_qh"
  [(set (reg:CCDSP CCDSP_CC_REGNUM)
	(unspec:CCDSP [(match_operand:V4HI 0 "reg_or_0_operand" "dY")
		       (match_operand:V4HI 1 "reg_or_0_operand" "dY")
		       (reg:CCDSP CCDSP_CC_REGNUM)]
		      UNSPEC_CMP_EQ_QH))]
  "TARGET_MIPS_DSP64 && !TARGET_MIPS16"
  "cmp.eq.qh\t%z0,%z1"
  [(set_attr "type"	"dspalu")
   (set_attr "mode"	"DI")])

(define_insn "mips_cmp_lt_qh"
  [(set (reg:CCDSP CCDSP_CC_REGNUM)
	(unspec:CCDSP [(match_operand:V4HI 0 "reg_or_0_operand" "dY")
		       (match_operand:V4HI 1 "reg_or_0_operand" "dY")
		       (reg:CCDSP CCDSP_CC_REGNUM)]
		      UNSPEC_CMP_LT_QH))]
  "TARGET_MIPS_DSP64 && !TARGET_MIPS16"
  "cmp.lt.qh\t%z0,%z1"
  [(set_attr "type"	"dspalu")
   (set_attr "mode"	"DI")])

(define_insn "mips_cmp_le_qh"
  [(set (reg:CCDSP CCDSP_CC_REGNUM)
	(unspec:CCDSP [(match_operand:V4HI 0 "reg_or_0_operand" "dY")
		       (match_operand:V4HI 1 "reg_or_0_operand" "dY")
		       (reg:CCDSP CCDSP_CC_REGNUM)]
		      UNSPEC_CMP_LE_QH))]
  "TARGET_MIPS_DSP64 && !TARGET_MIPS16"
  "cmp.le.qh\t%z0,%z1"
  [(set_attr "type"	"dspalu")
   (set_attr "mode"	"DI")])

(define_insn "mips_cmp_eq_pw"
  [(set (reg:CCDSP CCDSP_CC_REGNUM)
	(unspec:CCDSP [(match_operand:V2SI 0 "reg_or_0_operand" "dY")
		       (match_operand:V2SI 1 "reg_or_0_operand" "dY")
		       (reg:CCDSP CCDSP_CC_REGNUM)]
		      UNSPEC_CMP_EQ_PW))]
  "TARGET_MIPS_DSP64 && !TARGET_MIPS16"
  "cmp.eq.pw\t%z0,%z1"
  [(set_attr "type"	"dspalu")
   (set_attr "mode"	"DI")])

(define_insn "mips_cmp_lt_pw"
  [(set (reg:CCDSP CCDSP_CC_REGNUM)
	(unspec:CCDSP [(match_operand:V2SI 0 "reg_or_0_operand" "dY")
		       (match_operand:V2SI 1 "reg_or_0_operand" "dY")
		       (reg:CCDSP CCDSP_CC_REGNUM)]
		      UNSPEC_CMP_LT_PW))]
  "TARGET_MIPS_DSP64 && !TARGET_MIPS16"
  "cmp.lt.pw\t%z0,%z1"
  [(set_attr "type"	"dspalu")
   (set_attr "mode"	"DI")])

(define_insn "mips_cmp_le_pw"
  [(set (reg:CCDSP CCDSP_CC_REGNUM)
	(unspec:CCDSP [(match_operand:V2SI 0 "reg_or_0_operand" "dY")
		       (match_operand:V2SI 1 "reg_or_0_operand" "dY")
		       (reg:CCDSP CCDSP_CC_REGNUM)]
		      UNSPEC_CMP_LE_PW))]
  "TARGET_MIPS_DSP64 && !TARGET_MIPS16"
  "cmp.le.pw\t%z0,%z1"
  [(set_attr "type"	"dspalu")
   (set_attr "mode"	"DI")])

; PICK*
(define_insn "mips_pick_qb"
  [(set (match_operand:V4QI 0 "register_operand" "=d")
	(unspec:V4QI [(match_operand:V4QI 1 "reg_or_0_operand" "dY")
		      (match_operand:V4QI 2 "reg_or_0_operand" "dY")
		      (reg:CCDSP CCDSP_CC_REGNUM)]
		     UNSPEC_PICK_QB))]
  "TARGET_MIPS_DSP32 && !TARGET_MIPS16"
  "pick.qb\t%0,%z1,%z2"
  [(set_attr "type"	"dspalu")
   (set_attr "mode"	"SI")])

(define_insn "mips_pick_ph"
  [(set (match_operand:V2HI 0 "register_operand" "=d")
	(unspec:V2HI [(match_operand:V2HI 1 "reg_or_0_operand" "dY")
		      (match_operand:V2HI 2 "reg_or_0_operand" "dY")
		      (reg:CCDSP CCDSP_CC_REGNUM)]
		     UNSPEC_PICK_PH))]
  "TARGET_MIPS_DSP32 && !TARGET_MIPS16"
  "pick.ph\t%0,%z1,%z2"
  [(set_attr "type"	"dspalu")
   (set_attr "mode"	"SI")])

(define_insn "mips_pick_ob"
  [(set (match_operand:V8QI 0 "register_operand" "=d")
	(unspec:V8QI [(match_operand:V8QI 1 "reg_or_0_operand" "dY")
		      (match_operand:V8QI 2 "reg_or_0_operand" "dY")
		      (reg:CCDSP CCDSP_CC_REGNUM)]
		     UNSPEC_PICK_OB))]
  "TARGET_MIPS_DSP64 && !TARGET_MIPS16"
  "pick.ob\t%0,%z1,%z2"
  [(set_attr "type"	"dspalu")
   (set_attr "mode"	"DI")])

(define_insn "mips_pick_qh"
  [(set (match_operand:V4HI 0 "register_operand" "=d")
	(unspec:V4HI [(match_operand:V4HI 1 "reg_or_0_operand" "dY")
		      (match_operand:V4HI 2 "reg_or_0_operand" "dY")
		      (reg:CCDSP CCDSP_CC_REGNUM)]
		     UNSPEC_PICK_QH))]
  "TARGET_MIPS_DSP64 && !TARGET_MIPS16"
  "pick.qh\t%0,%z1,%z2"
  [(set_attr "type"	"dspalu")
   (set_attr "mode"	"DI")])

(define_insn "mips_pick_pw"
  [(set (match_operand:V2SI 0 "register_operand" "=d")
	(unspec:V2SI [(match_operand:V2SI 1 "reg_or_0_operand" "dY")
		      (match_operand:V2SI 2 "reg_or_0_operand" "dY")
		      (reg:CCDSP CCDSP_CC_REGNUM)]
		     UNSPEC_PICK_PW))]
  "TARGET_MIPS_DSP64 && !TARGET_MIPS16"
  "pick.pw\t%0,%z1,%z2"
  [(set_attr "type"	"dspalu")
   (set_attr "mode"	"DI")])

; PACKRL*
(define_insn "mips_packrl_ph"
  [(set (match_operand:V2HI 0 "register_operand" "=d")
	(unspec:V2HI [(match_operand:V2HI 1 "reg_or_0_operand" "dY")
		      (match_operand:V2HI 2 "reg_or_0_operand" "dY")]
		     UNSPEC_PACKRL_PH))]
  "TARGET_MIPS_DSP32 && !TARGET_MIPS16"
  "packrl.ph\t%0,%z1,%z2"
  [(set_attr "type"	"dspalu")
   (set_attr "mode"	"SI")])

; Table 2-6. MIPS DSP ASE Instructions: Accumulator and DSPControl Access
; EXTR*
(define_insn "mips_extr_w"
  [(parallel
    [(set (match_operand:SI 0 "register_operand" "=d,d")
	  (unspec:SI [(match_operand:DI 1 "register_operand" "a,a")
		      (match_operand:SI 2 "arith_operand" "I,d")]
		     UNSPEC_EXTR_W))
     (set (reg:CCDSP CCDSP_OU_REGNUM)
	  (unspec:CCDSP [(match_dup 1) (match_dup 2)] UNSPEC_EXTR_W))])]
  "TARGET_MIPS_DSP32 && !TARGET_64BIT && !TARGET_MIPS16"
  "@
   extr.w\t%0,%q1,%2
   extrv.w\t%0,%q1,%2"
  [(set_attr "type"	"accext")
   (set_attr "mode"	"SI")])

(define_insn "mips_extr_r_w"
  [(parallel
    [(set (match_operand:SI 0 "register_operand" "=d,d")
	  (unspec:SI [(match_operand:DI 1 "register_operand" "a,a")
		      (match_operand:SI 2 "arith_operand" "I,d")]
		     UNSPEC_EXTR_R_W))
     (set (reg:CCDSP CCDSP_OU_REGNUM)
	  (unspec:CCDSP [(match_dup 1) (match_dup 2)] UNSPEC_EXTR_R_W))])]
  "TARGET_MIPS_DSP32 && !TARGET_64BIT && !TARGET_MIPS16"
  "@
   extr_r.w\t%0,%q1,%2
   extrv_r.w\t%0,%q1,%2"
  [(set_attr "type"	"accext")
   (set_attr "mode"	"SI")])

(define_insn "mips_extr_rs_w"
  [(parallel
    [(set (match_operand:SI 0 "register_operand" "=d,d")
	  (unspec:SI [(match_operand:DI 1 "register_operand" "a,a")
		      (match_operand:SI 2 "arith_operand" "I,d")]
		     UNSPEC_EXTR_RS_W))
     (set (reg:CCDSP CCDSP_OU_REGNUM)
	  (unspec:CCDSP [(match_dup 1) (match_dup 2)] UNSPEC_EXTR_RS_W))])]
  "TARGET_MIPS_DSP32 && !TARGET_64BIT && !TARGET_MIPS16"
  "@
   extr_rs.w\t%0,%q1,%2
   extrv_rs.w\t%0,%q1,%2"
  [(set_attr "type"	"accext")
   (set_attr "mode"	"SI")])

(define_insn "mips_extr_w_64"
  [(parallel
    [(set (match_operand:SI 0 "register_operand" "=d,d")
	  (unspec:SI [(match_operand:TI 1 "register_operand" "a,a")
		      (match_operand:SI 2 "arith_operand" "I,d")]
		     UNSPEC_EXTR_W_64))
     (set (reg:CCDSP CCDSP_OU_REGNUM)
	  (unspec:CCDSP [(match_dup 1) (match_dup 2)] UNSPEC_EXTR_W_64))])]
  "TARGET_MIPS_DSP64 && !TARGET_MIPS16"
  "@
   extr.w\t%0,%q1,%2
   extrv.w\t%0,%q1,%2"
  [(set_attr "type"	"accext")
   (set_attr "mode"	"DI")])

(define_insn "mips_extr_r_w_64"
  [(parallel
    [(set (match_operand:SI 0 "register_operand" "=d,d")
	  (unspec:SI [(match_operand:TI 1 "register_operand" "a,a")
		      (match_operand:SI 2 "arith_operand" "I,d")]
		     UNSPEC_EXTR_R_W_64))
     (set (reg:CCDSP CCDSP_OU_REGNUM)
	  (unspec:CCDSP [(match_dup 1) (match_dup 2)] UNSPEC_EXTR_R_W_64))])]
  "TARGET_MIPS_DSP64 && !TARGET_MIPS16"
  "@
   extr_r.w\t%0,%q1,%2
   extrv_r.w\t%0,%q1,%2"
  [(set_attr "type"	"accext")
   (set_attr "mode"	"DI")])

(define_insn "mips_extr_rs_w_64"
  [(parallel
    [(set (match_operand:SI 0 "register_operand" "=d,d")
	  (unspec:SI [(match_operand:TI 1 "register_operand" "a,a")
		      (match_operand:SI 2 "arith_operand" "I,d")]
		     UNSPEC_EXTR_RS_W_64))
     (set (reg:CCDSP CCDSP_OU_REGNUM)
	  (unspec:CCDSP [(match_dup 1) (match_dup 2)] UNSPEC_EXTR_RS_W_64))])]
  "TARGET_MIPS_DSP64 && !TARGET_MIPS16"
  "@
   extr_rs.w\t%0,%q1,%2
   extrv_rs.w\t%0,%q1,%2"
  [(set_attr "type"	"accext")
   (set_attr "mode"	"DI")])

(define_insn "mips_dextr_w"
  [(parallel
    [(set (match_operand:DI 0 "register_operand" "=d,d")
	  (unspec:DI [(match_operand:TI 1 "register_operand" "a,a")
		      (match_operand:DI 2 "arith_operand" "I,d")]
		     UNSPEC_DEXTR_W))
     (set (reg:CCDSP CCDSP_OU_REGNUM)
	  (unspec:CCDSP [(match_dup 1) (match_dup 2)] UNSPEC_DEXTR_W))])]
  "TARGET_MIPS_DSP64 && !TARGET_MIPS16"
  "@
   dextr.w\t%0,%q1,%2
   dextrv.w\t%0,%q1,%2"
  [(set_attr "type"	"accext")
   (set_attr "mode"	"DI")])

(define_insn "mips_dextr_r_w"
  [(parallel
    [(set (match_operand:DI 0 "register_operand" "=d,d")
	  (unspec:DI [(match_operand:TI 1 "register_operand" "a,a")
		      (match_operand:DI 2 "arith_operand" "I,d")]
		     UNSPEC_DEXTR_R_W))
     (set (reg:CCDSP CCDSP_OU_REGNUM)
	  (unspec:CCDSP [(match_dup 1) (match_dup 2)] UNSPEC_DEXTR_R_W))])]
  "TARGET_MIPS_DSP64 && !TARGET_MIPS16"
  "@
   dextr_r.w\t%0,%q1,%2
   dextrv_r.w\t%0,%q1,%2"
  [(set_attr "type"	"accext")
   (set_attr "mode"	"DI")])

(define_insn "mips_dextr_rs_w"
  [(parallel
    [(set (match_operand:DI 0 "register_operand" "=d,d")
	  (unspec:DI [(match_operand:TI 1 "register_operand" "a,a")
		      (match_operand:DI 2 "arith_operand" "I,d")]
		     UNSPEC_DEXTR_RS_W))
     (set (reg:CCDSP CCDSP_OU_REGNUM)
	  (unspec:CCDSP [(match_dup 1) (match_dup 2)] UNSPEC_DEXTR_RS_W))])]
  "TARGET_MIPS_DSP64 && !TARGET_MIPS16"
  "@
   dextr_rs.w\t%0,%q1,%2
   dextrv_rs.w\t%0,%q1,%2"
  [(set_attr "type"	"accext")
   (set_attr "mode"	"DI")])

(define_insn "mips_dextr_l"
  [(parallel
    [(set (match_operand:DI 0 "register_operand" "=d,d")
	  (unspec:DI [(match_operand:TI 1 "register_operand" "a,a")
		      (match_operand:DI 2 "arith_operand" "I,d")]
		     UNSPEC_DEXTR_L))
     (set (reg:CCDSP CCDSP_OU_REGNUM)
	  (unspec:CCDSP [(match_dup 1) (match_dup 2)] UNSPEC_DEXTR_L))])]
  "TARGET_MIPS_DSP64 && !TARGET_MIPS16"
  "@
   dextr.l\t%0,%q1,%2
   dextrv.l\t%0,%q1,%2"
  [(set_attr "type"	"accext")
   (set_attr "mode"	"DI")])

(define_insn "mips_dextr_r_l"
  [(parallel
    [(set (match_operand:DI 0 "register_operand" "=d,d")
	  (unspec:DI [(match_operand:TI 1 "register_operand" "a,a")
		      (match_operand:DI 2 "arith_operand" "I,d")]
		     UNSPEC_DEXTR_R_L))
     (set (reg:CCDSP CCDSP_OU_REGNUM)
	  (unspec:CCDSP [(match_dup 1) (match_dup 2)] UNSPEC_DEXTR_R_L))])]
  "TARGET_MIPS_DSP64 && !TARGET_MIPS16"
  "@
   dextr_r.l\t%0,%q1,%2
   dextrv_r.l\t%0,%q1,%2"
  [(set_attr "type"	"accext")
   (set_attr "mode"	"DI")])

(define_insn "mips_dextr_rs_l"
  [(parallel
    [(set (match_operand:DI 0 "register_operand" "=d,d")
	  (unspec:DI [(match_operand:TI 1 "register_operand" "a,a")
		      (match_operand:DI 2 "arith_operand" "I,d")]
		     UNSPEC_DEXTR_RS_L))
     (set (reg:CCDSP CCDSP_OU_REGNUM)
	  (unspec:CCDSP [(match_dup 1) (match_dup 2)] UNSPEC_DEXTR_RS_L))])]
  "TARGET_MIPS_DSP64 && !TARGET_MIPS16"
  "@
   dextr_rs.l\t%0,%q1,%2
   dextrv_rs.l\t%0,%q1,%2"
  [(set_attr "type"	"accext")
   (set_attr "mode"	"DI")])

; EXTR*_S.H
(define_insn "mips_extr_s_h"
  [(parallel
    [(set (match_operand:SI 0 "register_operand" "=d,d")
	  (unspec:SI [(match_operand:DI 1 "register_operand" "a,a")
		      (match_operand:SI 2 "arith_operand" "I,d")]
		     UNSPEC_EXTR_S_H))
     (set (reg:CCDSP CCDSP_OU_REGNUM)
	  (unspec:CCDSP [(match_dup 1) (match_dup 2)] UNSPEC_EXTR_S_H))])]
  "TARGET_MIPS_DSP32 && !TARGET_64BIT && !TARGET_MIPS16"
  "@
   extr_s.h\t%0,%q1,%2
   extrv_s.h\t%0,%q1,%2"
  [(set_attr "type"	"accext")
   (set_attr "mode"	"SI")])

(define_insn "mips_extr_s_h_64"
  [(parallel
    [(set (match_operand:SI 0 "register_operand" "=d,d")
	  (unspec:SI [(match_operand:TI 1 "register_operand" "a,a")
		      (match_operand:SI 2 "arith_operand" "I,d")]
		     UNSPEC_EXTR_S_H_64))
     (set (reg:CCDSP CCDSP_OU_REGNUM)
	  (unspec:CCDSP [(match_dup 1) (match_dup 2)] UNSPEC_EXTR_S_H_64))])]
  "TARGET_MIPS_DSP64 && !TARGET_MIPS16"
  "@
   extr_s.h\t%0,%q1,%2
   extrv_s.h\t%0,%q1,%2"
  [(set_attr "type"	"accext")
   (set_attr "mode"	"DI")])

; EXTL*
(define_insn "mips_dextl_w"
  [(parallel
    [(set (match_operand:DI 0 "register_operand" "=d,d")
	  (unspec:DI [(match_operand:TI 1 "register_operand" "a,a")
		      (match_operand:DI 2 "arith_operand" "I,d")]
		     UNSPEC_DEXTL_W))
     (set (reg:CCDSP CCDSP_OU_REGNUM)
	  (unspec:CCDSP [(match_dup 1) (match_dup 2)] UNSPEC_DEXTL_W))])]
  "TARGET_MIPS_DSP64 && !TARGET_MIPS16"
  "@
   dextl.w\t%0,%q1,%2
   dextlv.w\t%0,%q1,%2"
  [(set_attr "type"	"accext")
   (set_attr "mode"	"DI")])

(define_insn "mips_dextl_s_w"
  [(parallel
    [(set (match_operand:DI 0 "register_operand" "=d,d")
	  (unspec:DI [(match_operand:TI 1 "register_operand" "a,a")
		      (match_operand:DI 2 "arith_operand" "I,d")]
		     UNSPEC_DEXTL_S_W))
     (set (reg:CCDSP CCDSP_OU_REGNUM)
	  (unspec:CCDSP [(match_dup 1) (match_dup 2)] UNSPEC_DEXTL_S_W))])]
  "TARGET_MIPS_DSP64 && !TARGET_MIPS16"
  "@
   dextl_s.w\t%0,%q1,%2
   dextlv_s.w\t%0,%q1,%2"
  [(set_attr "type"	"accext")
   (set_attr "mode"	"DI")])

(define_insn "mips_dextl_l"
  [(parallel
    [(set (match_operand:DI 0 "register_operand" "=d,d")
	  (unspec:DI [(match_operand:TI 1 "register_operand" "a,a")
		      (match_operand:DI 2 "arith_operand" "I,d")]
		     UNSPEC_DEXTL_L))
     (set (reg:CCDSP CCDSP_OU_REGNUM)
	  (unspec:CCDSP [(match_dup 1) (match_dup 2)] UNSPEC_DEXTL_L))])]
  "TARGET_MIPS_DSP64 && !TARGET_MIPS16"
  "@
   dextl.l\t%0,%q1,%2
   dextlv.l\t%0,%q1,%2"
  [(set_attr "type"	"accext")
   (set_attr "mode"	"DI")])

(define_insn "mips_dextl_s_l"
  [(parallel
    [(set (match_operand:DI 0 "register_operand" "=d,d")
	  (unspec:DI [(match_operand:TI 1 "register_operand" "a,a")
		      (match_operand:DI 2 "arith_operand" "I,d")]
		     UNSPEC_DEXTL_S_L))
     (set (reg:CCDSP CCDSP_OU_REGNUM)
	  (unspec:CCDSP [(match_dup 1) (match_dup 2)] UNSPEC_DEXTL_S_L))])]
  "TARGET_MIPS_DSP64 && !TARGET_MIPS16"
  "@
   dextl_s.l\t%0,%q1,%2
   dextlv_s.l\t%0,%q1,%2"
  [(set_attr "type"	"accext")
   (set_attr "mode"	"DI")])

; EXTP*
(define_insn "mips_extp"
  [(parallel
    [(set (match_operand:SI 0 "register_operand" "=d,d")
	  (unspec:SI [(match_operand:DI 1 "register_operand" "a,a")
		      (match_operand:SI 2 "arith_operand" "I,d")
		      (reg:CCDSP CCDSP_PO_REGNUM)]
		     UNSPEC_EXTP))
     (set (reg:CCDSP CCDSP_EF_REGNUM)
	  (unspec:CCDSP [(match_dup 1) (match_dup 2)] UNSPEC_EXTP))])]
  "TARGET_MIPS_DSP32 && !TARGET_64BIT && !TARGET_MIPS16"
  "@
   extp\t%0,%q1,%2
   extpv\t%0,%q1,%2"
  [(set_attr "type"	"accext")
   (set_attr "mode"	"SI")])

(define_insn "mips_extpdp"
  [(parallel
    [(set (match_operand:SI 0 "register_operand" "=d,d")
	  (unspec:SI [(match_operand:DI 1 "register_operand" "a,a")
		      (match_operand:SI 2 "arith_operand" "I,d")
		      (reg:CCDSP CCDSP_PO_REGNUM)]
		     UNSPEC_EXTPDP))
     (set (reg:CCDSP CCDSP_PO_REGNUM)
	  (unspec:CCDSP [(match_dup 1) (match_dup 2)
			 (reg:CCDSP CCDSP_PO_REGNUM)] UNSPEC_EXTPDP))
     (set (reg:CCDSP CCDSP_EF_REGNUM)
	  (unspec:CCDSP [(match_dup 1) (match_dup 2)] UNSPEC_EXTPDP))])]
  "TARGET_MIPS_DSP32 && !TARGET_64BIT && !TARGET_MIPS16"
  "@
   extpdp\t%0,%q1,%2
   extpdpv\t%0,%q1,%2"
  [(set_attr "type"	"accext")
   (set_attr "mode"	"SI")])

(define_insn "mips_extp_64"
  [(parallel
    [(set (match_operand:SI 0 "register_operand" "=d,d")
	  (unspec:SI [(match_operand:TI 1 "register_operand" "a,a")
		      (match_operand:SI 2 "arith_operand" "I,d")
		      (reg:CCDSP CCDSP_PO_REGNUM)]
		     UNSPEC_EXTP_64))
     (set (reg:CCDSP CCDSP_EF_REGNUM)
	  (unspec:CCDSP [(match_dup 1) (match_dup 2)] UNSPEC_EXTP_64))])]
  "TARGET_MIPS_DSP64 && !TARGET_MIPS16"
  "@
   extp\t%0,%q1,%2
   extpv\t%0,%q1,%2"
  [(set_attr "type"	"accext")
   (set_attr "mode"	"DI")])

(define_insn "mips_extpdp_64"
  [(parallel
    [(set (match_operand:SI 0 "register_operand" "=d,d")
	  (unspec:SI [(match_operand:TI 1 "register_operand" "a,a")
		      (match_operand:SI 2 "arith_operand" "I,d")
		      (reg:CCDSP CCDSP_PO_REGNUM)]
		     UNSPEC_EXTPDP_64))
     (set (reg:CCDSP CCDSP_PO_REGNUM)
	  (unspec:CCDSP [(match_dup 1) (match_dup 2)
			 (reg:CCDSP CCDSP_PO_REGNUM)] UNSPEC_EXTPDP_64))
     (set (reg:CCDSP CCDSP_EF_REGNUM)
	  (unspec:CCDSP [(match_dup 1) (match_dup 2)] UNSPEC_EXTPDP_64))])]
  "TARGET_MIPS_DSP64 && !TARGET_MIPS16"
  "@
   extpdp\t%0,%q1,%2
   extpdpv\t%0,%q1,%2"
  [(set_attr "type"	"accext")
   (set_attr "mode"	"DI")])

(define_insn "mips_dextp"
  [(parallel
    [(set (match_operand:DI 0 "register_operand" "=d,d")
	  (unspec:DI [(match_operand:TI 1 "register_operand" "a,a")
		      (match_operand:DI 2 "arith_operand" "I,d")
		      (reg:CCDSP CCDSP_PO_REGNUM)]
		     UNSPEC_DEXTP))
     (set (reg:CCDSP CCDSP_EF_REGNUM)
	  (unspec:CCDSP [(match_dup 1) (match_dup 2)] UNSPEC_DEXTP))])]
  "TARGET_MIPS_DSP64 && !TARGET_MIPS16"
  "@
   dextp\t%0,%q1,%2
   dextpv\t%0,%q1,%2"
  [(set_attr "type"	"accext")
   (set_attr "mode"	"DI")])

(define_insn "mips_dextpdp"
  [(parallel
    [(set (match_operand:DI 0 "register_operand" "=d,d")
	  (unspec:DI [(match_operand:TI 1 "register_operand" "a,a")
		      (match_operand:DI 2 "arith_operand" "I,d")
		      (reg:CCDSP CCDSP_PO_REGNUM)]
		     UNSPEC_DEXTPDP))
     (set (reg:CCDSP CCDSP_PO_REGNUM)
	  (unspec:CCDSP [(match_dup 1) (match_dup 2)
			 (reg:CCDSP CCDSP_PO_REGNUM)] UNSPEC_DEXTPDP))
     (set (reg:CCDSP CCDSP_EF_REGNUM)
	  (unspec:CCDSP [(match_dup 1) (match_dup 2)] UNSPEC_DEXTPDP))])]
  "TARGET_MIPS_DSP64 && !TARGET_MIPS16"
  "@
   dextpdp\t%0,%q1,%2
   dextpdpv\t%0,%q1,%2"
  [(set_attr "type"	"accext")
   (set_attr "mode"	"DI")])

; SHILO*
(define_insn "mips_shilo"
  [(set (match_operand:DI 0 "register_operand" "=a,a")
	(unspec:DI [(match_operand:DI 1 "register_operand" "0,0")
		    (match_operand:SI 2 "arith_operand" "I,d")]
		   UNSPEC_SHILO))]
  "TARGET_MIPS_DSP32 && !TARGET_64BIT && !TARGET_MIPS16"
  "@
   shilo\t%q0,%2
   shilov\t%q0,%2"
  [(set_attr "type"	"accmod")
   (set_attr "mode"	"SI")])

(define_insn "mips_shilo_64"
  [(set (match_operand:TI 0 "register_operand" "=a,a")
	(unspec:TI [(match_operand:TI 1 "register_operand" "0,0")
		    (match_operand:SI 2 "arith_operand" "I,d")]
		   UNSPEC_SHILO_64))]
  "TARGET_MIPS_DSP64 && !TARGET_MIPS16"
  "@
   shilo\t%q0,%2
   shilov\t%q0,%2"
  [(set_attr "type"	"accmod")
   (set_attr "mode"	"DI")])

(define_insn "mips_dshilo"
  [(set (match_operand:TI 0 "register_operand" "=a,a")
	(unspec:TI [(match_operand:TI 1 "register_operand" "0,0")
		    (match_operand:DI 2 "arith_operand" "I,d")]
		   UNSPEC_DSHILO))]
  "TARGET_MIPS_DSP64 && !TARGET_MIPS16"
  "@
   dshilo\t%q0,%2
   dshilov\t%q0,%2"
  [(set_attr "type"	"accmod")
   (set_attr "mode"	"DI")])

; MTHLIP*
(define_insn "mips_mthlip"
  [(parallel
    [(set (match_operand:DI 0 "register_operand" "=a")
	  (unspec:DI [(match_operand:DI 1 "register_operand" "0")
		      (match_operand:SI 2 "reg_or_0_operand" "dJ")
		      (reg:CCDSP CCDSP_PO_REGNUM)]
		     UNSPEC_MTHLIP))
     (set (reg:CCDSP CCDSP_PO_REGNUM)
	  (unspec:CCDSP [(match_dup 1) (match_dup 2)
			 (reg:CCDSP CCDSP_PO_REGNUM)] UNSPEC_MTHLIP))])]
  "TARGET_MIPS_DSP32 && !TARGET_64BIT && !TARGET_MIPS16"
  "mthlip\t%z2,%q0"
  [(set_attr "type"	"accmod")
   (set_attr "mode"	"SI")])

(define_insn "mips_mthlip_64"
  [(parallel
    [(set (match_operand:TI 0 "register_operand" "=a")
	  (unspec:TI [(match_operand:TI 1 "register_operand" "0")
		      (match_operand:SI 2 "reg_or_0_operand" "dJ")
		      (reg:CCDSP CCDSP_PO_REGNUM)]
		     UNSPEC_MTHLIP_64))
     (set (reg:CCDSP CCDSP_PO_REGNUM)
	  (unspec:CCDSP [(match_dup 1) (match_dup 2)
			 (reg:CCDSP CCDSP_PO_REGNUM)] UNSPEC_MTHLIP_64))])]
  "TARGET_MIPS_DSP64 && !TARGET_MIPS16"
  "mthlip\t%z2,%q0"
  [(set_attr "type"	"accmod")
   (set_attr "mode"	"DI")])

(define_insn "mips_dmthlip"
  [(parallel
    [(set (match_operand:TI 0 "register_operand" "=a")
	  (unspec:TI [(match_operand:TI 1 "register_operand" "0")
		      (match_operand:DI 2 "reg_or_0_operand" "dJ")
		      (reg:CCDSP CCDSP_PO_REGNUM)]
		     UNSPEC_DMTHLIP))
     (set (reg:CCDSP CCDSP_PO_REGNUM)
	  (unspec:CCDSP [(match_dup 1) (match_dup 2)
			 (reg:CCDSP CCDSP_PO_REGNUM)] UNSPEC_DMTHLIP))])]
  "TARGET_MIPS_DSP64 && !TARGET_MIPS16"
  "dmthlip\t%z2,%q0"
  [(set_attr "type"	"accmod")
   (set_attr "mode"	"DI")])

; WRDSP
(define_insn "mips_wrdsp"
  [(parallel
    [(set (reg:CCDSP CCDSP_PO_REGNUM)
	  (unspec:CCDSP [(match_operand:SI 0 "reg_or_0_operand" "dJ")
			 (match_operand:SI 1 "immediate_operand" "i")
			 (reg:CCDSP CCDSP_PO_REGNUM)]
			 UNSPEC_WRDSP))
     (set (reg:CCDSP CCDSP_SC_REGNUM)
	  (unspec:CCDSP [(match_dup 0) (match_dup 1)
			 (reg:CCDSP CCDSP_SC_REGNUM)] UNSPEC_WRDSP))
     (set (reg:CCDSP CCDSP_CA_REGNUM)
	  (unspec:CCDSP [(match_dup 0) (match_dup 1)
			 (reg:CCDSP CCDSP_CA_REGNUM)] UNSPEC_WRDSP))
     (set (reg:CCDSP CCDSP_OU_REGNUM)
	  (unspec:CCDSP [(match_dup 0) (match_dup 1)
			 (reg:CCDSP CCDSP_OU_REGNUM)] UNSPEC_WRDSP))
     (set (reg:CCDSP CCDSP_CC_REGNUM)
	  (unspec:CCDSP [(match_dup 0) (match_dup 1)
			 (reg:CCDSP CCDSP_CC_REGNUM)] UNSPEC_WRDSP))
     (set (reg:CCDSP CCDSP_EF_REGNUM)
	  (unspec:CCDSP [(match_dup 0) (match_dup 1)
			 (reg:CCDSP CCDSP_EF_REGNUM)] UNSPEC_WRDSP))])]
  "TARGET_MIPS_DSP32 && !TARGET_MIPS16"
  "wrdsp\t%z0,%1"
  [(set_attr "type"	"dspalu")
   (set_attr "mode"	"SI")])

; RDDSP
(define_insn "mips_rddsp"
  [(set (match_operand:SI 0 "register_operand" "=d")
	(unspec:SI [(match_operand:SI 1 "immediate_operand" "i")
		    (reg:CCDSP CCDSP_PO_REGNUM)
		    (reg:CCDSP CCDSP_SC_REGNUM)
		    (reg:CCDSP CCDSP_CA_REGNUM)
		    (reg:CCDSP CCDSP_OU_REGNUM)
		    (reg:CCDSP CCDSP_CC_REGNUM)
		    (reg:CCDSP CCDSP_EF_REGNUM)]
		   UNSPEC_RDDSP))]
  "TARGET_MIPS_DSP32 && !TARGET_MIPS16"
  "rddsp\t%0,%1"
  [(set_attr "type"	"dspalu")
   (set_attr "mode"	"SI")])

; Table 2-7. MIPS DSP ASE Instructions: Indexed-Load
; L*X
(define_insn "mips_lbux"
  [(set (match_operand:SI 0 "register_operand" "=d")
	(zero_extend:SI (mem:QI (plus:SI (match_operand:SI 1
					  "reg_or_0_operand" "dJ")
					 (match_operand:SI 2
					  "reg_or_0_operand" "dJ")))))]
  "TARGET_MIPS_DSP32 && !TARGET_MIPS16"
  "lbux\t%0,%z2(%z1)"
  [(set_attr "type"	"load")
   (set_attr "mode"	"SI")
   (set_attr "length"	"4")])

(define_insn "mips_lhx"
  [(set (match_operand:SI 0 "register_operand" "=d")
	(sign_extend:SI (mem:HI (plus:SI (match_operand:SI 1
					  "reg_or_0_operand" "dJ")
					 (match_operand:SI 2
					  "reg_or_0_operand" "dJ")))))]
  "TARGET_MIPS_DSP32 && !TARGET_MIPS16"
  "lhx\t%0,%z2(%z1)"
  [(set_attr "type"	"load")
   (set_attr "mode"	"SI")
   (set_attr "length"	"4")])

(define_insn "mips_lwx"
  [(set (match_operand:SI 0 "register_operand" "=d")
	(mem:SI (plus:SI (match_operand:SI 1 "reg_or_0_operand" "dJ")
			 (match_operand:SI 2 "reg_or_0_operand" "dJ"))))]
  "TARGET_MIPS_DSP32 && !TARGET_MIPS16"
  "lwx\t%0,%z2(%z1)"
  [(set_attr "type"	"load")
   (set_attr "mode"	"SI")
   (set_attr "length"	"4")])

(define_insn "mips_ldx"
  [(set (match_operand:DI 0 "register_operand" "=d")
	(mem:DI (plus:DI (match_operand:DI 1 "reg_or_0_operand" "dJ")
			 (match_operand:DI 2 "reg_or_0_operand" "dJ"))))]
  "TARGET_MIPS_DSP64 && !TARGET_MIPS16"
  "ldx\t%0,%z2(%z1)"
  [(set_attr "type"	"load")
   (set_attr "mode"	"DI")
   (set_attr "length"	"4")])

; Table 2-8. MIPS DSP ASE Instructions: Branch
; BPOSGE32
(define_insn "mips_bposge"
  [(set (pc)
	(if_then_else
	   (ge:CCDSP (reg:CCDSP CCDSP_PO_REGNUM)
		     (match_operand:SI 0 "immediate_operand" "i"))
	   (label_ref (match_operand 1 "" ""))
	   (pc)))]
  "TARGET_MIPS_DSP32 && !TARGET_MIPS16"
  "%*bposge%0\t%1%/"
  [(set_attr "type"	"branch")
   (set_attr "mode"	"none")])

