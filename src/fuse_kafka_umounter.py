#!/usr/bin/python
# chkconfig: 2345 11 89
### BEGIN INIT INFO
# Provides: fuse_kafka_umounter
# Required-Start:
# Required-Stop:
# Default-Start:  3 5    
# Default-Stop:
# Short-Description: run fuse_kafka_umounter
# Description:
### END INIT INFO
""" @package fuse_kafka_umounter
Startup script for fuse_kafka_umounter.
"""
import subprocess, os, time, sys, atexit, signal
sys.path.append(os.path.dirname(os.path.realpath(__file__)))
from fuse_kafka import Mountpoints
class Pid:
    """ Manages a pid file """
    def __init__(self, name):
        """
        name - the name with which the pid file will be saved
        """
        self.pid_path = "/var/run/" + str(name) + ".pid"
        if not os.access(self.pid_path, os.W_OK):
            self.pid_path = "/tmp/" + str(name) + ".pid"
    def is_running(self):
        """ Returns true if the pid file exists """
        return os.path.isfile(self.pid_path)
    def setup(self, pid):
        """ If pid is null (should be in fork child), setups the 
        teardown method, overwise creates the pid file.

        pid - the pid to save to pid file

        Returns the pid number given
        """
        if pid != 0:
            with open(self.pid_path, "w") as f:
                f.write(str(pid) + "\n")
        else:
            atexit.register(self.teardown, None, None)
            signal.signal(signal.SIGTERM, self.teardown)
        return pid
    def teardown(self, signo, frame):
        """ removes the pid file """
        os.remove(self.pid_path)
    def get(self):
        """ the pid number from the pid file, None if it does not exist """
        try:
            with open(self.pid_path) as f:
                return int(f.read().strip())
        except IOError:
            return None
class Service:
    """ Manages a service """
    def __init__(self):
        self.pid = Pid(self.__class__)
    def start(self):
        """ starts the service in a subprocess if it is not
        running yet, creating a pid file """
        if self.pid.is_running(): return
        if self.pid.setup(os.fork()) > 0: return
        self.run()
    def stop(self):
        """ stops the service, destroying the pid file """
        pid = self.pid.get()
        if pid != None:
            subprocess.call(['kill', str(pid)])
    def status(self):
        """ Returns the process status, and prints a status message """
        pid = self.pid.get()
        message = "service is"
        ret = 0
        if pid == None:
            message += " not"
            ret = 1
        print(message + " running.")
        return ret
    def do(self, action):
        """ run an action """
        return getattr(self, action)()
class FuseKafkaUmounter(Service):
    """ every minute, check that all 
    mountpoints are accessible, and umount non accessibles  """
    def __init__(self, forever = True):
        self.forever = forever
        Service.__init__(self)
    def run(self):
        while self.forever:
            Mountpoints().umount_non_accessible()
            time.sleep(60)
if __name__ == "__main__":
    FuseKafkaUmounter().do(sys.argv[1])
