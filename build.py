#!/usr/bin/env python
""" Builds unit test binary """
try:
    import base64, subprocess, sys, glob, os, json, thread
    import multiprocessing, shutil, time, unittest
except ImportError, e:
    print "failed importing module", e
from fabricate import *
def get_define(source, name):
    """ Returns the current define for fuse_kafka based on src/source """
    f = open("src/" + source + ".h")
    result = []
    while True:
        result = f.readline().split()
        if len(result) == 3 and result[0] == "#define" and result[1] == name:
            break;
    result = result[-1][1:-1]
    f.close()
    return result
class Plugins:
    def get_macro_definition(self, name, path):
        cmd = "sh -c \"grep --color=no '^"+name+"(' " + path + ".c | sed 's/^"+name+"(\(.*\))$/\\1/'\""
        return os.popen(cmd).read().rstrip()
    def get_target(self, path):
        target_str = self.get_macro_definition("target", path)
        if target_str == '': return '.*'
        return target_str
    def get_requirements(self, path):
        required_str = self.get_macro_definition("requires", path)
        required = []
        if required_str != '': required = required_str.split("\n")
        return required
    def __init__(self, cc):
        self.cc = cc
        _dir = "src/plugins/input/"
        self.tests_sources = [os.path.splitext(os.path.basename(a))[0] for a in glob.glob(_dir + "*_test.c")]
        self.tests_paths = [_dir.replace("src/", "") + x for x in self.tests_sources]
        self.libraries_sources = [os.path.splitext(os.path.basename(a))[0] for a in glob.glob(_dir + "*.c")]
        self.libraries_sources = [x for x in self.libraries_sources if x not in self.tests_sources]
        self.libs_of = {}
        self.targets_of = {}
        self.includes_of = {}
        self.kind_of = {}
        self.shareds_objects = {}
        self.objects = {}
        self.test_of = {}
        for kind in ['input', 'output']:
            self.fill_up(kind)
    def fill_up(self, kind):
        _dir = "src/plugins/" + kind + "/"
        self.tests_sources += [os.path.splitext(os.path.basename(a))[0] for a in glob.glob(_dir + "*_test.c")]
        self.tests_paths += [_dir.replace("src/", "") + x for x in self.tests_sources]
        current_sources = [os.path.splitext(os.path.basename(a))[0] for a in glob.glob(_dir + "*.c")]
        current_sources = [x for x in current_sources if x not in self.tests_sources]
        prefix = get_define("version", kind.upper() + "_PLUGIN_PREFIX")
        for lib in current_sources:
            self.kind_of[lib] = kind
            required = self.get_requirements(_dir + lib)
            self.targets_of[lib] = self.get_target(_dir + lib)
            self.libs_of[lib] = required + default_libs
            self.includes_of[lib] = required
            self.test_of[lib] = ((_dir + lib) +  "_test").replace("src/", "")
            self.shareds_objects[lib] = prefix + lib + ".so"
            self.objects[lib] = prefix + lib + ".o"
        self.libraries_sources +=  current_sources
cc = 'gcc'
if "CC" in os.environ:
    cc = os.environ["CC"]
_flags = []
for flag in ["CFLAGS", "LDFLAGS"]:
    if flag in os.environ:
        _flags = _flags + os.environ[flag].split()
zookeeper_type = "zookeeper_mt"
if "zookeeper_st" in os.environ:
    zookeeper_type = "zookeeper_st"
cc = [cc, _flags]
sources = ['fuse_kafka']
binary_name = sources[0]
common_libs = ["m", "dl", "pthread", "jansson"]#, "ulockmgr"]
libs = [zookeeper_type, "rdkafka",  "z"] + common_libs
default_libs = ["m",  zookeeper_type, "rdkafka", "jansson", "dl"]
if "LIBS" in os.environ:
    additional_libs = [a.replace("-l", "") for a in os.environ["LIBS"].split()]
    default_libs += additional_libs
    libs += additional_libs
plugins = Plugins(cc)
flags = ['-D_FILE_OFFSET_BITS=64']
if "CFLAGS" in os.environ:
    flags = os.environ["CFLAGS"].split() + flags
