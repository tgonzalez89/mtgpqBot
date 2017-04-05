#ifndef IMAGE_H
#define IMAGE_H

#include <string>
#include <vector>
#include <opencv2/opencv.hpp>
#include <Windows.h>

using namespace std;
using namespace cv;

class Image
{
public:
    static Mat HWND2Mat(HWND WindowHWND);
    static vector<Point> SearchInsideOtherImage(const Mat &ImageToSearch, const Mat &Image, int MatchMethod, char Mode, double TolerancePerc, vector<double> &Values, string ImgID = "");
    static vector<Point> SearchInsideOtherImage(const Mat &ImageToSearch, const Mat &Image, int MatchMethod, char Mode, double TolerancePerc, string ImgID = "");
    static bool Find(const Mat &ImageToSearch, const Mat &WindowImage, double TolerancePerc, bool &Result, string ImgID = "");
    static bool Find(const Mat &ImageToSearch, const Mat &WindowImage, double TolerancePerc, string ImgID = "");
    static bool Find(const Mat &ImageToSearch, HWND WindowHWND, double TolerancePerc, bool &Result, string ImgID = "");
    static bool Find(const Mat &ImageToSearch, HWND WindowHWND, double TolerancePerc, string ImgID = "");
    static bool Click(const Mat &ImageToSearch, const Mat &WindowImage, HWND WindowHWND, int Delay, double TolerancePerc, string ImgID = "");
    static bool Click(const Mat &ImageToSearch, HWND WindowHWND, int Delay, double TolerancePerc, string ImgID = "");
    static bool test(int a, bool &b) {b = a?true:false; return true;}

private:
    static bool FindClick(const Mat &ImageToSearch, const Mat &WindowImage, HWND WindowHWND, bool Click, int Delay, double TolerancePerc, bool &Result, string ImgID = "");
};

#endif // IMAGE_H
