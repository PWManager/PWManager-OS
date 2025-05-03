buildos:
	gcc -ffreestanding -m32 -c kernel.c -o kernel.o
	ld -m elf_i386 -T link.ld -o kernel.bin kernel.o
	mv kernel.bin build/isodir/boot/kernel.bin
	grub-mkrescue -o build/myos.iso build/isodir

run:
	qemu-system-i386 -cdrom build/myos.iso

prepare:
	mkdir build
	mkdir build/isodir
	mkdir build/isodir/boot
	mkdir build/isodir/boot/grub
	echo "set default=0\nset timeout=1\n\nmenuentry "PWManager OS" {\n	multiboot /boot/kernel.bin\n}" > build/isodir/boot/grub/grub.cfg

clean:
	rm build/myos.iso
	rm build/isodir/boot/kernel.bin