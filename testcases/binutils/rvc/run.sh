for name in *.s; do
   ../../../binutils/bin/as "-c" "-o" "results/${name%.s}.elf" "$name"
   ../../../binutils/bin/ld "-c" "-h" "-o" "results/${name%.s}.bin" "results/${name%.s}.elf"
   if cmp -s "${name%.s}.ref" "results/${name%.s}.bin"
   then
      echo "Testcase $name ok.">>results/summary.log
   else
      echo "Testcae $name fail.">>results/summary.log
   fi
done


