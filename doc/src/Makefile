# You can set these variables from the command line.
VENV          = .venv/.project
SPHINXOPTS    = -j8
SPHINXBUILD   = sphinx-build
SPHINXPROJ    = Simu5G
SOURCEDIR     = .
BUILDDIR      = _build

.PHONY: pdf html clean deploy help server

all: html pdf

clean:
	@rm -rf $(BUILDDIR)

help: $(VENV)
	@pipenv run $(SPHINXBUILD) -M help "$(SOURCEDIR)" "$(BUILDDIR)" $(SPHINXOPTS) $(O)

include tools/sphinx_deployment.mk

server: $(VENV)
	@pipenv run sphinx-autobuild --host 0.0.0.0 --port 8000 "$(SOURCEDIR)" "$(BUILDDIR)/html" $(SPHINXOPTS) $(O) --ignore .git

pdf: $(VENV)
	@pipenv run $(SPHINXBUILD) -M latexpdf "$(SOURCEDIR)" "$(BUILDDIR)" $(SPHINXOPTS) $(O)

html: $(VENV)
	@pipenv run $(SPHINXBUILD) -M html "$(SOURCEDIR)" "$(BUILDDIR)" $(SPHINXOPTS) $(O)

$(VENV): Pipfile Pipfile.lock
	@PIPENV_VENV_IN_PROJECT=1 pipenv sync -q
