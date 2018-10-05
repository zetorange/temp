include config.mk

build_arch := $(shell releng/detect-arch.sh)
test_args := $(addprefix -p=,$(tests))

HELP_FUN = \
	my (%help, @sections); \
	while(<>) { \
		if (/^([\w-]+)\s*:.*\#\#(?:@([\w-]+))?\s(.*)$$/) { \
			$$section = $$2 // 'options'; \
			push @sections, $$section unless exists $$help{$$section}; \
			push @{$$help{$$section}}, [$$1, $$3]; \
		} \
	} \
	$$target_color = "\033[32m"; \
	$$variable_color = "\033[36m"; \
	$$reset_color = "\033[0m"; \
	print "\n"; \
	print "\033[31mUsage:$${reset_color} make $${target_color}TARGET$${reset_color} [$${variable_color}VARIABLE$${reset_color}=value]\n\n"; \
	print "Where $${target_color}TARGET$${reset_color} specifies one or more of:\n"; \
	print "\n"; \
	for (@sections) { \
		print "  /* $$_ */\n"; $$sep = " " x (20 - length $$_->[0]); \
		printf("  $${target_color}%-20s$${reset_color}    %s\n", $$_->[0], $$_->[1]) for @{$$help{$$_}}; \
		print "\n"; \
	} \
	print "And optionally also $${variable_color}VARIABLE$${reset_color} values:\n"; \
	print "  $${variable_color}PYTHON$${reset_color}                  Absolute path of Python interpreter including version suffix\n"; \
	print "  $${variable_color}NODE$${reset_color}                    Absolute path of Node.js binary\n"; \
	print "\n"; \
	print "For example:\n"; \
	print "  \$$ make $${target_color}python-macos $${variable_color}PYTHON$${reset_color}=/usr/local/bin/python3.6\n"; \
	print "  \$$ make $${target_color}node-macos $${variable_color}NODE$${reset_color}=/usr/local/bin/node\n"; \
	print "\n";

help:
	@LC_ALL=C perl -e '$(HELP_FUN)' $(MAKEFILE_LIST)


include releng/common.mk

distclean: clean-submodules
	rm -rf build/

clean: clean-submodules
	rm -f build/.cateyes-gum-npm-stamp
	rm -f build/*-clang*
	rm -f build/*-pkg-config
	rm -f build/*-stamp
	rm -f build/*-strip
	rm -f build/*.rc
	rm -f build/*.sh
	rm -f build/*.site
	rm -f build/*.txt
	rm -f build/cateyes-version.h
	rm -rf build/cateyes-*-*
	rm -rf build/fs-*-*
	rm -rf build/ft-*-*
	rm -rf build/tmp-*-*
	rm -rf build/fs-tmp-*-*
	rm -rf build/ft-tmp-*-*

clean-submodules:
	cd capstone && git clean -xfd
	cd cateyes-gum && git clean -xfd
	cd cateyes-core && git clean -xfd
	cd cateyes-python && git clean -xfd
	cd cateyes-node && git clean -xfd
	cd cateyes-tools && git clean -xfd


define make-capstone-rule
build/$1-%/lib/pkgconfig/capstone.pc: build/$1-env-%.rc build/.capstone-submodule-stamp
	. build/$1-env-$$*.rc \
		&& export PACKAGE_TARNAME=capstone \
		&& . $$$$CONFIG_SITE \
		&& case $$* in \
			*-x86)    capstone_archs="x86"         ;; \
			*-x86_64) capstone_archs="x86"         ;; \
			*-arm)    capstone_archs="arm"         ;; \
			*-arm64)  capstone_archs="aarch64 arm" ;; \
		esac \
		&& make -C capstone \
			PREFIX=$$$$cateyes_prefix \
			BUILDDIR=../build/$2-$$*/capstone \
			CAPSTONE_BUILD_CORE_ONLY=yes \
			CAPSTONE_ARCHS="$$$$capstone_archs" \
			CAPSTONE_SHARED=$$$$enable_shared \
			CAPSTONE_STATIC=$$$$enable_static \
			LIBARCHS="" \
			install
endef
$(eval $(call make-capstone-rule,cateyes,tmp))
$(eval $(call make-capstone-rule,cateyes_thin,tmp_thin))


gum-macos: build/cateyes-macos-x86/lib/pkgconfig/cateyes-gum-1.0.pc build/cateyes-macos-x86_64/lib/pkgconfig/cateyes-gum-1.0.pc ##@gum Build for macOS
gum-macos-thin: build/cateyes_thin-macos-x86_64/lib/pkgconfig/cateyes-gum-1.0.pc ##@gum Build for macOS without cross-arch support
gum-ios: build/cateyes-ios-arm/lib/pkgconfig/cateyes-gum-1.0.pc build/cateyes-ios-arm64/lib/pkgconfig/cateyes-gum-1.0.pc ##@gum Build for iOS
gum-ios-thin: build/cateyes_thin-ios-arm64/lib/pkgconfig/cateyes-gum-1.0.pc ##@gum Build for iOS without cross-arch support
gum-android: build/cateyes-android-arm/lib/pkgconfig/cateyes-gum-1.0.pc build/cateyes-android-arm64/lib/pkgconfig/cateyes-gum-1.0.pc ##@gum Build for Android

