
CC_FOR_BUILD = gcc
CCFLAGS_FOR_BUILD = -O2

gen-pic32c-mmcu-specs: pic32c-devices.c gen-pic32c-mmcu-specs.c ../../version.c
	$(CC_FOR_BUILD) $(CCFLAGS_FOR_BUILD) -DTARGET_MCHP_PIC32C -DBASEVER="  " -DPKGVERSION="\"\"" pic32c-devices.c gen-pic32c-mmcu-specs.c ../../version.c -o $@ 

s-device-specs: gen-pic32c-mmcu-specs
	rm -rf device-specs
	mkdir device-specs && cd device-specs && $(RUN_GEN) ../$< $(STAMP) s-device-specs