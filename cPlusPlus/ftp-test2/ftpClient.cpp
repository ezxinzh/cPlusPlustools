#include "ftpClient.h"
 
static int SplitString( std::string strSrc, std::list<std::string> &strArray , std::string strFlag)
{
	int pos = 1; 
 
	while((pos = (int)strSrc.find_first_of(strFlag.c_str())) > 0) 
	{
		strArray.insert(strArray.end(), strSrc.substr(0 , pos));
		strSrc = strSrc.substr(pos + 1, strSrc.length() - pos - 1); 
	}
 
	strArray.insert(strArray.end(), strSrc.substr(0, strSrc.length()));
 
	return 0; 
}
 
CFTPManager::CFTPManager(string localPath, string deviceName): 
    m_bLogin(false)
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
	std::string strCmdLine = parseCommand(FTP_COMMAND_QUIT, "");
 
	Send(m_cmdSocket, strCmdLine.c_str());
	close(m_cmdSocket);
	m_bLogin = false;
}
 
FTP_API CFTPManager::login2Server(const std::string &serverIP)
{
	std::string strPort;
	int pos = serverIP.find_first_of(":");
 
	if (pos > 0)
	{
		strPort = serverIP.substr(pos + 1, serverIP.length() - pos);
	}
	else
	{
		pos = serverIP.length();
		strPort = FTP_DEFAULT_PORT;
	}
 
	m_strServerIP = serverIP.substr(0, pos);
	m_nServerPort = atol(strPort.c_str());
 
	LOGG_DEBUG("IP: %s port: %d\n", m_strServerIP.c_str(), m_nServerPort);
 
	if (Connect(m_cmdSocket, m_strServerIP, m_nServerPort) < 0)
	{
		
		return -1;
	}
	
	m_strResponse = serverResponse(m_cmdSocket);
	LOGG_DEBUG("@@@@Response: %s", m_strResponse.c_str());
 
	return	parseResponse(m_strResponse);
}
 
FTP_API CFTPManager::inputUserName(const std::string &userName)
{
	std::string strCommandLine = parseCommand(FTP_COMMAND_USERNAME, userName);
 
	m_strUserName = userName;
 
	if (Send(m_cmdSocket, strCommandLine) < 0)
	{
		return -1;
	}
 
	m_strResponse = serverResponse(m_cmdSocket);
	LOGG_DEBUG("Response: %s\n", m_strResponse.c_str());
 
	return parseResponse(m_strResponse);
}
 
FTP_API CFTPManager::inputPassWord(const std::string &password)
{
	std::string strCmdLine = parseCommand(FTP_COMMAND_PASSWORD, password);
 
	m_strPassWord = password;
	if (Send(m_cmdSocket, strCmdLine) < 0)
	{
		return -1;
	}
	else
	{
		m_bLogin = true;
 
		m_strResponse = serverResponse(m_cmdSocket);
		LOGG_DEBUG("Response: %s\n", m_strResponse.c_str());
 
		return parseResponse(m_strResponse);
	}
}
 
FTP_API CFTPManager::quitServer(void)
{
	std::string strCmdLine = parseCommand(FTP_COMMAND_QUIT, "");
	if (Send(m_cmdSocket, strCmdLine) < 0)
	{
		return -1;
	}
	else
	{
		m_strResponse = serverResponse(m_cmdSocket);
		LOGG_DEBUG("Response: %s\n", m_strResponse.c_str());
 
		return parseResponse(m_strResponse);
	}
 
}
 
