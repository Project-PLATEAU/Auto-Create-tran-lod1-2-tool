#pragma once
#include <string>
#include <iostream>
#include "shapefil.h"
#include <vector>

class CShapeManager
{
public:
	SHPHandle hSHP;     //!< shp fileのハンドル
	DBFHandle hDBF;     //!< dbf fileのハンドル
	SHPObject* psShape; //!< ポリゴン
	std::vector<SHPObject*> psShapeVec; //!< ポリゴン配列

	int nShapeType;         //!< 種別
	int nEntities;          //!< 要素数
	int nField;             //!< field数
	double adfMinBound[4] = { 0, 0, 0, 0 }; //!< バウンディング(最小値)
	double adfMaxBound[4] = { 0, 0, 0, 0 }; //!< バウンディング(最大値)

	CShapeManager(void);                        //!< コンストラクタ
	~CShapeManager(void);                       //!< デストラクタ
	bool Open(std::string strShpPath);          //!< open shape file
	void Close(void);                           //!< close shape file

	void Print()
	{
		std::cout << "種別：" << nShapeType << std::endl
			<< "要素数：" << nEntities << std::endl
			<< "field数：" << nField << std::endl;
	}

protected:

private:
};

