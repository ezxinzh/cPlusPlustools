#include "ftpClient.h"

/** 
 * 编译使用：g++ *.cpp -g -o ftp-test
 *  **/

int main(int argc, char **argv)
{
    string localPath = "/home/admin/upgrade/cplusplus/";
    string serverIP = "10.230.2.225:21";
    string userName = "ftpTest", password = "Platform@123";
    string remoteFile = "docker-lldp-sv2.gz", localFile = "docker-lldp-sv2.gz";
    string devName = "eth0";
    string rePath = "test111";
    CFTPManager ftpClient(localPath, devName);

    if(argc == 2)
    {
        remoteFile = argv[1];
        LOGG_DEBUG("========== remoteFile: %s\n", remoteFile);
        localFile = remoteFile;
    }
    else
    {
        LOGG_WARN("usage: ./ftpTest xxx\n");
        return -1;
    }
    
    ftpClient.login2Server(serverIP);
    ftpClient.inputUserName(userName);
    ftpClient.inputPassWord(password);

    if(ftpClient.CD(rePath) == -1)
    {
        LOGG_ERR("chdir to '%s' failed!", rePath.c_str());
        return -1;
    }

    if(ftpClient.Get(remoteFile, localFile) == -1)
    {
        printf("\ndownload file '%s' failed!\n", remoteFile.c_str());
        return -1;
    }
    else
    {
        printf("\ndownload file '%s' successfully.\n", remoteFile.c_str());
    }
    

    return 0;
}