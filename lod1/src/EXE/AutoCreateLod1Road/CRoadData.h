#pragma once
#include "AnalyzeRoadEdgeCommon.h"

/*!
 * @brief 道路形状タイプ
*/
enum class RoadSectionType
{
	ROAD_SECTION_UNKNOWN = 0,   //!< 未設定
	ROAD_SECTION_BRIDGE = 2,          //!< 高架橋
	ROAD_SECTION_CROSSING = 4,     //!< 交差点
	ROAD_SECTION_TUNNEL = 6,         //!< トンネル
};

/*!
 * @brief 道路ポリゴンデータクラス
 */
class CRoadData
{
public:
	/*!
	 * @brief コンストラクタ
	 */
	CRoadData()
	{
		m_type = RoadSectionType::ROAD_SECTION_UNKNOWN;
		m_division = 0;
	}

	/*!
	 * @brief デストラクタ
	 */
	~CRoadData() {};

	/*!
	 * @brief コピーコンストラクタ
	 * @param[in] x コピー元データ
	 */
	CRoadData(const CRoadData& x) { *this = x; }

	/*!
	 * @brief 代入演算子
	 */
	CRoadData& operator = (const CRoadData& x)
	{
		if (this != &x)
		{
			this->Polygon(x.m_polygon);
			this->Type(x.m_type);
			this->Division(x.m_division);
		}
		return *this;
	}

	/*!
	 * @brief 道路ポリゴンのゲッター
	 * @return 道路ポリゴン
	*/
	BoostPolygon Polygon() { return m_polygon; }

	/*!
	 * @brief 道路ポリゴンのセッター
	 * @param[in] polygon 道路ポリゴン
	*/
	void Polygon(BoostPolygon polygon) { m_polygon = polygon; }

	/*!
	 * @brief 道路形状タイプのゲッター
	 * @return 道路形状タイプ
	*/
	RoadSectionType Type() { return m_type; }

	/*!
	 * @brief 道路形状タイプのセッター
	 * @param[in] type 道路形状タイプ
	*/
	void Type(RoadSectionType type) { m_type = type; }

	/*!
	* @brief 道路分割数のゲッター
	* @return 道路分割数
	*/
	int Division() { return m_division; }

	/*!
	 * @brief 道路分割数のセッター
	 * @param[in] division 道路分割数
	*/
	void Division(int division) { m_division = division; }

protected:
private:
	BoostPolygon m_polygon;     //!< 道路ポリゴン
	RoadSectionType m_type;     //!< 道路形状タイプ
	int m_division;             //!< 道路分割数
};

