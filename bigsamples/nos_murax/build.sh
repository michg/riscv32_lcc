export LCCDIR=../../lcc/bin
export BINUTILSDIR=../../binutils/bin


${LCCDIR}/lcc -Wo-lccdir=${LCCDIR} -S -target=riscv32 nosalarm.c
${LCCDIR}/lcc -Wo-lccdir=${LCCDIR} -S -target=riscv32 nosbarrier.c
${LCCDIR}/lcc -Wo-lccdir=${LCCDIR} -S -target=riscv32 nosevent.c
${LCCDIR}/lcc -Wo-lccdir=${LCCDIR} -S -target=riscv32 nosflag.c
${LCCDIR}/lcc -Wo-lccdir=${LCCDIR} -S -target=riscv32 noslist.c
${LCCDIR}/lcc -Wo-lccdir=${LCCDIR} -S -target=riscv32 nosmem.c
${LCCDIR}/lcc -Wo-lccdir=${LCCDIR} -S -target=riscv32 nosmutex.c
${LCCDIR}/lcc -Wo-lccdir=${LCCDIR} -S -target=riscv32 nosqueue.c
${LCCDIR}/lcc -Wo-lccdir=${LCCDIR} -S -target=riscv32 nossched.c
${LCCDIR}/lcc -Wo-lccdir=${LCCDIR} -S -target=riscv32 nossem.c
${LCCDIR}/lcc -Wo-lccdir=${LCCDIR} -S -target=riscv32 nossignal.c
${LCCDIR}/lcc -Wo-lccdir=${LCCDIR} -S -target=riscv32 nosthread.c
${LCCDIR}/lcc -Wo-lccdir=${LCCDIR} -S -target=riscv32 nostime.c
${LCCDIR}/lcc -Wo-lccdir=${LCCDIR} -S -target=riscv32 nostimer.c
${LCCDIR}/lcc -Wo-lccdir=${LCCDIR} -S -target=riscv32 nosport.c
${LCCDIR}/lcc -Wo-lccdir=${LCCDIR} -S -target=riscv32 clib.c
${LCCDIR}/lcc -Wo-lccdir=${LCCDIR} -S -target=riscv32 main.c
${LCCDIR}/lcc -Wo-lccdir=${LCCDIR} -S -target=riscv32 strlen.c
${LCCDIR}/lcc -Wo-lccdir=${LCCDIR} -S -target=riscv32 memcpy.c
${BINUTILSDIR}/as -o boot.o bootlcc.s
${BINUTILSDIR}/as -o nosportasm.o nosportasm.s
${BINUTILSDIR}/as -o nosport.o nosport.s
${BINUTILSDIR}/as -o nosalarm.o nosalarm.s
${BINUTILSDIR}/as -o nosbarrier.o nosbarrier.s
${BINUTILSDIR}/as -o nosevent.o nosevent.s
${BINUTILSDIR}/as -o nosflag.o nosflag.s
${BINUTILSDIR}/as -o noslist.o noslist.s
${BINUTILSDIR}/as -o nosmem.o nosmem.s
${BINUTILSDIR}/as -o nosmutex.o nosmutex.s
${BINUTILSDIR}/as -o nosqueue.o nosqueue.s
${BINUTILSDIR}/as -o nossched.o nossched.s
${BINUTILSDIR}/as -o nossem.o nossem.s
${BINUTILSDIR}/as -o nossignal.o nossignal.s
${BINUTILSDIR}/as -o nosthread.o nosthread.s
${BINUTILSDIR}/as -o nostime.o nostime.s
${BINUTILSDIR}/as -o nostimer.o nostimer.s
${BINUTILSDIR}/as -o clib.o clib.s
${BINUTILSDIR}/as -o main.o main.s
${BINUTILSDIR}/as -o strlen.o strlen.s
${BINUTILSDIR}/as -o memcpy.o memcpy.s

${BINUTILSDIR}/ld -h -m firmware.mem -rc 0x80000000 -rd 0x80010000 -rb 0x8001C000 -l ${LCCDIR}/libs/string.lib -o Murax.bin boot.o nosportasm.o nosport.o nosalarm.o nosbarrier.o nosevent.o nosflag.o noslist.o nosmem.o nosmutex.o nosqueue.o nossched.o nossem.o nossignal.o nosthread.o nostime.o nostimer.o clib.o main.o strlen.o memcpy.o
python3 mkhex.py Murax
