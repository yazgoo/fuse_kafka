from centos6
run git clone https://github.com/yazgoo/fuse_kafka.git
workdir fuse_kafka
./setup.sh CentOS
yum install -y rpmbuild
./build.py rpm
