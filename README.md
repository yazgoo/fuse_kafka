[![Build Status](https://travis-ci.org/yazgoo/fuse_kafka.svg?branch=master)](https://travis-ci.org/yazgoo/fuse_kafka)
[![Gitter chat](https://badges.gitter.im/yazgoo/fuse_kafka.png)](https://gitter.im/yazgoo/fuse_kafka)

![fuse kafka logo](https://raw.githubusercontent.com/yazgoo/fuse_kafka/master/graphics/fuse_kafka_logo.png "Logo")

Intercepts all writes to specified directories and send them 
to apache kafka brokers.  Quite suited for log centralization.

Installing
==========

Packages for various distros can be installed from [these repositories](http://download.opensuse.org/repositories/home:/yazgoo/) at [openSUSE Build Service](https://build.opensuse.org/package/show/home:yazgoo/fuse_kafka).


Quickstart
==========

If you want to test fuse\_kafka, using a clone of this repository.

First, build it:

    $ ./build.py

On another terminal session, start zookeeper (this will download kafka):

    $ ./build.py zookeeper_start

On another one, start kafka:

    $ ./build.py kafka_start

The default configuration is conf/fuse\_kafka.properties.
An important piece of configuration is:

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
a line with

    user_allow_other

in /etc/fuse.conf

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

Configuration
=============

A default configuration file is available in conf/fuse\_kafka.properties.
An explanation for each parameter is available in this file.

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
