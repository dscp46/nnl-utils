BUILDDIR=./build
SRCDIR=./src
LDLIBS=-lcrypto -lssl
CFLAGS+=-Wall
INCLUDES+=$(shell pkg-config --cflags p11-kit-1)
CC=gcc
TARGETS:=kdc

kdc_OBJS=$(addprefix $(BUILDDIR)/, kdc.o ckr_name.o hsm.o addr_item.o nnl_crypto.o nnl_tree.o procsec.o settings.o)

all: $(TARGETS)

.PHONY: debug
debug: CFLAGS+=-Wextra -g -DDEBUG
debug: clean all

define PROGRAM_template
$(1): $$($(1)_OBJS)
	$(CC) $(CFLAGS) -o $$@ $$^ $(LDLIBS)
endef

$(foreach prog,$(TARGETS),$(eval $(call PROGRAM_template,$(prog))))

.PHONY: dirs
dirs:
	test -d $(BUILDDIR) || mkdir -p $(BUILDDIR)

$(BUILDDIR)/%.o: $(SRCDIR)/%.c | dirs
	$(CC) -c -o $@ $< $(CFLAGS) $(INCLUDES)

.PHONY: clean
clean:
	find $(BUILDDIR) -name '*.o' -delete
	rm -f $(TARGETS)
