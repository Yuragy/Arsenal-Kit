#!/usr/bin/env bash
# Default sizes
STAGER_SIZE=1024
MIN_STAGE_5K_SIZE=344564
MIN_STAGE_100K_SIZE=486388
STAGE_SIZE=${MIN_STAGE_5K_SIZE}
COMPILE_STACK_SPOOF=0
USE_SYSCALLS=0

# Optional resource file
resource_rc="src-main/resource.rc"
resource_o="resource.o"


# make our output look nice...
kit_name="Artifact kit"

function print_good () {
   echo -e "[${kit_name}] \x1B[01;32m[+]\x1B[0m $1"
}

function print_error () {
   echo -e "[${kit_name}] \x1B[01;31m[-]\x1B[0m $1"
}

function print_warning () {
   echo -e "[${kit_name}] \x1B[01;33m[-]\x1B[0m $1"
}

function print_info () {
   echo -e "[${kit_name}] \x1B[01;34m[*]\x1B[0m $1"
}

function check_alignment () {
   # This will check the file size and print an error when the
   # size is not a multiple of 4-bytes.
   # Same as the following command:
   #    ls -l dist-pipe | grep -v cna | awk '($5 % 4) != 0 {print $5 "\t" $9 "\t Is not 4-byte aligned."}'
   files=$(ls -l "${1}" | egrep -v "cna|total")
   i=0;
   size=0;
   file="";
   for f in ${files} ; do
      if [ ${i} == 4 ] ; then
         size=${f}
      elif [ ${i} == 8 ] ; then
         file=${f}
         if [ $((${size} % 4)) != 0 ] ; then
            print_warning "[OPSEC] ${f} is not 4-byte aligned. Check the compiler options."
         fi
         i=-1
      fi
      i=$(($i + 1))
   done
}

#
# Compile Artifacts to Deliver an X64 Payload
#
function build_resource32() {
   # Compile the optional resource file
   print_info "Compile ${resource_rc}"

   ${CCx86}-windres ${resource_rc} -o ${resource_o}
}

function build_resource64() {
   # Compile the optional resource file
   print_info "Compile ${resource_rc}"

   ${CCx64}-windres ${resource_rc} -o ${resource_o}
}

function build_artifacts64() {

   #
   # Compile Small Artifacts for Stager Payloads
   #

   # Set up the common compiler options for stager payloads
   c_options="-m64 ${options} -Wall -DUSE_${allocator} -DDATA_SIZE=${STAGER_SIZE} -DUSE_SYSCALLS=${USE_SYSCALLS}"

   # compile our 64-bit DLL
   print_info "Recompile artifact64.x64.dll with ${1}"
   source_files="${common} ${1} src-main/dllmain.c src-main/dllmain.def"
   ${CCx64}-gcc ${c_options} -Isrc-common -shared ${source_files} -s -o temp.dll
   mv temp.dll "${2}/artifact64.x64.dll"

   # compile our 64-bit EXE
   print_info "Recompile artifact64.exe with ${1}"
   source_files="${common} ${1} src-main/main.c ${resource_o}"
   ${CCx64}-gcc ${c_options} -mwindows ${source_files} -s -o "${2}/artifact64.exe"

   # compile our 64-bit Service EXE (auto-migrate to allow cleanup)
   print_info "Recompile artifact64svc.exe with ${1}"
   source_files="${common} ${1} src-main/svcmain.c ${resource_o}"
   ${CCx64}-gcc ${c_options} -mwindows -D_MIGRATE_ ${source_files} -s -o "${2}/artifact64svc.exe"

   #
   # Compile Large Artifacts for Fully-Staged Beacon Payloads
   #

   # Set up the common compiler options for fully-staged beacon payloads
   c_options="-m64 ${options} -Wall -DUSE_${allocator} -DDATA_SIZE=${STAGE_SIZE} -DSTACK_SPOOF=${COMPILE_STACK_SPOOF} -DUSE_SYSCALLS=${USE_SYSCALLS}"

   # compile our 64-bit DLL
   print_info "Recompile artifact64big.x64.dll with ${1}"
   source_files="${common} ${1} src-main/dllmain.c src-main/dllmain.def"
   ${CCx64}-gcc ${c_options} -Isrc-common -shared ${source_files} -s -o temp.dll
   mv temp.dll "${2}/artifact64big.x64.dll"

   # compile our 64-bit EXE
   print_info "Recompile artifact64big.exe with ${1}"
   source_files="${common} ${1} src-main/main.c ${resource_o}"
   ${CCx64}-gcc ${c_options} -mwindows ${source_files} -s -o "${2}/artifact64big.exe"

   # compile our 32-bit Service EXE (auto-migrate to allow cleanup)
   print_info "Recompile artifact64svcbig.exe with ${1}"
   source_files="${common} ${1} src-main/svcmain.c ${resource_o}"
   ${CCx64}-gcc ${c_options} -mwindows -D_MIGRATE_ ${source_files} -s -o "${2}/artifact64svcbig.exe"

   # cleanup
   rm -f *.o
}

