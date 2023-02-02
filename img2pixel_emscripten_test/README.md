# Compiling

```bash
emcc -O3 -s WASM=1 -s EXTRA_EXPORTED_RUNTIME_METHODS='["cwrap"]' -I libwebp img2pixel.c img2p/*.c
```
