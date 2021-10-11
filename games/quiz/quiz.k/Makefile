#
# @(#)Makefile 1.1 92/07/30 SMI
#
SUBJECTS= \
	africa \
	america \
	areas \
	arith \
	asia \
	babies \
	bard \
	chinese \
	collectives \
	ed \
	elements \
	europe \
	greek \
	inca \
	index \
	latin \
	locomotive \
	midearth \
	morse \
	murders \
	poetry \
	posneg \
	pres \
	province \
	seq-easy \
	seq-hard \
	sexes \
	sov \
	spell \
	state \
	trek \
	ucc 

.DEFAULT:
	sccs get $< -G$@

all: ${SUBJECTS}

install: ${SUBJECTS}
	cp ${SUBJECTS} ${DESTDIR}/usr/games/lib/quiz.k

clean:

