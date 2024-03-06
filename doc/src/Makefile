# You can set these variables from the command line.
SPHINXOPTS    = -j8
SPHINXBUILD   = sphinx-build
SPHINXPROJ    = Simu5G
SOURCEDIR     = .
BUILDDIR      = _build

.PHONY: pdf html clean deploy help server

all: html pdf

clean:
	@rm -rf $(BUILDDIR)

help:
	@$(SPHINXBUILD) -M help "$(SOURCEDIR)" "$(BUILDDIR)" $(SPHINXOPTS) $(O)

include tools/sphinx_deployment.mk

server: Makefile
	ln -sf ../../showcases showcases
	@sphinx-autobuild --host 0.0.0.0 --port 8000 "$(SOURCEDIR)" "$(BUILDDIR)/html" $(SPHINXOPTS) $(O) -r .git
	unlink showcases

pdf: Makefile
	ln -sf ../../showcases showcases
	@$(SPHINXBUILD) -M latexpdf "$(SOURCEDIR)" "$(BUILDDIR)" $(SPHINXOPTS) $(O)
	unlink showcases

html: Makefile
	ln -sf ../../showcases showcases
	@$(SPHINXBUILD) -M html "$(SOURCEDIR)" "$(BUILDDIR)" $(SPHINXOPTS) $(O)
	unlink showcases
