#!/usr/bin/env python
import json, base64, subprocess, sys, glob
from fabricate import *
import os
sources = ['fuse_kafka']
binary_name = sources[0]
common_libs = ["crypto", "fuse", "dl", "pthread"]#, "ulockmgr"]
libs = ["rdkafka",  "z", "rt"] + common_libs
flags = ['-D_FILE_OFFSET_BITS=64']
test_flags = ['-fprofile-arcs', '-ftest-coverage', '-DTEST="out"']
kafka_server = "http://mir2.ovh.net/ftp.apache.org/dist/kafka/"
kafka_version = "0.8.1.1"
scala_version = "2.8.0"
kafka_directory = "kafka_" + scala_version + "-" + kafka_version
kafka_archive = kafka_directory + ".tgz"
kafka_bin_directory = kafka_directory + "/bin/"
kafka_config_directory = kafka_directory + "/config/"
class FuseKafkaLog:
    def run_command(self, *command):
        p = subprocess.Popen(command,
                stdout=subprocess.PIPE,
                stderr=subprocess.STDOUT)
        return iter(p.stdout.readline, b'')
    def pretty_print(self, string):
        struct = self.load_fuse_kafka_event(string)
        print "event:"
        for key in struct:
            sys.stdout.write("    " + key + ": ")
            value = struct[key]
            if type(value) is dict:
                print
                for name in value:
                    print "        ", name + ':', value[name]
            elif type(value) is list:
                print
                for v in value:
                    print "        - ", v
            else:
                print value
    def load_fuse_kafka_event(self, string):
        event = json.loads(string)
        for item in ["@message", "command"]:
            event[item] += "=" * ((4 - len(event[item]) % 4) % 4)
            event[item] = base64.b64decode(event[item])
        return event
    def start(self):
        for line in self.run_command(os.getcwd() + "/"
                + kafka_bin_directory + 'kafka-console-consumer.sh',
            "--zookeeper", "localhost:2181",
            "--topic", "logs"):
            try:
                self.pretty_print(line)
            except ValueError:
                print line
def get_version():
    for source in sources:
        f = open("src/" + source + ".c")
        [f.readline() for i in range(3)]
        result = f.readline().split()[-1][1:-1]
        f.close()
        return result
def version():
    print(get_version())
def package():
    name = binary_name + "-" + get_version()
    tar = "../" + name + ".tar.gz"
    run("tar", "--transform", "s,^.," + name + ",",
            "--exclude=.git", 
            "--exclude=out", "-czf", tar , ".")
def filter_link(a):
    if a != "-lcrypto": return a
    result = []
    for pattern in ["/usr/lib*/libcrypto.a", "/usr/lib*/*/libcrypto.a"]:
        result += glob.glob(pattern)
    return result[0]
def to_links(libs):
    return [filter_link(a) for a in ['-l'+s for s in libs]]
def dotest():
    run('rm', '-rf', 'out')
    run('mkdir', '-p', 'out')
    compile_test()
    test()
def build():
    compile()
    link()
def test():
    for source in sources:
        run("ls")
        run("./" + source + ".test")
        run("gcov", "./src/" + source + ".c")
        run("lcov", "-c", "-d", ".", "-o", "./src/" + source + ".info")
        run("genhtml", source + ".info", "-o", "./out")
def compile():
    for source in sources:
        run('gcc', '-g', '-c', "./src/" + source+'.c', flags)
def compile_test():
    for source in sources:
        run('gcc', '-o', source+'.test', source+'.c', flags,
                test_flags, to_links(common_libs))
def link():
    objects = [s+'.o' for s in sources]
    run('gcc', '-g', objects, '-o', binary_name, flags, to_links(libs))
def install():
    root = '/'
    if os.environ.get('BUILDROOT') != None:
        root = os.environ.get('BUILDROOT') + "/"
    build()
    install_directory = root + 'usr/bin/'
    init_directory = root + 'etc/init.d/'
    conf_directory = root + 'etc/'
    [run('mkdir', '-p', d) for d in
            [conf_directory, init_directory, install_directory]]
    run('cp', binary_name, install_directory)
    run('cp', 'src/fuse_kafka.py', init_directory + "fuse_kafka")
    run('cp', 'conf/fuse_kafka.properties',
            conf_directory + "fuse_kafka.conf")
def clean():
    autoclean()
def kafka_download():
    run('wget', kafka_server + kafka_version + "/" + kafka_archive)
    run('tar', 'xzf', kafka_archive)
def zookeeper_start():
    kafka_download()
    run(kafka_bin_directory + 'zookeeper-server-start.sh',
            kafka_config_directory + 'zookeeper.properties')
def kafka_start():
    kafka_download()
    run(kafka_bin_directory + 'kafka-server-start.sh',
            kafka_config_directory + 'server.properties')
def kafka_consumer_start():
    FuseKafkaLog().start()
def doc():
    run('mkdir', '-p', 'doc')
    run("doxygen", "Doxyfile")
main()
