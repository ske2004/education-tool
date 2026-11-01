set -e
mkdir -p ./bin/wasm-out
lib/sokol-tools-bin/bin/linux/sokol-shdc --input catedu/shaders/amalgamation.glsl --output catedu/shaders.hxx --slang glsl300es
emcc -I.\
  -Ilib\
  -Ilib/umka/src\
  -O3\
  --preload-file assets\
  --shell-file scripts/shell.html\
  -sASYNCIFY=1\
  -sSTACK_SIZE=8MB\
  -sFULL_ES3\
  -sTOTAL_MEMORY=64MB\
  -s ALLOW_MEMORY_GROWTH=1\
  -s EXPORTED_FUNCTIONS="['_malloc', '_calloc', '_free', '_main']"\
  -lGL\
  -lc\
  -lm\
  catedu/**/*.cpp\
  catedu/**/**/*.cpp\
  catedu/*.cpp\
  lib/umka/src/*.c\
  -o bin/wasm-out/index.html
