#!/bin/bash

THISDIR="$(dirname ${BASH_SOURCE[0]})"
dockerfile="$THISDIR/Dockerfile.buildenv"

podman build --arch=amd64 -t plai-build -f "$dockerfile" "$THISDIR"
