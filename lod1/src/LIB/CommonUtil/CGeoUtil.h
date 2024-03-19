#pragma once
#include <math.h>
#include <vector>
#include <cassert>
#include <string>
#include "CEpsUtil.h"

const double _PI = 3.141592653589793;
const double _COEF_RAD_TO_DEG = 180.0 / _PI;
const double _COEF_DEG_TO_RAD = _PI / 180.0;
const double _MAX_TOL = 0.00001;		// ���e�덷(���l)

const double daa = 6378137;			//�����a
const double dF = 298.257222101;	//�t�G����
const double dM0 = 0.9999;			//���ʒ��p���W�n��Y����ɂ�����k�ڌW��(UTM���W�n�̏ꍇ��0.9996)

// ���ʒ��p���W�n�̌��_(�P�ʁF�b)
static const double JPN_ORG_LAT_SEC[20] = {
		 0.0,
	118800.0, 118800.0, 129600.0, 118800.0,129600.0, 129600.0,129600.0,
	129600.0, 129600.0, 144000.0, 158400.0, 158400.0,158400.0, 93600.0,
	 93600.0,  93600.0,  93600.0,72000.0, 93600.0 };
static const double JPN_ORG_LON_SEC[20] = {
		 0.0,
	466200.0, 471600.0, 475800.0, 480600.0, 483600.0, 489600.0, 493800.0,
	498600.0, 503400.0, 507000.0, 504900.0, 512100.0, 519300.0, 511200.0,
	459000.0, 446400.0, 471600.0, 489600.0, 554400.0 };

// ���W
class CPointBase
{
public:
	double x, y, z;

	CPointBase()
	{
		x = 0; y = 0; z = 0;
	}

	CPointBase(const double& x0, const double& y0, const double& z0)
	{
		x = x0; y = y0; z = z0;
	}
};

// �x�N�g��(3d)
class CVector3D : public CPointBase
{
public:
	CVector3D(const double& x, const double& y, const double& z) : CPointBase(x, y, z) {}
	CVector3D() {}
	// p2����p1�ւ̃x�N�g��
	CVector3D(const CPointBase& p1, const CPointBase& p2)
		: CPointBase(p1.x - p2.x, p1.y - p2.y, p1.z - p2.z) {}

	CVector3D& operator = (const CVector3D& v) {
		if (&v != this) { x = v.x; y = v.y; z = v.z; }
		return *this;
	}
	CVector3D  operator + (const CVector3D& v) const { return CVector3D(x + v.x, y + v.y, z + v.z); }
	CVector3D  operator - (const CVector3D& v) const { return CVector3D(x - v.x, y - v.y, z - v.z); }
	CVector3D  operator * (double a) const { return CVector3D(x * a, y * a, z * a); }
	CVector3D  operator *=(double a) { x *= a; y *= a; z *= a; return *this; }
	CVector3D& operator /=(double a) { assert(!(abs(a) < DBL_EPSILON)); x /= a; y /= a; z /= a; return *this; }
	CVector3D& operator +=(const CVector3D& a) { x += a.x; y += a.y; z += a.z; return *this; }
	CVector3D& operator -=(const CVector3D& a) { x -= a.x; y -= a.y; z -= a.z; return *this; }

	// �x�N�g�����]
	void Inverse() { x = -1 * x; y = -1 * y; z = -1 * z; }

	// �x�N�g���傫��
	inline double Length() const { return sqrt(x * x + y * y + z * z); }

	// �P�ʃx�N�g��
	void Normalize() {
		double dL = Length();
		x /= dL; y /= dL; z /= dL;
	}
};

// �X�J���[�ƃx�N�g���̊|���Z
inline CVector3D operator * (double a, const CVector3D& b) { return CVector3D(a * b.x, a * b.y, a * b.z); }

// ���W(2d)
class CPoint2D
{
public:
	double x, y;

	CPoint2D()
	{
		x = 0; y = 0;
	}

	CPoint2D(const double& x0, const double& y0)
	{
		x = x0; y = y0;
	}

};

// �x�N�g��(2d)
class CVector2D : public CPoint2D
{
public:
	CVector2D(const double& x, const double& y) : CPoint2D(x, y) {}
	CVector2D() {}

	CVector2D& operator = (const CVector2D& v) {
		if (&v != this) { x = v.x; y = v.y; }
		return *this;
	}
	CVector2D  operator + (const CVector2D& v) const { return CVector2D(x + v.x, y + v.y); }
	CVector2D  operator - (const CVector2D& v) const { return CVector2D(x - v.x, y - v.y); }
	CVector2D  operator * (double a) const { return CVector2D(x * a, y * a); }
	CVector2D  operator *=(double a) { x *= a; y *= a; return *this; }

