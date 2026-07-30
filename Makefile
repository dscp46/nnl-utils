BUILDDIR=./build
SRCDIR=./src
#LIBS=-lcrypto -lssl
CCFLAGS+=-Wall -g
CC=gcc
kdc_objects=$(addprefix $(BUILDDIR)/, kdc.o nnl_tree.o)

all: kdc

kdc: $(kdc_objects)
	$(CC) -o $@ $^ $(CCFLAGS) $(LIBS) 

.PHONY: dirs
dirs:
	test -d $(BUILDDIR) || mkdir -p $(BUILDDIR)

$(BUILDDIR)/%.o: $(SRCDIR)/%.c | dirs
	$(CC) -c -o $@ $< $(CCFLAGS)

.PHONY: clean
clean:
	find $(BUILDDIR) -name '*.o' -delete
	rm -f kdc

