;; .........................
;;
;; DFA-based pipeline descriptions for MIPS64 model 5Kc and
;; MIPS32 model 4Kc/m (and 4Kp and M4K).
;;
;; The 5Kc and 4Kc look nearly identical from a functional
;; unit/scheduling point of view.  They are both simple, single-issue
;; processors with no floating-point unit.  The 5Kc, however, also has
;; 64-bit operations. The 5Kf has a separate floating point pipe which
;; can dual-issue with the integer pipe.
;; 
;; References:
;;   "MIPS64 5Kc Processor Core Data Sheet, Rev. 1.7.3, December 14, 1999."
;;   "MIPS32 4Kc & 4Kp Processor Cores User Manual, Rev. 1.0, May, 1999."
;;
;; .........................

(define_automaton "r5kc_cpu, r5kc_mdu, r5kc_fpu")

;; Integer execution unit.
(define_cpu_unit "r5kc_ixu_arith"       "r5kc_cpu")
(define_cpu_unit "r5kc_ixu_mpydiv"      "r5kc_mdu")
(define_cpu_unit "r5kc_ixu_mpydiv_iter" "r5kc_mdu")
(define_cpu_unit "r5kf_fpu_arith"       "r5kc_fpu")

(define_reservation "r5kc_mpydiv"       "r5kc_ixu_arith+r5kc_ixu_mpydiv")

;;
;; Describe instruction reservations for integer operations.
;;

;; This description does not model the pipeline(s) at the hardware
;; level but instead as the interaction between instructions.

;; The result of a load appears with a latency of 2 cycles
;; but only stalls the pipeline for a single cycle if there
;; is a flow dependency.

(define_insn_reservation "r5kc_int_load" 2 
  			 (and (eq_attr "cpu" "4kc,4kp,5kc")
			      (eq_attr "type" "load,prefetch,prefetchx"))
                         "r5kc_ixu_arith")

(define_insn_reservation "r5kc_int_store" 1 
  			 (and (eq_attr "cpu" "4kc,4kp,5kc")
			      (eq_attr "type" "store"))
                         "r5kc_ixu_arith")

(define_insn_reservation "r5kc_idiv_si2" 34 
  			 (and (eq_attr "cpu" "4kc,4kp,5kc")
			      (and (eq_attr "type" "idiv")
				   (eq_attr "mode" "!DI")))
                         "r5kc_mpydiv+(r5kc_ixu_mpydiv_iter*34)")

(define_insn_reservation "r5kc_idiv_di2" 66 
  			 (and (eq_attr "cpu" "5kc")
			      (and (eq_attr "type" "idiv")
				   (eq_attr "mode" "DI")))
                         "r5kc_mpydiv+(r5kc_ixu_mpydiv_iter*66)")

;; 4kc/m fast 32x32 multiply
;; 16x32 is faster, but there's no way to detect this
(define_insn_reservation "r5kc_impy_4kc" 2 
  			 (and (eq_attr "cpu" "4kc")
			      (and (eq_attr "type" "imul,imadd")
				   (eq_attr "mode" "SI")))
                         "r5kc_mpydiv+(r5kc_ixu_mpydiv_iter*2)")

;; 4Kc/4Km MUL has 2 cycle latency, but has the special property that it will
;; stall the integer unit pipeline. MUL 16x16 or 32x16 forces 1 cycle stall,
;; while MUL 32x32 forces 2 cycle stall.  If next insn use the result, an
;; additional stall is forced.
(define_insn_reservation "r5k_mul_4kc" 4
                         (and (eq_attr "cpu" "4kc")
			      (and (eq_attr "type" "imul3")
				   (eq_attr "mode" "SI")))
			 "(r5kc_mpydiv+r5kc_ixu_mpydiv_iter)*3")

;; 4Kp slow iterative 2-op MULT
;; Latency of 32 if next insn is MADD/MSUB,MFHI/MFLO.
;; Repeat rate of 33 cycles.
(define_insn_reservation "r5kc_imul_4kp" 32
  			 (and (eq_attr "cpu" "4kp")
			      (and (eq_attr "type" "imul")
				   (eq_attr "mode" "SI")))
                         "r5kc_mpydiv+(r5kc_ixu_mpydiv_iter*32)")

