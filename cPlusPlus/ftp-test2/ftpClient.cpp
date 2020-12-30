#include "system_upgrade_ftpClient.h"
 
static int SplitString( string strSrc, std::list<string> &strArray , string strFlag)
{
	try
	{
		size_t  pos; 
	
		while(((pos = strSrc.find_first_of(strFlag.c_str()))) != strSrc.npos) 
		{
			strArray.insert(strArray.end(), strSrc.substr(0 , pos));
			strSrc = strSrc.substr(pos + 1, strSrc.length() - pos - 1); 
		}
	
		strArray.insert(strArray.end(), strSrc.substr(0, strSrc.length()));
	}
	catch(const std::exception& e)
	{
		LOGG_ERR("%s", e.what());
	}
 
	return 0; 
}
 
CFTPManager::CFTPManager(string localPath, string deviceName) : m_bLogin(false)
{
	m_cmdSocket = socket(AF_INET, SOCK_STREAM, 0);

    if(!localPath.empty())
    {
        m_localPath = localPath;
    }

    if(!deviceName.empty())
    {
        m_deviceName = deviceName;
    }

    bindToDevice(m_cmdSocket);
}
 
CFTPManager::~CFTPManager(void)
{
	string strCmdLine = parseCommand(FTP_COMMAND_QUIT, "");
	Send(m_cmdSocket, strCmdLine.c_str());
	close(m_cmdSocket);
	m_bLogin = false;
}
 
bool CFTPManager::login2Server(const string &serverIP)
{
	string strPort;
	auto pos = serverIP.find(":");
	string strResponse;
 
	if (pos != serverIP.npos)
	{
		strPort = serverIP.substr(pos + 1, serverIP.length() - pos);
	}
	else
	{
		pos = serverIP.length();
		strPort = FTP_DEFAULT_PORT;
	}
 
	m_strServerIP = serverIP.substr(0, pos);
	m_nServerPort = atoi(strPort.c_str());
 
	if(!Connect(m_cmdSocket, m_strServerIP, m_nServerPort))
	{
		return false;
	}
	
	strResponse = serverResponse(m_cmdSocket);
	LOGG_DEBUG("@@@@Response: %s", strResponse.c_str());

	return	parseResponse(strResponse);
}
 
bool CFTPManager::inputUserName(const string &userName)
{
	string strResponse;
	string strCommandLine = parseCommand(FTP_COMMAND_USERNAME, userName);
	m_strUserName = userName;
 
	if(!Send(m_cmdSocket, strCommandLine))
	{
		LOGG_ERR("send cmd '%s' falied.", strCommandLine.c_str());
		return false;
	}
 
	strResponse = serverResponse(m_cmdSocket);
	LOGG_DEBUG("Response: %s", strResponse.c_str());
	return parseResponse(strResponse);
}
 
bool CFTPManager::inputPassWord(const string &password)
{
	string strResponse;
	string strCmdLine = parseCommand(FTP_COMMAND_PASSWORD, password);
	m_strPassWord = password;

	if(!Send(m_cmdSocket, strCmdLine))
	{
		LOGG_ERR("send cmd '%s' falied.", strCmdLine.c_str());
		return false;
	}

	m_bLogin = true;
	strResponse = serverResponse(m_cmdSocket);
	LOGG_DEBUG("Response: %s", strResponse.c_str());

	return parseResponse(strResponse);
}
 
bool CFTPManager::quitServer(void)
{
	string strCmdLine = parseCommand(FTP_COMMAND_QUIT, "");

	if(!Send(m_cmdSocket, strCmdLine))
	{
		LOGG_ERR("send cmd '%s' falied.", strCmdLine.c_str());
		return false;
	}

	string strResponse = serverResponse(m_cmdSocket);
	LOGG_DEBUG("Response: %s", strResponse.c_str());

	return parseResponse(strResponse);
}
 
