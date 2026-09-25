![CAPIO-CL Logo](https://raw.githubusercontent.com/High-Performance-IO/CAPIO-CL/main/media/capiocl.png)

# CAPIO-CL — Cross-Application Programmable I/O Coordination Language

[![CI](https://github.com/High-Performance-IO/CAPIO-CL/actions/workflows/ci-test.yml/badge.svg)](https://github.com/High-Performance-IO/CAPIO-CL/actions/workflows/ci-test.yml)
[![Python Bindings](https://github.com/High-Performance-IO/CAPIO-CL/actions/workflows/python-bindings.yml/badge.svg)](https://github.com/High-Performance-IO/CAPIO-CL/actions/workflows/python-bindings.yml)
[![RISC-V Unit tests](https://github.com/High-Performance-IO/CAPIO-CL/actions/workflows/riscv.yml/badge.svg)](https://github.com/High-Performance-IO/CAPIO-CL/actions/workflows/riscv.yml)
[![codecov](https://codecov.io/gh/High-Performance-IO/CAPIO-CL/graph/badge.svg)](https://codecov.io/gh/High-Performance-IO/CAPIO-CL)

![CMake](https://img.shields.io/badge/CMake-%E2%89%A53.15-blue?logo=cmake&logoColor=white)
![C++](https://img.shields.io/badge/C%2B%2B-%E2%89%A517-blueviolet?logo=c%2B%2B&logoColor=white)
![Python Bindings](https://img.shields.io/badge/Python_Bindings-3.10–3.14-darkgreen?style=flat&logo=python&logoColor=white&labelColor=gray)

#### Platform support

| OS / Arch                                                                          | ![x86_64](https://img.shields.io/badge/x86__64-121212?logo=intel&logoColor=blue) | ![ARM](https://img.shields.io/badge/ARM-121212?logo=arm&logoColor=0091BD) | ![RISC-V](https://img.shields.io/badge/RISC--V-121212?logo=riscv&logoColor=F9A825) |
|------------------------------------------------------------------------------------|----------------------------------------------------------------------------------|---------------------------------------------------------------------------|------------------------------------------------------------------------------------|
| ![Ubuntu](https://img.shields.io/badge/Ubuntu-121212?logo=ubuntu&logoColor=E95420) | YES                                                                              | YES                                                                       | YES                                                                                |
| ![macOS](https://img.shields.io/badge/macOS-121212?logo=apple&logoColor=white)     | No CI/CD support                                                                              | YES                                                                       | N.A.                                                                               |

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
- [CALF](https://github.com/High-Performance-IO/CALF) for both logs and CLI messages

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

### Install from PyPI
CAPIO-CL is available on PyPI! Simply run
```bash
pip install py_capio_cl
```

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

## Runtime TOML Configuration

Runtime behavior is configured with a TOML file loaded into `CapioClConfiguration`. This is separate from the JSON
coordination-language document: the TOML file selects the JSON document, monitor backends, metadata storage, and dynamic
API settings.

### Complete example

```toml
# Workflow and optional JSON coordination-language document
[capiocl]
workflow_name = "my-workflow"
config_path = "workflow.json"
resolve_path = "/data/run-42"
store_all_in_memory = false

[capiocl.dynamic_api]
enabled = false
ip = "224.224.224.3"
port = 11223

[capiocl.monitor.filesystem]
enabled = true
metadata_dir = "/shared/trusted/run-42"

[capiocl.monitor.mcast]
enabled = false
delay_ms = 300

[capiocl.monitor.mcast.commit]
ip = "224.224.224.1"
port = 12345

[capiocl.monitor.mcast.homenode]
ip = "224.224.224.2"
port = 12345
```

### Options

| Key | Type | Default | Description |
|-----|------|---------|-------------|
| `capiocl.workflow_name` | string | JSON `name`, or `CAPIO` without JSON | Overrides the workflow name. |
| `capiocl.config_path` | string/path | empty | JSON CAPIO-CL document to parse. An empty value creates a runtime-only engine. |
| `capiocl.resolve_path` | string/path | empty | Prefix applied to relative paths in the JSON document. |
| `capiocl.store_all_in_memory` | boolean | `false` | Marks every parsed data path for in-memory storage. |
| `capiocl.dynamic_api.enabled` | boolean | `false` | Starts the dynamic configuration API. |
| `capiocl.dynamic_api.ip` | string | `224.224.224.3` | Multicast address used by the dynamic API. |
| `capiocl.dynamic_api.port` | integer | `11223` | UDP port used by the dynamic API. |
| `capiocl.monitor.filesystem.enabled` | boolean | see below | Enables filesystem commit and home-node tokens. |
| `capiocl.monitor.filesystem.metadata_dir` | string/path | empty | Trusted metadata root for persistent counted `ON_CLOSE` state. |
| `capiocl.monitor.mcast.enabled` | boolean | see below | Enables multicast commit and home-node propagation. |
| `capiocl.monitor.mcast.delay_ms` | integer | `300` | Delay before multicast status operations, in milliseconds. |
| `capiocl.monitor.mcast.commit.ip` | string | `224.224.224.1` | Multicast group for commit state. |
| `capiocl.monitor.mcast.commit.port` | integer | `12345` | UDP port for commit state. |
| `capiocl.monitor.mcast.homenode.ip` | string | `224.224.224.2` | Multicast group for home-node state. |
| `capiocl.monitor.mcast.homenode.port` | integer | `12345` | UDP port for home-node state. |

Every CAPIO-CL option is under the top-level `capiocl` table. Other top-level tables may coexist in the same TOML file
and are ignored by CAPIO-CL. When parsing a user-provided configuration, omitted
`capiocl.monitor.filesystem.enabled` and `capiocl.monitor.mcast.enabled` values are `false`. `Engine()` and
`CapioClConfiguration.loadDefaults()` use the built-in configuration, which enables both monitors. Set both values
explicitly in deployed TOML files to avoid ambiguity.

TOML booleans must be unquoted `true` or `false`, and ports/delays must be integers. Relative `capiocl.config_path`,
`capiocl.resolve_path`, and `capiocl.monitor.filesystem.metadata_dir` values are interpreted from the process working
directory. Unknown keys are retained but ignored by CAPIO-CL.

### Filesystem metadata

`capiocl.monitor.filesystem.metadata_dir` is required only when an `ON_CLOSE` rule commits after more than one close. It
must name a trusted, non-attacker-writable directory unique to the workflow run. CAPIO-CL creates and owns a `capiocl`
subdirectory beneath it. Multi-process and multi-node producers must share that directory through storage providing
coherent atomic exclusive file creation, rename, and unlink. Ordinary commit and home-node token operations do not
require this option.

### Loading configuration

```cpp
#include "capiocl/configuration.h"
#include "capiocl/parser.h"

using capiocl::configuration::CapioClConfiguration;

CapioClConfiguration config;
config.load("runtime.toml");
std::unique_ptr<capiocl::engine::Engine> engine(capiocl::parser::Parser::parse(config));
```

```python
import py_capio_cl

config = py_capio_cl.CapioClConfiguration()
config.load("runtime.toml")
engine = py_capio_cl.Parser.parse(config)
```

Configuration can also be constructed from a string map/dictionary. Values in that form must use their flattened keys
and string representations, for example `{"capiocl.monitor.filesystem.enabled": "true"}`.

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