const std::string CFTPManager::PWD()
{
	std::string strCmdLine = parseCommand(FTP_COMMAND_CURRENT_PATH, "");
 
	if (Send(m_cmdSocket, strCmdLine.c_str()) < 0)
	{
		return "";
	}
	else
	{
		return serverResponse(m_cmdSocket);
	}
}
 
 
FTP_API CFTPManager::setTransferMode(type mode)
{
	std::string strCmdLine;
 
	switch (mode)
	{
	case binary:
		strCmdLine = parseCommand(FTP_COMMAND_TYPE_MODE, "I");
		break;
	case ascii:
		strCmdLine = parseCommand(FTP_COMMAND_TYPE_MODE, "A");
		break;
	default:
		break;
	}
 
	if (Send(m_cmdSocket, strCmdLine.c_str()) < 0)
	{
		return -1;
	}
	else
	{	
		m_strResponse  = serverResponse(m_cmdSocket);
		LOGG_DEBUG("@@@@Response: %s", m_strResponse.c_str());
 
		return parseResponse(m_strResponse);
	}
}
 
 
const std::string CFTPManager::Pasv()
{
	std::string strCmdLine = parseCommand(FTP_COMMAND_PSAV_MODE, "");
 
	if (Send(m_cmdSocket, strCmdLine.c_str()) < 0)
	{
		return "";
	}
	else
	{
		m_strResponse = serverResponse(m_cmdSocket);
 
		return m_strResponse;
	}
}
 
 
const std::string CFTPManager::Dir(const std::string &path)
{
	int dataSocket = socket(AF_INET, SOCK_STREAM, 0);
    bindToDevice(dataSocket);
 
	if (createDataLink(dataSocket) < 0)
	{
		return "";
	}

	std::string strCmdLine = parseCommand(FTP_COMMAND_DIR, path);
 
	if (Send(m_cmdSocket, strCmdLine) < 0)
	{
		LOGG_DEBUG("@@@@Response: %s\n", serverResponse(m_cmdSocket).c_str());
		close(dataSocket);
		return "";
	}
	else
	{
		LOGG_DEBUG("@@@@Response: %s\n", serverResponse(m_cmdSocket).c_str());
		m_strResponse = serverResponse(dataSocket);
 
		LOGG_DEBUG("@@@@Response: \n%s\n", m_strResponse.c_str());
		close(dataSocket);
 
		return m_strResponse;
	}
	
}
 
 
FTP_API CFTPManager::CD(const std::string &path)
{
	if(m_cmdSocket == INVALID_SOCKET) return -1;
 
	std::string strCmdLine = parseCommand(FTP_COMMAND_CHANGE_DIRECTORY, path);
 
	if (Send(m_cmdSocket, strCmdLine) < 0)
	{
		return -1;
	}
		
	m_strResponse = serverResponse(m_cmdSocket);
	
	LOGG_DEBUG("@@@@Response: %s\n", m_strResponse.c_str());
	return parseResponse(m_strResponse);
}
 
FTP_API CFTPManager::DeleteFile(const std::string &strRemoteFile)
{
	if(m_cmdSocket == INVALID_SOCKET) return -1;
 
	std::string strCmdLine = parseCommand(FTP_COMMAND_DELETE_FILE, strRemoteFile);
 
	if (Send(m_cmdSocket, strCmdLine) < 0)
	{
		return -1;
	}
 
	m_strResponse = serverResponse(m_cmdSocket);
	LOGG_DEBUG("@@@@Response: %s\n", m_strResponse.c_str());
	return parseResponse(m_strResponse);
}
 
FTP_API CFTPManager::DeleteDirectory(const std::string &strRemoteDir)
{
	if(m_cmdSocket == INVALID_SOCKET) return -1;
 
	std::string strCmdLine = parseCommand(FTP_COMMAND_DELETE_DIRECTORY, strRemoteDir);
 
	if (Send(m_cmdSocket, strCmdLine) < 0)
	{
		return -1;
	}
	
	m_strResponse = serverResponse(m_cmdSocket);
 
	LOGG_DEBUG("@@@@Response: %s\n", m_strResponse.c_str());
	return parseResponse(m_strResponse);
}
 
FTP_API CFTPManager::CreateDirectory(const std::string &strRemoteDir)
{
	if(m_cmdSocket == INVALID_SOCKET) return -1;
 
	std::string strCmdLine = parseCommand(FTP_COMMAND_CREATE_DIRECTORY, strRemoteDir);
 
	if (Send(m_cmdSocket, strCmdLine) < 0)
	{
		return -1;
	}
	
	m_strResponse = serverResponse(m_cmdSocket);
 
	LOGG_DEBUG("@@@@Response: %s\n", m_strResponse.c_str());
	return parseResponse(m_strResponse);
}
 
