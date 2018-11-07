export LCCDIR=../../lcc/bin
BINUTILSDIR=../../binutils/bin


${LCCDIR}/lcc -Wo-lccdir=${LCCDIR} -S -target=riscv32 sdmm.c
${LCCDIR}/lcc -Wo-lccdir=${LCCDIR} -S -target=riscv32 ff.c
${LCCDIR}/lcc -Wo-lccdir=${LCCDIR} -S -target=riscv32 loader.c
${LCCDIR}/lcc -Wo-lccdir=${LCCDIR} -S -target=riscv32 xprintf.c
${BINUTILSDIR}/as -o start.o start.s
${BINUTILSDIR}/as -o sdmm.o sdmm.s
${BINUTILSDIR}/as -o ff.o ff.s
${BINUTILSDIR}/as -o loader.o loader.s
${BINUTILSDIR}/as -o xprintf.o xprintf.s
${BINUTILSDIR}/ld -h -m firmware.mem -o firmware.bin start.o ff.o sdmm.o xprintf.o loader.o
python3 mkhex.py firmware






