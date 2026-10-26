# ox knowledge base

This folder explains how ox works. The repo READMEs tell you how to build and install things. These pages explain the ideas behind them.

The pages assume you know C++ and have used a graphics API. They do not assume you know OpenXR or the ox codebase.

## Start here

Read these in order:

1. [Project overview](project-overview.md) - what ox is and which repo does what.
2. [OpenXR primer](openxr-primer.md) - the OpenXR words used everywhere else.
3. [Architecture](architecture.md) - the two ways ox can run and how data moves.

## Architecture & internals

| Page | What it covers |
|---|---|
| [Architecture](architecture.md) | Single-process and split-process modes, how a driver is chosen, and how frames and inputs move. |
| [Runtime](runtime.md) | `ox-runtime`: the OpenXR functions, the graphics backends, and how it calls a driver. |
| [Simulator](simulator.md) | `ox-sim-driver`: how it is built, device profiles, and the GUI, REST API, C API, and Python wrapper. |
| [IPC proxy](ipc.md) | `ox-ipc-proxy`: how driver calls cross a process boundary, and what travels through shared memory. |
| [Drivers](drivers.md) | The `ox_driver.h` contract, the folder layout, and how to write your own driver. |

## Features

| Page | What it covers |
|---|---|
| [Testing an application](testing.md) | How to test an OpenXR app with ox, including the unofficial Blender XR suite. |

## Processes

| Page | What it covers |
|---|---|
| [Building](building.md) | How the build fetches the sub-repos, platform dependencies, output files, and `config.yaml`. |
| [Development workflow](development-workflow.md) | Build, run the host, test, and fix common problems. |
| [Release process](release-process.md) | How a release is built, tested, packaged, signed, and uploaded. |

## Companion projects

| Page | What it covers |
|---|---|
| [Companion projects](companion-projects.md) | `vog`, `pyopenxr_examples`, and `ox-driver-example`. |

## Source docs

These are the original guides. The KB links back to them where useful:

- [ox documentation index](../docs/README.md)
- [Automated testing guide](../docs/automated_testing.md)
- [Development guide](../docs/development.md)
- [Custom drivers guide](../docs/drivers.md)
- [FAQ](../docs/faq.md)
- [GUI guide](../docs/gui.md)
- [REST API guide](../docs/rest_api.md)

The README in each repo is still the reference for that repo's build steps. The KB explains how the repos fit together.
