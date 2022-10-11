#ifndef _LINEARFIT_H_
#define _LINEARFIT_H_

#include <iostream>
#include <fstream>
#include <vector>
#include <math.h>

using namespace std;

struct CPoint
{
	double x;
	double y;
};

//直线形式y=a+bx, r为相关系数
class CLinearFit
{
public:
	vector<CPoint> mPointList;
	double a;
	double b;
	double r;

public:
	CLinearFit();
	~CLinearFit();

	double LineFit();
};

#endif  //_LINEARFIT_H_
