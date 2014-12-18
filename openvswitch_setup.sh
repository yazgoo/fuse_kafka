#!/bin/sh
apt-get update
apt-get install -y git automake autoconf gcc uml-utilities libtool build-essential git pkg-config linux-headers-`uname -r`
wget http://openvswitch.org/releases/openvswitch-1.10.0.tar.gz
tar zxvf openvswitch-1.10.0.tar.gz
cd openvswitch-1.10.0
./boot.sh
./configure --with-linux=/lib/modules/`uname -r`/build
make && make install
insmod datapath/linux/openvswitch.ko
mkdir -p /usr/local/etc/openvswitch
ovsdb-tool create /usr/local/etc/openvswitch/conf.db vswitchd/vswitch.ovsschema
ovsdb-server -v --remote=punix:/usr/local/var/run/openvswitch/db.sock \
                     --remote=db:Open_vSwitch,manager_options \
                     --private-key=db:SSL,private_key \
                     --certificate=db:SSL,certificate \
                     --pidfile --detach --log-file
ovs-vsctl --no-wait init
ovs-vswitchd --pidfile --detach
ovs-vsctl show
