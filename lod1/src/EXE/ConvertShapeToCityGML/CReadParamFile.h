#pragma once

#include <iostream>
#include <filesystem>
#include "pch.h"
#include "CINIFileIO.h"
#include "CFileUtil.h"

class CReadParamFile
{
public:
	static CReadParamFile* GetInstance() { return &m_instance; }
	bool Initialize(std::string strParamPath);

	std::string GetLOD1RoadSHPPath();
	std::string GetMapFilePath();
	std::string GetOutputFolderPath();
	std::string GetRoadSHPPath();               // 道路縁SHPファイルパス
	std::string GetRoadFacilitiesSHPPath();     // 道路設備SHPファイルパス
	std::string GetDMCodeAttribute();           // DMコードの属性名
	std::string GetGeometryTypeAttribute();     // 図形区分コードの属性
	int GetJPZone();

	void Print()
	{
		std::cout << "シェイプファイルパス：" << m_LOD1RoadSHPPath << std::endl
			<< "マップファイルパス：" << m_MapFilePath << std::endl
			<< "出力フォルダパス：" << m_OutputFolderPath << std::endl
			<< "平面直角座標系の系番号：" << m_JPZone << std::endl;
	}

	/*!
	 * @brief フォルダ作成
	 * @param strPath [in] フォルダパス
	 * @return 処理結果
	 * @retval true     作成成功 or 既存フォルダ
	 * @retval false    作成失敗
	 */
	static bool CreateFolder(std::string strPath)
	{
		bool bRet = CFileUtil::IsExistPath(strPath);
		if (!bRet)
		{
			// 存在しない場合
			bRet = std::filesystem::create_directories(strPath);
		}
		return bRet;
	}

private:
	CReadParamFile();
	virtual ~CReadParamFile();
	bool checkParam();

	static CReadParamFile m_instance;

	std::string m_strRoadSHPPath;               // 道路縁SHPファイルパス
	// DMの属性設定
	std::string m_strDMCodeAttr;                // DMコードの属性名

	// 基本設定
	std::string m_LOD1RoadSHPPath;
	std::string m_MapFilePath;
	std::string m_OutputFolderPath;
	int m_JPZone;
};

#define GetCreateParam() (CReadParamFile::GetInstance())