#!/usr/bin/env python
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
def get_version():
    for source in sources:
        f = open(source + ".c")
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
def to_links(libs):
    return ['-l'+s for s in libs]
def dotest():
    run('rm', '-rf', 'out')
    run('mkdir', '-p', 'out')
    compile_test()
    test()
def build():
    #compile()
    link()
def test():
    for source in sources:
        run("ls")
        run("./" + source + ".test")
        run("gcov", source + ".c")
        run("lcov", "-c", "-d", ".", "-o", source + ".info")
        run("genhtml", source + ".info", "-o", "./out")
def compile():
    for source in sources:
        run('gcc', '-g', '-c', source+'.c', flags)
def compile_test():
    for source in sources:
        run('gcc', '-o', source+'.test', source+'.c', flags,
                test_flags, to_links(common_libs))
def link():
    objects = [s+'.o' for s in sources]
    run('gcc', '-static', '-g', sources[0]+'.c', '-o', binary_name, flags, to_links(libs))
    #run('gcc', '-g', objects, '-o', binary_name, flags, to_links(libs))
def install():
    build()
    install_directory = '/usr/bin/'
    if not os.access(install_directory, os.W_OK):
        install_directory = '/tmp/'
    run('cp', binary_name, install_directory)
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
    run(kafka_bin_directory + 'kafka-console-consumer.sh',
            kafka_config_directory,
            "--zookeeper", "localhost:2181",
            "--topic", "logs")
main()
