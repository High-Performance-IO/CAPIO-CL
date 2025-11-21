# CAPIO-CL V1 Language Specification

This document describes the syntax and semantics of the **CAPIO-CL V1 configuration language**, expressed through a JSON-based configuration file.
A valid CAPIO-CL file defines workflow structure, file dependencies, streaming semantics, storage behavior, and home-node mapping policies for distributed execution.

## 1. JSON Syntax Overview

A CAPIO-CL V1 configuration file is a single JSON object composed of the following sections.
Sections marked with `*` are mandatory.

Section | Required | Purpose
------- | -------- | -------
name* | Yes | Identifies the workflow
aliases | No | Provides named groups of files/directories
IO_Graph* | Yes | Describes data dependencies among modules
permanent | No | Files to keep at the end of execution
exclude | No | Files CAPIO-CL should ignore
home_node_policy | No | Rules assigning files to CAPIO-CL servers

## 2. Workflow Name

The workflow name is provided under the key `name`.
This unique identifier allows distinguishing multiple workflows running on the same system.

### Example

```json
{
  "name": "my_workflow"
}
```

## 3. Aliases

Aliases reduce verbosity by allowing groups of files or directories to be referenced by a single identifier.
The `aliases` section is an array of objects, where each object contains:

- `group_name`: alias identifier
- `files`: an array of filename strings

### Example

```json
{
  "name": "my_workflow",
  "aliases": [
    {
      "group_name": "group-even",
      "files": ["file0.dat", "file2.dat", "file4.dat"]
    },
    {
      "group_name": "group-odd",
      "files": ["file1.dat", "file3.dat", "file5.dat"]
    }
  ]
}
```

## 4. IO_Graph

The `IO_Graph` describes how application modules exchange data.
It is an array of objects, each representing one module.

Key | Required | Description
--- | -------- | -----------
name | Yes | Module name
input_stream | No | Files/directories the module reads
output_stream | No | Files/directories the module writes
streaming | No | Commit and firing rules for produced data

### 4.1 Streaming Rules

Each entry in `streaming` may describe rules for **files** or **directories**.

#### For files (`name`):

- `name`: array of filenames
- `committed`: commit rule
    - `"on_close"` or `"on_close:N"`
    - `"on_termination"`
    - `"on_file:filename"`
- `mode`: firing rule
    - `"update"` (default)
    - `"no_update"`

#### For directories (`dirname`):

- `dirname`: array of directory names
- `committed`: directory commit rule
    - `"on_n_files"` (requires field `"n_files": N`)
    - `"on_termination"`
    - `"on_file"` (requires `"files_deps": [...]`)
- `mode`: same semantics as file rules

### Example

```json
{
  "name": "my_workflow",
  "IO_Graph": [
    {
      "name": "writer",
      "output_stream": ["file0.dat", "file1.dat", "file2.dat", "dir"],
      "streaming": [
        {
          "name": ["file0.dat"],
          "committed": "on_termination",
          "mode": "update"
        },
        {
          "name": ["file1.dat"],
          "committed": "on_close",
          "mode": "update"
        },
        {
          "name": ["file2.dat"],
          "committed": "on_close:10",
          "mode": "no_update"
        },
        {
          "dirname": ["dir"],
          "committed": "on_n_files",
          "n_files": 1000,
          "mode": "no_update"
        }
      ]
    },
    {
      "name": "reader",
      "input_stream": ["file0.dat", "file1.dat", "file2.dat", "dir"]
    }
  ]
}
```

## 5. Exclude Section

The `exclude` section identifies files that CAPIO-CL must ignore even if they appear inside `CAPIO-CL_DIR`.
Values may include filenames, wildcard patterns, or alias names.

### Example

```json
{
  "name": "my_workflow",
  "exclude": ["file1.dat", "group0", "*.tmp", "*~"]
}
```

## 6. Permanent Section

The `permanent` section specifies which files should persist on the filesystem after workflow completion.

### Example

```json
{
  "name": "my_workflow",
  "permanent": ["output.dat", "group0"]
}
```

At the end of the workflow, CAPIO-CL will store `output.dat` and all files belonging to alias `group0`.

## 7. Home Node Policy

The `home_node_policy` section defines where files and metadata are stored across CAPIO-CL servers.
The following optional strategies may be defined:

- `create` (default)
- `manual`
- `hashing`

Important constraint: no file may appear in more than one policy group.

### 7.1 Wildcard Ambiguities

Wildcards introduce risk of conflicting rules.

Example:

```json
{
  "name": "my_workflow",
  "IO_Graph": [
    {
      "name": "writer",
      "output_stream": ["file1.txt", "file2.txt", "file1.dat", "file2.dat"],
      "streaming": [
        { "name": ["file*"], "committed": "on_close" },
        { "name": ["*.dat"], "committed": "on_termination" }
      ]
    }
  ]
}
```

Files `file1.dat` and `file2.dat` match both rules, which leads to undefined behavior in the current version of CAPIO-CL.

### 7.2 Avoiding Ambiguity with Aliases

Proper use of aliases can help write a non-ambiguous and clean configuration file.

```json
{
  "name": "my_workflow",
  "aliases": [
    { "group_name": "group-dat", "files": ["file1.dat", "file2.dat"] },
    { "group_name": "group-txt", "files": ["file1.txt", "file2.txt"] }
  ],
  "IO_Graph": [
    {
      "name": "writer",
      "output_stream": ["group-dat", "group-txt"],
      "streaming": [
        { "name": ["group-txt"], "committed": "on_close" },
        { "name": ["group-dat"], "committed": "on_termination" }
      ]
    }
  ]
}
```
