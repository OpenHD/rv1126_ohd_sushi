DEPS =
OBJ = consti_demo.o
CFLAGS =
_LDFLAGS := $(LDFLAGS) -lrt -lpcap -lsodium
#_LDFLAGS := $(LDFLAGS) -lrt -lsodium
#_LDFLAGS := $(LDFLAGS)

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS) $(_LDFLAGS)

consti_demo: $(OBJ)
	$(CXX) -o $@ $^ $(CFLAGS) $(_LDFLAGS)

.PHONY: clean
clean:
	rm -f *.o *~ consti_demo

.PHONY: install
install:
	cp -f consti_demo $(TARGET_DIR)/usr/bin/

.PHONY: uninstall
uninstall:
	rm -f $(TARGET_DIR)/usr/bin/consti_demo