define make-gum-rules
build/.$1-gum-npm-stamp: build/$1-env-macos-$$(build_arch).rc
	@$$(NPM) --version &>/dev/null || (echo -e "\033[31mOops. It appears Node.js is not installed.\nWe need it for processing JavaScript code at build-time.\nCheck PATH or set NODE to the absolute path of your Node.js binary.\033[0m"; exit 1;)
	. build/$1-env-macos-$$(build_arch).rc && cd cateyes-gum/bindings/gumjs && npm install
	@touch $$@

build/$1-%/lib/pkgconfig/cateyes-gum-1.0.pc: build/.cateyes-gum-submodule-stamp build/.$1-gum-npm-stamp build/$1-%/lib/pkgconfig/capstone.pc
	. build/$1-meson-env-macos-$$(build_arch).rc; \
	builddir=build/$2-$$*/cateyes-gum; \
	if [ ! -f $$$$builddir/build.ninja ]; then \
		mkdir -p $$$$builddir; \
		$$(MESON) \
			--prefix $$(CATEYES)/build/$1-$$* \
			--cross-file build/$1-$$*.txt \
			$$(cateyes_gum_flags) \
			cateyes-gum $$$$builddir || exit 1; \
	fi; \
	$$(NINJA) -C $$$$builddir install || exit 1
	@touch -c $$@
endef
$(eval $(call make-gum-rules,cateyes,tmp))
$(eval $(call make-gum-rules,cateyes_thin,tmp_thin))

check-gum-macos: build/cateyes-macos-x86/lib/pkgconfig/cateyes-gum-1.0.pc build/cateyes-macos-x86_64/lib/pkgconfig/cateyes-gum-1.0.pc ##@gum Run tests for macOS
	build/tmp-macos-x86/cateyes-gum/tests/gum-tests $(test_args)
	build/tmp-macos-x86_64/cateyes-gum/tests/gum-tests $(test_args)
check-gum-macos-thin: build/cateyes_thin-macos-x86_64/lib/pkgconfig/cateyes-gum-1.0.pc ##@gum Run tests for macOS without cross-arch support
	build/tmp_thin-macos-x86_64/cateyes-gum/tests/gum-tests $(test_args)


core-macos: build/cateyes-macos-x86/lib/pkgconfig/cateyes-core-1.0.pc build/cateyes-macos-x86_64/lib/pkgconfig/cateyes-core-1.0.pc ##@core Build for macOS
core-macos-thin: build/cateyes_thin-macos-x86_64/lib/pkgconfig/cateyes-core-1.0.pc ##@core Build for macOS without cross-arch support
core-ios: build/cateyes-ios-arm/lib/pkgconfig/cateyes-core-1.0.pc build/cateyes-ios-arm64/lib/pkgconfig/cateyes-core-1.0.pc ##@core Build for iOS
core-ios-thin: build/cateyes_thin-ios-arm64/lib/pkgconfig/cateyes-core-1.0.pc ##@core Build for iOS without cross-arch support
core-android: build/cateyes-android-arm/lib/pkgconfig/cateyes-core-1.0.pc build/cateyes-android-arm64/lib/pkgconfig/cateyes-core-1.0.pc ##@core Build for Android

build/tmp-macos-%/cateyes-core/.cateyes-ninja-stamp: build/.cateyes-core-submodule-stamp build/cateyes-macos-%/lib/pkgconfig/cateyes-gum-1.0.pc
	. build/cateyes-meson-env-macos-$(build_arch).rc; \
	builddir=$(@D); \
	if [ ! -f $$builddir/build.ninja ]; then \
		mkdir -p $$builddir; \
		$(MESON) \
			--prefix $(CATEYES)/build/cateyes-macos-$* \
			--cross-file build/cateyes-macos-$*.txt \
			$(cateyes_core_flags) \
			-Dwith-64bit-helper=$(CATEYES)/build/tmp-macos-x86_64/cateyes-core/src/cateyes-helper \
			-Dwith-32bit-agent=$(CATEYES)/build/tmp-macos-x86/cateyes-core/lib/agent/cateyes-agent.dylib \
			-Dwith-64bit-agent=$(CATEYES)/build/tmp-macos-x86_64/cateyes-core/lib/agent/cateyes-agent.dylib \
			cateyes-core $$builddir || exit 1; \
	fi
	@touch $@
build/tmp-ios-x86/cateyes-core/.cateyes-ninja-stamp: build/.cateyes-core-submodule-stamp build/cateyes-ios-x86/lib/pkgconfig/cateyes-gum-1.0.pc
	. build/cateyes-meson-env-macos-$(build_arch).rc; \
	builddir=$(@D); \
	if [ ! -f $$builddir/build.ninja ]; then \
		mkdir -p $$builddir; \
		$(MESON) \
			--prefix $(CATEYES)/build/cateyes-ios-x86 \
			--cross-file build/cateyes-ios-x86.txt \
			$(cateyes_core_flags) \
			cateyes-core $$builddir || exit 1; \
	fi
	@touch $@
