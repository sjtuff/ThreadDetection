#include "mainwindow.h"
#include <QApplication>
#include <opencv2/opencv.hpp>
#include <QDebug>
#include <QtCore/qmath.h>

using namespace std;
using namespace cv;

#include <iostream>
#include <fstream>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include<cmath>
#include <QVector3D>

const int THRESH = 200;

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MainWindow w;
    /*
     * 控制电机运动，使得能够拍到端面及圆柱面位置，然后进行图像处理，得到端面
     * 斜率，大致估计轴线斜率k并保存这两张照片（照片用于之后计算直径用，上下
     * 相机均需要），找到B点大致位置Xb,Yb并记录，通过端面斜率和B点位置，计算出
     * 电机需要向右移动的距离，使得能拍到第3到9个齿（第9个齿是35.69处测中径的齿）
     * 距离计算为distance = 36*cos(old)-Yb,old=arctan(k)
     */
    void DrawVectorToMat(Mat image, Vector<Point>& contour, Scalar value);
    void LineFitLeastSquares(float *data_x, float *data_y, int data_n, Vector<float> &vResult);
    //圆弧段寻找
    Mat arc_image1 = imread("arc.bmp",0);
    Mat arc_image (arc_image1,Rect(0,200,2448,1858));
    Mat blur_rac_image;
    GaussianBlur(arc_image, blur_rac_image, Size(3, 3), 0, 0, BORDER_DEFAULT);
    Mat arc_edgeCanny;
    Canny(blur_rac_image, arc_edgeCanny, 130, 130 * 2, 3);
    Vector<Point> arc_contourCanny;//存储边缘点
    void load_arc_edge(Mat &arc_edgeCanny, Vector<Point> &arc_contourCanny);
    load_arc_edge(arc_edgeCanny, arc_contourCanny);

    //将边缘点由上到下拍排序
    bool comp2(const Point &a,const Point &b);
    sort(arc_contourCanny.begin(),arc_contourCanny.end(),comp2);
    for(int n=0;n<arc_contourCanny.size();n++)
    {
        //qDebug()<<arc_contourCanny[n].x<<arc_contourCanny[n].y;
    }

    Mat arc_mat(blur_rac_image.size().height, blur_rac_image.size().width, blur_rac_image.depth(), Scalar(0));
    DrawVectorToMat(arc_mat, arc_contourCanny, 255);

    Mat resizeone=Mat::zeros(2400,2900,CV_8UC3);
    resize(arc_mat,resizeone,resizeone.size());
    imshow("123",resizeone);

    /*从第10个点开始取50个点，拟合一条直线，算出斜率角度，再算出CD段的角度
     * 范围然后从第110个点开始遍历，每次取100个点拟合直线，直到角度在CD段的角
     * 度范围内，求着两条直线的交点，并以它为圆心，截取半径为100的圆弧，落在
     * 其中的边缘点视为圆弧段的点，然后进行圆弧拟合，求目标点
     */

    double tan_arc1, arc_k, arc_b, arc_Y;
    float arc_x[50]={0}, arc_y[50]={0};
    for(int n=10;n<60;n++)
    {
        arc_x[n-10] = arc_contourCanny[n].y;
        arc_y[n-10] = 1858 - arc_contourCanny[n].x;
    }
    //判断是否垂直,用后一个点减前一个点，求和
    int sum_v = 0;
    for(int n=0;n<49;n++)
    {
        sum_v = sum_v + arc_x[n+1] - arc_x[n];
    }
    if(sum_v<1)
    {
        tan_arc1=90.0;
        arc_Y = arc_contourCanny[20].y;
    }
    else
    {
        Vector<float> arc_Result;
        LineFitLeastSquares(arc_x, arc_y,50, arc_Result);
        arc_k = arc_Result[0];
        arc_b = arc_Result[1];
        tan_arc1 = qAtan(arc_k)*180/3.1415;
        if(tan_arc1<0)
        {
            tan_arc1 = tan_arc1 + 180;
        }
    }
    double up_tan = tan_arc1 - 70;
    double down_tan = tan_arc1 -100;
    int second_line = 0;//第二段直线段的位置
    int arc_num = arc_contourCanny.size() - 60;
    for(int n=60;n<arc_num;n=n+50)
    {

        for(int j=0;j<50;j++)
        {
            arc_x[j] = arc_contourCanny[n+j].y;
            arc_y[j] = 1858 - arc_contourCanny[n+j].x;
        }


        Vector<float> arc_Result2;
        LineFitLeastSquares(arc_x, arc_y,50, arc_Result2);
        double arc_k2=arc_Result2[0];
        double tan_arc2 = qAtan(arc_k2)*180/3.1415;
        if(tan_arc2>down_tan&&tan_arc2<up_tan)
        {
            second_line = n + 10;
        }


    }
    for(int n=second_line;n<(second_line+50);n++)
    {
        arc_x[n-second_line] = arc_contourCanny[n].y;
        arc_y[n-second_line] = 1858 - arc_contourCanny[n].x;
    }
    Vector<float> arc_Result3;
    LineFitLeastSquares(arc_x, arc_y,50, arc_Result3);
    double arc_k3 = arc_Result3[0];
    double arc_b3 = arc_Result3[1];
    double tan_arc3 = qAtan(arc_k3)*180/3.1415;
    //qDebug()<<second_line<<arc_k3<<arc_b3<<tan_arc3;

    //求交点
    Point interpoint;
    void CalculateInterpoint(double k1, double k2, double b1, double b2, Point &interpoint);
    void CalculateInterpoint(double k2, double b2, double arc_Y, Point &interpoint);//第一段垂直
    if(tan_arc1==90)
    {
        CalculateInterpoint(arc_k3, arc_b3, arc_Y, interpoint);
    }
    else {
        CalculateInterpoint(arc_k, arc_k3, arc_b, arc_b3, interpoint);
    }

    //取圆弧段范围，半径先暂取50；
    Vector<Point> arc_points;
    double heart_X = interpoint.x;
    double heart_Y = interpoint.y;
    for(int n=0;n<arc_contourCanny.size();n++)
    {
        double edge_X = arc_contourCanny[n].y;
        double edge_Y = 1858 - arc_contourCanny[n].x;
        double distance_heart = (edge_X-heart_X)*(edge_X-heart_X)+
                (edge_Y-heart_Y)*(edge_Y-heart_Y);
        distance_heart = sqrt(distance_heart);
        if(distance_heart<160)
        {
            arc_points.push_back(Point(1858-edge_Y,edge_X));
        }
    }
    Mat arc_mat2(blur_rac_image.size().height, blur_rac_image.size().width, blur_rac_image.depth(), Scalar(0));
    DrawVectorToMat(arc_mat2, arc_points, 255);

    Mat resizeone2=Mat::zeros(2400,2900,CV_8UC3);
    resize(arc_mat2,resizeone2,resizeone2.size());
    imshow("234",resizeone2);
    /*
     * 开始对第3到第9个齿进行计算
     */
    Mat src = imread("123.bmp", 0);
