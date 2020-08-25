#! /usr/bin/python -u
import os
import re
import signal
import stat
import sys
import time
import urllib
import subprocess
import click
import ftplib

"""
功能说明：通过ftp的方式，在指定的ftp服务器上下载文件，并且会实时显示下载进度条
"""
class ftpConnect:
    def __init__(self, host_ip, ftp_port=21):
        self.ftp = ftplib.FTP()
        self.host = host_ip
        self.port = ftp_port
        self.file = open('/home/admin/test.txt', 'wb')
        self.total_size = 0.0
        self.block_size = 0.0
        self.cursor_position = 0
        self.start_time = int(time.time())
        self.last_time = int(time.time())
        self.once = True
 
    def Login(self, user, passwd): 
        try:
            self.ftp.connect(self.host, self.port)
            self.ftp.login(user, passwd)
        except:
            print("Error: ftp login failed!")
            return False
        else:
            print(self.ftp.getwelcome())
            # print('====== FTP connection successful ======')
            return True
 
    def file_write(self, data):
        self.file.write(data) 
        self.block_size += len(data)
        self.cur_time = int(time.time())

        if self.once:
            self.start_time = self.cur_time
            self.last_time = self.cur_time
            self.once = False

        if self.cur_time == self.last_time:
            return

        self.last_time = self.cur_time
        duration = self.cur_time - self.start_time
        progress_size = self.block_size
        speed = int(progress_size / (1024 * duration))
        percent = self.block_size / self.total_size
        time_left = (self.total_size - progress_size) / speed / 1024
        self.cursor_position = (int(self.block_size) / int(self.total_size/100))
        
        # sys.stdout.write("\r...{0} | {1} MB | {2} KB/s | < {3} s | {4}"\
        #     .format('%.2f%%' % (percent * 100), '%d' % (progress_size / (1024 * 1024)), '%d' % speed, '%d' % time_left, "#"*self.cursor_position))
        sys.stdout.write("\r...{0} | {1} MB | {2} KB/s | < {3} s | {4}"\
            .format('%d%%' % (percent * 100), '%d' % (progress_size / (1024 * 1024)), '%d' % speed, '%d' % time_left, "#"*self.cursor_position))
        sys.stdout.flush()

    '''
    download single file
    LocalFile is local filepath + filename, RemoteFile is filename in ftp server
    '''
    def DownLoadFile(self, LocalFile, RemoteFile):
        try:
            self.file = open(LocalFile, 'wb')
            self.total_size = self.ftp.size(RemoteFile)
            click.echo('Downloading image...')
            # download
            self.ftp.retrbinary('RETR ' + RemoteFile, self.file_write)
        except Exception, e:
            print("Error: download failed '{}'".format(e))
            self.file.close()
            return False

        print("\n")
        self.file.close()
        return True

"""
功能说明：通过sftp的方式，在指定的sftp服务器上下载文件，并且会实时显示下载进度条
"""
class sftpConnect:
    def __init__(self, host_ip, sftp_port=22):
        self.host = host_ip
        self.port = sftp_port
        self.file = open('/home/admin/test.txt', 'wb')
        self.cursor_position = 0
        self.start_time = int(time.time())
        self.last_time = int(time.time())
        self.once = True

    def Login(self, user, passwd, sftp): 
        try:
            sftp.connect(username=user, password=passwd)
        except Exception, e:
            print("Error: sftp connected failed '{}'".format(e))
            return False
        else:
            # print('====== sftp connected successfully ======')
            return True
 
    def file_write(self, block_size, total_size):
        # print("block_size: {0}, total_size{1}".format(block_size, total_size))
        self.cur_time = int(time.time())

        if self.once:
            self.start_time = self.cur_time
            self.last_time = self.cur_time
            self.once = False

        if self.cur_time == self.last_time:
            return

        self.last_time = self.cur_time
        duration = self.cur_time - self.start_time
        progress_size = block_size
        speed = int(progress_size / (1024 * duration))
        percent = float(block_size) / float(total_size)
        time_left = (total_size - progress_size) / speed / 1024
        self.cursor_position = (int(block_size) / int(total_size/100))
        
        sys.stdout.write("\r...{0} | {1} MB | {2} KB/s | < {3} s | {4}"\
            .format('%d%%' % (percent * 100), '%d' % (progress_size / (1024 * 1024)), '%d' % speed, '%d' % time_left, "#"*self.cursor_position))
        sys.stdout.flush()

    '''
    download single file
    LocalFile is local filepath + filename, RemoteFile is filename in sftp server
    '''
    def DownLoadFile(self, LocalFile, RemoteFile, sftp):
        self.file = open(LocalFile, 'wb')

        try:
            my_sftp = paramiko.SFTPClient.from_transport(sftp)
            info = my_sftp.stat(RemoteFile)
            # print("size of remotefile: {}".format(info.st_size))
            click.echo('Downloading image...')
            # download
            my_sftp.get(RemoteFile, LocalFile, callback=self.file_write)
        except Exception, e:
            print("Error: download failed '{}'".format(e))
            self.file.close()
            return False

        print("\n")
        self.file.close()
        return True

