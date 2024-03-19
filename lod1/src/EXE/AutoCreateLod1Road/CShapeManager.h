#pragma once
#include <string>
#include <algorithm>
#include <iterator>
#include <vector>
#include "shapefil.h"
#include "AnalyzeRoadEdgeCommon.h"

/*!
 * @brief shape�t�@�C���Ǘ��N���X
*/
class CShapeManager
{
public:
    SHPHandle hSHP;     //!< shp file�̃n���h��
    DBFHandle hDBF;     //!< dbf file�̃n���h��

    int nShapeType;         //!< ���
    int nEntities;          //!< �v�f��
    int nField;             //!< field��
    double adfMinBound[4] = { 0, 0, 0, 0 }; //!< �o�E���f�B���O(�ŏ��l)
    double adfMaxBound[4] = { 0, 0, 0, 0 }; //!< �o�E���f�B���O(�ő�l)

    CShapeManager(void);                            //!< �R���X�g���N�^
    ~CShapeManager(void);                           //!< �f�X�g���N�^
    bool Open(std::string strShpPath);              //!< open shape file
    bool OpenWithoutDbf(std::string strShpPath);    //!< open shape file without dbf file
    void Close(void);                               //!< close shape file

protected:

private:
};

class CShapeAttribute
{
public:

    /*!
     * @brief �����l�̃t�B�[���h��`�^
    */
    enum class AttributeFieldType
    {
        ATTR_FIELD_TYPE_INT = 0,    //!< int�^
        ATTR_FIELD_TYPE_DOUBLE,     //!< double�^
        ATTR_FIELD_TYPE_STRING,     //!< ������^
    };

    /*!
     * @brief �����l�̃f�[�^�^
    */
    enum class AttributeDataType
    {
        ATTR_DATA_TYPE_NULL = 0,    //!< NULL�^
        ATTR_DATA_TYPE_INT,         //!< int�^
        ATTR_DATA_TYPE_DOUBLE,      //!< double�^
        ATTR_DATA_TYPE_STRING,      //!< ������^
    };

    /*!
     * @brief �����l�̃t�B�[���h��`�f�[�^
    */
    struct AttributeFieldData
    {
        CShapeAttribute::AttributeFieldType fieldType; //!< �����l�̃t�B�[���h��`�^
        std::string strName;    //!< ������
        int nWidth;             //!< �f�[�^�T�C�Y or ����������(double�̏ꍇ�͐���������+�����_����1��)
        int nDecimals;          //!< ����������


        /*!
         * �R���X�g���N�^
         */
        AttributeFieldData()
        {
            fieldType = CShapeAttribute::AttributeFieldType::ATTR_FIELD_TYPE_INT;
            strName = "";
            nWidth = 0;
            nDecimals = 0;
        }

        /*!
         * �f�X�g���N�^
         */
        virtual ~AttributeFieldData() {}

        /*!
         * �R�s�[�R���X�g���N�^
         */
        AttributeFieldData(const AttributeFieldData &x) { *this = x; }

        /*!
         * ������Z�q
         */
        AttributeFieldData &operator=(const AttributeFieldData &x)
        {
            if (this != &x)
            {
                fieldType = x.fieldType;
                strName = x.strName;
                nWidth = x.nWidth;
                nDecimals = x.nDecimals;
            }
            return *this;
        }

    };

    /*!
     * @brief �����l�f�[�^
    */
    struct AttributeData
    {
        CShapeAttribute::AttributeDataType dataType;   //!< �����l�̃f�[�^�^
        int nValue;                                 //!< �����l
        double dValue;                              //!< �����l
        std::string strValue;                       //!< ������

        /*!
         * �R���X�g���N�^
         */
        AttributeData()
        {
            dataType = CShapeAttribute::AttributeDataType::ATTR_DATA_TYPE_NULL;
            nValue = 0;
            dValue = 0;
            strValue = "";
        }

        /*!
         * @brief �R���X�g���N�^
         * @param[in] nVal �����l
         */
        AttributeData(int nVal)
        {
            dataType = CShapeAttribute::AttributeDataType::ATTR_DATA_TYPE_INT;
            nValue = nVal;
            dValue = 0;
            strValue = "";
        }

        /*!
         * @brief �R���X�g���N�^
         * @param[in] dVal �����l
         */
        AttributeData(double dVal)
        {
            dataType = CShapeAttribute::AttributeDataType::ATTR_DATA_TYPE_DOUBLE;
            nValue = 0;
            dValue = dVal;
            strValue = "";
        }

        /*!
         * @brief �R���X�g���N�^
         * @param[in] str ������
         */
        AttributeData(std::string str)
        {
            dataType = CShapeAttribute::AttributeDataType::ATTR_DATA_TYPE_STRING;
            nValue = 0;
            dValue = 0;
            strValue = str;
        }

