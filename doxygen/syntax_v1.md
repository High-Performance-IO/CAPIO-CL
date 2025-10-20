# Syntax of the CAPIO-CL V1 language

In this section we will illustrate how the semantics of the CAPIO-CL configuration language can be expressed in a json
configuration file.

## JSON syntax

A valid CAPIO-CL configuration file comprises five different sections. Sections marked with `*` are mandatory.

- `Workflow name(*)`: identifies the application workflow composed by multiple application modules;
- `Alias`: groups under a convenient name a set of files or directories;
- `IO_Graph(*)`: describes file data dependencies among application modules;
- `Permanent`: defines which files must be kept on the permanent storage at the end of the workflow execution;
- `Exclude`: identifies the files and directories not handled by the CAPIO-CL implementation;
- `Home Node Policies`: defines different file mapping policies to establish which CAPIO-CL servers store which files;

### Workflow name

This section is identified by the keyword `name`. The name is used as an identifier for the current application
workflow. This is useful as it is possible to distinguish different application workflows running on the same machine.

#### Example

```json
{
  "name": "my_workflow"
}
```  

### Alias

The aliases section, identified by the keyword aliases is useful to reduce the verbosity of the enumeration of files an
application can consume or produce. It is a vector of objects composed of the following items:

- `group_name`: the alias identifier
- `files`: an array of strings representing file names.

#### Example

The following snippet is a valid alias section for CAPIO-CL configuration language

```json
{
  "name": "my_workflow",
  "aliases": [
    {
      "group_name": "group-even",
      "files": [
        "file0.dat",
        "file2.dat",
        "file4.dat"
      ]
    },
    {
      "group_name": "group-odd",
      "files": [
        "file1.dat",
        "file3.dat",
        "file5.dat"
      ]
    }
  ]
}
```  

### IO Graph

This section defines the dependencies between input and output streams between application modules comprising the
workflow. It is identified by the keyword `IO_Graph`, and it requires an array of objects. Those objects specify the
input and output streams for each application component. Each object defines the following items:

- `name`: The name of the application. This keyword is mandatory
- `input_stream`: This keyword identifies the input files and directories the application module is expected to read.
  Its value is a vector of strings. This keyword is optional
- `output_stream`: This keyword identifies a vector of files and directory names, i.e., the files and directories the
  application module is expected to produce. Its value is a vector of strings. This keyword is optional.
- `streaming`: This optional keyword identifies the files and directories with their streaming semantics, i.e., the
  “commit and firing rules” (please see [Semantics](semantics.md) for a more detailed description). Its value is an
  array of objects.

#### Streaming section

Each object inside the streaming section may define the following attributes.
For **file** entries (either files or globs):

- `name`: The filenames to which the rule applies. The value of this keyword is an array of filenames.
- `committed`: This keyword defines the “commit rule” associated with the files identified with the keywords name. Its
  value can be either:
    - `on_close:N`: to express the CoC semantic, where N is an integer greater than 1, or can be omitted if N=1;
    - `on_termination`: to express the CoT semantic (default);
    - `on_file:filename`  to express the CoF semantic. `filename` is the file on which the commit rule will commit
      against.
- `mode`: his keyword defines the “firing rule” associated with the files and directories identified with the keys name
  and dirname, respectively. Its value can be either:
    - `update`: for the FoC semantic (default behavior);
    - `no_update`: for the FnU semantic;

For **directories** entries (either files or globs), that is that the rule will apply to files and subdirectories
inside the specified directory entry:

- `dirname`: The directory names to which the rule applies. The value of this keyword is an array of directory names;

- `committed`: This keyword defines the “commit rule” associated with the directories identified with the keywords
  dirname. Its value can be either:
    - `on_n_files`: to express the Commit on N Files commit rule. If this commit rule is chosen, then the keyword
      `n_files`
      must be set to `N` (integer greater than 1). The directory will be considered committed as soon as `N` files are
      committed inside it.;
    - `on_termination`: to express the CoT semantic (default);
    - `on_file`: to express the CoF semantic. In this case, the keyword `files_deps`, whose value is an array of
      filenames or directory names, defines the set of dependencies.

- `mode`: his keyword defines the “firing rule” associated with the files and directories identified with the keys name
  and dirname, respectively. Its value can be either:

    - `update`: for the FoC semantic (default behavior);
    - `no_update`: for the FnU semantic;

#### Example

The following snippet is a valid example of the `IO_Graph` section:

