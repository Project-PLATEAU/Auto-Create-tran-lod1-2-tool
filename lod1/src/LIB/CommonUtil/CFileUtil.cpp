#include "pch.h"
#include "CFileUtil.h"
#include "StringEx.h"
#include <iostream>
#include <filesystem>
#include <regex>
#include <cassert>

// �C���X�^���X��`
CFileUtil	CFileUtil::m_instance;

/*! �R���X�g���N�^
*/
CFileUtil::CFileUtil(void)
{
}
/*! �f�X�g���N�^
*/
CFileUtil::~CFileUtil(void)
{
}

void CFileUtil::CreateFileList(
	const std::string& strFolderPath,		//!< in		�e�t�H���_�p�X
	const std::string& strName,				//!< in		�t�@�C�������̏���
	std::list<std::string>* pLstFilePath			//!< out	�t�@�C���p�X�̃��X�g
)
{
	for (const auto& file : std::filesystem::directory_iterator(strFolderPath))
	{
		if (file.is_directory())
		{
			// �������Ȃ�
		}
		// �t�@�C��
		else
		{
			// ���X�g�ɒǉ�
			pLstFilePath->push_back(file.path().string());
		}
	}

}

void CFileUtil::CreateFileList(
	const std::wstring& strFolderPath,		//!< in		�e�t�H���_�p�X
	const std::wstring& strName,				//!< in		�t�@�C�������̏���
	std::list<std::wstring>* pLstFilePath			//!< out	�t�@�C���p�X�̃��X�g
)
{
	for (const auto& file : std::filesystem::directory_iterator(strFolderPath))
	{
		if (file.is_directory())
		{
			// �������Ȃ�
		}
		// �t�@�C��
		else
		{
			// ���X�g�ɒǉ�
			pLstFilePath->push_back(file.path().wstring());
		}
	}

}

/* �t�@�C�����̊g���q��ύX����
*/
std::string CFileUtil::ChangeFileNameExt(const std::string& strFileName,	//!< in �t�@�C����
	const std::string& strExtName		//!< in �g���q
)
{
	//exe�t�@�C���̃p�X�𕪉�
	char path[_MAX_PATH] = { '\0' };
	char drive[_MAX_DRIVE] = { '\0' };
	char dir[_MAX_DIR] = { '\0' };
	char fname[_MAX_FNAME] = { '\0' };
	char ext[_MAX_EXT] = { '\0' };
	_splitpath_s(strFileName.c_str(), drive, dir, fname, ext);
	_makepath_s(path, drive, dir, fname, strExtName.c_str());
	return path;
}


/* �t�@�C�����̊g���q��ύX����
*/
std::wstring CFileUtil::ChangeFileNameExt(const std::wstring& strFileName,	//!< in �t�@�C����
	const std::wstring& strExtName		//!< in �g���q
)
{
	//exe�t�@�C���̃p�X�𕪉�
	WCHAR path[_MAX_PATH] = { '\0' };
	WCHAR drive[_MAX_DRIVE] = { '\0' };
	WCHAR dir[_MAX_DIR] = { '\0' };
	WCHAR fname[_MAX_FNAME] = { '\0' };
	WCHAR ext[_MAX_EXT] = { '\0' };
	_wsplitpath_s(strFileName.c_str(), drive, dir, fname, ext);
	_wmakepath_s(path, drive, dir, fname, strExtName.c_str());
	return path;
}

/*! �p�X(�t�@�C���A�f�B���N�g��)�̑��݃`�F�b�N
@retval	true	���݂���
@retval	false	���݂��Ȃ�
@note	���C���h�J�[�h�������܂܂��t�@�C���p�X�͑Ή����Ȃ��B
*/
bool CFileUtil::IsExistPath(const std::string& strPath	//!< in	�p�X
)
{
	bool bExist = false;

	const int TEXT_LENGTH = (int)strPath.length();
	if (TEXT_LENGTH > 0)
	{
		// ������'\'�A��������'/'�̓J�b�g����B
		std::string strTarget = strPath;
		if (strPath.back() == '\\' || strPath.back() == '/')
		{
			strTarget.pop_back();
		}

		if(std::filesystem::exists(strTarget))
		{
			bExist = true;
		}
	}

	return bExist;
}

