INCLUDES ?= -I ../include

PLUGINDIR ?= ../build/src

CXX ?= g++
DEBUG_FLAGS ?= -O0 -g -ggdb -Wl,-undefined -Wl,dynamic_lookup -fsanitize=address -fsanitize-recover=address -fno-omit-frame-pointer -fsanitize-address-use-after-scope

all: frei0r-meta frei0r-asan
	@echo "Test targets available:"
	@echo "scan-meta      :: scan all plugins and produce metadata"
	@echo "check          :: run tests on all plugins"
	@echo "frei0r-meta    :: build metadata plugin scanner"
	@echo "frei0r-asan    :: build ASAN test utility"
	@echo "frei0r-gui     :: build ASAN test utility"


# build scan-plugins

frei0r-gui: frei0r-run.c test-pattern.c test-pattern.h
	$(info Build frei0r plugin test run utility)
	$(CXX) -O3 -ffast-math -I../include -o frei0r-run frei0r-run.c test-pattern.c -ldl -lm -lX11 -DGUI

frei0r-asan: frei0r-run.c
	$(info Build frei0r plugin test run utility)
	$(CXX) $(DEBUG_FLAGS) -I../include -o frei0r-run frei0r-run.c test-pattern.c -ldl

check: frei0r-run
	@$(if $(wildcard ${PLUGINDIR}),,>&2 echo "Scan dir not found: ${PLUGINDIR}" && exit 1)
	@find $(PLUGINDIR) -type f -name '*.so' | while read f; do \
	./frei0r-run -d -p $$f; if [ $$? -ne 0 ]; then break; fi; done


#	@find ${PLUGINDIR} -type f -name '*.so' -exec ./frei0r-run -d -p {} \;

#	@$(if $(wildcard frei0r-test),,make test-utils)

frei0r-meta: frei0r-meta.c
	$(info Build frei0r meta-data parsing utility)
	${CC} -o frei0r-meta -ggdb frei0r-meta.c ${INCLUDES}

scan-meta: frei0r-meta
	@$(if $(wildcard ${PLUGINDIR}),,>&2 echo "Scan dir not found: ${PLUGINDIR}" && exit 1)
	@find ${PLUGINDIR} -type f -name '*.so' -exec ./frei0r-meta {} \; > tmp.json
	@echo "[" > frei0r-plugin-list.json
	@head -n -1 tmp.json >> frei0r-plugin-list.json
	@echo "}\n]" >> frei0r-plugin-list.json
	@rm tmp.json
	$(info frei0r-plugin-list.json)



generate-metadata: EXTENSION ?= so
generate-metadata:
	@$(if $(wildcard ${PLUGINDIR}),,>&2 echo "Scan dir not found: ${PLUGINDIR}" && exit 1)
	sh extract-plugin-info.sh ${EXTENSION} ${PLUGINDIR}

clean:
	rm -f *.o
	rm -f frei0r-run frei0r-meta
	rm -f *.json
