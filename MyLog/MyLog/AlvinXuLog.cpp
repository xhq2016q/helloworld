#include "StdAfx.h"
#include "AlvinXuLog.h"


CAlvinXuLog::CAlvinXuLog(CAlvinLowDebug* pDebug, /*debug对象指针(log也需要debug) */ 
						 CAlvinMemoryPoolWithLock* pMemPool, /*内存池指针，内存队列要用 */ 
						 char* szLogPath, /*日志路径 */ 
						 char* szAppName, /*应用名（修饰日志文件） */ 
						 int nHoldFileMax/*=LOF_FILE_DEFAULT_HOLD*/, /*保留多少个文件 */ 
						 bool bSyslogFlag/*=true*/, /*日志级别开关 */ 
						 bool bDebug1Flag/*=true*/, 
						 bool bDebug2Flag/*=false*/, 
						 bool bDebug3Flag/*=false*/, 
						 bool bPrintf2ScrFlag/*=true*/)
{
	m_pDebug = pDebug;
	m_pMemPool = pMemPool;
	//请注意，这里从DEBUG对象中获取拦截回调函数的信息
	m_pInfoOutCallback = m_pDebug->m_pInfoOutCallback;
	m_pInfoOutCallbackParam = m_pDebug->m_pInfoOutCallbackParam;	//利用debug输出启动日志
	ALVIN_DEBUG("CAlvinXuLog:Start!\n");
	//获得日志文件名基准字符串，这里主要使用输入的路径名和应用名生成基本名
	//如路径是"/var",应用名是"test_project",
	//则基准名为 "/var/test_project",
	//这样，以后的文件名，就是在这个基本名后加时间戳实现
	//如：/var/test_project_Thu_16_14_31_44_2009.log
	FULL_NAME(szLogPath, szAppName, m_szFileInfoName, "info");
	//清空当前文件名缓冲区
	TONY_CLEAN_CHAR_BUFFER(m_szFileName);
	//当前文件尺寸设置为0
	m_nFileSize = 0;
	m_bSyslogFlag = bSyslogFlag;			//保存Debug级别开关变量
	m_bDebug1Flag = bDebug1Flag;
	m_bDebug2Flag = bDebug2Flag;
	m_bDebug3Flag = bDebug3Flag;
	m_bPrintf2ScrFlag = bPrintf2ScrFlag;	//保存屏幕输出开关
	m_nHoldFileMax = nHoldFileMax;			//保存最大保留文件个数
	m_pFileInfoQueue = new CAlvinXuMemoryQueue(		//实例化文件目录队列对象
		pDebug,
		m_pMemPool,
		"CAlvinXuLog::m_pFileInfoQueue");
	if (m_pFileInfoQueue)				//如果创建成功，注册到内存池
	{
		m_pMemPool->Register(m_pFileInfoQueue, 
			"CAlvinXuLog::m_pFileInfoQueue");
	}
	m_pFileInfoQueue->ReadFromFile(m_szFileInfoName);	//读入上次保留的文件名信息
	MakeFileName();										//根据当前时间戳，定制一个文件名
}

CAlvinXuLog::~CAlvinXuLog(void)
{
	if (m_pFileInfoQueue)								//清楚文件目录队列对象
	{
		m_pFileInfoQueue->Write2File(m_szFileInfoName);	//清除前先保留到磁盘
		m_pMemPool->UnRegister(m_pFileInfoQueue);		//反注册对象指针
		delete m_pFileInfoQueue;						//删除对象
		m_pFileInfoQueue = NULL;
	}
	ALVIN_DEBUG(CAlvinXuLog:Stop!\n);					//Debug输出
}


#ifdef WIN32
#define Linux_Win_vsnprintf _vsnprintf
#else //not WIN32
#define Linux_Win_vsnprintf vsnprintf
#endif

//安全的变参输出函数
//szBuf:用户指定的输出缓冲区
//nMaxLength:用户指定的输出缓冲区尺寸
//szFormat:格式化输出字符串（变参，可以是多个）
//返回输出的字符总数（strlen的长度，不包括最后的'\0'）
int SafePrintf(char* szBuf, int nMaxLength, char* szFormat, ...)
{
	int nListCount = 0;
	va_list pArglist;
	//此处做安全性防护，防止用户输入非法的缓冲区，导致程序在此崩溃。
	if(!szBuf)goto SafePrintf_END_PROCESS;
	//此处开启系统循环，解析每条格式化输出字符串
	va_start(pArglist, szFormat);
	nListCount += Linux_Win_vsnprintf(szBuf + nListCount,
		nMaxLength - nListCount, szFormat, pArglist);
	va_end(pArglist);
	//实现缓冲区超限保护
	if (nListCount > (nMaxLength - 1)) nListCount = nMaxLength - 1;
	//人工添加'\0'，确保输出100%标准C字符串
	*(szBuf + nListCount) = '\0';
SafePrintf_END_PROCESS:
	return nListCount;

}

