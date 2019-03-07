AM_CFLAGS	= -W -Wall -Wextra -Wno-unused
DISTCLEANFILES	= *~ DEADJOE semantic.cache *.gdb *.elf core core.* *.d
doc_DATA	= README.md LICENSE
EXTRA_DIST	= README.md LICENSE

bin_PROGRAMS	= solicit
solicit_SOURCES = solicit.c
sbin_PROGRAMS	= mrdisc
mrdisc_SOURCES	= mrdisc.c common.c if.c if.h inet.c inet.h

release: distcheck
	@for file in $(DIST_ARCHIVES); do	\
		md5sum $$file > ../$$file.md5;	\
	done
	@mv $(DIST_ARCHIVES) ../
	@echo
	@echo "Resulting release files in ../"
	@echo "================================================================="
	@for file in $(DIST_ARCHIVES); do					\
		printf "%-32s Distribution tarball\n" $$file;			\
		printf "%-32s " $$file.md5; cat ../$$file.md5 | cut -f1 -d' ';	\
	done
