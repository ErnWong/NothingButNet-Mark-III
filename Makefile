# Path to project root (for top-level, so the project is in ./; first-level, ../; etc.)
ROOT = .


-include $(ROOT)/Config.mk
-include $(ROOT)/common.mk

.PHONY: all clean upload test run_test _force_look


all: $(BINDIRS) $(OUT)

# Remove all intermediate object files (remove the binary directory)
clean:
	-rm -f $(OUT)
	-rm -rf $(BINDIR)
	-rm -rf $(BINDIR_TEST)

# Uploads program to device
upload: all
	$(UPLOAD)

test: $(BINDIRS) $(OUT_TEST) run_test

run_test: $(OUT_TEST)
	$(foreach test, $(OUT_TEST), @$(test))

_force_look:
	@true

$(BINDIRS):
	-@mkdir -p $(BINDIRS)

# Compile program
$(OUT): $(ASMOBJ) $(COBJ) $(CPPOBJ)
	@echo LN $(BINDIR)/*.o $(LIBRARIES) to $@
	@$(CC) $(LDFLAGS) $(BINDIR)/*.o $(LIBRARIES) -o $@
	@$(MCUPREFIX)size $(SIZEFLAGS) $(OUT)
	$(MCUPREPARE)

$(OUT_TEST): $(BINDIR_TEST)/%$(EXESUFFIX): $(BINDIR)/%.$(OEXT) $(BINDIR_TEST)/%.$(OEXT_TEST) $(LIBOBJ_TEST)
	@echo LN $^ to $@
	@$(CC) $(LDFLAGS_TEST) $^ -o $@

# Assembly source file management
$(ASMOBJ): $(BINDIR)/%.$(OEXT): $(SRCDIR)/%.$(ASMEXT) $(HEADERS)
	@echo AS $<
	@$(AS) $(AFLAGS) -o $@ $<

# Object management
$(COBJ): $(BINDIR)/%.$(OEXT): $(SRCDIR)/%.$(CEXT) $(HEADERS)
	@echo CC $(INCLUDE) $<
	@$(CC) $(INCLUDE) $(CFLAGS) -o $@ $<

$(CPPOBJ): $(BINDIR)/%.$(OEXT): $(SRCDIR)/%.$(CPPEXT) $(HEADERS)
	@echo CPC $(INCLUDE) $<
	@$(CPPCC) $(INCLUDE) $(CPPFLAGS) -o $@ $<

$(TESTOBJ): $(BINDIR_TEST)/%.$(OEXT_TEST): $(SRCDIR_TEST)/%.$(CEXT_TEST) $(HEADERS)
	@echo CC $(INCLUDE_TEST) $<
	@$(CC) $(INCLUDE_TEST) $(CFLAGS_TEST) -o $@ $<

$(LIBOBJ_TEST): $(LIBSRC_TEST) $(HEADERS)
	@echo CC $(INCLUDE_TEST) $<
	@$(CC) $(INCLUDE_TEST) $(CFLAGS_TEST) -o $@ $<
