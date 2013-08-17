###############################################################################
# Common 
###############################################################################

#=========================================================#
# 
#=========================================================#
INCLUDE	= ./include

AS	= as
ASFLAGS = -g --32

LD = ld
LDFLAGS = -m elf_i386


CC	= gcc34
CFLAGS	= -g -m32 -O -Wall \
	  -fno-builtin -nostdinc -I$(INCLUDE) \
	  -finline-functions #-fomit-frame-pointer

CPP	= gcc34 -E
CPPFLAGS= -g -nostdinc -I$(INCLUDE)

AR	= ar
RM	= rm -fr

###############################################################################
# MAKELEVEL == 0
ifeq (0, $(MAKELEVEL))
###############################################################################
SRCDIR = $(shell pwd)
TPLDIR = $(SRCDIR)/template
OBJDIR = $(SRCDIR)/../obj
BXSDIR = $(SRCDIR)/../bxs

export PATH	+= :$(SRCDIR)/bin

RAMDISK =#-DRAMDISK=512
ROOT_DEV=#FLOPPY
SWAP_DEV=

#=========================================================#
# 
#=========================================================#

BOOT	= boot/boot.a
MODS	= \
		init/init.a \
		mm/mm.o \
		kernel/kernel.o \
		fs/fs.o \
		fs/minix/minix.o \
		kernel/chr_drv/chr_drv.a \
		kernel/blk_drv/blk_drv.a \
		kernel/math/math.a \
		lib/lib.a

all : kernel.img

backup : clean
	cd ../../ && tar -cjf linux.tar.gz linux

kernel.img : compile
	rm -f $(BXSDIR)/kernel.img
	@$(AR) -p $(addprefix $(OBJDIR)/, $(BOOT)) bootsect.o > $(OBJDIR)/bootsect.o
	$(LD) $(LDFLAGS) -Ttext 0 -o bootsect $(OBJDIR)/bootsect.o ; rm -fr $(OBJDIR)/bootsect.o
	@objcopy -R .pdr -R .comment -R .note -O binary -S bootsect
	@$(AR) -p $(addprefix $(OBJDIR)/, $(BOOT)) setup.o > $(OBJDIR)/setup.o
	$(LD) $(LDFLAGS) -Ttext 0 -o setup $(OBJDIR)/setup.o ; rm -fr $(OBJDIR)/setup.o
	@objcopy -R .pdr -R .comment -R .note -O binary -S setup
	$(LD) $(LDFLAGS) -Ttext 0 -o $(OBJDIR)/system.o $(addprefix $(OBJDIR)/, $(MODS))
	@objcopy -R .pdr -R .comment -R .note -O binary $(OBJDIR)/system.o system
	build bootsect setup system kernel.img $(ROOT_DEV)
	@rm -f bootsect setup system $(OBJDIR)/head.o
	@mv kernel.img $(BXSDIR)/ && echo "Create kernel.img"
	
compile : 
	@for module in $(BOOT) $(MODS); do \
		$(MAKE) all -e TARGET=$$(basename $$module) -C $$(dirname $$module) -f $(TPLDIR)/compile.mak || exit 1; \
	done
	
#in case timestamp disturbed
clean : 
	@for module in $(BOOT) $(MODS); do \
		$(MAKE) clean -e TARGET=$$(basename $$module) -C $$(dirname $$module) -f $(TPLDIR)/compile.mak || exit 1; \
	done
	rm -fr $(BXSDIR)/kernel.img $(OBJDIR)/system.o

#in case file deleted
cleanall : 
	rm -fr $(OBJDIR) $(BXSDIR)/kernel.img
	
###############################################################################
# MAKELEVEL > 0
# LDFLAGS	:
# SRCS		:
# CUR_DIR	:
else
###############################################################################
#=========================================================#
# 
#=========================================================#
OBJM	= $(PATHM)/$(TARGET)
OBJS	= $(addprefix $(PATHM)/, \
			$(patsubst %.s, %.o, $(filter %.s, $(SRCS))) \
			$(patsubst %.S, %.o, $(filter %.S, $(SRCS))) \
			$(patsubst %.c, %.o, $(filter %.c, $(SRCS))) \
		  )
DEPS	= $(addprefix $(PATHM)/, \
			$(patsubst %.c, %.d, $(filter %.c, $(SRCS))) \
			$(patsubst %.S, %.d, $(filter %.S, $(SRCS))) \
		  )

#=========================================================#
# 
#=========================================================#
$(OBJDIR)/%.o : %.c
	$(CC) $(CFLAGS) -c -o $@ $<

$(OBJDIR)/%.o : %.s
	$(AS) $(ASFLAGS) -o $@ $<

$(OBJDIR)/%.o : %.S
	$(CPP) $(CPPFLAGS) -o $(<:.S=.s) $<
	$(AS) $(ASFLAGS) -o $@ $(<:.S=.s)
	$(RM) $(<:.S=.s)

$(OBJDIR)/%.d : %.c
	$(CPP) $(CPPFLAGS) -MT '$@ $(@:.d=.o)' -MM $< > $@
	
$(OBJDIR)/%.d : %.S
	$(CPP) $(CPPFLAGS) -MT '$@ $(@:.d=.o)' -MM $< > $@

#=========================================================#
# 
#=========================================================#
all : makedir $(OBJM) ;

makedir :
	@test ! -d "$(PATHM)" && mkdir -p $(PATHM)
	
$(OBJM) : $(OBJS)
  ifeq (.a, $(suffix $(OBJM)))
	$(AR) rcs $(OBJM) $(OBJS)
  else ifeq (.o, $(suffix $(OBJM)))
	$(LD) $(LDFLAGS) -r -o $(OBJM) $(OBJS)
  endif

ifneq ($(MAKECMDGOALS),clean)
  -include $(DEPS)
endif

clean:
	rm -fr $(OBJM) $(OBJS) $(DEPS)

###############################################################################
# MAKELEVEL end
endif
###############################################################################



