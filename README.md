[![Build Status](https://travis-ci.org/yazgoo/fuse_kafka.svg?branch=master)](https://travis-ci.org/yazgoo/fuse_kafka)
[![Build Status](https://api.shippable.com/projects/549439afd46935d5fbc0a9cf/badge?branchName=master)](https://app.shippable.com/projects/549439afd46935d5fbc0a9cf/builds/latest)
[![Coverage Status](https://img.shields.io/coveralls/yazgoo/fuse_kafka.svg)](https://coveralls.io/r/yazgoo/fuse_kafka?branch=master)
[![Gitter](http://img.shields.io/badge/gitter-join chat-1dce73.svg?style=flat)](https://gitter.im/yazgoo/fuse_kafka?utm_source=badge&utm_medium=badge&utm_campaign=pr-badge&utm_content=badge)
[![Documentation](http://img.shields.io/badge/doc-%E2%9C%93-blue.svg?style=flat)](http://yazgoo.github.io/fuse_kafka/html/)
[![Open Build Service](http://img.shields.io/badge/install-packages-yellow.svg?style=flat)](http://software.opensuse.org/download.html?project=home%3Ayazgoo&package=fuse_kafka)
[![Benchmarks](http://img.shields.io/badge/benchs-bonnie++-949494.svg?style=flat)](http://htmlpreview.github.io/?https://raw.githubusercontent.com/yazgoo/fuse_kafka/master/benchs/benchmarks.html#5)

[![Docker](http://dockeri.co/image/yazgoo/fuse_kafka)](https://registry.hub.docker.com/u/yazgoo/fuse_kafka/)

![fuse kafka logo](https://raw.githubusercontent.com/yazgoo/fuse_kafka/master/graphics/fuse_kafka_logo.png "Logo")

Intercepts all writes to specified directories and send them 
to apache kafka brokers.  Quite suited for log centralization.

Installing
==========

Packages for various distros can be installed from [these repositories](http://download.opensuse.org/repositories/home:/yazgoo/) at [openSUSE Build Service](https://build.opensuse.org/package/show/home:yazgoo/fuse_kafka).

The following should install the new repositories then install fuse\_kafka:

    # curl -O \
        https://raw.githubusercontent.com/yazgoo/fuse_kafka/master/setup.sh \
        && md5sum -c <(echo "5a33de1d49a5ab0aebff0e6196f048ef  setup.sh") \
        && chmod +x setup.sh && ./setup.sh

(for more options - e.g. to install on a machine with no access to the repos - see 'setup.sh options' section)

Configuration
=============

A default configuration file is available in conf/fuse\_kafka.properties.
An explanation for each parameter is available in this file.
The packages should install it in /etc/fuse\_kafka.conf.

Quickstart (from sources)
=========================

[Here is a capture of a quickstart](http://playterm.org/r/fusekafka-quickstart-1416935569).
([Download it](http://abdessel.iiens.net/fuse_kafka/ttyrecord) - use ttyplay to view)



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

````python
# directories fuse_kafka will listen to (launch script will try to
# create them if they don't exist)
fuse_kafka_directories=["/tmp/fuse-kafka-test"]
````

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

````yaml
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
````

When you're done, you can stop fuse\_kafka:

    $ src/fuse_kafka.py stop


Quota option test
=================

First, commment fuse\_kafka\_quota in conf/fuse\_kafka.properties.
Then, start fuse kafka.

    $ src/fuse_kafka.py start

Let's create a segfaulting program:

    $ cat first.c

````c
int main(void)
{
    *((int*)0) = 1;
}
````

````shell
$ gcc first.c
````

Then start a test consumer, displaying only the path and message\_size-added fields

Launch the segfaulting program in fuse-kafka-test directory:

````shell
$ /path/to/a.out
````
A new core file should appear in fused directory.

Here is the consumer output:

````shell
$ SELECT="message_size-added path" ./build.py kafka_consumer_start
event:
    message_size-added: 4096
    path: /tmp/fuse-kafka-test/core
...
event:
    message_size-added: 4096
    path: /tmp/fuse-kafka-test/core
````

Here we see many messages.

Then, uncomment fuse\_kafka\_quota in conf/fuse\_kafka.properties and 
launch the segfaulting program,
        
````shell
$ SELECT="message_size-added path" ./build.py kafka_consumer_start
event:
    message_size-added: 64
    path: /tmp/fuse-kafka-test/core
````

This time, we only receive the first write.

Event format
============

We use a logstash event, except the message and command are base64 encoded:

````json
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
````


Installing from sources
=======================

    # installing prerequisites
    $ sudo apt-get install librdkafka-dev libfuse-dev
    # building
    $ ./build.py 
    # testing
    $ ./build.py test
    # cleaning
    $ ./build.py clean
    # installing:
    $ ./build.py install

    You can add c compiling flags via CFLAGS environment variable:

    $ ./build.py CFLAGS=-Wall ./build.py

Using valgrind or another dynamic analysis tool
===============================================

The start script allows to specify a command to append when actually
launching the binary, FUSE\_KAFKA\_PREFIX, which can be used to
perform analyses while running (like memcheck):
    
    FUSE_KAFKA_PREFIX="valgrind --leak-check=yes" ./src/fuse_kafka.py start


Debugging with gdb
==================

You can also debug using FUSE\_KAFKA\_PREFIX, here is how to do so:

    $ echo -e "set follow-fork-mode child\nrun\nwhere" > /tmp/gdb_opts
    $ FUSE_KAFKA_PREFIX="gdb -x /tmp/gdb_opts --args" ./src/fuse_kafka.py start

Anti hanging
============

Fuse-kafka must never make your filesystem accesses hang.
Although this should be considered as a major bug, this might happen
since the soft is still young.
You can run a daemon so that any FS hanging
is umounted (the check will occur every minute).
To do so on an installed instance:

    # service fuse_kafka_umounter start

To do so on a source based instance:

    $ ./src/fuse_kafka.py start


setup.sh options
================

Here are available options:

 - `-r`: to do a remote install via ssh: `-r private_ssh_key user@host`
 - `-d`: download and do not install the packages, generating an archive
 - `-f`: install an archive already built via -d: `-f fuse_kafka.tar.bz2`

For example, this will download packages on a remote server:

````shell
$ ./setup.sh -r mykey.pem root@myserver -d
````

This will generate an archive that will be copied locally.
You can then install that archive via:

````shell
$ ./setup.sh -f fuse_kafka.tar.bz2
````


Networking tests
================

A more realistic network setup test can be launched (as root) via:

````shell
$ sudo ./build.py mininet
````

This requires [mininet](http://mininet.org).

This will launch kafka, zookeeper, fuse_kafka, and a consumer 
on their own mininet virtual hosts with their own network stacks.
fuse_kafka is running on h3 (host number three).
This will launch a mininet shell.
For example, if you want to try and write on fuse_kafka host, issue a:

````shell
mininet> h3 echo lol > /tmp/fuse-kafka-test/xd
````

The consumer log is available via (/tmp/kafka_consumer.log). 

`quit` or `^D` will stop mininet and cleanup the virtual network.


Logstash input plugin
=====================

A logstash input plugin to read from kafka is available in src/logstash/inputs/kafka.rb

Provided you have kafka installed in . (which `./build.py kafka_start` should do),
you can try it by downloading logastash and running:

````shell
$ /path/to/bin/logstash -p ./src/ -f ./conf/logstash.conf
````

Unit testing
============

To launch unit tests, issue a:

````shell
    ./build.py test
````

To test against multiple python versions (provided tox is installed), issue a:

````shell
$ tox
````

(see .travis.yml `# prerequisites for tox` to install these versions on ubuntu).


C Unit testing
==============

To run c unit tests, do a:

````shell
$ rm -rf out/ ; mkdir -p out/c ; ./build.py compile_test && ./build.py c_test
````


Working with other logging systems
==================================

Basically, any process that has a file handle opened before fuse_kafka starts
won't have it's writes captured.
Such a process must open a new file handle after fuse_kafka startup,
for example by restarting the process.

For example, If you're using rsyslogd and it is writing to /var/log/syslog,
after starting fuse_kafka on /var/log, you should issue a:

````shell
$ service rsyslogd restart
````

After stopping fuse_kafka, you should also restart rsyslogd so 
it re-acquires a file descriptor on the actual FS.

Benchmarks
==========

Provided you have bonnie++ installed, you can run benchmarks with

````shell
$ ./build.py bench
````

This will generate `bench/results.js`, which you can see via `benchs/benchmarks.html`

Dynamic configuration
=====================

You might want to have fuse_kafka start ahead of most processes.
But when it starts, you might not have all its configuration available yet.
Or you might want to add brokers or use new zookeepers.

Dynamic configuration allows to modify the configuration on the fly.
You will be able to:

* point to new zookeepers/brokers
* update tags, fields
* modify watched directories

Just update your configuration, then, issue a:

````shell
$ sevice fuse_kafka reload
````

Or, if you using the developer version:

````shell
./src/fuse_kafka.py reload
````

To use this feature, you must make sure that /var/run/fuse_kafka.args is accessible to fuse_kafka.


Input plugin
============

You can write your own input plugins in `src/plugins/input`.
An example input plugin is available in `src/plugins/input/example.c`.
A plugin should include:

````c
#include <input_plugin.h>
````

Its entry point is the function:

````c
int input_setup(int argc, char** argv, void* conf)
````

With parameters being:

parameter name | description
---------------|------------
argc           | number of command line arguments (without arguments given after `--` )
argv           | array of arguments
conf           | parsed configuration based on arguments given after `--` (see config.h)

It should output it's data using:

````c
void output_write(const char *path, const char *buf,
        size_t size, off_t offset)
````

With parameters being:

parameter name | description
---------------|------------
path           | path of the file where the log line comes from
buf            | buffer containing the log line
size           | size of the log line
offset         | start of the log line in buf

If you require some library, you should refer to its pkg-config name via the macro:

````c
require(your-library)
````

Input plugin unit testing
=========================

Each input plugin should have a unit test (with the suffix `_test`).

For example, 

    src/plugins/input/overlay.c

Has a unit test

    src/plugins/input/overlay_test.c

As for the rest of the project, We use minunit for that.
Just include `minuti.h`, your plugin source.
Define your unit test functions, as for example:

```C
static char* test_something()
{
    /*...*/
    mu_assert("42 is 42", 42 == 42);
    /*...*/
    return 0;
}
````

And then define an `all_test()` function calling all tests

````C
static char* all_tests()
{
    mu_run_test(test_something);
    mu_run_test(test_something_else);
    return 0;
}
````

and include "minunit.c"

````C
#include "minunit.c"
````

Licensing
=========

licensed under Apache v 2.0, see LICENSE file


Code of conduct
===============

Please note that this project is released with a Contributor Code of Conduct.
By participating in this project you agree to abide by its terms.
See CODE OF CONDUCT file.

