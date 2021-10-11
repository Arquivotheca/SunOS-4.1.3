#
# @(#)Makefile 1.1 92/07/30 SMI;
#

#	libm F directory Makefile
#	for machine-independent single-precision C calling convention functions

#	The following are expected to be defined outside this makefile:

#	FP=		floating-point option
#	../OBJDIR=	name of subdirectory to build libraries
#	IL=		inline expansion template file

#	The target libraries for this version:

OBJD=	$(WD)/../$(OBJDIR)
LIB = 	$(OBJD)/libm.a
LIBP = 	$(OBJD)/libm_p.a
MIDIR=  $(OBJD)/midir

#	These machine-independent sources are local:

SRCS= 	\
Facos.c		Fatan.c		Fexp10.c	Fnint.c		Ftan.c \
Faint.c		Fcos.c		Fexp2.c		Fsin.c		Ftanh.c \
Fanint.c	Fcosh.c		Flog.c		Fsinh.c \
Fasin.c		Fexp.c		Flog10.c	Fsqrt.c 

#	These are machine-specific objects of machine-independent sources
#	from SRCS, but they may be replaced by machine-dependent versions later:

OBJS = 	\
   $(MIDIR)/Facos.o	   $(MIDIR)/Fcosh.o	   $(MIDIR)/Fnint.o \
   $(MIDIR)/Faint.o	   $(MIDIR)/Fexp.o	   $(MIDIR)/Fsin.o \
   $(MIDIR)/Fanint.o	   $(MIDIR)/Fexp10.o	   $(MIDIR)/Fsinh.o \
   $(MIDIR)/Fasin.o	   $(MIDIR)/Fexp2.o	   $(MIDIR)/Fsqrt.o \
   $(MIDIR)/Fatan.o	   $(MIDIR)/Flog.o	   $(MIDIR)/Ftan.o \
   $(MIDIR)/Fcos.o	   $(MIDIR)/Flog10.o	   $(MIDIR)/Ftanh.o

.DEFAULT:
	sccs get -G$@ $@

all:	$(SRCS) $(OBJS) $(LIB)
	@echo done making F for $(LIB)

$(LIB):	$(OBJS)
	ar cru $(LIB) $(OBJS)

$(OBJS):
	$(CC) $(CFLAGS) -fsingle -DIEEE -c -$(FP) `basename $(@F) .o`.c $(IL) -p
	$(LD) $(PLDFLAG) -r $(@F)
	mv a.out $(@F)
	mv $(@F) $(MIDIR)/profiled
	$(CC) $(CFLAGS) -fsingle -DIEEE -c -$(FP) `basename $(@F) .o`.c $(IL)
	$(LD) -x -r $(@F)
	mv a.out $(@F)
	mv $(@F) $(MIDIR)

FORCEMAKE:

$(MIDIR)/Fcos.o: Fcos.c $(IL)
$(MIDIR)/Fsin.o: Fsin.c $(IL)
$(MIDIR)/Ftan.o: Ftan.c $(IL)
$(MIDIR)/Facos.o: Facos.c $(IL)
$(MIDIR)/Fasin.o: Fasin.c $(IL)
$(MIDIR)/Fatan.o: Fatan.c $(IL)
$(MIDIR)/Fcosh.o: Fcosh.c $(IL)
$(MIDIR)/Fsinh.o: Fsinh.c $(IL)
$(MIDIR)/Ftanh.o: Ftanh.c $(IL)
$(MIDIR)/Fexp.o: Fexp.c $(IL)
$(MIDIR)/Fexp2.o: Fexp2.c $(IL)
$(MIDIR)/Fexp10.o: Fexp10.c $(IL)
$(MIDIR)/Flog.o: Flog.c $(IL)
$(MIDIR)/Flog10.o: Flog10.c $(IL)
$(MIDIR)/Fsqrt.o: Fsqrt.c $(IL)
$(MIDIR)/Faint.o: Faint.c $(IL)
$(MIDIR)/Fanint.o: Fanint.c $(IL)
$(MIDIR)/Fnint.o: Fnint.c $(IL)
