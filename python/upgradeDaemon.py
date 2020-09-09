#/usr/bin/python -u
import sys, os, time, atexit, string
from signal import SIGTERM
import logging
import subprocess
import click
from upgrade import *

UPGRADE_DIAGNOSE_PATH = "/home/admin/upgradeDiagnose.txt"
UPGRADE_DAEMON_PID = "/tmp/upgradeDaemon.pid"
UPGRADE_DAEMON_LOG = "/tmp/upgradeDaemon.log"
UPGRADE_DIAGNOSE_RESULTS = "/home/admin/diagnose_results_path.txt"
SLEEP_TIME = 60     # unit: 1 second
CUTTING_LINE = "    --------------------------------------------------------------"

# ========================== Syslog wrappers ==========================
def log_debug(msg):
    logging.debug(msg)
 
def log_info(msg):
    logging.info(msg)

def log_warn(msg):
    logging.warning(msg)

def log_error(msg):
    logging.error(msg)

# Run bash command and print output to stdout
def run_command(self, command):
    sys.stdout.write('Command: {}\n'.format(command))
    proc = subprocess.Popen(command, shell=True, stdout=subprocess.PIPE)
    (out, _) = proc.communicate()
    sys.stdout.write(out + "\n")
    sys.stdout.flush()

    if proc.returncode != 0:
        sys.exit(proc.returncode)

class Daemon:
    def __init__(self, log_lever="DEBUG", stdin='/dev/null', stdout='/dev/null', stderr='/dev/null'):
        """ if you need to get debugging information, change to stdin='/dev/stdin', 
        stdout='/dev/stdout', stderr='/dev/stderr', Run as root
        """
        self.stdin = stdin
        self.stdout = stdout
        self.stderr = stderr
        self.pidfile = UPGRADE_DAEMON_PID
        self.diagnoseFile = UPGRADE_DIAGNOSE_PATH    # /home/admin/upgradeDiagnose.txt
        self.time_cnt = 30    # total_time: 30 * 60s
        self.upgradeType = ""
        self.pre_version = ""
        self.container_name = ""
        self.container_backup_path = ""
        self.deb_name = ""
        self.owner_of_deb = ""
        self.image_latest = ""

        # initial log module
        self.lever = 1
        if "DEBUG" in log_lever:
            self.lever = logging.DEBUG
        elif "INFO" in log_lever:
            self.lever = logging.INFO
        elif "WARNING" in log_lever:
            self.lever = logging.WARNING
        else:
            self.lever = logging.DEBUG

        # logging.basicConfig(filename=UPGRADE_DAEMON_LOG, \
        #     format='%(asctime)s - %(pathname)s[line:%(lineno)d] - %(levelname)s: %(message)s', level=self.lever)
        logging.basicConfig(filename=UPGRADE_DAEMON_LOG, \
            format='%(asctime)s - %(pathname)s - %(levelname)s: %(message)s', level=self.lever)
        
        self.diagnose = upgrade_diagnose()
  
    def _daemonize(self):
        try:
            pid = os.fork()    #the first time fork, Create child process, detach from parent process
            if pid > 0:
                sys.exit(0)      # exit main process 
        except OSError, e:
            sys.stderr.write('fork #1 failed: %d (%s)\n' % (e.errno, e.strerror))
            sys.exit(1)
    
        os.chdir("/")      # Modify working directory
        os.setsid()        # Set up a new session connection
        os.umask(0)        # Reset the file creation permissions
    
        try:
            pid = os.fork() #the second time fork, Disables the process from opening the terminal
            if pid > 0:
                sys.exit(0)
        except OSError, e:
            sys.stderr.write('fork #2 failed: %d (%s)\n' % (e.errno, e.strerror))
            sys.exit(1)
    
        # Redirect file descriptors
        sys.stdout.flush()
        sys.stderr.flush()
        si = file(self.stdin, 'r')
        so = file(self.stdout, 'a+')
        se = file(self.stderr, 'a+', 0)
        os.dup2(si.fileno(), sys.stdin.fileno())
        os.dup2(so.fileno(), sys.stdout.fileno())
        os.dup2(se.fileno(), sys.stderr.fileno())
    
        # Register the exit function and determine whether there is a process based on the file PID
        atexit.register(self.delpid)
        pid = str(os.getpid())
        file(self.pidfile, 'w+').write('%s\n' % pid)
  
    def delpid(self):
        os.remove(self.pidfile)
 
    def start(self):
        """ Check the PID file for existence to detect the presence of a process """
        try:
            pf = file(self.pidfile,'r')
            pid = int(pf.read().strip())
            pf.close()
        except IOError:
            pid = None
    
        if pid:
            message = 'pidfile %s already exist. Daemon already running!\n'
            sys.stderr.write(message % self.pidfile)
            log_error("pidfile '{}' already exist. Daemon already running!".format(self.pidfile))
            sys.exit(1)
        
        # Start the monitoring
        self._daemonize()
        log_info("start the diagnose process after the upgrade ...")
        self._run()
 
    def stop(self):
        # Get the PID from the PID file
        try:
            pf = file(self.pidfile,'r')
            pid = int(pf.read().strip())
            pf.close()
        except IOError:
            pid = None
    
        if not pid:   # Restart without error
            message = 'pidfile %s does not exist. Daemon not running!\n'
            sys.stderr.write(message % self.pidfile)
            log_error("pidfile '{}' does not exist. Daemon not running!".format(self.pidfile))
            return
    
        # kill process
        try:
            while 1:
                log_info("stop the diagnose process ...\n")
                if os.path.exists(self.pidfile):
                    os.remove(self.pidfile)

                os.kill(pid, SIGTERM)
                time.sleep(0.1)
        except OSError, err:
            err = str(err)
            if err.find('No such process') > 0:
                log_warn("No such process")
                if os.path.exists(self.pidfile):
                    os.remove(self.pidfile)
            else:
                log_error(str(err))
                sys.exit(1)
 
    def restart(self):
        self.stop()
        self.start()

    def _run(self):
        """ run your fun"""
        self.parse_info()
        running = True

        while running:
            # log_debug("{}:hello world".format(time.ctime()))
            if self.time_cnt > 0:
                if self.handle_check():
                    running = False
                    break

                log_debug("time_cnt: {}".format(self.time_cnt))
                self.time_cnt -= 1
                time.sleep(SLEEP_TIME)
            else:
                running = False
                log_info("It had taken 300 minutes, it will force a fallback to the previous version ...")
                self.handle_force_rollback()
            
        self.stop()

    def parse_info(self):
        try:
            fileObj = open(self.diagnoseFile, "r")
        except IOError:
            log_error("'{}' does not exist and it is not necessary to start \
                the monitoring process.\n".format(self.diagnoseFile))
            self.stop()
        else:
            for line in fileObj:
                if line.startswith("upgradeType:"):
                    self.upgradeType = line.split(":")[1].replace("\n", "")
                elif line.startswith("pre_version"):
                    self.pre_version = line.split(":")[1].replace("\n", "")
                elif line.startswith("container_name"):
                    self.container_name = line.split(":")[1].replace("\n", "")
                elif line.startswith("container_backup_path"):
                    self.container_backup_path = line.split(":")[1].replace("\n", "")
                elif line.startswith("image_latest"):
                    self.image_latest = line.split(":")[1].replace("\n", "")
                elif line.startswith("deb_name"):
                    self.deb_name = line.split(":")[1].replace("\n", "")
                elif line.startswith("owner_of_deb"):
                    self.owner_of_deb = line.split(":")[1].replace("\n", "")
        
            fileObj.close()
            log_debug("parse the '{}' successfully.".format(self.diagnoseFile))
            os.remove(self.diagnoseFile)

    def handle_check(self):
        log_info("upgradeType: '{}'".format(self.upgradeType))
        if "system" in self.upgradeType:
            return self.diagnose.system_diagnose(self.pre_version)
        elif "container" in self.upgradeType:
            return self.diagnose.container_diagnose(self.container_name, self.container_backup_path, self.image_latest)
        elif "deb" in self.upgradeType:
            return self.diagnose.deb_diagnose(self.deb_name, self.owner_of_deb)
        else:
            return True

    def handle_force_rollback(self):
        log_info("upgradeType: '{}'".format(self.upgradeType))
        if "system" in self.upgradeType:
            return self.diagnose.rollback_system(self.pre_version)
        elif "container" in self.upgradeType:
            return self.diagnose.rollback_container(self.container_name, self.container_backup_path, self.image_latest)
        elif "deb" in self.upgradeType:
            return self.diagnose.rollback_container_after_upgrade_deb(self.owner_of_deb)
        else:
            return True