//    Mat image(src,Rect(1,1123,2000,45));
    Mat image(src);



    /*
     * 图像预处理，包括滤波和边缘提取
     */

    //高斯滤波
    Mat pro_image;
    GaussianBlur(image, pro_image, Size(3, 3), 0, 0, BORDER_DEFAULT);
    //边缘提取
    Mat edgeCanny;
    Canny(pro_image, edgeCanny, 130, 130 * 2, 3);
    //
    /*
     *
     */


    //找第一个齿底位置
    int first_col=1;
    int FirstPoint(Mat &edgeCanny, int first_col);
    int first_row = FirstPoint(edgeCanny,first_col);
    while(first_row==0)
    {

        first_col=first_col+5;
        first_row=FirstPoint(edgeCanny,first_col);
    }

    //找所有的沿
    int line_row = first_row-40;
    Vector<int> transform_points;
    int count_trans=0;
    /*
     * 判断是否有毛刺，缺陷
     */
    int FindTransform(Mat &edgeCanny, int &row, int col, int &count_trans);
    int transform = FindTransform(edgeCanny, line_row,first_col,count_trans);
    while(transform != 0)
    {
        transform_points.push_back(transform);
        transform = FindTransform(edgeCanny, line_row,transform, count_trans);
    }
    int num_transfor = transform_points.size();

    //找所有的沿（↓新↓）（坐标系：左上原点，→x，↓y）
    //imshow("out", edgeCanny);
    Mat crop_pro_image=pro_image(Rect(0, 0, pro_image.cols-100, pro_image.rows));
    int cal_ROI_black_min_y(Mat &src);
    int cal_ROI_white_max_y(Mat &src);
    int ROI_black_min_y=cal_ROI_black_min_y(crop_pro_image);
    int ROI_white_max_y=cal_ROI_white_max_y(crop_pro_image);
    qDebug()<<"!";
    qDebug()<<ROI_black_min_y;
    qDebug()<<ROI_black_min_y;
    qDebug()<<"!";
    //Mat ROI_1=crop_pro_image(Rect(0, ROI_black_min_y, crop_pro_image.cols, ROI_white_max_y-ROI_black_min_y));
    Mat edgeCanny_ROI;
    //Canny(ROI_1, edgeCanny_ROI, 130, 130 * 2, 3);
    Canny(crop_pro_image, edgeCanny_ROI, 130, 130 * 2, 3);

    //作一条线贯穿边缘线（灰度150）
    Mat edgeCanny_ROI_intersection(edgeCanny_ROI);
    line(edgeCanny_ROI_intersection, Point(0, ROI_black_min_y + 40), Point(edgeCanny_ROI_intersection.cols, ROI_white_max_y - 40), Scalar(150, 150, 150), 1, 8);//40是大约值，后续需优化表达
    //line(edgeCanny_ROI_intersection, Point(0, 40), Point(edgeCanny_ROI_intersection.cols, edgeCanny_ROI_intersection.rows - 40), Scalar(150, 150, 150), 1, 8);

    //找出贯穿线与边缘轮廓的交点，并存入 intersectionPV 容器
    vector<Point> intersectionPV;
    Point intersectionPoint;
    for (int i = 1; i < edgeCanny_ROI_intersection.rows - 1; i = i + 3)
    {
        for (int j = 1; j < edgeCanny_ROI_intersection.cols - 1; j = j + 3)
        {
            int intersection_sum_1 = 0, intersection_sum_2 = 0;
            uchar*data1 = edgeCanny_ROI_intersection.ptr<uchar>(i - 1);
            if (data1[j - 1] == 150) intersection_sum_1 += 1;
            if (data1[j] == 150) intersection_sum_1 += 1;
            if (data1[j + 1] == 150) intersection_sum_1 += 1;
            if (data1[j - 1] == 255) intersection_sum_2 += 1;
            if (data1[j] == 255) intersection_sum_2 += 1;
            if (data1[j + 1] == 255) intersection_sum_2 += 1;
            uchar* data2 = edgeCanny_ROI_intersection.ptr<uchar>(i);
            if (data2[j - 1] == 150) intersection_sum_1 += 1;
            if (data2[j] == 150) intersection_sum_1 += 1;
            if (data2[j + 1] == 150) intersection_sum_1 += 1;
            if (data2[j - 1] == 255) intersection_sum_2 += 1;
            if (data2[j] == 255) intersection_sum_2 += 1;
            if (data2[j + 1] == 255) intersection_sum_2 += 1;
            uchar* data3 = edgeCanny_ROI_intersection.ptr<uchar>(i + 1);
            if (data3[j - 1] == 150) intersection_sum_1 += 1;
            if (data3[j] == 150) intersection_sum_1 += 1;
            if (data3[j + 1] == 150) intersection_sum_1 += 1;
            if (data3[j - 1] == 255) intersection_sum_2 += 1;
            if (data3[j] == 255) intersection_sum_2 += 1;
            if (data3[j + 1] == 255) intersection_sum_2 += 1;
            if ((intersection_sum_1 == 3 && intersection_sum_2 == 2) || (intersection_sum_1 == 3 && intersection_sum_2 == 1))
            {
                intersectionPoint.x = j;
                intersectionPoint.y = i;
                intersectionPV.push_back(intersectionPoint);
            }
        }
    }
    //排序
    //bool sortfunction(const Point &a,const Point &b);
    sort(intersectionPV.begin(), intersectionPV.end(), comp2);
