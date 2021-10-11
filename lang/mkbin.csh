#!/bin/csh
#
# %W% (Sun) %G% %U%
# c-shell script to make the bin directory with
# links to current versions of compiler pieces
#
rm -rf bin
mkdir bin
foreach CPU (m68k sparc)
    mkdir bin/$CPU
    (cd bin/$CPU; ln -s ../../compile/compile .)
    (cd bin/$CPU; ln -s compile cc)
    (cd bin/$CPU; ln -s compile f77)
    (cd bin/$CPU; ln -s compile pc)
    (cd bin/$CPU; ln -s ../../cpp/cpp .)
    (cd bin/$CPU; ln -s ../../pcc/$CPU/{ccom,cg,f1} .)
    if ($CPU == m68k) then
	(cd bin/$CPU; ln -s ../../c2/$CPU/c2 .)
    endif
    (cd bin/$CPU; ln -s ../../as/$CPU/as .)
    (cd bin/$CPU; ln -s ../../ld/$CPU/ld .)
    (cd bin/$CPU; ln -s ../../iropt/$CPU/iropt .)
    (cd bin/$CPU; ln -s ../../f77pass1/$CPU/f77pass1 .)
    (cd bin/$CPU; ln -s ../../pascal/$CPU/pc0 .)
end
