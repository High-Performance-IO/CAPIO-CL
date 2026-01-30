![CAPIO-CL Logo](media/capiocl.png)

# CAPIO-CL — Cross-Application Programmable I/O Coordination Language

[![CI](https://github.com/High-Performance-IO/CAPIO-CL/actions/workflows/ci-test.yml/badge.svg)](https://github.com/High-Performance-IO/CAPIO-CL/actions/workflows/ci-test.yml)
[![Python Bindings](https://github.com/High-Performance-IO/CAPIO-CL/actions/workflows/python-bindings.yml/badge.svg)](https://github.com/High-Performance-IO/CAPIO-CL/actions/workflows/python-bindings.yml)
[![codecov](https://codecov.io/gh/High-Performance-IO/CAPIO-CL/graph/badge.svg)](https://codecov.io/gh/High-Performance-IO/CAPIO-CL)

![CMake](https://img.shields.io/badge/CMake-%E2%89%A53.15-blue?logo=cmake&logoColor=white)
![C++](https://img.shields.io/badge/C%2B%2B-%E2%89%A517-blueviolet?logo=c%2B%2B&logoColor=white)
![Python Bindings](https://img.shields.io/badge/Python_Bindings-3.10–3.14-darkgreen?style=flat&logo=python&logoColor=white&labelColor=gray)

#### Platform support

| OS / Arch                                                                          | ![x86_64](https://img.shields.io/badge/x86__64-121212?logo=intel&logoColor=blue) | ![ARM](https://img.shields.io/badge/ARM-121212?logo=arm&logoColor=0091BD) | ![RISC-V](https://img.shields.io/badge/RISC--V-121212?logo=riscv&logoColor=F9A825) |
|------------------------------------------------------------------------------------|----------------------------------------------------------------------------------|---------------------------------------------------------------------------|------------------------------------------------------------------------------------|
| ![Ubuntu](https://img.shields.io/badge/Ubuntu-121212?logo=ubuntu&logoColor=E95420) | YES                                                                              | YES                                                                       | YES                                                                                |
| ![macOS](https://img.shields.io/badge/macOS-121212?logo=apple&logoColor=white)     | YES                                                                              | YES                                                                       | N.A.                                                                               |

#### Documentation

- [![Core Language](https://img.shields.io/badge/Core%20Language-10.1007%2Fs10766--025--00789--0-%23cc5500?logo=doi&logoColor=white&labelColor=2b2b2b)](https://doi.org/10.1007/s10766-025-00789-0)
- [![Metadata Streaming](https://img.shields.io/badge/Metadata%20Streaming-10.1145%2F3731599.3767577-%23cc5500?logo=doi&logoColor=white&labelColor=2b2b2b)](https://doi.org/10.1145/3731599.3767577)
- [![Doxygen documentation](https://img.shields.io/github/v/release/High-Performance-IO/CAPIO-CL?label=Doxygen%20documentation&labelColor=2b2b2b&color=brown&logo=readthedocs&logoColor=white)](https://github.com/High-Performance-IO/CAPIO-CL/releases/latest/download/documentation.pdf)

**CAPIO-CL** is a novel I/O coordination language that enables users to annotate file-based workflow data dependencies
with **synchronization semantics** for files and directories.
Designed to facilitate **transparent overlap between computation and I/O operations**, CAPIO-CL allows multiple
producer–consumer application modules to coordinate efficiently using a **JSON-based syntax**.

For detailed documentation and examples, please visit:

[![CAPIO Website](https://img.shields.io/badge/CAPIO%20Website-Documentation-brightgreen?logo=readthedocs&logoColor=white)](https://capio.hpc4ai.it/docs/coord-language/)



---

## Overview

The **CAPIO Coordination Language (CAPIO-CL)** allows applications to declare:

- **Data objects**, **I/O dependencies**, and **access modes**
- **Synchronization semantics** across different processes
- **Commit policies** for I/O objects

At runtime, CAPIO-CL’s parser and engine components analyze, track, and manage these declared relationships, enabling *
*transparent data sharing** and **cross-application optimizations**.


---

## Building

### Requirements & dependencies

- C++17 or greater
- Cmake 3.15 or newer
- Python3 to bundle CAPIO-CL json schemas into target binaries
- [danielaparker/jsoncons](https://github.com/danielaparker/jsoncons) to parse, serialize and validate CAPIO-CL JSON
  config files
- [GoogleTest](https://github.com/google/googletest) for automated testing
- [pybind11](https://github.com/pybind/pybind11) when building python wheels

jsoncons, GoogleTest and pybind11 are fetched automatically by CMake — no manual setup required.

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

## Integration as a Subproject

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

When included this way, unit tests and python bindings are **not built**, keeping integration clean for external
projects.

---

## Python Bindings

CAPIO-CL now provides native **Python bindings** built using [pybind11](https://github.com/pybind/pybind11).  
These bindings expose the core C++ APIs (`Engine`, `Parser` and `Serializer`), directly
to Python, allowing the CAPIO-CL logic to be used within python projects.

### Building the Bindings

You can build and install the Python bindings directly from the CAPIO-CL source tree using:

```bash
pip install --upgrade pip
pip install -r build-requirements.txt
python -m build
pip install dst/*.whl
```

This will build the Python wheel and install it into your current environment using an ad-hoc build environment, which
is downloaded, installed, and configured in isolation. A faster way to build and install CAPIO-CL is to use **native
system** packages and then run from within the CAPIO-CL root directory:

```bash
pip install .
```

This assumes that all build dependencies not fetched by cmake are available.

---

## API Snapshot

A simplified example of CAPIO-CL usage in C++:

```c++
#include "capiocl.hpp"

int main() {
    capiocl::Engine engine;
    engine.newFile("Hello_World.txt")
    engine.print();
    
    // Dump engine to configuration file
    capiocl::Serializer::dump(engine, "my_workflow", "my_workflow.json")
    return 0;
}
```

The `py_capio_cl` module provides access to CAPIO-CL’s core functionality through a high-level Python interface.

```python
from py_capio_cl import Engine, Serializer

engine = Engine()
engine.newFile("Hello_World.txt")
engine.print()

# Dump engine to configuration file
Serializer.dump(engine, "my_workflow", "my_workflow.json")
```

## CapioCL Web API Documentation

This section describes the REST-style Web API exposed by the CapioCL Web Server.
The server provides HTTP endpoints for configuring and querying the CapioCL engine at runtime.

All endpoints communicate using JSON over HTTP. To enable the webserver, users needs to explicitly start it with:

```cpp
capiocl::engine::Engine engine();

// start engine with default parameters
engine.startApiServer();

// or by specifying the address and port:
engine.startApiServer("127.0.0.1", 5520);
```


or equivalently in python with:

```python
engine = py_capio_cl.Engine()

#start engine with default parameters
engine.startApiServer()

# or by specifying the address and port:
engine.startApiServer("127.0.0.1", 5520)
```

By default, the webserver listens only on local connection at the following address: ```127.0.0.1:5520```. No
authentication
services are currently available, and as such, users should put particular care when allowing connections from external
endpoints.

---

### Content Type

All requests and responses use:

```
Content-Type: application/json
```

### Error Handling

If a request body is invalid or missing required fields, the server responds with:

```json
{
  "status": "error",
  "what": "Invalid request BODY data: <details>"
}
```

HTTP status code: **400**

### Success Responses

For POST endpoints:

```json
{
  "status": "OK"
}
```

For GET endpoints, a JSON object with the requested data is returned.

HTTP status code: **200**

---

### POST /producer

Registers a producer for a given path.

**Request Body**

```json
{
  "path": "string",
  "producer": "string"
}
```

**Example**

```bash
curl -X POST http://localhost:PORT/producer \
     -H "Content-Type: application/json" \
     -d '{"path":"src/file.cpp","producer":"compile"}'
```

---

### GET /producer

Returns all producers associated with a path.

**Request Body**

```json
{
  "path": "string"
}
```

**Response**

```json
{
  "producers": [
    "compile",
    "link"
  ]
}
```

---

### POST /consumer

Registers a consumer for a given path.

**Request Body**

```json
{
  "path": "string",
  "consumer": "string"
}
```

---

### GET /consumer

Returns all consumers associated with a path.

**Response**

```json
{
  "consumers": [
    "test",
    "package"
  ]
}
```

---

### POST /dependency

Adds a file dependency for a path.

**Request Body**

```json
{
  "path": "string",
  "dependency": "relative/or/absolute/path"
}
```

---

### GET /dependency

Returns file dependencies that trigger commits.

**Response**

```json
{
  "dependencies": [
    "file1.cpp",
    "file2.hpp"
  ]
}
```

---

### POST /commit

Sets the commit rule for a path.

**Request Body**

```json
{
  "path": "string",
  "commit": "rule-expression"
}
```

---

### GET /commit

Gets the commit rule for a path.

**Response**

```json
{
  "commit": "rule-expression"
}
```

---

### POST /commit/file-count

Sets the file count required to commit a directory.

**Request Body**

```json
{
  "path": "string",
  "count": 5
}
```

---

### GET /commit/file-count

Returns the directory file count.

**Response**

```json
{
  "count": 5
}
```

---

### POST /commit/close-count

Sets the close count required to commit.

**Request Body**

```json
{
  "path": "string",
  "count": 2
}
```

---

### GET /commit/close-count

Returns the commit close count.

**Response**

```json
{
  "count": 2
}
```

---

### POST /fire

Sets the fire rule for a path.

**Request Body**

```json
{
  "path": "string",
  "fire": "rule-expression"
}
```

---

### GET /fire

Returns the fire rule.

**Response**

```json
{
  "fire": "rule-expression"
}
```

---

### POST /permanent

Marks a path as permanent or temporary.

**Request Body**

```json
{
  "path": "string",
  "permanent": true
}
```

---

### GET /permanent

Returns permanent status.

**Response**

```json
{
  "permanent": true
}
```

---

### POST /exclude

Marks a path as excluded or included.

**Request Body**

```json
{
  "path": "string",
  "exclude": true
}
```

---

### GET /exclude

Returns exclusion status.

**Response**

```json
{
  "exclude": false
}
```

---

### POST /directory

Marks a path as a directory or file.

**Request Body**

```json
{
  "path": "string",
  "directory": true
}
```

---

### GET /directory

Returns directory status.

**Response**

```json
{
  "directory": true
}
```

---

### POST /workflow

Sets the workflow name.

**Request Body**

```json
{
  "name": "build-and-test"
}
```

---

### GET /workflow

Returns the workflow name.

**Response**

```json
{
  "name": "build-and-test"
}
```

---

## Notes

- All GET endpoints expect a JSON body containing the targeted file path.
- The API is intended for local control and orchestration, not public exposure.

---

## Developing team

| Name                         | Role                          | Contact                                                                                                      |
|------------------------------|-------------------------------|--------------------------------------------------------------------------------------------------------------|
| **Marco Edoardo Santimaria** | Designer and Maintainer       | [email](mailto:marcoedoardo.santimaria@unito.it)  \| [Homepage](https://alpha.di.unito.it/marco-santimaria/) |
| **Iacopo Colonnelli**        | Workflows Expert and Designer | [email](mailto:iacopo.colonnelli@unito.it)    \| [Homepage](https://alpha.di.unito.it/iacopo-colonnelli/)    |
| **Massimo Torquati**         | Designer                      | [email](mailto:massimo.torquati@unipi.it) \| [Homepage](http://calvados.di.unipi.it/paragroup/torquati/)     |
| **Marco Aldinucci**          | Designer                      | [email](mailto:marco.aldinucci@unito.it)  \| [Homepage](https://alpha.di.unito.it/marco-aldinucci/)          |

### Former Members

| Name                            | Role     | Contact                                                                                                          |
|---------------------------------|----------|------------------------------------------------------------------------------------------------------------------|
| **Alberto Riccardo Martinelli** | Designer | [email](mailto:albertoriccardo.martinelli@unito.it) \| [Homepage](https://alpha.di.unito.it/alberto-martinelli/) |

