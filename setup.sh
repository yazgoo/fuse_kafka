# /!\ When modifying this file don't forget to update checksum in README.md.
# This is a helper script to install fuse_kafka from OBS packages.
# If you want to generate a tar with all packages, use -d flag:
set -x
while [ $# -gt 1 ]
do
    if [ "$1" = "-r" ]
    then
        remotely="ssh -o LogLevel=ERROR -o \
            UserKnownHostsFile=/dev/null -o StrictHostKeyChecking=no \
            -o BatchMode=yes  -i $2 $3"
        shift; shift; shift
    elif [ "$1" = "-d" ]
    then
            downloaddir=/tmp/fuse_kafka
            yum_options="--downloadonly --downloaddir=$downloaddir"
            shift
    elif [ "$1" = "-f" ]
    then
        tar_path=$2
        shift; shift
    fi
done
if [ -n "$downloaddir" ]
then
    $remotely rm -rf $downloaddir
    $remotely mkdir $downloaddir
fi
distro_version=$($remotely lsb_release -s -r)
distro_name=$($remotely lsb_release -s -i)
install_yum() {
    [ -z "$downloaddir" ] || $remotely yum install -y yum-downloadonly
    if [ -z "$tar_path" ]
    then
        for who in edenhill yazgoo
        do
            $remotely wget http://download.opensuse.org/repositories/home:$who/$1/home:$who.repo
            $remotely cp  home:$who.repo /etc/yum.repos.d/
            shift
        done
        $remotely yum $yum_options install -y fuse_kafka
    else
        $remotely tar xjf $tar_path
        $remotely yum install -y fuse_kafka/*.rpm
        $remotely rm -rf fuse_kafka/
    fi
}
install_CentOS() {
    install_yum CentOS_CentOS-6 CentOS-6
}
install_Fedora() {
    distro=Fedora_${distro_version}
    install_yum $distro $distro
}
install_$distro_name
if [ -n "$downloaddir" ]
then
    $remotely tar cjf fuse_kafka.tar.bz2 -C $(dirname $downloaddir) $(basename $downloaddir)
    [ -z "$remotely" ] || $(echo $remotely|sed 's/ssh/scp/'):~/fuse_kafka.tar.bz2 .
fi
