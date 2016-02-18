#include "StdAfx.h"
#include "AlvinXuLog.h"


CAlvinXuLog::CAlvinXuLog(CAlvinLowDebug* pDebug, /*debug����ָ��(logҲ��Ҫdebug) */ 
						 CAlvinMemoryPoolWithLock* pMemPool, /*�ڴ��ָ�룬�ڴ����Ҫ�� */ 
						 char* szLogPath, /*��־·�� */ 
						 char* szAppName, /*Ӧ������������־�ļ��� */ 
						 int nHoldFileMax/*=LOF_FILE_DEFAULT_HOLD*/, /*�������ٸ��ļ� */ 
						 bool bSyslogFlag/*=true*/, /*��־���𿪹� */ 
						 bool bDebug1Flag/*=true*/, 
						 bool bDebug2Flag/*=false*/, 
						 bool bDebug3Flag/*=false*/, 
						 bool bPrintf2ScrFlag/*=true*/)
{
	m_pDebug = pDebug;
	m_pMemPool = pMemPool;
	//��ע�⣬�����DEBUG�����л�ȡ���ػص���������Ϣ
	m_pInfoOutCallback = m_pDebug->m_pInfoOutCallback;
	m_pInfoOutCallbackParam = m_pDebug->m_pInfoOutCallbackParam;	//����debug���������־
	ALVIN_DEBUG("CAlvinXuLog:Start!\n");
	//�����־�ļ�����׼�ַ�����������Ҫʹ�������·������Ӧ�������ɻ�����
	//��·����"/var",Ӧ������"test_project",
	//���׼��Ϊ "/var/test_project",
	//�������Ժ���ļ�����������������������ʱ���ʵ��
	//�磺/var/test_project_Thu_16_14_31_44_2009.log
	FULL_NAME(szLogPath, szAppName, m_szFileInfoName, "info");
	//��յ�ǰ�ļ���������
	TONY_CLEAN_CHAR_BUFFER(m_szFileName);
	//��ǰ�ļ��ߴ�����Ϊ0
	m_nFileSize = 0;
	m_bSyslogFlag = bSyslogFlag;			//����Debug���𿪹ر���
	m_bDebug1Flag = bDebug1Flag;
	m_bDebug2Flag = bDebug2Flag;
	m_bDebug3Flag = bDebug3Flag;
	m_bPrintf2ScrFlag = bPrintf2ScrFlag;	//������Ļ�������
	m_nHoldFileMax = nHoldFileMax;			//����������ļ�����
	m_pFileInfoQueue = new CAlvinXuMemoryQueue(		//ʵ�����ļ�Ŀ¼���ж���
		pDebug,
		m_pMemPool,
		"CAlvinXuLog::m_pFileInfoQueue");
	if (m_pFileInfoQueue)				//��������ɹ���ע�ᵽ�ڴ��
	{
		m_pMemPool->Register(m_pFileInfoQueue, 
			"CAlvinXuLog::m_pFileInfoQueue");
	}
	m_pFileInfoQueue->ReadFromFile(m_szFileInfoName);	//�����ϴα������ļ�����Ϣ
	MakeFileName();										//���ݵ�ǰʱ���������һ���ļ���
}

CAlvinXuLog::~CAlvinXuLog(void)
{
	if (m_pFileInfoQueue)								//����ļ�Ŀ¼���ж���
	{
		m_pFileInfoQueue->Write2File(m_szFileInfoName);	//���ǰ�ȱ���������
		m_pMemPool->UnRegister(m_pFileInfoQueue);		//��ע�����ָ��
		delete m_pFileInfoQueue;						//ɾ������
		m_pFileInfoQueue = NULL;
	}
	ALVIN_DEBUG(CAlvinXuLog:Stop!\n);					//Debug���
}


#ifdef WIN32
#define Linux_Win_vsnprintf _vsnprintf
#else //not WIN32
#define Linux_Win_vsnprintf vsnprintf
#endif

//��ȫ�ı���������
//szBuf:�û�ָ�������������
//nMaxLength:�û�ָ��������������ߴ�
//szFormat:��ʽ������ַ�������Σ������Ƕ����
//����������ַ�������strlen�ĳ��ȣ�����������'\0'��
int SafePrintf(char* szBuf, int nMaxLength, char* szFormat, ...)
{
	int nListCount = 0;
	va_list pArglist;
	//�˴�����ȫ�Է�������ֹ�û�����Ƿ��Ļ����������³����ڴ˱�����
	if(!szBuf)goto SafePrintf_END_PROCESS;
	//�˴�����ϵͳѭ��������ÿ����ʽ������ַ���
	va_start(pArglist, szFormat);
	nListCount += Linux_Win_vsnprintf(szBuf + nListCount,
		nMaxLength - nListCount, szFormat, pArglist);
	va_end(pArglist);
	//ʵ�ֻ��������ޱ���
	if (nListCount > (nMaxLength - 1)) nListCount = nMaxLength - 1;
	//�˹����'\0'��ȷ�����100%��׼C�ַ���
	*(szBuf + nListCount) = '\0';
SafePrintf_END_PROCESS:
	return nListCount;

}

