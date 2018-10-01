LCC=../../..
BINUTILSDIR=${LCC}/binutils/bin
LCCDIR=${LCC}/lcc/bin
RESDIR=./res

rm -f -r ${RESDIR}
mkdir ${RESDIR}  
${BINUTILSDIR}/as -o ${RESDIR}/crt1.o crt1.s
${LCCDIR}/lcc -Wo-lccdir=${LCCDIR} -Wf-hf -S -target=riscv32 es.c -o ${RESDIR}/es.s 
${BINUTILSDIR}/as -o ${RESDIR}/es.o ${RESDIR}/es.s 
${LCCDIR}/lcc -Wo-lccdir=${LCCDIR} -Wf-hf -S -target=riscv32 main.c -o ${RESDIR}/main.s
${BINUTILSDIR}/as -o ${RESDIR}/main.o ${RESDIR}/main.s 
${BINUTILSDIR}/ld -h -o ./res/pulpino.bin ${RESDIR}/crt1.o ${RESDIR}/es.o ${RESDIR}/main.o  



