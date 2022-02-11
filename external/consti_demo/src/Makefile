DEPS =
OBJ = freq.o
_CFLAGS := $(CFLAGS) -std=c++17
_LDFLAGS := $(LDFLAGS) -lrt
#_LDFLAGS := $(LDFLAGS) -lrt -lsodium
#_LDFLAGS := $(LDFLAGS)

all_bin: v4l2_test_mplane freq
all: all_bin

%.o: %.cpp $(DEPS)
	$(CXX) -c -o $@ $< $(_CFLAGS) $(_LDFLAGS)

freq: $(OBJ)
	$(CXX) -o $@ $^ $(CFLAGS) $(_LDFLAGS)

#v4l2_test: v4l2_test.o
#	$(CXX) -o $@ $^ $(CFLAGS) $(_LDFLAGS)


v4l2_test_mplane: v4l2_test_mplane.o
	$(CXX) -o $@ $^ $(_CFLAGS) $(_LDFLAGS)

.PHONY: clean
clean:
	rm -f *.o *~ freq v4l2_test v4l2_test_mplane

.PHONY: install
install:
	#cp -f freq v4l2_test $(TARGET_DIR)/usr/bin/

.PHONY: uninstall
uninstall:
	#rm -f $(TARGET_DIR)/usr/bin/freq $(TARGET_DIR)/usr/bin/v4l2_test
