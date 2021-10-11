#
# @(#)Makefile 1.1 92/07/30 SMI; from UCB 4.7 85/11/11
#
CFLAGS=	-O
ALL=	tek t4013 t300 t300s t450 aedplot bgplot crtplot dumbplot gigiplot \
	hpplot hp7221plot implot atoplot plottoa vplot

all:	${ALL} debug

tek:	driver.o
	${CC} -o tek  driver.o -l4014 -lm

t4013:	driver.o
	${CC} -o t4013  driver.o -l4013 -lm

t300:	driver.o 
	${CC} -o t300 driver.o -l300 -lm

t300s:	driver.o 
	${CC} -o t300s driver.o -l300s -lm

t450:	driver.o 
	${CC} -o t450 driver.o -l450 -lm

vplot:	vplot.o chrtab.o
	${CC} -o vplot vplot.o chrtab.o

aedplot: driver.o
	${CC} -o aedplot driver.o -lplotaed

bgplot: driver.o
	${CC} -o bgplot driver.o -lplotbg -lm

crtplot: crtdriver.o crtplot.o
	${CC} -o crtplot crtdriver.o crtplot.o -lcurses -ltermcap -lm

dumbplot: driver.o
	${CC} -o dumbplot driver.o -lplotdumb -ltermcap -lm

gigiplot: driver.o
	${CC} -o gigiplot driver.o -lplotgigi -lm

hpplot: driver.o
	${CC} -o hpplot driver.o -lplot2648 -lm

hp7221plot: driver.o
	${CC} -o hp7221plot driver.o -lplot7221 -lm

implot: driver.o
	${CC} -o implot driver.o -lplotimagen -lm

atoplot: atoplot.o
	${CC} -o atoplot atoplot.o -lplot -lm

plottoa: plottoa.o
	${CC} -o plottoa plottoa.o

debug: debug.o
	${CC} -o debug debug.o

install: all plot.sh
	-for i in ${ALL}; do \
		(install -s $$i ${DESTDIR}/usr/bin/$$i); done
	install -s vplot ${DESTDIR}/usr/bin
	install -c plot.sh ${DESTDIR}/usr/bin/plot

clean:
	rm -f *.o ${ALL} a.out core errs
