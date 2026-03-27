#!/bin/sh

podman run -it --rm -p 8080:80 \
  -v $(pwd):/usr/share/nginx/html/spec \
  -e SPEC_URL=spec/api.yaml \
  docker.io/redocly/redoc
