# CAPIO-CL V1.1 Language Specification

This document describes the syntax and semantics of the CAPIO-CL V1.1
language.\
Version 1.1 extends Version 1.0 by introducing two additional fields:
`version` and `configuration`.

------------------------------------------------------------------------

## 1. Version Field

The `version` field introduces an explicit versioning mechanism,
allowing CAPIO-CL files to clearly indicate which syntax standard they
follow.\
This ensures compatibility and avoids ambiguity between V1.0 and V1.1
documents.

**Type:** float\
**Purpose:** Identifies the CAPIO-CL language version\
**Behavior:**\
- If omitted, tools may assume legacy V1.0 behavior.\
- If present (e.g., `1.1`), tools can activate V1.1 features.

### Example (JSON)

```
{
  "version": 1.1,
  ...
}
```

------------------------------------------------------------------------

## 2. Configuration Field

CAPIO-CL V1.1 introduces a new optional field, `configuration`, which
allows users to define runtime options for the `capiocl::Engine` class.

The configuration is stored in a separate **TOML** file.\
For details on valid TOML configuration syntax and supported engine
parameters, refer to **configuration.md**.

**Type:** string (file path)\
**Purpose:** Supplies runtime engine settings\
**Behavior:**\
- File must be a valid TOML document.\
- Engine loads configuration before execution.

### Example (JSON + TOML)

**capio.cl file:**

```
{
  "version": 1.1,
  "configuration": "engine-config.toml",
  ...
}
```

**engine-config.toml:**

``` 
    [monitor.mcast]
    commit.ip   = "224.224.224.1"
    commit.port = 12345
    delay_ms = 300
    homenode.ip   = "224.224.224.2"
    homenode.port = 12345
```

