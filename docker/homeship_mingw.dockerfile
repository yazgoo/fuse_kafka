#include "homeship.dockerfile
env http_proxy http://10.232.69.38:8080
env https_proxy http://10.232.69.38:8080
user root
run apt-get -y install mingw-w64
run dpkg --add-architecture i386
run apt-get -y install ppa-purge
run add-apt-repository ppa:ubuntu-wine/ppa && apt-get update
run apt-get -y install wine1.6-amd64
run apt-get -y install ant default-jdk
run apt-get -y install autoconf libcppunit-dev libtool
user homeship
run git reset --hard HEAD
run git pull
run git pull
run git pull
run git pull
run git pull
run SRCROOT=/tmp/lolo BUILDROOT=$PWD/../out/fuse_kafka/ CXX=x86_64-w64-mingw32-g++ CC=x86_64-w64-mingw32-gcc CFLAGS="-I$PWD/../out/fuse_kafka/include -DMINGW_VER -D_X86INTRIN_H_INCLUDED -DWIN32 -DNDEBUG -D_WINDOWS -D_USRDLL -DZOOKEEPER_EXPORTS -DDLL_EXPORT -w -fpermissive -D_X86INTRIN_H_INCLUDED -DLIBRDKAFKA_EXPORTS -DInterlockedAdd=_InterlockedAdd -DMINGW_VER -D_WIN32_WINNT=0x0760" LDFLAGS="-L$PWD/../out/fuse_kafka/lib -Xlinker --no-undefined -Xlinker --enable-runtime-pseudo-reloc" LIBS="-lwsock32 -lws2_32 -lpsapi" archive_cmds_need_lc=no LDSHAREDLIBC= ./build.py binary_archive