FTP_API CFTPManager::Rename(const std::string &strRemoteFile, const std::string &strNewFile)
{
	if(m_cmdSocket == INVALID_SOCKET) return -1;
 
	std::string strCmdLine = parseCommand(FTP_COMMAND_RENAME_BEGIN, strRemoteFile);
	Send(m_cmdSocket, strCmdLine);
	LOGG_DEBUG("@@@@Response: %s\n", serverResponse(m_cmdSocket).c_str());
 
	Send(m_cmdSocket, parseCommand(FTP_COMMAND_RENAME_END, strNewFile));
 
	m_strResponse = serverResponse(m_cmdSocket);
	LOGG_DEBUG("@@@@Response: %s\n", m_strResponse.c_str());
	return parseResponse(m_strResponse);
}
 
long CFTPManager::getFileLength(const std::string &strRemoteFile)
{
	if(m_cmdSocket == INVALID_SOCKET) return -1;
 
	std::string strCmdLine = parseCommand(FTP_COMMAND_FILE_SIZE, strRemoteFile);
 
	if (Send(m_cmdSocket, strCmdLine) < 0)
	{
		return -1;
	}
 
	m_strResponse = serverResponse(m_cmdSocket);
 
	LOGG_DEBUG("@@@@Response: %s\n", m_strResponse.c_str());
 
	std::string strData = m_strResponse.substr(0, 3);
	unsigned long val = atol(strData.c_str());
 
	if (val == 213)
	{
		strData = m_strResponse.substr(4);
		LOGG_DEBUG("strData: %s\n", strData.c_str());
		val = atol(strData.c_str());
 
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
 
FTP_API CFTPManager::Get(const std::string &strRemoteFile, const std::string &strLocalFile)
{
	return downLoad(strRemoteFile, strLocalFile);
}
 
 
FTP_API CFTPManager::Put(const std::string &strRemoteFile, const std::string &strLocalFile)
{
	std::string strCmdLine;
	const unsigned long dataLen = FTP_DEFAULT_BUFFER;
	char strBuf[dataLen] = {0};
	unsigned long nSize = getFileLength(strRemoteFile);
	unsigned long nLen = 0;
 
	FILE *pFile = fopen(strLocalFile.c_str(), "rb"); 
	if(pFile == NULL) return -1;
 
	int data_fd = socket(AF_INET, SOCK_STREAM, 0);
	if(data_fd == -1) return -1;
    bindToDevice(data_fd);
 
	if (createDataLink(data_fd) < 0)
	{
		return -1;
	}
	
	if (nSize == -1)
	{
		strCmdLine = parseCommand(FTP_COMMAND_UPLOAD_FILE, strRemoteFile);
	}
	else
	{
		strCmdLine = parseCommand(FTP_COMMAND_APPEND_FILE, strRemoteFile);
	}
 
	if (Send(m_cmdSocket, strCmdLine) < 0)
	{
		Close(data_fd);
		return -1;
	}
 
	LOGG_DEBUG("@@@@Response: %s\n", serverResponse(m_cmdSocket).c_str());
	fseek(pFile, nSize, SEEK_SET);

	while (!feof(pFile))
	{
		nLen = fread(strBuf, 1, dataLen, pFile);
		if (nLen < 0)
		{
			break;
		}
 
		if (Send(data_fd, strBuf) < 0)
		{
			Close(data_fd);
			return -1;
		}
	}
 
	LOGG_DEBUG("@@@@Response: %s\n", serverResponse(data_fd).c_str());
 
	Close(data_fd);
	LOGG_DEBUG("@@@@Response: %s\n", serverResponse(m_cmdSocket).c_str());
	fclose(pFile);
 
	return 0;
}
 
const std::string CFTPManager::parseCommand(const unsigned int command, const std::string &strParam)
{
	if (command < FTP_COMMAND_BASE || command > FTP_COMMAND_END)
	{
		return "";
	}
 
	std::string strCommandLine;
 
	m_nCurrentCommand = command;
	m_commandStr.clear();
 
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
	strCommandLine += "\r\n";
 
	m_commandStr = strCommandLine;
	LOGG_DEBUG("parseCommand: %s\n", m_commandStr.c_str());
 
	return m_commandStr;
}
 
FTP_API CFTPManager::Connect(int socketfd, const std::string &serverIP, unsigned int nPort)
{
	if (socketfd == INVALID_SOCKET)
	{
		return -1;
	}
 
	unsigned int argp = 1;
	int error = -1;
	int len = sizeof(int);
	struct sockaddr_in  addr;
	bool ret = false;
	timeval stime;
	fd_set  set;
 
	ioctl(socketfd, FIONBIO, &argp);  
 
	memset(&addr, 0, sizeof(struct sockaddr_in));
	addr.sin_family = AF_INET;
	addr.sin_port	= htons(nPort);
	addr.sin_addr.s_addr = inet_addr(serverIP.c_str());
	bzero(&(addr.sin_zero), 8);
 
	LOGG_DEBUG("Address: %s %d\n", inet_ntoa(addr.sin_addr), ntohs(addr.sin_port));
	
	if (connect(socketfd, (struct sockaddr*)&addr, sizeof(struct sockaddr)) == -1)   
	{
		stime.tv_sec = 20;  
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
				ret = false;
			}
		}
	}
	else
	{	LOGG_DEBUG("Connect Immediately!!!\n");
		ret = true;
	}
 
	argp = 0;
	ioctl(socketfd, FIONBIO, &argp);
 
	if (!ret)
	{
		close(socketfd);
		LOGG_ERR("cannot connect server!!\n");
		return -1;
	}
 
	return 0;
}
 
 
const std::string CFTPManager::serverResponse(int sockfd)
{
	if (sockfd == INVALID_SOCKET)
	{
		return "";
	}
	
	int nRet = -1;
	char buf[MAX_PATH] = {0};
 
	m_strResponse.clear();
 
	while ((nRet = getData(sockfd, buf, MAX_PATH)) > 0)
	{
		buf[MAX_PATH - 1] = '\0';
		m_strResponse += buf;
	}
 
	return m_strResponse;
}
 