//    for(int i=0; i<intersectionPV.size();i++)
//    {
//        qDebug()<<"("<<intersectionPV[i].x<<" , "<<intersectionPV[i].y<<")";
//    }

    //清除杂点
    for (int i = 0; i < intersectionPV.size() - 1; i++)
    {
        if((intersectionPV[i+1].x-intersectionPV[i].x)<100)
            intersectionPV[i].x=0;
    }
    //清理后的交点存放在 intersectionPV_new 容器中
    vector<Point> intersectionPV_new;
    for (int i = 0; i < intersectionPV.size(); i++)
    {
        if(intersectionPV[i].x!=0)
            intersectionPV_new.push_back(intersectionPV[i]);
    }
    qDebug()<<"!!!";
    for(int i=0; i<intersectionPV_new.size();i++)
    {
        qDebug()<<"("<<intersectionPV_new[i].x<<" , "<<intersectionPV_new[i].y<<")";
    }
    //作出以交点为圆心，半径为3的圆（方便查看运行结果，实际可省去）
    for (int i = 0; i < intersectionPV_new.size(); i++)
    {
        cout << intersectionPV_new[i] << " ";
        circle(edgeCanny_ROI_intersection, Point(intersectionPV_new[i].x, intersectionPV_new[i].y), 5, Scalar(150, 150, 150), 1, 8);
    }

    //imshow("out", edgeCanny_ROI_intersection);
    //（↑新↑）



    //分别存储齿高和齿底，最右边的放在前面
    //齿底范围取小，用于拟合螺纹轴线
    Vector<Vector<Point> > high_ones;
    Vector<Vector<Point> > low_ones;
    /*
     * 数目调整
     */
    void load_high_ones(Mat &edgeCanny, Vector<Vector<Point> > &high_ones,
                        Vector<int> &transform_points, int line_row);
    void load_low_ones(Mat &edgeCanny, Vector<Vector<Point> > &low_ones,
                       Vector<int> &transform_points, int line_row);
    load_high_ones(edgeCanny_ROI,high_ones,transform_points, line_row);//_ROI
    load_low_ones(edgeCanny_ROI, low_ones, transform_points, line_row);//_ROI

    /*
     * 增加取的点数
     */
    //每个齿底选2个点，共12个点拟合齿底轴线
    float pX[12]={0}, pY[12]={0};
    for(int i=0;i<12;i=i+2)
    {
        int num_low_ones=i/2;
        Vector<Point> selected_ones = low_ones[num_low_ones];
        int number_ones = selected_ones.size();
        int first_one = number_ones/3;
        int second_one = first_one*2;
        //坐标系变化成左下角原点，向右X向上Y，防止溢出
        pX[i] = selected_ones[first_one].y;
        pY[i] = 2448 - selected_ones[first_one].x;
        pX[i+1] = selected_ones[second_one].y;
        pY[i+1] = 2448 - selected_ones[second_one].x;
    }

    //用最小二乘法拟合轴线
    Vector<float> vResult;
    //void LineFitLeastSquares(float *data_x, float *data_y, int data_n, Vector<float> &vResult);
    LineFitLeastSquares(pX, pY,12, vResult);
    double line_k = (double)vResult[0];
    double line_b = (double)vResult[1];
    qDebug()<<line_k<<line_b;

    /*反推边缘最左边的点距B点的轴线距离(distance+Yb)/cos(new),new = arctan(k+f(line_k))
    *找到35.69mm处的垂线（垂直于轴线），从而找到对应的齿顶，再逆变换找到矫正前
    * 的对应点，结合之前保存的两张图，找到圆弧上的密封面直径测量点、圆柱面直径
    * 测量点，就能测出1.密封面直径；2.圆柱面直径；3.螺纹中径；
    */

    //分别寻找每个齿顶的最高处
    Vector<QVector3D> max_heights;
    void find_max(Vector<Vector<Point>> &high_ones, Vector<QVector3D> &max_heights, double k, double b);
    find_max(high_ones, max_heights, line_k, line_b);


    //将5个齿的最高点从高到低排序
    bool comp(const QVector3D &a,const QVector3D &b);
    sort(max_heights.begin(),max_heights.end(),comp);

    for(int n=0;n<max_heights.size();n++)
    {
        qDebug()<<max_heights[n].x()<<max_heights[n].y()<<max_heights[n].z();
    }



    //Vector<Point>化为Mat，方便检验
    //void DrawVectorToMat(Mat image, Vector<Point>& contour, Scalar value);
    Mat matContoursCanny(pro_image.size().height,
                         pro_image.size().width, pro_image.depth(), Scalar(0));
    Vector<Point> check = high_ones[5];
    DrawVectorToMat(matContoursCanny, check, 255);

