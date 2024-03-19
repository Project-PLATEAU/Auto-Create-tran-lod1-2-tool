#pragma once
#include <string>
#include <algorithm>
#include <iterator>
#include <vector>
#include "shapefil.h"
#include "AnalyzeRoadEdgeCommon.h"

/*!
 * @brief shapeファイル管理クラス
*/
class CShapeManager
{
public:
    SHPHandle hSHP;     //!< shp fileのハンドル
    DBFHandle hDBF;     //!< dbf fileのハンドル

    int nShapeType;         //!< 種別
    int nEntities;          //!< 要素数
    int nField;             //!< field数
    double adfMinBound[4] = { 0, 0, 0, 0 }; //!< バウンディング(最小値)
    double adfMaxBound[4] = { 0, 0, 0, 0 }; //!< バウンディング(最大値)

    CShapeManager(void);                            //!< コンストラクタ
    ~CShapeManager(void);                           //!< デストラクタ
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
     * @brief 属性値のフィールド定義型
    */
    enum class AttributeFieldType
    {
        ATTR_FIELD_TYPE_INT = 0,    //!< int型
        ATTR_FIELD_TYPE_DOUBLE,     //!< double型
        ATTR_FIELD_TYPE_STRING,     //!< 文字列型
    };

    /*!
     * @brief 属性値のデータ型
    */
    enum class AttributeDataType
    {
        ATTR_DATA_TYPE_NULL = 0,    //!< NULL型
        ATTR_DATA_TYPE_INT,         //!< int型
        ATTR_DATA_TYPE_DOUBLE,      //!< double型
        ATTR_DATA_TYPE_STRING,      //!< 文字列型
    };

    /*!
     * @brief 属性値のフィールド定義データ
    */
    struct AttributeFieldData
    {
        CShapeAttribute::AttributeFieldType fieldType; //!< 属性値のフィールド定義型
        std::string strName;    //!< 属性名
        int nWidth;             //!< データサイズ or 整数部桁数(doubleの場合は整数部桁数+小数点分の1桁)
        int nDecimals;          //!< 小数部桁数


        /*!
         * コンストラクタ
         */
        AttributeFieldData()
        {
            fieldType = CShapeAttribute::AttributeFieldType::ATTR_FIELD_TYPE_INT;
            strName = "";
            nWidth = 0;
            nDecimals = 0;
        }

        /*!
         * デストラクタ
         */
        virtual ~AttributeFieldData() {}

        /*!
         * コピーコンストラクタ
         */
        AttributeFieldData(const AttributeFieldData &x) { *this = x; }

        /*!
         * 代入演算子
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
     * @brief 属性値データ
    */
    struct AttributeData
    {
        CShapeAttribute::AttributeDataType dataType;   //!< 属性値のデータ型
        int nValue;                                 //!< 整数値
        double dValue;                              //!< 実数値
        std::string strValue;                       //!< 文字列

        /*!
         * コンストラクタ
         */
        AttributeData()
        {
            dataType = CShapeAttribute::AttributeDataType::ATTR_DATA_TYPE_NULL;
            nValue = 0;
            dValue = 0;
            strValue = "";
        }

        /*!
         * @brief コンストラクタ
         * @param[in] nVal 整数値
         */
        AttributeData(int nVal)
        {
            dataType = CShapeAttribute::AttributeDataType::ATTR_DATA_TYPE_INT;
            nValue = nVal;
            dValue = 0;
            strValue = "";
        }

        /*!
         * @brief コンストラクタ
         * @param[in] dVal 実数値
         */
        AttributeData(double dVal)
        {
            dataType = CShapeAttribute::AttributeDataType::ATTR_DATA_TYPE_DOUBLE;
            nValue = 0;
            dValue = dVal;
            strValue = "";
        }

        /*!
         * @brief コンストラクタ
         * @param[in] str 文字列
         */
        AttributeData(std::string str)
        {
            dataType = CShapeAttribute::AttributeDataType::ATTR_DATA_TYPE_STRING;
            nValue = 0;
            dValue = 0;
            strValue = str;
        }

        /*!
         * デストラクタ
         */
        virtual ~AttributeData() {}

        /*!
         * コピーコンストラクタ
         */
        AttributeData(const AttributeData &x) { *this = x; }

        /*!
         * 代入演算子
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
     * @brief 1レコード分の属性値データ
    */
    struct AttributeDataRecord
    {
        int nShapeId;       //!< shape id
        std::vector<CShapeAttribute::AttributeData> vecAttribute;  //!< 属性データ

        /*!
         * コンストラクタ
         */
        AttributeDataRecord()
        {
            nShapeId = 0;
        }

        /*!
         * デストラクタ
         */
        virtual ~AttributeDataRecord() {}

        /*!
         * コピーコンストラクタ
         */
        AttributeDataRecord(const AttributeDataRecord &x) { *this = x; }

        /*!
         * 代入演算子
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
 * @brief shapeファイル書き込みクラス
*/
class CShapeWriter
{
public:
    CShapeWriter(void) {}   //!< コンストラクタ
    ~CShapeWriter(void) {}  //!< デストラクタ

    // ポリゴンのshape file出力
    bool OutputPolygons(
        const BoostMultiPolygon &polygons,
        std::string strShpPath,
        const std::vector<CShapeAttribute::AttributeFieldData> &vecFields,
        const std::vector<CShapeAttribute::AttributeDataRecord> &vecAttrRecords,
        const bool bHole = false);


    // ポリラインのshapefile出力
    bool OutputPolylines(
        const BoostMultiLines &polylines,
        std::string strShpPath,
        const std::vector<CShapeAttribute::AttributeFieldData> &vecFields,
        const std::vector<CShapeAttribute::AttributeDataRecord> &vecAttrRecords);

    // マルチポイントのshapfile出力
    bool OutputMultiPoints(
        const BoostMultiPoints &points,
        std::string strShpPath,
        const std::vector<CShapeAttribute::AttributeFieldData> &vecFields,
        const std::vector<CShapeAttribute::AttributeDataRecord> &vecAttrRecords);

protected:

private:
    // 属性情報の書き込み
    bool writeAttribute(
        std::string strShpPath,
        const std::vector<CShapeAttribute::AttributeFieldData> &vecFields,
        const std::vector<CShapeAttribute::AttributeDataRecord> &vecAttrRecords,
        const size_t shapeNum);
};

/*!
 * @brief shapeファイル読み込みクラス
*/
class CShapeReader
{
public:

    CShapeReader(void) {}   //!< コンストラクタ
    ~CShapeReader(void) {}  //!< デストラクタ

    // ポリゴン読み込み(属性データも読み込む)
    bool ReadPolygons(
        const std::string strShpPath,
        BoostMultiPolygon &polygons,
        std::vector<CShapeAttribute::AttributeFieldData> &vecFields,
        std::vector<CShapeAttribute::AttributeDataRecord> &vecAttrRecords);

    // ポリゴン読み込み(属性データは無視)
    bool ReadPolygons(
        const std::string &strShpPath,
        BoostMultiPolygon &polygons);

    // ポリライン読み込み(属性データは無視)
    bool ReadPolylines(
        const std::string &strShpPath,
        BoostMultiLines &polylines);

    // マルチポイント読み込み(属性データは無視)
    bool ReadPoints(
        const std::string &strShpPath,
        BoostMultiPoints &points);
protected:

private:

    // パート分割
    int separateParts(
        SHPObject *psElem,
        std::vector<std::vector<BoostPoint>> &vecParts);

    // 属性読み込み
    int readAttribute(
        CShapeManager &shpMng,
        std::vector<CShapeAttribute::AttributeFieldData> &vecFields,
        std::vector<CShapeAttribute::AttributeDataRecord> &vecAttrRecords);

    // ポリゴン読み込み
    int readPolygons(
        CShapeManager &shpMng,
        BoostMultiPolygon &polygons);
};
