surffs_inode.o: src/surffs_inode.c
	cc -c src/surffs_inode.c

surffs_dentry.o: src/surffs_dentry.c
	cc -c src/surffs_dentry.c

surffs_sb.o: src/surffs_sb.c
	cc -c src/surffs_sb.c
	
surffs_ops.o: src/surffs_ops.c
	cc -c src/surffs_ops.c	

surffs_debug.o: src/surffs_debug.c
	cc -c src/surffs_debug.c
	
surffs_helpers.o: src/surffs_helpers.c
	cc -c src/surffs_helpers.c	
	
surffs_socket.o: src/surffs_socket.c
	cc -c src/surffs_socket.c		
	
surffs_internet.o: src/surffs_internet.c
	cc -c src/surffs_internet.c	
	
surffs_parser.o: src/surffs_parser.c
	cc -c src/surffs_parser.c		

surffs_webpages.o: src/surffs_webpages.c
	cc -c src/surffs_webpages.c			

surffs_main.o: src/surffs_main.c
	cc -c src/surffs_main.c


obj-m += surffs.o
surffs-objs := 	src/surffs_inode.o \
				src/surffs_dentry.o \
				src/surffs_sb.o \
				src/surffs_ops.o \
				src/surffs_debug.o \
				src/surffs_helpers.o \
				src/surffs_socket.o \
				src/surffs_internet.o \
				src/surffs_parser.o \
				src/surffs_webpages.o \
				src/surffs_main.o



all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean

