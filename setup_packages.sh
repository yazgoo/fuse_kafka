# /!\ when modifying this file don't forget to update checksum in README.md
# helper script to install fuse_kafka from OBS packages
set -x
distro_version=$(lsb_release -s -r)
distro_name=$(lsb_release -s -i)
install_CentOS() {
    cd /etc/yum.repos.d/
    wget http://download.opensuse.org/repositories/home:yazgoo/CentOS-6/home:yazgoo.repo
    yum install fuse_kafka
}
install_Fedora() {
    cd /etc/yum.repos.d/
    wget http://download.opensuse.org/repositories/home:yazgoo/Fedora_${distro_version}/home:yazgoo.repo
    yum install fuse_kafka
}
install_$distro_name
