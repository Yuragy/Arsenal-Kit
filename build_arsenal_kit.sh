## Include configuration script
. ./arsenal_kit.config

## Init kits string for Aggressor script
kits_string=""

########################
## Functions
function banner () {
cat << "EOF" 
-----------------------------------------------------------------
EOF
}

# make our output look nice...
kit_name="arsenal kit"

function print_good () {
   echo -e "[${kit_name}] \x1B[01;32m[+]\x1B[0m $1"
}

function print_error () {
   echo -e "[${kit_name}] \x1B[01;31m[-]\x1B[0m $1"
}

function print_info () {
   echo -e "[${kit_name}] \x1B[01;34m[*]\x1B[0m $1"
}

########################
## Build Kits

## Make output directory
mkdir -p ./${dist_directory}/

## Artifact kit
function build_artifact_kit () {
    print_good "Building Artifact Kit"
    cd "${artifactkit_directory}"
    "${artifactkit_directory}/build.sh" "${artifactkit_technique}" "${artifactkit_allocator}" "${artifactkit_stage_size}" "${rdll_size}" "${artifactkit_include_resource}" "${artifactkit_stack_spoof}" "${artifactkit_syscalls_method}" "${artifactkit_dist_directory}"
    if [ $? -gt 0 ] ; then
      exit 2
    fi
}

## Sleepmask kit
function build_sleepmask_kit () {
    print_good "Building Sleepmask Kit"
    cd "${sleepmask_directory}"
    "${sleepmask_directory}/build.sh" "${sleepmask_version}" "${sleepmask_sleep_method}" "${sleepmask_mask_text_section}" "${sleepmask_syscalls_method}" "${sleepmask_dist_directory}"
    if [ $? -gt 0 ] ; then
      exit 2
    fi
}

## Process Inject Kit
function build_process_inject_kit () {
    print_good "Building Process Inject Kit"
    cd "${process_inject_directory}"
    "${process_inject_directory}/build.sh" "${process_inject_dist_directory}"
    if [ $? -gt 0 ] ; then
      exit 2
    fi
}

## UDRL kit
function build_udrl_kit () {
    print_good "Building UDRL Kit"
    cd "${udrl_directory}"
    "${udrl_directory}/build.sh" "${rdll_size}" "${udrl_dist_directory}"
    if [ $? -gt 0 ] ; then
      exit 2
    fi
}

## Mimikatz kit
function build_mimikatz_kit () {
    print_good "Building Mimikatz Kit"
    cd "${mimikatz_kit_directory}"
    "${mimikatz_kit_directory}/build.sh" "${mimikatz_kit_dist_directory}"
    if [ $? -gt 0 ] ; then
      exit 2
    fi
}

## Resource kit
function build_resource_kit () {
    print_good "Building Resource Kit"
    cd "${resource_kit_directory}"
    "${resource_kit_directory}/build.sh" "${resource_kit_dist_directory}"
    if [ $? -gt 0 ] ; then
      exit 2
    fi
}

########################
## Start
banner

##### Cobalt Strike Arsenal Kit
cd "${cwd}"

# copy base template
cp ./templates/arsenal_kit.cna.template ./${dist_directory}/arsenal_kit.cna

##### Artifact Kit
if [ ${include_artifact_kit} = "true" ]; then
    if [[ "${artifactkit_technique}" != "${artifactkit_technique%[[:space:]]*}" ]] ; then
      print_error "The artifact bypass technique is invalid: '${artifactkit_technique}'"
      print_info "When building the arsenal kit only provide a single bypass technique for the artifact kit."
      exit 1;
    fi

    build_artifact_kit

    # By default, the build script saves the output to artifact/technique/*. Move this to dist/artifact/
    print_good "Moving the artifacts for the bypass technique '${artifactkit_technique}' to '${artifactkit_dist_directory}'"
    mv "${artifactkit_dist_directory}/${artifactkit_technique}"/* "${artifactkit_dist_directory}/"
    rm -rf "${artifactkit_dist_directory}/${artifactkit_technique}"

    cd "${cwd}"
    print_good "Add the artifact kit hooks to the ${dist_directory}/arsenal_kit.cna file"
    cat "${artifactkit_dist_directory}/artifact.cna" >> ./${dist_directory}/arsenal_kit.cna

    echo ""
fi

##### Sleep Mask Kit
if [ ${include_sleepmask_kit} = "true" ]; then
    build_sleepmask_kit

    cd "${cwd}"
    print_good "Add the sleepmask kit hooks to the ${dist_directory}/arsenal_kit.cna file"
    cat "${sleepmask_directory}/script_template.cna" >> ./${dist_directory}/arsenal_kit.cna

    echo ""
fi

##### Process Inject Kit
if [ ${include_process_inject_kit} = "true" ]; then
    build_process_inject_kit

    cd "${cwd}"
    print_good "Add the process inject kit hooks to the ${dist_directory}/arsenal_kit.cna file"
    cat "${process_inject_directory}/script_template.cna" >> ./${dist_directory}/arsenal_kit.cna

    echo ""
fi

##### UDRL Kit
if [ ${include_udrl_kit} = "true" ]; then
    build_udrl_kit

    cd "${cwd}"
    print_good "Add the udrl kit hooks to the ${dist_directory}/arsenal_kit.cna file"
    sed "s/__RDLL_SIZE__/${rdll_size}/" "${udrl_directory}/script_template.cna" >> ${dist_directory}/arsenal_kit.cna

    echo ""
fi

##### Mimikit
if [ ${include_mimikatz_kit} = true ]; then
    build_mimikatz_kit

    cd "${cwd}"
    print_good "Add the mimikatz kit hooks to the ${dist_directory}/arsenal_kit.cna file"
    cat "${mimikatz_kit_directory}/script_template.cna" >> ./${dist_directory}/arsenal_kit.cna

    echo ""
fi

##### Resource Kit'
if [ ${include_resource_kit} = true ]; then
    build_resource_kit

    cd "${cwd}"
    print_good "Add the resource kit hooks to the ${dist_directory}/arsenal_kit.cna file"
    cat "${resource_kit_directory}/script_template.cna" >> ./${dist_directory}/arsenal_kit.cna

    echo ""
fi

