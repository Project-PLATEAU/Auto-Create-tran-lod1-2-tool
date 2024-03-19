#pragma once
#include "CINIFileIO.h"

/*!
 * @brief �ݒ�t�@�C���ǂݍ��݃N���X
*/
class CReadParamFile
{
public:
    static CReadParamFile* GetInstance() { return &m_instance; }    // �C���X�^���X�擾
    bool Initialize(std::string strParamPath);  // ������
    bool IsErrCheckFromShp();

    int GetJPZone();                                    // ���ʒ��p���W�n�̌n�ԍ�
    std::string GetRoadSHPPath();                       // ���H��SHP�t�@�C���p�X
    std::string GetRoadFacilitiesPointSHPPath();        // ���H�{��(�_)SHP�t�@�C���p�X
    std::string GetRoadFacilitiesLineSHPPath();         // ���H�{��(��)SHP�t�@�C���p�X
    std::string GetRoadFacilitiesPolygonSHPPath();      // ���H�{��(��)SHP�t�@�C���p�X
    std::string GetOutputFolderPath();                  // �o�̓t�H���_�p�X
    std::string GetDMCodeAttribute();                   // DM�R�[�h�̑�����
    std::string GetGeometryTypeAttribute();             // �}�`�敪�R�[�h�̑�����
    double GetMinArea();                                // �|���S���̖ʐς̍ŏ��l
    double GetMaxDistance();                            // �ԓ��������|���S�����S�ƌ����_�̋����̍ő�l
    std::string GetRoadBeforeDivisionShpFilePath();     // �����O�̓��H�|���S���̃t�@�C���p�X(Debug)
    std::string GetDividedRoadShpFilePath();            // ������̓��H�|���S���̃t�@�C���p�X(Debug)
    std::string GetIntersectionShpFilePath();           // �����_���̃t�@�C���p�X(Debug)
    int GetRegionWidth();                               // ���H����͏����̒��ڗ̈敝(m)
    int GetRegionHeight();                              // ���H����͏����̒��ڗ̈捂��(m)
    int GetThreadNum();                                 // �}���`�X���b�h��

protected:

private:
    CReadParamFile(void);                       // �R���X�g���N�^
    virtual ~CReadParamFile(void);              // �f�X�g���N�^
    bool checkParam(void);                      // �p�����[�^�̃G���[�`�F�b�N

    static CReadParamFile m_instance;           // ���N���X�B��̃C���X�^���X
    // ��{�ݒ�
    std::string m_strRoadSHPPath;                   // ���H��SHP�t�@�C���p�X
    std::string m_strRoadFacilitiesPointSHPPath;    // ���H�{��(�_)SHP�t�@�C���p�X
    std::string m_strRoadFacilitiesLineSHPPath;     // ���H�{��(��)SHP�t�@�C���p�X
    std::string m_strRoadFacilitiesPolygonSHPPath;  // ���H�{��(��)SHP�t�@�C���p�X
    std::string m_strOutputFolderPath;              // �o�̓t�H���_�p�X
    int m_nJPZone;                                  // ���ʒ��p���W�n�̌n�ԍ�
    int m_nRegionWidth;                             // ���H����͏����̒��ڗ̈敝(m)
    int m_nRegionHeight;                            // ���H����͏����̒��ڗ̈捂��(m)
    int m_nThreadNum;                               // �}���`�X���b�h��

    // DM�̑����ݒ�
    std::string m_strDMCodeAttr;                // DM�R�[�h�̑�����
    //std::string m_strRecordTypeAttr;            // ���R�[�h�^�C�v�̑�����
    std::string m_strGeometryTypeAttr;          // �}�`�敪�R�[�h�̑�����

    // �G���[�`�F�b�N�̐ݒ�
    std::string m_strMinArea;                           // �|���S���̖ʐς̍ŏ��l(���͊m�F�p)
    double m_minArea;                                // �|���S���̖ʐς̍ŏ��l
    std::string m_strMaxDistance;                       // �ԓ��������|���S�����S�ƌ����_�̋����̍ő�l(���͊m�F�p)
    double m_maxDistance;                            // �ԓ��������|���S�����S�ƌ����_�̋����̍ő�l
    std::string m_strRoadBeforeDivisionShpFilePath;     // �����O�̓��H�|���S���̃t�@�C���p�X(Debug)
    std::string m_strDividedRoadShpFilePath;            // ������̓��H�|���S���̃t�@�C���p�X(Debug)
    std::string m_strIntersectionShpFilePath;           // �����_���̃t�@�C���p�X(Debug)
};

#define GetCreateParam() (CReadParamFile::GetInstance())