test_flags = ['-fprofile-arcs', '-ftest-coverage', '-DTEST="out"']
kafka_server = "http://mir2.ovh.net/ftp.apache.org/dist/kafka/"
kafka_version = "0.8.1.1"
scala_version = "2.8.0"
kafka_directory = "kafka_" + scala_version + "-" + kafka_version
kafka_archive = kafka_directory + ".tgz"
kafka_bin_directory = kafka_directory + "/bin/"
kafka_config_directory = kafka_directory + "/config/"
class Benchmark:
    def run_low_level(self):
        return os.popen("bonnie++ -q -n 128 -d /tmp/fuse-kafka-test").read()
    def run(self):
        s = self.run_low_level().split(",")
        create_subsections = ["create", "read", "delete"]
        output_subsections = ["per char", "block", "rewrite"]
        input_subsections = ["sequential per char",
                "sequential block", "random seeks"]
        res = {
                "version": s[0],
                "hostname": s[2],
                "timestamp": int(s[4]),
                "files": int(s[19]),
                "sequential create": self.parse_action(
                    s, 24, 0, create_subsections),
                "random create": self.parse_action(s, 24, 6,
                    create_subsections),
                "sequential output": self.parse_action(s, 7, 0,
                    output_subsections),
                "input": self.parse_action(s, 7, 6,
                    input_subsections),
                }
        return res
    def int(self, i):
        if i[0] == '+': return 0;
        return float(i)
    def p(self, s, _, d, subsections, i, n):
        return {
                "per second": self.int(s[_ + i + d]),
                "cpu percent": self.int(s[_ + i + 1 + d]),
                "latency": s[_ + n + d/2]
                }
    def parse_action(self, s, _, d, subsections):
        return {
                subsections[0]: self.p(s, _, d, subsections, 0, 18),
                subsections[1]: self.p(s, _, d, subsections, 2, 19),
                subsections[2]: self.p(s, _, d, subsections, 4, 20)
                }
class Benchmarks:
    def generate(self):
        import json
        result = {}
        os.system('./src/fuse_kafka.py start')
        result["with fuse kafka"] = Benchmark().run()
        os.system('./src/fuse_kafka.py stop')
        result["without fuse kafka"] = Benchmark().run()
        f = open("benchs/results.js", "w")
        f.write("function get_results() { \nreturn "\
                + json.dumps(result, sort_keys=True,
                    indent=4, separators=(',', ': ')) + "; \n}\n")
        f.close()
class FuseKafkaLog:
    """ Utility to read messages from kafka based on fuse kafka format """
    def __init__(self, zkconnect = "localhost"):
        self.select = None
        self.zkconnect = zkconnect
    def run_command(self, *command):
        """ Run an interactive command line

        command - the command to run as an argument array

        Returns an iterator to the command stdout
        """
        p = subprocess.Popen(command,
                stdout=subprocess.PIPE,
                stderr=subprocess.STDOUT)
        return iter(p.stdout.readline, '')
    def pretty_print(self, string):
        """ Displays a json logstash/fuse_kafka event in a user friendly fashion
        
        string - the JSON input logstash/fuse_kafka event

        Example

            pretty_print('{"command": "bXkgY29tbWFuZA==", "@message": "bXkgbWVzc2FnZQ==", '
                + '"fields": {"a": "v"}, "tags": ["tag"]}')

            prints:

                event:
                  message_size-added: 0
                  fields:
                     a: v
                  command: my command
                  @message: my message
                  tags:
                    -  tag
        """
        struct = self.load_fuse_kafka_event(string)
        print "event:"
        for key in struct:
            if self.select != None and not key in self.select:
                continue
            sys.stdout.write("  " + key + ": ")
            value = struct[key]
            if type(value) is dict:
                print
                for name in value:
                    print "    ", name + ':', value[name]
            elif type(value) is list:
                print
                for v in value:
                    print "    - ", v
            else:
                print value
    def load_fuse_kafka_event(self, string):
        """ Decodes a json logstash/fuse_kafka event string, i.e.:
                - does a json decoding
                - decodes @message and command fields 
                - adds message_size-added field 
        
        string - the JSON input logstash/fuse_kafka event

        Example
            
            build.FuseKafkaLog().load_fuse_kafka_event(
              '{"command": "bXkgY29tbWFuZGU=", "@message": "bXkgbWVzc2FnZQ=="}')

            => {'message_size-added': 10,
                u'command': 'my commande',
                u'@message': 'my message'}

        Returns the decoded json object
        """
        event = json.loads(string)
        for item in ["@message", "command"]:
            event[item] += "=" * ((4 - len(event[item]) % 4) % 4)
            event[item] = base64.b64decode(event[item] + "==")
        event["message_size-added"] = len(event["@message"])
        return event
    def start(self):
        """ Launches a kafka console consumer and pretty prints 
            fuse_kafka/logstash events from this consumer

        - SELECT (environment variable): if defined, lists what field 
            names should be retrieved (whitespace separated)
        """
        if os.environ.get('SELECT') != None:
            self.select = os.environ.get('SELECT').split()
        for line in self.run_command(os.getcwd() + "/"
                + kafka_bin_directory + 'kafka-console-consumer.sh',
            "--zookeeper", self.zkconnect, "--topic", "logs"):
            try:
                self.pretty_print(line)
            except ValueError:
                print line
