#!/bin/sh

THISDIR=$(dirname $(realpath $0))
podman run --rm -v $THISDIR:/spec docker.io/redocly/cli build-docs api.yaml --output api.html