/*! �p�X(�t�@�C���A�f�B���N�g��)�̑��݃`�F�b�N
@retval	true	���݂���
@retval	false	���݂��Ȃ�
@note	���C���h�J�[�h�������܂܂��t�@�C���p�X�͑Ή����Ȃ��B
*/
bool CFileUtil::IsExistPath(const std::wstring& strPath	//!< in	�p�X
)
{
	bool bExist = false;

	const int TEXT_LENGTH = (int)strPath.length();
	if (TEXT_LENGTH > 0)
	{
		// ������'\'�A��������'/'�̓J�b�g����B
		std::wstring strTarget = strPath;
		if (strPath.back() == '\\' || strPath.back() == '/')
		{
			strTarget.pop_back();
		}

		if (std::filesystem::exists(strTarget))
		{
			bExist = true;
		}
	}

	return bExist;
}

/*! �t�@�C���p�X����
@retval	true	����
@retval	false	���s
@note	�t�@�C���p�X�𕪉����܂��B
@note	�o�͈�����pstrDrive,pstrDir,pstrName,pstrExt�̂����A�s�v�Ȃ��̂�NULL���Z�b�g�\�ł��B
*/
bool CFileUtil::SplitPath(const std::string& strPath,				//!< in		�p�X
	std::string* pstrDrive		/*= NULL*/,	//!< out	�h���C�u
	std::string* pstrDir		/*= NULL*/,	//!< out	�f�B���N�g��
	std::string* pstrName		/*= NULL*/,	//!< out	���O
	std::string* pstrExt		/*= NULL*/	//!< out	�g���q
)
{
	bool bRet = false;

	char	drive[_MAX_DRIVE],
		dir[_MAX_DIR],
		name[_MAX_FNAME],
		ext[_MAX_EXT];
	if (_splitpath_s(strPath.c_str(), drive, dir, name, ext) == 0)
	{
		if (pstrDrive != NULL)
		{
			*pstrDrive = drive;
		}
		if (pstrDir != NULL)
		{
			*pstrDir = dir;
		}
		if (pstrName != NULL)
		{
			*pstrName = name;
		}
		if (pstrExt != NULL)
		{
			*pstrExt = ext;
		}
		bRet = true;
	}

	return bRet;
}


/*! �t�@�C���p�X����
@retval	true	����
@retval	false	���s
@note	�t�@�C���p�X�𕪉����܂��B
@note	�o�͈�����pstrDrive,pstrDir,pstrName,pstrExt�̂����A�s�v�Ȃ��̂�NULL���Z�b�g�\�ł��B
*/
bool CFileUtil::SplitPath(const std::wstring& strPath,				//!< in		�p�X
	std::wstring* pstrDrive		/*= NULL*/,	//!< out	�h���C�u
	std::wstring* pstrDir		/*= NULL*/,	//!< out	�f�B���N�g��
	std::wstring* pstrName		/*= NULL*/,	//!< out	���O
	std::wstring* pstrExt		/*= NULL*/	//!< out	�g���q
)
{
	bool bRet = false;

	WCHAR	drive[_MAX_DRIVE],
		dir[_MAX_DIR],
		name[_MAX_FNAME],
		ext[_MAX_EXT];
	if (_wsplitpath_s(strPath.c_str(), drive, dir, name, ext) == 0)
	{
		if (pstrDrive != NULL)
		{
			*pstrDrive = drive;
		}
		if (pstrDir != NULL)
		{
			*pstrDir = dir;
		}
		if (pstrName != NULL)
		{
			*pstrName = name;
		}
		if (pstrExt != NULL)
		{
			*pstrExt = ext;
		}
		bRet = true;
	}

	return bRet;
}