FTP_API CFTPManager::getData(int fd, char *strBuf, unsigned long length)
{
	if(strBuf == NULL) return -1;
 
	if (fd == INVALID_SOCKET)
	{
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
			return -2;
		}
	}
	return 0;
}
 
FTP_API CFTPManager::Send(int fd, const std::string &cmd)
{
	if (fd == INVALID_SOCKET)
	{
		return -1;
	}
 
	return Send(fd, cmd.c_str(), cmd.length());
}
 
FTP_API CFTPManager::Send(int fd, const char *cmd, const size_t len)
{
    if((FTP_COMMAND_USERNAME != m_nCurrentCommand)
        &&(FTP_COMMAND_PASSWORD != m_nCurrentCommand)
        &&(!m_bLogin))
	{
		return -1;
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
		while (nlen >0) 
		{
			nSendLen = send(fd, cmd , (int)nlen , 0);
 
			if(nSendLen == -1) 
				return -2; 
 
			nlen = nlen - nSendLen;
			cmd +=  nSendLen;
		}
		return 0;
	}
	return -1;
}
 
 
FTP_API CFTPManager::createDataLink(int data_fd)
{
	if(data_fd == INVALID_SOCKET) return -1;
 
	std::string strData;
	unsigned long nPort = 0 ;
	std::string strServerIp ; 
	std::list<std::string> strArray ;
 
	std::string parseStr = Pasv();
 
	if (parseStr.size() <= 0)
	{
		return -1;
	}
 
	//trace("parseInfo: %s\n", parseStr.c_str());
 
	size_t nBegin = parseStr.find_first_of("(");
	size_t nEnd	  = parseStr.find_first_of(")");
	strData		  = parseStr.substr(nBegin + 1, nEnd - nBegin - 1);
 
	//trace("ParseAfter: %s\n", strData.c_str());
	if( SplitString( strData , strArray , "," ) <0)
		return -1;
 
	if( ParseString( strArray , nPort , strServerIp) < 0)
		return -1;
 
	//trace("nPort: %ld IP: %s\n", nPort, strServerIp.c_str());
 
	if (Connect(data_fd, strServerIp, nPort) < 0)
	{
		return -1;
	}
 
	return 0;
 
}
 
FTP_API CFTPManager::ParseString(std::list<std::string> strArray, unsigned long & nPort ,std::string & strServerIp)
{
	if (strArray.size() < 6 )
		return -1 ;
 
	std::list<std::string>::iterator citor;
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
	return 0 ; 
}
 
FILE *CFTPManager::createLocalFile(const std::string &strLocalFile)
{
	return fopen(strLocalFile.c_str(), "w+b");
}
 
