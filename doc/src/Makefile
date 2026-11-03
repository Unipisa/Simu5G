# Builds the website like the GitHub workflow does (see README), with the NED reference of this working tree.
# You can set these variables from the command line.
SIMU5G_DIR    := $(abspath ../..)
ROOT          ?= $(abspath $(SIMU5G_DIR)/..)
DOC_BUILDER_DIR ?= $(ROOT)/doc-builder
INET_DIR      ?= $(ROOT)/inet
BUILDDIR      = _build

INET_VERSION  := $(shell awk '/repository: inet-framework\/inet/ { inet = 1 } inet && /ref:/ { print $$2; exit }' \
                   $(SIMU5G_DIR)/.github/workflows/fingerprint-tests.yml)
INET_DOC_URL  := https://inet.omnetpp.org/docs/$(shell echo "$(INET_VERSION:v%=%)" | cut -d. -f1,2)/api/types

DOC_BUILDER   = cd $(ROOT) && $(DOC_BUILDER_DIR)/doc-builder.sh \
                --project-dir $(SIMU5G_DIR) --sphinx-source $(SIMU5G_DIR)/doc/src --ned-dir neddoc \
                --title "Simu5G NED Documentation" --ned-path $(abspath $(INET_DIR))/src \
                --external-docs inet=$(INET_DOC_URL) --docgen-args --no-automatic-hyperlinks --output-dir $(abspath $(BUILDDIR))/html

.PHONY: all html pdf serve clean checkdirs

all: html

html: checkdirs
	@$(DOC_BUILDER)

pdf: checkdirs
	@$(DOC_BUILDER) --pdf

serve:
	python3 -m http.server -d $(BUILDDIR)/html 8000

clean:
	@rm -rf $(BUILDDIR) neddoc

# doc-builder.sh only sees the directory it runs in (ROOT), so Simu5G and INET must be in it.
checkdirs:
	@test -x $(DOC_BUILDER_DIR)/doc-builder.sh || { echo "No checkout of omnetpp/doc-builder in $(DOC_BUILDER_DIR); set DOC_BUILDER_DIR"; exit 1; }
	@test -d $(INET_DIR)/src || { echo "No INET checkout in $(INET_DIR); set INET_DIR"; exit 1; }
	@case "$(abspath $(INET_DIR))/" in $(ROOT)/*) ;; *) echo "$(INET_DIR) is not in $(ROOT); set ROOT to a directory that contains Simu5G and INET"; exit 1 ;; esac
