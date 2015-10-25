#
# Files
#


ifeq ($(OS),Windows_NT)
	EXESUFFIX = .exe
else
	EXESUFFIX =
endif

INCLUDE := -I$(INCDIR) -I$(SRCDIR)
BINDIRS := $(BINDIR) $(BINDIR_TEST)

INCLUDE_TEST = $(INCLUDE) -I$(LIBDIR_TEST)

HEADERS := \
	$(wildcard $(SRCDIR)/*.$(HEXT)) \
	$(wildcard $(INCDIR)/*.$(HEXT)) \
	$(wildcard $(LIBDIR_TEST)/*.$(HEXT))

ASMSRC := $(wildcard $(SRCDIR)/*.$(ASMEXT))
CPPSRC := $(wildcard $(SRCDIR)/*.$(CPPEXT))
CSRC   := $(wildcard $(SRCDIR)/*.$(CEXT))
ASMOBJ := $(patsubst $(SRCDIR)/%.$(ASMEXT), $(BINDIR)/%.$(OEXT), $(ASMSRC))
CPPOBJ := $(patsubst $(SRCDIR)/%.$(CPPEXT), $(BINDIR)/%.$(OEXT), $(CPPSRC))
COBJ   := $(patsubst $(SRCDIR)/%.$(CEXT), $(BINDIR)/%.$(OEXT), $(CSRC))
OUT := $(BINDIR)/$(OUTNAME)
