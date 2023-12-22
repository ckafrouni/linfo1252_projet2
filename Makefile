CC := gcc
CFLAGS := -g -Wall -Werror -Wextra -std=c11

SRC_DIR := src
INCLUDE_DIR := include
BIN_DIR := bin

BINS := $(BIN_DIR)/lib_tar.o

TESTS_DIR := tests
TESTS_BIN_DIR := $(TESTS_DIR)/bin
TESTS := $(wildcard $(TESTS_DIR)/*.c)
TESTS_BINS := $(patsubst $(TESTS_DIR)/%.c,$(TESTS_BIN_DIR)/%,$(TESTS))


##################
# Compilation
##################
all: $(BIN_DIR) $(BINS)

$(BIN_DIR):
	mkdir -p $@

$(TESTS_BIN_DIR):
	mkdir -p $@

$(BIN_DIR)/lib_tar.o: $(SRC_DIR)/lib_tar.c $(INCLUDE_DIR)/lib_tar.h $(INCLUDE_DIR)/lib_tar_internal.h
	$(CC) $(CFLAGS) -c $< -o $@ -I$(INCLUDE_DIR)

$(TESTS_BIN_DIR)/%: $(TESTS_DIR)/%.c $(BIN_DIR)/lib_tar.o $(TESTS_DIR)/helpers.h
	$(CC) $(CFLAGS) -o $@ $^ -lcriterion -I$(INCLUDE_DIR)


##################
# Submission
##################
SUBMISSION_FILES := $(SRC_DIR)/ $(TESTS_DIR)/ Makefile
SUBMISSION_TAR := soumission.tar

submit: $(SUBMISSION_FILES)
	tar --posix --pax-option delete=".*" --pax-option delete="*time*" --no-xattrs --no-acl --no-selinux -c $^ > $(SUBMISSION_TAR)

test_submit: submit
	mkdir -p $(TESTS_BIN_DIR)/test_submit
	tar -xf $(SUBMISSION_TAR) -C $(TESTS_BIN_DIR)/test_submit
	$(MAKE) -C $(TESTS_BIN_DIR)/test_submit
	$(MAKE) -C $(TESTS_BIN_DIR)/test_submit test
	rm -rf $(TESTS_BIN_DIR)/test_submit

##################
# Tests
##################
test: all $(TESTS_BIN_DIR) $(TESTS_BINS) 
	@exit_code=0; \
	for test in $(TESTS_BINS); do \
		cd $(TESTS_DIR)/resources/$$(basename $$test) && \
		tar --posix --pax-option delete=".*" --pax-option delete="*time*" --no-xattrs --no-acl --no-selinux -c * > ../../bin/$$(basename $$test).tar && \
		cd - && \
		./$$test --verbose -j1; \
		if [ $$? -ne 0 ]; then \
			exit_code=1; \
		fi; \
	done; \
	exit $$exit_code

clean:
	$(RM) $(SUBMISSION_TAR)
	$(RM) -r $(TESTS_BIN_DIR) $(BIN_DIR)

.PHONY: clean test all submit test_submit