        /*!
         * �f�X�g���N�^
         */
        virtual ~AttributeData() {}

        /*!
         * �R�s�[�R���X�g���N�^
         */
        AttributeData(const AttributeData &x) { *this = x; }

        /*!
         * ������Z�q
         */
        AttributeData &operator=(const AttributeData &x)
        {
            if (this != &x)
            {
                dataType = x.dataType;
                nValue = x.nValue;
                dValue = x.dValue;
                strValue = x.strValue;
            }
            return *this;
        }
    };

    /*!
     * @brief 1���R�[�h���̑����l�f�[�^
    */
    struct AttributeDataRecord
    {
        int nShapeId;       //!< shape id
        std::vector<CShapeAttribute::AttributeData> vecAttribute;  //!< �����f�[�^

        /*!
         * �R���X�g���N�^
         */
        AttributeDataRecord()
        {
            nShapeId = 0;
        }

        /*!
         * �f�X�g���N�^
         */
        virtual ~AttributeDataRecord() {}

        /*!
         * �R�s�[�R���X�g���N�^
         */
        AttributeDataRecord(const AttributeDataRecord &x) { *this = x; }

        /*!
         * ������Z�q
         */
        AttributeDataRecord &operator=(const AttributeDataRecord &x)
        {
            if (this != &x)
            {
                nShapeId = x.nShapeId;
                std::copy(x.vecAttribute.begin(), x.vecAttribute.end(), std::back_inserter(vecAttribute));
            }
            return *this;
        }
    };
};

/*!
 * @brief shape�t�@�C���������݃N���X
*/
class CShapeWriter
{
public:
    CShapeWriter(void) {}   //!< �R���X�g���N�^
    ~CShapeWriter(void) {}  //!< �f�X�g���N�^

    // �|���S����shape file�o��
    bool OutputPolygons(
        const BoostMultiPolygon &polygons,
        std::string strShpPath,
        const std::vector<CShapeAttribute::AttributeFieldData> &vecFields,
        const std::vector<CShapeAttribute::AttributeDataRecord> &vecAttrRecords,
        const bool bHole = false);


    // �|�����C����shapefile�o��
    bool OutputPolylines(
        const BoostMultiLines &polylines,
        std::string strShpPath,
        const std::vector<CShapeAttribute::AttributeFieldData> &vecFields,
        const std::vector<CShapeAttribute::AttributeDataRecord> &vecAttrRecords);

    // �}���`�|�C���g��shapfile�o��
    bool OutputMultiPoints(
        const BoostMultiPoints &points,
        std::string strShpPath,
        const std::vector<CShapeAttribute::AttributeFieldData> &vecFields,
        const std::vector<CShapeAttribute::AttributeDataRecord> &vecAttrRecords);

protected:

private:
    // �������̏�������
    bool writeAttribute(
        std::string strShpPath,
        const std::vector<CShapeAttribute::AttributeFieldData> &vecFields,
        const std::vector<CShapeAttribute::AttributeDataRecord> &vecAttrRecords,
        const size_t shapeNum);
};

/*!
 * @brief shape�t�@�C���ǂݍ��݃N���X
*/
class CShapeReader
{
public:

    CShapeReader(void) {}   //!< �R���X�g���N�^
    ~CShapeReader(void) {}  //!< �f�X�g���N�^

    // �|���S���ǂݍ���(�����f�[�^���ǂݍ���)
    bool ReadPolygons(
        const std::string strShpPath,
        BoostMultiPolygon &polygons,
        std::vector<CShapeAttribute::AttributeFieldData> &vecFields,
        std::vector<CShapeAttribute::AttributeDataRecord> &vecAttrRecords);

    // �|���S���ǂݍ���(�����f�[�^�͖���)
    bool ReadPolygons(
        const std::string &strShpPath,
        BoostMultiPolygon &polygons);

    // �|�����C���ǂݍ���(�����f�[�^�͖���)
    bool ReadPolylines(
        const std::string &strShpPath,
        BoostMultiLines &polylines);

    // �}���`�|�C���g�ǂݍ���(�����f�[�^�͖���)
    bool ReadPoints(
        const std::string &strShpPath,
        BoostMultiPoints &points);
protected:

private:

    // �p�[�g����
    int separateParts(
        SHPObject *psElem,
        std::vector<std::vector<BoostPoint>> &vecParts);

    // �����ǂݍ���
    int readAttribute(
        CShapeManager &shpMng,
        std::vector<CShapeAttribute::AttributeFieldData> &vecFields,
        std::vector<CShapeAttribute::AttributeDataRecord> &vecAttrRecords);

    // �|���S���ǂݍ���
    int readPolygons(
        CShapeManager &shpMng,
        BoostMultiPolygon &polygons);
};