def get_version():
    """ Returns the current version for fuse_kafka based on src/version.h """
    return get_define("version", "VERSION")
def bump_version():
    """ Changes the version number if v variable if specified:
            - The version number is changed in src/version.h
            - New packaging files are created with their version bumped
        displays a Usage message otherwise

    Example

        $ ./build.py bump_version
        Usage: $ v=0.1.4 ./build.py bump_version

    - v (environment variable): the new version
    """
    v = os.environ.get('v')
    if v == None:
        print("Usage: $ v=" + get_version() + " " + sys.argv[0] + " bump_version")
        return
    previous_v = get_version()
    for ext in ["spec", "dsc"]:
        previous_path = "./packaging/fuse_kafka-{}.{}".format(previous_v, ext)
        path = "./packaging/fuse_kafka-{}.{}".format(v, ext)
        run("mv", previous_path, path)
        run("sed", "-i", "s/^\(Version: \).*/\\1{}/".format(v), path)
        run("git", "add", path)
    version = open("src/version.h", "w")
    version.write("#define VERSION \""+ v + "\"\n")
    version.close()
    print "version bumped from {} to {} ".format(previous_v, v)
def version():
    """ Displays the current version number

    Example

        $ ./build.py version
        0.1.4
    """
    print(get_version())
def package():
    """ Generates a tar.gz corresponding to current directory in the parent directory,
    excluding .git, .nfs*, out directory

    Example

        ./build.py package
        tar --transform s,^.,fuse_kafka-0.1.4, --exclude=.git --exclude=.nfs* --exclude=out -czf ../fuse_kafka-0.1.4.tar.gz .
    """
    clean()
    name = binary_name + "-" + get_version()
    tar = "../" + name + ".tar.gz"
    run("tar", "--transform", "s,^.," + name + ",",
            "--exclude=.git", 
            "--exclude=.nfs*",
            "--exclude=out", "-czf", tar , ".")
def rpm():
    name = binary_name + "-" + get_version()
    package()
    sources = os.environ["HOME"] + "/rpmbuild/SOURCES"
    run("mkdir", "-p", sources)
    tar = "../" + name + ".tar.gz"
    run("cp", tar, sources)
    run("rpmbuild", "-ba", "--define", "_srcdefattr (-,root,root)",
            "packaging/fuse_kafka.spec")
def filter_link(a):
    """ Filter function for link flags:
    takes a link flag and modifies it if necessary, i.e.
    in the link is -lcrypto, returns a static library path instead

    Examples:
        filter_link('-lblah')
        => '-lblah'
        filter_link('-lcrypto')
        => '/usr/lib/x86_64-linux-gnu/libcrypto.a'

    Returns the new gcc option
    """
    if a != "-lcrypto": return a
    result = []
    for pattern in ["/usr/lib*/libcrypto.a", "/usr/lib*/*/libcrypto.a"]:
        result += glob.glob(pattern)
    if len(result) > 0:
        return result[0]
    else:
        return a
def to_links(libs):
    """ Convert a library list to gcc link flags Prepends -l to a given list 

    Examples:

        to_links(['curl', 'au'])
        => ['-lcurl', '-lau']

    Returns the new converted list
    """
    #return [filter_link(a) for a in ['-l'+s for s in libs]]
    return [a for a in ['-l'+s for s in libs]]
def binary_exists(name):
    """ Checks if a binary exists (requires 'which' utility)
    Returns true if the binary exists
    """
    cmd = ["which",name]
    p = subprocess.Popen(cmd, stdout=subprocess.PIPE)
    res = p.stdout.readlines()
    if len(res) == 0:
        return False
    return True
def test():
    """ Compile, Run unit tests generating reports in out directory """
    run('rm', '-rf', 'out/*')
    for d in ["c", "python"]: run('mkdir', '-p', 'out/' + d)
    compile_test()
    test_run()
def build():
    """ Builds fuse_kafka binary """
    compile()
    link()
def run_c_test(source):
    bin_path = get_test_bin(source)
    batch = "-batch"
    if os.getenv("NO_BATCH") != None: batch = ""
    if os.path.exists(bin_path):
        gdb = "gdb " + batch + " -return-child-result \
                --eval-command=run --eval-command=where \
                --eval-command='info locals' --args "
        if os.getenv("NO_GDB") != None: gdb = ""
        result = os.system(gdb + "./" + bin_path)
        if result != 0:
            print("failed running {}, RC: {}".format(source, result))
            exit(result)
    else:
        print("warning: no binary test {}".format(bin_path))
