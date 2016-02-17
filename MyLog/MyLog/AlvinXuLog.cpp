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