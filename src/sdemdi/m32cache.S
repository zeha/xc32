/*
 * m32cache.s: MIPS32 cache support functions 
 */


#if __mips == 32 || __mips == 64 || __mips16 >= 2
#include <mips/asm.h>
#include <mips/regdef.h>
#include <mips/m32c0.h>

#define DECL(x, val)		\
	.sdata;			\
	EXPORTS(x, 4);		\
	.word	val
	
DECL(mips_dcache_linesize,-1)
DECL(mips_scache_linesize,-1)
	
#define tmp		t0	
#define cfg		t1
#define icachesize	t2
#define ilinesize	t3
#define dcachesize	ta0
#define dlinesize	ta1
#define scachesize	ta2	
#define slinesize	ta3

/* Don't try to compile for mips16 */
	.set nomips16
		
/*
 * void mips_init_cache()
 * 
 * Work out size of and initialize I, D & S caches.
 */
	.section .text.boot, "ax", @progbits
	.global mips_init_cache
	.ent mips_init_cache
mips_init_cache:	
	/*
 	 * Determine the cache sizes
	 */
	mfc0	cfg,$config,0
	
	li	icachesize,0
	li	ilinesize,0
	li	dcachesize,0
	li	dlinesize,0
	li	scachesize,0
	li	slinesize,0
	
	/* check that we have Config1 */
	and	tmp,cfg,CFG0_M
	mfc0	cfg,$config,1
	beqz	tmp,9f
	
	/* get icache line size (log2) */
	and	tmp,cfg,CFG1_ILMASK
	srl	tmp,CFG1_ILSHIFT
	beqz	tmp,8f		# no i-cache
	addu	tmp,1
	
	/* get number of icache ways */
	and	icachesize,cfg,CFG1_IAMASK
	srl	icachesize,CFG1_IASHIFT
	addu	icachesize,1
		
	/* total icache size = lines/way * linesize * ways */
	li	ilinesize,1
	sll	ilinesize,tmp
	sll	icachesize,tmp
	
	/* get icache lines per way */
	and	tmp,cfg,CFG1_ISMASK
	srl	tmp,CFG1_ISSHIFT
	addu	tmp,6
	sll	icachesize,tmp
	
	/* get dcache line size (log2) */
8:	and	tmp,cfg,CFG1_DLMASK
	srl	tmp,CFG1_DLSHIFT
	beqz	tmp,8f		# no d-cache
	addu	tmp,1
	
	/* get number of dcache ways */
	and	dcachesize,cfg,CFG1_DAMASK
	srl	dcachesize,CFG1_DASHIFT
	addu	dcachesize,1
	
	/* total dcache size = lines/way * linesize * ways */
	li	dlinesize,1
	sll	dlinesize,tmp
	sll	dcachesize,tmp
	
	/* get dcache lines per way */
	and	tmp,cfg,CFG1_DSMASK
	srl	tmp,CFG1_DSSHIFT
	addu	tmp,6
	sll	dcachesize,tmp
	
8:	/* check that we have Config2 */
	and	tmp,cfg,CFG0_M
	mfc0	cfg,$config,2
	beqz	tmp,9f
	
	/* get scache line size (log2) */
	and	tmp,cfg,CFG2_SLMASK
	srl	tmp,CFG2_SLSHIFT
	beqz	tmp,8f		# no s-cache
	addu	tmp,1
	
	/* get number of scache ways */
	and	scachesize,cfg,CFG2_SAMASK
	srl	scachesize,CFG2_SASHIFT
	addu	scachesize,1
	
	/* scache size = lines/way * linesize * ways */
	li	slinesize,1
	sll	slinesize,tmp
	sll	scachesize,tmp
	
	/* get scache lines per way */
	and	tmp,cfg,CFG2_SSMASK
	srl	tmp,CFG2_SSSHIFT
	addu	tmp,6
	sll	scachesize,tmp
	
9:		
#undef	cfg
#undef	tmp
	
	/* run uncached */
	la	t1,1f
	or	t1,KSEG1_BASE
	jr	t1
1:
	
	/* disable all i/u and cache exceptions */
	.set noreorder
	mfc0	t0,$sr
	li	t1,~SR_IE
	and	t1,t0
	or	t1,SR_ERL		# will this work?
	mtc0	t1,$sr
#if !__mipsr2
	ssnop; ssnop
