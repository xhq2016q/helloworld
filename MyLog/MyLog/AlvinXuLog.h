#pragma once
//仿写一个日志文件
#define FILENAME_STRING_LENGTH 256
class CAlvinXuLog
{
public:					//静态工具函数类
	static int MakeTimeString(char* szBuffer, int nBuffer, int nBufferSize);

public:														//构造函数和析构函数
	CAlvinXuLog(CAlvinLowDebug* pDebug,					//debug对象指针(log也需要debug)
				CAlvinMemoryPoolWithLock* pMemPool,		//内存池指针，内存队列要用
				char* szLogPath,						//日志路径
				char* szAppName,						//应用名（修饰日志文件）
				int nHoldFileMax=LOF_FILE_DEFAULT_HOLD,	//保留多少个文件
				bool bSyslogFlag=true,					//日志级别开关
				bool bDebug1Flag=true,
				bool bDebug2Flag=false,
				bool bDebug3Flag=false,
				bool bPrintf2ScrFlag=true);				//是否打印到屏幕的开关
	~CAlvinXuLog(void);									//析构函数

public:													//公有输出方法
	void _AlvinDebug4Bin(char* pBuffer, int nLength);	//二进制输出
	int _AlvinSyslog(char *szFormat, ...);				//Syslog输出，变参函数
	int _AlvinDebug1(char *szFormat, ...);				//Debug1输出，变参函数
	int _AlvinDebug2(char *szFormat, ...);				//Debug2输出，变参函数
	int _AlvinDebug3(char *szFormat, ...);				//Debug3输出，变参函数

public:													//开关变量，对应构造函数的开关
	bool m_bSyslogFlag;
	bool m_bDebug1Flag;
	bool m_bDebug2Flag;
	bool m_bDebug3Flag;

private:												//内部功能函数
	int _Printf(char *szFormat, ...);					//最核心的打印输出模块，变参函数
	void DeleteFirstFile(void);							//删除最老的文件
	void FixFileInfo(void);								//修订文件名目录队列
	void MakeFileName(void);							//根据时间和文件大小，定制文件名
private://内部私有变量区
	CMutexLock m_Lock;									//线程安全锁
	char m_szFilePath(FILENAME_STRING_LENGTH);			//文件路径
	char m_szFileName[(FILENAME_STRING_LENGTH*2)];		//文件名
	unsigned long m_nFileSize;							//当前文件大小
	time_t m_tFileNameMake;								//定制文件名的时间戳
	int m_nHoldFileMax;									//保留文件的数量，构造函数传入
	_APP_INFO_OUT_CALLBACK m_pInfoOutCallback;			//应用程序拦截输出回调函数
	void* m_pInfoOutCallbackParam;						//透传的回调函数的指针
	bool m_bPrintf2ScrFlag;								//是否答印到屏幕的开关
	char m_szFileInfoName[FILENAME_STRING_LENGTH];		//文件名
	CAlvinLowDebug* m_pDebug;							//Debug对象指针
	CAlvinMemoryPoolWithLock* m_pMemPool;				//内存池对象指针
	CAlvinXuMemoryQueue* m_pFileInfoQueue;				//文件名队列
};

int SafePrintf(char* szBuf, int nMaxLength, char* szFormat, ...);
#if WIN32
#define PATH_CHAR "\\"									//Windows 下使用'\'作为路径的间隔符
#else
#define PATH_CHAR "/"									//Unix,Linux使用'/'
#endif

//参数表
//path:路径
//name:主文件名(file.ext中的file)
//fullname:最后生成的全文件名缓冲区指针
#define FULL_NAME(path, name, fullname, ext_name)\
{\
	if (strlen(path))\
	{\
		if (strlen(ext_name))\
			SafePrintf(fullname, FILENAME_STRING_LENGTH,\
			"%s%s%s.%s", path, PATH_CHAR, name, ext_name);\
		else\
			SafePrintf(fullname, FILENAME_STRING_LENGTH,\
			"%s%s%s", path, PATH_CHAR, name);\
	}\ 
	else\
	{\
		if (strlen(ext_name))\
			SafePrintf(fullname, FILENAME_STRING_LENGTH,\
			"%s.%s", name, ext_name);\
		else\
			SafePrintf(fullname, FILENAME_STRING_LENGTH, "%s", name);\
	}\
}