	// Pos2����Pos1�����ւ̃x�N�g���ŏ�����
	CVector2D(const CPoint2D& p1,	//!< in Pos1
		const CPoint2D& p2			//!< in Pos2
	) {
		x = p1.x - p2.x; y = p1.y - p2.y;
	}

	// �x�N�g�����]
	void Inverse() { x = -1 * x; y = -1 * y; }

	// �x�N�g���傫��
	double Length() const { return sqrt(x * x + y * y); }

	// �P�ʃx�N�g��
	void Normalize() {
		double dL = Length();
		x /= dL; y /= dL;
	}

	// �Q�_�Ԃ̋��������߂�
	double Distance(const CPoint2D& p) const { return(sqrt(pow(x - p.x, 2) + pow(y - p.y, 2))); }
	double Distance(const double& x, const double& y) const {
		return(Distance(CPoint2D(x, y)));
	}
	static double Distance(const CPoint2D& p1, const CPoint2D& p2) {
		CVector2D tmp(p1.x, p1.y);
		return(tmp.Distance(p2));
	}
	static double Distance(const double& x1, const double& y1,
		const double& x2, const double& y2) {
		CVector2D tmp(x1, y1);
		return(tmp.Distance(x2, y2));
	}

};

// �X�J���[�ƃx�N�g���̊|���Z
inline CVector2D operator * (double a, const CVector2D& b) { return CVector2D(a * b.x, a * b.y); }


// ����( ax+by+cz+d=0 )
class CPlane
{
public:
	double a, b, c, d;
};

// �O�p�`�N���X
class CTriangle
{
public:
	std::vector<CPointBase> posTriangle;

	CTriangle(CPointBase p1, CPointBase p2, CPointBase p3)
	{
		posTriangle.push_back(p1);
		posTriangle.push_back(p2);
		posTriangle.push_back(p3);

	}

	CTriangle()
	{
		posTriangle.clear();
	}
};


class CGeoUtil
{
public:

#pragma region �􉽌v�Z
	//�x�N�g������
	static double InnerProduct(const CVector3D& vl, const CVector3D& vr);

	//�x�N�g��2D����
	static double InnerProduct(const CVector2D& vl, const CVector2D& vr);

	//�x�N�g���O��
	static void OuterProduct(const CVector3D& v1, const CVector3D& v2, CVector3D& vOut);

	// ���ʂ̎��擾
	static void GetPlaneParameter(const CPointBase& pos1, const CPointBase& pos2, const CPointBase& pos3, CPlane& plane);

	// �X�Ίp
	// ������x�N�g���Ɩʖ@���̐����p�x
	static void CalcSlope(CVector3D& vP, double& slopeDegree);

	// ���ʊp
	// 2�����̖@���x�N�g���Ɛ^�k�̂Ȃ��p�x
	static void CalcAzimuth(CVector3D& vP, double& azDegree, int iJPZone);

	//�_�ƕ��ʂ̋��������߂� ( P=���ʏ�̓_ N=���ʂ̖@�� )
	static double CalcDistancePointToFace(const CVector3D& A, const CVector3D& P, const CVector3D& N);

	// �_�ƕ��ʂƂ̋����̎Z�o
	// ���ʂ�ax + by + cz + d = 0�ŗ^������̂Ƃ���
	// �_�ƕ��ʂƂ̋���
	static double CalcDistancePointToFace(const double& x, const double& y, const double& z, const double& a, const double& b, const double& c, const double& d);

	// ���O����
	static bool IsPointInPolygon(
		const CPoint2D& pointTarget,		// ���O������s���_���W
		unsigned int uiCountPoint,			// ���_��
		const CPoint2D* pPoint);			// ���p�`�̍��W

	// �P�ʃx�N�g��
	static CVector3D Normalize(const CVector3D& vec);

    // 2�̃x�N�g���̂Ȃ��p
    static double Angle(const CVector3D &v1, const CVector3D &v2);
    // 2�̃x�N�g���̂Ȃ��p
    static double Angle(const CVector2D &v1, const CVector2D &v2);

#pragma endregion

#pragma region ���W�ϊ�
	// �œK�Ȍn�ԍ������߂�
	static int GetNearJPZone(const double& dLon, const double& dLat);
	
	// �ܓx�o�x����C�ӌ��_�̕��ʒ��p���W�����߂�
	static void LonLatToXY(const double& dLon, const double& dLat, int JPZone, double& X, double& Y );

	// ���ʒ��p���W����ܓx�o�x�����߂�
	static void XYToLatLon(int JPZone, const double& dNorth, const double& dEast, double& dLat, double& dLon);

	// �q�ߐ������p(=-�^�k�����p)�����߂�
	static double calcNorth(int JPZone, const double& dNorth, const double& dEast);

	// �R�����b�V���R�[�h����ܓx�o�x�����߂�
	static bool MeshIDToLatLon(const std::string& meshID, double& lat, double& lon);
#pragma endregion	
};

