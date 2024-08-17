build: configure
	cmake --build build

configure:
	cmake -GNinja -B build -S .

clean:
	rm -rf build
