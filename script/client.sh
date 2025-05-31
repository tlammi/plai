#!/bin/bash

: ${PLAI_SOCKET:=/tmp/plai.sock}

ucurl() {
  curl --unix-socket "$PLAI_SOCKET" "${@}"
}

is_vid() {
  [[ $1 == *.mp4 ]]
}

type() {
  if is_vid "$1"; then
    printf "image"
  else
    printf "image"
  fi
}

upload() {
  nm="${1}"
  shift
  path="$(realpath "${1}")"
  typ="$(type "$path")"
  ucurl -X PUT "http://_/plai/v1/media/${typ}/${nm}" --data-binary "@${path}" >/dev/null
  echo "${typ}/${nm}"
}

play() {
  data="${1}"
  ucurl "http://_/plai/v1/play?replay=true" -d "${data}"
}

cmd="${1}"
shift

"$cmd" "${@}"
