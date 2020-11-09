#include "ftpClient.h"

/** 
 * 编译使用：g++ *.cpp -g -o ftp-test
 *  **/

int main(int argc, char **argv)
{
    CFTPManager ftpClient;
    string serverIP = "10.230.2.225:21";
    string userName = "ftpTest";
    string password = "Platform@123";
    string remoteFile = "111.txt", localFile = "111.txt";

    if(argc == 2)
    {
        remoteFile = argv[1];
        PRINT_DEBUG("========== remoteFile: %s\n", remoteFile);
        localFile = remoteFile;
    }
    else
    {
        PRINT_WARN("usage: ./ftpTest xxx\n");
        return -1;
    }
    

    ftpClient.login2Server(serverIP);
    ftpClient.inputUserName(userName);
    ftpClient.inputPassWord(password);
    ftpClient.Get(remoteFile, localFile);
    ftpClient.quitServer();


    return 0;
}