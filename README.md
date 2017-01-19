A 32-bit RISCV compiler toolchain, based on LCC(https://github.com/drh/lcc)
and binutils from eco32(https://github.com/hgeisse/eco32) with
testcases for iverilog(https://github.com/steveicarus/iverilog)
running on picorv32(https://github.com/cliffordwolf/picorv32).

Compilation of LCC with libraries and binutils:
./mkbinaries.sh

Running the testcases:
for binutils:
  cd testcases/binutils
  ./run.sh
A Summary of the results will be in ./results/summary.log.

For picorv32 (python3 and iverilog installation required):
cd testcases/picorv32
1.Build the simulation:
  ./buildsim.sh
2.Either run a single test:
  ./runtest.sh sort
  The result will ./sort/result/sort.log.
  or run all tests:
  ./runalltest.sh
  A Summary of the results will be in ./results.log.
