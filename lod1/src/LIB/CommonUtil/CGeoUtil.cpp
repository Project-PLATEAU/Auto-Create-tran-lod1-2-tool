#include "pch.h"
#include "CGeoUtil.h"
#include <cmath>
#include <random>

#pragma region �􉽌v�Z

//�x�N�g������
double CGeoUtil::InnerProduct(const CVector3D& vl, const CVector3D& vr)
{
	return vl.x * vr.x + vl.y * vr.y + vl.z * vr.z;
}

//�x�N�g���O��
void CGeoUtil::OuterProduct(const CVector3D& v1, const CVector3D& v2, CVector3D& vOut)
{
	vOut.x = v1.y * v2.z - v1.z * v2.y;
	vOut.y = v1.z * v2.x - v1.x * v2.z;
	vOut.z = v1.x * v2.y - v1.y * v2.x;
}

// �x�N�g��2D����
double CGeoUtil::InnerProduct(const CVector2D& vl, const CVector2D& vr)
{
	return vl.x * vr.x + vl.y * vr.y;
}

// ���ʂ̎��擾
void CGeoUtil::GetPlaneParameter
(
	const CPointBase& pos1,	//!< in ���ʏ�̔C�ӂ̓_1
	const CPointBase& pos2,	//!< in ���ʏ�̔C�ӂ̓_2
	const CPointBase& pos3,	//!< in ���ʏ�̔C�ӂ̓_3
	CPlane& plane			//!< out	����
)
{
	CVector3D v1(pos2.x - pos1.x, pos2.y - pos1.y, pos2.z - pos1.z);
	CVector3D v2(pos3.x - pos1.x, pos3.y - pos1.y, pos3.z - pos1.z);

	CVector3D vNormal;
	OuterProduct(v1, v2, vNormal);
	if (vNormal.z < 0)
	{
		// ������ɔ��]
		vNormal.Inverse();
	}
	vNormal.Normalize();

	plane.a = vNormal.x;
	plane.b = vNormal.y;
	plane.c = vNormal.z;
	plane.d = -vNormal.x * pos1.x - vNormal.y * pos1.y - vNormal.z * pos1.z;
}

// �X�Ίp
// ������x�N�g���Ɩʖ@���̐����p�x
void CGeoUtil::CalcSlope(CVector3D& vP, double& slopeDegree)
{
	// �����
	CVector3D vUp(0, 0, 1);

	double dL1 = vP.Length();
	if (abs(dL1) < _MAX_TOL) return;

	// cos��
	double costh = InnerProduct(vP, vUp) / dL1;
	if (costh > 1.0) costh = 1.0;
	if (-1.0 > costh)costh = -1.0;
	double dAngle = acos(costh);
	slopeDegree = dAngle * _COEF_RAD_TO_DEG;
}

// ���ʊp
// 2�����̖@���x�N�g���Ɛ^�k�̂Ȃ��p�x
void CGeoUtil::CalcAzimuth(CVector3D& vP, double& azDegree, int iJPZone)
{
	CVector3D vP2d(vP.x, vP.y, 0);

	double dL1 = vP2d.Length();
	if (abs(dL1) < _MAX_TOL) return;

	CVector3D vNup(0, 1, 0);
	// cos��
	double costh = InnerProduct(vP2d, vNup) / dL1;
	if (costh > 1.0) costh = 1.0;
	if (-1.0 > costh)costh = -1.0;
	double dAngle = acos(costh);	// �����p

	// �����̏ꍇ
	if (vP.x < 0.0)	dAngle = _PI * 2.0 - dAngle;

	// ���ʒ��p�̖k�Ɛ^�k�̕␳
	// �z�u���W���ܓx�o�x�Ő^�k�����Ɉړ���̍��W���Z�o
	double dN = calcNorth(iJPZone, vP2d.y, vP2d.x);

	azDegree = (dAngle + dN) * _COEF_RAD_TO_DEG;
}