build/tmp-ios-x86_64/cateyes-core/.cateyes-ninja-stamp: build/.cateyes-core-submodule-stamp build/cateyes-ios-x86_64/lib/pkgconfig/cateyes-gum-1.0.pc
	. build/cateyes-meson-env-macos-$(build_arch).rc; \
	builddir=$(@D); \
	if [ ! -f $$builddir/build.ninja ]; then \
		mkdir -p $$builddir; \
		$(MESON) \
			--prefix $(CATEYES)/build/cateyes-ios-x86_64 \
			--cross-file build/cateyes-ios-x86_64.txt \
			$(cateyes_core_flags) \
			-Dwith-64bit-helper=$(CATEYES)/build/tmp-ios-x86_64/cateyes-core/src/cateyes-helper \
			-Dwith-32bit-agent=$(CATEYES)/build/tmp-ios-x86/cateyes-core/lib/agent/cateyes-agent.dylib \
			-Dwith-64bit-agent=$(CATEYES)/build/tmp-ios-x86_64/cateyes-core/lib/agent/cateyes-agent.dylib \
			cateyes-core $$builddir || exit 1; \
	fi
	@touch $@
build/tmp-ios-arm/cateyes-core/.cateyes-ninja-stamp: build/.cateyes-core-submodule-stamp build/cateyes-ios-arm/lib/pkgconfig/cateyes-gum-1.0.pc
	. build/cateyes-meson-env-macos-$(build_arch).rc; \
	builddir=$(@D); \
	if [ ! -f $$builddir/build.ninja ]; then \
		mkdir -p $$builddir; \
		$(MESON) \
			--prefix $(CATEYES)/build/cateyes-ios-arm \
			--cross-file build/cateyes-ios-arm.txt \
			$(cateyes_core_flags) \
			cateyes-core $$builddir || exit 1; \
	fi
	@touch $@
build/tmp-ios-arm64/cateyes-core/.cateyes-ninja-stamp: build/.cateyes-core-submodule-stamp build/cateyes-ios-arm64/lib/pkgconfig/cateyes-gum-1.0.pc
	. build/cateyes-meson-env-macos-$(build_arch).rc; \
	builddir=$(@D); \
	if [ ! -f $$builddir/build.ninja ]; then \
		mkdir -p $$builddir; \
		$(MESON) \
			--prefix $(CATEYES)/build/cateyes-ios-arm64 \
			--cross-file build/cateyes-ios-arm64.txt \
			$(cateyes_core_flags) \
			-Dwith-64bit-helper=$(CATEYES)/build/tmp-ios-arm64/cateyes-core/src/cateyes-helper \
			-Dwith-32bit-agent=$(CATEYES)/build/tmp-ios-arm/cateyes-core/lib/agent/cateyes-agent.dylib \
			-Dwith-64bit-agent=$(CATEYES)/build/tmp-ios-arm64/cateyes-core/lib/agent/cateyes-agent.dylib \
			cateyes-core $$builddir || exit 1; \
	fi
	@touch $@
build/tmp-android-x86/cateyes-core/.cateyes-ninja-stamp: build/.cateyes-core-submodule-stamp build/cateyes-android-x86/lib/pkgconfig/cateyes-gum-1.0.pc
	. build/cateyes-meson-env-macos-$(build_arch).rc; \
	builddir=$(@D); \
	if [ ! -f $$builddir/build.ninja ]; then \
		mkdir -p $$builddir; \
		$(MESON) \
			--prefix $(CATEYES)/build/cateyes-android-x86 \
			--cross-file build/cateyes-android-x86.txt \
			$(cateyes_core_flags) \
			cateyes-core $$builddir || exit 1; \
	fi
	@touch $@
build/tmp-android-x86_64/cateyes-core/.cateyes-ninja-stamp: build/.cateyes-core-submodule-stamp build/cateyes-android-x86_64/lib/pkgconfig/cateyes-gum-1.0.pc
	. build/cateyes-meson-env-macos-$(build_arch).rc; \
	builddir=$(@D); \
	if [ ! -f $$builddir/build.ninja ]; then \
		mkdir -p $$builddir; \
		$(MESON) \
			--prefix $(CATEYES)/build/cateyes-android-x86_64 \
			--cross-file build/cateyes-android-x86_64.txt \
			$(cateyes_core_flags) \
			-Dwith-32bit-helper=$(CATEYES)/build/tmp-android-x86/cateyes-core/src/cateyes-helper \
			-Dwith-64bit-helper=$(CATEYES)/build/tmp-android-x86_64/cateyes-core/src/cateyes-helper \
			-Dwith-32bit-agent=$(CATEYES)/build/tmp-android-x86/cateyes-core/lib/agent/cateyes-agent.so \
			-Dwith-64bit-agent=$(CATEYES)/build/tmp-android-x86_64/cateyes-core/lib/agent/cateyes-agent.so \
			cateyes-core $$builddir || exit 1; \
	fi
	@touch $@
build/tmp-android-arm/cateyes-core/.cateyes-ninja-stamp: build/.cateyes-core-submodule-stamp build/cateyes-android-arm/lib/pkgconfig/cateyes-gum-1.0.pc
	. build/cateyes-meson-env-macos-$(build_arch).rc; \
	builddir=$(@D); \
	if [ ! -f $$builddir/build.ninja ]; then \
		mkdir -p $$builddir; \
		$(MESON) \
			--prefix $(CATEYES)/build/cateyes-android-arm \
			--cross-file build/cateyes-android-arm.txt \
			$(cateyes_core_flags) \
			cateyes-core $$builddir || exit 1; \
	fi
	@touch $@
