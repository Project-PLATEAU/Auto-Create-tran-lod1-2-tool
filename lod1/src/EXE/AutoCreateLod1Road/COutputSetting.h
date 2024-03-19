#pragma once
#include <string>
#include <filesystem>
#include "CFileUtil.h"

/*!
 * @brief �o�̓t�@�C���ݒ�N���X
*/
class COutputSetting
{
public:
    static COutputSetting *GetInstance() { return &m_instance; }    // �C���X�^���X�擾

    // ������
    bool Initialize(
        std::string strOutputFolderPath,
        std::string strShpFileName,
        std::string strErrFileName);

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

    std::string GetShpFilePath(void);           // shp�t�@�C���p�X(�������|���S��)
    std::string GetShpFilePathWithHoles(void);  // shp�t�@�C���p�X(���L��|���S��)
    std::string GetErrFilePath(void);           // �G���[�`�F�b�N���ʂ̃t�@�C���p�X

protected:

private:

    COutputSetting(void);                   // �R���X�g���N�^
    virtual ~COutputSetting(void);          // �f�X�g���N�^

    static COutputSetting m_instance;       //!< ���N���X�B��̃C���X�^���X
    std::string m_strShpFilePath;           //!< shp�t�@�C���p�X(�������|���S��)
    std::string m_strShpFilePathWithHoles;  //!< shp�t�@�C���p�X(���L��|���S��)
    std::string m_strErrFilePath;           //!< �G���[�`�F�b�N���ʂ̃t�@�C���p�X

};

#define GetOutputSetting() (COutputSetting::GetInstance())
