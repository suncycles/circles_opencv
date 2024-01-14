#ifndef UNICODE
	
#endif

#define CVUI_DISABLE_COMPILATION_NOTICES
#define CVUI_IMPLEMENTATION
#define BUFSIZE 512

#include <iostream>
#include <fstream>
#include <string.h>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/opencv.hpp>
#include <opencv2/imgcodecs.hpp>
#include <windows.h>
#include "cvui.h"
#include "commdlg.h"
#include <psapi.h>
#include <strsafe.h>
#include <tchar.h>
#include <stdio.h>
#include <strsafe.h>
#include <filesystem>

#define WINDOW_NAME "window"

using namespace cv;
using namespace std; 

struct Params {
	cv::Mat* src;
	cv::Mat* dest;
};


Mat img, out, canny, brightImgCopy;
vector<Vec3f> circles;
int dp = 2;
int minDist = 40;
int param1 = 200;
int param2 = 40;
int minRad = 50;
int maxRad = 130;
const int maxVal = 255;
int lowThreshold;
int minCanny;
int minCanny1;
int brightness = 40;
int houghThreshold;

OPENFILENAME ofn = {0};
wchar_t szFile[260];
HWND hwnd;
HANDLE hf;
TCHAR pszFilename[MAX_PATH + 1];

