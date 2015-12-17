#include <opencv2/opencv.hpp>
#include <vector>
#include <string>
#include "scope_exit.h"

#ifdef _MSC_VER
#ifdef _DEBUG
#pragma comment(lib,"opencv_world300d.lib")
#else
#pragma comment(lib,"opencv_world300.lib")
#endif //_DEBUG
#endif //_MSC_VER

cv::Mat scaleToSuitableSize(const cv::Mat& srcImg)
{
	const int suitableSize = 800;
	if (srcImg.cols < suitableSize && srcImg.rows < suitableSize)
	{
		return srcImg;
	}
	const float widthRate = (float)suitableSize / (float)srcImg.cols;
	const float heightRate = (float)suitableSize / (float)srcImg.rows;
	const auto scaleRate = std::min(widthRate, heightRate);
	cv::Mat dstImg;
	cv::resize(srcImg, dstImg, cv::Size(srcImg.cols*scaleRate,srcImg.rows*scaleRate));
	return dstImg;
}
cv::Mat binaryImage(const cv::Mat& grayImg)
{
	cv::Mat dstImg;
	//cv::threshold(grayImg, dstImg, 125, 255, cv::THRESH_TOZERO_INV);
	cv::adaptiveThreshold(grayImg, dstImg, 255, cv::ADAPTIVE_THRESH_MEAN_C, cv::THRESH_BINARY_INV, 21, 21);
	return dstImg;
}
cv::Mat smoothImage(const cv::Mat& binaryImg)
{
	cv::Mat dstImg;
	const int elementSize = 3;
	cv::Mat kernel = cv::getStructuringElement(cv::MORPH_RECT,cv::Size(2 * elementSize + 1, 2 * elementSize + 1));
	cv::morphologyEx(binaryImg, dstImg, cv::MORPH_DILATE, kernel);
	return dstImg;
}
std::vector<cv::Rect> extraChars(const cv::Mat& binaryImg)
{
	std::vector<cv::Rect> result;
	std::vector<std::vector<cv::Point>> contours;
	cv::findContours(binaryImg.clone(), contours, cv::RETR_LIST, cv::CHAIN_APPROX_SIMPLE);
	if (contours.empty())
	{
		return result;
	}
	int sumHeight = 0;
	for (const auto& contour : contours)
	{
		const auto box = cv::boundingRect(contour);
		sumHeight += box.height;
	}
	const int averHeight = (sumHeight / contours.size()) / 3;
	cv::Mat debugImg(binaryImg.rows, binaryImg.cols, CV_8UC1);
	debugImg.setTo(0);
	int contourIdx = 0;
	for (const auto& contour : contours)
	{
		SCOPEEXITEXEC(contourIdx++);
		const auto box = cv::boundingRect(contour);
		if ( (box.height < averHeight/2 && box.width < averHeight/2)
			|| box.height > averHeight*3 || box.width > averHeight*3)
		{
			continue;
		}
		cv::drawContours(debugImg, contours, contourIdx, 255, -1);
	}
	cv::imshow("debug image", debugImg);
	return result;
}
cv::Mat drawRectsOnImg(const cv::Mat& grayImg, const std::vector<cv::Rect>& boxes)
{
	cv::Mat colorImg;
	cv::cvtColor(grayImg, colorImg, cv::COLOR_GRAY2BGR);
	for (const auto& box : boxes)
	{
		cv::rectangle(colorImg, box, cv::Scalar(255, 0, 0), 2);
	}
	return colorImg;
}
std::vector<std::string> getFilesInFloder(const std::string& floder)
{
	assert(!floder.empty() && floder[floder.size() - 1] == '\\');
	std::vector<std::string> result;
	const std::string dirCmd = std::string("dir /A-D /B /S ") + floder;
	FILE* pipe = _popen(dirCmd.c_str(), "r");
	if (pipe)
	{
		char buffer[1024];
		while (!feof(pipe)) {
			if (fgets(buffer, 1024, pipe)){
				buffer[strlen(buffer)-1] = '\0';
				result.push_back(buffer);
			}
		}
		_pclose(pipe);
	}
	return result;
}
int main(int argc, char* argv[])
{
	const auto srcFiles = getFilesInFloder(R"(D:\ÊÖÐ´Í¼Æ¬\)");
	for (const auto& srcFile :srcFiles)
	{
		const auto srcImg = cv::imread(srcFile, cv::IMREAD_GRAYSCALE);
		const auto scaledImg = scaleToSuitableSize(srcImg);
		const auto binaryImg = binaryImage(scaledImg);
		//todo : ÇãÐ±Ð£Õý
		const auto smoothedImg = smoothImage(binaryImg);
		const auto charBoxes = extraChars(smoothedImg);
		const auto resultImg = drawRectsOnImg(scaledImg, charBoxes);
		cv::imshow("source scaled image", scaledImg);
		cv::imshow("binary image", binaryImg);
		cv::imshow("smoothed binary image", smoothedImg);
		cv::imshow("extra result image", resultImg);
		cv::waitKey(0);
	}
	return 0;
}