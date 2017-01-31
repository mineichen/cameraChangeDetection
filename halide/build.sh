platform="host-linux-64"
buildDir="build/"$platform

#Compile generator and Build directory
oldDir=$PWD
cd $(dirname "$0")
mkdir -p $buildDir
c++ -g harris.cpp -std=c++11 -lHalide `libpng-config --cflags --ldflags` -O3 -fno-rtti -o build/harris_generator
rm $buildDir/*

# First we define a helper function that checks that a file exists
check_file_exists()
{
    FILE=$1
    if [ ! -f $FILE ]; then
        echo $FILE not found
        exit -1
    fi
}

# And another helper function to check if a symbol exists in an object file
check_symbol()
{
    FILE=$1
    SYM=$2
    if !(nm $FILE | grep $SYM > /dev/null); then
        echo "$SYM not found in $FILE"
    exit -1
    fi
}
check_runtime()
{
    if !(nm $1 | grep "[TSW] _\?halide_" > /dev/null); then
        echo "Halide runtime not found in $1"
	exit -1
    fi
}

check_no_runtime()
{
    if nm $1 | grep "[TSW] _\?halide_" > /dev/null; then
        echo "Halide runtime found in $1"
	exit -1
    fi
}

# Bail out on error
#set -e

#####################
# Cross-compilation #
#####################
./build/harris_generator \
    -g harris_generator \
    -f harris \
    -e o,h \
    -o $buildDir \
    target=${platform}-no_runtime \
    protectOverflow=false 

./build/harris_generator \
    -g harris_generator \
    -f harris_nocache \
    -e o,h \
    -o $buildDir \
    target=${platform}-no_runtime
    

./build/harris_generator \
    -g harris_generator \
    -f harris_uint8 \
    -e o,h \
    -o $buildDir \
    target=${platform}-no_runtime \
    outputShift=7 \
    outputDatetype=uint8

# These files don't contain the runtime
check_no_runtime $buildDir/harris.o
check_symbol     $buildDir/harris.o harris
check_no_runtime $buildDir/harris_nocache.o
check_symbol     $buildDir/harris_nocache.o harris_nocache
check_no_runtime $buildDir/harris_uint8.o
check_symbol     $buildDir/harris_uint8.o harris_uint8

# We can then use the generator to emit just the runtime:
./build/harris_generator \
    -r halide_runtime \
    -e o,h \
    -o $buildDir \
    target=$platform
check_runtime $buildDir/halide_runtime.o

# Linking the standalone runtime with the three generated object files     
# gives us three versions of the pipeline for varying levels of x86,      
# combined with a single runtime that will work on nearly all x86     
# processors.
ar q $buildDir/harris.a \
    $buildDir/harris.o \
    $buildDir/harris_nocache.o \
    $buildDir/harris_uint8.o \
    $buildDir/halide_runtime.o

check_runtime $buildDir/harris.a
check_symbol  $buildDir/harris.a harris
check_symbol  $buildDir/harris.a harris_nocache
check_symbol  $buildDir/harris.a harris_uint8

cd $OLDDIR
echo "Success!"
