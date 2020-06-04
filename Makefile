CC         := g++
LD         := g++

EXEC       := lz77 pal2tpl
MODULES    := common
SRCDIR     := src
BUILDDIR   := build
BINDIR     := bin
SRC_DIRS   := $(addprefix $(SRCDIR)/,$(MODULES))
BUILD_DIRS := $(addprefix $(BUILDDIR)/,$(MODULES)) $(addprefix $(BUILDDIR)/,$(EXEC))
INC_DIRS   := third_party/tclap-1.2.2/include/
INCS       := $(SRC_DIRS) $(addsuffix /include,$(SRC_DIRS)) $(addprefix $(SRCDIR)/,$(INC_DIRS))

SRC       := $(foreach sdir,$(SRC_DIRS),$(wildcard $(sdir)/*.cpp))
SRC       += $(foreach sdir,$(SRC_DIRS),$(wildcard $(sdir)/src/*.cpp))
OBJ       := $(patsubst $(SRCDIR)/%.cpp,$(BUILDDIR)/%.o,$(SRC))
INCLUDES  := $(addprefix -I,$(INCS))
EXECS     := $(addprefix $(BINDIR)/,$(EXEC))
EXEC_SDIR := $(addprefix $(SRCDIR)/,$(EXEC))
EXEC_ODIR := $(addprefix $(BUILDDIR)/,$(EXEC))

vpath %.cpp $(SRC_DIRS) $(EXEC_SDIR)

define make-obj
$(BUILDDIR)/$1/%.o: $(SRCDIR)/$1/$2/%.cpp
	$(CC) $(CFLAGS) $(CXXFLAGS) $(INCLUDES) -c $$< -o $$@
endef

define make-exec
$(BINDIR)/$1: $(OBJ) $(patsubst $(SRCDIR)/$1/%.cpp,$(BUILDDIR)/$1/%.o,$(wildcard $(SRCDIR)/$1/*.cpp))
	$(LD) $(LDFLAGS) $(LIBS) $$^ -o $(BINDIR)/$1
endef

.PHONY: all checkdirs clean clean-all

all: checkdirs $(EXECS)

checkdirs: $(BUILD_DIRS) $(BINDIR)

$(BUILD_DIRS):
	@mkdir -p $@/src

$(BINDIR):
	@mkdir -p $@

clean:
	@rm -rf $(BUILDDIR)

clean-all: clean
	@rm -rf $(BINDIR)

$(foreach module,$(MODULES),$(eval $(call make-obj,$(module)/src)))
$(foreach exec,$(EXEC),$(eval $(call make-obj,$(exec))))
$(foreach exec,$(EXEC),$(eval $(call make-exec,$(exec))))