build/tmp-android-arm64/cateyes-core/.cateyes-ninja-stamp: build/.cateyes-core-submodule-stamp build/cateyes-android-arm64/lib/pkgconfig/cateyes-gum-1.0.pc
	. build/cateyes-meson-env-macos-$(build_arch).rc; \
	builddir=$(@D); \
	if [ ! -f $$builddir/build.ninja ]; then \
		mkdir -p $$builddir; \
		$(MESON) \
			--prefix $(CATEYES)/build/cateyes-android-arm64 \
			--cross-file build/cateyes-android-arm64.txt \
			$(cateyes_core_flags) \
			-Dwith-32bit-helper=$(CATEYES)/build/tmp-android-arm/cateyes-core/src/cateyes-helper \
			-Dwith-64bit-helper=$(CATEYES)/build/tmp-android-arm64/cateyes-core/src/cateyes-helper \
			-Dwith-32bit-agent=$(CATEYES)/build/tmp-android-arm/cateyes-core/lib/agent/cateyes-agent.so \
			-Dwith-64bit-agent=$(CATEYES)/build/tmp-android-arm64/cateyes-core/lib/agent/cateyes-agent.so \
			cateyes-core $$builddir || exit 1; \
	fi
	@touch $@
build/tmp_thin-%/cateyes-core/.cateyes-ninja-stamp: build/.cateyes-core-submodule-stamp build/cateyes_thin-%/lib/pkgconfig/cateyes-gum-1.0.pc
	. build/cateyes_thin-meson-env-macos-$(build_arch).rc; \
	builddir=$(@D); \
	if [ ! -f $$builddir/build.ninja ]; then \
		mkdir -p $$builddir; \
		$(MESON) \
			--prefix $(CATEYES)/build/cateyes_thin-$* \
			--cross-file build/cateyes_thin-$*.txt \
			$(cateyes_core_flags) \
			cateyes-core $$builddir || exit 1; \
	fi
	@touch $@

build/cateyes-macos-%/lib/pkgconfig/cateyes-core-1.0.pc: build/tmp-macos-x86/cateyes-core/.cateyes-agent-stamp build/tmp-macos-x86_64/cateyes-core/.cateyes-helper-and-agent-stamp
	. build/cateyes-meson-env-macos-$(build_arch).rc && $(NINJA) -C build/tmp-macos-$*/cateyes-core install
	@touch $@
build/cateyes-ios-x86/lib/pkgconfig/cateyes-core-1.0.pc: build/tmp-ios-x86/cateyes-core/.cateyes-helper-and-agent-stamp
	. build/cateyes-meson-env-macos-$(build_arch).rc && $(NINJA) -C build/tmp-ios-x86/cateyes-core install
	@touch $@
build/cateyes-ios-x86_64/lib/pkgconfig/cateyes-core-1.0.pc: build/tmp-ios-x86/cateyes-core/.cateyes-agent-stamp build/tmp-ios-x86_64/cateyes-core/.cateyes-helper-and-agent-stamp
	. build/cateyes-meson-env-macos-$(build_arch).rc && $(NINJA) -C build/tmp-ios-x86_64/cateyes-core install
	@touch $@
build/cateyes-ios-arm/lib/pkgconfig/cateyes-core-1.0.pc: build/tmp-ios-arm/cateyes-core/.cateyes-helper-and-agent-stamp
	. build/cateyes-meson-env-macos-$(build_arch).rc && $(NINJA) -C build/tmp-ios-arm/cateyes-core install
	@touch $@
build/cateyes-ios-arm64/lib/pkgconfig/cateyes-core-1.0.pc: build/tmp-ios-arm/cateyes-core/.cateyes-agent-stamp build/tmp-ios-arm64/cateyes-core/.cateyes-helper-and-agent-stamp
	. build/cateyes-meson-env-macos-$(build_arch).rc && $(NINJA) -C build/tmp-ios-arm64/cateyes-core install
	@touch $@
build/cateyes-android-x86/lib/pkgconfig/cateyes-core-1.0.pc: build/tmp-android-x86/cateyes-core/.cateyes-helper-and-agent-stamp
	. build/cateyes-meson-env-macos-$(build_arch).rc && $(NINJA) -C build/tmp-android-x86/cateyes-core install
	@touch $@
build/cateyes-android-x86_64/lib/pkgconfig/cateyes-core-1.0.pc: build/tmp-android-x86/cateyes-core/.cateyes-helper-and-agent-stamp build/tmp-android-x86_64/cateyes-core/.cateyes-helper-and-agent-stamp
	. build/cateyes-meson-env-macos-$(build_arch).rc && $(NINJA) -C build/tmp-android-x86_64/cateyes-core install
	@touch $@
build/cateyes-android-arm/lib/pkgconfig/cateyes-core-1.0.pc: build/tmp-android-arm/cateyes-core/.cateyes-helper-and-agent-stamp
	. build/cateyes-meson-env-macos-$(build_arch).rc && $(NINJA) -C build/tmp-android-arm/cateyes-core install
	@touch $@
