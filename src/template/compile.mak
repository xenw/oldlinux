
SUBDIR=$(shell pwd)
SRCDIR=$(shell \
	srcdir=$(SUBDIR) ; \
	while [ "/" != "$$srcdir" -o "" != "$$srcdir" ] ; do \
		if [ -f "$$srcdir/Makefile" ] ; then \
			break ; \
		else \
			srcdir="$$(dirname $$srcdir)" ; \
		fi \
	done ; \
	echo $$srcdir \
	)

export OBJDIR = $(SRCDIR)/../obj
export PATHM = $(subst $(SRCDIR), $(OBJDIR), $(SUBDIR))
export INCLUDE	= $(SRCDIR)/include
export SRCS=$(wildcard *.s) $(wildcard *.S) $(wildcard *.c)

default : all ;
.DEFAULT :
	$(MAKE) $@ -e TARGET=$(TARGET) -C $(SRCDIR) --no-print-directory
