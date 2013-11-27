#!/bin/sh

ulimit -s unlimited
ulimit -c unlimited
#ulimit -n 65536

export LD_LIBRARY_PATH=../lib:../plugins/:../../lib/elib:../../lib/3rd_party/libevent/lib:$LD_LIBRARY_PATH
#. ./method epoll
