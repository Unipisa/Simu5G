all: checkmakefiles
	@cd src && $(MAKE)

tests: tests_lte tests_NR tests_mec
	@echo "All tests done."

tests_lte: all
	@cd src && $(MAKE) && cd ../tests/fingerprint/LTE && ./fingerprints

tests_NR: all
	@cd src && $(MAKE) && cd ../tests/fingerprint/NR && ./fingerprints

tests_mec: all
	@cd src && $(MAKE) && cd ../tests/fingerprint/mec && ./fingerprints

clean: checkmakefiles
	@cd src && $(MAKE) clean

cleanall: checkmakefiles
	@cd src && $(MAKE) MODE=release clean
	@cd src && $(MAKE) MODE=debug clean
	@rm -f src/Makefile

makefiles:
	@cd src && opp_makemake --make-so -f --deep -o simu5g -O out -KINET_PROJ=../../inet4.5 -DINET_IMPORT -I. -I$$\(INET_PROJ\)/src -L$$\(INET_PROJ\)/src -lINET$$\(D\)

checkmakefiles:
	@if [ ! -f src/Makefile ]; then \
	echo; \
	echo '======================================================================='; \
	echo 'src/Makefile does not exist. Please use "make makefiles" to generate it!'; \
	echo '======================================================================='; \
	echo; \
	exit 1; \
	fi
