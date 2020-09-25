BINUTILSDIR=../../binutils/bin
LCCDIR=../../lcc/bin
SRCDIR=./src/$1
RESDIR=./result/$1

rm -f -r result
mkdir result 
mkdir ${RESDIR}
${LCCDIR}/lcc -Wo-lccdir=${LCCDIR} -S -g -target=riscv32 ${SRCDIR}/$1.c -o ${RESDIR}/$1.s
${LCCDIR}/lcc  -Wo-lccdir=${LCCDIR} -S -target=riscv32 io.c -o ${RESDIR}/io.s
${BINUTILSDIR}/as -o ${RESDIR}/start.o start.s
${BINUTILSDIR}/as -o ${RESDIR}/io.o ${RESDIR}/io.s
${BINUTILSDIR}/as -g -o ${RESDIR}/$1.o ${RESDIR}/$1.s
libs=$(<./src/$1/libs.dep)
objs=$(<./src/$1/objs.dep)
${BINUTILSDIR}/ld -g -h -o ${RESDIR}/$1.bin ${libs} ${RESDIR}/start.o ${RESDIR}/io.o ${RESDIR}/$1.o ${objs}
python3 mkhex.py ${RESDIR}/$1
cp ${RESDIR}/$1.hex firmware.mem
./simv >> ${RESDIR}/$1.log
if cmp -s "${SRCDIR}/$1.ref" "${RESDIR}/$1.log"
   then
      echo "Testcase $1 ok."
   else
      echo "Testcase $1 fail."
   fi 



