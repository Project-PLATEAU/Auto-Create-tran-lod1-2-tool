#include "MeshSplitter.h"
#include "CGeoUtil.h"
#include <cassert>
#include <iostream>
#include <boost/geometry.hpp>

MeshSplitter::MeshSplitter()
{
}

MeshSplitter::~MeshSplitter()
{
}


/*! 1�����b�V������
@note �ܓx�E�o�x�ɑ�������1�����b�V��
*/
bool MeshSplitter::Get1stMeshLB(
	const double& dLatDeg,	//!< in �ܓx(�x)
	const double& dLonDeg,	//!< in �o�x(�x)
	CString& strCode,		//!< out ���b�V���R�[�h
	double& dLBLatDeg,		//!< out �����ܓx(�x)
	double& dLBLonDeg		//!< out �����o�x(�x)
)
{
	strCode.Empty();
	int iCode12 = (int)(floor(dLatDeg * 1.5));	// 1,2����
	int iCode34 = (int)(floor(dLonDeg - 100));	// 3,4����

	if (iCode12 < 10 || 99 < iCode12) return false;
	if (iCode34 < 10 || 79 < iCode34) return false;	// ���o179���܂�

	strCode.Format(_T("%02d%02d"), iCode12, iCode34);


	dLBLatDeg = iCode12 / 1.5;
	dLBLonDeg = iCode34 + 100.0;

	if (strCode.GetLength() != 4)
	{
		strCode.Empty();
		return false;
	}
	return true;
}

/*! 2�����b�V������
@note �ܓx�E�o�x�ɑ�������2�����b�V��
*/
bool MeshSplitter::Get2ndMeshLB(
	const double& dLatDeg,	//!< in �ܓx(�x)
	const double& dLonDeg,	//!< in �o�x(�x)
	CString& strCode,		//!< out ���b�V���R�[�h
	double& dLBLatDeg,		//!< out �����ܓx(�x)
	double& dLBLonDeg		//!< out �����o�x(�x)
)
{
	strCode.Empty();
	double dCode12 = dLatDeg * 1.5;
	int iCode5 = (int)((dCode12 - floor(dCode12)) / 1.5 * 60 / 5.0);	// 5����
	int iCode6 = (int)((dLonDeg - floor(dLonDeg)) * 60 / 7.5);		// 6����

	CString strCode1;
	double dLBLatDeg1, dLBLonDeg1;
	if (!Get1stMeshLB(dLatDeg, dLonDeg, strCode1, dLBLatDeg1, dLBLonDeg1))
	{
		return false;
	}
	strCode.Format(_T("%4s%1d%1d"), strCode1, iCode5, iCode6);

	dLBLatDeg = dLBLatDeg1 + iCode5 * 5.0 / 60.0;
	dLBLonDeg = dLBLonDeg1 + iCode6 * 7.5 / 60.0;

	if (strCode.GetLength() != 6)
	{
		strCode.Empty();
		return false;
	}
	return true;
}

/*! 2�����b�V������
@note �ܓx�E�o�x�ɑ�������2�����b�V��
*/
bool MeshSplitter::Get2ndMesh(
	const double& dLatDeg,	//!< in �ܓx(�x)
	const double& dLonDeg,	//!< in �o�x(�x)
	CString& strCode		//!< out ���b�V���R�[�h
)
{
	double dLBLat, dLBLon;
	return Get2ndMeshLB(dLatDeg, dLonDeg, strCode, dLBLat, dLBLon);
}

/*! 3�����b�V������
@note �ܓx�E�o�x�ɑ�������3�����b�V��
*/
bool MeshSplitter::Get3rdMeshLB(
	const double& dLatDeg,	//!< in �ܓx(�x)
	const double& dLonDeg,	//!< in �o�x(�x)
	CString& strCode,		//!< out ���b�V���R�[�h
	double& dLBLatDeg,		//!< out �����ܓx(�x)
	double& dLBLonDeg		//!< out �����o�x(�x)
)
{
	CString	strCode2;
	double dLBLatDeg2, dLBLonDeg2;
	if (!Get2ndMeshLB(dLatDeg, dLonDeg, strCode2, dLBLatDeg2, dLBLonDeg2))
	{
		return false;
	}
	int Lat3 = (int)((dLatDeg - dLBLatDeg2) / (30.0 / 3600.0));
	int Lon3 = (int)((dLonDeg - dLBLonDeg2) / (45.0 / 3600.0));
	if ((0 <= Lat3 && Lat3 <= 9) &&
		(0 <= Lon3 && Lon3 <= 9)
		)
	{
		strCode.Format(_T("%s%d%d"), strCode2, Lat3, Lon3);
	}

	dLBLatDeg = dLBLatDeg2 + Lat3 * (30.0 / 3600.0);
	dLBLonDeg = dLBLonDeg2 + Lon3 * (45.0 / 3600.0);

	if (strCode.GetLength() != 8)
	{
		strCode.Empty();
		return false;
	}
	return true;
}

