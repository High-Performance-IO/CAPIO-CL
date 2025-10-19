# Building Doxygen Documentation

To build the **CAPIO-CL** documentation, you need **Doxygen** installed. **LaTeX** is optional but required if you wish to generate the PDF version.

## Steps
- Navigate to the `doxygen` directory.
- To build the HTML documentation, run:
   ```bash
   make
   ```
-  To build the PDF documentation, run:
   ```bash
   make pdf
   ```
   This will produce a pdf file named    `documentation_x.y.z.pdf` which will contain the documentation in a PDF version.