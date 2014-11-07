
	.text
func:	
	addiu   $2, $3, 5
        lw      $4, 0($2)

        .set    noreorder
        beq     $3, 0, .L1
        sw      $3, 4($2)
        .set    reorder

.L1:
        lw      $5, 16($2)

        .p2align        4
