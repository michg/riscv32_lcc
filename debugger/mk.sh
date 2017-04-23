export LCCDIR=../lcc/bin
BINUTILSDIR=../binutils/bin


${LCCDIR}/lcc -Wo-lccdir=${LCCDIR} -S -target=riscv32 gdbstub.c
${LCCDIR}/lcc -g -Wo-lccdir=${LCCDIR} -S -target=riscv32 main.c
${BINUTILSDIR}/as -o starterirq.o starterirq.s
${BINUTILSDIR}/as -o gdbstub.o gdbstub.s
${BINUTILSDIR}/as -g -o main.o main.s
${BINUTILSDIR}/ld -g -h -m firmware.mem -o firmware.bin starterirq.o gdbstub.o main.o
python3 mkhex.py firmware
cp firmware.deb main.dbg





