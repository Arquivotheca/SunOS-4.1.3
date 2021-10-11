#
# @(#)Makefile 1.1 92/07/30 SMI
#
BIN = eeprom
ALL = eeprom openprom
SRC= eeprom.c openprom.c error.c loadlogo.c getdate.y
OBJ= $@.o getdate.o
INSTALL_ARGS= -s -o root -g kmem -m 2755
ARCH:sh = arch -k

EEPROM:sh = \
	case `arch -k` in \
	sun4c) echo openprom ;; \
	sun4m) echo openprom ;; \
	*) echo eeprom ;; \
	esac

CFLAGS=	-O

openprom := OBJ += loadlogo.o error.o
install_openprom := INSTALL_ARGS = -s -o root -g kmem -m 755

.KEEP_STATE:

default: $(EEPROM)

all: $(ALL)

$(ALL): $$(OBJ)
	$(LINK.c) -o $@ $(OBJ)

install: install_$(EEPROM)

install_$(EEPROM): $(EEPROM)
	install -d $(DESTDIR)/usr/etc 
	install -d $(DESTDIR)/usr/kvm
	install $(INSTALL_ARGS) $(EEPROM) $(DESTDIR)/usr/kvm/$(BIN)
	$(RM) $(DESTDIR)/usr/etc/$(BIN)
	ln -s ../kvm/$(BIN) $(DESTDIR)/usr/etc/$(BIN)

clean:
	$(RM) $(EEPROM) *.o a.out core y.tab.c

tags:	$(SRC)
	ctags $(SRC)
