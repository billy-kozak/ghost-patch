#!/usr/bin/make -f
###############################################################################
# Copyright (C) 2019  Billy Kozak                                             #
#                                                                             #
# This file is part of the gorilla-patch program                              #
#                                                                             #
# This program is free software: you can redistribute it and/or modify        #
# it under the terms of the GNU Lesser General Public License as published by #
# the Free Software Foundation, either version 3 of the License, or           #
# (at your option) any later version.                                         #
#                                                                             #
# This program is distributed in the hope that it will be useful,             #
# but WITHOUT ANY WARRANTY; without even the implied warranty of              #
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the               #
# GNU Lesser General Public License for more details.                         #
#                                                                             #
# You should have received a copy of the GNU Lesser General Public License    #
# along with this program.  If not, see <http://www.gnu.org/licenses/>.       #
###############################################################################
###############################################################################
#                                    VARS                                     #
###############################################################################
CC := gcc
LD := gcc

CFLAGS += -Wall -std=gnu99
LDFLAGS += -Wall

CFLAGS += -fvisibility=hidden -fPIC
LDFLAGS += -fvisibility=hidden -fPIC

PROJECT := gorilla-patch

NO_DEPS_TARGETS += clean directories dir_clean
###############################################################################
#                                 BUILD DIRS                                  #
###############################################################################
BUILD_DIR   := obj
EXE_DIR     := bin
ASM_GEN_DIR := asm_gen

SRC_TREE += src

INC_DIRS += src/c/inc
CSRC_DIRS += src/c
ASM_DIRS += src/asm
###############################################################################
#                                 BUILD FILES                                 #
###############################################################################
C_PATHS   += $(foreach dir, $(CSRC_DIRS),$(wildcard $(dir)/*.c))

C_FILES   += $(foreach f, $(C_PATHS),$(notdir $(f)))
OBJ_FILES += $(foreach f,$(C_FILES),$(BUILD_DIR)/$(patsubst %.c,%.o,$(f)))
DEP_FILES += $(foreach f,$(C_FILES),$(BUILD_DIR)/$(patsubst %.c,%.d,$(f)))
ASM_GEN   += $(foreach f,$(C_FILES),$(ASM_GEN_DIR)/$(patsubst %.c,%.s,$(f)))

ASM_PATHS += $(foreach dir, $(ASM_DIRS),$(wildcard $(dir)/*.S))

ASM_FILES += $(foreach f, $(ASM_PATHS),$(notdir $(f)))
OBJ_FILES += $(foreach f,$(ASM_FILES),$(BUILD_DIR)/$(patsubst %.S,%.o,$(f)))
DEP_FILES += $(foreach f,$(ASM_FILES),$(BUILD_DIR)/$(patsubst %.S,%.d,$(f)))

LIBS = -ldl -lpthread

BINARY := $(EXE_DIR)/$(PROJECT)

INCLUDES += $(foreach f,$(INC_DIRS),-I$(f))

CFLAGS += $(INCLUDES)

vpath %.c $(CSRC_DIRS)
vpath %.S $(ASM_DIRS)

C_DIRS += $(CSRC_DIRS) $(INC_DIRS)

CLEAN_FILES += $(foreach dir,$(C_DIRS),$(wildcard $(dir)/*~))
CLEAN_FILES += $(foreach dir,$(C_DIRS),$(wildcard $(dir)/*\#))
CLEAN_FILES += $(wildcard Makefile~)
CLEAN_FILES += $(wildcard $(BUILD_DIR)/*) $(wildcard $(EXE_DIR)/*)
CLEAN_FILES += $(wildcard $(ASM_GEN_DIR)/*)
###############################################################################
#                                   TARGETS                                   #
###############################################################################
all: optomized

no_trace: CFLAGS += -DDEBUG_MODE_NO_PTRACE
no_trace: debug

no_thread: CFLAGS += -DDEBUG_MODE_NO_THREAD
no_thread: debug

debug: CFLAGS += -DDEBUG=1 -g
debug: $(BINARY)

optomized: CFLAGS += -DNDEBUG=1 -march=native -Os -flto
optomized: LDFLAGS += -march=native -Os -flto
optomized: $(BINARY)

asg_gen: CFLAGS += -fverbose-asm
asm_gen: CFLAGS += -DNDEBUG=1 -march=native -Os -flto
asm_gen: $(ASM_GEN)

directories: $(BUILD_DIR)/.dir_dummy $(EXE_DIR)/.dir_dummy

%.dir_dummy:
	mkdir -p $(dir $(@))
	@touch $(@)

$(BUILD_DIR)/%.d: %.S | $(BUILD_DIR)/.dir_dummy
	$(CC) -MF $@ -M -MT "$(patsubst %.d,%.o,$@) $@" $<

$(BUILD_DIR)/%.o: %.S | $(BUILD_DIR)/.dir_dummy
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/%.d: %.c | $(BUILD_DIR)/.dir_dummy
	$(CC) $(CFLAGS) -MF $@ -M -MT "$(patsubst %.d,%.o,$@) $@" $<

$(BUILD_DIR)/%.o: %.c | $(BUILD_DIR)/.dir_dummy
	$(CC) $(CFLAGS) -c $< -o $@

$(ASM_GEN): $(ASM_GEN_DIR)/%.s : %.c | $(ASM_GEN_DIR)/.dir_dummy
	$(CC) $(CFLAGS) -S $< -o $@

$(BINARY): $(OBJ_FILES) | $(EXE_DIR)/.dir_dummy
	$(LD) $(LDFLAGS) -pie  $^ $(LIBS) -o $@

clean:
	rm -f $(CLEAN_FILES)

dir_clean:
	rm -rf $(BUILD_DIR) $(EXE_DIR) $(ASM_GEN_DIR)

ifeq (,$(filter $(MAKECMDGOALS), $(NO_DEPS_TARGETS)))

#next two conditonals prevent make from running on dry run or when
#invoked for tab-completion
ifneq (n,$(findstring n,$(firstword $(MAKEFLAGS))))
ifneq (p,$(findstring p,$(firstword $(MAKEFLAGS))))
-include $(DEP_FILES)
endif
endif


endif
