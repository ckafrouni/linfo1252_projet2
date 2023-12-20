CC := gcc
CFLAGS := -g -Wall -Werror

HEADERS := lib_tar.h
SOURCES := lib_tar.c tests.c
SUBMISSION_TAR := soumission.tar
DIR := test_dir
SUBMISSION_FILES := $(HEADERS) $(SOURCES) Makefile $(DIR) files.tar


all: tests lib_tar.o

lib_tar.o: lib_tar.c lib_tar.h
	@$(CC) $(CFLAGS) -c $<

tests: tests.c lib_tar.o
	@$(CC) $(CFLAGS) -o tests $^

clean:
	rm -rf lib_tar.o tests soumission.tar test_dir.tar

submit: $(SUBMISSION_FILES)
	tar --posix --pax-option delete=".*" --pax-option delete="*time*" --no-xattrs --no-acl --no-selinux -c $^ > $(SUBMISSION_TAR)


simulate: TMP_DIR := $(shell mktemp -u)
simulate: clean submit
	mkdir $(TMP_DIR)
	tar -xf $(SUBMISSION_TAR) -C $(TMP_DIR)
	cd $(TMP_DIR) && make -s run
	rm -rf $(TMP_DIR)
	make -s clean

test: clean all
	@cd $(DIR) && tar --posix --pax-option delete=".*" --pax-option delete="*time*" --no-xattrs --no-acl --no-selinux -c * > ../$(DIR).tar
	@echo "\e[31m====================================\e[0m"
	@echo "\e[31mTesting with \e[32m'$(DIR).tar'\e[0m"
	@echo "\e[31m====================================\e[0m"
	@./tests $(DIR).tar
	@echo "\e[31m====================================\e[0m"
run: test
	@make -s clean
	@rm -rf $(DIR).tar

.PHONY: all clean run test submit simulate
