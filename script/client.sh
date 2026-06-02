#!/bin/bash

: ${PLAI_SOCKET:=/tmp/plai.sock}

ucurl() {
  curl --unix-socket "$PLAI_SOCKET" "${@}"
}

upload() {
  nm="${1}"
  shift
  path="$(realpath "${1}")"
  ucurl -X PUT "http://_/plai/v1/media/items/${nm}" --data-binary "@${path}" >/dev/null
  echo "${typ}/${nm}"
}

activate() {
  nm="${1}"
  ucurl "http://_/plai/v1/playlists/items/${nm}/active" -d "${2}"
}

append() {
  nm="${1}"
  shift
  medias="$(echo ${@} | tr ' ' '\n' | jq -Rn '[inputs]')"
  ucurl "http://_/plai/v1/playlists/items/${nm}/medias" -d "{"
}

play() {
  data="${1}"
  ucurl "http://_/plai/v1/play?replay=true" -d "${data}"
}

cmd="${1}"
shift

"$cmd" "${@}"