build/cateyes-android-arm64/lib/pkgconfig/cateyes-core-1.0.pc: build/tmp-android-arm/cateyes-core/.cateyes-helper-and-agent-stamp build/tmp-android-arm64/cateyes-core/.cateyes-helper-and-agent-stamp
	. build/cateyes-meson-env-macos-$(build_arch).rc && $(NINJA) -C build/tmp-android-arm64/cateyes-core install
	@touch $@
build/cateyes_thin-%/lib/pkgconfig/cateyes-core-1.0.pc: build/tmp_thin-%/cateyes-core/.cateyes-ninja-stamp
	. build/cateyes_thin-meson-env-macos-$(build_arch).rc && $(NINJA) -C build/tmp_thin-$*/cateyes-core install
	@touch $@

build/tmp-macos-%/cateyes-core/.cateyes-helper-and-agent-stamp: build/tmp-macos-%/cateyes-core/.cateyes-ninja-stamp
	. build/cateyes-meson-env-macos-$(build_arch).rc && $(NINJA) -C build/tmp-macos-$*/cateyes-core src/cateyes-helper lib/agent/cateyes-agent.dylib
	@touch $@
build/tmp-macos-%/cateyes-core/.cateyes-agent-stamp: build/tmp-macos-%/cateyes-core/.cateyes-ninja-stamp
	. build/cateyes-meson-env-macos-$(build_arch).rc && $(NINJA) -C build/tmp-macos-$*/cateyes-core lib/agent/cateyes-agent.dylib
	@touch $@
build/tmp-ios-%/cateyes-core/.cateyes-helper-and-agent-stamp: build/tmp-ios-%/cateyes-core/.cateyes-ninja-stamp
	. build/cateyes-meson-env-macos-$(build_arch).rc && $(NINJA) -C build/tmp-ios-$*/cateyes-core src/cateyes-helper lib/agent/cateyes-agent.dylib
	@touch $@
build/tmp-ios-%/cateyes-core/.cateyes-agent-stamp: build/tmp-ios-%/cateyes-core/.cateyes-ninja-stamp
	. build/cateyes-meson-env-macos-$(build_arch).rc && $(NINJA) -C build/tmp-ios-$*/cateyes-core lib/agent/cateyes-agent.dylib
	@touch $@
build/tmp-android-%/cateyes-core/.cateyes-helper-and-agent-stamp: build/tmp-android-%/cateyes-core/.cateyes-ninja-stamp
	. build/cateyes-meson-env-macos-$(build_arch).rc && $(NINJA) -C build/tmp-android-$*/cateyes-core src/cateyes-helper lib/agent/cateyes-agent.so
	@touch $@

build/cateyes-macos-universal/lib/CateyesGadget.dylib: build/cateyes-macos-x86/lib/pkgconfig/cateyes-core-1.0.pc build/cateyes-macos-x86_64/lib/pkgconfig/cateyes-core-1.0.pc
	@if [ -z "$$MAC_CERTID" ]; then echo "MAC_CERTID not set, see https://github.com/cateyes/cateyes#macos-and-macos"; exit 1; fi
	mkdir -p $(@D)
	cp build/cateyes-macos-x86/lib/CateyesGadget.dylib $(@D)/CateyesGadget-x86.dylib
	cp build/cateyes-macos-x86_64/lib/CateyesGadget.dylib $(@D)/CateyesGadget-x86_64.dylib
	. build/cateyes-env-macos-x86_64.rc \
		&& $$STRIP $$STRIP_FLAGS $(@D)/CateyesGadget-x86.dylib $(@D)/CateyesGadget-x86_64.dylib \
		&& $$LIPO $(@D)/CateyesGadget-x86.dylib $(@D)/CateyesGadget-x86_64.dylib -create -output $@.tmp \
		&& $$INSTALL_NAME_TOOL -id @executable_path/../Frameworks/CateyesGadget.dylib $@.tmp \
		&& $$CODESIGN -f -s "$$MAC_CERTID" $@.tmp
	rm $(@D)/CateyesGadget-*.dylib
	mv $@.tmp $@
build/cateyes_thin-macos-%/lib/CateyesGadget.dylib: build/cateyes_thin-macos-%/lib/pkgconfig/cateyes-core-1.0.pc
	@if [ -z "$$MAC_CERTID" ]; then echo "MAC_CERTID not set, see https://github.com/cateyes/cateyes#macos-and-macos"; exit 1; fi
	mkdir -p $(@D)
	cp build/cateyes_thin-macos-$*/lib/CateyesGadget.dylib $@.tmp
	. build/cateyes_thin-env-macos-$*.rc \
		&& $$STRIP $$STRIP_FLAGS $@.tmp \
		&& $$INSTALL_NAME_TOOL -id @executable_path/../Frameworks/CateyesGadget.dylib $@.tmp \
		&& $$CODESIGN -f -s "$$MAC_CERTID" $@.tmp
	mv $@.tmp $@
