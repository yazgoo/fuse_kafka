#!/usr/bin/env python
from fabricate import *
import os
sources = ['fuse_kafka']
binary_name = sources[0]
common_libs = ["fuse", "dl", "plc4", "ulockmgr"]
libs = common_libs + ["rdkafka",  "z", "rt"]
flags = ['-D_FILE_OFFSET_BITS=64']
test_flags = ['-fprofile-arcs', '-ftest-coverage', '-DTEST="out"']
def version():
    for source in sources:
        with open(source + ".c") as f:
            return f.readline().split()[-1][1:-1]
def package():
    name = binary_name + "-" + version()
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
    compile()
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
        run('gcc', '-c', source+'.c', to_links(libs), flags)
def compile_test():
    for source in sources:
        run('gcc', '-o', source+'.test', source+'.c', flags,
                test_flags, to_links(common_libs))
def link():
    objects = [s+'.o' for s in sources]
    run('gcc', '-o', binary_name, objects, to_links(libs))
def install():
    build()
    install_directory = '/usr/bin/'
    if not os.access(install_directory, os.W_OK):
        install_directory = '/tmp/'
    run('cp', binary_name, install_directory)
def clean():
    autoclean()
main()
