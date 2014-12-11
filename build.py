#!/usr/bin/env python
try:
    import base64, subprocess, sys, glob, os, json, thread, multiprocessing, shutil, time
except ImportError, e:
    print "failed importing module", e
from fabricate import *
sources = ['fuse_kafka']
binary_name = sources[0]
common_libs = ["crypto", "fuse", "dl", "pthread", "jansson"]#, "ulockmgr"]
libs = ["zookeeper_mt", "rdkafka",  "z", "rt"] + common_libs
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
    def __init__(self, zkconnect = "localhost"):
        self.select = None
        self.zkconnect = zkconnect
    def run_command(self, *command):
        p = subprocess.Popen(command,
                stdout=subprocess.PIPE,
                stderr=subprocess.STDOUT)
        #return iter(p.stdout.readline, b'')
        return iter(p.stdout.readline, '')
    def pretty_print(self, string):
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
        event = json.loads(string)
        for item in ["@message", "command"]:
            event[item] += "=" * ((4 - len(event[item]) % 4) % 4)
            event[item] = base64.b64decode(event[item])
        event["message_size-added"] = len(event["@message"])
        return event
    def start(self):
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
    f = open("src/version.h")
    result = []
    while True:
        result = f.readline().split()
        if len(result) == 3 and result[0] == "#define" and result[1] == "VERSION":
            break;
    result = result[-1][1:-1]
    f.close()
    return result
def bump_version():
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
    print(get_version())
def package():
    name = binary_name + "-" + get_version()
    tar = "../" + name + ".tar.gz"
    run("tar", "--transform", "s,^.," + name + ",",
            "--exclude=.git", 
            "--exclude=.nfs*",
            "--exclude=out", "-czf", tar , ".")
def filter_link(a):
    if a != "-lcrypto": return a
    result = []
    for pattern in ["/usr/lib*/libcrypto.a", "/usr/lib*/*/libcrypto.a"]:
        result += glob.glob(pattern)
    if len(result) > 0:
        return result[0]
    else:
        return a
def to_links(libs):
    return [filter_link(a) for a in ['-l'+s for s in libs]]
def binary_exists(name):
    cmd = ["which",name]
    p = subprocess.Popen(cmd, stdout=subprocess.PIPE)
    res = p.stdout.readlines()
    if len(res) == 0:
        return False
    return True
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
        run("./" + source + ".test")
        run("gcov", "./src/" + source + ".c","-o", ".")
        if binary_exists("lcov"):
            run("lcov", "-c", "-d", ".", "-o", "./src/" + source + ".info")
            if binary_exists("genhtml"):
                run("genhtml", "src/" + source + ".info", "-o", "./out")
def compile():
    for source in sources:
        run('gcc', '-g', '-c', "./src/" + source+'.c', flags)
def compile_test():
    for source in sources:
        run('gcc', '-g', '-o', source+'.test', "./src/" + source+'.c', flags,
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
    zkconnect = os.environ.get('zkconnect')
    if zkconnect == None: zkconnect = "localhost"
    FuseKafkaLog(zkconnect).start()
def create_topic_command(zkconnect):
    return kafka_bin_directory + 'kafka-topics.sh --create --topic logs --zookeeper {} --partitions 1 --replication-factor 1'.format(zkconnect)
def quickstart():
    klog = '/tmp/kafka-logs'
    zlog = '/tmp/zookeeper'
    if os.path.exists(klog): shutil.rmtree(klog)
    if os.path.exists(zlog): shutil.rmtree(zlog)
    p1 = multiprocessing.Process(target=zookeeper_start, args=())
    p2 = multiprocessing.Process(target=kafka_start, args=())
    p3 = multiprocessing.Process(target=kafka_consumer_start, args=())
    p1.start()
    p2.start()
    result = 1
    while result != 0:
        result = os.system(create_topic_command('localhost'))
        time.sleep(0.2)
    os.system('./src/fuse_kafka.py start')
    p3.start()
    try:
        raw_input(">")
    except:
        print("done")
    p1.terminate()
    p2.terminate()
    p3.terminate()
    os.system('pkill -9 -f java.*kafka.consumer.ConsoleConsumer')
    os.system('./src/fuse_kafka.py stop')
    os.system(kafka_bin_directory + 'kafka-server-stop.sh')
    os.system(kafka_bin_directory + 'zookeeper-server-stop.sh')
def doc():
    run('mkdir', '-p', 'doc')
    run("doxygen", "Doxyfile")
class TestMininet():
    def __init__(self):
        import tempfile
        from mininet.topo import Topo
        from mininet.net import Mininet
        from mininet.cli import CLI
        from mininet.node import OVSController
        class SingleSwitchTopo(Topo):
            "Single Switch Topology"
            def __init__(self, count=1, **params):
                Topo.__init__(self, **params)
                hosts = [ self.addHost('h%d' % i) for i in range(1, count + 1) ]
                s1 = self.addSwitch('s1')
                for h in hosts: self.addLink(h, s1)
        net = Mininet(topo = SingleSwitchTopo(4), controller = OVSController)
        net.start()
        kafka_download()
        kafka = net.get('h1')
        zookeeper = net.get('h2')
        fuse_kafka = net.get('h3')
        client = net.get('h4')
        launch = kafka_bin_directory + '{}-server-start.sh '
        zookeeper.cmd("rm -rf /tmp/zookeeper")
        zookeeper.cmd(launch.format("zookeeper") 
            + kafka_config_directory + 'zookeeper.properties > /tmp/zookeeper.log &')
        kafka_config = tempfile.NamedTemporaryFile(delete=False)
        kafka_config.write("zookeeper.connect={}\n".format(zookeeper.IP()))
        kafka_config.write("broker.id=0\n")
        kafka_config.write("host.name={}\n".format(kafka.IP()))
        kafka_config.close()
        zookeeper.cmd("rm -rf /tmp/kafka-logs")
        kafka.cmd(launch.format("kafka") + kafka_config.name + ' > /tmp/kafka.log &')
        cwd = os.getcwd() + "/"
        conf = "/tmp/conf"
        fuse_kafka.cmd("mkdir -p {}".format(conf))
        fuse_kafka.cmd("cp {}conf/fuse_kafka.properties {}".format(cwd, conf))
        fuse_kafka.cmd("sed -i 's/127.0.0.1/{}/' {}/fuse_kafka.properties"
                .format(zookeeper.IP(), conf))
        fuse_kafka.cmd("ln -s {}/fuse_kafka {}/../fuse_kafka".format(cwd, conf))
        result = kafka.cmd(create_topic_command(zookeeper.IP()) + " > /tmp/create_topic.log")
        print result
        client.cmd("zkconnect={} ./build.py kafka_consumer_start > /tmp/kafka_consumer.log &"
                .format(zookeeper.IP()));
        fuse_kafka.cmd('bash -c "cd {}/..;{}src/fuse_kafka.py start"'.format(conf, cwd))
        CLI(net)
        fuse_kafka.cmd('src/fuse_kafka.py stop')
        [ host.cmd('pkill -9 java') for host in [client, kafka, zookeeper]]
        net.stop()
def mininet():
    TestMininet()
if len(sys.argv) <= 1 or (sys.argv[1] != "quickstart" and sys.argv[1] != "mininet"):
    main()
else:
    if sys.argv[1] == "quickstart": quickstart()
    else: mininet()
