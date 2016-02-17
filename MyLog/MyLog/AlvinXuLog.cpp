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