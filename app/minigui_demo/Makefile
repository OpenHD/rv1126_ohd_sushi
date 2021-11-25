BIN	 = minigui_demo
all: $(BIN)

STAGING_DIR ?= $(TARGET_OUTPUT_DIR)/staging/

INCS	 = -I $(STAGING_DIR)/usr/include \
             -I $(STAGING_DIR)/usr/include/libdrm \
             -I $(STAGING_DIR)/usr/include
LIBS	 = -lpthread  -ldrm -lminigui_ths -ljpeg -lpng -lm -lfreetype -lrga

define all-cpp-files-under
$(shell find $(1) -name "*."$(2) -and -not -name ".*" )
endef

define all-subdir-cpp-files
$(call all-cpp-files-under,.,"cpp")
endef

CPPSRCS	 = $(call all-subdir-cpp-files)

rwildcard=$(wildcard $(1)/$(2)) \
	$(foreach d,$(wildcard $(1)/*),$(call rwildcard,$(d),$(2)))

CSRCS += $(call rwildcard,*.c)

COBJS	:= $(CSRCS:.c=.o)
CPPOBJS	:= $(CPPSRCS:.cpp=.o)
CDEPS = $(CSRCS:.c=.d)
CXXDEPS = $(CPPSRCS:.cpp=.d)

ifeq ($(filter clean,$(MAKECMDGOALS)),)
-include $(CDEPS) $(CXXDEPS)
endif

$(COBJS): %.o : %.c
	$(CC) -c $(INCS) $(CFLAGS) $< -o $@
	$(if $(Q),@echo CC $(@))
$(CPPOBJS): %.o : %.cpp
	$(CXX) -c $(INCS) $(CXXFLAGS) $< -o $@
	@echo "fuck cpp build success!"
	$(if $(Q),@echo CXX $(@))

$(BIN): $(COBJS) $(CPPOBJS)
	$(CXX) -o $(BIN) $(COBJS) $(CPPOBJS) $(LIBS)

install: $(ETCPATH) $(RESPATH) $(INSTALL_BIN_PATH) $(BIN)
	$(Q)install -d $(INSTALL_BIN_PATH)
	$(Q)install -C $(BIN) $(INSTALL_BIN_PATH)

clean:
	$(Q)rm -f $(BIN) $(COBJS) $(CPPOBJS) $(CDEPS) $(CXXDEPS)
	@echo "minigui_demo clean success!"

.PHONY: install clean all
