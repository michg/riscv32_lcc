BINUTILSDIR=../../binutils/bin
LCCDIR=../../lcc/bin

rm -f -r result
mkdir result
for name in src/*/; do
	name=${name%/}
	name=${name#src/}
	SRCDIR=src/${name}
	RESDIR=result/${name}
	mkdir ${RESDIR}
	${LCCDIR}/lcc -Wo-lccdir=${LCCDIR} -S -g -target=riscv32 ${SRCDIR}/${name}.c -o ${RESDIR}/${name}.s
	${LCCDIR}/lcc -Wo-lccdir=${LCCDIR} -S -target=riscv32 io.c -o ${RESDIR}/io.s
	${BINUTILSDIR}/as -o ${RESDIR}/start.o start.s
	${BINUTILSDIR}/as -o ${RESDIR}/io.o ${RESDIR}/io.s
	${BINUTILSDIR}/as -g -o ${RESDIR}/${name}.o ${RESDIR}/${name}.s
    libs=$(<./src/${name}/libs.dep)
    objs=$(<./src/${name}/objs.dep)
	${BINUTILSDIR}/ld -g -h -o ${RESDIR}/${name}.bin ${libs} ${RESDIR}/start.o ${RESDIR}/io.o ${RESDIR}/${name}.o ${objs} 
	python3 mkhex.py ${RESDIR}/${name}
	cp ${RESDIR}/${name}.hex firmware.mem
	./simv >> ${RESDIR}/${name}.log
	if cmp -s "${SRCDIR}/${name}.ref" "${RESDIR}/${name}.log"
	then
		echo "Testcase ${name} ok." >>result/results.log
	else
		echo "Testcase ${name} fail." >>result/results.log
	fi 
done