//�_�ƕ��ʂ̋��������߂� ( P=���ʏ�̓_ N=���ʂ̖@�� )
double CGeoUtil::CalcDistancePointToFace(const CVector3D& A, const CVector3D& P, const CVector3D& N)
{
	//PA�x�N�g��(A-P)
	CVector3D PA(0, 0, 0);
	PA.x = A.x - P.x;
	PA.y = A.y - P.y;
	PA.z = A.z - P.z;

	//�@��N��PA�����,���̐�Βl���_�ƕ��ʂ̋���
	return abs(InnerProduct(N, PA));
}

// �_�ƕ��ʂƂ̋����̎Z�o
// ���ʂ�ax + by + cz + d = 0�ŗ^������̂Ƃ���
// �_�ƕ��ʂƂ̋���
double CGeoUtil::CalcDistancePointToFace
(
	const double& x, //!< in �_���WX
	const double& y, //!< in �_���WY
	const double& z, //!< in �_���WZ
	const double& a, //!< in ���ʂ̃p�����[�^a
	const double& b, //!< in ���ʂ̃p�����[�^b
	const double& c, //!< in ���ʂ̃p�����[�^c
	const double& d	 //!< in ���ʂ̃p�����[�^d
)
{
	// �_�ƕ��ʂ̋�����abs(a * x + b *y + c * z + d) / sqrt(a * a + b * b + c * c)
	return abs(a * x + b * y + c * z + d) / sqrt(a * a + b * b + c * c);
}

// ���O����
bool CGeoUtil::IsPointInPolygon(const CPoint2D& pointTarget, unsigned int uiCountPoint, const CPoint2D* pPoint)
{
	int iCountCrossing = 0;

	CPoint2D point0 = pPoint[0];
	bool bFlag0x = (pointTarget.x <= point0.x);
	bool bFlag0y = (pointTarget.y <= point0.y);

	// ���C�̕����́A�w�v���X����
	for (unsigned int ui = 1; ui < uiCountPoint + 1; ui++)
	{
		CPoint2D point1 = pPoint[ui % uiCountPoint];	// �Ō�͎n�_������i���p�`�f�[�^�̎n�_�ƏI�_����v���Ă��Ȃ��f�[�^�Ή��j
		bool bFlag1x = (pointTarget.x <= point1.x);
		bool bFlag1y = (pointTarget.y <= point1.y);
		if (bFlag0y != bFlag1y)
		{	// �����̓��C�����؂�\������B
			if (bFlag0x == bFlag1x)
			{	// �����̂Q�[�_�͑Ώۓ_�ɑ΂��ė����E���������ɂ���
				if (bFlag0x)
				{	// ���S�ɉE�B�ː����̓��C�����؂�
					iCountCrossing += (bFlag0y ? -1 : 1);	// �ォ�牺�Ƀ��C�����؂�Ƃ��ɂ́A�����񐔂��P�����A�������͂P�����B
				}
			}
			else
			{	// ���C�ƌ������邩�ǂ����A�Ώۓ_�Ɠ��������ŁA�Ώۓ_�̉E�Ō������邩�A���Ō������邩�����߂�B
				if (pointTarget.x <= (point0.x + (point1.x - point0.x) * (pointTarget.y - point0.y) / (point1.y - point0.y)))
				{	// �����́A�Ώۓ_�Ɠ��������ŁA�Ώۓ_�̉E�Ō�������B�ː����̓��C�����؂�
					iCountCrossing += (bFlag0y ? -1 : 1);	// �ォ�牺�Ƀ��C�����؂�Ƃ��ɂ́A�����񐔂��P�����A�������͂P�����B
				}
			}
		}
		// ���̔���̂��߂ɁA
		point0 = point1;
		bFlag0x = bFlag1x;
		bFlag0y = bFlag1y;
	}

	// �N���X�J�E���g���[���̂Ƃ��O�A�[���ȊO�̂Ƃ����B
	return (0 != iCountCrossing);
}

// �P�ʃx�N�g��
CVector3D CGeoUtil::Normalize(const CVector3D& vec)
{
	CVector3D result = vec;
	result.Normalize();
	return result;
}