```json
{
  "name": "my_workflow",
  "IO_Graph": [
    {
      "name": "writer",
      "output_stream": [
        "file0.dat",
        "file1.dat",
        "file2.dat",
        "dir"
      ],
      "streaming": [
        {
          "name": [
            "file0.dat"
          ],
          "committed": "on_termination",
          "mode": "update"
        },
        {
          "name": [
            "file1.dat"
          ],
          "committed": "on_close",
          "mode": "update"
        },
        {
          "name": [
            "file2.dat"
          ],
          "committed": "on_close:10",
          "mode": "no_update"
        },
        {
          "dirname": [
            "dir"
          ],
          "committed": "n_files:1000",
          "mode": "no_update"
        }
      ]
    },
    {
      "name": "reader",
      "input_stream": [
        "file0.dat",
        "file1.dat",
        "file2.dat",
        "dir"
      ]
    }
  ]
}
```

### Exclude

This section, identified by the keyword `exclude`, is used to specify those files that will not be handled by CAPIO-CL
even if they will be created inside the CAPIO-CL_DIR directory. Its value is an array of file names (whose values can
also be aliases).

#### Example

The following snippet is a valid section for the `exclude` section:

```json
{
  "name": "my_workflow",
  "exclude": [
    "file1.dat",
    "group0",
    "*.tmp",
    "*~"
  ]
}
```

### Permanent

This language section, identified by the keyword `permanent`, is used to specify those files that will be stored on the
filesystem at the end of the workflow execution. Its value is an array of file names (whose values can be also aliases).

#### Example

The following snippet is a valid example of how the `permanent` section should be specified:

```json
{
  "name": "my_workflow",
  "permanent": [
    "output.dat",
    "group0"
  ]
}
```

At the end of the execution of “my_workflow”, the file output.dat and all files belonging to group0 will be stored in
the file system.

### Home Node Policy

This section allows the CAPIO-CL user to selectively choose the CAPIO-CL server node where all files (or a subset of
them) and their associated metadata should be stored. The user can define different policies for different files. In the
current version of the CAPIO-CL language, the home node policy options are:

- `create` (default option)
- `manual`
- `hashing`

These three keywords are optional, i.e., the user can avoid setting them explicitly in the CAPIO-CL configuration file.
In this latter case, the default policy adopted by the CAPIO-CL middleware for the file-to-node mapping is `create`.


> **There cannot be filename overlap among the policies specified (i.e., the intersection of the set defined by the
different home-node policies must be empty).**


For the `hashing` and the `create` policies, the value is an array of files.

For the `manual` configuration, the syntax is more verbose and requires defining, for each file or set of files, the
logical identifier of the application process whose associated CAPIO-CL server (i.e., the server process running on the
same node where the application process is running) will store the file. With this information, each CAPIO-CL server may
statically know the file-to-node mapping and thus retrieve the node where this process is executing at runtime.

#### Wildcards ambiguities

The CAPIO-CL language syntax leverages wildcards to provide the user with flexibility and reduce the burden of
enumerating all files and directories.

Using wildcards in the language syntax can introduce unexpected behavior due to unintended matches or undefined behavior
due to multiple matches associated with different semantics rules. To clarify the point, let us consider the following
example:

```json
{
  "name": "my_workflow",
  "IO_Graph": [
    {
      "name": "writer",
      "output_stream": [
        "file1.txt",
        "file2.txt",
        "file1.dat",
        "file2.dat"
      ],
      "streaming": [
        {
          "name": [
            "file*"
          ],
          "committed": "on_close"
        },
        {
          "name": [
            "*.dat"
          ],
          "committed": "on_termination"
        }
      ]
    }
  ]
}
```

In the example, there is an overlapping match for the files `file1.dat` and `file2.dat`. For those files, it is
ambiguous if the commit semantics should be `on_close` or `on_termination`.

Even if, in most cases, the ambiguity can be eliminated considering the most specific match for the rules that must be
used (for example, `*.dat` is more specific than `file*` if we consider the context, i.e., `output_stream` list of files
specified by the user), in the current version of CAPIO-CL, all ambiguities are not solved, and an undefined behavior
exception is raised by the CAPIO-CL implementation.

It is worth mentioning that, other than carefully using wildcards, proper use of the `aliases` section can help the user
write a non-ambiguous and clean configuration file. An example is shown below:

```json
{
  "name": "my_workflow",
  "aliases": [
    {
      "group_name": "group-dat",
      "files": [
        "file1.dat",
        "file2.dat"
      ]
    },
    {
      "group_name": "group-txt",
      "files": [
        "file1.txt",
        "file2.txt"
      ]
    }
  ],
  "IO_Graph": [
    {
      "name": "writer",
      "output_stream": [
        "group-dat",
        "group-txt"
      ],
      "streaming": [
        {
          "name": [
            "group-txt"
          ],
          "committed": "on_close"
        },
        {
          "name": [
            "group-dat"
          ],
          "committed": "on_termination"
        }
      ]
    }
  ]
}
```