#
# This file is part of the flashrom project.
#
# This Makefile works standalone, but it is usually called from the main
# Makefile in the flashrom directory.

PROGRAM=ich_descriptors_tool
EXTRAINCDIRS = ../../ .
DEPPATH = .dep
OBJATH = .obj
SHAREDSRC = ich_descriptors.c
SHAREDSRCDIR = ../..
# If your compiler spits out excessive warnings, run make WARNERROR=no
# You shouldn't have to change this flag.
WARNERROR ?= yes

SRC = $(wildcard *.c)

CC ?= gcc

# If the user has specified custom CFLAGS, all CFLAGS settings below will be
# completely ignored by gnumake.
CFLAGS ?= -Os -Wall -Wshadow
ifeq ($(TARGET_OS), DOS)
# DJGPP has odd uint*_t definitions which cause lots of format string warnings.
CFLAGS += -Wno-format
endif
ifeq ($(WARNERROR), yes)
CFLAGS += -Werror
endif


FLASHROM_CFLAGS += -MMD -MP -MF $(DEPPATH)/$(@F).d
# enables functions that populate the descriptor structs from plain binary dumps
FLASHROM_CFLAGS += -D ICH_DESCRIPTORS_FROM_DUMP
FLASHROM_CFLAGS += $(patsubst %,-I%,$(EXTRAINCDIRS))

OBJ = $(OBJATH)/$(SRC:%.c=%.o)

SHAREDOBJ = $(OBJATH)/$(notdir $(SHAREDSRC:%.c=%.o))

all:$(PROGRAM)$(EXEC_SUFFIX)

$(OBJ): $(OBJATH)/%.o : %.c
	$(CC) $(CFLAGS) $(CPPFLAGS) $(FLASHROM_CFLAGS) -o $@ -c $<

# this enables us to share source files without simultaneously sharing .o files
# with flashrom, which would lead to unexpected results (w/o running make clean)
$(SHAREDOBJ): $(OBJATH)/%.o : $(SHAREDSRCDIR)/%.c
	$(CC) $(CFLAGS) $(CPPFLAGS) $(FLASHROM_CFLAGS) -o $@ -c $<

$(PROGRAM)$(EXEC_SUFFIX): $(OBJ) $(SHAREDOBJ)
	$(CC) $(LDFLAGS) -o $(PROGRAM)$(EXEC_SUFFIX) $(OBJ) $(SHAREDOBJ)

clean:
	rm -f $(PROGRAM) $(PROGRAM).exe
	rm -rf $(DEPPATH) $(OBJATH)

# Include the dependency files.
-include $(shell mkdir -p $(DEPPATH) $(OBJATH) 2>/dev/null) $(wildcard $(DEPPATH)/*)

.PHONY: all clean