build/cateyes-ios-universal/lib/CateyesGadget.dylib: build/cateyes-ios-x86/lib/pkgconfig/cateyes-core-1.0.pc build/cateyes-ios-x86_64/lib/pkgconfig/cateyes-core-1.0.pc build/cateyes-ios-arm/lib/pkgconfig/cateyes-core-1.0.pc build/cateyes-ios-arm64/lib/pkgconfig/cateyes-core-1.0.pc
	@if [ -z "$$IOS_CERTID" ]; then echo "IOS_CERTID not set, see https://github.com/cateyes/cateyes#macos-and-ios"; exit 1; fi
	mkdir -p $(@D)
	cp build/cateyes-ios-x86/lib/CateyesGadget.dylib $(@D)/CateyesGadget-x86.dylib
	cp build/cateyes-ios-x86_64/lib/CateyesGadget.dylib $(@D)/CateyesGadget-x86_64.dylib
	cp build/cateyes-ios-arm/lib/CateyesGadget.dylib $(@D)/CateyesGadget-arm.dylib
	cp build/cateyes-ios-arm64/lib/CateyesGadget.dylib $(@D)/CateyesGadget-arm64.dylib
	. build/cateyes-env-ios-arm64.rc \
		&& $$STRIP $$STRIP_FLAGS $(@D)/CateyesGadget-x86.dylib $(@D)/CateyesGadget-x86_64.dylib $(@D)/CateyesGadget-arm.dylib $(@D)/CateyesGadget-arm64.dylib \
		&& $$LIPO $(@D)/CateyesGadget-x86.dylib $(@D)/CateyesGadget-x86_64.dylib $(@D)/CateyesGadget-arm.dylib $(@D)/CateyesGadget-arm64.dylib -create -output $@.tmp \
		&& $$INSTALL_NAME_TOOL -id @executable_path/Frameworks/CateyesGadget.dylib $@.tmp \
		&& $$CODESIGN -f -s "$$IOS_CERTID" $@.tmp
	rm $(@D)/CateyesGadget-*.dylib
	mv $@.tmp $@
build/cateyes_thin-ios-%/lib/CateyesGadget.dylib: build/cateyes_thin-ios-%/lib/pkgconfig/cateyes-core-1.0.pc
	@if [ -z "$$IOS_CERTID" ]; then echo "IOS_CERTID not set, see https://github.com/cateyes/cateyes#ios-and-ios"; exit 1; fi
	mkdir -p $(@D)
	cp build/cateyes_thin-ios-$*/lib/CateyesGadget.dylib $@.tmp
	. build/cateyes_thin-env-ios-$*.rc \
		&& $$STRIP $$STRIP_FLAGS $@.tmp \
		&& $$INSTALL_NAME_TOOL -id @executable_path/../Frameworks/CateyesGadget.dylib $@.tmp \
		&& $$CODESIGN -f -s "$$IOS_CERTID" $@.tmp
	mv $@.tmp $@

check-core-macos: build/cateyes-macos-x86/lib/pkgconfig/cateyes-core-1.0.pc build/cateyes-macos-x86_64/lib/pkgconfig/cateyes-core-1.0.pc ##@core Run tests for macOS
	build/tmp-macos-x86/cateyes-core/tests/cateyes-tests $(test_args)
	build/tmp-macos-x86_64/cateyes-core/tests/cateyes-tests $(test_args)
check-core-macos-thin: build/cateyes_thin-macos-x86_64/lib/pkgconfig/cateyes-core-1.0.pc ##@core Run tests for macOS without cross-arch support
	build/tmp_thin-macos-x86_64/cateyes-core/tests/cateyes-tests $(test_args)

server-macos: build/cateyes-macos-x86_64/lib/pkgconfig/cateyes-core-1.0.pc ##@server Build for macOS
server-macos-thin: build/cateyes_thin-macos-x86_64/lib/pkgconfig/cateyes-core-1.0.pc ##@server Build for macOS without cross-arch support
server-ios: build/cateyes-ios-arm/lib/pkgconfig/cateyes-core-1.0.pc build/cateyes-ios-arm64/lib/pkgconfig/cateyes-core-1.0.pc ##@server Build for iOS
server-ios-thin: build/cateyes_thin-ios-arm64/lib/pkgconfig/cateyes-core-1.0.pc ##@server Build for iOS without cross-arch support
server-android: build/cateyes-android-x86/lib/pkgconfig/cateyes-core-1.0.pc build/cateyes-android-x86_64/lib/pkgconfig/cateyes-core-1.0.pc build/cateyes-android-arm/lib/pkgconfig/cateyes-core-1.0.pc build/cateyes-android-arm64/lib/pkgconfig/cateyes-core-1.0.pc ##@server Build for Android

gadget-macos: build/cateyes-macos-universal/lib/CateyesGadget.dylib ##@gadget Build for macOS
gadget-macos-thin: build/cateyes_thin-macos-x86_64/lib/CateyesGadget.dylib ##@gadget Build for macOS without cross-arch support
gadget-ios: build/cateyes-ios-universal/lib/CateyesGadget.dylib ##@gadget Build for iOS
gadget-ios-thin: build/cateyes_thin-ios-arm64/lib/CateyesGadget.dylib ##@gadget Build for iOS without cross-arch support
gadget-android: build/cateyes-android-x86/lib/pkgconfig/cateyes-core-1.0.pc build/cateyes-android-x86_64/lib/pkgconfig/cateyes-core-1.0.pc build/cateyes-android-arm/lib/pkgconfig/cateyes-core-1.0.pc build/cateyes-android-arm64/lib/pkgconfig/cateyes-core-1.0.pc ##@gadget Build for Android


