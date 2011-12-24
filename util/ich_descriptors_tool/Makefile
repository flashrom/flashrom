CC ?= gcc

PROGRAM=ich_descriptors_tool
EXTRAINCDIRS = ../../ .
DEPPATH = .dep
OBJATH = .obj
SHAREDSRC = ich_descriptors.c
SHAREDSRCDIR = ../..

SRC = $(wildcard *.c)

CFLAGS += -Wall
CFLAGS += -MMD -MP -MF $(DEPPATH)/$(@F).d
# enables functions that populate the descriptor structs from plain binary dumps
CFLAGS += -D ICH_DESCRIPTORS_FROM_DUMP
CFLAGS += $(patsubst %,-I%,$(EXTRAINCDIRS))

OBJ = $(OBJATH)/$(SRC:%.c=%.o)

SHAREDOBJ = $(OBJATH)/$(notdir $(SHAREDSRC:%.c=%.o))

all:$(PROGRAM)

$(OBJ): $(OBJATH)/%.o : %.c
	$(CC) $(CFLAGS) -o $@ -c $<

# this enables us to share source files without simultaneously sharing .o files
# with flashrom, which would lead to unexpected results (w/o running make clean)
$(SHAREDOBJ): $(OBJATH)/%.o : $(SHAREDSRCDIR)/%.c
	$(CC) $(CFLAGS) -o $@ -c $<

$(PROGRAM): $(OBJ) $(SHAREDOBJ)
	$(CC) -o $(PROGRAM) $(OBJ) $(SHAREDOBJ)

clean:
	rm -f $(PROGRAM)
	rm -rf $(DEPPATH) $(OBJATH)

# Include the dependency files.
-include $(shell mkdir -p $(DEPPATH) $(OBJATH) 2>/dev/null) $(wildcard $(DEPPATH)/*)

.PHONY: all clean