//    Mat resizeone=Mat::zeros(1600,1800,CV_8UC3);
//    resize(matContoursCanny,resizeone,resizeone.size());
//    imshow("123",resizeone);

    //测量螺距（↓新↓）
    //作特征线（灰度150）
    Mat edgeCanny_ROI_pitch(edgeCanny_ROI);
    line(edgeCanny_ROI_pitch, Point(first_col, first_row - 40), Point(edgeCanny_ROI_pitch.cols, line_k*edgeCanny_ROI_pitch.cols + line_b - 40), Scalar(150, 150, 150), 1, 8);//40是大约值，后续需优化表达
    //line(edgeCanny_ROI_pitch, Point(0, 40), Point(edgeCanny_ROI_pitch.cols, edgeCanny_ROI_pitch.rows - 40), Scalar(150, 150, 150), 1, 8);

    //找出特征线与边缘轮廓的交点，并存入 pitchPV 容器
    vector<Point> pitchPV;
    Point pitchPoint;
    for (int i = 1; i < edgeCanny_ROI_pitch.rows - 1; i = i + 3)
    {
        for (int j = 1; j < edgeCanny_ROI_pitch.cols - 1; j = j + 3)
        {
            int pitch_sum_1 = 0, pitch_sum_2 = 0;
            uchar*data1 = edgeCanny_ROI_pitch.ptr<uchar>(i - 1);
            if (data1[j - 1] == 150) pitch_sum_1 += 1;
            if (data1[j] == 150) pitch_sum_1 += 1;
            if (data1[j + 1] == 150) pitch_sum_1 += 1;
            if (data1[j - 1] == 255) pitch_sum_2 += 1;
            if (data1[j] == 255) pitch_sum_2 += 1;
            if (data1[j + 1] == 255) pitch_sum_2 += 1;
            uchar* data2 = edgeCanny_ROI_pitch.ptr<uchar>(i);
            if (data2[j - 1] == 150) pitch_sum_1 += 1;
            if (data2[j] == 150) pitch_sum_1 += 1;
            if (data2[j + 1] == 150) pitch_sum_1 += 1;
            if (data2[j - 1] == 255) pitch_sum_2 += 1;
            if (data2[j] == 255) pitch_sum_2 += 1;
            if (data2[j + 1] == 255) pitch_sum_2 += 1;
            uchar* data3 = edgeCanny_ROI_pitch.ptr<uchar>(i + 1);
            if (data3[j - 1] == 150) pitch_sum_1 += 1;
            if (data3[j] == 150) pitch_sum_1 += 1;
            if (data3[j + 1] == 150) pitch_sum_1 += 1;
            if (data3[j - 1] == 255) pitch_sum_2 += 1;
            if (data3[j] == 255) pitch_sum_2 += 1;
            if (data3[j + 1] == 255) pitch_sum_2 += 1;
            if ((pitch_sum_1 == 3 && pitch_sum_2 == 2) || (pitch_sum_1 == 3 && pitch_sum_2 == 1))
            {
                pitchPoint.x = j;
                pitchPoint.y = i;
                pitchPV.push_back(pitchPoint);
            }
        }
    }
    //排序
    sort(pitchPV.begin(), pitchPV.end(), comp2);
