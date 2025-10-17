VERSION := $(shell grep -E 'VERSION [0-9]+\.[0-9]+\.[0-9]+' ../CMakeLists.txt | awk '{print $$2}')

docs:
	@grep -v '!\[CAPIO-CL Logo\]' ../README.md > README.tmp.md
	sed "s|@VERSION@|$(VERSION)|" Doxygen.in > Doxyfile
	doxygen Doxyfile
	@echo "\n\nBuild documentation for version $(VERSION)\n\n"
