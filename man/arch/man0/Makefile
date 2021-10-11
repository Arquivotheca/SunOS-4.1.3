#  @(#)Makefile 1.1 92/07/30 SMI;
#
#  Makes a permuted index for the UNIX Reference Manuals
#

FORMATTER = /usr/local/iroff

TOCX	= tocx1 tocx2 tocx3 tocx4 tocx5 tocx6 tocx7 tocx8
TOC 	= toc1 toc2 toc3 toc4 toc5 toc6 toc7 toc8

permuted.index:	ptxx ${TOC}
	$(FORMATTER) -t ptx.in ptxx > permuted.index.dit

#
# Get down to brass tacks and make the permuted index
#
ptxx:	/tmp/ptx.input break ignore
	@echo "Expect the ptx command to have an exit status of 1"
	-ptx -r -t -b break -f -w 108 -i ignore /tmp/ptx.input ptxx
	rm /tmp/ptx.input

#
# make.ptx.input takes the output of gettocx and changes all the
# section letters to upper case
#
/tmp/ptx.input:	${TOCX} cshcmd
	make.ptx.input ${TOCX} cshcmd > /tmp/ptx.input

#
# gettocx creates files containing lines like:
#       name(section): description
#
tocx1:
	gettocx 1
tocx2:
	gettocx 2
tocx3:
	gettocx 3
tocx4:
	gettocx 4
tocx5:
	gettocx 5
tocx6:
	gettocx 6
tocx7:
	gettocx 7
tocx8:
	gettocx 8

#
# make.toc takes the output of gettocx and makes line like:
#        .xx "name" "description"
toc1:	tocx1
	make.toc 1
toc2:	tocx2
	make.toc 2
toc3:	tocx3
	make.toc 3
toc4:	tocx4
	make.toc 4
toc5:	tocx5
	make.toc 5
toc6:	tocx6
	make.toc 6
toc7:	tocx7
	make.toc 7
toc8:	tocx8
	make.toc 8

clean:
	rm -f  ${TOCX}  ${TOC}  permuted.index.dit
