INCLUDES ?= -I ../include

PLUGINDIR ?= ../build/src

all: build scan-plugins

scan-plugins:
	@$(if $(wildcard ${PLUGINDIR}),,>&2 echo "Scan dir not found: ${PLUGINDIR}" && exit 1)
	@find ${PLUGINDIR} -type f -name '*.so' -exec ./frei0r-info {} \; > tmp.json
	@echo "[" > frei0r-plugin-list.json
	@head -n -1 tmp.json >> frei0r-plugin-list.json
	@echo "}\n]" >> frei0r-plugin-list.json
	@rm tmp.json
	$(info frei0r-plugin-list.json)

build:
	@${CC} -o frei0r-info -ggdb frei0r-info.c ${INCLUDES}

clean:
	rm -f *.o
	rm -f frei0r-info
	rm -f *.json