/*!
@brief	�p�X�̘A��
@return	�A�������p�X
*/
std::string CFileUtil::Combine(
	const std::string& strPath1,		//!< [in]	�p�X1
	const std::string& strPath2		//!< [in]	�p�X2
)
{
	LPCSTR pPath2Header = strPath2.c_str();
	
	if (strPath2.front() == '\\' || *pPath2Header == '/')
	{
		// �p�X2�̐擪��\�̏ꍇ�̓G���[
		return "";
	}
	// �p�X��A��
	char	szPath[MAX_PATH] = {'\0'};
	errno_t _err = _makepath_s(szPath, NULL, strPath1.c_str(), strPath2.c_str(), NULL);
	return szPath;
}

/*!
@brief	�p�X�̘A��
@return	�A�������p�X
*/
std::wstring CFileUtil::Combine(
	const std::wstring& strPath1,		//!< [in]	�p�X1
	const std::wstring& strPath2		//!< [in]	�p�X2
)
{
	LPCWSTR pPath2Header = strPath2.c_str();

	if (strPath2.front() == '\\' || *pPath2Header == '/')
	{
		// �p�X2�̐擪��\�̏ꍇ�̓G���[
		return L"";
	}
	// �p�X��A��
	WCHAR	szPath[MAX_PATH] = { '\0' };
	errno_t _err = _wmakepath_s(szPath, NULL, strPath1.c_str(), strPath2.c_str(), NULL);
	return szPath;
}

/*! �t�H���_�폜
*/
bool CFileUtil::RemoveMultiFolder(const std::string& strFolderPath	//!< in �t�H���_��
)
{
	if (strFolderPath.length() == 0) {
		return true;
	}

	bool		bReturn = true;
	std::string		strFolderName(strFolderPath);

	// �Ō�� '\'or'/' ��t��
	if (strFolderName.back() != '\\' && strFolderName.back() != '/')
		strFolderName.push_back('\\');

	// �ċA�I�ɍ폜
	std::filesystem::remove_all(strFolderName);

	// �f�B���N�g���̍폜
	bReturn = std::filesystem::remove(strFolderName);

	return bReturn;
}

/*! �t�H���_�폜
*/
bool CFileUtil::RemoveMultiFolder(const std::wstring& strFolderPath	//!< in �t�H���_��
)
{
	if (strFolderPath.length() == 0) {
		return true;
	}

	bool		bReturn = true;
	std::wstring		strFolderName(strFolderPath);

	// �Ō�� '\'or'/' ��t��
	if (strFolderName.back() != '\\' && strFolderName.back() != '/')
		strFolderName.push_back('\\');

	// �ċA�I�ɍ폜
	std::filesystem::remove_all(strFolderName);

	// �f�B���N�g���̍폜
	bReturn = std::filesystem::remove(strFolderName);

	return bReturn;
}