#
# Compile Artifacts (x86/x64) to Deliver an x86 Payload
#
function build_artifacts() {

   #
   # Compile Small Artifacts for Stager Payloads
   #

   # Set up the common compiler options for stager payloads
   c_options="${options} -Wall -DUSE_${allocator} -DDATA_SIZE=${STAGER_SIZE} -DUSE_SYSCALLS=${USE_SYSCALLS}"

   # compile our 32-bit DLL
   #   -static-libgcc - Remove the dependency on libgcc_s_dw2-1.dll
   print_info "Recompile artifact32.dll with ${1}"
   source_files="${common} ${1} src-main/dllmain.c src-main/dllmain.def"
   ${CCx86}-gcc ${c_options} -Isrc-common -shared -static-libgcc -Wl,--enable-stdcall-fixup ${source_files} -s -o temp.dll
   mv temp.dll "${2}/artifact32.dll"

   # compile our 32-bit EXE
   print_info "Recompile artifact32.exe with ${1}"
   source_files="${common} ${1} src-main/main.c ${resource_o}"
   ${CCx86}-gcc ${c_options} -mwindows ${source_files} -s -o "${2}/artifact32.exe"

   # compile our 32-bit Service EXE (auto-migrate to allow cleanup)
   print_info "Recompile artifact32svc.exe with ${1}"
   source_files="${common} ${1} src-main/svcmain.c ${resource_o}"
   ${CCx86}-gcc ${c_options} -mwindows -D_MIGRATE_ ${source_files} -s -o "${2}/artifact32svc.exe"

   #
   # Compile Large Artifacts for Fully-Staged Beacon Payloads
   #

   # Set up the common compiler options for fully-staged beacon payloads
   c_options="${options} -Wall -DUSE_${allocator} -DDATA_SIZE=${STAGE_SIZE} -DSTACK_SPOOF=${COMPILE_STACK_SPOOF} -DUSE_SYSCALLS=${USE_SYSCALLS}"

   # compile our 32-bit DLL
   #   -static-libgcc - Remove the dependency on libgcc_s_dw2-1.dll
   print_info "Recompile artifact32big.dll with ${1}"
   source_files="${common} ${1} src-main/dllmain.c src-main/dllmain.def"
   ${CCx86}-gcc ${c_options} -Isrc-common -shared -static-libgcc -Wl,--enable-stdcall-fixup ${source_files} -s -o temp.dll
   mv temp.dll "${2}/artifact32big.dll"

   # compile our 32-bit EXE
   print_info "Recompile artifact32big.exe with ${1}"
   source_files="${common} ${1} src-main/main.c ${resource_o}"
   ${CCx86}-gcc ${c_options} -mwindows ${source_files} -s -o "${2}/artifact32big.exe"

   # compile our 32-bit Service EXE (auto-migrate to allow cleanup)
   print_info "Recompile artifact32svcbig.exe with ${1}"
   source_files="${common} ${1} src-main/svcmain.c ${resource_o}"
   ${CCx86}-gcc ${c_options} -mwindows -D_MIGRATE_ ${source_files} -s -o "${2}/artifact32svcbig.exe"

   # cleanup
   rm -f *.o
}

# compiler flags to pass to all builds.
# -Os         - Optimize for size.
options="-Os"

# our common files... goal is to stay very light
common="src-common/patch.c"

# change up the compiler if you need to
CCx86="i686-w64-mingw32"
CCx64="x86_64-w64-mingw32"

# check for a cross-compiler
if [ $(command -v ${CCx64}-gcc) ]; then
   print_good "You have a x86_64 mingw--I will recompile the artifacts"
else
   print_error "No cross-compiler detected. Try: apt-get install mingw-w64"
   exit 2
fi

#
# build our artifacts with different bypass techniques passed in as parameters
#