python-macos: build/cateyes-macos-universal/lib/$(PYTHON_NAME)/site-packages/cateyes build/cateyes-macos-universal/lib/$(PYTHON_NAME)/site-packages/_cateyes.so ##@python Build Python bindings for macOS
python-macos-thin: build/tmp_thin-macos-x86_64/cateyes-$(PYTHON_NAME)/.cateyes-stamp ##@python Build Python bindings for macOS without cross-arch support

define make-python-rule
build/$2-%/cateyes-$$(PYTHON_NAME)/.cateyes-stamp: build/.cateyes-python-submodule-stamp build/$1-%/lib/pkgconfig/cateyes-core-1.0.pc
	. build/$1-meson-env-macos-$$(build_arch).rc; \
	builddir=$$(@D); \
	if [ ! -f $$$$builddir/build.ninja ]; then \
		mkdir -p $$$$builddir; \
		$$(MESON) \
			--prefix $$(CATEYES)/build/$1-$$* \
			--cross-file build/$1-$$*.txt \
			-Dwith-python=$$(PYTHON) \
			cateyes-python $$$$builddir || exit 1; \
	fi; \
	$$(NINJA) -C $$$$builddir install || exit 1
	@touch $$@
endef
$(eval $(call make-python-rule,cateyes,tmp))
$(eval $(call make-python-rule,cateyes_thin,tmp_thin))
build/cateyes-macos-universal/lib/$(PYTHON_NAME)/site-packages/cateyes: build/tmp-macos-x86_64/cateyes-$(PYTHON_NAME)/.cateyes-stamp
	rm -rf $@
	mkdir -p $(@D)
	cp -a build/cateyes-macos-x86_64/lib/$(PYTHON_NAME)/site-packages/cateyes $@
	@touch $@
build/cateyes-macos-universal/lib/$(PYTHON_NAME)/site-packages/_cateyes.so: build/tmp-macos-x86/cateyes-$(PYTHON_NAME)/.cateyes-stamp build/tmp-macos-x86_64/cateyes-$(PYTHON_NAME)/.cateyes-stamp
	mkdir -p $(@D)
	cp build/cateyes-macos-x86/lib/$(PYTHON_NAME)/site-packages/_cateyes.so $(@D)/_cateyes-32.so
	cp build/cateyes-macos-x86_64/lib/$(PYTHON_NAME)/site-packages/_cateyes.so $(@D)/_cateyes-64.so
	. build/cateyes-env-macos-$(build_arch).rc \
		&& $$STRIP $$STRIP_FLAGS $(@D)/_cateyes-32.so $(@D)/_cateyes-64.so \
		&& $$LIPO $(@D)/_cateyes-32.so $(@D)/_cateyes-64.so -create -output $@
	rm $(@D)/_cateyes-32.so $(@D)/_cateyes-64.so

check-python-macos: python-macos ##@python Test Python bindings for macOS
	export PYTHONPATH="$(shell pwd)/build/cateyes-macos-universal/lib/$(PYTHON_NAME)/site-packages" \
		&& cd cateyes-python \
		&& $(PYTHON) -m unittest discover
check-python-macos-thin: python-macos-thin ##@python Test Python bindings for macOS without cross-arch support
	export PYTHONPATH="$(shell pwd)/build/cateyes_thin-macos-x86_64/lib/$(PYTHON_NAME)/site-packages" \
		&& cd cateyes-python \
		&& $(PYTHON) -m unittest discover


node-macos: build/cateyes-macos-$(build_arch)/lib/node_modules/cateyes ##@node Build Node.js bindings for macOS
node-macos-thin: build/cateyes_thin-macos-x86_64/lib/node_modules/cateyes ##@node Build Node.js bindings for macOS without cross-arch support

define make-node-rule
build/$1-%/lib/node_modules/cateyes: build/$1-%/lib/pkgconfig/cateyes-core-1.0.pc build/.cateyes-node-submodule-stamp
	@$$(NPM) --version &>/dev/null || (echo -e "\033[31mOops. It appears Node.js is not installed.\nCheck PATH or set NODE to the absolute path of your Node.js binary.\033[0m"; exit 1;)
	export PATH=$$(NODE_BIN_DIR):$$$$PATH CATEYES=$$(CATEYES) \
		&& cd cateyes-node \
		&& rm -rf cateyes-0.0.0.tgz build node_modules \
		&& $$(NPM) install \
		&& $$(NPM) pack \
		&& rm -rf ../$$@/ ../$$@.tmp/ \
		&& mkdir -p ../$$@.tmp/build/ \
		&& tar -C ../$$@.tmp/ --strip-components 1 -x -f cateyes-0.0.0.tgz \
		&& rm cateyes-0.0.0.tgz \
		&& mv build/Release/cateyes_binding.node ../$$@.tmp/build/ \
		&& rm -rf build \
		&& mv node_modules ../$$@.tmp/ \
		&& . ../build/$1-env-macos-$$(build_arch).rc && $$$$STRIP $$$$STRIP_FLAGS ../$$@.tmp/build/cateyes_binding.node \
		&& mv ../$$@.tmp ../$$@
