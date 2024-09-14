
for file in ./*.{vert,comp,frag,glsl}
do
  if [ -e "$file" ]; then
    filename=$(basename "$file")
    glslc "$file" -o "./compiled/$filename.spv"
  fi
done
