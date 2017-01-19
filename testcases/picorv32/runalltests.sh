LCCDIR=../../lcc/bin
BINUTILSDIR=../../binutils/bin
${BINUTILSDIR}/as -o start.o start.s
rm results.log
for name in */; do
    name=${name%/}
    rm  ./$name/result/*.*
    ${LCCDIR}/lcc -Wo-lccdir=../../lcc/bin -S -target=riscv32 ./$name/$name.c  -o ./$name/$name.s
    ${BINUTILSDIR}/as -o ./$name/result/$name.o ./$name/$name.s
    libs=$(<./$name/libs.txt)
    ${BINUTILSDIR}/ld -h -o ./$name/result/$name.bin start.o ./$name/result/$name.o $libs
    python3 mkhex.py ./$name/result/$name
    cp ./$name/result/$name.hex ./firmware.mem
    ./simv >>./$name/result/$name.log
    if cmp -s "./$name/$name.ref" "./$name/result/$name.log"
    then
      echo "Testcase $name ok.">>results.log
    else
      echo "Testcase $name fail.">>results.log
    fi
done
