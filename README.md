[![Build
Status](https://travis-ci.org/yazgoo/fuse_kafka.svg?branch=master)](https://travis-ci.org/yazgoo/fuse_kafka)

Intercepts all writes to specified directories and send them 
to apache kafka brokers.  Quite suited for log centralization.


Installing
==========

Packages for various distros can be installed from [these repositories](http://download.opensuse.org/repositories/home:/yazgoo/) at [openSUSE Build Service](https://build.opensuse.org/package/show/home:yazgoo/fuse_kafka)

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

Usage
=====

mounting:

    $ ./fuse_kafka __mountpoint__ -oallow_other -ononempty \
        -s -omodules=subdir,subdir=. -- \
        --topic logs --fields first_field first_value \
            second_field second_value \
           --directories /var/log /other/path/to/overlay \
           --tags mytag \
           --brokers mybroker1:9092,mybroker2:9092
unmounting:

    $ fusermount -u /var/log /other/path/to/overlay

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
