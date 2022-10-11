#include "linearfit.h"
#include "common.h"

using namespace std;
#pragma warning(disable :4996)

CLinearFit::CLinearFit()
{
}

CLinearFit::~CLinearFit()
{
}

double CLinearFit::LineFit()
{
	int len;
	len = mPointList.size();

	if(len < 3) 
	{
		cerr << "错误：线性拟合输入数据个数小于3！" << ENDLINE;
		return 0;
	}

	double av_x, av_y;
	double L_xx, L_yy, L_xy;
	int i;
	//变量初始化
	av_x=0; //x的平均值
	av_y=0; //y的平均值
	L_xx=0; //Lxx
	L_yy=0; //Lyy
	L_xy=0; //Lxy


	for(i=0; i<len; i++) //计算x、y的平均值
	{
	   av_x+=mPointList[i].x/len;
	   av_y+=mPointList[i].y/len;
	}

	for(i=0; i<len; i++) //计算Lxx、Lyy和Lxy
	{
	   L_xx+=(mPointList[i].x-av_x)*(mPointList[i].x-av_x);
	   L_yy+=(mPointList[i].y-av_y)*(mPointList[i].y-av_y);
	   L_xy+=(mPointList[i].x-av_x)*(mPointList[i].y-av_y);
	}

	//to avoid dividing by zero
	double eps = 0.0000001;
	if(L_xx < eps) L_xx = eps;
	if(L_yy < eps) L_yy = eps;

	b = L_xy/L_xx;
	a = av_y-L_xy*av_x/L_xx;
	r = L_xy/sqrt(L_xx*L_yy);

	return r;
}
