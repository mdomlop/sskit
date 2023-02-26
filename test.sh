echo Filling with $1 random entries...
for i in `seq 1 $1`; do src/makesnap.elf -a -i /Subvolumen -o /Directorio -f 1m -q $(tr -dc '[:digit:]' < /dev/urandom | head -c 4); done
echo Done

echo Reading...
src/makesnap.elf -p

