# Cobalt Strike Arsenal Kit

Build script for the Cobalt Strike Arsenal Kit

## Description

The Arsenal Kit is the combination of individual kits into a single kit.
Building this kit produces a single Aggressor script, `dist/arsenal_kit.cna`, which can be loaded instead of loading each kit separately. This kit is controlled via the `arsenal_kit.config` file, which specifies which kits are built by the `build_arsenal_kit.sh` script.

Individual kits can still be used independently. Each kit has its own Aggressor script at `dist/<kit_name>/<kit_name>.cna`, which can be loaded on its own. See each kit’s README for details.

The `build_arsenal_kit.sh` script builds all kits enabled in `arsenal_kit.config` by running each enabled kit’s `kits/<kit_name>/build.sh` script in turn. To build a single kit, change into its directory (`kits/<kit_name>`) and run `build.sh`. Each kit’s build script, when run without arguments, displays help text detailing the required parameters.

Please note that the `udrl-vs`, `mutator`, and `postex` kits are standalone and must be built separately. They are not included in `arsenal_kit.config` or in the `build_arsenal_kit.sh` script.

## Requirements

This kit is designed to run on a Linux platform. It has been tested on Debian-based distributions.

Required software:

* Minimal GNU for Windows Cross-Compiler – `apt-get install mingw-w64`

## Quick Start

1. Review and edit `arsenal_kit.config`, setting your desired kits to `true`.
2. Run the build: `build_arsenal_kit.sh`
3. Load the script into Cobalt Strike: **Script Manager → Load `dist/arsenal_kit.cna`**

Commands that use the hooks will display their output in the script console.
