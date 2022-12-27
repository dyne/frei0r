INCLUDES ?= -I ../include

all: build scan-filters

scan-filters:
	@$(if $(wildcard ../build),,>&2 echo "Test needs a ../build" && exit 1)
	@find ../build/src -type f -name '*.so' -exec ./frei0r-info {} \; > tmp.json && echo "[" > frei0r-plugin-list.json && head -n -1 tmp.json >> frei0r-plugin-list.json && echo "}\n]" >> frei0r-plugin-list.json && rm tmp.json && >&2 echo "frei0r-plugin-list.json"

build:
	@${CC} -o frei0r-info -ggdb frei0r-info.c ${INCLUDES}

clean:
	rm -f *.o
	rm -f frei0r-info
	rm -f *.json