#endif	
	ehb

	mtc0	zero,$errctl
	mtc0	zero,$taglo		# taglo / 20Kc itaglo
	mtc0	zero,$taghi		# taghi / 20Kc itaghi
	mtc0	zero,$taglo,2		# 20Kc dtaglo
	mtc0	zero,$taghi,2		# 20Kc dtaghi
	mtc0	zero,$taglo,4		# 25Kf l23taglo
	mtc0	zero,$taghi,4		# 25Kf l23taghi
	
	.set reorder

4:		
	/* 
	 * Initialise primary instruction cache.
	 */
	.set	noreorder
	li	a0,KSEG0_BASE
	beqz	icachesize,8f
	addu	a1,a0,icachesize		# limit = base + icachesize 
1:	addu	a0,ilinesize
	bne	a0,a1,1b
	cache	Index_Store_Tag_I,-4(a0)	# BDSLOT: clear tag
	.set	reorder

	/* 
	 * Initialise primary data cache.
	 */
	.set	noreorder
8:	li	a0,KSEG0_BASE
	beqz	dcachesize,8f
	addu	a1,a0,dcachesize        	# limit = base + dcachesize 
1:	addu	a0,dlinesize
	bne	a0,a1,1b
	cache	Index_Store_Tag_D,-4(a0)	# BDSLOT: clear tag
	.set	reorder
	
	/* 
	 * Initialise secondary cache.
	 */
	.set	noreorder
8:	li	a0,KSEG0_BASE
	beqz	scachesize,8f
	addu	a1,a0,scachesize        	# limit = base + scachesize 
1:	addu	a0,slinesize
	bne	a0,a1,1b
	cache	Index_Store_Tag_S,-4(a0)	# BDSLOT: clear tag
	.set	reorder
	
8:	sync
	
	/* we store the sizes only after the caches are initialised */
4:
	sw	dlinesize,mips_dcache_linesize
	sw	slinesize,mips_scache_linesize
	
	.set	noreorder
	mtc0	t0,$sr
#if !__mipsr2
	ssnop; ssnop
#endif	
	.set	reorder
	
	jr.hb	ra
.end mips_init_cache

	
/*
 * void mips_clean_cache (unsigned kva, size_t n)
 *
 * Writeback and invalidate address range in all caches
 */
LEAF(mips_clean_cache)
	lw a2,mips_dcache_linesize
	
	/* bad size? */
 	blez	a1,11f
	
	/* no dcache? */
	blez	a2,8f
	
#define addr	t0
#define maxaddr	t1
#define mask	t2
	/* align to line boundaries */
	addu	maxaddr,a0,a1
	subu	mask,a2,1
	not	mask
	and	addr,a0,mask
	addu	maxaddr,-1
	and	maxaddr,mask
	.set	noreorder
1: 	cache	Hit_Writeback_Inv_D,0(addr)
 	cache	Hit_Invalidate_I,0(addr)
	bne     addr,maxaddr,1b
	addu   	addr,a2
	.set	reorder
	
	sync
	
8:	lw a2,mips_scache_linesize
	blez	a2,8f
	
	/* align to line boundaries */
	addu	maxaddr,a0,a1
	subu	mask,a2,1
	not	mask
	and	addr,a0,mask
	addu	maxaddr,-1
	and	maxaddr,mask
	.set	noreorder
1: 	cache	Hit_Writeback_Inv_S,0(addr)
	bne     addr,maxaddr,1b
	addu   	addr,a2
	.set	reorder
	
	sync
	
11:;8:	jr.hb	ra
END(mips_clean_cache)

/*
 * void m32_sync_icache (unsigned kva, size_t n)
 *
 * Synchronise icache and dcache for virtual address range 
 */
LEAF(mips_sync_icache)
#if __mips_isa_rev < 2
	b	mips_clean_cache
#else
	/* check for bad size */
	addu	maxaddr,a0,a1
	blez	a1,9f
	
	/* get synci step and skip if not required */
	rdhwr	a2,$1
	addu	maxaddr,-1
	beqz	a2,9f	
	
	/* align to line boundaries */
	subu	mask,a2,1
	not	mask
	and	addr,a0,mask
	and	maxaddr,mask
	
	/* the cacheop loop */
	.set	noreorder
10: 	synci	0(addr)
	bne     addr,maxaddr,10b
	addu   	addr,a2
	.set	reorder
	
	sync
9:	jr.hb	ra
#endif	
END(mips_sync_icache)
	
#endif