/*! CSV�f�[�^����
@note	strData���J���}���ɕ������܂��B
<br>	strData��','���P�������ꍇ�AparyData�ɂ�strData��1���ǉ����܂��B
@note	""�Ŋ���ꂽ�f�[�^��1�t�B�[���h�Ƃ��Ĉ����܂��B
<br>	""�Ŋ���ꂽ�t�B�[���h��""������������ԂŃf�[�^�Ɋi�[����܂��B
*/
void CFileUtil::SplitCSVData(const std::string& strRecord,		//!< in		������
	std::vector<std::string>* paryData,					//!< out	���������f�[�^
	char			cSeparator /*= ','*/		//!< in		��؂蕶��
)
{
	assert(paryData != NULL);
	paryData->clear();

	if (!strRecord.empty())
	{
		std::string strData = strRecord;
		// ���s�R�[�h���܂܂��ꍇ�͏�������
		strData = regex_replace(strData, std::regex("\r"), "\0");
		strData = regex_replace(strData, std::regex("\n"), "\0");

		if (!strData.empty())
		{
			bool bDblQuotation = false;						// "�n�܂�̕�����t���O
			std::string strEndField = "\"" + cSeparator;	// ������̏I�[

			// "�n�܂肾�����ꍇ�Ƃ���ȊO�ŕ����񃌃R�[�h�̈������ς��
			bDblQuotation = ('\"' == strData.front()) ? true : false;
			std::string strEndFieldSeparator = (bDblQuotation) ? strEndField : std::string(1, cSeparator);

			std::string strField;
			int iLastFind = 0;
			int iFindIndex = (int)strData.find(strEndFieldSeparator);
			if (iFindIndex >= 0)
			{
				while (iFindIndex >= 0)
				{
					if (bDblQuotation)
					{
						strField = strData.substr((int64_t)iLastFind + 1, (((int64_t)iFindIndex - iLastFind) - 1));
					}
					else
					{
						strField = strData.substr(iLastFind, ((int64_t)iFindIndex - iLastFind));
					}
					paryData->push_back(strField);
					iLastFind = iFindIndex + (int)strEndFieldSeparator.length();

					// "�n�܂肾�����ꍇ�Ƃ���ȊO�ŕ����񃌃R�[�h�̈������ς��
					bDblQuotation = ("\"" == strData.substr(iLastFind, 1)) ? true : false;
					strEndFieldSeparator = (bDblQuotation) ? strEndField : std::string(1, cSeparator);

					iFindIndex = (int)strData.find(strEndFieldSeparator, iLastFind);
				}
				strField = strData.substr(iLastFind);
			}
			else
			{
				strField = strData;
			}
			// 2�����ȏ゠��ꍇ��""�Ŋ����Ă���\��������
			if (strField.length() >= 2)
			{
				// ������""�Ŋ����Ă���ꍇ�͏�������
				std::string str1 = { strField.front() },
						str2 = strField.substr(strField.length() - 2, 1);
				if (strField.front() == '\"' && strField.substr(strField.length() - 2, 1) == "\"")
				{
					strField = strField.substr(1, strField.length() - 3);
				}
			}
			paryData->push_back(strField);
		}
		else
		{
			// �f�[�^����ł��A���1����ǉ�����
			paryData->push_back(strData);
		}
	}
	else
	{
		// �f�[�^����ł��A���1����ǉ�����
		paryData->push_back(strRecord);
	}
}

/*! CSV�f�[�^����
@note	strData���J���}���ɕ������܂��B
<br>	strData��','���P�������ꍇ�AparyData�ɂ�strData��1���ǉ����܂��B
@note	""�Ŋ���ꂽ�f�[�^��1�t�B�[���h�Ƃ��Ĉ����܂��B
<br>	""�Ŋ���ꂽ�t�B�[���h��""������������ԂŃf�[�^�Ɋi�[����܂��B
*/
void CFileUtil::SplitCSVData(const std::wstring& strRecord,		//!< in		������
	std::vector<std::wstring>* paryData,					//!< out	���������f�[�^
	wchar_t			cSeparator /*= ','*/		//!< in		��؂蕶��
)
{
	assert(paryData != NULL);
	paryData->clear();

	if (!strRecord.empty())
	{
		std::wstring strData = strRecord;
		// ���s�R�[�h���܂܂��ꍇ�͏�������
		strData = regex_replace(strData, std::wregex(L"\r"), L"\0");
		strData = regex_replace(strData, std::wregex(L"\n"), L"\0");

		if (!strData.empty())
		{
			bool bDblQuotation = false;						// "�n�܂�̕�����t���O
			std::wstring strEndField = L"\"" + cSeparator;	// ������̏I�[

			// "�n�܂肾�����ꍇ�Ƃ���ȊO�ŕ����񃌃R�[�h�̈������ς��
			bDblQuotation = ('\"' == strData.front()) ? true : false;
			std::wstring strEndFieldSeparator = (bDblQuotation) ? strEndField : std::wstring(1, cSeparator);

			std::wstring strField;
			int iLastFind = 0;
			int iFindIndex = (int)strData.find(strEndFieldSeparator);
			if (iFindIndex >= 0)
			{
				while (iFindIndex >= 0)
				{
					if (bDblQuotation)
					{
						strField = strData.substr((int64_t)iLastFind + 1, (((int64_t)iFindIndex - iLastFind) - 1));
					}
					else
					{
						strField = strData.substr(iLastFind, ((int64_t)iFindIndex - iLastFind));
					}
					paryData->push_back(strField);
					iLastFind = iFindIndex + (int)strEndFieldSeparator.length();

					// "�n�܂肾�����ꍇ�Ƃ���ȊO�ŕ����񃌃R�[�h�̈������ς��
					bDblQuotation = (L"\"" == strData.substr(iLastFind, 1)) ? true : false;
					strEndFieldSeparator = (bDblQuotation) ? strEndField : std::wstring(1, cSeparator);

					iFindIndex = (int)strData.find(strEndFieldSeparator, iLastFind);
				}
				strField = strData.substr(iLastFind);
			}
			else
			{
				strField = strData;
			}
			// 2�����ȏ゠��ꍇ��""�Ŋ����Ă���\��������
			if (strField.length() >= 2)
			{
				// ������""�Ŋ����Ă���ꍇ�͏�������
				std::wstring str1 = { strField.front() },
					str2 = strField.substr(strField.length() - 2, 1);
				if (strField.front() == '\"' && strField.substr(strField.length() - 2, 1) == L"\"")
				{
					strField = strField.substr(1, strField.length() - 3);
				}
			}
			paryData->push_back(strField);
		}
		else
		{
			// �f�[�^����ł��A���1����ǉ�����
			paryData->push_back(strData);
		}
	}
	else
	{
		// �f�[�^����ł��A���1����ǉ�����
		paryData->push_back(strRecord);
	}
}

