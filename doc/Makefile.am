# Copyright (C) 2007 Richard Spindler <richard.spindler AT gmail.com>
#  
# This file is free software; as a special exception the author gives
# unlimited permission to copy and/or distribute it, with or without 
# modifications, as long as this notice is preserved.
# 
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY, to the extent permitted by law; without even the
# implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

html:
	$(DOXYGEN) Doxyfile

htmldocs_DATA = html

htmldocsdir=${prefix}/share/doc/${PACKAGE}

clean:
	rm -rf html

dist-hook:
	-cp -ra html $(distdir)/doc/

install-data:
	install -c -m 644 -d $(htmldocs_DATA) $(DESTDIR)$(htmldocsdir)

# Modify the install hook, so we can install directories
install-htmldocsDATA:
	mkdir -p $(DESTDIR)$(htmldocsdir)
	cp -r ./html $(DESTDIR)$(htmldocsdir)/