const string CFTPManager::PWD()
{
	string ret = "";
	string strCmdLine;
	strCmdLine = parseCommand(FTP_COMMAND_CURRENT_PATH, "");
 
	if(!Send(m_cmdSocket, strCmdLine.c_str()))
	{
		LOGG_ERR("send cmd '%s' falied.", strCmdLine.c_str());
		return ret;
	}

	ret = serverResponse(m_cmdSocket);
	return ret;
}
 
 
bool CFTPManager::setTransferMode(type mode)
{
	string strCmdLine, strResponse;
 
	switch (mode)
	{
		case binary:
		{
			strCmdLine = parseCommand(FTP_COMMAND_TYPE_MODE, "I");
			break;
		}
		case ascii:
		{
			strCmdLine = parseCommand(FTP_COMMAND_TYPE_MODE, "A");
			break;
		}
		default: 
		{
			return false;
		}
	}
 
	if(!Send(m_cmdSocket, strCmdLine.c_str()))
	{
		LOGG_ERR("Send cmd '%s' falied.", strCmdLine.c_str());
		return false;
	}

	strResponse  = serverResponse(m_cmdSocket);
	LOGG_DEBUG("@@@@Response: %s", strResponse.c_str());

	return parseResponse(strResponse);
}
 
 
const string CFTPManager::Pasv()
{
	string strCmdLine = parseCommand(FTP_COMMAND_PSAV_MODE, "");
 
	if(!Send(m_cmdSocket, strCmdLine))
	{
		LOGG_ERR("Send cmd '%s' falied.", strCmdLine.c_str());
		return "";
	}

	string strResponse = serverResponse(m_cmdSocket);
	return strResponse;;
}
 
 
const string CFTPManager::Dir(const string &path)
{
	string strResponse = "";
	int dataSocket = socket(AF_INET, SOCK_STREAM, 0);

    if(!bindToDevice(dataSocket))
	{
		return strResponse;
	}
 
	if(!createDataLink(dataSocket))
	{
		return strResponse;
	}

	string strCmdLine = parseCommand(FTP_COMMAND_DIR, path);
 
	if(!Send(m_cmdSocket, strCmdLine))
	{
		LOGG_ERR("send cmd '%s' falied.", strCmdLine.c_str());
		close(dataSocket);
	}
	else
	{
		strResponse = serverResponse(m_cmdSocket);
		LOGG_DEBUG("@@@@Response: %s", strResponse.c_str());
		strResponse = serverResponse(dataSocket);
		LOGG_DEBUG("@@@@Response: %s", strResponse.c_str());
		close(dataSocket);
	}
	
	return strResponse;
}
 
 
bool CFTPManager::CD(const string &path)
{
	if(m_cmdSocket == INVALID_SOCKET)
	{
		LOGG_ERR("Invalid cmdSocket %d.", m_cmdSocket);
		return false;
	}
 
	string strCmdLine = parseCommand(FTP_COMMAND_CHANGE_DIRECTORY, path);
 
	if(!Send(m_cmdSocket, strCmdLine))
	{
		LOGG_ERR("send cmd '%s' falied.", strCmdLine.c_str());
		return false;
	}
		
	string strResponse = serverResponse(m_cmdSocket);
	LOGG_DEBUG("@@@@Response: %s", strResponse.c_str());

	return parseResponse(strResponse);
}
 
bool CFTPManager::DeleteFile(const string &strRemoteFile)
{
	if(m_cmdSocket == INVALID_SOCKET) 
	{
		LOGG_ERR("Invalid cmdSocket %d.", m_cmdSocket);
		return false;
	}
 
	string strCmdLine = parseCommand(FTP_COMMAND_DELETE_FILE, strRemoteFile);
 
	if(!Send(m_cmdSocket, strCmdLine))
	{
		LOGG_ERR("send cmd:'%s' falied.", strCmdLine.c_str());
		return false;
	}
 
	string strResponse = serverResponse(m_cmdSocket);
	LOGG_DEBUG("@@@@Response: %s", strResponse.c_str());

	return parseResponse(strResponse);
}
 
bool CFTPManager::DeleteDirectory(const string &strRemoteDir)
{
	if(m_cmdSocket == INVALID_SOCKET) 
	{
		LOGG_ERR("Invalid cmdSocket %s.", m_cmdSocket);
		return false;
	}
 
	string strCmdLine = parseCommand(FTP_COMMAND_DELETE_DIRECTORY, strRemoteDir);
 
	if(!Send(m_cmdSocket, strCmdLine))
	{
		LOGG_ERR("send cmd '%s' falied.", strCmdLine.c_str());
		return false;
	}
	
	string strResponse = serverResponse(m_cmdSocket);
	LOGG_DEBUG("@@@@Response: %s", strResponse.c_str());

	return parseResponse(strResponse);
}
 
