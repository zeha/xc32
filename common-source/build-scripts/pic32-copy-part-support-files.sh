#!/bin/sh

# Copy part support files from a generated tree, to pic32-libs
usage()
{
    echo "Usage:"
    echo "$0 <path to generated files directory>"
    echo "Requires, copies generated files to ./pic32-libs tree."
}

# Check for path to generated files
if [ -z "$1" ]; then
    echo "Please provide a path to generated files to copy."
    usage
    exit 1
fi

# Need help ?
if [ ! -z "$2" ] || [ "$1" == "help" ]; then
    usage
    exit 1
fi

GENERATED_FILES=$1
if [ ! -d "$GENERATED_FILES" ]; then
    echo "'$GENERATED_FILES' is not a directory."
    usage
    exit 1
fi

# Is there a pic32-libs directory ?
if [ ! -d ./pic32-libs ] || [ ! -d ./pic32-libs/proc ] || [ ! -d ./pic32-libs/include ] || [ ! -d ./pic32-libs/include/proc ] || [ ! -d ./pic32-libs/include/lega-c ]; then
    echo "It doesn't look like there's a complete pic32-libs directory here."
    usage
    exit 1
fi

# Is there a c30_resource directory ?
if [ ! -d ./c30_resource ] || [ ! -d ./c30_resource/src ] || [ ! -d ./c30_resource/src/xc32 ]; then
    echo "It doesn't look like there's a complete c30_resource directory here."
    usage
    exit 1
fi

# Is there an XC32-docs directory ?
if [ ! -d ./docs ] || [ ! -d ./docs/config_docs ]; then
    echo "It doesn't look like there's a complete XC32-docs directory here."
    usage
    exit 1
fi

# Copy the xc.h file to pic32-libs/include
if [ ! -e $GENERATED_FILES/xc.h ]; then
    echo "No 'xc.h' file found in $GENERATED_FILES."
    exit 1
fi
echo "Copying $GENERATED_FILES/xc.h to ./pic32-libs/include."
cp $GENERATED_FILES/xc.h ./pic32-libs/include

# Copy the xc-pic32m.h file to pic32-libs/include
if [ ! -e $GENERATED_FILES/xc-pic32m.h ]; then
    echo "No 'xc-pic32m.h' file found in $GENERATED_FILES."
    exit 1
fi
echo "Copying $GENERATED_FILES/xc-pic32m.h to ./pic32-libs/include."
cp $GENERATED_FILES/xc-pic32m.h ./pic32-libs/include

# Copy the resource file to c30_resource/src/xc32
if [ ! -e $GENERATED_FILES/xc32_DVSdevices.res ]; then
    echo "No 'xc32_DVSdevices.res' file found in $GENERATED_FILES."
    exit 1
fi
echo "Copying $GENERATED_FILES/xc32_DVSdevices.res to ./c30_resource/src/xc32."
cp $GENERATED_FILES/xc32_DVSdevices.res ./c30_resource/src/xc32

# Copy the config bit index file, and its pages, to XC32-docs
if [ ! -e $GENERATED_FILES/PIC32ConfigSet.html ]; then
    echo "No 'PIC32ConfigSet.html' file found in $GENERATED_FILES."
    exit 1
fi

echo "Copying $GENERATED_FILES/PIC32ConfigSet.html to ./XC32-docs."
tidy -im -wrap 110 $GENERATED_FILES/PIC32ConfigSet.html
cp $GENERATED_FILES/PIC32ConfigSet.html ./XC32-docs

if [ ! -d $GENERATED_FILES/config_docs ]; then
    echo "No 'config_docs' directory found in $GENERATED_FILES."
    exit 1
fi
echo "Copying $GENERATED_FILES/config_docs to ./XC32-docs."
cp $GENERATED_FILES/config_docs/* ./docs/config_docs
find ./docs -name "*.htm*" -exec tidy -im -wrap 110 \{\} \;

# Loop over all devices
DEVICES=`ls $GENERATED_FILES | grep -v 'config_docs' | grep -v '\.h' | grep -v '\.html' | grep -v '\.res'`
for DEVICE in $DEVICES
do
    # Check that $DEVICE is a directory
    if [ ! -d "$GENERATED_FILES/$DEVICE" ]; then
	echo "$DEVICE is not a directory."
	exit 1
    fi
    echo "Device: $DEVICE"

    # Header file goes to include/proc
    HEADER_FILE=`ls $GENERATED_FILES/$DEVICE | grep '\.h'`
    echo "Copying $GENERATED_FILES/$DEVICE/$HEADER_FILE to ./pic32-libs/include/proc."
    cp $GENERATED_FILES/$DEVICE/$HEADER_FILE ./pic32-libs/include/proc

    # Everything else goes to proc/$DEVICE
    if [ ! -e "./pic32-libs/proc/$DEVICE" ]; then
	mkdir ./pic32-libs/proc/$DEVICE
    fi
    if [ ! -d "./pic32-libs/proc/$DEVICE" ]; then
	echo "Couldn't make, or $DEVICE is not a, directory."
	exit 1
    fi
    ALLELSE=`ls $GENERATED_FILES/$DEVICE | grep -v '\.h'`
    for OTHERFILE in $ALLELSE
    do
	echo "Copying $GENERATED_FILES/$DEVICE/$OTHERFILE to ./pic32-libs/proc/$DEVICE."
        cp $GENERATED_FILES/$DEVICE/$OTHERFILE ./pic32-libs/proc/$DEVICE
    done

done
