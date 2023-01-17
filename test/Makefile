INCLUDES ?= -I ../include

PLUGINDIR ?= ../build/src

CXX ?= g++
DEBUG_FLAGS ?= -O0 -g -ggdb -Wl,-undefined -Wl,dynamic_lookup -fsanitize=address -fsanitize-recover=address -fno-omit-frame-pointer -fsanitize-address-use-after-scope

all: test-utils
# build scan-plugins

frei0r-test: CVFLAGS := $(shell pkg-config --libs --cflags opencv4)
frei0r-test: frei0r-test.c
	${CXX} ${DEBUG_FLAGS} -ggdb -O0 -I../include -o frei0r-test frei0r-test.c ${CVFLAGS} -ldl

test-all-filters: frei0r-test
	@$(if $(wildcard ${PLUGINDIR}),,>&2 echo "Scan dir not found: ${PLUGINDIR}" && exit 1)
	@find ${PLUGINDIR} -type f -name '*.so' -exec ./frei0r-test -t -v ${HOME}/Downloads/frei0r-all.webm -p {} \;

#	@$(if $(wildcard frei0r-test),,make test-utils)

scan-plugins:
	@$(if $(wildcard ${PLUGINDIR}),,>&2 echo "Scan dir not found: ${PLUGINDIR}" && exit 1)
	@find ${PLUGINDIR} -type f -name '*.so' -exec ./frei0r-info {} \; > tmp.json
	@echo "[" > frei0r-plugin-list.json
	@head -n -1 tmp.json >> frei0r-plugin-list.json
	@echo "}\n]" >> frei0r-plugin-list.json
	@rm tmp.json
	$(info frei0r-plugin-list.json)



generate-metadata: EXTENSION ?= so
generate-metadata:
	@$(if $(wildcard ${PLUGINDIR}),,>&2 echo "Scan dir not found: ${PLUGINDIR}" && exit 1)
	sh extract-plugin-info.sh ${EXTENSION} ${PLUGINDIR}

build:
	@${CC} -o frei0r-info -ggdb frei0r-info.c ${INCLUDES}

clean:
	rm -f *.o
	rm -f frei0r-info frei0r-test
	rm -f *.json
