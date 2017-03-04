export LCCDIR=../../lcc/bin
BINUTILSDIR=../../binutils/bin
LIBDIR=${LCCDIR}/libs
olist=""

function extract()
{
while read p; do
      file=$(../../${BINUTILSDIR}/ar -f ../../${LIBDIR}/$2 $p)
      if [[ ${olist} != *"${file}"* ]]
      then
         olist+=" ${file}"
      fi
      if [ -n ${file} ] && [ ! -s "$p.res" ]
      then
          ../../${BINUTILSDIR}/dof -u ${file} > $p.res
          extract $p.res $2
      fi
done < $1
}

rm  ./$1/result/*.*
${LCCDIR}/lcc -Wo-lccdir=${LCCDIR} -S -target=riscv32 ./$1/$1.c  -o ./$1/$1.s
${BINUTILSDIR}/as -o ./$1/result/main.o  start.s ./$1/$1.s
${BINUTILSDIR}/dof -u ./$1/result/main.o > ./$1/result/main.res
lib=$(<./$1/lib.txt)
objs=$(<./$1/objs.txt)
cd ./$1/result
if [ ${lib} ]
then
    extract main.res ${lib}
fi
../../${BINUTILSDIR}/ld -h -m $1.mem -o $1.bin main.o ${olist} ${objs}
cd ../..
python3 mkhex.py ./$1/result/$1
cp ./$1/result/$1.hex ./firmware.mem
./simv >>./$1/result/$1.log
if cmp -s "./$1/$1.ref" "./$1/result/$1.log"
   then
      echo "Testcase $1 ok."
   else
      echo "Testcase $1 fail."
   fi




