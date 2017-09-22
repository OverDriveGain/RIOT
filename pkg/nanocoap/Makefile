PKG_NAME=nanocoap
PKG_URL=https://github.com/kaspar030/sock
PKG_VERSION=b65b991efee17c1192d8f058a07a38bf688b6ee7
PKG_LICENSE=LGPL-2.1

.PHONY: all

all: git-download
	@cp Makefile.nanocoap $(PKG_BUILDDIR)/nanocoap/Makefile
	"$(MAKE)" -C $(PKG_BUILDDIR)/nanocoap

include $(RIOTBASE)/pkg/pkg.mk