void CAlvinXuLog::MakeFileName(void)						//����һ���µ��ļ���
{
	char szTemp[LOG_ITEM_LENGTH_MAX];						//��ʱ������
	MakeATimeString(szTemp, LOG_ITEM_LENGTH_MAX);			//���ʱ����ַ���
	FixFileInfo();											//ά���ļ��ܸ���������(Ĭ��72��)
	int nLen = SafePrintf(									//ע�⿴��䣬���ù��캯���е���������
		m_szFileName,										//����ʱ����������ټ���".log"��׺
		FILENAME_STRING_LENGTH*2,							//������־�ļ���
		"%s_%s.log",
		m_szFilePath,
		szTemp);
	nLen++;													//�������'\0'��λ��
	//���µ��ļ�����ӵ�����
	int nAddLastRet = m_pFileInfoQueue->AddLast(m_szFileName, nLen);
	if (0 >= nAddLastRet)
	{	//����һ������ķ����������������(�ڴ治����)��ɾ���ʼ�������ļ���
		//�ͷ��ڴ�ռ䣬����Ԥ��������ҵ��̫��æ�������ڴ治���ã������޷���ӵ�
		//��ܴ�ʩ����Ҳ���ַǹؼ�ģ��Ϊ�ؼ�ҵ��ģ����·��˼ά
		DeleteFirstFile();
		DeleteFirstFile();
		DeleteFirstFile();
		//ɾ������֮�����������
		nAddLastRet = m_pFileInfoQueue->AddLast(m_szFileName, nLen);
		//�����ʱ�����Ȼʧ�ܣ�Ͷ������־����һ�����û�й�ϵ��
	}
	m_nFileSize = 0;							//���ļ��������ļ�����Ϊ0
	//�����߼����´���һ���ļ������ļ�ͷ�ȴ�ӡһ���ļ��������Ϣ�������Ժ�ĸ��ٲ���
	time(&m_tFileNameMake);
	{	//�������Ƿ�ҵ���ӡ����˲�ϣ���������Ļ��������ʱ����Ļ���عر�
		bool bPrintf2Scr = m_bPrintf2ScrFlag;
		m_bPrintf2ScrFlag = false;
		_Printf("Alvin.Xu. base libeary log file %s\n", m_szFileName);
		_Printf("-----------------------------------\n");
		m_bPrintf2ScrFlag = bPrintf2Scr;			//��������Ļ���ػָ�ԭֵ
	}
}

void CAlvinXuLog::GetFileName(void)//��ȡ��ǰ�ļ���
{
	time_t tNow;				//��ǰʱ�������
	unsigned long ulDeltaT = 0; //t����
	if('\0' == m_szFileName[0])		//�����һ�������ļ���Ϊ��
	{
		MakeFileName();
		goto CAlvinXuLog_GetFileName_End_Process;
	}
	time(&tNow);					//��õ�ǰʱ��
	ulDeltaT = (unsigned long)tNow - m_tFileNameMake;	//����t
	if (LOG_FILE_CHANGE_NAME_PRE_SECONDS <= ulDeltaT)
	{
		MakeFileName();				//���t����3600�룬�����ļ�������
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
	if (LOG_FILE_SIZE_MAX > nBufferSize)		//���������
		goto CAlvinXuLog_MakeATimeString_End_Process;
	time(&t);									//��õ�ǰʱ��
	pTM = localtime(&t);						//��õ�ǰʱ����ʱ����ַ���
	nLength = SafePrintf(szBuffer, LOG_ITEM_LENGTH_MAX, "%s", asctime(pTM));	//ʱ����ַ����뻺����
	//localtime���ɵ��ַ�������Դ�һ��'\n'�����س������ⲻ���ں����Ĵ�ӡ
	//�������������ǰ��һ�����������س���������һ��С���顣
	szBuffer[nLength - 1] = '\0';
	//�ļ�����һ�������ƣ�һ����Ҫ�пո�':'�ַ�������Թ���һ�£�
	//��ʱ����еķǷ��ַ����ı�ɸ�ϵͳ���ܽ��ܵ�'_'�»���
	for(i = 0; i < nLength; i++)
	{
		if ('' == szBuffer[i])szBuffer[i] = '_';
		if (':' == szBuffer[i])szBuffer[i] = '_';
	}		
CAlvinXuLog_MakeATimeString_End_Process:
	return nLength;
}
