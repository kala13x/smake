#!/bin/bash

PROJ_PATH=$(dirname $(readlink -f "$0"))
cd $PROJ_PATH/xutils && ./clean.sh
cd $PROJ_PATH && make clean
