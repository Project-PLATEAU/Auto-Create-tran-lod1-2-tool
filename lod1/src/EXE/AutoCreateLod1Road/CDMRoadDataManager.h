#pragma once
#include <string>
#include <algorithm>
#include <iterator>
#include "../../LIB/CommonUtil/CGeoUtil.h"
#include "DMType.h"
#include "shapefil.h"


class CDMRoadDataManager
{
public:
    /*!
     * @brief ���H�{�݂̃f�[�^�`��^�C�v
    */
    enum class RoadFacilitiesDataType
    {
        ROAD_FACILITIES_UNKNOWN = 0,    //!< ���ݒ�
        ROAD_FACILITIES_POINT_DATA,     //!< �_�f�[�^
        ROAD_FACILITIES_LINE_DATA,      //!< ���f�[�^
        ROAD_FACILITIES_POLYGON_DATA,   //!< �ʃf�[�^

    };

    /*!
     * @brief ���H���f�[�^
    */
    struct RoadEdgeData
    {
        //int                         nId;            //!< id
        DMCode                      nDMCode;        //!< DM�R�[�h(�啪��)
        DMRoadCode                  nRoadCode;      //!< DM�R�[�h(������)
        //DMRecordType                nRecordType;    //!< ���R�[�h�^�C�v
        //DMNumericClassificationType nNumClassType;  //!< ���x�敪(���l���敪)
        //DMMapInfoLevelType          nMapInfoLevel;  //!< ���x�敪(�n�}��񃌃x��)
        //int                         nInterrupt;     //!< �Ԓf�敪 0:�Ԓf���Ȃ� 1-9:�Ԓf����(���l�͗D�揇��)
        std::vector<CVector3D>      vecPolyline;    //!< �|�����C�����_��

        /*!
         * �R���X�g���N�^
         */
        RoadEdgeData()
        {
            //nId = -1;
            nDMCode = DMCode::UNCATEGORIZED;
            nRoadCode = DMRoadCode::UNCATEGORIZED_ROAD;
            //nRecordType = DMRecordType::DM_RECORD_PLANE;
            //nNumClassType = DMNumericClassificationType::OTHER_NUMERIC_CLASSIFICATION;
            //nMapInfoLevel = DMMapInfoLevelType::OTHER_LEVEL;
            //nInterrupt = 0;
        }

        /*!
         * �f�X�g���N�^
         */
        virtual ~RoadEdgeData() {}

        /*!
         * �R�s�[�R���X�g���N�^
         */
        RoadEdgeData(const RoadEdgeData& x) { *this = x; }

        /*!
         * ������Z�q
         */
        RoadEdgeData& operator=(const RoadEdgeData& x)
        {
            if (this != &x)
            {
                //nId = x.nId;
                nDMCode = x.nDMCode;
                nRoadCode = x.nRoadCode;
                //nRecordType = x.nRecordType;
                //nNumClassType = x.nNumClassType;
                //nMapInfoLevel = x.nMapInfoLevel;
                std::copy(x.vecPolyline.begin(), x.vecPolyline.end(), std::back_inserter(vecPolyline));
            }
            return *this;
        }
    };

    /*!
     * @brief ���H�{�݃f�[�^
    */
    struct RoadFacilitiesData
    {
        //int nId;    //!< id
        std::vector<CVector3D>  vecPolyline;                //!< �|�����C�����_��
        DMCode                  nDMCode;                    //!< DM�R�[�h(�啪��)
        DMRoadFacilitiesCode    nRoadFacilitiesCode;        //!< DM�R�[�h(������)
        DMGeometryType          nGeometryType;              //!< �}�`�敪
        RoadFacilitiesDataType  nRoadFacilitiesDataType;    //!< ���H�{�݂̌`��f�[�^�^�C�v

        /*!
         * �R���X�g���N�^
         */
        RoadFacilitiesData()
        {
            //nId = -1;
            nDMCode = DMCode::UNCATEGORIZED;
            nRoadFacilitiesCode = DMRoadFacilitiesCode::UNCATEGORIZED_ROAD_FACILITIES;
            nGeometryType = DMGeometryType::UNCLASSIFIED;
            nRoadFacilitiesDataType = RoadFacilitiesDataType::ROAD_FACILITIES_UNKNOWN;
        }

        /*!
         * �f�X�g���N�^
         */
        virtual ~RoadFacilitiesData() {}

        /*!
         * �R�s�[�R���X�g���N�^
         */
        RoadFacilitiesData(const RoadFacilitiesData& x) { *this = x; }

        /*!
         * ������Z�q
         */
        RoadFacilitiesData& operator=(const RoadFacilitiesData& x)
        {
            if (this != &x)
            {
                //nId = x.nId;
                nDMCode = x.nDMCode;
                nRoadFacilitiesCode = x.nRoadFacilitiesCode;
                nGeometryType = x.nGeometryType;
                nRoadFacilitiesDataType = x.nRoadFacilitiesDataType;
                std::copy(x.vecPolyline.begin(), x.vecPolyline.end(), std::back_inserter(vecPolyline));
            }
            return *this;
        }
    };

    CDMRoadDataManager(void) {}     //!< �R���X�g���N�^
    ~CDMRoadDataManager(void) {}    //!< �f�X�g���N�^
    bool ReadRoadEdgeShapeFile(void);   // ���H��shape�ǂݍ���
    void ReadRoadFacilitiesShapeFile(void);   // ���H�{��shape�ǂݍ���

    /*!
     * @brief  ���H���f�[�^�̃Q�b�^�[
     * @return ���H���f�[�^
    */
    std::vector<RoadEdgeData> GetRoadEdges() { return m_vecRoadEdgeData; }

    /*!
     * @brief  ���H�{�݃f�[�^�̃Q�b�^�[
     * @return ���H�{�݃f�[�^
    */
    std::vector<RoadFacilitiesData> GetRoadFacilities() { return m_vecRoadFacilitiesData; }

protected:

private:
    std::vector<RoadEdgeData>       m_vecRoadEdgeData;              //!< ���H���f�[�^�z��
    std::vector<RoadFacilitiesData> m_vecRoadFacilitiesData;        //!< ���H�{�݃f�[�^�z��

    void separateDMCode(int nCode, int& nUpper, int& nLower);       //!< DM�R�[�h����
    int combineDMCode(int nUpper, int nLower);                      //!< DM�R�[�h����
    void separateAccuracyCode(int nCode, int& nUpper, int& nLower); //!< ���x�敪�R�[�h����
    int separateParts(SHPObject* psElem, std::vector<std::vector<CVector3D>>& vecParts);  //!< �p�[�g����

    //!< ���H�{��shape�t�@�C���ǂݍ���(�����֐�)
    bool readRoadFacilitiesShapeFile(const std::string &strShpPath, RoadFacilitiesDataType type);
};
