#include "pch.h"
#include "COutputSetting.h"
#include "boost/format.hpp"
#include "CTime.h"

COutputSetting COutputSetting::m_instance;  // �C���X�^���X

/*!
 * @brief �R���X�g���N�^
 */
COutputSetting::COutputSetting(void)
    : m_strShpFilePath(""),
    m_strShpFilePathWithHoles(""),
    m_strErrFilePath("")
{

}

/*!
 * @brief �f�X�g���N�^
 */
COutputSetting::~COutputSetting(void)
{
}

/*!
 * @brief ������
 * @param[in]   strOutputFolderPath �o�̓t�H���_�p�X
 * @param[in]   strShpFileName      shape�t�@�C����
 * @param[in]   strErrFileName      �G���[�`�F�b�N���ʂ̃t�@�C����
 * @return      ��������
 * @retval      true     ����
 * @retval      false    ���s
*/
bool COutputSetting::Initialize(
    std::string strOutputFolderPath,
    std::string strShpFileName,
    std::string strErrFileName)
{
    // �o�̓t�H���_�����݂��Ȃ��ꍇ�͍쐬(���݂���ꍇ�͉������Ȃ�)
    bool bRet = COutputSetting::CreateFolder(strOutputFolderPath);

    // �o��shp�t�@�C���p�X
    std::string strTempShpFileName = (boost::format("%s.shp") % strShpFileName).str();
    std::string strShpPath = CFileUtil::Combine(strOutputFolderPath, strTempShpFileName);   // ������
    strTempShpFileName = (boost::format("%s_withHoles.shp") % strShpFileName).str();
    std::string strShpPathWithHoles = CFileUtil::Combine(strOutputFolderPath, strTempShpFileName);   // ������
    // �G���[�`�F�b�N���ʂ̃t�@�C���p�X
    std::string strTempErrFileName = (boost::format("%s.csv") % strErrFileName).str();
    std::string strErrPath = CFileUtil::Combine(strOutputFolderPath, strTempErrFileName);

    if (CFileUtil::IsExistPath(strShpPath))
    {
        // �����t�@�C�������݂���ꍇ�͏o�̓t�@�C������ύX����
        std::string strTime = CTime::GetCurrentTime().Format("%Y%m%d_%H%M%S");  // ���ݎ���
        strTempShpFileName = (boost::format("%s_%s.shp") % strShpFileName % strTime).str();
        strShpPath = CFileUtil::Combine(strOutputFolderPath, strTempShpFileName);   // ������

        strTempShpFileName = (boost::format("%s_withHoles_%s.shp") % strShpFileName % strTime).str();
        strShpPathWithHoles = CFileUtil::Combine(strOutputFolderPath, strTempShpFileName);  // ������

        strTempErrFileName = (boost::format("%s_%s.csv") % strErrFileName % strTime).str();
        strErrPath = CFileUtil::Combine(strOutputFolderPath, strTempErrFileName);
    }

    m_strShpFilePath = strShpPath;
    m_strShpFilePathWithHoles = strShpPathWithHoles;
    m_strErrFilePath = strErrPath;

    return bRet;
}

/*!
 * @brief  shp�t�@�C���p�X(�������|���S��)
 * @return shp�t�@�C���p�X(�������|���S��)
*/
std::string COutputSetting::GetShpFilePath(void)
{
    return m_strShpFilePath;
}

/*!
 * @brief  shp�t�@�C���p�X(���L��|���S��)
 * @return shp�t�@�C���p�X(���L��|���S��)
*/
std::string COutputSetting::GetShpFilePathWithHoles(void)
{
    return m_strShpFilePathWithHoles;
}

/*!
 * @brief  �G���[�`�F�b�N���ʂ̃t�@�C���p�X
 * @return �G���[�`�F�b�N���ʂ̃t�@�C���p�X
*/
std::string COutputSetting::GetErrFilePath(void)
{
    return m_strErrFilePath;
}