"""
功能说明：通过http的方式，在指定的http服务器上下载文件，并且会实时显示下载进度条
"""
class httpConnect:
    def __init__(self, url, target):
        self.URL = url
        # self.ONIE_DEFAULT_IMAGE_PATH = '/tmp/test/sonic_image'
        self.ONIE_DEFAULT_IMAGE_PATH = target
        self.cursor_position = 0
        self.targetfile = ""

    """
    Displays the current download progress
    """
    def reporthook(self, count, block_size, total_size):
        global start_time, last_time
        cur_time = int(time.time())
        if count == 0:
            start_time = cur_time
            last_time = cur_time
            return

        if cur_time == last_time:
            return

        last_time = cur_time
        duration = cur_time - start_time
        progress_size = int(count * block_size)
        speed = int(progress_size / (1024 * duration))
        percent = int(count * block_size * 100 / total_size)
        time_left = (total_size - progress_size) / speed / 1024
        self.cursor_position = (int(progress_size) / int(total_size/100))

        sys.stdout.write("\r...%d%% | %d MB | %d KB/s | < %d s | %s" %
                                    (percent, progress_size / (1024 * 1024), speed, time_left, "#"*self.cursor_position))
        sys.stdout.flush()

    """
    Function which validates whether a given URL specifies an existent file
    on a reachable remote machine. Will abort the current operation if not
    """
    def validate_url_or_abort(self, url):
        # Attempt to retrieve HTTP response code
        try:
            urlfile = urllib.urlopen(url)
            response_code = urlfile.getcode()
            urlfile.close()
        except IOError:
            response_code = None

        if not response_code:
            click.echo("Did not receive a response from remote machine. Aborting...")
            raise click.Abort()
        else:
            # Check for a 4xx response code which indicates a nonexistent URL
            if response_code / 100 == 4:
                click.echo("Image file not found on remote machine. Aborting...")
                raise click.Abort()

    '''
    download image from local binary or URL
    '''
    def download(self):
        DEFAULT_IMAGE_PATH = self.ONIE_DEFAULT_IMAGE_PATH
        image_url = self.URL
        if image_url.startswith('http://') or image_url.startswith('https://'):
            click.echo('Downloading image...')
            self.validate_url_or_abort(image_url)
            url_sp = str(self.URL).rsplit("/")
            self.targetfile = url_sp[len(url_sp) - 1]

            try:
                urllib.urlretrieve(image_url, DEFAULT_IMAGE_PATH, self.reporthook)
            except Exception, e:
                click.echo("Download error", e)
                raise click.Abort()
            image_path = DEFAULT_IMAGE_PATH
        else:
            image_path = os.path.join("./", image_url)

        # Verify that the local file exists and is a regular file
        # TODO: Verify the file is a *proper SONiC image file*
        print("\nimage_path: {}".format(image_path))
        if not os.path.isfile(image_path):
            print("Image file '{}' does not exist or is not a regular file. Aborting...".format(image_path))
        print("download '{}' successfully.".format(self.targetfile))

"""
使用举例：可以直接调用下面类中的方法进行下载文件的操作，先初始化一个实例，然后直接调用该实例相关的函数即可
"""
class upgrade():
    def __init__(self):
        self.bootloader = ""

    # Download image by ftp
    def ftpDownload(self, username, password, ip, port, localpath, localFilename, targetpath, targetFilename):
        ftp_connect = ftpConnect(ip, port)
        if ftp_connect.Login(username, password):
            image_path = localpath + localFilename
            if ftp_connect.DownLoadFile(image_path, targetFilename):
                print("download '{}' successfully.".format(targetFilename))
            else:
                print("download '{}' failed!".format(targetFilename))
        else:
            raise click.Abort()

    # Download image by sftp
    def sftpDownload(self, username, password, ip, port, localpath, localFilename, targetpath, targetFilename):
        sftp_connect = sftpConnect(ip, port)
        sftp = paramiko.Transport((ip, int(port)))
        if sftp_connect.Login(username, password, sftp):
            image_path = localpath + localFilename
            # print("image_path: {}, targetFilename: {}".format(image_path, targetFilename))
            if sftp_connect.DownLoadFile(image_path, targetFilename, sftp):
                print("download '{}' successfully.".format(targetFilename))
            else:
                print("download '{}' failed!".format(targetFilename))
        else:
            raise click.Abort()

    # Download image by http
    def httpDownload(self, url, image_path):
        http_connect = httpConnect(url, image_path)
        http_connect.download()




