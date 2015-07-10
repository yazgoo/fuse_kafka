# 1 "homeship_mingw.dockerfile"
# 1 "<built-in>"
# 1 "<command-line>"
# 1 "/usr/include/stdc-predef.h" 1 3 4
# 1 "<command-line>" 2
# 1 "homeship_mingw.dockerfile"
# 1 "homeship.dockerfile" 1
from shippableimages/ubuntu1404_python
ENV SHIPPABLE TRUE
ENV HOMESHIP TRUE
run groupadd homeship
run useradd --home /home/homeship homeship -g homeship
run sudo usermod -a -G sudo homeship
run echo homeship:homeship | chpasswd
run sed -i 's/^.sudo.*/%sudo  ALL=(ALL) NOPASSWD:ALL/' /etc/sudoers
run cat /etc/sudoers
run adduser homeship sudo
run mkdir -p /home/homeship
run chown -R homeship:homeship /home/homeship
user homeship
workdir /home/homeship
run ls -ld .
run pwd
run git clone https:
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
# 1 "homeship_mingw.dockerfile" 2
