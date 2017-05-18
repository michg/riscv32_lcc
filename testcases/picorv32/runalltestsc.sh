export LCCDIR=../../lcc/bin
BINUTILSDIR=../../binutils/bin
LIBDIR=${LCCDIR}/libs


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
rm results.log
for name in */; do
    olist=""
    name=${name%/}
    rm  ./$name/result/*.*
    ${LCCDIR}/lcc -Wo-lccdir=${LCCDIR} -S -target=riscv32 ./$name/$name.c  -o ./$name/$name.s
    ${BINUTILSDIR}/as -c -o ./$name/result/main.o  start.s ./$name/$name.s
    ${BINUTILSDIR}/dof -u ./$name/result/main.o > ./$name/result/main.res
    lib=$(<./$name/lib.txt)
    objs=$(<./$name/objs.txt)
    cd ./$name/result
    if [ ${lib} ]
    then
        extract main.res ${lib}
    fi
    ../../${BINUTILSDIR}/ld -c -h -m $name.mem -o $name.bin main.o ${olist} ${objs}
    cd ../..
    python3 mkhex.py ./$name/result/$name
    cp ./$name/result/$name.hex ./firmware.mem
    ./simv >>./$name/result/$name.log
    if cmp -s "./$name/$name.ref" "./$name/result/$name.log"
    then
      echo "Testcase $name ok." >>results.log
    else
      echo "Testcase $name fail." >>results.log
    fi

done


