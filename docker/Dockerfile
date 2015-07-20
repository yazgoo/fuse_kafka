from shippableimages/ubuntu1404_python
ENV SHIPPABLE TRUE
ENV HOMESHIP TRUE
env http_proxy http://10.232.69.26:8080/
env https_proxy http://10.232.69.26:8080/
run groupadd homeship
run useradd --home /home/homeship homeship -g homeship
run sudo usermod -a -G sudo homeship
run echo homeship:homeship | chpasswd
run sed -i 's/^.sudo.*/%sudo  ALL=(ALL) NOPASSWD:ALL/' /etc/sudoers
run echo 'Defaults env_keep="http_proxy https_proxy"' >> /etc/sudoers
run cat /etc/sudoers
run adduser homeship sudo
run mkdir -p /home/homeship
run chown -R homeship:homeship /home/homeship
user homeship
workdir /home/homeship
run ls -ld .
run pwd
run git clone https://github.com/yazgoo/fuse_kafka.git
workdir fuse_kafka
run bash -c "[ -z \"${SHIPPABLE}\" ] && ( echo 'deb http://fr.archive.ubuntu.com/ubuntu/ trusty universe' | sudo tee -a /etc/apt/sources.list ) || echo"
run sudo apt-get -y update -qq
run ulimit -Sc unlimited
run sudo apt-get -y install gdb
run sudo add-apt-repository -y ppa:fkrull/deadsnakes
run sudo apt-get -y update
run bash -c "[ -z \"${HOMESHIP}\" ] && sudo apt-get -y install python python2.6 python2.7 python3.3 python3.4 || sudo apt-get install python"
run bash -c "[ -z \"${HOMESHIP}\" ] && sudo pip install tox || echo"
run sed -i 's/_mt/_st/' build.py
run sudo apt-get -y install python-coverage libfuse-dev librdkafka-dev libjansson-dev libzookeeper-st-dev
run bash -c "[ -z \"${SHIPPABLE}\" ] || sudo apt-get -y install libglib2.0"
run sudo pip install fabricate cpp-coveralls
run ./build.py clean
run ./build.py
run LD_LIBRARY_PATH=. ./build.py test
run bash -c "[ -z \"${HOMESHIP}\" ] && tox || echo"
run sudo ./build.py install
#include "homeship.dockerfile
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
