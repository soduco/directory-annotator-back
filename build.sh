#!/bin/bash

# ==============================================================================
# We like clean code
set -o errexit
set -o nounset

# ==============================================================================
# Constants
PROGRAM=`basename $0`

# Default parameters
DEBUG_DEF="false"
BUILDTYPE_DEF="Release"

# Real parameters
DEBUG=$DEBUG_DEF
BUILDTYPE=$BUILDTYPE_DEF
EXTRA_ARGS=
# ==============================================================================

# ------------------------------------------------------------------------------
debug_guard() {
  [ "$DEBUG" = "true" ] && "$@" || :
}

# ------------------------------------------------------------------------------
echo_err() {
  echo $@ >&2
}


# ------------------------------------------------------------------------------
usage()
{
cat << EOF
usage: $PROGRAM [-h] [-d] [BUILDTYPE}

Builds SODUCO programs.

POSITIONAL PARAMETERS:
    BUILDTYPE
        Type of build to generate. Must be either "Release" or "Debug".
        [default: $BUILDTYPE_DEF]

OPTIONS:
   -h      Show this message
   -d      Activate debug mode 
           [default: $DEBUG_DEF]

EOF
}

# ------------------------------------------------------------------------------
# Build programs
# $1: debug type (mandatory) - should be either Debug or Release
build()
{
    echo_err "Recreating build dir"
    rm -rf build && mkdir build
    echo_err "conan: install deps"
    conan install . --build missing -s compiler.libcxx=libstdc++11 -s compiler.cppstd=20 --output-folder build
    echo_err "cmake: generate build scripts"
    cmake .. -DCMAKE_BUILD_TYPE=$1
    echo_err "cmake: launch build"
    cmake --build . --config $1
    echo_err "cpack: create artefacts"
    cpack -G ZIP -G TGZ .    
    echo_err "**********************"
    echo_err "BUILD COMPLETE"
    echo_err "**********************"
}

# ------------------------------------------------------------------------------
parseargs()
{
	# Get option(s)
	while getopts “hd” OPTION
	do
		 case $OPTION in
		     h)
		         usage
		         exit 1
		         ;;
		     d)
		         DEBUG="true"
		         ;;
		     ?)
		         usage
		         exit
		         ;;
		 esac
	done

	# Get positional parameter(s)
	shift $((OPTIND-1))
    if [ $# -eq 0 ]
    then
        debug_guard echo_err "No positional parameter was provided."
        echo_err "No build type provided. Defaulting to \"$BUILDTYPE_DEF\""
        BUILDTYPE=$BUILDTYPE_DEF
    elif [ $# -eq 1 ]
    then
        BUILDTYPE=$1
    elif [ $# -gt 1 ]
    then
        EXTRA_ARGS=$@
    else
        # Should never happen
        debug_guard echo_err "BUG in positional argument processing."
    fi	
}

# ------------------------------------------------------------------------------
checkargs()
{
	if [ ! -z "$EXTRA_ARGS" ]
	then
		echo_err "ERROR: extra parameters were detected"
		echo_err "$EXTRA_ARGS"
		echo_err "^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^"
		usage
		exit 1
	fi

	if [ -z "$BUILDTYPE" ]
	then
		BUILDTYPE=$BUILDTYPE_DEF
	elif [ "release" = $(echo $BUILDTYPE | tr '[:upper:]' '[:lower:]') ]
    then
        BUILDTYPE="Release"
	elif [ "debug" = $(echo $BUILDTYPE | tr '[:upper:]' '[:lower:]') ]
    then
        BUILDTYPE="Debug"
    else
        echo_err "Unknown build type. Got \"$BUILDTYPE\" but expecting \"Release\" or \"Debug\"."
		echo_err "^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^"
        usage
        exit 2
    fi
}

# ------------------------------------------------------------------------------
main()
{
	# Parse command line (forward args)
	parseargs $@
    if [ "$DEBUG" = "true" ]
    then
        set -o verbose
        set -o xtrace
        export PS4='+(${BASH_SOURCE}:${LINENO}): ${FUNCNAME[0]:+${FUNCNAME[0]}(): }'
    fi
	
	echo_err "BUILDTYPE: $BUILDTYPE"

	# Check the options
	checkargs

	# Run if everything's allright.
    build "$BUILDTYPE"
}


# ==============================================================================
# Call entry point (forward args)
main $@


