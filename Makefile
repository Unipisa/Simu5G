all: checkmakefiles
	@cd src && $(MAKE)

tests: all
	@cd src && $(MAKE) && cd ../tests/fingerprint/ && ./fingerprints

tests_lte: all
	@cd src && $(MAKE) && cd ../tests/fingerprint && ./fingerprints lte_*.csv

tests_NR: all
	@cd src && $(MAKE) && cd ../tests/fingerprint && ./fingerprints nr_*.csv

tests_mec: all
	@cd src && $(MAKE) && cd ../tests/fingerprint && ./fingerprints mec*.csv

clean: checkmakefiles
	@cd src && $(MAKE) clean

cleanall: checkmakefiles
	@cd src && $(MAKE) MODE=release clean
	@cd src && $(MAKE) MODE=debug clean
	@rm -f src/Makefile

makefiles:
	@cd src && opp_makemake --make-so -f --deep -o simu5g -O out -KINET_PROJ=$(INET_ROOT) -DINET_IMPORT -I. -I$$\(INET_PROJ\)/src -L$$\(INET_PROJ\)/src -lINET$$\(D\)

checkmakefiles:
	@if [ ! -f src/Makefile ]; then \
	echo; \
	echo '======================================================================='; \
	echo 'src/Makefile does not exist. Please use "make makefiles" to generate it!'; \
	echo '======================================================================='; \
	echo; \
	exit 1; \
	fi

dist:
	releng/makedist

neddoc:
	@opp_neddoc --verbose --no-automatic-hyperlinks -x "/*/simulations,/*/tests,/*/showcases" .