FTP_API CFTPManager::downLoad(const std::string &strRemoteFile, const std::string &strLocalFile, 
    const int pos, const unsigned int length)
{
    int ret = -1;
	if(length < 0) return -1;

    memset(&m_progress, 0, sizeof(progress_t));
    m_progress.total_size = getFileLength(strRemoteFile);
//    LOGG_INFO("============= total_size: '%d' of file: '%s'", m_progress.total_size, strRemoteFile.c_str());
 
	FILE *file = NULL;
	unsigned long nDataLen = FTP_DEFAULT_BUFFER;
	char strPos[MAX_PATH]  = {0};
	int data_fd = socket(AF_INET, SOCK_STREAM, 0);
    bindToDevice(data_fd);
	
	if(data_fd == -1) return -1;
 
	if ((length != 0) && (length < nDataLen))
	{
		nDataLen = length;
	}
	char *dataBuf = new char[nDataLen];
	if(dataBuf == NULL) return -1;
 
	sprintf(strPos, "%d", pos);
 
	if (createDataLink(data_fd) < 0)
	{
	    LOGG_ERR("Create Data Link error!!!\n");
		return -1;
	}
 
	std::string strCmdLine = parseCommand(FTP_COMMAND_DOWNLOAD_POS, strPos);
	if (Send(m_cmdSocket, strCmdLine) < 0)
	{
	    LOGG_ERR("unknown error!");
		return -1;
	}
	LOGG_DEBUG("Response: %s\n", serverResponse(m_cmdSocket).c_str());
 
	strCmdLine = parseCommand(FTP_COMMAND_DOWNLOAD_FILE, strRemoteFile);
 
	if (Send(m_cmdSocket, strCmdLine) < 0)
	{
	    LOGG_ERR("unknown error!");
		return -1;
	}
	LOGG_DEBUG("Response: %s\n", serverResponse(m_cmdSocket).c_str());
 
	file = createLocalFile(std::string(m_localPath + strLocalFile));
	if(file == NULL) return -1;
	
	int len = 0;
	int nReceiveLen = 0;
	while ((len = getData(data_fd, dataBuf, nDataLen)) > 0)
	{
		nReceiveLen += len;
 
		int num = fwrite(dataBuf, 1, len, file);
		memset(dataBuf, 0, sizeof(dataBuf));
	
		if (nReceiveLen == length && length != 0) break;
 
		if ((nReceiveLen + nDataLen) > length  && length != 0)
		{
			delete []dataBuf;
			nDataLen = length - nReceiveLen;
			dataBuf = new char[nDataLen];
		}

        downloadProgress(len);
	}

	if(nReceiveLen > 1) ret = 0;

	Close(data_fd);
	fclose(file);
	delete []dataBuf;
 
	return ret;
}
 
FTP_API CFTPManager::parseResponse(const std::string &str)
{
	if(str.empty()) return -1;
 
	std::string strData = str.substr(0, 3);
	unsigned int val = atoi(strData.c_str());
 
	return val;
}

int CFTPManager::downloadProgress(int datasize)
{
    const char *lable = "|/-\\";
    m_progress.block_size += datasize;
    int percent = ((double)m_progress.block_size / (double)m_progress.total_size) * 100;
    memset(m_progress.bar, '#', percent);

    printf("[%-100s][%d%%][%c]\r", m_progress.bar, percent, lable[percent%4]);
    fflush(stdout);
    m_progress.bar[percent] = '#';

    return 0;
}

/*******************************************************************************
 * func: split a string with specified char
 * return: vector
 * *****************************************************************************/
static vector<string> split(const char *s, const char *delim)
{
    vector<string> result;
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
    return result;
}

string getModule(string filename)
{
    vector<string> nameSplit = split(filename.c_str(), "/");
    string file = nameSplit.back();
    vector<string> fileSplit = split(file.c_str(), ".");
    file = fileSplit.front();

    return file;
}

bool CFTPManager::bindToDevice(int &socket_fd)
{
    int rc = -1;

    //bind device for ftp
    if(!m_deviceName.empty())
    {
        rc = setsockopt(socket_fd, SOL_SOCKET, SO_BINDTODEVICE, m_deviceName.c_str(), m_deviceName.length());
        if (rc < 0) 
        {
            LOGG_ERR("bind to %s failed, errno=%d %s\n", m_deviceName.c_str(), errno, strerror(errno));
            return false;
        }
        else
        {
            LOGG_DEBUG("bind to %s success\n", m_deviceName.c_str());
        }
    }

    return true;
}
