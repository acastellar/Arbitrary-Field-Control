#!/bin/bash
for file in ./*.{vert,comp,frag}
do
  if [ -e "$file" ]; then
    filename=$(basename "$file")
    glslc "$file" -o "./compiled/$filename.spv"
  fi
done
