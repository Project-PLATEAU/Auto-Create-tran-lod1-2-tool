#include "pch.h"
#include "CShapeManager.h"
#include "CFileUtil.h"

/*!
 * @brief コンストラクタ
*/
CShapeManager::CShapeManager()
{
	// ハンドル
	hSHP = NULL;
	hDBF = NULL;

	nShapeType = SHPT_NULL; // 種別
	nEntities = 0;          // 要素数
	nField = 0;             // field数
}

/*!
 * @brief デストラクタ
*/
CShapeManager::~CShapeManager()
{
	Close();
}

/*!
 * @brief   open shape file
 * @param   strShpPath  shpファイルパス
 * @return  処理結果
 * @retval  true        成功
 * @retval  false       失敗
 * @note    shpファイルと同階層にdbfファイルが存在すること
*/
bool CShapeManager::Open(std::string strShpPath)
{
	std::string strDbfPath = CFileUtil::ChangeFileNameExt(strShpPath, ".dbf");

	if (!GetFUtil()->IsExistPath(strShpPath))
	{
		return false;
	}
	if (!GetFUtil()->IsExistPath(strDbfPath))
	{
		return false;
	}

	// open shape file
	hSHP = SHPOpen(strShpPath.c_str(), "r");
	if (hSHP == NULL)
	{
		return false;
	}
	// open dbf file
	hDBF = DBFOpen(strDbfPath.c_str(), "rb");
	if (hDBF == NULL)
	{
		SHPClose(hSHP);
		hSHP = NULL;
		return false;
	}

	// 要素数、種別、バウンディング情報の取得
	SHPGetInfo(hSHP, &nEntities, &nShapeType, adfMinBound, adfMaxBound);

	// 要素数の確認
	if (nEntities <= 0)
	{
		Close();
		return false;
	}

	// field数
	nField = DBFGetFieldCount(hDBF);

	// ポリゴン読み込み
	for (int i = 0; i < nEntities; i++)
	{
		psShape = SHPReadObject(hSHP, i);
		psShapeVec.push_back(psShape);
	}

	return true;
}

/*!
 * @brief close shape file
 */
void CShapeManager::Close(void)
{
	if (hSHP != NULL)
	{
		SHPClose(hSHP);
		hSHP = NULL;
	}
	if (hDBF != NULL)
	{
		DBFClose(hDBF);
		hDBF = NULL;
	}

	// ポリゴンのメモリを解放する
	SHPDestroyObject(psShape);
}
