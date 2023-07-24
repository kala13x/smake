#!/bin/bash
./clean.sh

PROJ_PATH=$(dirname $(readlink -f "$0"))
CPU_COUNT=1

if [ $OSTYPE == linux-gnu ]; then
    CPU_COUNT=$(nproc)
fi

cp $PROJ_PATH/misc/smake.xutils $PROJ_PATH/xutils/smake.json
cp $PROJ_PATH/misc/make.xutils $PROJ_PATH/xutils/Makefile
cd $PROJ_PATH/xutils && ./build.sh make && make install

cd $PROJ_PATH
make -j $CPU_COUNT

for arg in "$@"; do
    if [[ $arg == "--install" ]]; then
        sudo make install
    fi

    if [[ $arg == "--cleanup" ]]; then
        ./clean.sh
    fi
done