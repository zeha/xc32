# Target: MIPS SDE
TDEPFILES= mips-tdep.o mips-sde-tdep.o remote-mips.o
TM_FILE= tm-sde.h
MT_CFLAGS=-DMIPSSDE

SIM_OBS = remote-sim.o
SIM = ../sim/mips/libsim.a
