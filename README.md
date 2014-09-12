Fuse-kafka intercepts all writes to specified directories and send them to
kafka brokers.
It is quite suited for log centralization.

Prerequisites
=============

    $ sudo apt-get install librdkafka-dev libfuse-dev

To build, you will need fabricate:

    $ pip install fabricate

Building
========

building:

    $ ./build.py

testing:

    $ ./build.py dotest

cleaning:

    $ ./build.py clean

install:

    $ ./build.py install

Usage
=====

mounting:

    $ ./fuse_kafka __mountpoint__ -oallow_other -ononempty \
        -s -omodules=subdir,subdir=. -- \
        --topic logs --fields first_field first_value \
            second_field second_value \
           --directories /var/log /other/path/to/overlay \
           --tags mytag
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
