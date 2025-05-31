#!/bin/bash

set -e

if [ $# -eq 0 ]; then
  echo "usage: [podman flags...] envname"
fi

THISDIR="$(dirname ${BASH_SOURCE[0]})"
dockerfile="$THISDIR/Dockerfile.${1}"

opts=(${@:1:$(($# - 1))})
container="${@: -1}"

podman build ${opts} -t "plai-$container" -f "$THISDIR/Dockerfile.$container" "$THISDIR"
