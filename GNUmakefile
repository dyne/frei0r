all: release-gcc-ninja

debug: debug-gcc

release-gcc:
	mkdir -p build
	cd build && cmake -DCMAKE_C_COMPILER=gcc -DCMAKE_CXX_COMPILER=g++ -DCMAKE_BUILD_TYPE=Release ..
	cd build && make

release-gcc-ninja:
	mkdir -p build
	cd build && cmake -DCMAKE_C_COMPILER=gcc -DCMAKE_CXX_COMPILER=g++ -DCMAKE_BUILD_TYPE=Release -G 'Ninja' ..
	cd build && ninja

release-clang:
	mkdir -p build
	cd build && cmake -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ -DCMAKE_BUILD_TYPE=Release ..
	cd build && make

release-clang-ninja:
	mkdir -p build
	cd build && cmake -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ -DCMAKE_BUILD_TYPE=Release -G 'Ninja' ..
	cd build && ninja

debug-gcc:
	mkdir -p build
	cd build && cmake -DCMAKE_C_COMPILER=gcc -DCMAKE_CXX_COMPILER=g++ -DCMAKE_BUILD_TYPE=Debug -DCMAKE_CXX_FLAGS_DEBUG='-ggdb -fno-omit-frame-pointer -fsanitize=address' -DCMAKE_C_FLAGS_DEBUG='-ggdb -fno-omit-frame-pointer -fsanitize=address' ..
	cd build && make


debug-clang-ninja:
	mkdir -p build
	cd build && cmake -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ -DCMAKE_BUILD_TYPE=Debug -DCMAKE_CXX_FLAGS_DEBUG='-ggdb -fno-omit-frame-pointer -fsanitize=address' -DCMAKE_C_FLAGS_DEBUG='-ggdb -fno-omit-frame-pointer -fsanitize=address' -G 'Ninja' ..
	cd build && ninja

clean:
	rm -rf build