def c_test():
    """ Builds, run unit tests, generating coverage reports in out directory """
    compile_test()
    for source in sources: run_c_test(source)
    for library_source in plugins.libraries_sources:
        run_c_test(plugins.test_of[library_source])
    tests = sources + map(
            lambda x: plugins.test_of[library_source],
            plugins.libraries_sources)
    run("gcov", ["src/" + x + ".c" for x in tests] ,"-o", ".")
    if binary_exists("lcov"):
        run("lcov", "--no-external", "--rc", "lcov_branch_coverage=1", "-c", "-d", ".", "-o", "./src/coverage.info")
        if binary_exists("genhtml"):
            run("genhtml", "--rc", "lcov_branch_coverage=1", 
                    "./src/coverage.info", "-o", "./out/c")
def python_test():
    run("python-coverage", "run", "src/fuse_kafka_test.py")
    run("find", "out")
    try:
        run("python-coverage", "html", "-d", "out/python")
    except:
        print("error while generating html coverage report")
def test_run():
    c_test()
    python_test()
def to_includes(what):
    return [os.popen("pkg-config --cflags " + a).read().split() for a in what]
def target_matched(target):
    compiler = cc[0]
    if type(cc) is str:
        compiler = cc
    cmd = "sh -c '" + compiler + " -v 2>&1|grep Target:|grep \"" + target + "\"'"
    return len(os.popen(cmd).read().split()) > 0
def compile_plugins():
    for library_source in plugins.libraries_sources:
        if not target_matched(plugins.targets_of[library_source]):
            print("skipping " + library_source + " plugin because not compiling for target")
            continue
        run(cc, '-g', '-c', '-fpic', '-I', 'src', to_includes(plugins.includes_of[library_source]), "./src/plugins/" + plugins.kind_of[library_source] + "/" + library_source +'.c', flags, '-o', plugins.objects[library_source])
        run(cc, '-shared', '-o', plugins.shareds_objects[library_source], plugins.objects[library_source], flags, to_links(plugins.libs_of[library_source]))
def compile():
    """ Compiles *.c files in source directory """
    compile_plugins()
    for source in sources:
        run(cc, '-g', '-c', "./src/" + source+'.c', flags)
def get_test_bin(source):
    return source.replace("/", "_") +'.test'
def compile_test_with_libs(source, libs, includes = []):
    """ Builds unit test binary """
    path = "./src/" + source +'.c'
    if not os.path.exists(path):
        print("warning: no test for {}".format(path))
    else: 
        run(cc, '-I', 'src', to_includes(includes), '-g', '-o', get_test_bin(source), path, flags,
                test_flags, to_links(libs))
def compile_test():
    """ Builds unit test binary """
    for source in sources:
        compile_test_with_libs(source, common_libs)
    for library_source in plugins.libraries_sources:
        compile_test_with_libs(plugins.test_of[library_source],
                plugins.libs_of[library_source], plugins.includes_of[library_source])
def link():
    """ Finalize the binary generation by linking all object files """
    objects = [s+'.o' for s in sources]
    run(cc, '-g', objects, '-o', binary_name, flags, to_links(libs))
def install():
    """ installs fuse kafka on current system, i.e. installs:
        - fuse_kafka binary in $BUILDROOT/usr/bin
        - fuse_kafka init script in $BUILDROOT/etc/init.d
        - fuse_kafka configuration $BUILDROOT/etc

    - BUILDROOT (environment variable): the target directory where to install fuse_kafka, 
        if not specified, will be filesystem root ('/')
    """
    root = '/'
    if os.environ.get('BUILDROOT') != None:
        root = os.environ.get('BUILDROOT') + "/"
    build()
    install_directory = root + 'usr/bin/'
    lib_directory = root + 'usr/lib/'
    init_directory = root + 'etc/init.d/'
    conf_directory = root + 'etc/'
    [run('mkdir', '-p', d) for d in
            [conf_directory, init_directory, install_directory, lib_directory]]
    for key in plugins.shareds_objects:
        if not target_matched(plugins.targets_of[key]): continue
        run('cp', plugins.shareds_objects[key], lib_directory)
    run('cp', binary_name, install_directory)
    [run('cp', 'src/' + init_name + '.py', init_directory + init_name)
            for init_name in ["fuse_kafka", "fuse_kafka_umounter"]]
    run('cp', 'conf/fuse_kafka.properties',
            conf_directory + "fuse_kafka.conf")
