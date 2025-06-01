#!/bin/bash

set -eu

PROJECT_ROOT="$(realpath $(dirname ${BASH_SOURCE[0]})/..)"

podman run -e XDG_RUNTIME_DIR=/run/xdg \
  -e WAYLAND_DISPLAY \
  -v $XDG_RUNTIME_DIR/$WAYLAND_DISPLAY:/run/xdg/$WAYLAND_DISPLAY \
  --rm -ti \
  -v $PROJECT_ROOT:/mnt \
  -v /dev/dri:/dev/dri \
  --userns=keep-id \
  --group-add=keep-groups \
  --workdir=/mnt \
  --privileged \
  "plai-$1"
