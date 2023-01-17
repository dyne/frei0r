

debug:
	mkdir -p build
	cd build && cmake -DCMAKE_C_COMPILER=gcc -DCMAKE_CXX_COMPILER=g++ -DCMAKE_BUILD_TYPE=Debug -DCMAKE_CXX_FLAGS_DEBUG='-ggdb -fno-omit-frame-pointer -fsanitize=address' -DCMAKE_C_FLAGS_DEBUG='-ggdb -fno-omit-frame-pointer -fsanitize=address' ..
	cd build && make


debug-clang-ninja:
	mkdir -p build
	cd build && cmake -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ -DCMAKE_BUILD_TYPE=Debug -DCMAKE_CXX_FLAGS_DEBUG='-ggdb -fno-omit-frame-pointer -fsanitize=address' -DCMAKE_C_FLAGS_DEBUG='-ggdb -fno-omit-frame-pointer -fsanitize=address' -G 'Ninja' ..
	cd build && ninja