/*! 3�����b�V������
@note �ܓx�E�o�x�ɑ�������3�����b�V��
*/
bool MeshSplitter::Get3rdMesh(
	const double& dLatDeg,	//!< in �ܓx(�x)
	const double& dLonDeg,	//!< in �o�x(�x)
	CString& strCode		//!< out ���b�V���R�[�h
)
{
	double dLBLat, dLBLon;
	return Get3rdMeshLB(dLatDeg, dLonDeg, strCode, dLBLat, dLBLon);
}

/*! �ċA���b�V������
@note �ܓx�E�o�x�ɑ��������ċA���b�V��
*/
bool MeshSplitter::GetRecursiveMeshLB(
	const double& dLatDeg,	//!< in �ܓx(�x)
	const double& dLonDeg,	//!< in �o�x(�x)
	CString& strCode,		//!< out ���b�V���R�[�h
	double& dLBLatDeg,		//!< out �����ܓx(�x)
	double& dLBLonDeg,		//!< out �����o�x(�x)
	int meshLevel
)
{
	CString	strCodeParent;
	double dLBLatDegParent, dLBLonDegParent;
	switch (meshLevel)
	{
	case 1:
	case 2:
	case 3:
		return false;
		break;
	case 4:
		if (!Get3rdMeshLB(dLatDeg, dLonDeg, strCodeParent, dLBLatDegParent, dLBLonDegParent))
		{
			return false;
		}
		break;

	default:
		if (!GetRecursiveMeshLB(dLatDeg, dLonDeg, strCodeParent, dLBLatDegParent, dLBLonDegParent, meshLevel - 1))
		{
			return false;
		}
		break;
	}
	double LatDiv = 30.0 / (std::pow(2, meshLevel - 3)) / 3600.0;
	double LonDiv = 45.0 / (std::pow(2, meshLevel - 3)) / 3600.0;
	int Lat = (int)((dLatDeg - dLBLatDegParent) / LatDiv);
	int Lon = (int)((dLonDeg - dLBLonDegParent) / LonDiv);
	int code = (Lon + 1) + Lat * 2;

	strCode.Format(_T("%s%d"), strCodeParent, code);

	dLBLatDeg = dLBLatDegParent + Lat * LatDiv;
	dLBLonDeg = dLBLonDegParent + Lon * LonDiv;

	if (strCode.GetLength() != meshLevel + 5)
	{
		strCode.Empty();
		return false;
	}
	return true;
}

/*! �ċA���b�V������
@note �ܓx�E�o�x�ɑ��������ċA���b�V��
*/
bool MeshSplitter::GetRecursiveMesh(
	const double& dLatDeg,	//!< in �ܓx(�x)
	const double& dLonDeg,	//!< in �o�x(�x)
	CString& strCode,		//!< out ���b�V���R�[�h
	int meshLevel			//!< in ���b�V�����x��
)
{
	double dLBLat, dLBLon;
	return GetRecursiveMeshLB(dLatDeg, dLonDeg, strCode, dLBLat, dLBLon, meshLevel);
}

