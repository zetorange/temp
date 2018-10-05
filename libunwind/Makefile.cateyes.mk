CATEYES_HOST ?= qnx-i486

all: integrate

configure:
	(. ../build/fs-env-$(CATEYES_HOST).rc \
		&& autoreconf -ifv \
		&& rm -rf ../build/fs-tmp-$(CATEYES_HOST)/libunwind \
		&& mkdir ../build/fs-tmp-$(CATEYES_HOST)/libunwind \
		&& cd ../build/fs-tmp-$(CATEYES_HOST)/libunwind \
		&& ../../../libunwind/configure)

check:
	(. ../build/fs-env-$(CATEYES_HOST).rc \
		&& make -C ../build/fs-tmp-$(CATEYES_HOST)/libunwind install \
		&& $$CC -Wall -pipe -O0 -I../build/fs-$(CATEYES_HOST)/include -L../build/fs-$(CATEYES_HOST)/lib cateyes-test.c -o cateyes-test -lunwind -llzma)
	scp cateyes-test qnx:/root/
	ssh qnx "/root/cateyes-test"

integrate:
	(. ../build/fs-env-$(CATEYES_HOST).rc && make -C ../build/fs-tmp-$(CATEYES_HOST)/libunwind install)
	cp ../build/fs-$(CATEYES_HOST)/lib/libunwind.a ../build/sdk-$(CATEYES_HOST)/lib/libunwind.a
	(. ../build/cateyes-env-$(CATEYES_HOST).rc && make -C ../build/tmp-$(CATEYES_HOST)/cateyes-gum)

.PHONY: configure check integrate