bool CFTPManager::CreateDirectory(const string &strRemoteDir)
{
	if(m_cmdSocket == INVALID_SOCKET) 
	{
		LOGG_ERR("Invalid cmdSocket %s.", m_cmdSocket);
		return false;
	}
 
	string strCmdLine = parseCommand(FTP_COMMAND_CREATE_DIRECTORY, strRemoteDir);
 
	if(!Send(m_cmdSocket, strCmdLine))
	{
		LOGG_ERR("send cmd '%s' falied.", strCmdLine.c_str());
		return false;
	}
	
	string strResponse = serverResponse(m_cmdSocket);
	LOGG_DEBUG("@@@@Response: %s", strResponse.c_str());

	return parseResponse(strResponse);
}
 
bool CFTPManager::Rename(const string &strRemoteFile, const string &strNewFile)
{
	string strResponse = "";
	string strCmdLine;

	if(m_cmdSocket == INVALID_SOCKET) 
	{
		LOGG_ERR("Invalid cmdSocket %ld.", m_cmdSocket);
		return false;
	}
 
	strCmdLine = parseCommand(FTP_COMMAND_RENAME_BEGIN, strRemoteFile);
	if(!Send(m_cmdSocket, strCmdLine))
	{
		LOGG_ERR("Send cmd:%s failed.", strCmdLine.c_str());
		return false;
	}

	strResponse = serverResponse(m_cmdSocket);
	LOGG_DEBUG("@@@@Response: %s", strResponse.c_str());
	strCmdLine = parseCommand(FTP_COMMAND_RENAME_END, strNewFile);

	if(!Send(m_cmdSocket, strCmdLine))
	{
		LOGG_ERR("Send cmd:%s failed.", strCmdLine.c_str());
		return false;
	}
 
	strResponse = serverResponse(m_cmdSocket);
	LOGG_DEBUG("@@@@Response: %s", strResponse.c_str());

	return parseResponse(strResponse);
}
 
long CFTPManager::getFileLength(const string &strRemoteFile)
{
	string strResponse;

	if(m_cmdSocket == INVALID_SOCKET) 
	{
		LOGG_ERR("Invalid m_cmdSocket %ld.", m_cmdSocket);
		return -1;
	}
 
	string strCmdLine = parseCommand(FTP_COMMAND_FILE_SIZE, strRemoteFile);
 
	if(!Send(m_cmdSocket, strCmdLine))
	{
		LOGG_ERR("Send cmd:%s failed.", strCmdLine.c_str());
		return -1;
	}
 
	strResponse = serverResponse(m_cmdSocket);
	LOGG_DEBUG("@@@@Response: %s", strResponse.c_str());
	string strData = strResponse.substr(0, 3);
	unsigned long val = atol(strData.c_str());
 
	if (val == 213)
	{
		strData = strResponse.substr(4);
		val = atol(strData.c_str());
		LOGG_DEBUG("Get length '%ld' of file '%s'", val, strRemoteFile.c_str());
		return val;
	}
 
	return -1;
}
 
 
void CFTPManager::Close(int sock)
{
	shutdown(sock, SHUT_RDWR);
	close(sock);
	sock = INVALID_SOCKET;
}
 
