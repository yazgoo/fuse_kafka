# /!\ when modifying this file don't forget to update checksum in README.md
# helper script to install fuse_kafka from OBS packages
set -x
distro_version=$(lsb_release -s -r)
distro_name=$(lsb_release -s -i)
add_yum_repo() {
    cat  > /etc/yum.repos.d/$1.repo << EOF
[$1]
name=$1's repo
baseurl=http://download.opensuse.org/repositories/home\:/$1/$2
gpgcheck=0
enabled=1
EOF
}
install_CentOS() {
    add_yum_repo edenhill ${distro_name}_$distro_name-6
    add_yum_repo yazgoo $distro_name-6
    [ "$distro_version" = "6.3" ] && yum -y update glibc
    yum -y install fuse_kafka
}
install_$distro_name
