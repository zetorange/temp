CATEYES_VERSION := $(shell git describe --tags --always --long | sed 's,-,.,g' | cut -f1-3 -d'.')

build_platform := $(shell uname -s | tr '[A-Z]' '[a-z]' | sed 's,^darwin$$,macos,')
build_arch := $(shell releng/detect-arch.sh)
build_platform_arch := $(build_platform)-$(build_arch)

FOR_HOST ?= $(build_platform_arch)

cateyes_gum_flags := --default-library static $(CATEYES_COMMON_FLAGS) $(CATEYES_DIET_FLAGS)
cateyes_core_flags := --default-library static $(CATEYES_COMMON_FLAGS) $(CATEYES_DIET_FLAGS) $(CATEYES_MAPPER_FLAGS)

cateyes_tools := cateyes cateyes-discover cateyes-kill cateyes-ls-devices cateyes-ps cateyes-trace


modules = capstone cateyes-gum cateyes-core cateyes-python cateyes-node cateyes-tools

git-submodules:
#	@if [ ! -f cateyes-core/meson.build ]; then \
#		git submodule init; \
#		git submodule update; \
#	fi
-include git-submodules

define make-update-submodule-stamp
$1-update-submodule-stamp: git-submodules
	@mkdir -p build
	
	
endef
$(foreach m,$(modules),$(eval $(call make-update-submodule-stamp,$m)))
git-submodule-stamps: $(foreach m,$(modules),$m-update-submodule-stamp)
-include git-submodule-stamps

build/cateyes-env-%.rc: releng/setup-env.sh releng/config.site.in build/cateyes-version.h
	CATEYES_HOST=$* \
		CATEYES_OPTIMIZATION_FLAGS="$(CATEYES_OPTIMIZATION_FLAGS)" \
		CATEYES_DEBUG_FLAGS="$(CATEYES_DEBUG_FLAGS)" \
		CATEYES_ASAN=$(CATEYES_ASAN) \
		./releng/setup-env.sh
build/cateyes_thin-env-%.rc: releng/setup-env.sh releng/config.site.in build/cateyes-version.h
	CATEYES_HOST=$* \
		CATEYES_OPTIMIZATION_FLAGS="$(CATEYES_OPTIMIZATION_FLAGS)" \
		CATEYES_DEBUG_FLAGS="$(CATEYES_DEBUG_FLAGS)" \
		CATEYES_ASAN=$(CATEYES_ASAN) \
		CATEYES_ENV_NAME=cateyes_thin \
		./releng/setup-env.sh
	cd $(CATEYES)/build/; \
	ln -sf cateyes_thin-env-$*.rc cateyes-env-$*.rc; \
	ln -sf cateyes_thin-meson-env-$*.rc cateyes-env-$*.rc; \
	ln -sf cateyes_thin-$* cateyes-$*; \
	ln -sf cateyes_thin-sdk-$* sdk-$*; \
	ln -sf cateyes_thin-toolchain-$* toolchain-$*

build/cateyes-version.h: releng/generate-version-header.py
	@python releng/generate-version-header.py > $@.tmp
	@mv $@.tmp $@

glib:
	@make -f Makefile.sdk.mk CATEYES_HOST=$(FOR_HOST) build/fs-$(FOR_HOST)/lib/pkgconfig/glib-2.0.pc
glib-shell:
	@. build/fs-env-$(FOR_HOST).rc && cd build/fs-tmp-$(FOR_HOST)/glib && bash
glib-symlinks:
	@cd build; \
	for candidate in $$(find . -mindepth 1 -maxdepth 1 -type d -name "cateyes-*"); do \
		host_arch=$$(echo $$candidate | cut -f2- -d"-"); \
		if [ -d "fs-tmp-$$host_arch/glib" ]; then \
			echo "✓ $$host_arch"; \
			rm -rf sdk-$$host_arch/include/glib-2.0 sdk-$$host_arch/include/gio-unix-2.0 sdk-$$host_arch/lib/glib-2.0; \
			ln -s ../../fs-$$host_arch/include/glib-2.0 sdk-$$host_arch/include/glib-2.0; \
			ln -s ../../fs-$$host_arch/include/gio-unix-2.0 sdk-$$host_arch/include/gio-unix-2.0; \
			ln -s ../../fs-$$host_arch/lib/glib-2.0 sdk-$$host_arch/lib/glib-2.0; \
			for name in glib gthread gmodule gobject gio; do \
				libname=lib$$name-2.0.a; \
				rm -f sdk-$$host_arch/lib/$$libname; \
				ln -s ../../fs-tmp-$$host_arch/glib/$$name/.libs/$$libname sdk-$$host_arch/lib/$$libname; \
				pcname=$$name-2.0.pc; \
				rm -f sdk-$$host_arch/lib/pkgconfig/$$pcname; \
				ln -s ../../../fs-$$host_arch/lib/pkgconfig/$$pcname sdk-$$host_arch/lib/pkgconfig/$$pcname; \
			done; \
			for name in gmodule-export gmodule-no-export gio-unix; do \
				pcname=$$name-2.0.pc; \
				rm -f sdk-$$host_arch/lib/pkgconfig/$$pcname; \
				ln -s ../../../fs-$$host_arch/lib/pkgconfig/$$pcname sdk-$$host_arch/lib/pkgconfig/$$pcname; \
			done; \
			for name in glib-2.0.m4 glib-gettext.m4 gsettings.m4; do \
				rm -f sdk-$$host_arch/share/aclocal/$$name; \
				ln -s ../../../fs-$$host_arch/share/aclocal/$$name sdk-$$host_arch/share/aclocal/$$name; \
			done; \
			rm -rf sdk-$$host_arch/share/glib-2.0; \
			ln -s ../../fs-$$host_arch/share/glib-2.0 sdk-$$host_arch/share/glib-2.0; \
		fi; \
	done

v8:
	@rm -f build/fs-$(FOR_HOST)/lib/pkgconfig/v8.pc build/fs-tmp-$(FOR_HOST)/.v8-build-stamp
	@make -f Makefile.sdk.mk CATEYES_HOST=$(FOR_HOST) build/fs-$(FOR_HOST)/lib/pkgconfig/v8.pc
v8-symlinks:
	@cd build; \
	for candidate in $$(find . -mindepth 1 -maxdepth 1 -type d -name "cateyes-*"); do \
		host_arch=$$(echo $$candidate | cut -f2- -d"-"); \
		if [ -d "fs-tmp-$$host_arch/v8" ]; then \
			echo "✓ $$host_arch"; \
			rm -rf sdk-$$host_arch/include/v8/include; \
			ln -s ../../../fs-tmp-$$host_arch/v8/include sdk-$$host_arch/include/v8/include; \
			v8_target=$$(basename $$(cd fs-tmp-$$host_arch/v8/out/*.release/ && pwd)); \
			for name in libbase base libplatform libsampler snapshot; do \
				libname=libv8_$$name.a; \
				rm -f sdk-$$host_arch/lib/$$libname; \
				ln -s ../../fs-tmp-$$host_arch/v8/out/$$v8_target/$$libname sdk-$$host_arch/lib/$$libname; \
			done; \
			pcname=v8.pc; \
			rm -f sdk-$$host_arch/lib/pkgconfig/$$pcname; \
			ln -s ../../../fs-$$host_arch/lib/pkgconfig/$$pcname sdk-$$host_arch/lib/pkgconfig/$$pcname; \
		fi; \
	done
