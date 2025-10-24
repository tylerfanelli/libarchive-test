all:
	gcc unzip.c -o unzip -larchive
clean:
	rm unzip
	rm -rf rootfs