void CAlvinXuLog::MakeFileName(void)						//创造一个新的文件名
{
	char szTemp[LOG_ITEM_LENGTH_MAX];						//临时缓冲区
	MakeATimeString(szTemp, LOG_ITEM_LENGTH_MAX);			//获得时间戳字符串
	FixFileInfo();											//维护文件总个数不超标(默认72个)
	int nLen = SafePrintf(									//注意看这句，利用构造函数中的种子名字
		m_szFileName,										//加上时间戳，后面再加上".log"后缀
		FILENAME_STRING_LENGTH*2,							//生成日志文件名
		"%s_%s.log",
		m_szFilePath,
		szTemp);
	nLen++;													//保留最后'\0'的位置
	//将新的文件名添加到队列
	int nAddLastRet = m_pFileInfoQueue->AddLast(m_szFileName, nLen);
	if (0 >= nAddLastRet)
	{	//这是一个特殊的防护，如果队列满了(内存不够用)，删除最开始的三个文件名
		//释放内存空间，这是预防服务器业务太繁忙，导致内存不够用，队列无法添加的
		//规避措施，这也体现非关键模块为关键业务模块让路的思维
		DeleteFirstFile();
		DeleteFirstFile();
		DeleteFirstFile();
		//删除三个之后尝试重新添加
		nAddLastRet = m_pFileInfoQueue->AddLast(m_szFileName, nLen);
		//如果此时添加仍然失败，投降，日志发生一点错乱没有关系。
	}
	m_nFileSize = 0;							//新文件创建，文件长度为0
	//下面逻辑，新创建一个文件，在文件头先打印一点文件名相关信息，帮助以后的跟踪查找
	time(&m_tFileNameMake);
	{	//由于这是非业务打印，因此不希望输出到屏幕，这里临时将屏幕开关关闭
		bool bPrintf2Scr = m_bPrintf2ScrFlag;
		m_bPrintf2ScrFlag = false;
		_Printf("Alvin.Xu. base libeary log file %s\n", m_szFileName);
		_Printf("-----------------------------------\n");
		m_bPrintf2ScrFlag = bPrintf2Scr;			//输出完毕屏幕开关恢复原值
	}
}

void CAlvinXuLog::GetFileName(void)//获取当前文件名
{
	time_t tNow;				//当前时间戳变量
	unsigned long ulDeltaT = 0; //t变量
	if('\0' == m_szFileName[0])		//如果第一次启动文件名为空
	{
		MakeFileName();
		goto CAlvinXuLog_GetFileName_End_Process;
	}
	time(&tNow);					//求得当前时间
	ulDeltaT = (unsigned long)tNow - m_tFileNameMake;	//计算t
	if (LOG_FILE_CHANGE_NAME_PRE_SECONDS <= ulDeltaT)
	{
		MakeFileName();				//如果t超过3600秒，创造文件名返回
		goto CAlvinXuLog_GetFileName_End_Process;
	}
CAlvinXuLog_GetFileName_End_Process:
	return;
}

int CAlvinXuLog::MakeATimeString(char* szBuffer, int nBufferSize)
{
	int i = 0;
	time_t t;
	struct tm *pTM = NULL;
	int nLength = 0;
	if (LOG_FILE_SIZE_MAX > nBufferSize)		//防御性设计
		goto CAlvinXuLog_MakeATimeString_End_Process;
	time(&t);									//获得当前时间
	pTM = localtime(&t);						//或得当前时区的时间戳字符串
	nLength = SafePrintf(szBuffer, LOG_ITEM_LENGTH_MAX, "%s", asctime(pTM));	//时间戳字符串入缓冲区
	//localtime生成的字符串最后自带一个'\n'，即回车符，这不利于后续的打印
	//因此下面这行向前退一格清除掉这个回车符。这是一个小经验。
	szBuffer[nLength - 1] = '\0';
	//文件名有一定的限制，一定不要有空格，':'字符这里可以过滤一下，
	//将时间戳中的非法字符都改变成各系统都能接受的'_'下划线
	for(i = 0; i < nLength; i++)
	{
		if (' ' == szBuffer[i])szBuffer[i] = '_';
		if (':' == szBuffer[i])szBuffer[i] = '_';
	}		
CAlvinXuLog_MakeATimeString_End_Process:
	return nLength;
}