class upgrade_diagnose:
    def __init__(self, diagnose_results_path=UPGRADE_DIAGNOSE_RESULTS):
        # self.results_path = diagnose_results_path
        self.procs_table = {"lldp": ["lldpd", "lldp_syncd", "lldpmgrd"],  \
            "swss": ["orchagent", "portsyncd", "neighsyncd", "vrfmgrd", "vlanmgrd", "intfmgrd", "portmgrd", "subportmgrd", \
                "bridgemgrd", "buffermgrd", "nbrmgrd", "vxlanmgrd", "aclmgrd", "coppmgrd", "cfmmgrd", "nqamgrd", "mirrormgrd"], \
                    "snmp": ["snmpd", "python3.6"], \
                        "syncd": ["syncd"], \
                            "bgp": ["fpmsyncd", "zebra", "bgpd", "ldpd", "bfdd"]}
                    
        self.container_names = ["lldp", "swss", "syncd", "snmp", "bgp"]
        self.bootloader = Bootloader()

    def get_docker_tag_name(self, image):
        cmd = "docker inspect --format '{{.ContainerConfig.Labels.Tag}}' " + image
        proc = subprocess.Popen(cmd, stdout=subprocess.PIPE, shell=True)
        (out, err) = proc.communicate()
        if proc.returncode != 0:
            return "unknown"

        tag = out.rstrip()
        if tag == "<no value>":
            return "unknown"
        return tag

    def rollback_container(self, container_name="", container_backup_path="", image_latest=""):
        if not container_backup_path:
            log_error("No backup related container.")
            return True

        log_info("container '{}' will rollback to the previous container.".format(container_name))
        image_name = image_latest.split(":")[0]        # example image_name: docker-lldp-sv2
        run_command("systemctl stop %s" % container_name)
        run_command("docker rm %s " % container_name)
        run_command("docker rmi %s " % image_latest)
        run_command("docker load < %s" % container_backup_path)

        tag = self.get_docker_tag_name(image_latest)        # example: HEAD.494-ctc-20200902.003331
        run_command("docker tag %s:latest %s:%s" % (image_name, image_name, tag))
        run_command("systemctl restart %s" % container_name)
        run_command("sleep 5") # wait 5 seconds for application to sync
        return True

    def rollback_container_after_upgrade_deb(self, container_name=""):
        log_info("upgrade-deb: '{}' will rollback to the previous container.".format(container_name))
        run_command("systemctl stop %s" % container_name)
        run_command("systemctl restart %s" % container_name)
        run_command("sleep 5") # wait 5 seconds for application to sync
        return True

    def rollback_system(self, presys_version=""):
        if not presys_version:
            log_error("Previous version does not exist.")
            return True

        available_images = self.bootloader.get_installed_images()
        available_images.sort(reverse = False)
        log_info("    system will rollback to the previous version '{}'.".format(available_images[0]))
        self.bootloader.set_default_image(available_images[0])
        log_info("    system will reboot fast...")
        run_command("reboot")
        return True

    def system_diagnose(self, presys_version=""):
        # Check the status of all the container
        log_info("upgrade-system-check:")
        log_info(CUTTING_LINE)
        for container_name in self.container_names:
            if not self.check_container_status(container_name):
                self.rollback_system(presys_version)
                return False

            if not self.check_processes_status(container_name):
                self.rollback_system(presys_version)
                return False

            time.sleep(0.5)

        log_info("The diagnostic results of The system version '{}' is OK."\
            .format(self.bootloader.get_current_image()))
        return True

    def container_diagnose(self, container_name, container_backup_path, image_latest):
        log_info("upgrade-container-check: ")
        if not self.check_container_status(container_name):
            self.rollback_container(container_name, container_backup_path, image_latest)
            return False

        if not self.check_processes_status(container_name):
            self.rollback_container(container_name, container_backup_path, image_latest)
        else:
            log_info("    - The container '{}' diagnostic result is OK.".format(container_name))
            return True
 
    def deb_diagnose(self, container_name, deb_name):       # container_name: owner_of_deb
        log_info("upgrade-deb-check: ")
        if not self.check_container_status(container_name):
            self.rollback_container_after_upgrade_deb(container_name)
            return False

        if not self.check_processes_status(container_name):
            self.rollback_container_after_upgrade_deb(container_name)
        else:
            log_info("    - The container '{}' diagnostic result is OK.".format(container_name))
            return True

    def check_processes_status(self, container_name=""):
        # Check the status of all the processes in the container
        cmd = "docker exec {} ps -ef".format(container_name) 
        ret = subprocess.check_output(cmd, shell=True)
        ret_splits = ret.split("\n")
        ret_splits.pop(0)
        ret_splits.pop()
        ps_info_list = []

        for line in ret_splits:
            if (not line.endswith(" ps -ef")) and (not line.endswith(" bash")):
                ps_info_list.append(line)

        try:
            container_procs = self.procs_table[container_name]
        except KeyError:
            pass
        else:
            found = False
            for proc in container_procs:
                for line in ps_info_list:
                    if proc in line:
                        found = True
                        break

                if not found:
                    log_error("        - Error: Process '{0}' in container '{1}' was abnormal.".format(proc, container_name))
                    return False

                log_info("        - The status of process '{}' is OK.".format(proc))
                found = False
                time.sleep(0.5)
        
        log_info("    - All processes in The container '{}' are OK.".format(container_name))
        log_info(CUTTING_LINE)
        return True

    def check_container_status(self, container_name=""):
        # Check status of the container
        cmd = "docker inspect --format '{{.State.Status}}' " + container_name
        try:
            ret = subprocess.check_output(cmd, shell=True) 
        except:
            pass
        else:
            if not "running" in str(ret):
                log_error("    - Error: The container '{}' is not running.".format(container_name))
                return False

        log_info("    - Container '{}' is running.".format(container_name))
        return True
 
if __name__ == '__main__':
    if len(sys.argv) >= 2:
        lever = ""
        if len(sys.argv) == 3:
            lever = sys.argv[2]

        if not lever:
            daemon = Daemon()
        else:
            daemon = Daemon(log_lever=lever)

        if 'start' == sys.argv[1]:
            daemon.start()
        elif 'stop' == sys.argv[1]:
            daemon.stop()
        elif 'restart' == sys.argv[1]:
            daemon.restart()
        else:
            print("unknown command")
            sys.exit(2)
        sys.exit(0)
    else:
        print 'usage: %s start|stop|restart DEBUG|INFO|WARNING' % sys.argv[0]
        sys.exit(2)