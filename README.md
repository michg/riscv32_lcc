###A 32-bit RISCV compiler toolchain (c-compiler, assembler and linker)
based on lcc (https://github.com/drh/lcc)
and binutils from eco32 (https://github.com/hgeisse/eco32) with
testcases for iverilog (https://github.com/steveicarus/iverilog)
running on picorv32 (https://github.com/cliffordwolf/picorv32).

#####Compilation of lcc with libraries and binutils:      
>./mkbinaries.sh

#####Running the testcases:      
for binutils testcase:  
>cd testcases/binutils  
>./run.sh  

A summary of the results will be in ./results/summary.log.  

For picorv32 testcase (python3 and iverilog installation required):  
> cd testcases/picorv32  
First build the simulation:    
> ./buildsim.sh  

Then either run a single test:      
> ./runtest.sh sort  

The result will be in ./sort/result/sort.log.  
The simulation output will be in ./sort/result/result.log
Or run all tests:    
> ./runalltest.sh  

A summary of the results will be in ./results.log.
