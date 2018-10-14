#!/bin/bash

export DEFINE_MK=1
export BASE_DIR=`pwd -P`
export OUT_DIR=$BASE_DIR/out
export LIBS_DIR=$OUT_DIR/lib
export CONF_DIR=$OUT_DIR/conf
export OBJS_DIR=$OUT_DIR/objs
#export MAKE="make --no-print-directory"
export BUILD_TARGET=

function croot() {
    if [ -n "$BASE_DIR" ];then
        cd $BASE_DIR
    else
        echo "Couldn't locate the top of the tree.  Try setting TOP."
    fi
}
