gcc -g zpipe.c -o zpipe -lz
gcc -g convert_payload.c -o convert_payload
gcc -g pack_payload.c -o pack_payload

./zpipe < $1 > boot.elf.zlib
truncate -s %4 boot.elf.zlib
./pack_payload PONY boot.elf.zlib payload.bin
./convert_payload payload.bin out.bin
convert -depth 8 -size $(expr $(stat -c%s out.bin) / 4)x1+0 rgba:out.bin payload.png
rm -f boot.elf.zlib payload.bin out.bin
