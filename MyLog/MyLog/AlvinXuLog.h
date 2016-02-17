#pragma once
//��дһ����־�ļ�
#define FILENAME_STRING_LENGTH 256
class CAlvinXuLog
{
public:					//��̬���ߺ�����
	static int MakeTimeString(char* szBuffer, int nBuffer, int nBufferSize);

public:														//���캯������������
	CAlvinXuLog(CAlvinLowDebug* pDebug,					//debug����ָ��(logҲ��Ҫdebug)
				CAlvinMemoryPoolWithLock* pMemPool,		//�ڴ��ָ�룬�ڴ����Ҫ��
				char* szLogPath,						//��־·��
				char* szAppName,						//Ӧ������������־�ļ���
				int nHoldFileMax=LOF_FILE_DEFAULT_HOLD,	//�������ٸ��ļ�
				bool bSyslogFlag=true,					//��־���𿪹�
				bool bDebug1Flag=true,
				bool bDebug2Flag=false,
				bool bDebug3Flag=false,
				bool bPrintf2ScrFlag=true);				//�Ƿ��ӡ����Ļ�Ŀ���
	~CAlvinXuLog(void);									//��������

public:													//�����������
	void _AlvinDebug4Bin(char* pBuffer, int nLength);	//���������
	int _AlvinSyslog(char *szFormat, ...);				//Syslog�������κ���
	int _AlvinDebug1(char *szFormat, ...);				//Debug1�������κ���
	int _AlvinDebug2(char *szFormat, ...);				//Debug2�������κ���
	int _AlvinDebug3(char *szFormat, ...);				//Debug3�������κ���

public:													//���ر�������Ӧ���캯���Ŀ���
	bool m_bSyslogFlag;
	bool m_bDebug1Flag;
	bool m_bDebug2Flag;
	bool m_bDebug3Flag;

private:												//�ڲ����ܺ���
	int _Printf(char *szFormat, ...);					//����ĵĴ�ӡ���ģ�飬��κ���
	void DeleteFirstFile(void);							//ɾ�����ϵ��ļ�
	void FixFileInfo(void);								//�޶��ļ���Ŀ¼����
	void MakeFileName(void);							//����ʱ����ļ���С�������ļ���
private://�ڲ�˽�б�����
	CMutexLock m_Lock;									//�̰߳�ȫ��
	char m_szFilePath(FILENAME_STRING_LENGTH);			//�ļ�·��
	char m_szFileName[(FILENAME_STRING_LENGTH*2)];		//�ļ���
	unsigned long m_nFileSize;							//��ǰ�ļ���С
	time_t m_tFileNameMake;								//�����ļ�����ʱ���
	int m_nHoldFileMax;									//�����ļ������������캯������
	_APP_INFO_OUT_CALLBACK m_pInfoOutCallback;			//Ӧ�ó�����������ص�����
	void* m_pInfoOutCallbackParam;						//͸���Ļص�������ָ��
	bool m_bPrintf2ScrFlag;								//�Ƿ��ӡ����Ļ�Ŀ���
	char m_szFileInfoName[FILENAME_STRING_LENGTH];		//�ļ���
	CAlvinLowDebug* m_pDebug;							//Debug����ָ��
	CAlvinMemoryPoolWithLock* m_pMemPool;				//�ڴ�ض���ָ��
	CAlvinXuMemoryQueue* m_pFileInfoQueue;				//�ļ�������
};

int SafePrintf(char* szBuf, int nMaxLength, char* szFormat, ...);
#if WIN32
#define PATH_CHAR "\\"									//Windows ��ʹ��'\'��Ϊ·���ļ����
#else
#define PATH_CHAR "/"									//Unix,Linuxʹ��'/'
#endif

//������
//path:·��
//name:���ļ���(file.ext�е�file)
//fullname:������ɵ�ȫ�ļ���������ָ��
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