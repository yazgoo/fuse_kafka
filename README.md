[![Build Status](https://travis-ci.org/yazgoo/fuse_kafka.svg?branch=master)](https://travis-ci.org/yazgoo/fuse_kafka)
[![Coverage Status](https://img.shields.io/coveralls/yazgoo/fuse_kafka.svg)](https://coveralls.io/r/yazgoo/fuse_kafka?branch=master)
[![Gitter](http://img.shields.io/badge/gitter-join chat-1dce73.svg?style=flat)](https://gitter.im/yazgoo/fuse_kafka?utm_source=badge&utm_medium=badge&utm_campaign=pr-badge&utm_content=badge)
[![Documentation](http://img.shields.io/badge/documentation-%E2%9C%93-blue.svg?style=flat)](http://yazgoo.github.io/fuse_kafka/html/)
[![Open Build Service](http://img.shields.io/badge/install-packages-yellow.svg?style=flat)](http://software.opensuse.org/download.html?project=home%3Ayazgoo&package=fuse_kafka)
[![Benchmarks](http://img.shields.io/badge/benchmarks-bonnie++-949494.svg?style=flat)](http://htmlpreview.github.io/?https://raw.githubusercontent.com/yazgoo/fuse_kafka/master/benchs/benchmarks.html#5)

![fuse kafka logo](https://raw.githubusercontent.com/yazgoo/fuse_kafka/master/graphics/fuse_kafka_logo.png "Logo")

Intercepts all writes to specified directories and send them 
to apache kafka brokers.  Quite suited for log centralization.

Installing
==========

Packages for various distros can be installed from [these repositories](http://download.opensuse.org/repositories/home:/yazgoo/) at [openSUSE Build Service](https://build.opensuse.org/package/show/home:yazgoo/fuse_kafka).

The following should install the new repositories then install fuse\_kafka:

    # curl -O \
        https://raw.githubusercontent.com/yazgoo/fuse_kafka/master/setup_packages.sh \
        && md5sum -c <(echo "123cd73857d94af4dfc64e03d29d9789 setup_packages.sh") \
        && chmod +x setup_packages.sh && ./setup_packages.sh

Configuration
=============

A default configuration file is available in conf/fuse\_kafka.properties.
An explanation for each parameter is available in this file.
The packages should install it in /etc/fuse\_kafka.conf.

Quickstart (from sources)
=========================

[Here is a capture of a quickstart](http://playterm.org/r/fusekafka-quickstart-1416935569).



If you want to test fuse\_kafka, using a clone of this repository.

On Debian and Ubuntu, you should install the following:

 *  librdkafka-dev
 *  librdkafka1
 *  libzookeeper-mt-dev
 *  libzookeeper-mt2
 *  libjansson-dev
 *  libjansson4
 *  python

First, build it:

    $ ./build.py

On another terminal session, start zookeeper (this will download kafka):

    $ ./build.py zookeeper_start

On another one, start kafka:

    $ ./build.py kafka_start

The default configuration is conf/fuse\_kafka.properties.
An important piece of configuration is fuse\_kafka\_directories:

    $ grep fuse_kafka_directories conf/fuse_kafka.properties -B2
    # directories fuse_kafka will listen to (launch script will try to
    # create them if they don't exist)
    fuse_kafka_directories=["/tmp/fuse-kafka-test"]

Start fuse\_kafka using the init script:

    $ src/fuse_kafka.py start

If you're not running as root, you might have to make 
/etc/fuse.conf readable by your user (here to all users):

    $ chmod a+r /etc/fuse.conf

And allow non-root user to specify the allow\_other option, by adding
a line with user\_allow\_other in /etc/fuse.conf.

If fuse\_kafka is running, you should get the following output when
running:

    $ src/fuse_kafka.py status
    listening on /tmp/fuse-kafka-test
    fuse kafka is running

In yet another terminal, start a test consumer:

    $ ./build.py kafka_consumer_start

Then start writing to a file under the overlay directory:

    $ bash -c 'echo "foo"' > /tmp/fuse-kafka-test/bar

You should have an output from the consumer similar to this:

    event:
        group: users
        uid: 1497
        @tags:
            -  test
        @fields:
             hostname: test
        @timestamp: 2014-10-03T09:07:04.000+0000
        pid: 6485
        gid: 604
        command: bash -c echo "foo"
        @message: foo
        path: /tmp/fuse-kafka-test/bar
        @version: 0.1.3
        user: yazgoo

When you're done, you can stop fuse\_kafka:

    $ src/fuse_kafka.py stop

Quota option test
=================

First, commment fuse\_kafka\_quota in conf/fuse\_kafka.properties.
Then, start fuse kafka.

    $ src/fuse_kafka.py start

Let's create a segfaulting program:

    $ cat first.c
    int main(void)
    {
        *((int*)0) = 1;
    }

    $ gcc first.c

Then start a test consumer, displaying only the path and message\_size-added fields

Launch the segfaulting program in fuse-kafka-test directory:

    $ /path/to/a.out

A new core file should appear in fused directory.

Here is the consumer output:

    $ SELECT="message_size-added path" ./build.py kafka_consumer_start
    event:
        message_size-added: 4096
        path: /tmp/fuse-kafka-test/core
    ...
    event:
        message_size-added: 4096
        path: /tmp/fuse-kafka-test/core

Here we see many messages.

Then, uncomment fuse\_kafka\_quota in conf/fuse\_kafka.properties and 
launch the segfaulting program,
        
    $ SELECT="message_size-added path" ./build.py kafka_consumer_start
    event:
        message_size-added: 64
        path: /tmp/fuse-kafka-test/core

This time, we only receive the first write.

Event format
============

We use a logstash event, except the message and command are base64 encoded:

    {"path": "/var/log/redis_6380.log", "pid": 1262, "uid": 0, "gid": 0,
    "@message": "aGVsbG8gd29ybGQ=",
    "@timestamp": "2014-09-11T14:19:09.000+0000","user": "root", "group":
    "root",
    "command": "L3Vzci9sb2NhbC9iaW4vcmVkaXMtc2VydmVyIC",
    "@version": "0.1.2",
    "@fields": {
        "first_field": "first_value",
        "second_field": "second_value" },
    "@tags": ["mytag"]}


Installing from sources
=======================

    # installing prerequisites
    $ sudo apt-get install librdkafka-dev libfuse-dev
    # building
    $ ./build.py 
    # testing
    $ ./build.py dotest
    # cleaning
    $ ./build.py clean
    # installing:
    $ ./build.py install


Using valgrind or another dynamic analysis tool
===============================================

The start script allows to specify a command to append when actually
launching the binary, FUSE\_KAFKA\_PREFIX, which can be used to
perform analyses while running (like memcheck):
    
    FUSE_KAFKA_PREFIX="valgrind --leak-check=yes" ./src/fuse_kafka.py start


Anti hanging
============

Fuse-kafka must never make your filesystem accesses hang.
Although this should be considered as a major bug, this might happen
since the soft is still young.
You can install a script call in your crontab so that any FS hanging
is umounted (the check will occur every minute).
To do so on an installed instance:

    # service fuse_kafka cleanup

To do so on a source based instance:

    $ ./src/fuse_kafka.py cleanup

Licensing
=========

licensed under Apache v 2.0, see LICENSE file