/*! ���b�V���R�[�h�G���A�擾
*/
bool MeshSplitter::GetMeshCodeArea(
	const CString& strCode,				//!< in		���b�V���R�[�h
	Datum			datum,					//!< in		���n�n
	int				meshLevel,				//!< in		���b�V�����x��
	std::pair<double, double>& vLB,			//!< out	����
	std::pair<double, double>& vRB,			//!< out	�E��
	std::pair<double, double>& vRT,			//!< out	�E��
	std::pair<double, double>& vLT			//!< out	����
)
{
	double	dLatDegMin, dLonDegMin;
	double	dLatDegMax, dLonDegMax;
	if (strCode.GetLength() == 6)
	{
		// 2�����b�V��
		dLatDegMin = (_tstoi(strCode.Left(2)) / 1.5 * 3600
			+ _tstoi(strCode.Mid(4, 1)) * 5.0 * 60) / 3600.0;

		dLonDegMin = ((_tstoi(strCode.Mid(2, 2)) + 100.0) * 3600
			+ _tstoi(strCode.Mid(5, 1)) * 7.5 * 60) / 3600.0;

		dLatDegMax = dLatDegMin + (5.0 * 60.0 / 3600.0);
		dLonDegMax = dLonDegMin + (7.5 * 60.0 / 3600.0);
	}
	else if (strCode.GetLength() == 8)
	{
		// 3�����b�V��
		dLatDegMin = (_tstoi(strCode.Left(2)) / 1.5 * 3600
			+ _tstoi(strCode.Mid(4, 1)) * 5.0 * 60
			+ _tstoi(strCode.Mid(6, 1)) * 0.5 * 60
			) / 3600.0;

		dLonDegMin = ((_tstoi(strCode.Mid(2, 2)) + 100.0) * 3600
			+ _tstoi(strCode.Mid(5, 1)) * 7.5 * 60
			+ _tstoi(strCode.Mid(7, 1)) * 0.75 * 60
			) / 3600.0;

		dLatDegMax = dLatDegMin + (0.5 * 60.0 / 3600.0);
		dLonDegMax = dLonDegMin + (0.75 * 60.0 / 3600.0);
	}
	else if (strCode.GetLength() >= 9)
	{
		// 4�����b�V���ȍ~
		std::vector<int> latVec;
		std::vector<int> lonVec;
		for (int i = 8; i < strCode.GetLength(); i++)
		{
			int lat = 0, lon = 0;
			if ((_tstoi(strCode.Mid(i, 1)) - 1) / 2 == 1)
			{
				lat = 1;
			}
			if (_tstoi(strCode.Mid(i, 1)) % 2 == 0)
			{
				lon = 1;
			}
			latVec.push_back(lat);
			lonVec.push_back(lon);
		}

		double appendLat = 0.0;
		for (int i = 0; i < latVec.size(); i++)
		{
			appendLat += latVec[i] * (0.5 / std::pow(2, i + 1)) * 60;
		}
		dLatDegMin = (_tstoi(strCode.Left(2)) / 1.5 * 3600
			+ _tstoi(strCode.Mid(4, 1)) * 5.0 * 60
			+ _tstoi(strCode.Mid(6, 1)) * 0.5 * 60
			+ appendLat
			) / 3600.0;

		double appendLon = 0.0;
		for (int i = 0; i < lonVec.size(); i++)
		{
			appendLon += lonVec[i] * (0.75 / std::pow(2, i + 1)) * 60;
		}
		dLonDegMin = ((_tstoi(strCode.Mid(2, 2)) + 100.0) * 3600
			+ _tstoi(strCode.Mid(5, 1)) * 7.5 * 60
			+ _tstoi(strCode.Mid(7, 1)) * 0.75 * 60
			+ appendLon
			) / 3600.0;

		dLatDegMax = dLatDegMin + (0.5 / std::pow(2, strCode.GetLength() - 8) * 60.0 / 3600.0);
		dLonDegMax = dLonDegMin + (0.75 / std::pow(2, strCode.GetLength() - 8) * 60.0 / 3600.0);
	}
	else
	{
		assert(false);
	}

	double	x, y;
	// ����
	CGeoUtil::LonLatToXY(dLonDegMin, dLatDegMin, datum, x, y);
	vLB = std::make_pair(x, y);
	// �E��
	CGeoUtil::LonLatToXY(dLonDegMax, dLatDegMin, datum, x, y);
	vRB = std::make_pair(x, y);
	// �E��
	CGeoUtil::LonLatToXY(dLonDegMax, dLatDegMax, datum, x, y);
	vRT = std::make_pair(x, y);
	// ����
	CGeoUtil::LonLatToXY(dLonDegMin, dLatDegMax, datum, x, y);
	vLT = std::make_pair(x, y);

	return	true;
}


/*!	���b�V���R�[�h�擾
*/
/*static*/
bool	MeshSplitter::GetMeshCode(
	const double& dLat,				//!< in		�ܓx
	const double& dLon,				//!< in		�o�x
	int meshLevel,					//!< in		���b�V�����x�� 3:3�����b�V���A4:4�����b�V��	
	CString& strCode				//!< out	���b�V���R�[�h
)
{
	if (meshLevel == 3)
	{
		return Get3rdMesh(dLat, dLon, strCode);
	}
	else if (meshLevel >= 4)
	{
		return GetRecursiveMesh(dLat, dLon, strCode, meshLevel);
	}

	return	false;
}


