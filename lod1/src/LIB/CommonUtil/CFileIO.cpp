#include "pch.h"
#include "CFileIO.h"
#include <iostream>
#include <filesystem>
#include <cassert>

/*! �R���X�g���N�^
*/
CFileIO::CFileIO(void)
{
	// ������
	m_pFp = NULL;
	m_strFileName = L"";
	m_strOpenMode = L"";
}

/*! �f�X�g���N�^
*/
CFileIO::~CFileIO(void)
{
	this->Close();
}

/*! �t�@�C���T�C�Y���擾����
@retval	�t�@�C���T�C�Y
*/
int CFileIO::GetFileLength()
{
	uintmax_t nRet = 0;

	nRet = std::filesystem::file_size(m_strFileName);

	return (int)nRet;
}

/*! �t�@�C���|�C���^�擪�ړ�
@note	�ǂݍ��񂾃t�@�C���̃|�C���^��擪�Ɉړ�����B
*/
void CFileIO::SeekToBegin()
{
	// �t�@�C���|�C���^��擪�Ɉړ�
	if (m_pFp != NULL)
	{
		fseek(m_pFp, 0L, (int)SEEK_SET);
	}

}

/*! �t�@�C���|�C���^�J�����g�ړ�
@note	���݂̃t�@�C���|�C���^�ʒu����w��o�C�g�����ړ�����B
*/
void CFileIO::SeekToCurrent(int nByte	//!< in �w��o�C�g��
)
{
	// �t�@�C���|�C���^���w��ʒu��������o�C�g���V�t�g
	if (m_pFp != NULL)
	{
		fseek(m_pFp, nByte, (int)SEEK_CUR);
	}
}

/*! ������t�@�C����������
@note	�w�蕶����ɉ��s�R�[�h��ǉ����A�t�@�C���ɏo�͂���B
*/
bool CFileIO::WriteLineA(std::string strLine	//!< in �o�͑Ώە�����
)
{
	// ���s�R�[�h��ǉ�
	std::string str = strLine + "\n";

	fputs(str.c_str(), m_pFp);

	return true;
}

/*! ������t�@�C����������
@note	�w�蕶����ɉ��s�R�[�h��ǉ����A�t�@�C���ɏo�͂���B
*/
bool CFileIO::WriteLineW(std::wstring strLine	//!< in �o�͑Ώە�����
)
{
	// ���s�R�[�h��ǉ�
	std::wstring wstr = strLine + L"\n";
	std::string str = CStringEx::ToString(wstr);

	fputs(str.c_str(), m_pFp);

	return true;
}

/*! �t�@�C�������
*/
void CFileIO::Close(void)
{
	if (m_pFp != NULL) {
		fclose(m_pFp);
	}
	m_pFp = NULL;
	m_strFileName = L"";
	m_strOpenMode = L"";
}

/*! �t�@�C�����J��
@retval	true	����
@retval	false	���s
*/
bool CFileIO::Open(
	const std::wstring& strFileName,			//!< in �t�@�C��(URL��)
	const std::wstring& strMode				//!< in �A�N�Z�X���̎��("r","w+"���ACRT��fopen�֐��̃��[�h�Ɠ��l)
)
{
	bool	bRtn = true;

	errno_t		err;

	// �t�@�C���|�C���^��������
	this->Close();

	// �t�@�C�����J��
	err = _wfopen_s(&m_pFp, strFileName.c_str(), strMode.c_str());

	if (err == 0) {
		m_strFileName = strFileName;
		m_strOpenMode = strMode;
	}
	else {
		bRtn = false;
	}
	if (bRtn == false) {
		m_strFileName = L"";
	}
	return	bRtn;
}

/*! �֘A�t����ꂽ�t�@�C������o�b�t�@�Ƀf�[�^��ǂݏo��
@return	�o�b�t�@�ɓ]�����ꂽ�o�C�g��
*/
size_t CFileIO::Read(void* pBuff,	//!< out �f�[�^�擾�p�̃��[�U�[���m�ۂ����o�b�t�@�ւ̃|�C���^
	size_t	nSize	//!< in  �o�b�t�@�T�C�Y
)
{
	assert(pBuff != NULL);
	assert(nSize > 0);
	assert(m_pFp != NULL);

	size_t	nReadSize = 0;

	if (m_pFp != NULL) {
		nReadSize = fread(pBuff, 1, nSize, m_pFp);
	}
	return	nReadSize;
}

wchar_t* CFileIO::ReadLineW(wchar_t* pBuff,	//!< out �f�[�^�擾�p�̃��[�U�[���m�ۂ����o�b�t�@�ւ̃|�C���^
	int			nSize)	//!< in  �o�b�t�@�T�C�Y
{
	assert(pBuff != NULL);
	assert(nSize > 0);
	assert(m_pFp != NULL);

	wchar_t* pstr = NULL;

	if (m_pFp != NULL)
	{
		memset(pBuff, '\0', nSize * sizeof(wchar_t));
		pstr = fgetws(pBuff, nSize, m_pFp);
	}
	return	pstr;
}

char* CFileIO::ReadLineA(char* pBuff,	//!< out �f�[�^�擾�p�̃��[�U�[���m�ۂ����o�b�t�@�ւ̃|�C���^
	int			nSize)	//!< in  �o�b�t�@�T�C�Y
{
	assert(pBuff != NULL);
	assert(nSize > 0);
	assert(m_pFp != NULL);

	char* pstr = NULL;

	if (m_pFp != NULL)
	{
		memset(pBuff, '\0', nSize * sizeof(wchar_t));
		pstr = fgets(pBuff, nSize, m_pFp);
	}
	return	pstr;
}