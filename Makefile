CC         := g++
LD         := g++

MODULES    := common lz77
SRCDIR     := src
BUILDDIR   := build
BINDIR     := bin
SRC_DIRS   := $(addprefix $(SRCDIR)/,$(MODULES))
BUILD_DIRS := $(addprefix $(BUILDDIR)/,$(MODULES))
INC_DIRS   := third_party/tclap-1.2.2/include/
EXECS      := lz77
INCS       := $(SRC_DIRS) $(addsuffix /include,$(SRC_DIRS)) $(addprefix $(SRCDIR)/,$(INC_DIRS))

SRC       := $(foreach sdir,$(SRC_DIRS),$(wildcard $(sdir)/*.cpp))
SRC       += $(foreach sdir,$(SRC_DIRS),$(wildcard $(sdir)/src/*.cpp))
OBJ       := $(patsubst src/%.cpp,build/%.o,$(SRC))
INCLUDES  := $(addprefix -I,$(INCS))

vpath %.cpp $(SRC_DIRS)

define make-goal
$1/%.o: %.cpp
	$(CC) $(INCLUDES) -c $$< -o $$@
endef

.PHONY: all checkdirs clean

all: checkdirs $(BINDIR)/lz77

$(BINDIR)/lz77: $(OBJ)
	$(LD) $^ -o $@

checkdirs: $(BUILD_DIRS) $(BINDIR)

$(BUILD_DIRS):
	@mkdir -p $@/src

$(BINDIR):
	@mkdir -p $@

clean:
	echo $(SRC_DIRS)
	echo $(SRC)
	@rm -rf $(BUILD_DIRS)

clean-all: clean
	@rm -rf $(BINDIR)

$(foreach bdir,$(BUILD_DIRS),$(eval $(call make-goal,$(bdir))))
