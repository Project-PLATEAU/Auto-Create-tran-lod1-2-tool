#include "pch.h"
#include "CINIFileIO.h"
#include "CFileUtil.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

/*! �R���X�g���N�^
*/
CINIFileIO::CINIFileIO(void)
{
}

/*! �f�X�g���N�^
*/
CINIFileIO::~CINIFileIO(void)
{
	Close();
}

/*! Open
@retval	true	����
@retval	false	���s
*/
bool CINIFileIO::Open(const std::string& strFilePath	//!< in	�t�@�C���p�X
)
{
	bool bOpen = true;
	m_strLocalFilePath = strFilePath;

	return bOpen;
}

/*! Close
*/
void CINIFileIO::Close()
{
	m_strLocalFilePath = "";
}


/*! �����l�擾
@return	�擾�����l
@note	INI�t�@�C�����琮�����擾���܂��B
@note	Open���Ă��Ȃ��ꍇ�̓f�t�H���g�l���Ԃ�܂��B
*/
UINT CINIFileIO::GetInt(const std::string& strAppName,		//!< in	�Z�N�V������
	const std::string& strKeyName,		//!< in	�L�[��
	INT				iDefault		//!< in	�f�t�H���g�l
) const
{
	UINT uRet = iDefault;
	if (!m_strLocalFilePath.empty())
	{
		uRet = ::GetPrivateProfileIntA(strAppName.c_str(), strKeyName.c_str(),
			iDefault,
			m_strLocalFilePath.c_str());
	}
	return uRet;
}

/*! ������擾
@return	�擾�����l
@note	INI�t�@�C�����當������擾���܂��B
@note	Open���Ă��Ȃ��ꍇ�̓f�t�H���g�l���Ԃ�܂��B
*/
std::string CINIFileIO::GetString(const std::string& strAppName,	//!< in	�Z�N�V������
	const std::string& strKeyName,	//!< in	�L�[��
	const std::string& strDefault	//!< in	�f�t�H���g�l
) const
{
	std::string strRet = strDefault;

	if (!m_strLocalFilePath.empty())
	{
		int iBuffSize = 256;	// �o�b�t�@�͂ЂƂ܂�256�o�C�g�ŁE�E�E
		char* pBuff = (char*)malloc(iBuffSize);

		// INI�t�@�C�����當������擾
		// �擾�o�b�t�@������Ȃ��ꍇ�̓o�b�t�@�T�C�Y-1���Ԃ�̂ŁA���̏ꍇ�̓o�b�t�@��2�{�ɂ��ă��g���C����
		while ((iBuffSize - 1) == ::GetPrivateProfileStringA(strAppName.c_str(),
			strKeyName.c_str(),
			strDefault.c_str(),
			pBuff, iBuffSize,
			m_strLocalFilePath.c_str()))
		{
			free(pBuff);
			iBuffSize *= 2;
			pBuff = (char*)malloc(iBuffSize);
		}

		strRet = pBuff;
		free(pBuff);
	}

	return strRet;
}

/*! �����l�擾
@return	�擾�����l
@note	INI�t�@�C������������擾���܂��B
@note	Open���Ă��Ȃ��ꍇ�̓f�t�H���g�l���Ԃ�܂��B
*/
double CINIFileIO::GetDouble(const std::string& strAppName,	//!< in	�Z�N�V������
	const std::string& strKeyName,	//!< in	�L�[��
	const double& dDefault	//!< in	�f�t�H���g�l
) const
{
	double dRet = dDefault;
	if (!m_strLocalFilePath.empty())
	{
		std::string strDefault =CStringEx::Format("%f", dDefault);
		dRet = stof(GetString(strAppName, strKeyName, strDefault));
	}
	return dRet;
}
