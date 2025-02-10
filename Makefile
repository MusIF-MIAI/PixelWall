.PHONY: all pixelwall others wasm docker-wasm clean

all: pixelwall others

pixelwall:
	$(MAKE) -C pixelwall

others:
	$(MAKE) -C others

wasm:
	$(MAKE) -C pixelwall \
		CC=emcc \
		RAYLIB_PATH=../raylib/raylib-5.5-wasm \
		RAYLIB_LIBS="-s USE_GLFW=3 -s ASYNCIFY"
	cp pixelwall/pixelwall others/wasm/pixelwall.js
	cp pixelwall/pixelwall.wasm others/wasm

docker-wasm:
	docker run -v `pwd`:/src emscripten/emsdk make wasm

clean:
	$(MAKE) -C pixelwall clean
	$(MAKE) -C others clean