if [[ $# -ne 8 ]]; then
    print_error "Missing Parameters:"
    print_error "Usage:"
    print_error './build <techniques> <allocator> <stage size> <rdll size> <include resource file> <stack spoof> <syscalls> <output directory>'
    print_error " - Techniques       - a space separated list"
    print_error " - Allocator        - set how to allocate memory for the reflective loader."
    print_error "                      Valid values [HeapAlloc VirtualAlloc MapViewOfFile]"
    print_error " - Stage Size       - integer used to set the space needed for the beacon stage."
    print_error "                      For a 0K   RDLL stage size should be ${MIN_STAGE_5K_SIZE} or larger"
    print_error "                      For a 5K   RDLL stage size should be ${MIN_STAGE_5K_SIZE} or larger"
    print_error "                      For a 100K RDLL stage size should be ${MIN_STAGE_100K_SIZE} or larger"
    print_error " - RDLL Size        - integer used to specify the RDLL size. Valid values [0, 5, 100]"
    print_error " - Resource File    - true or false to include the resource file"
    print_error " - Stack Spoof      - true or false to use the stack spoofing technique"
    print_error " - Syscalls         - set the system call method"
    print_error "                      Valid values [none embedded indirect indirect_randomized]"
    print_error " - Output Directory - Destination directory to save the output"
    print_error "Example:"
    print_error "  ./build.sh \"peek pipe readfile\" HeapAlloc ${MIN_STAGE_5K_SIZE} 5 true true indirect /tmp/dist/artifact"

    exit 2
fi

# Set variables from parameters
techniques="${1}"
allocator="${2}"
STAGE_SIZE="${3}"
rdll_size="${4}"
include_resource="${5}"
stack_spoof="${6}"
syscalls="${7}"
dist_directory="${8}"

# Check if the allocator is valid.
valid_values="HeapAlloc VirtualAlloc MapViewOfFile"
if [[ ! $valid_values =~ (^|[[:space:]])"${allocator}"($|[[:space:]]) ]] ; then
  print_error "Invalid allocator value: ${allocator}"
  print_error "Valid values are: ${valid_values}"
  exit 2
fi
print_info "Using allocator: ${allocator}"

# Check if the rdll_size is valid.
valid_values="0 5 100"
if [[ ! $valid_values =~ (^|[[:space:]])"${rdll_size}"($|[[:space:]]) ]] ; then
  print_error "Invalid RDLL Size: ${rdll_size}"
  exit 2
fi

# Check if STAGE_SIZE meets minimum size based on the rdll_size
if [[ ${rdll_size} -lt 100 && ${STAGE_SIZE} -ge ${MIN_STAGE_5K_SIZE} ]] ; then
   print_info "Using STAGE size: ${STAGE_SIZE}"
   print_info "Using RDLL size: ${rdll_size}K"
elif [[ ${rdll_size} -eq 100 && ${STAGE_SIZE} -ge ${MIN_STAGE_100K_SIZE} ]] ; then
   print_info "Using STAGE size: ${STAGE_SIZE}"
   print_info "Using RDLL size: 100K"
else
   if [[ ${rdll_size} -lt 100 ]] ; then
     print_error "The STAGE size needs to be ${MIN_STAGE_5K_SIZE} or larger when using a RDLL size of ${rdll_size}K"
   else
     print_error "The STAGE size needs to be ${MIN_STAGE_100K_SIZE} or larger when using a RDLL size of 100K"
   fi
   exit 2
fi
#check if stack spoofing technique should be compiled
if [[ ${stack_spoof} == "true" ]] ; then
  print_info "Using stack spoofing technique"
  COMPILE_STACK_SPOOF=1
fi

# Check if the syscalls is valid.
valid_values="none embedded indirect indirect_randomized"
if [[ ! $valid_values =~ (^|[[:space:]])"${syscalls}"($|[[:space:]]) ]] ; then
  print_error "Invalid system call method: ${syscalls}"
  print_error "Valid values are: ${valid_values}"
  exit 2
fi
# Setup variables for the system call method
if [[ ${syscalls} == "none" ]] ; then
  USE_SYSCALLS=0
else
  USE_SYSCALLS=1
  common="${common} src-common/utils.c src-common/syscalls_${syscalls}.c"

  # -masm=intel - Added to support system calls.
  options="${options} -masm=intel"
fi
print_info "Using system call method: ${syscalls}"

for technique in ${techniques} ; do

   if [ -f src-common/bypass-${technique}.c ] ; then
      mkdir -p "${dist_directory}/${technique}"

      print_good "Artifact Kit: Building artifacts for technique: ${technique}"

      if [ $include_resource = 'true' ]; then
         build_resource32
      else
         resource_o=""
      fi
      build_artifacts "src-common/bypass-${technique}.c" "${dist_directory}/${technique}"
      
      if [ $include_resource = 'true' ]; then
         build_resource64
      else
         resource_o=""
      fi
      build_artifacts64 "src-common/bypass-${technique}.c" "${dist_directory}/${technique}"

      # copy our script over
      sed 's/KITNAME/artifact_kit/' ../../templates/helper_functions.template > "${dist_directory}/${technique}/artifact.cna"
      sed "s/__STAGE_SIZE__/${STAGE_SIZE}/" ./script_template.cna >> "${dist_directory}/${technique}/artifact.cna"

      # Check for file alignment
      check_alignment "${dist_directory}/${technique}"

      print_good "The artifacts for the bypass technique '${technique}' are saved in '${dist_directory}/${technique}'"
   else
      print_error "The ${technique} bypass technique could not be found"
   fi
done