bool CFTPManager::Get(const string &strRemoteFile, const string &strLocalFile)
{
	bool ret = false;
	ret = downLoad(strRemoteFile, strLocalFile);
	return ret;
}
 
 
bool CFTPManager::Put(const string &strRemoteFile, const string &strLocalFile)
{
	string strCmdLine;
	const unsigned long dataLen = FTP_DEFAULT_BUFFER;
	char strBuf[dataLen] = {0};
	unsigned long nSize = getFileLength(strRemoteFile);
	unsigned long nLen = 0;
 
	FILE *pFile = fopen(strLocalFile.c_str(), "rb"); 

	if(pFile == NULL)
	{
		 return false;
	}
 
	int data_fd = socket(AF_INET, SOCK_STREAM, 0);

	if(data_fd == -1) 
	{
		return false;
	}

    if(!bindToDevice(data_fd))
	{
		return false;
	}
 
	if(!createDataLink(data_fd))
	{
		return false;
	}
	
	if (nSize <= 0)
	{
		strCmdLine = parseCommand(FTP_COMMAND_UPLOAD_FILE, strRemoteFile);
	}
	else
	{
		strCmdLine = parseCommand(FTP_COMMAND_APPEND_FILE, strRemoteFile);
	}
 
	if(!Send(m_cmdSocket, strCmdLine))
	{
		LOGG_ERR("Send cmd:%s failed.", strCmdLine.c_str());
		Close(data_fd);
		return false;
	}
 
	string strResponse = serverResponse(m_cmdSocket);
	LOGG_DEBUG("@@@@Response: %s", strResponse.c_str());
	fseek(pFile, nSize, SEEK_SET);

	while (!feof(pFile))
	{
		nLen = fread(strBuf, 1, dataLen, pFile);
		if (nLen < 0)
		{
			break;
		}
 
		if(!Send(data_fd, strBuf))
		{
			LOGG_ERR("Send cmd:%s failed.", strBuf);
			Close(data_fd);
			return false;
		}
	}
 
	strResponse = serverResponse(data_fd);
	LOGG_DEBUG("@@@@Response: %s", strResponse.c_str());
	Close(data_fd);
	strResponse = serverResponse(m_cmdSocket);
	LOGG_DEBUG("@@@@Response: %s", strResponse.c_str());
	fclose(pFile);
 
	return true;
}
 
const string CFTPManager::parseCommand(const unsigned int command, const string &strParam)
{
	if (command < FTP_COMMAND_BASE || command > FTP_COMMAND_END)
	{
		LOGG_ERR("Invalid command:%d .", command);
		return "";
	}
 
	string strCommandLine = "";
	m_nCurrentCommand = command;
 
	switch (command)
	{
		case FTP_COMMAND_USERNAME:
			strCommandLine = "USER ";
			break;
		case FTP_COMMAND_PASSWORD:
			strCommandLine = "PASS ";
			break;
		case FTP_COMMAND_QUIT:
			strCommandLine = "QUIT ";
			break;
		case FTP_COMMAND_CURRENT_PATH:
			strCommandLine = "PWD ";
			break;
		case FTP_COMMAND_TYPE_MODE:
			strCommandLine = "TYPE ";
			break;
		case FTP_COMMAND_PSAV_MODE:
			strCommandLine = "PASV ";
			break;
		case FTP_COMMAND_DIR:
			strCommandLine = "LIST ";
			break;
		case FTP_COMMAND_CHANGE_DIRECTORY:
			strCommandLine = "CWD ";
			break;
		case FTP_COMMAND_DELETE_FILE:
			strCommandLine = "DELE ";
			break;
		case FTP_COMMAND_DELETE_DIRECTORY:
			strCommandLine = "RMD ";
			break;
		case FTP_COMMAND_CREATE_DIRECTORY:
			strCommandLine = "MKD ";
			break;
		case FTP_COMMAND_RENAME_BEGIN:
			strCommandLine = "RNFR ";
			break;
		case FTP_COMMAND_RENAME_END:
			strCommandLine = "RNTO ";
			break;
		case FTP_COMMAND_FILE_SIZE:
			strCommandLine = "SIZE ";
			break;
		case FTP_COMMAND_DOWNLOAD_FILE:
			strCommandLine = "RETR ";
			break;
		case FTP_COMMAND_DOWNLOAD_POS:
			strCommandLine = "REST ";
			break;
		case FTP_COMMAND_UPLOAD_FILE:
			strCommandLine = "STOR ";
			break;
		case FTP_COMMAND_APPEND_FILE:
			strCommandLine = "APPE ";
			break;
		default :
			break;
	}
 
	strCommandLine += strParam;
	strCommandLine += "\r";
	m_commandStr = strCommandLine;
	LOGG_DEBUG("parseCommand: %s", m_commandStr.c_str());
 
	return m_commandStr;
}
 