std::vector<PolygonData> MeshSplitter::FindMaxAreaPolygonMesh(
	std::vector<SHPObject*> shapeVec,
	std::set<CString>& meshCodes,
	std::set<CString>& targetMeshCodes,
	int JPZone,
	int meshLevel
)
{
	typedef boost::geometry::model::d2::point_xy<double> PointType;
	typedef boost::geometry::model::polygon<PointType> PolygonType;
	typedef boost::geometry::model::polygon<PointType, false> ReversePolygonType;

	std::vector<PolygonData> polygonData;
	int index = -1;
	int id = 0;

	for (SHPObject* psShape : shapeVec)
	{
		// ���s��
		index++;

		// ���W�̊i�[���������t���O
		bool clockwiseFlag = true;

		// �p�[�g
		int partCount = psShape->nParts;

		// ���t���|���S���������ꍇ�ǂݔ�΂�
		if (partCount >= 2)
		{
			std::cout << "shape ���t���|���S�������o���܂��� �s��:" << index << std::endl;
			continue;
		}

		for (int nPart = 0; nPart < partCount; nPart++)
		{
			int start = psShape->panPartStart[nPart];
			int end = psShape->nVertices;
			if (nPart < partCount - 1)
			{
				end = psShape->panPartStart[nPart + 1]; //���p�[�g�̊J�n�C���f�b�N�X
			}
			PolygonType polygon;
			ReversePolygonType reversePolygon;

			for (int nPt = start; nPt < end; nPt++)
			{
				double x = psShape->padfX[nPt];
				double y = psShape->padfY[nPt];
				if (clockwiseFlag)
				{
					boost::geometry::append(polygon.outer(), PointType(x, y));
				}
				else
				{
					boost::geometry::append(reversePolygon.outer(), PointType(x, y));
				}

				// ���b�V���R�[�h�擾
				double dLat;
				double dLon;
				CGeoUtil::XYToLatLon(JPZone, y, x, dLat, dLon);
				CString strCode = "";
				GetMeshCode(dLat, dLon, meshLevel, strCode);
				LPCWSTR wstr = strCode.GetString();
				std::wstring compareWstr(wstr);
				// �Ō�̈ꕶ�������폜
				if (compareWstr.empty() == false)
				{
					compareWstr.pop_back();
				}

				if (meshLevel >= 4)
				{
					if (targetMeshCodes.find(compareWstr.c_str()) == targetMeshCodes.end())
					{
						continue;
					}
				}

				if (meshCodes.find(wstr) == meshCodes.end())
				{
					meshCodes.insert(wstr);
					std::wcout << "meshCode : " << wstr << std::endl;
				}
			}

			// �ʐς��}�C�i�X�̏ꍇ�͊i�[�����t�ɕύX���čČv�Z
			if (boost::geometry::area(polygon) < 0 && clockwiseFlag == true)
			{
				clockwiseFlag = false;
				nPart--;
				continue;
			}
			if (boost::geometry::area(reversePolygon) < 0 && clockwiseFlag == false)
			{
				clockwiseFlag = true;
				nPart--;
				continue;
			}

			std::map<LPCWSTR, double> resultArea;
			// �����������b�V���R�[�h��
			for (LPCWSTR value : meshCodes)
			{
				// ���b�V���R�[�h������W���擾
				std::pair<double, double> vLB;			//!< out	����
				std::pair<double, double> vRB;			//!< out	�E��
				std::pair<double, double> vRT;			//!< out	�E��
				std::pair<double, double> vLT;			//!< out	����
				GetMeshCodeArea(value, Datum(JPZone), meshLevel, vLB, vRB, vRT, vLT);

				// ���b�V���̐}�`���쐬
				PolygonType mesh;
				mesh.outer().push_back(PointType(vLB.first, vLB.second));
				mesh.outer().push_back(PointType(vRB.first, vRB.second));
				mesh.outer().push_back(PointType(vRT.first, vRT.second));
				mesh.outer().push_back(PointType(vLT.first, vLT.second));

				// �|���S���ƃ��b�V���̏d���ʐς��Z�o
				std::deque<PolygonType> intersection;
				if (clockwiseFlag)
				{
					if (boost::geometry::intersects(polygon, mesh))
					{
						boost::geometry::intersection(polygon, mesh, intersection);
						for (auto p : intersection)
						{
							double area = boost::geometry::area(p);
							resultArea[value] = area;
						}
					}
				}
				else
				{
					if (boost::geometry::intersects(reversePolygon, mesh))
					{
						boost::geometry::intersection(reversePolygon, mesh, intersection);
						for (auto p : intersection)
						{
							double area = boost::geometry::area(p);
							resultArea[value] = area;
						}
					}
				}
			}

			double maxArea = 0.0;
			CString maxMeshCode;
			for (LPCWSTR value : meshCodes)
			{
				double area = resultArea[value];
				// �����̃��b�V���ɂ܂�����|���S���̏ꍇ�͏d���ʐς̑傫�����̃��b�V���R�[�h���̗p����
				if (area > maxArea)
				{
					maxArea = area;
					maxMeshCode = value;
				}
			}

			// ���ʂ̊i�[
			PolygonData polygons(id++, maxMeshCode);
			if (clockwiseFlag)
			{
				for (auto value : polygon.outer())
				{
					polygons.vertices.push_back({ value.x(), value.y() });
				}
			}
			else
			{
				for (auto value : reversePolygon.outer())
				{
					polygons.vertices.push_back({ value.x(), value.y() });
				}
			}
			polygonData.push_back(polygons);
		}
	}

	return polygonData;
}