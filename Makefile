FILTERS := nervous glow
PWD := $(shell pwd)
.PHONY: gallery

list-plugins = 	find ${PWD}/build/src/${1} -type f -name '*.so' |	while read -r plugin; do echo -n "$$(basename $${plugin} | cut -d. -f1) "; done

list-filters:
	@$(call list-plugins,filter)
list-generators:
	@$(call list-plugins,generator)
list-mixer2:
	@$(call list-plugins,mixer2)
list-mixer3:
	@$(call list-plugins,mixer3)


gallery: tmp := $(shell mktemp -u)
gallery:
	@for f in `$(call list-plugins,filter)`; do \
		[ "$$f" = "timeout" ] && continue ;\
		[ "$$f" = "curves" ] && continue ;\
		[ -r "gallery/$${f}.mp4" ] && continue ;\
		dir="`echo $${f} | cut -d_ -f1`" ;\
		[ -r "${PWD}/build/src/filter/$${dir}/$${f}.so" ] || { >&2 echo "Plugin not found: $$dir/$$f"; exit 1; } ;\
		echo; echo "RENDER: $$f"; echo ;\
		FREI0R_PATH="${PWD}/build/src/filter/$${dir}" \
		ffmpeg -y -i gallery/the_end.mp4 -vf "frei0r=$${f}" -vsync vfr "${tmp}.mp4"       ;\
		ffmpeg -y -i gallery/the_end.mp4 -i "${tmp}.mp4" -filter_complex vstack "gallery/$${f}.mp4" ;\
		rm -f "${tmp}" ;\
	done