//    for(int i=0; i<pitchPV.size();i++)
//    {
//        qDebug()<<"("<<pitchPV[i].x<<" , "<<pitchPV[i].y<<")";
//    }

    //清除杂点
    for (int i = 0; i < pitchPV.size() - 1; i++)
    {
        if((pitchPV[i+1].x-pitchPV[i].x)<100)
            pitchPV[i].x=0;
    }
    //清理后的交点存放在 pitchPV_new 容器中
    vector<Point> pitchPV_new;
    for (int i = 0; i < pitchPV.size(); i++)
    {
        if(pitchPV[i].x!=0)
            pitchPV_new.push_back(pitchPV[i]);
    }
    qDebug()<<"!!!";
    for(int i=0; i<pitchPV_new.size();i++)
    {
        qDebug()<<"("<<pitchPV_new[i].x<<" , "<<pitchPV_new[i].y<<")";
    }
    //作出以交点为圆心，半径为3的圆（方便查看运行结果，实际可省去）
    for (int i = 0; i < pitchPV_new.size(); i++)
    {
        cout <<pitchPV_new[i] << " ";
        circle(edgeCanny_ROI_pitch, Point(pitchPV_new[i].x, pitchPV_new[i].y), 5, Scalar(150, 150, 150), 1, 8);
    }
    vector<int> pitch_result;
    for (int i = 0; i < pitchPV_new.size() - 2; i=i+2)
    {
        int result;
        result=pitchPV_new[i+2].x - pitchPV_new[i].x;
        pitch_result.push_back(result);
        qDebug()<<result;
    }
    //（↑新↑）

    return a.exec();
}

