#!/bin/bash

function create_shared_library {
  TEMP=`mktemp --suffix=.c`
  TEMPO=build/template.o

  gcc $TEMP -c -shared -fPIC -o $TEMPO
  objcopy --add-section .brig=$1 --set-section-flags .brig=alloc $TEMPO
  gcc -shared -fPIC -Wl,-Tsrc/hsa-script.x $TEMPO -o $2
  echo "$1 library has been created"
  rm $TEMP
}

TEMP=`mktemp --suffix=.c`
BUILD=build

gcc -c $TEMP -fdump-tree-hsagen -o /dev/null 2>/dev/null
if [ $? -ne 0 ]; then
    echo "GCC compiler is probably not configured with HSA support enabled"
    rm $TEMP
    exit 1
fi

mkdir -p $BUILD

# 1) HSA math library 
BRIG=src/math64.brig
HSAIL=build/libhsamath.hsail

# all f32 functions have to end with 'f'
cat src/math32.hsail | sed 's/function\([^(]*\)(/function\1f(/' > $HSAIL

cat src/math64.hsail >> $HSAIL
sed 's/^decl function /decl prog function /g' -i $HSAIL
sed 's/^function /prog function /g' -i $HSAIL
sed 's/_gcc_//g' -i $HSAIL
sed '1s/^/module \&libhsamath:1:0:$full:$large:$default;\n/' -i $HSAIL

# HSAIL contains some strange prefixed
sed 's/gcn_//g' -i $HSAIL
sed 's/relaxednarrow//g' -i $HSAIL
sed 's/divrelaxed/div/g' -i $HSAIL
HSAILasm -assemble $HSAIL

create_shared_library build/libhsamath.brig build/libhsamath.so

# 2) HSA standard library
INPUT=src/libhsastd.c
OBJ=build/libhsastd.o
BRIG=build/libhsastd.brig
HSAIL=build/libhsastd.hsail

gcc -fopenmp -c $INPUT -O3 -o $OBJ
objcopy -O binary -j .brig $OBJ $BRIG
create_shared_library $BRIG build/libhsastd.so

rm build/*.o build/*.brig
