import unittest, subprocess
from fuse_kafka import *
class Stub:
    def __init__(*args, **kwargs):
        print(args, kwargs)
class PopenStub(Stub):
    def communicate(self, *args, **kwargs):
        return [""]
class ProcessStub(Stub):
    def start(self):
        print("start")
    def join(self, time = 0):
        print("join")
    def terminate(self):
        print("join")
    def is_alive(self):
        return True
def nothing_stub(what):
    print(what)
def return_stub(*args):
    return return_value
class TestFuseKafkaInit(unittest.TestCase):
    def setUp(self):
        subprocess.Popen = PopenStub
        multiprocessing.Process = ProcessStub
        subprocess.call = sys.exit = time.sleep = nothing_stub 
    def test_crontab(self):
        crontab = Crontab()
        crontab.add_line_if_necessary("blah")
    def test_mountpoints(self):
        mountpoint = Mountpoints()
        self.assertFalse(mountpoint.access("/tmp"))
        mountpoint.umount_non_accessible()
    def test_configuration(self):
        configuration = Configuration()
        configuration.get_property("conf/fuse_kafka.properties",
                "fuse_kafka_quota")
        self.assertTrue(configuration.includes_subdir(["/"], "/tmp"))
        self.assertEqual(["/"], configuration.exclude_directories(["/"], ["/tmp"]))
        configuration.load()
        self.assertFalse(configuration.is_sleeping())
        args = ['--tags', 'test', '--fields', 'hostname', 'test',
                '--directories', '/tmp/fuse-kafka-test', '--quota',
                '512', '--topic', 'logs', '--zookeepers', '127.0.0.1:2181']
        self.assertEqual(args, configuration.args())
        self.assertEqual(" ".join(args), str(configuration))
    def test_fuse_kafka_service(self):
        os.environ['FUSE_KAFKA_PREFIX'] = '/'
        service = FuseKafkaService()
        service.get_status()
        service.status()
        service.start()
        service.stop()
        service.cleanup()
        service.restart()
        service.do("status")
if __name__ == '__main__':
    unittest.main()
