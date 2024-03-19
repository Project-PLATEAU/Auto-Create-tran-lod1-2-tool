#include "CReadParamFile.h"

CReadParamFile CReadParamFile::m_instance;  // インスタンス

CReadParamFile::CReadParamFile()
	: m_LOD1RoadSHPPath(""),
	m_MapFilePath(""),
	m_OutputFolderPath(""),
	m_JPZone(0)
{

}

CReadParamFile::~CReadParamFile()
{
}

bool CReadParamFile::Initialize(std::string strParamPath)
{
	bool bRet = false;
	CINIFileIO inifile;
	std::string SETTING = "Setting";

	// file open
	bRet = inifile.Open(strParamPath);
	if (bRet)
	{
		// パラメータ取得
		// 系番号
		m_LOD1RoadSHPPath = inifile.GetString(SETTING, "LOD1RoadSHPPath", "");

		// 道路縁SHPパス
		m_MapFilePath = inifile.GetString(SETTING, "MapFilePath", "");

		// 道路施設SHPパス
		m_OutputFolderPath = inifile.GetString(SETTING, "OutputFolderPath", "");

		// 出力フォルダパス
		m_JPZone = inifile.GetInt(SETTING, "JPZone", 0);
		if (checkParam())
		{
			// パラメータエラー有り
			bRet = false;
		}
	}

	return bRet;
}

std::string CReadParamFile::GetLOD1RoadSHPPath()
{
	return m_LOD1RoadSHPPath;
}

std::string CReadParamFile::GetMapFilePath()
{
	return m_MapFilePath;
}

std::string CReadParamFile::GetOutputFolderPath()
{
	return m_OutputFolderPath;
}

int CReadParamFile::GetJPZone()
{
	return m_JPZone;
}

bool CReadParamFile::checkParam()
{
	bool bRet = false;
	if (m_JPZone < 1 || 19 < m_JPZone
		|| m_LOD1RoadSHPPath.empty() || !CFileUtil::IsExistPath(m_LOD1RoadSHPPath)
		|| m_MapFilePath.empty() || !CFileUtil::IsExistPath(m_MapFilePath)
		|| m_OutputFolderPath.empty() || !CreateFolder(m_OutputFolderPath))
	{
		// 系番号範囲外
		// shapeファイルパスが空 or ファイルが存在しない
		// 出力フォルダパスが空
		// DMコード,レコードタイプ,図形区分属性名が空
		bRet = true;
	}
	return bRet;
}

/*!
 * @brief  道路縁SHPファイルパス設定のゲッター
 * @return 道路縁SHPファイルパス
*/
std::string CReadParamFile::GetRoadSHPPath()
{
	return m_LOD1RoadSHPPath;
}

/*!
 * @brief  DMコード属性名設定のゲッター
 * @return DMコード属性名
*/
std::string CReadParamFile::GetDMCodeAttribute()
{
	return m_strDMCodeAttr;
}