//FUNCTIONS

int FirstPoint(Mat &edgeCanny, int first_col)
{

    for(int i=0;i<edgeCanny.rows;i++)
    {
        uchar v = edgeCanny.at<uchar>(i, first_col);

        if(v!=0)
        {
            for(int j=0;j<edgeCanny.rows;j++)
            {
                uchar u = edgeCanny.at<uchar>(j, first_col+120);
                if(u!=0)
                {
                    int delta = i - j;
                    if(delta > 60)
                    {
                        return i;
                    }
                }

            }
        }
    }
    return 0;
}

int FindTransform(Mat &edgeCanny, int &row, int col, int &count_trans)
{
    count_trans=count_trans+1;
    if(count_trans>1)
    {
        col = col + 60;
    }

    for(int i=col;i<edgeCanny.cols;i++)
    {
        uchar v1 = edgeCanny.at<uchar>(row-1, i);
        uchar v2 = edgeCanny.at<uchar>(row, i);
        uchar v3 = edgeCanny.at<uchar>(row+1, i);

        if(v1!=0||v2!=0||v3!=0)
        {
            count_trans=count_trans+1;
            return i;
        }
    }
    return 0;
}


void load_high_ones(Mat &edgeCanny, Vector<Vector<Point> > &high_ones,
                    Vector<int> &transform_points, int line_row)
{
    for(int i=13;i>=0;i=i-2)
    {
        int start_row = line_row-80;
        int end_row = line_row;
        int start_col =  transform_points[i-1];
        int end_col = transform_points[i];
        Vector<Point> high_points;
        for(int j=start_col;j<=end_col;j++)
        {
            for(int k=start_row;k<=end_row;k++)
            {
                uchar v=edgeCanny.at<uchar>(k,j);
                if(v!=0)
                {
                    high_points.push_back(Point(k,j));
                }
            }
        }
        high_ones.push_back(high_points);
    }

}

void load_low_ones(Mat &edgeCanny, Vector<Vector<Point> > &low_ones,
                   Vector<int> &transform_points, int line_row)
{
    for(int i=12;i>0;i=i-2)
    {
        int start_row = line_row;
        int end_row = line_row+80;
        int start_col =  transform_points[i-1]+40;
        int end_col = transform_points[i]-35;
        Vector<Point> low_points;
        for(int j=start_col;j<=end_col;j++)
        {
            for(int k=start_row;k<=end_row;k++)
            {
                uchar v=edgeCanny.at<uchar>(k,j);
                if(v!=0)
                {
                    low_points.push_back(Point(k,j));
                }
            }
        }
        low_ones.push_back(low_points);
    }
}

void DrawVectorToMat(Mat image, Vector<Point>& contour, Scalar value)
{
    //	image.empty();
    for (unsigned int k = 0; k < contour.size(); k++)
    {
        int r = contour[k].x;
        int c = contour[k].y;
        image.at<uchar>(r, c) = static_cast<uchar>(value.val[0]);
    }
}

