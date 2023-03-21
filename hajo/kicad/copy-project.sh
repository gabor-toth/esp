#!/bin/bash

if [[ $# -ne 2 ]]; then
  echo "Usage: $0 src dst"
  exit 1
fi

src=$1
dst=$2

if [[ -f $dst ]]; then
  echo "$dst already exist, delete it first"
  exit 2
fi

files=$(find $src -name "$src.kicad_*" -type f)
mkdir $dst
for i in $files; do 
  fn=$(basename $i)
  ext=${fn#*.}
  sed -e "s/$src/$dst/g" $i > $dst/${dst}.${ext}
done