void CAlvinXuLog::FixFileInfo(void)			//维护文件总个数不超标
{
	int nAddLastRet = 0;
	//请注意，这里不是if，而是一个while，如果因某种原因，超标很多文件
	//利用这个循环技巧，很轻松的将超标文件删到只有72个，
	//很多时候，维护数组不超限，都是使用这个技巧
	while(m_pFileInfoQueue->GetTokenCount() >= m_nHoldFileMax)
	{
		DeleteFirstFile();
	}
}

void CAlvinXuLog::DeleteFirstFile(void)						//删除第一个文件
{
	char szFirstFile[FILENAME_STRING_LENGTH];				//文件名缓冲区
	int nFirstFileNameLen = 0;								//文件名字符串长度
	//从文件名数组中，弹出第一个文件名
	nFirstFileNameLen = m_pFileInfoQueue->GetAndDeleteFirst(szFirstFile, FILENAME_STRING_LENGTH);
	if (0 >= nFirstFileNameLen)								//失败返回
	goto CAlvinXuLog_DeleteFirstFile_End_Process;
CAlvinXuLog_DeleteFirstFile_End_Process:
	return;
}

int CAlvinXuLog::_AlvinSyslog(char *szFormat, ...)
{	//这段比较经典，变参函数处理模块，不再赘述
	char szBuf[LOG_ITEM_LENGTH_MAX];
	int nMaxLength = LOG_ITEM_LENGTH_MAX;
	int nListCount = 0;
	va_list pArgList;
	va_start(pArgList, szFormat);
	nListCount += Linux_Win_vsnprintf(szBuf + nListCount,
		nMaxLength - nListCount, szFormat, pArgList);
	va_end(pArgList);
	if (m_bSyslogFlag)				//如果开关打开
	{
		m_Lock.Lock();				//加锁
		{
			_Printf("%s", szBuf);	//真实的打印
		}
		m_Lock.Unlock();				//解锁
	}
	return nListCount;				//返回长度
}

int CAlvinXuLog::_AlvinDebug1(char *szFormat, ...)
{	//这段比较经典，变参函数处理模块，不再赘述
	char szBuf[LOG_ITEM_LENGTH_MAX];
	int nMaxLength = LOG_ITEM_LENGTH_MAX;
	int nListCount = 0;
	va_list pArgList;
	va_start(pArgList, szFormat);
	nListCount += Linux_Win_vsnprintf(szBuf+nListCount,
		nMaxLength - nListCount, szFormat, pArgList);
	va_end(pArgList);
	if(nListCount > (nMaxLength - 1)) nMaxLength - 1;
	*(szBuf + nListCount) = '\0';

	if (m_bDebug1Flag)				//如果开关打开
	{
		m_Lock.Lock();				//加锁
		{
			_Printf("%s", szBuf);	//真实的打印
		}
		m_Lock.Unlock();				//解锁
	}
	return nListCount;				//返回长度
}



int CAlvinXuLog::_AlvinDebug2(char *szFormat, ...)
{	//这段比较经典，变参函数处理模块，不再赘述
	char szBuf[LOG_ITEM_LENGTH_MAX];
	int nMaxLength = LOG_ITEM_LENGTH_MAX;
	int nListCount = 0;
	va_list pArgList;
	va_start(pArgList,szFormat);
	nListCount += Linux_Win_vsnprintf(szBuf + nListCount, nMaxLength - nListCount, szFormat, pArgList);
	va_end(pArgList);
	if (nListCount > (nMaxLength - 1))nListCount = nMaxLength - 1;
	*(szBuf + nListCount) = '\0';

	if(m_bDebug2Flag)						//如果开关打开
	{
		m_Lock.Lock();						//加锁
		{
			_Printf("%s", szBuf);			//真实的执行打印
		}
		m_Lock.Unlock();					//解锁
	}
	return nListCount;						//返回长度
}

int CAlvinXuLog::_AlvinDebug3(char *szFormat, ...)
{	//这段比较经典，变参函数处理模块，不再赘述
	char szBuf[LOG_ITEM_LENGTH_MAX];
	int nMaxLength = LOG_ITEM_LENGTH_MAX;
	int nListCount = 0;
	va_list pArgList;
	va_start(pArgList,szFormat);
	nListCount += Linux_Win_vsnprintf(szBuf + nListCount, nMaxLength - nListCount, szFormat, pArgList);
	va_end(pArgList);
	if (nListCount > (nMaxLength - 1))nListCount = nMaxLength - 1;
	*(szBuf + nListCount) = '\0';

	if(m_bDebug3Flag)						//如果开关打开
	{
		m_Lock.Lock();						//加锁
		{
			_Printf("%s", szBuf);			//真实的执行打印
		}
		m_Lock.Unlock();					//解锁
	}
	return nListCount;						//返回长度
}