/*!
 * @brief 2�̃x�N�g���̂Ȃ��p
 * @param[in] v �x�N�g��
 * @return �x�N�g���̂Ȃ��p(deg)
 * @note 0�x�N�g���̏ꍇ�́A0��ԋp
*/
double CGeoUtil::Angle(const CVector3D &v1, const CVector3D &v2)
{
    double dL1, dL2;
    dL1 = v1.Length();
    if (CEpsUtil::Zero(dL1))
        return 0.0;
    dL2 = v2.Length();
    if (CEpsUtil::Zero(dL2))
        return 0.0;
    double costh = InnerProduct(v1, v2) / dL1 / dL2;
    if (CEpsUtil::Greater(costh, 1.0))
        costh = 1.0;
    if (CEpsUtil::Greater(-1.0, costh))
        costh = -1.0;
    double dAngle = acos(costh);
    return dAngle * _COEF_RAD_TO_DEG;
}

/*!
 * @brief 2�̃x�N�g���̂Ȃ��p
 * @param[in] v �x�N�g��
 * @return �x�N�g���̂Ȃ��p(deg)
 * @note 0�x�N�g���̏ꍇ�́A0��ԋp
*/
double CGeoUtil::Angle(const CVector2D &v1, const CVector2D &v2)
{
    CVector3D vTmp1(v1.x, v1.y, 0.0);
    CVector3D vTmp2(v2.x, v2.y, 0.0);
    return Angle(vTmp1, vTmp2);
}
#pragma endregion


#pragma region ���W�ϊ�


/*! �œK�Ȍn�ԍ������߂�
@note �{�@�\�ł͋����Ŕ��f���Ă��邽�ߎ��ۂƂ͈قȂ�n�ԍ��ƂȂ肤��B
*/
int CGeoUtil::GetNearJPZone(
	const double& dLon,		//!<in �o�x[�x]
	const double& dLat		//!<in �ܓx[�x]
)
{
	int jpzone = 0;
	double dMinDist = DBL_MAX;
	double dX, dY;
	for (int ic = 1; ic < 20; ic++)
	{
		LonLatToXY(dLon, dLat, ic, dX, dY);
		double dTmpDist = dX * dX + dY * dY;
		if (dTmpDist < dMinDist)
		{
			jpzone = ic;
			dMinDist = dTmpDist;
		}
	}
	return jpzone;
}

/*! �ܓx�o�x����C�ӌ��_�̕��ʒ��p���W�����߂�
*/
void CGeoUtil::LonLatToXY(
	const double& dLon,		//!<in �o�x[�x]
	const double& dLat,		//!<in �ܓx[�x]
	int JPZone,				//!<in �n�ԍ�
	double& dEast,				//!<out X���W(���������Am)
	double& dNorth				//!<out Y���W(��k�����Am)
)
{
	double dLat0 = JPN_ORG_LAT_SEC[JPZone] / 3600.0 * _COEF_DEG_TO_RAD;
	double dLon0 = JPN_ORG_LON_SEC[JPZone] / 3600.0 * _COEF_DEG_TO_RAD;

	double dn = 1.0 / (2 * dF - 1);

	//���W�A���P�ʂ�
	double dLonRad = dLon * _COEF_DEG_TO_RAD;
	double dLatRad = dLat * _COEF_DEG_TO_RAD;

	double dt = sinh(atanh(sin(dLatRad)) - (2 * sqrt(dn)) / (1 + dn) * atanh(2 * sqrt(dn) / (1 + dn) * sin(dLatRad)));
	double dtb = sqrt(1 + pow(dt, 2));
	double dLmc = cos(dLonRad - dLon0);
	double dLms = sin(dLonRad - dLon0);
	double dXi = atan(dt / dLmc);
	double dEt = atanh(dLms / dtb);

	//��1��0�`��5��4
	double dal[6];
	dal[0] = 0;
	dal[1] = 1.0 / 2.0 * dn - 2.0 / 3.0 * pow(dn, 2) + 5.0 / 16.0 * pow(dn, 3) + 41.0 / 180.0 * pow(dn, 4) - 127.0 / 288.0 * pow(dn, 5);
	dal[2] = 13.0 / 48.0 * pow(dn, 2) - 3.0 / 5.0 * pow(dn, 3) + 557.0 / 1440.0 * pow(dn, 4) + 281.0 / 630.0 * pow(dn, 5);
	dal[3] = 61.0 / 240.0 * pow(dn, 3) - 103.0 / 140.0 * pow(dn, 4) + 15061.0 / 26880.0 * pow(dn, 5);
	dal[4] = 49561.0 / 161280.0 * pow(dn, 4) - 179.0 / 168.0 * pow(dn, 5);
	dal[5] = 34729.0 / 80640.0 * pow(dn, 5);
	double dSg = 0; double dTu = 0;
	for (int j = 1; j <= 5; j++)
	{
		dSg = dSg + 2 * j * dal[j] * cos(2 * j * dXi) * cosh(2 * j * dEt);
		dTu = dTu + 2 * j * dal[j] * sin(2 * j * dXi) * sinh(2 * j * dEt);
	}
	dSg = 1 + dSg;

	//A0-A5
	double dA[6];
	dA[0] = 1 + pow(dn, 2) / 4 + pow(dn, 4) / 64;
	dA[1] = -3.0 / 2.0 * (dn - pow(dn, 3) / 8 - pow(dn, 5) / 64);
	dA[2] = 15.0 / 16.0 * (pow(dn, 2) - pow(dn, 4) / 4);
	dA[3] = -35.0 / 48.0 * (pow(dn, 3) - 5.0 / 16.0 * pow(dn, 5));
	dA[4] = 315.0 / 512.0 * pow(dn, 4);
	dA[5] = -693.0 / 1280.0 * pow(dn, 5);
	double dAb = dM0 * daa / (1 + dn) * dA[0];
	double dSb = 0;
	for (int j = 1; j <= 5; j++)
	{
		dSb = dSb + dA[j] * sin(2 * j * dLat0);
	}
	dSb = dM0 * daa / (1 + dn) * (dA[0] * dLat0 + dSb);

	double dNorthTmp = 0;
	double dEastTmp = 0;
	for (int j = 1; j <= 5; j++)
	{
		dNorthTmp += dal[j] * sin(2 * j * dXi) * cosh(2 * j * dEt);
		dEastTmp += dal[j] * cos(2 * j * dXi) * sinh(2 * j * dEt);
	}
	dNorth = dAb * (dXi + dNorthTmp) - dSb;
	dEast = dAb * (dEt + dEastTmp);
}

/*! ���ʒ��p���W����ܓx�o�x�����߂�
*/
void CGeoUtil::XYToLatLon(
	int JPZone,				//!< in �ܓx���_
	const double& dNorth, 	//!< in X�i�k, m�j
	const double& dEast, 	//!< in Y�i��, m�j
	double& dLat, 			//!< out �ܓx�i�x�j
	double& dLon			//!< out �o�x�i�x�j
)
{
	// ���ʒ��p���W�n���_�̈ܓx�y�ьo�x 
	double dLat0 = JPN_ORG_LAT_SEC[JPZone] / 3600.0 * _COEF_DEG_TO_RAD;
	double dLon0 = JPN_ORG_LON_SEC[JPZone] / 3600.0 * _COEF_DEG_TO_RAD;
	double M0 = 0.9999; // ���S�̏k�ڌW��

	double dn = 1.0 / (2 * dF - 1);

	//S��0�AA
	double dA[6];
	dA[0] = 1.0 + pow(dn, 2) / 4.0 + pow(dn, 4) / 64.0;
	dA[1] = -3.0 / 2.0 * (dn - pow(dn, 3) / 8.0 - pow(dn, 5) / 64.0);
	dA[2] = 15.0 / 16.0 * (pow(dn, 2) - pow(dn, 4) / 4);
	dA[3] = -35.0 / 48.0 * (pow(dn, 3) - 5.0 / 16.0 * pow(dn, 5));
	dA[4] = 315.0 / 512.0 * pow(dn, 4);
	dA[5] = -693.0 / 1280.0 * pow(dn, 5);

	//��
	double dBt[6];
	dBt[1] = 1 / 2.0 * dn - 2 / 3.0 * pow(dn, 2) + 37.0 / 96.0 * pow(dn, 3) - 1 / 360.0 * pow(dn, 4) - 81 / 512.0 * pow(dn, 5);
	dBt[2] = 1 / 48.0 * pow(dn, 2) + 1 / 15.0 * pow(dn, 3) - 437 / 1440.0 * pow(dn, 4) + 46.0 / 105.0 * pow(dn, 5);
	dBt[3] = 17 / 480.0 * pow(dn, 3) - 37 / 840.0 * pow(dn, 4) - 209 / 4480.0 * pow(dn, 5);
	dBt[4] = 4397 / 161280.0 * pow(dn, 4) - 11 / 504.0 * pow(dn, 5);
	dBt[5] = 4583 / 161280.0 * pow(dn, 5);

	//��
	double dDt[7];
	dDt[1] = 2 * dn - 2.0 / 3.0 * pow(dn, 2) - 2 * pow(dn, 3) + 116.0 / 45.0 * pow(dn, 4)
		+ 26.0 / 45.0 * pow(dn, 5) - 2854.0 / 675.0 * pow(dn, 6);
	dDt[2] = 7.0 / 3.0 * pow(dn, 2) - 8.0 / 5.0 * pow(dn, 3) - 227.0 / 45.0 * pow(dn, 4)
		+ 2704.0 / 315.0 * pow(dn, 5) + 2323.0 / 945.0 * pow(dn, 6);
	dDt[3] = 56.0 / 15.0 * pow(dn, 3) - 136.0 / 35.0 * pow(dn, 4)
		- 1262.0 / 105.0 * pow(dn, 5) + 73814.0 / 2835.0 * pow(dn, 6);
	dDt[4] = 4279.0 / 630.0 * pow(dn, 4) - 332.0 / 35.0 * pow(dn, 5) - 399572.0 / 14175.0 * pow(dn, 6);
	dDt[5] = 4174.0 / 315.0 * pow(dn, 5) - 144838.0 / 6237.0 * pow(dn, 6);
	dDt[6] = 601676.0 / 22275.0 * pow(dn, 6);

	double dAb = dM0 * daa / (1 + dn) * dA[0];
	double dSb0 = 0;
	for (int j = 1; j <= 5; j++)
	{
		dSb0 += dA[j] * sin(2 * j * dLat0);
	}
	double dSb = dM0 * daa / (1 + dn) * (dA[0] * dLat0 + dSb0);

	//��
	double dXi = (dNorth + dSb) / dAb;
	// ��
	double dEt = dEast / dAb;

	//�́f�E��'�E��'�E��'�E��
	double dXi2tmp = 0;
	double dEt2tmp = 0;
	double dSg2tmp = 0;
	double dTu2tmp = 0;
	for (int j = 1; j <= 5; j++)
	{
		dXi2tmp += dBt[j] * sin(2 * j * dXi) * cosh(2 * j * dEt);
		dEt2tmp += dBt[j] * cos(2 * j * dXi) * sinh(2 * j * dEt);
		dSg2tmp += dBt[j] * cos(2 * j * dXi) * cosh(2 * j * dEt);
		dTu2tmp += dBt[j] * sin(2 * j * dXi) * sinh(2 * j * dEt);
	}
	double dXi2 = dXi - dXi2tmp;
	double dEt2 = dEt - dEt2tmp;
	double dSg2 = 1 - dSg2tmp;
	double dCi = asin(sin(dXi2) / cosh(dEt2));
	double dLatRad = dCi;
	for (int j = 1; j <= 6; j++)
	{
		dLatRad += dDt[j] * sin(2 * j * dCi);
	}
	//���W�A���P�ʂ̈ܓx�o�x
	double dLonRad = dLon0 + atan(sinh(dEt2) / cos(dXi2));

	//�x�P�ʂ�
	dLon = dLonRad * _COEF_RAD_TO_DEG;
	dLat = dLatRad * _COEF_RAD_TO_DEG;

	return;
}

