global target_triplet

if { [istarget "mips*-sde*-elf*"] } {
	set torture_execute_xfail "$target_triplet"
}

return 0
