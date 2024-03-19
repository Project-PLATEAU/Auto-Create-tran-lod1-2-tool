#pragma once
#include "StringEx.h"

/*!
@brief	�t�@�C���Ǎ��݃N���X
*/
class CFileIO 
{
/* Param */
public:

/* Function */
public:
	int GetFileLength();					//!< �t�@�C���T�C�Y�擾
	void SeekToBegin();					//!< �t�@�C���|�C���^�擪�ړ�
	void SeekToCurrent( int nByte );		//!< �t�@�C���|�C���^�J�����g�ړ�

	bool WriteLineA(std::string strLine);	//!< ������t�@�C����������
	bool WriteLineW(std::wstring strLine );	//!< ������t�@�C����������
	wchar_t* ReadLineW(wchar_t* pBuff, int nSize);
	char* ReadLineA(char* pBuff, int nSize);
	void Close(void);
	bool Open(const std::wstring& strFileName, const std::wstring& strMode);
	size_t Read(void* pBuff, size_t nSize);

/* Event */
public:
	CFileIO(void);
	~CFileIO(void);

private:
	std::wstring		m_strFileName;				//!< ���ۂɊJ���Ă���t�@�C���̖���
	std::wstring		m_strOpenMode;				//!< �A�N�Z�X���̎��
	FILE*		m_pFp;						//!< �J�����t�@�C���̃|�C���^
};
