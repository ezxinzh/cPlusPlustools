#ifndef _FTP_CLIENT_H_
#define _FTP_CLIENT_H_
#include <sys/socket.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <sys/stat.h>
#include <netdb.h>
#include <errno.h>
#include <arpa/inet.h>
#include <sys/stat.h>
#include <net/if.h>
#include <list>
#include <assert.h>
#include <string.h>
#include <vector>
#include <string>

/*		usage:
		login2Server
			|
		inputUserName
			|
		inputPassWord
			|
		User-defined operation
			|
		  quit
*/
 
using namespace std;

static string getModule(string filename);

#define LOGG_DEBUG(fmt,args...)                                                                    \
    if(0)                                                                                           \
    {                                                                                               \
        printf("%s: DEBUG :- %s: " #fmt "\n", getModule(__FILE__).c_str(), __FUNCTION__, ##args);   \
    }

#define LOGG_ERR(fmt,args...) \
        printf("ERROR: "); \
        printf(fmt, ##args); \
        printf("\n");
#define LOGG_WARN(fmt,args...) \
        printf("WARNING: "); \
        printf(fmt, ##args); \
        printf("\n");
#define LOGG_INFO(fmt,args...) \
        printf("INFO: "); \
        printf(fmt, ##args); \
        printf("\n");
 
#define INVALID_SOCKET				-1
#define MAX_PATH					1028
#define FTP_DEFAULT_PORT			"21"							/* default port */
#define FTP_DEFAULT_BUFFER			(1024*6)						/* FTP default buffer size */
#define FTP_DEFAULT_PATH			"/"								/* FTP default local path */
	
#define FTP_COMMAND_BASE				1000
#define FTP_COMMAND_END					FTP_COMMAND_BASE + 30
#define FTP_COMMAND_USERNAME			FTP_COMMAND_BASE + 1			
#define FTP_COMMAND_PASSWORD			FTP_COMMAND_BASE + 2			
#define FTP_COMMAND_QUIT				FTP_COMMAND_BASE + 3			
#define FTP_COMMAND_CURRENT_PATH		FTP_COMMAND_BASE + 4			
#define FTP_COMMAND_TYPE_MODE			FTP_COMMAND_BASE + 5			
#define FTP_COMMAND_PSAV_MODE			FTP_COMMAND_BASE + 6			
#define FTP_COMMAND_DIR					FTP_COMMAND_BASE + 7			
#define FTP_COMMAND_CHANGE_DIRECTORY 	FTP_COMMAND_BASE + 8			
#define FTP_COMMAND_DELETE_FILE			FTP_COMMAND_BASE + 9			
#define FTP_COMMAND_DELETE_DIRECTORY 	FTP_COMMAND_BASE + 10			
#define FTP_COMMAND_CREATE_DIRECTORY 	FTP_COMMAND_BASE + 11			
#define FTP_COMMAND_RENAME_BEGIN    	FTP_COMMAND_BASE  +12			
#define FTP_COMMAND_RENAME_END      	FTP_COMMAND_BASE + 13	
#define FTP_COMMAND_FILE_SIZE			FTP_COMMAND_BASE + 14			
#define FTP_COMMAND_DOWNLOAD_POS		FTP_COMMAND_BASE + 15			
#define FTP_COMMAND_DOWNLOAD_FILE		FTP_COMMAND_BASE + 16			
#define FTP_COMMAND_UPLOAD_FILE			FTP_COMMAND_BASE + 17		
#define FTP_COMMAND_APPEND_FILE			FTP_COMMAND_BASE + 18			

typedef struct
{
    long block_size = 0;
    long total_size = 0;     /* the total length of remote file */
    char bar[102] = {0};
}progress_t;

enum type {
	binary = 0x31,
	ascii,
};
 
class CFTPManager
{
public :
	CFTPManager(string localPath, string deviceName);
	virtual ~CFTPManager(void);

	bool login2Server(const std::string &serverIP);
	bool inputUserName(const std::string &userName);
	bool inputPassWord(const std::string &password);
	bool quitServer(void);
	const std::string PWD();
	bool setTransferMode(type mode);
	const std::string Pasv();
	const std::string Dir(const std::string &path);
	bool CD(const std::string &path);
	bool DeleteFile(const std::string &strRemoteFile);
	bool DeleteDirectory(const std::string &strRemoteDir);
	bool CreateDirectory(const std::string &strRemoteDir);
	bool Rename(const std::string &strRemoteFile, const std::string &strNewFile);
	long getFileLength(const std::string &strRemoteFile);
	void Close(int sock);
	bool Get(const std::string &strRemoteFile, const std::string &strLocalFile);
	bool Put(const std::string &strRemoteFile, const std::string &strLocalFile);
    void downloadProgress(int datasize);

private:
	const std::string parseCommand(const unsigned int command, const std::string &strParam);
	bool Connect(int socketfd, const std::string &serverIP, unsigned int nPort);
	const std::string serverResponse(int sockfd);
	int getData(int fd, char *strBuf, unsigned long length);
	bool Send(int fd, const std::string &cmd);
	bool Send(int fd, const char *cmd, const size_t len);
	bool createDataLink(int data_fd);
	int ParseString(std::list<std::string> strArray, unsigned long & nPort ,std::string & strServerIp);
	FILE *createLocalFile(const std::string &strLocalFile);
	bool downLoad(const std::string &strRemoteFile, const std::string &strLocalFile, const int pos = 0, 
		const unsigned int length = 0);
	bool parseResponse(const std::string &str);
    bool bindToDevice(int &socket_fd);
 
private:
	int	m_cmdSocket;
	std::string m_strUserName;
	std::string m_strPassWord;
	std::string m_strServerIP;
	unsigned int m_nServerPort;
	std::string m_commandStr;
	unsigned int m_nCurrentCommand;
	bool m_bLogin;
    progress_t m_progress;
    string m_localPath = FTP_DEFAULT_PATH;
    string m_remPath = FTP_DEFAULT_PATH;
    string m_deviceName;        /* the device to be binded with socket */
};
 
 
#endif		/* _FTP_CLIENT_H_ */