def install_from_source(root, src, source):
    path = src + source['name']
    print(">> in " + path)
    if not 'branch' in source: source['branch'] = "master"
    if not 'append' in source: source['append'] = ""
    if not 'ignore' in source: source['ignore'] = False 
    if not os.path.exists(path):
        run("git", "clone", "--single-branch", "--branch", source['branch'], source['url'], cwd = src)
    if 'pre' in source: 
        pre = source['pre']
        run(*pre['action'], cwd = src + pre['cwd'])
    wd = os.getcwd()
    os.chdir(path)
    if not os.path.exists("configure"): os.system("autoreconf -if")
    if os.system("./configure --prefix=" + root + " " + source['append']) != 0 or\
        (os.system("make") != 0 and not source['ignore']) or\
        os.system("make install") != 0:
            raise Exception("build failed for " + path)
    os.chdir(wd)
def install_dependencies():
    if os.environ.get('BUILDROOT') == None:
        print("no BUILDROOT specified")
        return
    if os.environ.get('SRCROOT') == None:
        os.environ['SRCROOT'] = "/tmp/fuse_kafka_src"
        return
    root = os.environ.get('BUILDROOT') + "/"
    src = os.environ.get('SRCROOT') + "/"
    for d in [root, src]: run("mkdir", "-p", d)
    for source in [
            {
                'name'  :   'zookeeper/src/c',
                'pre'   :   {'cwd': 'zookeeper', 'action':  ["ant", "compile_jute"]},
                'url'   :   'https://github.com/fuse-kafka/zookeeper.git',
                'branch':   'mingw',
                'append':    '--host=mingw32',
                },
            {
                'name'  :   'zlib',
                'url'   :   'https://github.com/fuse-kafka/zlib.git',
                },
            {
                'name'  :   'jansson',
                'url'   :   'https://github.com/akheron/jansson',
                'append':    '--host=mingw32',
                },
            {
                'name'  :   'dlfcn-win32',
                'url'   :   'https://github.com/dlfcn-win32/dlfcn-win32',
                'append':    '--cc=' + cc[0]
                },
            {
                'name'  :   'librdkafka',
                'url'   :   'https://github.com/yazgoo/librdkafka',
                'branch':   'win32',
                'ignore':   True,
                },
            ]: install_from_source(root, src, source)
    if target_matched("mingw"):
        for header in ["winconfig.h", "winstdint.h"]:
            run("cp", src + "/zookeeper/src/c/include/" + header, root + "/include/")
def binary_archive():
    if os.environ.get('BUILDROOT') == None:
        os.environ['BUILDROOT'] = "/tmp/" + binary_name + "_install"
    install_dependencies()
    install()
    root = (os.environ.get('BUILDROOT') + "/").replace("//", "/")
    main_bin = root + "/usr/bin/" + binary_name
    if target_matched("mingw"):
        run("cp", main_bin, main_bin + ".exe")
        for dll in ["/usr/x86_64-w64-mingw32/lib/libwinpthread-1.dll",
                "/usr/lib/gcc/x86_64-w64-mingw32/4.8/libgcc_s_sjlj-1.dll"]:
            run("cp", dll, root + "/usr/lib/")
    name = binary_name + "-" + get_version()
    package_name = "../" + name + "-bin"
    tar = package_name + ".tar.gz"
    if target_matched("mingw"):
        _zip = "IronPython-2.7.5.zip"
        run("wget", "http://download-codeplex.sec.s-msft.com/Download/Release?ProjectName=ironpython&DownloadId=970326&FileTime=130623736032830000&Build=21024", "-O", _zip)
        run("unzip", "-o", _zip, "-d", root)
    run("tar", "czf", tar, root,  "-C", root)
    run("zip", "-r", os.getcwd() + "/" + package_name + ".zip", os.path.basename(os.path.dirname(root)), cwd = root + "/..")
def clean():
    """ Cleanups file generated by this script """
    autoclean()
def kafka_download():
    """ Downloads kafka binary distribution archive and uncompresses it """
    if not os.path.exists(kafka_archive):
        os.system('wget ' + kafka_server + kafka_version + "/" + kafka_archive)
    if not os.path.exists(kafka_directory):
        os.system('tar xzf ' + kafka_archive)
def zookeeper_start():
    """ Does kafka_dowload() and starts zookeeper server """
    kafka_download()
    run(kafka_bin_directory + 'zookeeper-server-start.sh',
            kafka_config_directory + 'zookeeper.properties')
def bench():
    Benchmarks().generate()
