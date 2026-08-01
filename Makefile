BUILDDIR=./build
SRCDIR=./src
#LIBS=-lcrypto -lssl
CCFLAGS+=-Wall -g
INCLUDES+=$(shell pkg-config --cflags p11-kit-1)
CC=gcc
kdc_objects=$(addprefix $(BUILDDIR)/, kdc.o hsm.o nnl_crypto.o nnl_tree.o)

all: kdc

kdc: $(kdc_objects)
	$(CC) -o $@ $^ $(CCFLAGS) $(LIBS) 

.PHONY: dirs
dirs:
	test -d $(BUILDDIR) || mkdir -p $(BUILDDIR)

$(BUILDDIR)/%.o: $(SRCDIR)/%.c | dirs
	$(CC) -c -o $@ $< $(CCFLAGS) $(INCLUDES)

.PHONY: clean
clean:
	find $(BUILDDIR) -name '*.o' -delete
	rm -f kdc

