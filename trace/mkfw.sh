export LCCDIR=../lcc/bin
BINUTILSDIR=../binutils/bin

${LCCDIR}/lcc -g -Wo-lccdir=${LCCDIR} -S -target=riscv32 $1.c
${LCCDIR}/lcc -g -Wo-lccdir=${LCCDIR} -S -target=riscv32 io.c
${BINUTILSDIR}/as -o start.o start.s
${BINUTILSDIR}/as -o io.o io.s
${BINUTILSDIR}/as -g -o $1.o $1.s
${BINUTILSDIR}/ld -g -h -m $1.mem -o $1.bin start.o io.o $1.o
python3 mkhex.py $1






