#include "pch.h"
#include "CShapeManager.h"
#include "CFileUtil.h"

/*!
 * @brief �R���X�g���N�^
*/
CShapeManager::CShapeManager()
{
	// �n���h��
	hSHP = NULL;
	hDBF = NULL;

	nShapeType = SHPT_NULL; // ���
	nEntities = 0;          // �v�f��
	nField = 0;             // field��
}

/*!
 * @brief �f�X�g���N�^
*/
CShapeManager::~CShapeManager()
{
	Close();
}

/*!
 * @brief   open shape file
 * @param   strShpPath  shp�t�@�C���p�X
 * @return  ��������
 * @retval  true        ����
 * @retval  false       ���s
 * @note    shp�t�@�C���Ɠ��K�w��dbf�t�@�C�������݂��邱��
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

	// �v�f���A��ʁA�o�E���f�B���O���̎擾
	SHPGetInfo(hSHP, &nEntities, &nShapeType, adfMinBound, adfMaxBound);

	// �v�f���̊m�F
	if (nEntities <= 0)
	{
		Close();
		return false;
	}

	// field��
	nField = DBFGetFieldCount(hDBF);

	// �|���S���ǂݍ���
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

	// �|���S���̃��������������
	SHPDestroyObject(psShape);
}
