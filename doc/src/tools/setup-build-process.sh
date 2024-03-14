#!/bin/sh

export DEBIAN_FRONTEND=noninteractive
apt-get update -yy
apt-get upgrade -yy
apt-get install -yy git pipenv
git config --global --add safe.directory /github/workspace
pip3 install sphinx-immaterial doxysphinx
