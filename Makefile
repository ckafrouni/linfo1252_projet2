CC := gcc
CFLAGS := -g -Wall -Werror -Wextra

SRC_DIR := src

TESTS_DIR := tests
TESTS_BIN_DIR := $(TESTS_DIR)/bin
TESTS := $(wildcard $(TESTS_DIR)/*.c)
TESTS_BINS := $(patsubst $(TESTS_DIR)/%.c,$(TESTS_BIN_DIR)/%,$(TESTS))


##################
# Compilation
##################
TARGETS := lib_tar.o

all: $(TARGETS)

lib_tar.o: $(SRC_DIR)/lib_tar.c $(SRC_DIR)/lib_tar.h $(SRC_DIR)/lib_tar_internal.h
	$(CC) $(CFLAGS) -c $< -o $@

$(TESTS_BIN_DIR)/%: $(TESTS_DIR)/%.c lib_tar.o $(TESTS_DIR)/helpers.h
	$(CC) $(CFLAGS) -o $@ $^ -lcriterion

$(TESTS_BIN_DIR):
	mkdir -p $@

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
test: all $(TESTS_BIN_DIR) $(TESTS_BIN_DIR) $(TESTS_BINS) 
	@for test in $(TESTS_BINS); do \
		cd $(TESTS_DIR)/resources/$$(basename $$test) && \
		tar --posix --pax-option delete=".*" --pax-option delete="*time*" --no-xattrs --no-acl --no-selinux -c * > ../../bin/$$(basename $$test).tar && \
		cd - && \
		./$$test --verbose --always-succeed -j1; \
	done

clean:
	$(RM) lib_tar.o $(SUBMISSION_TAR)
	$(RM) -r $(TESTS_BIN_DIR)

.PHONY: clean test all
