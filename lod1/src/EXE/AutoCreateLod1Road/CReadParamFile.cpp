#include "pch.h"
#include "CReadParamFile.h"
#include "CFileUtil.h"

CReadParamFile CReadParamFile::m_instance;  // �C���X�^���X

/*!
 * @brief �R���X�g���N�^
 */
CReadParamFile::CReadParamFile(void)
    : m_nJPZone(0),
      m_strRoadSHPPath(""),
      m_strRoadFacilitiesPointSHPPath(""),
      m_strRoadFacilitiesLineSHPPath(""),
      m_strRoadFacilitiesPolygonSHPPath(""),
      m_strOutputFolderPath(""),
      m_strDMCodeAttr(""),
      m_strGeometryTypeAttr(""),
      m_strMinArea(""),
      m_minArea(0),
      m_strMaxDistance(""),
      m_maxDistance(0),
      m_strRoadBeforeDivisionShpFilePath(""),
      m_strDividedRoadShpFilePath(""),
      m_strIntersectionShpFilePath(""),
      m_nRegionWidth(0),
      m_nRegionHeight(0),
      m_nThreadNum(2)
{

}

/*!
 * @brief �f�X�g���N�^
 */
CReadParamFile::~CReadParamFile(void)
{
}

/*!
 * @brief ������
 * @param strParamPath �p�����[�^�t�@�C���p�X
 * @return ��������
 * @retval true     ����
 * @retval false    ���s
 */
bool CReadParamFile::Initialize(std::string strParamPath)
{
    bool bRet = false;
    CINIFileIO inifile;
    std::string SETTING = "Setting";
    std::string DM_ATTR_SETTING = "DM_Attribute";
    std::string ERR_CHECK_SETTING = "ErrCheck";

    // file open
    bRet = inifile.Open(strParamPath);
    if (bRet)
    {
        // �p�����[�^�擾
        // �n�ԍ�
        m_nJPZone = inifile.GetInt(SETTING, "JPZone", 0);

        // ���H��SHP�p�X
        m_strRoadSHPPath = inifile.GetString(SETTING, "RoadSHPPath", "");

        // ���H�{��(�_)SHP�p�X
        m_strRoadFacilitiesPointSHPPath = inifile.GetString(SETTING, "RoadFacilitiesPointSHPPath", "");

        // ���H�{��(��)SHP�p�X
        m_strRoadFacilitiesLineSHPPath = inifile.GetString(SETTING, "RoadFacilitiesLineSHPPath", "");

        // ���H�{��(��)SHP�p�X
        m_strRoadFacilitiesPolygonSHPPath = inifile.GetString(SETTING, "RoadFacilitiesPolygonSHPPath", "");

        // ���H����͗̈�̕�(m)
        m_nRegionWidth = inifile.GetInt(SETTING, "RegionWidth", 0);

        // ���H����͗̈�̍���(m)
        m_nRegionHeight = inifile.GetInt(SETTING, "RegionHeight", 0);

        // �}���`�X���b�h��
        m_nThreadNum = inifile.GetInt(SETTING, "ThreadNum", 0);

        // �o�̓t�H���_�p�X
        m_strOutputFolderPath = inifile.GetString(SETTING, "OutputFolderPath", "");

        // DM�R�[�h������
        m_strDMCodeAttr = inifile.GetString(DM_ATTR_SETTING, "DMCode", "");

        // DM�R�[�h������
        m_strGeometryTypeAttr = inifile.GetString(DM_ATTR_SETTING, "GeometryType", "");

        // �|���S���̖ʐς̍ŏ��l
        m_strMinArea = inifile.GetString(ERR_CHECK_SETTING, "MinArea", "");

        // �ԓ��������|���S�����S�ƌ����_�̋����̍ő�l
        m_strMaxDistance = inifile.GetString(ERR_CHECK_SETTING, "MaxDistance", "");

        // �����O�̓��H�|���S���̃t�@�C���p�X(Debug)
        m_strRoadBeforeDivisionShpFilePath = inifile.GetString(ERR_CHECK_SETTING, "inputRoadBeforeDivisionShpFilePath", "");

        // ������̓��H�|���S���̃t�@�C���p�X(Debug)
        m_strDividedRoadShpFilePath = inifile.GetString(ERR_CHECK_SETTING, "inputDividedShpFilePath", "");

        // �����_���̃t�@�C���p�X(Debug)
        m_strIntersectionShpFilePath = inifile.GetString(ERR_CHECK_SETTING, "inputIntersectionShpFilePath", "");

        if (checkParam())
        {
            // �p�����[�^�G���[�L��
            bRet = false;
        }
        else
        {
            // ������𐔒l�ɕϊ�
            m_minArea = std::stof(m_strMinArea);
            m_maxDistance = std::stof(m_strMaxDistance);
        }
    }

    return bRet;
}

/*!
 * @brief �G���[�`�F�b�N�p�̃t�@�C���p�X���ݒ肳��Ă��邩�ǂ���
 * @return �G���[�`�F�b�N�p�̃t�@�C���p�X���ݒ肳��Ă���
 * @retval true     �t�@�C���p�X���ݒ肳��Ă���
 * @retval false    �t�@�C���p�X���ݒ肳��Ă��Ȃ�
 */
bool CReadParamFile::IsErrCheckFromShp()
{
    if (m_strRoadBeforeDivisionShpFilePath.empty() || m_strDividedRoadShpFilePath.empty() || m_strIntersectionShpFilePath.empty())
    {
        return false;
    }

    return true;
}

/*!
 * @brief �p�����[�^�̃G���[�`�F�b�N
 * @return �G���[����
 * @retval true     �G���[�L��
 * @retval false    �G���[����
 */
