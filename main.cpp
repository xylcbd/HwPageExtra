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
// std::vector<cv::Rect> extraChars(const cv::Mat& binaryImg)
// {
// 	std::vector<cv::Rect> result;
// 	std::vector<std::vector<cv::Point>> contours;
// 	cv::findContours(binaryImg.clone(), contours, cv::RETR_LIST, cv::CHAIN_APPROX_NONE);
// 	if (contours.empty())
// 	{
// 		return result;
// 	}
// 	int sumHeight = 0;
// 	for (const auto& contour : contours)
// 	{
// 		const auto box = cv::boundingRect(contour);
// 		sumHeight += box.height;
// 	}
// 	const int averHeight = (sumHeight / contours.size()) / 3;
// 	cv::Mat debugImg(binaryImg.rows, binaryImg.cols, CV_8UC1);
// 	debugImg.setTo(0);
// 	int contourIdx = 0;
// 	for (const auto& contour : contours)
// 	{
// 		SCOPEEXITEXEC(contourIdx++);
// 		const auto box = cv::boundingRect(contour);
// 		if ( (box.height < averHeight/2 && box.width < averHeight/2)
// 			|| box.height > averHeight*3 || box.width > averHeight*3)
// 		{
// 			continue;
// 		}
// 		if (box.y+box.height/2 < 0.08f * binaryImg.rows ||
// 			box.y + box.height / 2 > 0.93f * binaryImg.rows ||
// 			box.x + box.width / 2 < 0.06f * binaryImg.cols ||
// 			box.x + box.width / 2 > 0.94f * binaryImg.cols)
// 		{
// 			continue;
// 		}
// 		cv::drawContours(debugImg, contours, contourIdx, 255, -1);
// 	}
// 	cv::imshow("debug image", debugImg);
// 	return result;
// }
std::vector<std::vector<cv::Rect>> extraChars(const cv::Mat& binaryImg)
{
	std::vector<std::vector<cv::Rect>> result;
	std::vector<std::vector<cv::Point>> contours;
	cv::findContours(binaryImg.clone(), contours, cv::RETR_LIST, cv::CHAIN_APPROX_NONE);
	if (contours.empty())
	{
		return result;
	}
	int sumHeight = 0;
	int sumCount = 0;
	for (const auto& contour : contours)
	{
		const auto box = cv::boundingRect(contour);
		if (box.width <= 6 && box.height <= 6)
		{
			continue;
		}else if (box.width >= 100 || box.height >=100)
		{
			continue;
		}
		sumHeight += box.height;
		sumCount++;
	}
	const int averHeight = (sumCount == 0) ? 15 : (sumHeight / sumCount);
	std::vector<cv::Rect> textBoxes;
	cv::Mat cleanImg(binaryImg.rows, binaryImg.cols, CV_8UC1);
	cleanImg.setTo(0);
	int contourIdx = 0;
	for (const auto& contour : contours)
	{
		SCOPEEXITEXEC(contourIdx++);
		const auto box = cv::boundingRect(contour);
		if ((box.height < averHeight / 2 && box.width < averHeight / 2)
			|| box.height > averHeight * 3 || box.width > averHeight * 3)
		{
			continue;
		}
		if (box.y + box.height / 2 < 0.08f * binaryImg.rows ||
			box.y + box.height / 2 > 0.93f * binaryImg.rows ||
			box.x + box.width / 2 < 0.06f * binaryImg.cols ||
			box.x + box.width / 2 > 0.94f * binaryImg.cols)
		{
			continue;
		}
		textBoxes.push_back(box);
		cv::drawContours(cleanImg, contours, contourIdx, 255, -1);
	}
	cleanImg = smoothImage(cleanImg);
	cv::findContours(cleanImg.clone(), contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_NONE);
	if (contours.empty())
	{
		return result;
	}
	cleanImg.setTo(0);
	contourIdx = 0;
	textBoxes.clear();
	for (const auto& contour : contours)
	{
		SCOPEEXITEXEC(contourIdx++);
		const auto box = cv::boundingRect(contour);
		textBoxes.push_back(box);
		cv::drawContours(cleanImg, contours, contourIdx, 255, -1);
	}
	cv::imshow("clean image", cleanImg);

	std::sort(textBoxes.begin(), textBoxes.end(), [](const cv::Rect& lhs, const cv::Rect& rhs)->bool{
		const auto lhsCenter = lhs.y + lhs.height / 2;
		const auto rhsCenter = rhs.y + rhs.height / 2;
		if (lhsCenter == rhsCenter)
		{
			return false;
		}
		return lhsCenter < rhsCenter;
	});
	std::vector<int> rowDesc;
	std::vector<int> rowCharCount;
	std::vector<int> rowIdx(textBoxes.size(),-1);
	const auto rowThreshold = averHeight*2;
	for (int i = 0; i < textBoxes.size();i++)
	{
		const auto box = textBoxes[i];
		const auto yCenter = box.y + box.height / 2;
		for (int j = 0; j < rowDesc.size(); j++)
		{
			if (abs(yCenter-rowDesc[j])<rowThreshold)
			{
				rowIdx[i] = j;
				rowCharCount[j]++;
				break;
			}
		}
		if (rowIdx[i] == -1)
		{
			rowDesc.push_back(yCenter);
			rowCharCount.push_back(1);
			rowIdx[i] = rowDesc.size() - 1;			
		}
	}

	for (int i = 0; i < rowCharCount.size();i++)
	{
		if (rowCharCount[i] >= 7)
		{
			//delete this row
			continue;
		}
		//
		for (int j = 0; j < rowIdx.size(); j++)
		{
			const auto idx = rowIdx[j];
			if (idx == i)
			{
				rowIdx[j] = -1;
			}
		}
	}
	int lastRowIdx = -1;
	for (int i = 0; i < rowIdx.size(); i++)
	{
		const auto idx = rowIdx[i];
		if (idx != -1)
		{
			if (lastRowIdx == -1 || lastRowIdx != idx)
			{
				result.resize(std::max(result.size(), (size_t)idx+1));
			}			
			result[idx].push_back(textBoxes[i]);			
			lastRowIdx = idx;
		}
	}
	int usedSize = 0;
	for (int i = 0; i < result.size();i++)
	{
		if (result[i].empty())
		{
			continue;
		}
		std::sort(result[i].begin(), result[i].end(), [](const cv::Rect& lhs, const cv::Rect& rhs)->bool{
			const auto lhsCenter = lhs.x + lhs.width / 2;
			const auto rhsCenter = rhs.x + rhs.width / 2;
			if (lhsCenter == rhsCenter)
			{
				return false;
			}
			return lhsCenter < rhsCenter;
		});
		result[usedSize++] = result[i];
	}
	result.resize(usedSize);
	cv::Mat usedTextImg(binaryImg.rows, binaryImg.cols, CV_8UC1);
	usedTextImg.setTo(0);
	for (int i = 0; i < result.size(); i++)
	{
		for (int j = 0; j < result[i].size(); j++)
		{
			cv::rectangle(usedTextImg, result[i][j], 255, -1);
		}
	}
	cv::imshow("used text image", usedTextImg);
	return result;
}
cv::Mat drawRectsOnImg(const cv::Mat& grayImg, const std::vector<std::vector<cv::Rect>>& boxes)
{
	cv::Mat colorImg;
	cv::cvtColor(grayImg, colorImg, cv::COLOR_GRAY2BGR);
	for (const auto& rowBox : boxes)
	{
		int idxInRow = 0;
		for (const auto& box : rowBox)
		{
			if (idxInRow % 3 == 0)
			{
				cv::rectangle(colorImg, box, cv::Scalar(0, 0, 255), 2);
			}
			else
			{
				cv::rectangle(colorImg, box, cv::Scalar(255, 0, 0), 2);
			}
			idxInRow++;			
		}		
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
static float getSkewAngle(const cv::Mat& binaryImg)
{
	cv::Mat cannyImg;
	cv::Canny(binaryImg, cannyImg, 50, 200, 3);

	std::vector<cv::Vec4i> lines;
	cv::HoughLinesP(cannyImg, lines, 1, CV_PI / 180, binaryImg.cols*0.6f, binaryImg.cols*0.6f, 40);
	//debug
	cv::Mat colorImg;
	cv::cvtColor(cannyImg, colorImg, cv::COLOR_GRAY2BGR);
	for (size_t i = 0; i < lines.size(); i++)
	{
		cv::line(colorImg, cv::Point(lines[i][0], lines[i][1]),
			cv::Point(lines[i][2], lines[i][3]), cv::Scalar(0, 0, 255), 3, 8);
	}
 	cv::imshow("hough lines image", colorImg);
	return 0.0f;
}
int main(int argc, char* argv[])
{
	const auto srcFiles = getFilesInFloder(R"(D:\ÊÖÐ´Í¼Æ¬\)");
	for (const auto& srcFile :srcFiles)
	{
		const auto srcImg = cv::imread(srcFile, cv::IMREAD_GRAYSCALE);
		const auto scaledImg = scaleToSuitableSize(srcImg);		
		const auto binaryImg = binaryImage(scaledImg);				
		const auto smoothedImg = smoothImage(binaryImg);
		//const auto charBoxes = extraChars(smoothedImg);
		const auto charBoxes = extraChars(binaryImg);
		const auto resultImg = drawRectsOnImg(scaledImg, charBoxes);

		//const auto skewAngle = getSkewAngle(smoothedImg);

		cv::imshow("source scaled image", scaledImg);
		cv::imshow("binary image", binaryImg);
		cv::imshow("smoothed binary image", smoothedImg);
		cv::imshow("extra result image", resultImg);
		const auto key = cv::waitKey(0);
		if (key == 27)
		{
			break;
		}
	}
	return 0;
}