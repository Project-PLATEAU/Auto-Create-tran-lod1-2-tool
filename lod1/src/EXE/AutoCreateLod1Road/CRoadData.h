#pragma once
#include "AnalyzeRoadEdgeCommon.h"

/*!
 * @brief ���H�`��^�C�v
*/
enum class RoadSectionType
{
	ROAD_SECTION_UNKNOWN = 0,   //!< ���ݒ�
	ROAD_SECTION_BRIDGE = 2,          //!< ���ˋ�
	ROAD_SECTION_CROSSING = 4,     //!< �����_
	ROAD_SECTION_TUNNEL = 6,         //!< �g���l��
};

/*!
 * @brief ���H�|���S���f�[�^�N���X
 */
class CRoadData
{
public:
	/*!
	 * @brief �R���X�g���N�^
	 */
	CRoadData()
	{
		m_type = RoadSectionType::ROAD_SECTION_UNKNOWN;
		m_division = 0;
	}

	/*!
	 * @brief �f�X�g���N�^
	 */
	~CRoadData() {};

	/*!
	 * @brief �R�s�[�R���X�g���N�^
	 * @param[in] x �R�s�[���f�[�^
	 */
	CRoadData(const CRoadData& x) { *this = x; }

	/*!
	 * @brief ������Z�q
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
	 * @brief ���H�|���S���̃Q�b�^�[
	 * @return ���H�|���S��
	*/
	BoostPolygon Polygon() { return m_polygon; }

	/*!
	 * @brief ���H�|���S���̃Z�b�^�[
	 * @param[in] polygon ���H�|���S��
	*/
	void Polygon(BoostPolygon polygon) { m_polygon = polygon; }

	/*!
	 * @brief ���H�`��^�C�v�̃Q�b�^�[
	 * @return ���H�`��^�C�v
	*/
	RoadSectionType Type() { return m_type; }

	/*!
	 * @brief ���H�`��^�C�v�̃Z�b�^�[
	 * @param[in] type ���H�`��^�C�v
	*/
	void Type(RoadSectionType type) { m_type = type; }

	/*!
	* @brief ���H�������̃Q�b�^�[
	* @return ���H������
	*/
	int Division() { return m_division; }

	/*!
	 * @brief ���H�������̃Z�b�^�[
	 * @param[in] division ���H������
	*/
	void Division(int division) { m_division = division; }

protected:
private:
	BoostPolygon m_polygon;     //!< ���H�|���S��
	RoadSectionType m_type;     //!< ���H�`��^�C�v
	int m_division;             //!< ���H������
};

