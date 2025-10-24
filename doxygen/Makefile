VERSION := $(shell grep -E 'VERSION [0-9]+\.[0-9]+\.[0-9]+' ../CMakeLists.txt | awk '{print $$2}')

docs:
	@grep -v '!\[CAPIO-CL Logo\]' ../README.md > .README.md
	sed "s|@VERSION@|$(VERSION)|" Doxyfile > .Doxyfile
	doxygen .Doxyfile
	@echo "\n\nBuild documentation for version $(VERSION)\n\n"

pdf: docs
	cd generated/latex && $(MAKE)
	cp generated/latex/refman.pdf documentation.pdf

clean:
	rm -rf generated *.pdf .Doxyfile .README.md