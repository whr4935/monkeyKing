#!/bin/sh

set -x

SOURCE_DIR=`pwd`/
BUILD_DIR=${BUILD_DIR:-./out}
INSTALL_DIR=${INSTALL_DIR:-${BUILD_DIR}/install}

ln -sf $BUILD_DIR/compile_commands.json

mkdir -p $BUILD_DIR \
  && cd $BUILD_DIR \
  && cmake \
           -DCMAKE_INSTALL_PREFIX=$INSTALL_DIR \
           -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
           $SOURCE_DIR \
  && make $*

# Use the following command to run all the unit tests
# at the dir $BUILD_DIR/$BUILD_TYPE :
# CTEST_OUTPUT_ON_FAILURE=TRUE make test

# cd $SOURCE_DIR && doxygen

