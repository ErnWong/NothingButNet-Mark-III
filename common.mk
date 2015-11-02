#
# Files
#


ifeq ($(OS),Windows_NT)
	EXESUFFIX = .exe
else
	EXESUFFIX =
endif

INCLUDE := -I$(INCDIR) -I$(SRCDIR)
BINDIRS := $(BINDIR) $(BINDIR_TEST) $(BINDIR_SIZE)

INCLUDE_TEST = $(INCLUDE) -I$(SRCDIR_TEST) -I$(LIBDIR_TEST)
INCLUDE_SIZE = $(INCLUDE_TEST)

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

CSRC_TEST := $(wildcard $(SRCDIR_TEST)/*.$(CEXT_TEST))
COBJ_TEST := $(patsubst $(SRCDIR)/%.$(CEXT), $(BINDIR_TEST)/%.$(OEXT), $(CSRC))
TESTOBJ   := $(patsubst $(SRCDIR_TEST)/%.$(CEXT_TEST), $(BINDIR_TEST)/%.$(OEXT_TEST), $(CSRC_TEST))
CSRC_SIZE := $(wildcard $(SRCDIR_SIZE)/*.$(CEXT_SIZE))
COBJ_SIZE := $(patsubst $(SRCDIR_SIZE)/%.$(CEXT_SIZE), $(BINDIR_SIZE)/%.$(OEXT_SIZE), $(CSRC_SIZE))
OUT := $(BINDIR)/$(OUTNAME)
OUT_TEST := $(patsubst %.$(OEXT_TEST), %$(EXESUFFIX), $(TESTOBJ))
OUT_SIZE := $(BINDIR_SIZE)/$(OUTNAME_SIZE)