/*! ���W���[���p�X�擾�֐�
@return ���W���[���p�X
*/
std::string CFileUtil::GetModulePath(
	const HINSTANCE hModule /*= NULL*/	//!< in	�擾�Ώۃ��W���[���n���h��
)
{
	std::string	szValue;

	char szModuleFileName[MAX_PATH];

	GetModuleFileNameA(hModule, szModuleFileName, MAX_PATH);
	std::string strModulePath = szModuleFileName;
	strModulePath = strModulePath.substr(0, strModulePath.rfind('\\') + 1);

	szValue = strModulePath;
	return szValue;
}

/*! ���W���[���p�X�擾�֐�
@return ���W���[���p�X
*/
std::wstring CFileUtil::GetModulePathW(
	const HINSTANCE hModule /*= NULL*/	//!< in	�擾�Ώۃ��W���[���n���h��
)
{
	std::wstring	szValue;

	WCHAR szModuleFileName[MAX_PATH];

	GetModuleFileNameW(hModule, szModuleFileName, MAX_PATH);
	std::wstring strModulePath = szModuleFileName;
	strModulePath = strModulePath.substr(0, strModulePath.rfind('\\') + 1);

	szValue = strModulePath;
	return szValue;
}


/*! �e�f�B���N�g���擾
@return	�e�f�B���N�g���̃p�X
@note	�e�f�B���N�g�����擾�������p�X��n���Đe�f�B���N�g�����擾���܂��B
*/
std::wstring CFileUtil::GetParentDir(const std::wstring& strTargetPath		//!< in	�擾�������Ώۂ̃p�X
) const
{
	// �f�B���N�g���̏ꍇ�A������'\\'��'/'���Ɛe�f�B���N�g�������Ȃ��̂Ŗ������폜����
	std::wstring strTarget = (strTargetPath.back() == '\\' || strTargetPath.back() == '/')
		? strTargetPath.substr(0, (strTargetPath.length() - 1)) : strTargetPath,
		strParent, strDrive, strDir;

	// �p�X�̕���
	if (SplitPath(strTarget, &strDrive, &strDir))
	{
		strParent = CStringEx::Format(L"%s%s", strDrive.c_str(), strDir.c_str());
	}

	return strParent;
}