void CAlvinXuLog::_AlvinDebug4Bin(char* pBuffer, int nLength)
{
	m_Lock.Lock();						//加锁
	{
		GetFileName();					//获取文件名
		dbg2file4bin(m_szFileName, "a+", pBuffer, nLength);		//输出到文件
		dbg_bin(pBuffer, nLength);		//输出到屏幕
	}
	m_Lock.Unlock();					//解锁
}

int CAlvinXuLog::_Printf(char *szFormat, ...)
{
	char szTime[LOG_ITEM_LENGTH_MAX];
	char szTemp[LOG_ITEM_LENGTH_MAX];
	char szBuf[LOG_ITEM_LENGTH_MAX];
	int nMaxLength = LOG_ITEM_LENGTH_MAX;
	int nListCount = 0;
	time_t t;
	struct tm *pTM = NULL;
	int nLength = 0;
	//获取当前时间戳，这在MakeATimeString中已经有介绍
	time(&t);
	pTM = localtime(&t);
	nLength = SafePrintf(szTemp, LOG_ITEM_LENGTH_MAX, "%s", asctime(pTM));
	szTemp[nLength - 1] = '\0';
	SafePrintf(szTime, LOG_ITEM_LENGTH_MAX, "[%s]", szTemp);
	//这段比较经典，变参函数处理模块，不再赘述
	char szBuf[LOG_ITEM_LENGTH_MAX];
	int nMaxLength = LOG_ITEM_LENGTH_MAX;
	int nListCount = 0;
	va_list pArgList;
	va_start(pArgList,szFormat);
	nListCount += Linux_Win_vsnprintf(szBuf + nListCount, nMaxLength - nListCount, szFormat, pArgList);
	va_end(pArgList);
	if (nListCount > (nMaxLength - 1))nListCount = nMaxLength - 1;
	*(szBuf + nListCount) = '\0';

	//得到当前使用的文件名
	GetFileName();
	//调用debug的功能函数，直接将信息输出到文件
	nListCount = dbg2file(m_szFileName, "a+", "%s%s", szTime, szBuf);
	if (m_bPrintf2ScrFlag)					//如果屏幕输出开关打开
	{
		ALVIN_XU_PRINTF("%s%s", szTime, szBuf);		//输出到屏幕
	}
	if(m_pInfoOutCallback)					//如果拦截函数存在
	{										//输出到拦截函数
		char szInfoOut[APP_INFO_OIT_STRING_MAX];
		SafePrintf(szInfoOut, APP_INFO_OIT_STRING_MAX, "%s%s", szTime, szBuf);
		m_pInfoOutCallback(szInfoOut, m_pInfoOutCallbackParam);
	}
	m_nFileSize += nListCount;				//这里很重要，修订文件长度
											//维护模块需要这个值判定文件大小是否超标
	return nListCount;						//返回输出的字节数
}

//向指定的缓冲区输出一个时间戳字符串
//szFileName:用户指定的输出文件
//szMode:常见的文件打开方式描述字符串，一般建议"a+"
//返回输出字符总数(strlen的长度，不包括最后的'\0')
int dbg2file(char* szFileName, char* szMode, char *szFormat, ...)
{
	//前一段和SafePrintf几乎一模一样
	char szBuf[DEBUG_BUFFER_LENGTH];
	char szTime[256];
	int nListCount = 0;
	va_list pArgList;
	va_start(pArgList, szFormat);
	nListCount += Linux_Win_vsnprintf(szBuf + nListCount, DEBUG_BUFFER_LENGTH + nListCount, szFormat, pArgList);
	va_end(pArgList);
	if (nListCount > (DEBUG_BUFFER_LENGTH - 1))
		nListCount = DEBUG_BUFFER_LENGTH - 1;
	*(szBuf + nListCount) = '\0';
	//在此开始正式的输出到各个目标设备
	GetATimeStamp(szTime, 256);
	FILE *fp;
	fp = fopen(szFileName, szMode);
	if (fp)
	{
		nListCount = fprintf(fp, "[%s]%s", szTime, szBuf);		//文件打印
		CON_PRINTF("[%s]%s", szTime, szBuf);					//屏幕打印
		fclose(fp);
	}
	else
		nListCount = 0;
	return nListCount;
}