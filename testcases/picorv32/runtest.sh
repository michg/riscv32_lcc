export LCCDIR=../../lcc/bin
BINUTILSDIR=../../binutils/bin
${BINUTILSDIR}/as -o start.o start.s
rm  ./$1/result/*.*
${LCCDIR}/lcc -Wo-lccdir=${LCCDIR} -S -target=riscv32 ./$1/$1.c  -o ./$1/$1.s
${BINUTILSDIR}/as -o ./$1/result/$1.o ./$1/$1.s
libs=$(<./$1/libs.txt)
${BINUTILSDIR}/ld -h -m ./$1/$1.mem -o ./$1/result/$1.bin start.o ./$1/result/$1.o $libs
python3 mkhex.py ./$1/result/$1
cp ./$1/result/$1.hex ./firmware.mem
./simv >>./$1/result/$1.log
if cmp -s "./$1/$1.ref" "./$1/result/$1.log"
   then
      echo "Testcase $1 ok."
   else
      echo "Testcase $1 fail."
   fi

