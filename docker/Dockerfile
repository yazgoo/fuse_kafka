from centos:6
env http_proxy http://10.232.69.9:8080/
env https_proxy http://10.232.69.9:8080/
run yum install -y rpm-build git lsb wget
run git clone https://github.com/yazgoo/fuse_kafka.git
workdir fuse_kafka
run bash ./setup.sh
run yum install -y zlib zlib-devel zlib-static fuse-devel librdkafka-devel libzookeeper-devel jansson-devel
run ./build.py rpm
# docker cp  image_id:/rpmbuild/RPMS/x86_64/fuse_kafka-0.1.5-1.el6.x86_64.rpm .
