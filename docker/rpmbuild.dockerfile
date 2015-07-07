from centos:6
run yum install -y rpm-build git lsb wget
run git clone https://github.com/yazgoo/fuse_kafka.git
workdir fuse_kafka
run bash ./setup.sh
run ./build.py rpm