BOOL GetFileNameFromHandle(HANDLE hFile)
{
	BOOL bSuccess = FALSE;
	HANDLE hFileMap;

	// Get the file size.
	DWORD dwFileSizeHi = 0;
	DWORD dwFileSizeLo = GetFileSize(hFile, &dwFileSizeHi);

	if (dwFileSizeLo == 0 && dwFileSizeHi == 0)
	{
		_tprintf(TEXT("Cannot map a file with a length of zero.\n"));
		return FALSE;
	}

	// Create a file mapping object.
	hFileMap = CreateFileMapping(hFile, NULL, PAGE_READONLY, 0, 1, NULL);

	if (hFileMap)
	{
		// Create a file mapping to get the file name.
		void* pMem = MapViewOfFile(hFileMap, FILE_MAP_READ, 0, 0, 1);

		if (pMem)
		{
			if (GetMappedFileName(GetCurrentProcess(),
				pMem,
				pszFilename,
				MAX_PATH))
			{
				// Translate path with device name to drive letters.
				TCHAR szTemp[BUFSIZE];
				szTemp[0] = '\0';

				if (GetLogicalDriveStrings(BUFSIZE - 1, szTemp))
				{
					TCHAR szName[MAX_PATH];
					TCHAR szDrive[3] = TEXT(" :");
					BOOL bFound = FALSE;
					TCHAR* p = szTemp;

					do
					{
						// Copy the drive letter to the template string
						*szDrive = *p;

						// Look up each device name
						if (QueryDosDevice(szDrive, szName, MAX_PATH))
						{
							size_t uNameLen = _tcslen(szName);

							if (uNameLen < MAX_PATH)
							{
								bFound = _tcsnicmp(pszFilename, szName, uNameLen) == 0
									&& *(pszFilename + uNameLen) == _T('\\');

								if (bFound)
								{
									// Reconstruct pszFilename using szTempFile
									// Replace device path with DOS path
									TCHAR szTempFile[MAX_PATH];
									StringCchPrintf(szTempFile, MAX_PATH, TEXT("%s%s"), szDrive, pszFilename + uNameLen);
									StringCchCopyN(pszFilename, MAX_PATH + 1, szTempFile, _tcslen(szTempFile));
								}
							}
						}
						// Go to the next NULL character.
						while (*p++);
					} while (!bFound && *p); // end of string
				}
			}
			bSuccess = TRUE;
			UnmapViewOfFile(pMem);
		}

		CloseHandle(hFileMap);
	}
	return(bSuccess);
}
void minDistSlider(int i, void* data) {
	minDist = i;
}
void param1Slider(int i, void* data) {
	param1 = i;
}
void param2Slider(int i, void* data) {
	param2 = i;
}
void minRadSlider(int i, void* data) {
	minRad = i;
}
void maxRadSlider(int i, void* data) {
	maxRad = i;
}
static void CannyThreshold(int, void*) {
	Canny(brightImgCopy, canny, minCanny, maxVal);
	imshow("canny", canny);
}
static void CannyThreshold1(int, void*) {
	Canny(brightImgCopy, canny, minCanny1, maxVal);
	imshow("canny", canny);
}
void darkenTrackbar(int pos, void* data) {
	Params* params = (Params*)data;
	for (int i = 0; i < params->src->rows; i++) {
		for (int j = 0; j < params->src->cols; j++) {
			Vec3b pixelColor;
			pixelColor = params->src->at<Vec3b>(Point(j, i));
			for (int k = 0; k < 3; k++) {
				if (pixelColor[k] + pos > 255)
					pixelColor[k] = 255;
				else
					pixelColor[k] -= pos;
				params->dest->at<Vec3b>(Point(j, i)) = pixelColor;
			}
		}
	}
	imshow("Bright Image", *(params->dest));
	(*(params->dest)).copyTo(brightImgCopy);
}
void showCircles(vector<Vec3f> &circles, Mat &src) {

	for (size_t i = 0; i < circles.size(); i++)
	{
		Vec3i c = circles[i];
		Point center = Point(c[0], c[1]);
		// circle center
		circle(src, center, 1, Scalar(0, 100, 100), 3, LINE_AA);
		// circle outline
		int radius = c[2];
		circle(src, center, radius, Scalar(255, 0, 255), 3, LINE_AA);
	}
	imshow("post", src);
}
void getFile() {
	//init
	ZeroMemory(&ofn, sizeof(ofn));
	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = hwnd;
	ofn.lpstrFile = (szFile);
	ofn.lpstrFile[0] = '\0';
	ofn.nMaxFile = sizeof(szFile);
	ofn.lpstrFilter = L"All\0*.*\0Text\0*.TXT\0";
	ofn.nFilterIndex = 1;
	ofn.lpstrFileTitle = NULL;
	ofn.nMaxFileTitle = 0;
	ofn.lpstrInitialDir = NULL;
	ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;

	if (GetOpenFileName(&ofn) == TRUE)
		hf = CreateFile(ofn.lpstrFile, GENERIC_READ, 0, (LPSECURITY_ATTRIBUTES)NULL,
			OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, (HANDLE)NULL);
		GetFileNameFromHandle(hf); // filename goes into pszFileName
		CloseHandle(hf);
}
string fixPath(string input) {
	std::string result;
	for (char c : input) {
		if (c == '\\') {
			result += '/';
		}
		else {
			result += c;
		}
	}
	return result;
}
std::string UnicodeToASCII(const std::wstring& unicodeString) {
	std::string asciiString;
	for (wchar_t c : unicodeString) {
		if (c <= 0x7F) {
			asciiString += static_cast<char>(c);
		}
		else {
			asciiString += '?';
		}
	}
	return asciiString;
}
void DrawButtonCallback(int state, void* userdata) {
	state = 1;
	std::cout << "button pressed";
	
}
int main(int argc, char** argv) {
	bool preview = false;
	string img_Path_Preview;
	cvui::init(WINDOW_NAME);
	Mat frame = Mat(Size(800, 600), CV_8UC3);
	//gui
	while (true) {
		frame = cv::Scalar(49, 52, 49);
		cvui::text(frame, 20, 25, "openCV Circle Detection");

		if (cvui::button(frame, 20, 50, 100, 50, "Open File")) {
			getFile();
			wprintf(L"\nSelected %s", pszFilename);

			//generate a preview on the window
			string input = fixPath(UnicodeToASCII(pszFilename));
			img_Path_Preview = "\nPath: " + input;

			img = imread(input);

			if (img.empty()) {
				std::cout << "\nCould not open or find the image" << endl;
			}
			else {
				preview = true;
			}
		}	

		if (preview) {
			cvui::text(frame, 20, 150, img_Path_Preview);
			cvui::text(frame, 20, 130, "Preview");
			string preview_temp_string = fixPath(UnicodeToASCII(pszFilename));
			Mat preview_temp = imread(preview_temp_string);
			cv::resize(preview_temp, preview_temp, Size(), 0.2, 0.2);
			cvui::image(frame, 20, 170, preview_temp);
		}

		//make selected image into main imread input file
		if (cvui::button(frame, 155, 50, 50, 50, "OK")) {
			// Main processing funciton
			Mat blur;
			Mat brightImage, post_img;

			cv::resize(img, img, Size(), 0.5, 0.5);
			img.copyTo(brightImage);

			Params params;

			params.src = &img;
			params.dest = &brightImage;
			String Parameter_WINDOWNAME = "Parameter Sliders";
			cv::namedWindow("Bright Image", WINDOW_AUTOSIZE);
			cv::namedWindow(Parameter_WINDOWNAME, WINDOW_NORMAL);
			resizeWindow(Parameter_WINDOWNAME, 400, 200);
			//sliders
			cv::createTrackbar("Min Dist", Parameter_WINDOWNAME, &minDist, maxVal, minDistSlider);
			cv::createTrackbar("Canny Edge Parameter", Parameter_WINDOWNAME, &param1, maxVal, param1Slider);
			cv::createTrackbar("Center Detection Parameter", Parameter_WINDOWNAME, &param2, maxVal, param2Slider);
			cv::createTrackbar("Min Rad", Parameter_WINDOWNAME, &minRad, maxVal, minRadSlider);
			cv::createTrackbar("Max Rad", Parameter_WINDOWNAME, &maxRad, maxVal, maxRadSlider);
			cv::createTrackbar("Hue", "Bright Image", &brightness, maxVal, darkenTrackbar, &params);
			cv::putText(brightImgCopy, "Press ENTER", cv::Point(50, 50), cv::FONT_HERSHEY_DUPLEX, 1, cv::Scalar(0, 255, 0), 2, false);
			darkenTrackbar(brightness, &params);

			while (cv::getWindowProperty("Bright Image", WND_PROP_VISIBLE) == 1) {
				if (cv::waitKey(0) == (char)13) {
					brightImgCopy.copyTo(post_img);

					cvtColor(post_img, post_img, COLOR_BGR2GRAY);
					GaussianBlur(post_img, post_img, Size(5, 5), 2);

					HoughCircles(post_img, circles, HOUGH_GRADIENT, dp, minDist, param1, param2, minRad, maxRad);
					showCircles(circles, post_img);

					imshow("post", post_img);
					cout << "\nDetected " << circles.size() << " Circles." << endl;
				}
			}
		}
		cvui::update();
		cvui::imshow(WINDOW_NAME, frame);

		if (cv::waitKey(1) == 27) {
			break;
		}
	}

	cv::waitKey(0);
	return 0;
}