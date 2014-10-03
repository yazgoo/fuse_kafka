[![Build Status](https://travis-ci.org/yazgoo/fuse_kafka.svg?branch=master)](https://travis-ci.org/yazgoo/fuse_kafka)

Intercepts all writes to specified directories and send them 
to apache kafka brokers.  Quite suited for log centralization.

![fuse kafka logo](https://raw.githubusercontent.com/yazgoo/fuse_kafka/master/graphics/fuse_kafka_logo.png "Logo")


Installing
==========

Packages for various distros can be installed from [these repositories](http://download.opensuse.org/repositories/home:/yazgoo/) at [openSUSE Build Service](https://build.opensuse.org/package/show/home:yazgoo/fuse_kafka).


Quickstart
==========

If you want to test fuse\_kafka

First, build it:

    $ ./build.py

On another terminal session, start zookeeper (this will download kafka):

    $ ./build.py zookeeper_start

On another one, start kafka:

    $ ./build.py kafka_start

Overlay a directory (here I mount /tmp/blax):

    $ ./fuse_kafka  __mountpoint__ -oallow_other -ononempty -s \
        -omodules=subdir,subdir=. -f -- --topic logs --directories \
        /tmp/blax --brokers localhost:9092 --tags lol --fields a b

If you're not running as root, you might have to make 
/etc/fuse.conf readable by your user:

    $ chmod a+r /etc/fuse.conf

And allow non-root user to specify the allow\_other option, by adding
a line with

    user_allow_other

in /etc/fuse.conf

In yet another terminal, start a consumer:

    $ ./build.py kafka_consumer_start

Then start writing to a file under the overlay directory:

    $ bash -c 'echo "foo"' > /tmp/blax/bar

You should have an output from the consumer similar to this:

```yaml
event:
    group: users
    uid: 1497
    @tags:
        -  lol
    @fields:
         a: b
    @timestamp: 2014-10-03T09:07:04.000+0000
    pid: 6485
    gid: 604
    command: bash -c echo "foo"
    @message: foo
    path: /tmp/blax/bar
    @version: 0.1.3
    user: yazgoo
```

Usage
=====

mounting:

```shell
fuse_kafka __mountpoint__ -oallow_other -ononempty \
        -s -omodules=subdir,subdir=. -- \
        --topic logs --fields first_field first_value \
            second_field second_value \
           --directories /var/log /other/path/to/overlay \
           --tags mytag \
           --brokers mybroker1:9092,mybroker2:9092
```
unmounting:

```shell
fusermount -u /var/log /other/path/to/overlay
```

Event format
============

We use a logstash event, except the message and command are base64 encoded:

```json
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
```

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