;; 4Kp slow iterative 3-op MUL
;; Latency of 32 cycles, but stalls the whole pipeline until complete.
(define_insn_reservation "r5kc_imul3_4kp" 32
                         (and (eq_attr "cpu" "4kp")
			      (and (eq_attr "type" "imul3")
				   (eq_attr "mode" "SI")))
			 "(r5kc_mpydiv+r5kc_ixu_mpydiv_iter)*32")

;; 4Kp slow iterative MADD
;; Latency of 34 if next use insn is MADD/MSUB,MFHI/MFLO.
;; Repeat rate of 35 cycles.
(define_insn_reservation "r5kc_imadd_4kp" 34
                         (and (eq_attr "cpu" "4kp")
			      (and (eq_attr "type" "imadd")
				   (eq_attr "mode" "SI")))
                         "r5kc_mpydiv+(r5kc_ixu_mpydiv_iter*34)")

;; 5kc 32x32 multiply
(define_insn_reservation "r5kc_impy_5k_si" 4 
  			 (and (eq_attr "cpu" "5kc")
			      (and (eq_attr "type" "imul,imul3,imadd")
				   (eq_attr "mode" "SI")))
                         "r5kc_mpydiv+(r5kc_ixu_mpydiv_iter*2)")

;; 5kc 64x64 multiply (passes through multiplier 9 times).
(define_insn_reservation "r5kc_impy_di2" 11 
  			 (and (eq_attr "cpu" "5kc")
			      (and (eq_attr "type" "imul")
				   (eq_attr "mode" "DI")))
                         "r5kc_mpydiv+(r5kc_ixu_mpydiv_iter*11)")

;; Move to/from HI/LO has a 2 cycle latency but is pipelined.
(define_insn_reservation "r5kc_impy_hilo" 2 
			 (and (eq_attr "cpu" "4kc,4kp,5kc")
			      (eq_attr "type" "mfhilo,mthilo"))
                         "r5kc_ixu_arith+r5kc_ixu_mpydiv_iter")

;; All other integer insns.  
(define_insn_reservation "r5kc_int_alu" 1 
  			 (and (eq_attr "cpu" "4kc,4kp,5kc")
			      (and (eq_attr "type" "move,condmove,arith,darith,shift,vshift,const,icmp,nop")
				   (eq_attr "mode" "!SF,DF")))
  			 "r5kc_ixu_arith")

(define_insn_reservation "r5kc_int_branch" 1 
  			 (and (eq_attr "cpu" "4kc,4kp,5kc")
			      (eq_attr "type" "branch"))
  			 "r5kc_ixu_arith")

(define_insn_reservation "r5kc_int_jump_4k" 1 
			 (and (eq_attr "cpu" "4kc,4kp")
			      (eq_attr "type" "jump,call"))
  			 "r5kc_ixu_arith")

(define_insn_reservation "r5kc_int_jump_5k" 2 
  			 (and (eq_attr "cpu" "5kc")
			      (eq_attr "type" "jump,call"))
  	                 "r5kc_ixu_arith")

;; Extra latency caused by 5kc jr/jalr dependency 
(define_bypass 3 "r5kc_int_load" "r5kc_int_jump_5k")
(define_bypass 2 "r5kc_int_alu" "r5kc_int_jump_5k")

;; mfcx/mtcx - non FPU
(define_insn_reservation "r5kc_int_xfer" 2 
  			 (and (eq_attr "cpu" "4kc,4kp,5kc")
			      (and (eq_attr "type" "xfer")
				   (eq_attr "cnv_mode" "unknown")))
                         "r5kc_ixu_arith")

;; Unknown or multi - single issue
(define_insn_reservation "r5kc_unknown" 1 
			 (and (eq_attr "cpu" "4kc,4kp,5kc")
			      (eq_attr "type" "unknown,multi"))
                         "r5kc_ixu_arith+r5kf_fpu_arith")

; --------------------------------------------------------------
; Floating Point Instructions
; --------------------------------------------------------------

; The 5Kf is a partial dual-issue cpu which can dual issue an integer 
; and floating-point instruction in the same cycle.

(define_reservation "r5kf_fpu_iss"	"r5kf_fpu_arith")

; fadd, fabs, fneg
(define_insn_reservation "r5kf_fadd" 4 
                         (and (eq_attr "cpu" "5kc")
			      (eq_attr "type" "fadd,fabs,fneg"))
			 "r5kf_fpu_iss")

