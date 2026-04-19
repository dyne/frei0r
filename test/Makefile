INCLUDES ?= -I ../include

PLUGINDIR ?= ../build/src

CXX ?= g++

DEBUG_FLAGS := -O0 -g -ggdb -Wl,-undefined -Wl,dynamic_lookup -fsanitize=address -fsanitize-recover=address -fno-omit-frame-pointer -fsanitize-address-use-after-scope

CI_OPT_FLAGS := -O3 -ffast-math -flto -march=native -pipe

# CI runs: make && make check (see .github/workflow)
# so the 'all' target is built in CI
all: frei0r-meta frei0r-asan
	@echo "Test targets available:"
	@echo "frei0r-run     :: build headless test utility (default)"
	@echo "frei0r-asan    :: build ASAN test utility"
	@echo "frei0r-gui     :: build graphical test utility"
	@echo "check          :: run the built test on all plugins"
	@echo "frei0r-meta    :: build metadata plugin scanner"
	@echo "scan-meta      :: scan all plugins and produce metadata"

frei0r-run: frei0r-run.c
	@printf 'Build frei0r plugin test run utility\n'
	$(CXX) $(CI_OPT_FLAGS) -I../include -o frei0r-run frei0r-run.c -ldl

frei0r-asan: frei0r-run.c
	@printf 'Build frei0r plugin test run utility\n'
	$(CXX) $(DEBUG_FLAGS) -I../include -o frei0r-run frei0r-run.c -ldl

frei0r-gui: frei0r-run.c test-pattern.c test-pattern.h
	@printf 'Build frei0r plugin test run utility\n'
	$(CXX) $(CI_OPT_FLAGS) -I../include -o frei0r-run frei0r-run.c test-pattern.c -ldl -lm -lX11 -DGUI

check: frei0r-run
	@if [ ! -d "${PLUGINDIR}" ]; then printf 'Scan dir not found: %s\n' "${PLUGINDIR}" >&2; exit 1; fi
	@find "${PLUGINDIR}" -type f -name '*.so' | \
		while IFS= read -r f; do \
		./frei0r-run -d -p "$$f" || break; done

frei0r-meta: frei0r-meta.c
	@printf 'Build frei0r meta-data parsing utility\n'
	${CC} -o frei0r-meta -ggdb frei0r-meta.c ${INCLUDES}

scan-meta: frei0r-meta
	@if [ ! -d "${PLUGINDIR}" ]; then printf 'Scan dir not found: %s\n' "${PLUGINDIR}" >&2; exit 1; fi
	@find ${PLUGINDIR} -type f -name '*.so' -exec ./frei0r-meta {} \; > tmp.json
	@echo "[" > frei0r-plugin-list.json
	@head -n -1 tmp.json >> frei0r-plugin-list.json
	@echo "}\n]" >> frei0r-plugin-list.json
	@rm tmp.json
	@printf 'frei0r-plugin-list.json\n'



generate-metadata: EXTENSION ?= so
generate-metadata:
	@if [ ! -d "${PLUGINDIR}" ]; then printf 'Scan dir not found: %s\n' "${PLUGINDIR}" >&2; exit 1; fi
	sh extract-plugin-info.sh ${EXTENSION} ${PLUGINDIR}

clean:
	rm -f *.o
	rm -f frei0r-run frei0r-meta
	rm -f *.json