void find_max(Vector<Vector<Point> > &high_ones, Vector<QVector3D> &max_heights, double k, double b)
{
    for(int i=2;i<7;i++)
    {
        int max=0;
        int max_x;
        int max_y;
        Vector<Point> high_one = high_ones[i];
        for(int j=0;j<high_one.size();j++)
        {
            double x = high_one[j].y;
            double y = 2448 - high_one[j].x;
            double line_distance = (k*x-y+b)/sqrt(1+k*k);
            line_distance = -line_distance;
            if(line_distance>max)
            {
                max = line_distance;
                max_x = 2448 - y;
                max_y = x;
            }
        }
        QVector3D max_one(max_x, max_y, max);
        max_heights.push_back(max_one);

    }

}

bool comp(const QVector3D &a, const QVector3D &b)
{
    return a.z()>b.z();
}

bool comp2(const Point &a,const Point &b)
{
    return (a.x<b.x);
}

void LineFitLeastSquares(float *data_x, float *data_y, int data_n, Vector<float> &vResult)
{
    float A = 0.0;
    float B = 0.0;
    float C = 0.0;
    float D = 0.0;
    for (int i=0; i<data_n; i++)
    {
        A += data_x[i] * data_x[i];
        B += data_x[i];
        C += data_x[i] * data_y[i];
        D += data_y[i];
    }
    // 计算斜率a和截距b
    float a, b, temp = 0;
    if( temp = (data_n*A - B*B) )// 判断分母不为0
    {
        a = (data_n*C - B*D) / temp;
        b = (A*D - B*C) / temp;
    }
    else
    {
        a = 1;
        b = 0;
    }
    vResult.push_back(a);
    vResult.push_back(b);
}

//计算指定图像的最上方的黑点位置，返回row坐标
int cal_ROI_black_min_y(Mat &src)
{
    int srcImage_black_min_y = 0;
    //cvtColor(src, src, CV_BGR2GRAY);
    threshold(src, src, THRESH, 255, 0);
    for (int i = src.rows - 1; i >= 0; i--) //GAI: -1
    {
        int total_white_pixel = 0;
        uchar* data = src.ptr<uchar>(i);
        for (int j = 0; j < src.cols; j++)
        {
            if (data[j] == 0)
            {
                srcImage_black_min_y = i;
                break;
            }
            else
                total_white_pixel += 1;
        }
        if (total_white_pixel == src.cols)
            break;
    }
    return srcImage_black_min_y;
}

//计算指定图像的最下方的白点位置，返回row坐标
int cal_ROI_white_max_y(Mat &src)
{
    int srcImage_white_max_y = 0;
    //cvtColor(src, src, CV_BGR2GRAY);
    threshold(src, src, THRESH, 255, 0);
    for (int i = 0; i < src.rows; i++)
    {
        int total_black_pixel = 0;
        uchar* data = src.ptr<uchar>(i);
        for (int j = 0; j <src.cols; j++)
        {
            if (data[j] == 255)
            {
                srcImage_white_max_y = i;
                break;
            }
            else
                total_black_pixel += 1;
        }
        if (total_black_pixel == src.cols)
            break;
    }
    return srcImage_white_max_y;
}

void CalculateInterpoint(double k1, double k2, double b1, double b2, Point &interpoint)
{
    double interX = (b2-b1)/(k1-k2);
    double interY = (k1*b2-k2*b1)/(k1-k2);
    interpoint = Point(interX,interY);

}

void CalculateInterpoint(double k2, double b2, double arc_Y, Point &interpoint)
{
   double interX = arc_Y;
   double interY = k2*interX+b2;
   interpoint = Point(interX,interY);

}

void load_arc_edge(Mat &arc_edgeCanny, Vector<Point> &arc_contourCanny)
{
    for(int i=1;i<1000;i++)
    {
        for(int j=0;j<2400;j++)
        {
            uchar v = arc_edgeCanny.at<uchar>(i, j) ;
            if(v!=0)
            {
                arc_contourCanny.push_back(Point(i,j));
            }
        }
    }
}
