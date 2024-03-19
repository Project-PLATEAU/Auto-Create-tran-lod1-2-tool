#pragma once
#include <vector>
#include <string>
#include <filesystem>
#include "AnalyzeRoadEdgeCommon.h"
#include "CShapeManager.h"
#include "CFileUtil.h"

/*!
 * @brief ���H����͏����̃f�o�b�O�p�N���X
*/
class CAnalyzeRoadEdgeDebugUtil
{
public:
    CAnalyzeRoadEdgeDebugUtil() {}      //!< �R���X�g���N�^
    ~CAnalyzeRoadEdgeDebugUtil() {}     //!< �f�X�g���N�^

    /*!
     * @brief  ���݂̍�ƃf�B���N�g���p�X�̎擾
     * @return ��ƃf�B���N�g���p�X
     */
    static std::string GetCurrentPath()
    {
        std::filesystem::path path = std::filesystem::current_path();
        return path.string();
    }

    /*!
     * @brief �t�H���_�쐬
     * @param strPath [in] �t�H���_�p�X
     * @return ��������
     * @retval true     �쐬���� or �����t�H���_
     * @retval false    �쐬���s
     */
    static bool CreateFolder(std::string strPath)
    {
        bool bRet = CFileUtil::IsExistPath(strPath);
        if (!bRet)
        {
            // ���݂��Ȃ��ꍇ
            bRet = std::filesystem::create_directories(strPath);
        }
        return bRet;
    }

    // �_��shapefile�o��
    bool OutputMultiPointsToShp(
        const BoostMultiPoints &points,
        std::string strShpPath);

    // �|�����C����shapefile�o��
    bool OutputPolylinesToShp(
        const BoostMultiLines &polylines,
        std::string strShpPath);

    // �|���S����shapefile�o��
    bool OutputPolygonsToShp(
        const BoostMultiPolygon &polygons,
        std::string strShpPath,
        const bool bHole = false);

    // ���ڔ͈͂̉���
    void OutputProcArea(
        std::string strShpPath,
        const double dInputMinX,
        const double dInputMinY,
        const int nRow,
        const int nColumn,
        const double dProcWidth,
        const double dProcHeight);

protected:

private:

};
