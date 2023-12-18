CC := gcc
CFLAGS := -g -Wall -Werror -Wextra

##################
# Compilation
##################
TARGETS := lib_tar.o

all: $(TARGETS)

lib_tar.o: src/lib_tar.c src/lib_tar.h src/lib_tar_internal.h
	$(CC) $(CFLAGS) -c $<

tests/bin:
	mkdir -p $@


tests/bin/%: tests/%.c lib_tar.o tests/helpers.h
	$(CC) $(CFLAGS) -o $@ $^ -lcriterion

##################
# Submission
##################
SUBMISSION_FILES := src/ tests/ Makefile
SUBMISSION_TAR := soumission.tar

submit: $(SUBMISSION_FILES)
	tar --posix --pax-option delete=".*" --pax-option delete="*time*" --no-xattrs --no-acl --no-selinux -c $^ > $(SUBMISSION_TAR)

test_submit: submit
	mkdir -p tests/bin/test_submit
	tar -xf $(SUBMISSION_TAR) -C tests/bin/test_submit
	$(MAKE) -C tests/bin/test_submit
	$(MAKE) -C tests/bin/test_submit test
	rm -rf tests/bin/test_submit

##################
# Tests
################## 
test: all tests/bin tests/bin/test_dir1
	@for test in tests/bin/*; do \
		./$$test --verbose --always-succeed -j1; \
	done

clean:
	rm -rf lib_tar.o tests/bin $(SUBMISSION_TAR)

.PHONY: clean test all
