# 🧠 CAPIO-CL — Cross-Application Programmable I/O Coordination Language

[![DOI](https://img.shields.io/badge/DOI-10.1007%2Fs10766--025--00789--0-blue?logo=doi&logoColor=white)](https://doi.org/10.1007/s10766-025-00789-0)



![CMake](https://img.shields.io/badge/CMake-%E2%89%A53.15-blue?logo=cmake&logoColor=white) 
![C++](https://img.shields.io/badge/C%2B%2B-%E2%89%A517-blueviolet?logo=c%2B%2B&logoColor=white)

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

## 🧩 Project Structure
```yaml
CAPIO-CL/
├── capiocl.hpp # Main public API header
├── src/ 
│ ├── Engine.cpp # Coordination engine logic
│ └── Parser.cpp # Language parser logic
├── tests/ # Unit tests (GoogleTest-based)
│ └── main.cpp
├── CMakeLists.txt
└── README.md
```

---

## ⚙️ Building

### Requirements & dependencies
- C++17 or greater
- Cmake 3.15 or newer
- [simdjson](https://github.com/simdjson/simdjson) to parse JSON config files
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

---

## 📦 Integration as a Subproject

**CAPIO-CL** can be included directly into another CMake project using:

```cmake
include(FetchContent)

FetchContent_Declare(
        capio_cl
        GIT_REPOSITORY https://github.com/High-Performance-IO/CAPIO-CL.git
        GIT_TAG main  
)

FetchContent_MakeAvailable(capio_cl)

target_link_libraries(${PROJECT_NAME} PRIVATE libcapio_cl)
```

When included this way, tests are **not built**, keeping integration clean for external projects.

---

## 🧩 API Snapshot

A simplified example of CAPIO-CL usage in C++:

```c++
#include "capiocl.hpp"

int main() {
    capiocl::Engine engine;
    std::string path = "input.txt";
    std::vector<std::string> producers, consumers, others;
    engine.add(path, producers, consumers, capiocl::COMMITTED_ON_TERMINATION, capiocl::MODE_UPDATE, false, false, others);
    engine.print();
    return 0;
}
```