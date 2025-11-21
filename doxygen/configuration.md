# Runtime Configuration for **CAPIO-CL**

CAPIO-CL supports **runtime configuration** through a user-provided **TOML** configuration file.  
This allows you to customize various aspects of the library without recompiling it.

A configuration file can be loaded into a running CAPIO-CL instance by calling:

    capiocl::Engine engine;
    engine.load("path/to/config.toml");

If no configuration file is provided, CAPIO-CL falls back to its built-in defaults.

---

## TOML Configuration Structure

The configuration file uses a structured namespace under the top-level table `monitor.mcast`.

Available configuration parameters:

| Key                              | Type     | Default        | Description                                                                                  |
|----------------------------------|----------|----------------|----------------------------------------------------------------------------------------------|
| `monitor.mcast.commit.ip`        | string   | `224.224.224.1` | Multicast IP address used for commit messages                                                |
| `monitor.mcast.commit.port`      | integer  | `12345`         | UDP port for commit messages                                                                  |
| `monitor.mcast.delay_ms`         | integer  | `300`           | Artificial delay (in milliseconds) inserted before sending multicast messages. Useful for debugging or simulating slower networks. |
| `monitor.mcast.homenode.ip`      | string   | `224.224.224.2` | IP address of the home node for monitoring operations                                         |
| `monitor.mcast.homenode.port`    | integer  | `12345`         | Port associated with the home node monitoring endpoint                                        |

---

## Example configuration file

Below is a complete example of a `config.toml` file:

    # Example CAPIO-CL TOML configuration

    [monitor.mcast]

    # Multicast settings for commit messages
    commit.ip   = "224.224.224.1"
    commit.port = 12345

    # Delay (in milliseconds)
    delay_ms = 300

    # Home node information
    homenode.ip   = "224.224.224.2"
    homenode.port = 12345

---

## How CAPIO-CL Uses These Settings

### `commit.ip` and `commit.port`
These fields define where CAPIO-CL sends **commit broadcast messages**.  
Commit messages are used for consistency coordination across distributed nodes.

### `delay_ms`
A small configurable delay may help with:

- debugging message-ordering issues,
- testing high-latency environments,
- reproducing specific timing-related behavior.

A value of `0` means no delay.

### `homenode.ip` and `homenode.port`
These define the **central monitoring endpoint** (the “home node”).  
CAPIO-CL uses this endpoint to coordinate monitoring metadata and cluster-wide communication.
