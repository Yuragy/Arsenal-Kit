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


## Kits and Supported Cobalt Strike Versions

| Kit                 | Cobalt Strike Version |
| ------------------- | --------------------- |
| Artifact            | 4.x                   |
| Sleepmask           | 4.7 and later         |
| UDRL                | 4.4 and later         |
| UDRL-VS             | 4.4 and later         |
| Mimikatz (20220919) | 4.5 and later         |
| Postex              | 4.10 and later        |
| Resource            | 4.x                   |
| Process Inject      | 4.5 and later         |
| Mutator             | 4.7 and later         |

**Not included:**

* Elevate Kit
* applet
* powerapplet

## Description of Kit Files and Directories

| Location                              | Description                                  |
| ------------------------------------- | -------------------------------------------- |
| `arsenal_kit.config`                  | Arsenal Kit configuration                    |
| `build_arsenal_kit.sh`                | Arsenal Kit build script                     |
| `templates/`                          | Global Aggressor script templates            |
| `templates/arsenal_kit.cna.template`  | Base Arsenal Kit script template             |
| `templates/helper_functions.template` | Helper functions template                    |
| `dist/`                               | Default output directory for build artifacts |
| `kits/`                               | Source code for individual kits              |
| `kits/<KIT>/script_template.cna`      | Base template for a specific kit             |



# Arsenal Kit Release Notes

## Latest Updates

### Artifact Kit
- Updated the artifact `stage_size` to support 4.10 beacons.

### Postex Kit
- Added a new kit for creating custom long-running Postex tasks.

### Sleep Mask Kit
- Added a new Sleep Mask source code example for the 4.10 release.  
- Introduced a new system call method `beacon` to retrieve syscall information from Beacon.  
- Updated directory structure for Sleep Mask support:  
  - `sleepmask/src` — supports current release (4.10)  
  - `sleepmask/src49` — supports 4.9.x only  
  - `sleepmask/src47` — supports 4.7.x and 4.8.x only  

### User Defined Reflective Loader for Visual Studio (UDRL-VS) Kit
- Updated examples to work with the 4.10 release.  
- Enhanced `bud-loader` and `obfuscation-loader` to populate and pass the `ALLOCATED_MEMORY` structure to Beacon.  
- `bud-loader` now provides allocated memory for `PURPOSE_BOF_MEMORY` and `PURPOSE_SLEEPMASK_MEMORY`.  
- Added support in `bud-loader` for `CreateFile`, `ReadFile`, and `WriteFile` syscalls.

---

## Previous Releases

### January 25, 2024
**Mutator Kit**  
- Added new kit for generating mutated sleep masks.

### October 10, 2023
- Fixed issue with valid Sleep Mask version list in `arsenal_kit.config`.  
- **UDRL-VS Kit**: added x86 loader comments, fixed loader start address calculations, and corrected entry-point logic for second calls.

### September 19, 2023
**Artifact Kit**  
- Updated `stage_size` to support 4.9 beacons.  
- Revised Reflective Loader Size (`rdll_size`) info per changes in the 4.9 hook.  
- Fixed MinGW compiler issues.  

**BOF-VS Kit**  
- Migrated to https://github.com/Cobalt-Strike/bof-vs.  

**Sleep Mask Kit**  
- Added a second CFG bypass attempt on invalid-parameter `NTSTATUS`.  
- Introduced Sleep Mask v4.9 to leverage BeaconInformation BOF API (in `kits/sleepmask/src49`).  
- Removed support for Sleep Mask v4.6 and earlier.  

**UDRL-VS Kit**  
- Added example UDRL for post-execution DLLs.  
- Included Beacon User Data (BUD) example.  
- Fixed DEBUG build issue in `FindBufferBaseAddressStephenFewer`.

### September 11, 2023
**UDRL-VS Kit**  
- Converted to a UDRL library with updated default-loader and added obfuscation-loader example.  
- See blog post: https://cobaltstrike.com/blog/revisiting-the-udrl-part-2-obfuscation-masking

### August 15 & 9, 2023
**BOF-VS Kit**  
- Fixed string argument packing bug.  
- Added Visual Studio project template for BOF development.

### March 15 & 07, 2023
**UDRL-VS Kit**  
- Introduced kit to simplify UDRL development, per blog post: https://cobaltstrike.com/blog/revisiting-the-udrl-part-1-simplifying-development  

**Artifact & Sleep Mask Kits**  
- Added syscall support, evasive sleep with stack spoofing, CFG bypass, helper utilities, updated build scripts, increased size limits, and more.

### December 05, 2022
**Mimikatz Kit**  
- Upgraded to version 2.2.0-20220919.  
- Added `mimikatz-max` DLL variant (see `kits/mimikatz/README.md`).

### August 17, 2022
**Sleep Mask Kit**  
- Continued support for v4.5–4.6 (`kits/sleepmask/src`).  
- Updated for BOF execution on v4.7+ (`kits/sleepmask/src47`), increased size limit, improved templates, and added example features (MASK_TEXT_SECTION, EVASIVE_SLEEP, LOG_SLEEPMASK_PARMS).

**UDRL Kit**  
- Enhanced script template with `log_pe`, error checks, and size output.

### June 13, 2022
**Artifact Kit**  
- Added thread stack spoofing support (see `kits/artifact/README_STACK_SPOOF.md`).

### April 27 & 18, 2022
**Arsenal Kit**  
- Whitespace support in file paths.  
- Introduced initial combined kit (`Artifact`, `Mimikatz`, `Process Inject`, `Resource`, `Sleep Mask`, `UDRL`), config file, and build script.  
- Refactored individual kit build scripts and Aggressor logging.

---

> **Disclaimer:**  
> This software is provided as-is, without any express or implied warranty. Use it at your own risk. We are not responsible for any damage or misuse. Cobalt Strike is a commercial product of Fortra, LLC; this kit is a community extension and is not endorsed by or affiliated with Fortra, LLC.

