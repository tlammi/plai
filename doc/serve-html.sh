#!/bin/sh

THISDIR=$(dirname $(realpath $0))

podman run -it --rm -p 8080:80 \
  -v $THISDIR:/usr/share/nginx/html/spec \
  -e SPEC_URL=spec/api.yaml \
  docker.io/redocly/redoc
