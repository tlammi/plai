#!/bin/bash

set -eu

THISDIR="$(dirname ${BASH_SOURCE[0]})"
dockerfile="$THISDIR/Dockerfile.${1}"

podman build --arch=amd64 -t "plai-$1" -f "$dockerfile" "$THISDIR"