bool CReadParamFile::checkParam(void)
{
    bool bRet = false;
    if (m_nJPZone < 1 || 19 < m_nJPZone
        || m_strRoadSHPPath.empty() || !CFileUtil::IsExistPath(m_strRoadSHPPath)
        || (!m_strRoadFacilitiesPointSHPPath.empty() && !CFileUtil::IsExistPath(m_strRoadFacilitiesPointSHPPath))
        || (!m_strRoadFacilitiesLineSHPPath.empty() && !CFileUtil::IsExistPath(m_strRoadFacilitiesLineSHPPath))
        || (!m_strRoadFacilitiesPolygonSHPPath.empty() && !CFileUtil::IsExistPath(m_strRoadFacilitiesPolygonSHPPath))
        || m_nRegionWidth < 1 || m_nRegionHeight < 1
        || m_nThreadNum < 1
        || m_strOutputFolderPath.empty()
        || m_strDMCodeAttr.empty()
        || m_strGeometryTypeAttr.empty()
        || m_strMinArea.empty()
        || m_strMaxDistance.empty())
    {
        // �n�ԍ��͈͊O
        // ���H��shape�t�@�C���p�X���� or �t�@�C�������݂��Ȃ�
        // �ݔ�shape�t�@�C���p�X���ݒ�ς݂��t�@�C�������݂��Ȃ��ꍇ
        // �o�̓t�H���_�p�X����
        // DM�R�[�h,���R�[�h�^�C�v,�}�`�敪����������
        // ���ڗ̈敝�A�����A�}���`�X���b�h�������ݒ�
        bRet = true;
    }
    return bRet;
}

/*!
 * @brief  SHP�t�@�C���̕��ʒ��p���W�n�̌n�ԍ��ݒ�̃Q�b�^�[
 * @return �n�ԍ�(1 - 19)
 */
int CReadParamFile::GetJPZone()
{
    return m_nJPZone;
}

/*!
 * @brief  ���H��SHP�t�@�C���p�X�ݒ�̃Q�b�^�[
 * @return ���H��SHP�t�@�C���p�X
*/
std::string CReadParamFile::GetRoadSHPPath()
{
    return m_strRoadSHPPath;
}

/*!
 * @brief  ���H�{��(�_)SHP�t�@�C���p�X�ݒ�̃Q�b�^�[
 * @return ���H�{��(�_)SHP�t�@�C���p�X
*/
std::string CReadParamFile::GetRoadFacilitiesPointSHPPath()
{
    return m_strRoadFacilitiesPointSHPPath;
}

/*!
 * @brief  ���H�{��(��)SHP�t�@�C���p�X�ݒ�̃Q�b�^�[
 * @return ���H�{��(��)SHP�t�@�C���p�X
*/
std::string CReadParamFile::GetRoadFacilitiesLineSHPPath()
{
    return m_strRoadFacilitiesLineSHPPath;
}

/*!
 * @brief  ���H�{��(��)SHP�t�@�C���p�X�ݒ�̃Q�b�^�[
 * @return ���H�{��(��)SHP�t�@�C���p�X
*/
std::string CReadParamFile::GetRoadFacilitiesPolygonSHPPath()
{
    return m_strRoadFacilitiesPolygonSHPPath;
}

/*!
 * @brief  ���H����͏����̒��ڗ̈敝(m)�̃Q�b�^�[
 * @return ���H����͏����̒��ڗ̈敝(m)
 */
int CReadParamFile::GetRegionWidth()
{
    return m_nRegionWidth;
}

/*!
 *@brief  ���H����͏����̒��ڗ̈捂��(m)�̃Q�b�^�[
 *@return ���H����͏����̒��ڗ̈捂��(m)
 */
int CReadParamFile::GetRegionHeight()
{
    return m_nRegionHeight;
}

/*!
 *@brief  �}���`�X���b�h���̃Q�b�^�[
 *@return �}���`�X���b�h��
 */
int CReadParamFile::GetThreadNum()
{
    return m_nThreadNum;
}

/*!
 * @brief  �o�̓t�H���_�p�X�ݒ�̃Q�b�^�[
 * @return �o�̓t�H���_�p�X
*/
std::string CReadParamFile::GetOutputFolderPath()
{
    return m_strOutputFolderPath;
}

/*!
 * @brief  DM�R�[�h�������ݒ�̃Q�b�^�[
 * @return DM�R�[�h������
*/
std::string CReadParamFile::GetDMCodeAttribute()
{
    return m_strDMCodeAttr;
}

/*!
 * @brief  D�}�`�敪�R�[�h�̑������ݒ�̃Q�b�^�[
 * @return �}�`�敪�R�[�h�̑�����
*/
std::string CReadParamFile::GetGeometryTypeAttribute()
{
    return m_strGeometryTypeAttr;
}

/*!
 * @brief  ���R�[�h�^�C�v�������ݒ�̃Q�b�^�[
 * @return ���R�[�h�^�C�v������
*/
double CReadParamFile::GetMinArea()
{
    return m_minArea;
}

/*!
 * @brief  D�}�`�敪�R�[�h�̑������ݒ�̃Q�b�^�[
 * @return �}�`�敪�R�[�h�̑�����
*/
double CReadParamFile::GetMaxDistance()
{
    return m_maxDistance;
}

std::string CReadParamFile::GetRoadBeforeDivisionShpFilePath()
{
    return m_strRoadBeforeDivisionShpFilePath;
}

std::string CReadParamFile::GetDividedRoadShpFilePath()
{
    return m_strDividedRoadShpFilePath;
}

std::string CReadParamFile::GetIntersectionShpFilePath()
{
    return m_strIntersectionShpFilePath;
}
