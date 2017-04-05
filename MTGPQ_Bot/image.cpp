#include "image.hpp"
#include "mouse.hpp"
#include <iostream>

////////////////////////////////////////////////////////////////////////////////////////////////////
/// \brief Image::HWND2Mat Create an image, in OpenCV's Mat format, from a window HWND.
/// \param [in] WindowHWND Window handler used to extract the image.
/// \return A Mat containing the image.
////////////////////////////////////////////////////////////////////////////////////////////////////
Mat Image::HWND2Mat(HWND WindowHWND) {
    if (WindowHWND == NULL) {
        cout << "-E- Image::HWND2Mat - Couldn't find the window." << endl;
        Mat fail;
        return fail;
    }

    RECT WindowRect;
    GetWindowRect(WindowHWND, &WindowRect);
    int WindowWidth = WindowRect.right - WindowRect.left;
    int WindowHeight = WindowRect.bottom - WindowRect.top;
    HDC WindowHDC = GetDC(WindowHWND);
    HDC WindowCompatibleHDC = CreateCompatibleDC(WindowHDC);
    HBITMAP WindowBitmap = CreateCompatibleBitmap(WindowHDC, WindowWidth, WindowHeight);
    SelectObject(WindowCompatibleHDC, WindowBitmap);
    PrintWindow(WindowHWND, WindowCompatibleHDC, 2);

    BITMAPINFOHEADER bi;
    bi.biSize = sizeof(BITMAPINFOHEADER);
    bi.biWidth = WindowWidth;
    bi.biHeight = -WindowHeight; //Negative indicates it's a top-down bitmap with it's origin at the upper-left corner
    bi.biPlanes = 1;
    bi.biBitCount = 32;
    bi.biCompression = BI_RGB;
    bi.biSizeImage = 0;
    bi.biXPelsPerMeter = 0;
    bi.biYPelsPerMeter = 0;
    bi.biClrUsed = 0;
    bi.biClrImportant = 0;

    Mat WindowMat(WindowHeight, WindowWidth, CV_8UC4);
    GetDIBits(WindowCompatibleHDC, WindowBitmap, 0, WindowHeight, WindowMat.data, (BITMAPINFO *)&bi, DIB_RGB_COLORS);

    DeleteObject(WindowBitmap);
    DeleteDC(WindowCompatibleHDC);
    ReleaseDC(WindowHWND, WindowHDC);

    return WindowMat;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// \brief Image::SearchInsideOtherImage Search for an image inside another image. Return the coordinates where it's found.
/// \param [in] ImageToSearch Image that is going to be searched inside a bigger image.
/// \param [in] Image Image where ImageToSearch is going to be searched.
/// \param [in] MatchMethod Determines the algorithm used to search the image.
/// \param [in] Mode s = search a single instance of Image. m = search for multiple instances of Image.
/// \param [in] TolerancePerc Determines how good must the match be to be succesful. Value must be between 0 and 1, 0 = image must match exactly.
/// \param [out] Values of how good the match was. Between 0 and 1. 0 = perfect match.
/// \param [in] ImgID (Optional) ID for ImageToSearch. Makes it easier to debug if something fails.
/// \return A vector with the coordinates where Image matched.
////////////////////////////////////////////////////////////////////////////////////////////////////
vector<Point> Image::SearchInsideOtherImage(const Mat &ImageToSearch, const Mat &Image, int MatchMethod, char Mode, double TolerancePerc, vector<double> &Values, string ImgID) {
    vector<Point> Coordinates;
    Values.clear();

    if (ImageToSearch.empty()) {
        cout << "-E- Image::SearchInsideOtherImage - Can't read the image to search";
        if (ImgID != "") cout << ": '" << ImgID << "'" << endl;
        else cout << "." << endl;
        return Coordinates;
    }
    if (Image.empty()) {
        cout << "-E- Image::SearchInsideOtherImage - Can't read the image to search into." << endl;
        return Coordinates;
    }
    if (ImageToSearch.cols > Image.cols || ImageToSearch.rows > Image.rows) {
        cout << "-E- Image::SearchInsideOtherImage - <ImageToSearch> dimensions must be smaller than <Image>." << endl;
        return Coordinates;
    }
    if (ImageToSearch.type() != Image.type()) {
        cout << "-E- Image::SearchInsideOtherImage - <ImageToSearch> type must be the same as <Image>." << endl;
        return Coordinates;
    }
    if (MatchMethod < 0 || MatchMethod > 5) {
        cout << "-E- Image::SearchInsideOtherImage - <MatchMethod> must be one of these: CV_TM_SQDIFF, CV_TM_SQDIFF_NORMED, CV_TM_CCORR, CV_TM_CCORR_NORMED, CV_TM_CCOEFF or CV_TM_CCOEFF_NORMED." << endl;
        return Coordinates;
    }
    if (TolerancePerc < 0.0 || TolerancePerc > 1.0) {
        cout << "-E- Image::SearchInsideOtherImage - <TolerancePerc> must be >= 0 and <= 1." << endl;
        return Coordinates;
    }
    if (Mode != 'm' && Mode != 's') {
        cout << "-E- Image::SearchInsideOtherImage - <Mode> must be 's' to search for a single instance of <ImageToSearch> or 'm' for multiple instances of <ImageToSearch>." << endl;
        return Coordinates;
    }

    // Create an image with ImageToSearch appended to it so that the algorithm that searches has a reference of a perfect match.
    Mat ImageToSearchWithBorder;
    Mat SearchInsideThisImage;
    copyMakeBorder(ImageToSearch, ImageToSearchWithBorder, ImageToSearch.rows/4, ImageToSearch.rows/4, ImageToSearch.cols/4, ImageToSearch.cols/4, BORDER_REPLICATE);
    copyMakeBorder(Image, SearchInsideThisImage, 0, ImageToSearchWithBorder.rows, 0, ImageToSearchWithBorder.cols, BORDER_CONSTANT, Scalar::all(0));
    ImageToSearchWithBorder.copyTo(SearchInsideThisImage.rowRange(0, ImageToSearchWithBorder.rows).colRange(SearchInsideThisImage.cols-ImageToSearchWithBorder.cols, SearchInsideThisImage.cols));

    // Search for the image and normalize the results.
    Mat Result;
    matchTemplate(SearchInsideThisImage, ImageToSearch, Result, MatchMethod);
    normalize(Result, Result, 0, 1, NORM_MINMAX);

    // Get the coordinates and values of the image matches.
    double minVal, maxVal, matchVal;
    Point minLoc, maxLoc, matchLoc;
    int RectColor = (MatchMethod == CV_TM_SQDIFF || MatchMethod == CV_TM_SQDIFF_NORMED) ? 255 : 0;
    // Loop until matches are worse than the accepted tolerance.
    int i = 0;
    do {
        // Stop if only a single match is required.
        if (Mode == 's') {
            if (i > 1) break;
            i++;
        }
        // Get the min/max values and their locations.
        minMaxLoc(Result, &minVal, &maxVal, &minLoc, &maxLoc);
        // Set matchLoc and matchVal
        if (MatchMethod  == CV_TM_SQDIFF || MatchMethod == CV_TM_SQDIFF_NORMED) {
            matchLoc = minLoc;
            matchVal = minVal;
        } else {
            matchLoc = maxLoc;
            matchVal = 1.0-maxVal;
        }
        // Adjust match location to be the center of the image.
        matchLoc.x += ImageToSearch.cols/2;
        matchLoc.y += ImageToSearch.rows/2;
        // Erase the current match to leave it out of the min/max search in the next iteration.
        rectangle(Result, Point(matchLoc.x-ImageToSearch.cols, matchLoc.y-ImageToSearch.rows), Point(matchLoc.x, matchLoc.y), Scalar::all(RectColor), CV_FILLED, 8, 0);
        // Image found.
        if (matchLoc.x < Image.cols && matchLoc.y < Image.rows && matchVal <= TolerancePerc) {
            Coordinates.push_back(matchLoc);
            Values.push_back(matchVal);
            // DEBUG START
            //cout << "-D- Image::SearchInsideOtherImage - " << ImgID << " i: " << i << " matchVal: " << matchVal << endl;
            //imshow("-D- Image::SearchInsideOtherImage", Result);
            //waitKey(0);
            // DEBUG END
        }
    } while (matchVal <= TolerancePerc);

    return Coordinates;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// \brief Image::SearchInsideOtherImage Variant that doesn't use the Values parameter.
////////////////////////////////////////////////////////////////////////////////////////////////////
vector<Point> Image::SearchInsideOtherImage(const Mat &ImageToSearch, const Mat &Image, int MatchMethod, char Mode, double TolerancePerc, string ImgID) {
    vector<double> Values;
    return SearchInsideOtherImage(ImageToSearch, Image, MatchMethod, Mode, TolerancePerc, Values, ImgID);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////
/// \brief Image::FindClick Find an image inside WindowImage. If found, click on WindowHWND on the coordinates where found.
/// \param [in] ImageToSearch Image that is going to be searched inside a bigger image.
/// \param [in] WindowImage Image where ImageToSearch is going to be searched.
/// \param [in] WindowHWND Window handler where to click.
/// \param [in] Click true = click, false = don't click
/// \param [in] Delay Amount of milliseconds to wait after click.
/// \param [in] TolerancePerc Determines how good must the match be to be succesful. Value must be between 0 and 1, 0 = image must match exactly.
/// \param [out] Result true = image was found, false = image was not found. Useful when using the function in threads, where there is no possibility of using the return value.
/// \param [in] ImgID (Optional) ID for ImageToSearch. Makes it easier to debug if something fails.
/// \return true = image was found, false = image was not found.
///////////////////////////////////////////////////////////////////////////////////////////////////////
bool Image::FindClick(const Mat &ImageToSearch, const Mat &WindowImage, HWND WindowHWND, bool Click, int Delay, double TolerancePerc, bool &Result, string ImgID) {
    // Search the image inside the window image.
    vector<Point> Coordinates = SearchInsideOtherImage(ImageToSearch, WindowImage, CV_TM_CCOEFF_NORMED, 's', TolerancePerc, ImgID);
    if (Coordinates.size() != 1) {
        Result = false;
        return false;
    }
    // If found and Click is true, click the image.
    if (Click) {
        Mouse::WindowHWND = WindowHWND;
        Mouse::PositionMode = MOUSE_POSITION_MODE_WINDOW;
        Mouse::LeftClick({Coordinates[0].x, Coordinates[0].y});
        if (ImgID != "") cout << "-D- Image::FindClick - Clicked image: '" << ImgID << "'" << endl;
        Sleep(Delay);
    }

    Result = true;
    return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// For all the Find and Click methods below the description of the parameters
/// and return value can be found at FindClick's description.
////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////
/// \brief Image::Find FindClick variant that only finds and doesn't click.
/// \brief Doesn't use the WindowHWND parameter.
////////////////////////////////////////////////////////////////////////////////////////////////////
bool Image::Find(const Mat &ImageToSearch, const Mat &WindowImage, double TolerancePerc, bool &Result, string ImgID) {
    bool Click = false;
    int Delay = 0;
    HWND WindowHWND = NULL;
    return FindClick(ImageToSearch, WindowImage, WindowHWND, Click, Delay, TolerancePerc, Result, ImgID);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// \brief Image::Find FindClick variant that only finds and doesn't click.
/// \brief Doesn't use the Result parameter.
/// \brief Doesn't use the WindowHWND parameter.
////////////////////////////////////////////////////////////////////////////////////////////////////
bool Image::Find(const Mat &ImageToSearch, const Mat &WindowImage, double TolerancePerc, string ImgID) {
    bool Click = false;
    int Delay = 0;
    bool Result;
    HWND WindowHWND = NULL;
    return FindClick(ImageToSearch, WindowImage, WindowHWND, Click, Delay, TolerancePerc, Result, ImgID);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// \brief Image::Find FindClick variant that only finds and doesn't click.
/// \brief Doesn't use the WindowImage parameter, it automatically obtains it.
////////////////////////////////////////////////////////////////////////////////////////////////////
bool Image::Find(const Mat &ImageToSearch, HWND WindowHWND, double TolerancePerc, bool &Result, string ImgID) {
    bool Click = false;
    int Delay = 0;
    Mat WindowImage = HWND2Mat(WindowHWND);
    return FindClick(ImageToSearch, WindowImage, WindowHWND, Click, Delay, TolerancePerc, Result, ImgID);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// \brief Image::Find FindClick variant that only finds and doesn't click.
/// \brief Doesn't use the Result parameter.
/// \brief Doesn't use the WindowImage parameter, it automatically obtains it.
////////////////////////////////////////////////////////////////////////////////////////////////////
bool Image::Find(const Mat &ImageToSearch, HWND WindowHWND, double TolerancePerc, string ImgID) {
    bool Click = false;
    int Delay = 0;
    bool Result;
    Mat WindowImage = HWND2Mat(WindowHWND);
    return FindClick(ImageToSearch, WindowImage, WindowHWND, Click, Delay, TolerancePerc, Result, ImgID);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// \brief Image::Click FindClick variant that always clicks.
/// \brief Doesn't use the Result parameter.
////////////////////////////////////////////////////////////////////////////////////////////////////
bool Image::Click(const Mat &ImageToSearch, const Mat &WindowImage, HWND WindowHWND, int Delay, double TolerancePerc, string ImgID) {
    bool Click = true;
    bool Result;
    return FindClick(ImageToSearch, WindowImage, WindowHWND, Click, Delay, TolerancePerc, Result, ImgID);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// \brief Image::Click FindClick variant that always clicks.
/// \brief Doesn't use the Result parameter.
/// \brief Doesn't use the WindowImage parameter, it automatically obtains it.
////////////////////////////////////////////////////////////////////////////////////////////////////
bool Image::Click(const Mat &ImageToSearch, HWND WindowHWND, int Delay, double TolerancePerc, string ImgID) {
    bool Click = true;
    bool Result;
    Mat WindowImage = HWND2Mat(WindowHWND);
    return FindClick(ImageToSearch, WindowImage, WindowHWND, Click, Delay, TolerancePerc, Result, ImgID);
}