def kafka_start():
    """ Does kafka_dowload() and starts kafka server """
    kafka_download()
    run(kafka_bin_directory + 'kafka-server-start.sh',
            kafka_config_directory + 'server.properties')
def kafka_consumer_start():
    """ Starts a kafka logstash/fuse_kafka events consumer
        (pretty printing events)
    
    - zkconnect (environment variable): if specified, launches the kafka
        consumer based on this zookeeper cluster address, otherwise looks for a 
        zookeeper on localhost
    """
    zkconnect = os.environ.get('zkconnect')
    if zkconnect == None: zkconnect = "localhost"
    FuseKafkaLog(zkconnect).start()
def create_topic_command(zkconnect):
    """
    Return a command line for creating a new logging topic on kafka cluster

    - zkconnect: the zookeeper cluster endpoint to kafka
    """
    return kafka_bin_directory + 'kafka-topics.sh --create --topic logs --zookeeper {} --partitions 2 --replication-factor 1'.format(zkconnect)
def wait_for_input():
    try:
        raw_input(">")
    except:
        print("done")
fuse_kafka_input_line = "fuse_kafka_input=[\"{0}\"]"
overlay_line = fuse_kafka_input_line.format("overlay")
inotify_line = fuse_kafka_input_line.format("inotify")
def ruby_write_tests():
    print(">> launching overlay write")
    cmd = "ruby write_tests.rb /tmp/write_tests."
    os.system('cat ./conf/fuse_kafka.properties')
    os.system(cmd + "overlay")
    comment_conf(overlay_line)
    uncomment_conf(inotify_line)
    os.system('./src/fuse_kafka.py restart')
    print(">> launching inotify write")
    os.system('cat ./conf/fuse_kafka.properties')
    os.system(cmd + "inotify")
def comment_conf(what, first = "", second = "#"):
    os.system("sed -i 's/^{}{}$/{}{}/' conf/fuse_kafka.properties".format(
        first, what.replace("[", ".").replace("]", "."), second, what))
def uncomment_conf(what):
    comment_conf(what, first = "#", second = "")
def write_tests():
    """ Launches kafka, zookeeper, fuse_kafka """
    uncomment_conf(overlay_line)
    comment_conf(inotify_line)
    quickstart(consumer = False, synchronous_action = "ruby_write_tests")
def quickstart(consumer = True, synchronous_action = "wait_for_input"):
    """ Launches kafka, zookeeper, fuse_kafka and a console consumer locally """
    kafka_download()
    klog = '/tmp/kafka.log'
    zlog = '/tmp/zookeeper.log'
    if os.path.exists(klog): shutil.rmtree(klog)
    if os.path.exists(zlog): shutil.rmtree(zlog)
    p1 = multiprocessing.Process(target=zookeeper_start, args=())
    p2 = multiprocessing.Process(target=kafka_start, args=())
    p3 = None
    if consumer: p3 = multiprocessing.Process(target=kafka_consumer_start, args=())
    p1.start()
    p2.start()
    result = 1
    while result != 0:
        result = os.system(create_topic_command('localhost'))
        time.sleep(0.2)
    os.system('./src/fuse_kafka.py start')
    if consumer: p3.start()
    try:
        f = globals()[synchronous_action]
        if f == None:
            f = locals()[synchronous_action]
            if f != None:
                f()
        else:
            f()
    except e:
        print(e)
    p1.terminate()
    p2.terminate()
    if consumer:
        p3.terminate()
        os.system('pkill -9 -f java.*kafka.consumer.ConsoleConsumer')
    os.system('./src/fuse_kafka.py stop')
    os.system(kafka_bin_directory + 'kafka-server-stop.sh')
    os.system(kafka_bin_directory + 'zookeeper-server-stop.sh')
def doc():
    """ generates the project documentation """
    run('mkdir', '-p', 'doc')
    run("doxygen", "Doxyfile")
