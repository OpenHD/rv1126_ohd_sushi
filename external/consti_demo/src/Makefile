DEPS =
OBJ = freq.o
CFLAGS =
_LDFLAGS := $(LDFLAGS) -lrt -lpcap -lsodium
#_LDFLAGS := $(LDFLAGS) -lrt -lsodium
#_LDFLAGS := $(LDFLAGS)

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS) $(_LDFLAGS)

freq: $(OBJ)
	$(CXX) -o $@ $^ $(CFLAGS) $(_LDFLAGS)

.PHONY: clean
clean:
	rm -f *.o *~ freq

.PHONY: install
install:
	cp -f freq $(TARGET_DIR)/usr/bin/

.PHONY: uninstall
uninstall:
	rm -f $(TARGET_DIR)/usr/bin/freq
