#!/bin/bash

export DEFINE_MK=1
export BASE_DIR=$PWD
export BUILD_DIR=$BASE_DIR/build
export LIBS_DIR=$BUILD_DIR/lib
export CONF_DIR=$BUILD_DIR/conf
export OBJS_DIR=$BUILD_DIR/objs
#export MAKE="make --no-print-directory"
export BUILD_TARGET=

function croot() {
    if [ -n "$BASE_DIR" ];then
        cd $BASE_DIR
    else
        echo "Couldn't locate the top of the tree.  Try setting TOP."
    fi
}
