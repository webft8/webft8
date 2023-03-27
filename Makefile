
all: wasm web

debug:
	cd src && DEBUG=1 make web
	cp -R src/dist/web/* ./web/webft8/
	cp -R src/ft8_lib ./web/
	cp -R src/*.h src/*.cpp ./web/
	cd web && python3 server.py

clean:
	cd web && rm -vrf *.cpp *.c *.h *.hpp ./ft8_lib/ ./webft8/*

.PHONY: wasm
wasm:
	cd src && DEBUG=0 make web
	cp -vR src/dist/web/*.wasm src/dist/web/*.js src/dist/web/*.wasm.map ./web/webft8/
	rm -f ./web/webft8/test_data.js

.PHONY: web
web:
	cd web && python3 server.py

submodule_update:
	git submodule update --init --recursive

submodule_update_to_tip:
	git submodule update --recursive --remote