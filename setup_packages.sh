# helper script to install fuse_kafka from OBS packages
distro_version=$(lsb_release -s -r)
distro_name=$(lsb_release -s -i)
add_yum_repos() {
    echo \
    "[$1]
    name=$1's repo
    baseurl=http://download.opensuse.org/repositories/home\:/$1/$2
    gpgcheck=0
    enabled=1" > /etc/yum.repos.d/$name.repo
}
install_CentOS() {
    for who in edenhill yazgoo
    do add_yum_repo $who $distro_name-6
    done
    add_yum_repo yazgoo $distro_name-6
    [ "$distro_version" = "6.3" ] && yum -y update glibc
    yum -y install fuse_kafka
}
install_$distro_name