bool CFTPManager::Connect(int socketfd, const string &serverIP, unsigned int nPort)
{
	if (socketfd == INVALID_SOCKET)
	{
		LOGG_ERR("Invalid socket %ld.", socketfd);
		return false;
	}
 
	unsigned int argp = 1;
	int error = -1;
	int len = sizeof(int);
	struct sockaddr_in  addr;
	bool ret = false;
	timeval stime;
	fd_set  set;
 
	ioctl(socketfd, FIONBIO, &argp);  
	bzero(&addr, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(nPort);
	addr.sin_addr.s_addr = inet_addr(serverIP.c_str());
	LOGG_DEBUG("============= ip:%s port:%d", inet_ntoa(addr.sin_addr), ntohs(addr.sin_port));
	
	if (connect(socketfd, (struct sockaddr*)&addr, sizeof(struct sockaddr)) == -1)   
	{
		stime.tv_sec = 100;  
		stime.tv_usec = 0;
		FD_ZERO(&set);
		FD_SET(socketfd, &set);
 
		if (select(socketfd + 1, NULL, &set, NULL, &stime) > 0)   
		{
			getsockopt(socketfd, SOL_SOCKET, SO_ERROR, &error, (socklen_t*)&len);

			if (error == 0)
			{
				ret = true;
			}
			else
			{
				LOGG_ERR("Select timeout.");
				ret = false;
			}
		}
	}
	else
	{	
		LOGG_DEBUG("Connect Immediately.");
		ret = true;
	}
 
	argp = 0;
	ioctl(socketfd, FIONBIO, &argp);
 
	if (!ret)
	{
		close(socketfd);
		LOGG_ERR("cannot connect server!!");
		return false;
	}
 
	return true;
}
 
 
const string CFTPManager::serverResponse(int sockfd)
{
	string strResponse = "";

	try
	{
		if (sockfd == INVALID_SOCKET)
		{
			LOGG_ERR("Invalid socket:%ld .", sockfd);
			return "";
		}
		
		int nRet = -1;
		char buf[MAX_PATH] = {0};
	
		while ((nRet = getData(sockfd, buf, MAX_PATH)) > 0)
		{
			buf[MAX_PATH - 1] = '\0';
			strResponse += buf;
		}
	}
	catch(const std::exception& e)
	{
		LOGG_ERR("%s", e.what());
	}
 
	return strResponse;
}
 
int CFTPManager::getData(int fd, char *strBuf, unsigned long length)
{
	try
	{
		if(strBuf == NULL) 
		{
			LOGG_ERR("strBuf was null .");
			return -1;
		}
	
		if (fd == INVALID_SOCKET)
		{
			LOGG_ERR("Invalid fd %s.", fd);
			return -1;
		}
	
		memset(strBuf, 0, length);
		timeval stime;
		int nLen;
	
		stime.tv_sec = 1;
		stime.tv_usec = 0;
	
		fd_set	readfd;
		FD_ZERO( &readfd );
		FD_SET(fd, &readfd );
	
		if (select(fd + 1, &readfd, 0, 0, &stime) > 0)
		{
			if ((nLen = recv(fd, strBuf, length, 0)) > 0)
			{
				return nLen;
			}
			else
			{
				return -1;
			}
		}
	}
	catch(const std::exception& e)
	{
		LOGG_ERR("%s", e.what());
	}

	return -1;
}
 
bool CFTPManager::Send(int fd, const string &cmd)
{
	int ret = false;

	if(fd == INVALID_SOCKET)
	{
		LOGG_ERR("Invalid socket '%d' .", fd);
		return false;
	}
 
	ret =  Send(fd, cmd.c_str(), cmd.length());

	return ret;
}
 
bool CFTPManager::Send(int fd, const char *cmd, const size_t len)
{
	try
	{
		if((FTP_COMMAND_USERNAME != m_nCurrentCommand) && (FTP_COMMAND_PASSWORD != m_nCurrentCommand)
			&&(!m_bLogin))
		{
			return false;
		}
	
		timeval timeout;
		timeout.tv_sec  = 1;
		timeout.tv_usec = 0;
		fd_set  writefd;
		FD_ZERO(&writefd);  
		FD_SET(fd, &writefd);
	
		if(select(fd + 1, 0, &writefd , 0 , &timeout) > 0)
		{
			size_t nlen  = len; 
			int nSendLen = 0; 

			while (nlen > 0) 
			{
				nSendLen = send(fd, cmd , (int)nlen , 0);
				if(nSendLen == -1) 
				{
					return false; 
				}
					
				nlen = nlen - nSendLen;
				cmd +=  nSendLen;
			}

			return true;
		}
	}
	catch(const std::exception& e)
	{
		LOGG_ERR("%s", e.what());
	}

	return false;
}
 
 
bool CFTPManager::createDataLink(int data_fd)
{
	try
	{
		if(data_fd == INVALID_SOCKET) 
		{
			LOGG_ERR("Invalid socket %d.", data_fd);
			return false;
		}
	
		string strData;
		unsigned long nPort = 0 ;
		string strServerIp ; 
		std::list<string> strArray ;
		string parseStr = Pasv();
	
		if (parseStr.size() <= 0)
		{
			LOGG_ERR("parseStr was empty.");
			return false;
		}
	
		size_t nBegin = parseStr.find_first_of("(");
		size_t nEnd	  = parseStr.find_first_of(")");
		strData		  = parseStr.substr(nBegin + 1, nEnd - nBegin - 1);
	
		if( SplitString( strData , strArray , "," ) < 0)
		{
			LOGG_ERR("Split %s failed.", strData.c_str());
			return false;
		}
	
		if( ParseString( strArray , nPort , strServerIp) < 0)
		{
			LOGG_ERR("Parse string Array %s failed.");
			return false;
		}
	
		LOGG_DEBUG("nPort: %ld IP: %s", nPort, strServerIp.c_str());
	
		if(!Connect(data_fd, strServerIp, nPort))
		{
			LOGG_ERR("Connect ftp server '%s:%ld' failed.", strServerIp.c_str(), nPort);
			return false;
		}
	}
	catch(const std::exception& e)
	{
		LOGG_ERR("%s", e.what());
	}
 
	return true;
}
 
int CFTPManager::ParseString(std::list<string> strArray, unsigned long & nPort ,string & strServerIp)
{
	try
	{
		if (strArray.size() < 6 )
		{
			LOGG_ERR("Size of strArray less than 6.");
			return -1 ;
		}
	
		std::list<string>::iterator citor;
		citor = strArray.begin();
		strServerIp = *citor;
		strServerIp += ".";
		citor ++;
		strServerIp += *citor;
		strServerIp += ".";
		citor ++ ;
		strServerIp += *citor;
		strServerIp += ".";
		citor ++ ;
		strServerIp += *citor;
		citor = strArray.end();
		citor--;
		nPort = atol( (*citor).c_str());
		citor--;
		nPort += atol( (*(citor)).c_str()) * 256 ;
	}
	catch(const std::exception& e)
	{
		LOGG_ERR("%s", e.what());
	}

	return 0 ; 
}
 
FILE *CFTPManager::createLocalFile(const string &strLocalFile)
{
	FILE * file = NULL;
	file = fopen(strLocalFile.c_str(), "w+b");
	return file;
}
 
bool CFTPManager::downLoad(const string &strRemoteFile, const string &strLocalFile, 
    const int pos, const unsigned int length)
{
    bool ret = false;

	if(length < 0) 
	{
		LOGG_ERR("Length less than 0.");
		return false;
	}

	try
	{
		memset(&m_progress, 0, sizeof(progress_t));
		int length = getFileLength(strRemoteFile);

		if(length <= 0)
		{
			LOGG_ERR("The size of '%s' is less than 0.", strRemoteFile.c_str());
			return false;
		}

		m_progress.total_size = length;
		FILE *file = NULL;
		unsigned long nDataLen = FTP_DEFAULT_BUFFER;
		char strPos[MAX_PATH]  = {0};
		int data_fd = socket(AF_INET, SOCK_STREAM, 0);
		char *dataBuf = NULL;

		if(data_fd == -1) 
		{
			LOGG_ERR("Invalid socket '%d'.", data_fd);
			return false;
		}

		if(!bindToDevice(data_fd))
		{
			return false;
		}
	
		if ((length != 0) && (length < nDataLen))
		{
			nDataLen = length;
		}

		dataBuf = new char[nDataLen];

		if(dataBuf == NULL) 
		{
			LOGG_ERR("Failed to allocate memory to dataBuf.");
			return false;
		}
	
		if(!createDataLink(data_fd))
		{
			LOGG_ERR("Create Data Link error.");
			return false;
		}
	
		sprintf(strPos, "%d", pos);
		string strCmdLine = parseCommand(FTP_COMMAND_DOWNLOAD_POS, strPos);

		if(!Send(m_cmdSocket, strCmdLine))
		{
			LOGG_ERR("Send cmd:%s failed.", strCmdLine.c_str());
			return false;
		}

		LOGG_DEBUG("Response: %s", serverResponse(m_cmdSocket).c_str());
	
		strCmdLine = parseCommand(FTP_COMMAND_DOWNLOAD_FILE, strRemoteFile);
	
		if(!Send(m_cmdSocket, strCmdLine))
		{
			LOGG_ERR("File '%s' does not exist!", strRemoteFile.c_str());
			return false;
		}

		LOGG_DEBUG("Response: %s", serverResponse(m_cmdSocket).c_str());
		file = createLocalFile(string(m_localPath + strLocalFile));

		if(file == NULL) 
		{
			LOGG_ERR("Create local file failed.");
			return false;
		}
		
		int len = 0;
		int nReceiveLen = 0;

		while ((len = getData(data_fd, dataBuf, nDataLen)) > 0)
		{
			nReceiveLen += len;
			int num = fwrite(dataBuf, 1, len, file);
			memset(dataBuf, 0, sizeof(dataBuf));
		
			if (nReceiveLen == length && length != 0) 
			{
				break;
			}
	
			if ((nReceiveLen + nDataLen) > length  && length != 0)
			{
				delete []dataBuf;
				nDataLen = length - nReceiveLen;
				dataBuf = new char[nDataLen];
			}

			downloadProgress(len);
		}

		if(nReceiveLen > 1) 
		{
			ret = true;
		}

		Close(data_fd);
		fclose(file);
		delete []dataBuf;
	}
	catch(const std::exception& e)
	{
		LOGG_ERR("%s", e.what());
	}
 
	return ret;
}
 
bool CFTPManager::parseResponse(const string &str)
{
	if(str.empty()) 
	{
		return false;
	}
 
	string strData = str.substr(0, 3);
	int val = atoi(strData.c_str());

	return (val>0 ? true : false);
}

void CFTPManager::downloadProgress(int datasize)
{
	try
	{
		const char *lable = "|/-\\";
		m_progress.block_size += datasize;
		int percent = ((double)m_progress.block_size / (double)m_progress.total_size) * 100;
		memset(m_progress.bar, '#', percent);

		printf("[%-100s][%d%%][%c]\r", m_progress.bar, percent, lable[percent%4]);
		fflush(stdout);
		m_progress.bar[percent] = '#';
	}
	catch(const std::exception& e)
	{
		LOGG_ERR("%s", e.what());
	}
}

static vector<string> split(const char *s, const char *delim)
{
    vector<string> result;

	try
	{
		if (s && strlen(s))
		{
			int len = strlen(s);
			char *src = new char[len + 1];
			strcpy(src, s);
			src[len] = '\0';
			char *tokenptr = strtok(src, delim);
			while (tokenptr != NULL)
			{
				string tk = tokenptr;
				result.emplace_back(tk);
				tokenptr = strtok(NULL, delim);
			}
			delete[] src;
		}
	}
	catch(const std::exception& e)
	{
		LOGG_ERR("%s", e.what());
	}

    return result;
}

static string getModule(string filename)
{
	string file = "";
    vector<string> nameSplit = split(filename.c_str(), "/");
    string tmp = nameSplit.back();
    vector<string> fileSplit = split(tmp.c_str(), ".");
    file = fileSplit.front();

    return file;
}

/* bind device for ftp */
bool CFTPManager::bindToDevice(int &socket_fd)
{
    int rc = -1;

    if(!m_deviceName.empty())
    {
        rc = setsockopt(socket_fd, SOL_SOCKET, SO_BINDTODEVICE, m_deviceName.c_str(), 
			m_deviceName.length());

        if (rc < 0) 
        {
            LOGG_ERR("bind to %s failed, errno=%d %s", m_deviceName.c_str(), errno, 
				strerror(errno));
            return false;
        }
    }

    return true;
}
