UNAME := $(shell uname -s)
ifeq ($(UNAME), Darwin)
	PLATFORM := mac
else
	PLATFORM := linux
endif

BINDIR := bin/$(PLATFORM)/sc

.PHONY: all clean distclean install uninstall

all: $(BINDIR)
	$(MAKE) -C src
	cp src/sc src/psc src/scqref $(BINDIR)/

$(BINDIR):
	mkdir -p $(BINDIR)

clean:
	$(MAKE) -C src clean
	rm -rf bin

distclean:
	$(MAKE) -C src distclean
	rm -rf bin

install uninstall:
	$(MAKE) -C src $@
