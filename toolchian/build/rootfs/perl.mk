all:
	$(Q)+sb2 make CC=gcc -f Makefile.micro
	$(Q)+sb2 make install cp microperl /usr/bin/perl -av
