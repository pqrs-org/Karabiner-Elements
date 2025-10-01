#!/bin/bash

set -u # forbid undefined variables
set -e # forbid command failure

for f in "$@"; do
  basename=$(basename "$f")
  extension="${basename##*.}"
  filename="${basename%.*}"

  if [[ $extension = 'png' ]]; then
    before_size=$(stat -f '%z' "$f")

    # apply twice
    for i in 0 1; do
      # allow command failure
      set +e

      pngquant --skip-if-larger "$f" --ext .png --force

      # forbid command failure
      set -e
    done

    after_size=$(stat -f '%z' "$f")
    if [ $before_size -ne $after_size ]; then
      echo -e "$before_size -> $after_size\t$f"
    fi
  fi
done