class TestMininet(unittest.TestCase):
    """ Utility to create a virtual network to test fuse kafka resiliancy """
    def impersonate(self, inital_user = True):
        """ changes effective group and user ids """
        uid = gid = None
        if inital_user:
            uid = os.getuid()
            gid = os.getuid()
        else:
            stat = os.stat(".")
            uid = stat.st_uid
            gid = stat.st_gid
        os.setegid(gid)
        os.seteuid(uid)
    def start_network(self):
        """ starts-up a single switch topology """
        from mininet.topo import Topo
        from mininet.net import Mininet
        from mininet.node import OVSController
        class SingleSwitchTopo(Topo):
            "Single Switch Topology"
            def __init__(self, count=1, **params):
                Topo.__init__(self, **params)
                hosts = [ self.addHost('h%d' % i) for i in range(1, count + 1) ]
                s1 = self.addSwitch('s1')
                for h in hosts:
                    self.addLink(h, s1)
        self.net = Mininet(topo = SingleSwitchTopo(4), controller = OVSController)
        self.net.start()
        self.impersonate(False)
    def log_path(self, name):
        return "/tmp/{}.log".format(name)
    def shell(self):
        """ launches mininet CLI """
        from mininet.cli import CLI
        CLI(self.net)
    def clients_initialize(self):
        """ initializes clients variables based on hosts """
        self.kafka = self.net.get('h1')
        self.zookeeper = self.net.get('h2')
        self.fuse_kafka = self.net.get('h3')
        self.client = self.net.get('h4')
        self.hosts = [self.kafka, self.zookeeper, self.fuse_kafka, self.client]
        self.switch = self.net.get('s1')
        self.java_clients = [self.client, self.kafka, self.zookeeper]
    def cmd(self, where, cmd):
        import pwd
        command = "su {} -c '{}'".format(
                pwd.getpwuid(os.stat(".").st_uid).pw_name, cmd)
        print(command)
        return where.cmd(command)
    def data_directories_cleanup(self):
        """ cleanups generated directory """
        self.cmd(self.zookeeper, "rm -rf /tmp/kafka-logs /tmp/zookeeper")
    def zookeeper_start(self):
        """ starts zookeeper server """
        self.cmd(self.zookeeper, self.launch.format("zookeeper") 
            + kafka_config_directory
            + 'zookeeper.properties >> {} 2>&1 &'.format(self.log_path('zookeeper')))
    def kafka_start(self):
        """ starts kafka server and creates logging topic """
        import tempfile
        if not hasattr(self, 'kafka_config'):
            self.kafka_config = tempfile.NamedTemporaryFile(delete=False)
            self.kafka_config.write("zookeeper.connect={}\n".format(self.zookeeper.IP()))
            self.kafka_config.write("broker.id=0\n")
            self.kafka_config.write("host.name={}\n".format(self.kafka.IP()))
            self.kafka_config.close()
        self.cmd(self.kafka, self.launch.format("kafka")
                + self.kafka_config.name + ' > {} 2>&1 &'.format(self.log_path('kafka')))
        self.cmd(self.kafka, create_topic_command(
            self.zookeeper.IP()) + " > {} 2>&1 ".format(self.log_path('create_topic')))
    def kafka_stop(self):
        """ stops kafka server """
        self.cmd(self.kafka, self.stop.format("kafka"))
    def zookeeper_stop(self):
        """ stops zookeeper server """
        self.cmd(self.zookeeper, "pkill -9 -f zookeeper.properties")
    def fuse_kafka_start(self):
        """ starts fuse_kafka """
        cwd = os.getcwd() + "/"
        self.fuse_kafka_path = '{}/fuse_kafka'.format(cwd)
        conf = "/tmp/conf"
        self.cmd(self.fuse_kafka, "mkdir -p {}".format(conf))
        self.cmd(self.fuse_kafka, "cp {}conf/fuse_kafka.properties {}".format(cwd, conf))
        self.cmd(self.fuse_kafka, "sed -i 's/127.0.0.1/{}/' {}/fuse_kafka.properties"
                .format(self.zookeeper.IP(), conf))
        self.cmd(self.fuse_kafka, "ln -s {}/fuse_kafka {}/../fuse_kafka"
                .format(cwd, conf))
        for path in glob.glob(cwd + "/*.so"):
            self.cmd(self.fuse_kafka, "ln -s {} {}/../{}"
                    .format(path, conf, path.split("/")[-1]))
        self.cmd(self.fuse_kafka, "ln -s {}/fuse_kafka {}/../fuse_kafka"
                .format(cwd, conf))
        self.cmd(self.fuse_kafka, 'bash -c "cd {}/..;{}src/fuse_kafka.py start > {} 2>&1"'
                .format(conf, cwd, self.log_path('fuse_kafka')))
    def consumer_start(self):
        """ starts fuse_kafka consumer """
        if os.path.exists(self.log_path('consumer')):
            os.remove(self.log_path('consumer'))
        command = os.getcwd() + "/" + kafka_bin_directory
        command += "kafka-console-consumer.sh --zookeeper "
        command += self.zookeeper.IP() + " --topic logs"
        print(command)
        self.impersonate() # popen require setns()
        self.consumer = self.client.popen(command)
        self.impersonate(False)
    def tearDown(self):
        """ stops fuse_kafka, zookeeper, kafka, cleans their working directory and 
        stops the virtual topology """
        for host in self.java_clients: self.cmd(host, 'pkill -9 java') 
        self.consumer.kill()
        self.cmd(self.fuse_kafka, 'src/fuse_kafka.py stop')
        os.remove(self.kafka_config.name)
        self.data_directories_cleanup()
        self.impersonate()
        self.net.stop()
    def setUp(self):
        """ starts the topology, downloads kafka, does a data directory
        cleanup in case of previous run """
        self.launch = kafka_bin_directory + '{}-server-start.sh '
        self.stop = kafka_bin_directory + '{}-server-stop.sh '
        self.start_network()
        kafka_download()
        self.clients_initialize()
        self.data_directories_cleanup()
        self.components_start()
        # wait for fuse-kafka to be ready
        time.sleep(2)
    def check(self):
        self.assertTrue(os.path.exists(self.fuse_kafka_path),
            "you must build fuse kafka to run tests")
        os.stat("/tmp/fuse-kafka-test")
    def get_consumed_events(self, expected_number):
        from mininet.util import pmonitor
        events = []
        log = FuseKafkaLog()
        popens = {}
        popens[self.client] = self.consumer
        for host, line in pmonitor(popens):
            self.consumer.poll()
            events.append(log.load_fuse_kafka_event(line))
            if len(events) >= expected_number:
                break
        self.assertEqual(expected_number, len(events))
        return events
    def write_to_log(self, what = "test"):
        self.cmd(self.fuse_kafka, "echo -n {} > /tmp/fuse-kafka-test/xd 2>&1".format(what))
    def components_start(self):
        """ starts zookeepre, kafka, fuse_kafka, fuse_kafka consumer """
        self.zookeeper_start()
        self.kafka_start()
        self.fuse_kafka_start()
        self.consumer_start()
    def test_basic(self):
        """ runs the topology with a mininet shell """
        self.check()
        for message in ["hello", "world"]:
            self.write_to_log(message)
            events = self.get_consumed_events(1)
            self.assertEqual(message, events[0]["@message"])
        expected = ["foo", "bar"]
        for message in expected:
            self.write_to_log(message)
        actual = [event["@message"] for event in self.get_consumed_events(2)]
        self.assertEqual(sorted(expected), sorted(actual))
    def test_shutting_down_kafka(self):
        self.check()
        self.kafka_stop()
        self.write_to_log()
        self.kafka_start()
        self.get_consumed_events(1)
    def test_shutting_down_zookeeper(self):
        self.check()
        self.zookeeper_stop()
        self.write_to_log()
        self.zookeeper_start()
        self.get_consumed_events(1)
    def test_bringing_down_kafka(self):
        self.check()
        self.kafka_stop()
        self.write_to_log()
        self.kafka_start()
        self.get_consumed_events(1)
    def test_cutting_kafka(self):
        self.check()
        self.write_to_log()
        self.net.configLinkStatus(self.kafka.name, self.switch.name, "down") 
        self.assertRaises(ValueError, self.get_consumed_events, (1))
        self.net.configLinkStatus(self.kafka.name, self.switch.name, "up") 
        self.get_consumed_events(1)
    def test_cutting_zookeeper(self):
        self.check()
        self.write_to_log()
        self.net.configLinkStatus(self.zookeeper.name, self.switch.name, "down") 
        # zookeeper being brought down should not influence an already launched producer
        self.get_consumed_events(1)
    def test_cutting_kafka_periodically(self):
        self.check()
        ranges = {10: range(3), 1: range(4), 0: range(10)}
        for sleep_time in ranges:
            print("sleep time: " + str(sleep_time))
            for i in ranges[sleep_time]:
                print("loop # " + str(i))
                self.net.configLinkStatus(self.kafka.name, self.switch.name, "down") 
                time.sleep(sleep_time)
                self.assertRaises(ValueError, self.get_consumed_events, (1))
                self.net.configLinkStatus(self.kafka.name, self.switch.name, "up") 
                if sleep_time > 1:
                    time.sleep(7) # wait for kafka to be restarted
                self.write_to_log()
                self.get_consumed_events(1)
if __name__ == "__main__":
    if len(sys.argv) <= 1 or not (sys.argv[1] in ["quickstart", "mininet", "bench", "multiple"]):
        main()
    else:
        if sys.argv[1] == "multiple":
            for arg in sys.argv[2:]:
                locals()[arg]()
        elif sys.argv[1] == "quickstart": quickstart()
        elif sys.argv[1] == "bench": bench()
        else:
            sys.argv.pop(0)
            unittest.main()