; fmove, fcmove
(define_insn_reservation "r5kf_fmove" 4 
                         (and (eq_attr "cpu" "5kc")
			      (and (eq_attr "type" "move,condmove")
				   (eq_attr "mode" "SF,DF")))
			 "r5kf_fpu_iss")

; fload
(define_insn_reservation "r5kf_fload" 3 
  			 (and (eq_attr "cpu" "5kc")
			      (eq_attr "type" "fpload"))
			 "r5kf_fpu_iss")

; fstore
(define_insn_reservation "r5kf_fstore" 1 
                         (and (eq_attr "cpu" "5kc")
			      (eq_attr "type" "fpstore"))
			 "r5kf_fpu_iss")

; fmul, fmadd
(define_insn_reservation "r5kf_fmul_sf" 4 
                         (and (eq_attr "cpu" "5kc")
			      (and (eq_attr "type" "fmul,fmadd")
				   (eq_attr "mode" "SF")))
			 "r5kf_fpu_iss")

(define_insn_reservation "r5kf_fmul_df" 5 
                         (and (eq_attr "cpu" "5kc")
			      (and (eq_attr "type" "fmul,fmadd")
				   (eq_attr "mode" "DF")))
			 "r5kf_fpu_iss,r5kf_fpu_arith")

; fdiv, fsqrt, frsqrt
(define_insn_reservation "r5kf_fdiv_sf" 17 
                         (and (eq_attr "cpu" "5kc")
			      (and (eq_attr "type" "fdiv,fsqrt,frsqrt")
				   (eq_attr "mode" "SF")))
			 "r5kf_fpu_iss,(r5kf_fpu_arith*13)")

(define_insn_reservation "r5kf_fdiv_df" 32 
                         (and (eq_attr "cpu" "5kc")
			      (and (eq_attr "type" "fdiv,fsqrt")
				   (eq_attr "mode" "DF")))
			 "r5kf_fpu_iss,(r5kf_fpu_arith*28)")

; frsqrt
(define_insn_reservation "r5kf_frsqrt_df" 35 
                         (and (eq_attr "cpu" "5kc")
			      (and (eq_attr "type" "frsqrt")
				   (eq_attr "mode" "DF")))
			 "r5kf_fpu_iss,(r5kf_fpu_arith*30)")

; fcmp
(define_insn_reservation "r5kf_fcmp" 2 
                         (and (eq_attr "cpu" "5kc")
			      (eq_attr "type" "fcmp"))
			 "r5kf_fpu_iss")

; fcmp -> movf.fmt & movt.fmt bypass (dependency must be on condition)
(define_bypass 1 "r5kf_fcmp" "r5kf_fmove")

; fcvt (cvt.d.s, cvt.[sd].[wl]
(define_insn_reservation "r5kf_fcvt_d2s" 4 
                         (and (eq_attr "cpu" "5kc")
			      (and (eq_attr "type" "fcvt")
				   (eq_attr "cnv_mode" "I2S,I2D,S2D")))
			 "r5kf_fpu_iss")

; fcvt (cvt.s.d)
(define_insn_reservation "r5kf_fcvt_s2d" 6 
                         (and (eq_attr "cpu" "5kc")
			      (and (eq_attr "type" "fcvt")
				   (eq_attr "cnv_mode" "D2S")))
			 "r5kf_fpu_iss")

; fcvt (cvt.[wl].[sd], etc)
(define_insn_reservation "r5kf_fcvt_f2i" 5 
                         (and (eq_attr "cpu" "5kc")
			      (and (eq_attr "type" "fcvt")
				   (eq_attr "cnv_mode" "F2I")))
			 "r5kf_fpu_iss")

; fxfer (mfc1, mfhc1, mtc1, mthc1) - single issue
(define_insn_reservation "r5kf_fxfer" 2 
                         (and (eq_attr "cpu" "5kc")
			      (and (eq_attr "type" "xfer")
				   (eq_attr "cnv_mode" "I2S,I2D,F2I")))
                         "r5kc_ixu_arith+r5kf_fpu_arith")

;;
;; End of DFA-based pipeline description for MIPS64 model R5Kc/f and
;; MIPS32 model R4Kc/m, R4Kp and M4K
;;
