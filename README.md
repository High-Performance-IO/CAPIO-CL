![CAPIO-CL Logo](media/capiocl.png)

# CAPIO-CL — Cross-Application Programmable I/O Coordination Language

[![CI](https://github.com/High-Performance-IO/CAPIO-CL/actions/workflows/ci-test.yml/badge.svg)](https://github.com/High-Performance-IO/CAPIO-CL/actions/workflows/ci-test.yml)
[![Python Bindings](https://github.com/High-Performance-IO/CAPIO-CL/actions/workflows/python-bindings.yml/badge.svg)](https://github.com/High-Performance-IO/CAPIO-CL/actions/workflows/python-bindings.yml)
[![codecov](https://codecov.io/gh/High-Performance-IO/CAPIO-CL/graph/badge.svg)](https://codecov.io/gh/High-Performance-IO/CAPIO-CL)

![CMake](https://img.shields.io/badge/CMake-%E2%89%A53.15-blue?logo=cmake&logoColor=white) 
![C++](https://img.shields.io/badge/C%2B%2B-%E2%89%A517-blueviolet?logo=c%2B%2B&logoColor=white)
![Python Bindings](https://img.shields.io/badge/Python_Bindings-3.10–3.14-darkgreen?style=flat&logo=python&logoColor=white&labelColor=gray)

![Ubuntu](https://img.shields.io/badge/Ubuntu-121212?logo=ubuntu&logoColor=E95420)
![macOS](https://img.shields.io/badge/macOS-121212?logo=apple&logoColor=white)

[![DOI](https://img.shields.io/badge/DOI-10.1007%2Fs10766--025--00789--0-%23cc5500?logo=doi&logoColor=white&labelColor=2b2b2b)](https://doi.org/10.1007/s10766-025-00789-0)

**CAPIO-CL** is a novel I/O coordination language that enables users to annotate file-based workflow data dependencies 
with **synchronization semantics** for files and directories.
Designed to facilitate **transparent overlap between computation and I/O operations**, CAPIO-CL allows multiple 
producer–consumer application modules to coordinate efficiently using a **JSON-based syntax**.

For detailed documentation and examples, please visit:  
👉 [https://capio.hpc4ai.it/docs/coord-language/](https://capio.hpc4ai.it/docs/coord-language/)

---

## 📘 Overview

The **CAPIO Coordination Language (CAPIO-CL)** allows applications to declare:
- **Data objects**, **I/O dependencies**, and **access modes**
- **Synchronization semantics** across different processes
- **Commitment and lifetime policies** for I/O objects

At runtime, CAPIO-CL’s parser and engine components analyze, track, and manage these declared relationships, enabling **transparent data sharing** and **cross-application optimizations**.


---

## ⚙️ Building

### Requirements & dependencies
- C++17 or greater
- Cmake 3.15 or newer
- [nlohmann/json](https://github.com/nlohmann/json) to parse JSON config files
- [GoogleTest](https://github.com/google/googletest) for automated testing

All dependencies are fetched automatically by CMake — no manual setup required.

### Steps
```bash
Clone
git clone https://github.com/High-Performance-IO/CAPIO-CL.git


mkdir -p CAPIO-CL/build && cd CAPIO-CL/build
cmake ..
make 
```

By default, this will:
- Build the **"libcapio_cl"** static library
- Build the **"CAPIO_CL_tests"** executable (GoogleTest-based)
- Build the **"py_capio_cl"** python bindings (pybind11)

---

## 📦 Integration as a Subproject

**CAPIO-CL** can be included directly into another CMake project using:

```cmake
include(FetchContent)

#####################################
# External projects
#####################################
FetchContent_Declare(
        capio_cl
        GIT_REPOSITORY https://github.com/High-Performance-IO/CAPIO-CL.git
        GIT_TAG main  
)
FetchContent_MakeAvailable(capio_cl)

#####################################
# Include files and directories
#####################################
target_include_directories(${TARGET_NAME} PRIVATE 
        ${capio_cl_SOURCE_DIR}
)

#####################################
# Link libraries
#####################################
target_link_libraries(${PROJECT_NAME} PRIVATE 
        libcapio_cl
)
```

When included this way, tests and python bindings are **not built**, keeping integration clean for external projects.

---

## 🐍 Python Bindings

CAPIO-CL now provides native **Python bindings** built using [pybind11](https://github.com/pybind/pybind11).  
These bindings expose the core C++ APIs—such as `Engine`, `Parser`, directly 
to Python, allowing the CAPIO-CL logic to be used within python projects.

### 🔧 Building the Bindings
You can build and install the Python bindings directly from the CAPIO-CL source tree using:

```bash
pip install --upgrade pip
pip install -r build-requirements.txt
python -m build
pip install dst/*.whl
```
Now you will be able to directly import the package **py_capio_cl** in your project!

---

## 🧩 API Snapshot

A simplified example of CAPIO-CL usage in C++:

```c++
#include "capiocl.hpp"

int main() {
    capiocl::Engine engine;
    engine.newFile("Hello_World.txt")
    engine.print();
    return 0;
}
```


The `py_capio_cl` module provides access to CAPIO-CL’s core functionality through a high-level Python interface.
```python
from py_capio_cl import Engine

engine = Engine()
engine.newFile("Hello_World.txt")
engine.print()
```
