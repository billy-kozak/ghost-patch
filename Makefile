#!/usr/bin/make -f
###############################################################################
# Copyright (C) 2019  Billy Kozak                                             #
#                                                                             #
# This file is part of the ghost-patch program                                #
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

PROJECT := ghost-patch

NO_DEPS_TARGETS += clean directories dir_clean
###############################################################################
#                                 BUILD DIRS                                  #
###############################################################################
BUILD_DIR   := obj
EXE_DIR     := bin
ASM_GEN_DIR := asm_gen

BUILD_TEST_DIR := $(BUILD_DIR)/tests
TEST_EXE_DIR := $(EXE_DIR)/tests

ASM_DIRS += src/asm

I_COMMON += src/c/common
I_SO += src/c/so/
I_MAIN += src/c/main/
I_TEST += src/c/test/

CD_COMMON += $(shell find src/c/common -type d)
CD_SO += $(shell find src/c/so -type d)
CD_MAIN += $(shell find src/c/main -type d)
CD_TEST += $(shell find src/c/test -type d)

DD_COMMON = $(BUILD_DIR)/common
DD_SO = $(BUILD_DIR)/so
DD_MAIN = $(BUILD_DIR)/main
DD_TEST = $(BUILD_DIR)/test

CSRC_DIRS = $(CD_COMMON) $(CD_SO) $(CD_MAIN) $(CD_TEST)
###############################################################################
#                                 BUILD FILES                                 #
###############################################################################
CP_COMMON += $(foreach dir, $(CD_COMMON),$(wildcard $(dir)/*.c))
CP_SO += $(foreach dir, $(CD_SO),$(wildcard $(dir)/*.c))
CP_MAIN += $(foreach dir, $(CD_MAIN),$(wildcard $(dir)/*.c))
CP_TEST += $(foreach dir, $(CD_TEST),$(wildcard $(dir)/*.c))

CF_COMMON += $(foreach f, $(CP_COMMON),$(notdir $(f)))
CF_SO += $(foreach f, $(CP_SO),$(notdir $(f)))
CF_MAIN += $(foreach f, $(CP_MAIN),$(notdir $(f)))
CF_TEST += $(foreach f, $(CP_TEST),$(notdir $(f)))

O_COMMON += $(foreach f,$(CF_COMMON),$(BUILD_DIR)/$(patsubst %.c,%.o,$(f)))
O_SO += $(foreach f,$(CF_SO),$(BUILD_DIR)/$(patsubst %.c,%.o,$(f)))
O_MAIN += $(foreach f,$(CF_MAIN),$(BUILD_DIR)/$(patsubst %.c,%.o,$(f)))
O_TEST += $(foreach f,$(CF_TEST),$(BUILD_DIR)/$(patsubst %.c,%.o,$(f)))

DP_COMMON = $(foreach f,$(CF_COMMON),$(DD_COMMON)/$(patsubst %.c,%.d,$(f)))
DP_SO = $(foreach f,$(CF_SO),$(DD_SO)/$(patsubst %.c,%.d,$(f)))
DP_MAIN = $(foreach f,$(CF_MAIN),$(DD_MAIN)/$(patsubst %.c,%.d,$(f)))
DP_TEST = $(foreach f,$(CF_TEST),$(DD_TEST)/$(patsubst %.c,%.d,$(f)))

C_PATHS   += $(CP_COMMON) $(CP_SO) $(CP_MAIN) $(CP_TEST)
C_FILES   += $(CF_COMMON) $(CF_SO) $(CF_MAIN) $(CF_TEST)

ASM_GEN   += $(foreach f,$(C_FILES),$(ASM_GEN_DIR)/$(patsubst %.c,%.s,$(f)))

ASM_PATHS += $(foreach dir, $(ASM_DIRS),$(wildcard $(dir)/*.S))

ASM_FILES += $(foreach f, $(ASM_PATHS),$(notdir $(f)))
ASM_O += $(foreach f,$(ASM_FILES),$(BUILD_DIR)/$(patsubst %.S,%.o,$(f)))

DEP_FILES += $(foreach f,$(ASM_FILES),$(BUILD_DIR)/$(patsubst %.S,%.d,$(f)))
DEP_FILES += $(DP_COMMON) $(DP_SO) $(DP_MAIN) $(DP_TEST)

SO_OBJ = $(O_SO) $(O_COMMON) $(ASM_O)
MAIN_OBJ = $(O_MAIN) $(O_COMMON)
TEST_OBJ = $(O_TEST) $(O_COMMON) $(ASM_O)
TEST_OBJ += $(filter-out %/shared.o, $(O_SO))

MAIN_LIBS = -ldl -lpthread
SO_LIBS = -lpthread -lm
TEST_LIBS = $(SO_LIBS)

BINARY := $(EXE_DIR)/$(PROJECT)
SO := $(EXE_DIR)/$(PROJECT).so

INC_COMMON += $(foreach f,$(I_COMMON),-I$(f))

INC_MAIN += $(foreach f,$(I_MAIN),-I$(f))
INC_MAIN += $(INC_COMMON)

INC_SO += $(foreach f,$(I_SO),-I$(f))
INC_SO += $(INC_COMMON)

INC_TEST += $(foreach f,$(I_TEST),-I$(f))
INC_TEST += $(INC_COMMON) $(INC_SO)

TEST_EXE = $(TEST_EXE_DIR)/ghost-patch-tests

O_COMMON_DUMMY = $(BUILD_DIR)/.o_common.dummy
O_SO_DUMMY = $(BUILD_DIR)/.o_so.dummy
O_MAIN_DUMMY = $(BUILD_DIR)/.o_main.dummy
O_TEST_DUMMY = $(BUILD_DIR)/.o_test.dummy
O_ASM_DUMMY = $(BUILD_DIR)/.o_asm.dummy

vpath %.c $(CSRC_DIRS)
vpath %.S $(ASM_DIRS)

CLEAN_FILES += $(foreach dir,$(CSRC_DIRS),$(wildcard $(dir)/*~))
CLEAN_FILES += $(foreach dir,$(CSRC_DIRS),$(wildcard $(dir)/*\#))
CLEAN_FILES += $(wildcard Makefile~)
CLEAN_FILES += $(wildcard $(BUILD_DIR)/*) $(wildcard $(EXE_DIR)/*)
CLEAN_FILES += $(wildcard $(ASM_GEN_DIR)/*)
CLEAN_FILES += $(wildcard $(BUILD_DIR)/.o_*.dummy)
###############################################################################
#                                   TARGETS                                   #
###############################################################################
all: optomized

no_trace: CFLAGS += -DDEBUG_MODE_NO_PTRACE
no_trace: debug

tests: $(BUILD_TEST_DIR)/.dir_dummy
tests: CFLAGS += -DDEBUG=1 -g -O0
tests: $(DEP_FILES)
tests: $(TEST_EXE)

fast_tests: $(BUILD_TEST_DIR)/.dir_dummy
fast_tests: CFLAGS += -DNDEBUG=1 -march=native -Os -flto=auto
fast_tests: LDFLAGS += -march=native -Os -flto=auto
fast_tests: $(DEP_FILES)
fast_tests: $(TEST_EXE)

debug: CFLAGS += -DDEBUG=1 -g -O0
debug: $(DEP_FILES)
debug: $(BINARY)
debug: $(SO)

optomized: CFLAGS += -DNDEBUG=1 -march=native -Os -flto=auto
optomized: LDFLAGS += -march=native -Os -flto=auto
optomized: $(DEP_FILES)
optomized: $(BINARY)
optomized: $(SO)

asm_gen: CFLAGS += -fverbose-asm
asm_gen: CFLAGS += -DNDEBUG=1 -march=native -Os -flto=auto
asm_gen: $(DEP_FILES)
asm_gen: $(ASM_GEN)

$(O_COMMON_DUMMY): CFLAGS += $(INC_COMMON)
$(O_COMMON_DUMMY): $(O_COMMON)
	touch $(O_COMMON_DUMMY)

$(O_MAIN_DUMMY): CFLAGS += $(INC_MAIN)
$(O_MAIN_DUMMY): $(O_MAIN)
	touch $(O_MAIN_DUMMY)

$(O_SO_DUMMY): CFLAGS += $(INC_SO)
$(O_SO_DUMMY): $(O_SO)
	touch $(O_SO_DUMMY)

$(O_TEST_DUMMY): CFLAGS += $(INC_TEST)
$(O_TEST_DUMMY): $(O_TEST)
	touch $(O_TEST_DUMMY)

$(O_ASM_DUMMY): $(ASM_O)
	touch $(O_ASM_DUMMY)

directories: $(BUILD_DIR)/.dir_dummy $(EXE_DIR)/.dir_dummy

%.dir_dummy:
	mkdir -p $(dir $(@))
	@touch $(@)

$(DD_COMMON)/%.d: DO += $(BUILD_DIR)/$(patsubst $(DD_SO)/%.d,%.o,$@)
$(DD_COMMON)/%.d: %.c | $(DD_COMMON)/.dir_dummy
	$(CC) $(INC_COMMON) -MF $@ -M -MT "$(DO) $@" $<

$(DD_SO)/%.d: DO += $(BUILD_DIR)/$(patsubst $(DD_SO)/%.d,%.o,$@)
$(DD_SO)/%.d: %.c | $(DD_SO)/.dir_dummy
	$(CC) $(INC_SO) -MF $@ -M -MT "$(DO) $@" $<

$(DD_MAIN)/%.d: DO += $(BUILD_DIR)/$(patsubst $(DD_SO)/%.d,%.o,$@)
$(DD_MAIN)/%.d: %.c | $(DD_MAIN)/.dir_dummy
	$(CC) $(INC_MAIN) -MF $@ -M -MT "$(DO) $@" $<

$(DD_TEST)/%.d: %.c | $(DD_MAIN)/.dir_dummy
$(DD_TEST)/%.d: %.c | $(DD_TEST)/.dir_dummy
	$(CC) $(INC_TEST) -MF $@ -M -MT "$(@) $@" $<

$(BUILD_DIR)/%.d: %.S | $(BUILD_DIR)/.dir_dummy
	$(CC) -MF $@ -M -MT "$(patsubst %.d,%.o,$@) $@" $<

$(BUILD_DIR)/%.d: %.c | $(BUILD_DIR)/.dir_dummy
	$(CC) $(CFLAGS) -MF $@ -M -MT "$(patsubst %.d,%.o,$@) $@" $<

$(BUILD_DIR)/%.o: %.S | $(BUILD_DIR)/.dir_dummy
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/%.o: %.c | $(BUILD_DIR)/.dir_dummy
	$(CC) $(CFLAGS) -c $< -o $@

$(ASM_GEN): $(ASM_GEN_DIR)/%.s : %.c | $(ASM_GEN_DIR)/.dir_dummy
	$(CC) $(CFLAGS) -S $< -o $@

$(SO): $(O_COMMON_DUMMY) $(O_SO_DUMMY) $(O_ASM_DUMMY) | $(EXE_DIR)/.dir_dummy
	$(LD) $(LDFLAGS) -pie  $(SO_OBJ) $(MAIN_LIBS) -o $@ -shared

$(BINARY): $(O_COMMON_DUMMY) $(O_MAIN_DUMMY) | $(EXE_DIR)/.dir_dummy
	$(LD) $(LDFLAGS) -pie  $(MAIN_OBJ) $(MAIN_LIBS) -o $@

$(TEST_EXE): $(O_COMMON_DUMMY) $(O_SO_DUMMY)
$(TEST_EXE): $(O_TEST_DUMMY) $(O_ASM_DUMMY) | $(TEST_EXE_DIR)/.dir_dummy
	$(LD) $(LDFLAGS) $(TEST_OBJ) $(TEST_LIBS) -o $@

.PHONY: clean
clean:
	rm -rf $(CLEAN_FILES)

.PHONY: dir_clean
dir_clean:
	rm -rf $(BUILD_DIR) $(EXE_DIR) $(ASM_GEN_DIR)

ifeq (,$(filter $(NO_DEPS_TARGETS), $(MAKECMDGOALS)))

#next two conditonals prevent make from running on dry run or when
#invoked for tab-completion
ifneq (n,$(findstring n,$(firstword $(MAKEFLAGS))))
ifneq (p,$(findstring p,$(firstword $(MAKEFLAGS))))
-include $(DEP_FILES)
endif
endif


endif