endef
$(eval $(call make-node-rule,cateyes,tmp))
$(eval $(call make-node-rule,cateyes_thin,tmp_thin))

define run-node-tests
	export PATH=$3:$$PATH CATEYES=$2 \
		&& cd cateyes-node \
		&& git clean -xfd \
		&& $5 install \
		&& $4 \
			--expose-gc \
			../build/$1/lib/node_modules/cateyes/node_modules/.bin/_mocha \
			-r ts-node/register \
			--timeout 60000 \
			test/*.ts
endef
check-node-macos: node-macos ##@node Test Node.js bindings for macOS
	$(call run-node-tests,cateyes-macos-$(build_arch),$(CATEYES),$(NODE_BIN_DIR),$(NODE),$(NPM))
check-node-macos-thin: node-macos-thin ##@node Test Node.js bindings for macOS without cross-arch support
	$(call run-node-tests,cateyes_thin-macos-$(build_arch),$(CATEYES),$(NODE_BIN_DIR),$(NODE),$(NPM))


tools-macos: build/cateyes-macos-universal/bin/cateyes build/cateyes-macos-universal/lib/$(PYTHON_NAME)/site-packages/cateyes_tools ##@tools Build CLI tools for macOS
tools-macos-thin: build/tmp_thin-macos-x86_64/cateyes-tools-$(PYTHON_NAME)/.cateyes-stamp ##@tools Build CLI tools for macOS without cross-arch support

define make-tools-rule
build/$2-%/cateyes-tools-$$(PYTHON_NAME)/.cateyes-stamp: build/.cateyes-tools-submodule-stamp build/$2-%/cateyes-$$(PYTHON_NAME)/.cateyes-stamp
	. build/$1-meson-env-macos-$$(build_arch).rc; \
	builddir=$$(@D); \
	if [ ! -f $$$$builddir/build.ninja ]; then \
		mkdir -p $$$$builddir; \
		$$(MESON) \
			--prefix $$(CATEYES)/build/$1-$$* \
			--cross-file build/$1-$$*.txt \
			-Dwith-python=$$(PYTHON) \
			cateyes-tools $$$$builddir || exit 1; \
	fi; \
	$$(NINJA) -C $$$$builddir install || exit 1
	@touch $$@
endef
$(eval $(call make-tools-rule,cateyes,tmp))
$(eval $(call make-tools-rule,cateyes_thin,tmp_thin))
build/cateyes-macos-universal/bin/cateyes: build/tmp-macos-x86_64/cateyes-tools-$(PYTHON_NAME)/.cateyes-stamp
	mkdir -p build/cateyes-macos-universal/bin
	for tool in $(cateyes_tools); do \
		cp build/cateyes-macos-x86_64/bin/$$tool build/cateyes-macos-universal/bin/; \
	done
build/cateyes-macos-universal/lib/$(PYTHON_NAME)/site-packages/cateyes_tools: build/tmp-macos-x86_64/cateyes-tools-$(PYTHON_NAME)/.cateyes-stamp
	rm -rf $@
	mkdir -p $(@D)
	cp -a build/cateyes-macos-x86_64/lib/$(PYTHON_NAME)/site-packages/cateyes_tools $@
	@touch $@

check-tools-macos: tools-macos ##@tools Test CLI tools for macOS
	export PYTHONPATH="$(shell pwd)/build/cateyes-macos-universal/lib/$(PYTHON_NAME)/site-packages" \
		&& cd cateyes-tools \
		&& $(PYTHON) -m unittest discover
check-tools-macos-thin: tools-macos-thin ##@tools Test CLI tools for macOS without cross-arch support
	export PYTHONPATH="$(shell pwd)/build/cateyes_thin-macos-x86_64/lib/$(PYTHON_NAME)/site-packages" \
		&& cd cateyes-tools \
		&& $(PYTHON) -m unittest discover


.PHONY: \
	distclean clean clean-submodules git-submodules git-submodule-stamps \
	capstone-update-submodule-stamp \
	gum-macos gum-macos-thin gum-ios gum-ios-thin gum-android check-gum-macos check-gum-macos-thin cateyes-gum-update-submodule-stamp \
	core-macos core-macos-thin core-ios core-ios-thin core-android check-core-macos check-core-macos-thin check-core-android-arm64 cateyes-core-update-submodule-stamp \
	server-macos server-macos-thin server-ios server-ios-thin server-android \
	gadget-macos gadget-macos-thin gadget-ios gadget-ios-thin gadget-android \
	python-macos python-macos-thin check-python-macos check-python-macos-thin cateyes-python-update-submodule-stamp \
	node-macos node-macos-thin check-node-macos check-node-macos-thin cateyes-node-update-submodule-stamp \
	tools-macos tools-macos-thin check-tools-macos check-tools-macos-thin cateyes-tools-update-submodule-stamp \
	glib glib-shell glib-symlinks \
	v8 v8-symlinks
.SECONDARY:
