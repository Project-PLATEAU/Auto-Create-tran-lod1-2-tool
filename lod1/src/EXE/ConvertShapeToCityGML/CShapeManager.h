#pragma once
#include <string>
#include <iostream>
#include "shapefil.h"
#include <vector>

class CShapeManager
{
public:
	SHPHandle hSHP;     //!< shp file�̃n���h��
	DBFHandle hDBF;     //!< dbf file�̃n���h��
	SHPObject* psShape; //!< �|���S��
	std::vector<SHPObject*> psShapeVec; //!< �|���S���z��

	int nShapeType;         //!< ���
	int nEntities;          //!< �v�f��
	int nField;             //!< field��
	double adfMinBound[4] = { 0, 0, 0, 0 }; //!< �o�E���f�B���O(�ŏ��l)
	double adfMaxBound[4] = { 0, 0, 0, 0 }; //!< �o�E���f�B���O(�ő�l)

	CShapeManager(void);                        //!< �R���X�g���N�^
	~CShapeManager(void);                       //!< �f�X�g���N�^
	bool Open(std::string strShpPath);          //!< open shape file
	void Close(void);                           //!< close shape file

	void Print()
	{
		std::cout << "��ʁF" << nShapeType << std::endl
			<< "�v�f���F" << nEntities << std::endl
			<< "field���F" << nField << std::endl;
	}

protected:

private:
};