// �q�ߐ������p(=-�^�k�����p)�����߂�
double CGeoUtil::calcNorth(int JPZone, const double& dNorth, const double& dEast)
{
	//
	double dGm = 0.0;

	// ���ʒ��p���W�n���_�̈ܓx
	double dLat0 = JPN_ORG_LAT_SEC[JPZone] / 3600.0 * _COEF_DEG_TO_RAD;

	double dn = 1.0 / (2 * dF - 1);

	//S��0�AA
	double dA[6];
	dA[0] = 1.0 + pow(dn, 2) / 4.0 + pow(dn, 4) / 64.0;
	dA[1] = -3.0 / 2.0 * (dn - pow(dn, 3) / 8.0 - pow(dn, 5) / 64.0);
	dA[2] = 15.0 / 16.0 * (pow(dn, 2) - pow(dn, 4) / 4);
	dA[3] = -35.0 / 48.0 * (pow(dn, 3) - 5.0 / 16.0 * pow(dn, 5));
	dA[4] = 315.0 / 512.0 * pow(dn, 4);
	dA[5] = -693.0 / 1280.0 * pow(dn, 5);

	//��
	double dBt[6];
	dBt[1] = 1 / 2.0 * dn - 2 / 3.0 * pow(dn, 2) + 37.0 / 96.0 * pow(dn, 3) - 1 / 360.0 * pow(dn, 4) - 81 / 512.0 * pow(dn, 5);
	dBt[2] = 1 / 48.0 * pow(dn, 2) + 1 / 15.0 * pow(dn, 3) - 437 / 1440.0 * pow(dn, 4) + 46.0 / 105.0 * pow(dn, 5);
	dBt[3] = 17 / 480.0 * pow(dn, 3) - 37 / 840.0 * pow(dn, 4) - 209 / 4480.0 * pow(dn, 5);
	dBt[4] = 4397 / 161280.0 * pow(dn, 4) - 11 / 504.0 * pow(dn, 5);
	dBt[5] = 4583 / 161280.0 * pow(dn, 5);

	double dAb = dM0 * daa / (1 + dn) * dA[0];
	double dSb0 = 0;
	for (int j = 1; j <= 5; j++)
	{
		dSb0 += dA[j] * sin(2 * j * dLat0);
	}
	double dSb = dM0 * daa / (1 + dn) * (dA[0] * dLat0 + dSb0);

	//��
	double dXi = (dNorth + dSb) / dAb;
	// ��
	double dEt = dEast / dAb;

	//�́f�E��'�E��'�E��'�E��
	double dXi2tmp = 0;
	double dEt2tmp = 0;
	double dSg2tmp = 0;
	double dTu2tmp = 0;
	for (int j = 1; j <= 5; j++)
	{
		dXi2tmp += dBt[j] * sin(2 * j * dXi) * cosh(2 * j * dEt);
		dEt2tmp += dBt[j] * cos(2 * j * dXi) * sinh(2 * j * dEt);
		dSg2tmp += dBt[j] * cos(2 * j * dXi) * cosh(2 * j * dEt);
		dTu2tmp += dBt[j] * sin(2 * j * dXi) * sinh(2 * j * dEt);
	}
	double dXi2 = dXi - dXi2tmp;
	double dEt2 = dEt - dEt2tmp;
	double dSg2 = 1 - dSg2tmp;

	dGm = atan((dTu2tmp + dSg2 * tan(dXi2) * tanh(dEt2)) / (dSg2 - dTu2tmp * tan(dXi2) * tanh(dEt2)));

	return dGm;

}

// �R�����b�V���R�[�h����ܓx�o�x�����߂�
bool CGeoUtil::MeshIDToLatLon(const std::string& meshid, double& lat, double& lon)
{
	if (meshid.length() != 8)
		return false;

	// 1�����b�V���p�v�Z
	double first_two = std::stod(meshid.substr(0, 2));
	double last_two = std::stod(meshid.substr(2, 2));
	lat = first_two * 2 / 3;
	lon = last_two + 100;

	// 2�����b�V���p�v�Z
	double fifth = std::stod(meshid.substr(4, 1));
	double sixth = std::stod(meshid.substr(5, 1));
	lat += fifth * 2 / 3 / 8;
	lon += sixth / 8;

	// 3�����b�V���p�v�Z
	double seventh = std::stod(meshid.substr(6, 1));
	double eighth = std::stod(meshid.substr(7, 1));
	lat += seventh * 2 / 3 / 8 / 10;
	lon += eighth / 8 / 10;

	return true;
}
#pragma endregion
