#include <iostream>
#include <stdio.h>  
#include <vector>

#include "opencv2/core.hpp"  
#include "opencv2/core/utility.hpp"  
#include "opencv2/core/ocl.hpp"  
#include "opencv2/imgcodecs.hpp"  
#include "opencv2/highgui.hpp"  
#include "opencv2/features2d.hpp"  
#include "opencv2/calib3d.hpp"  
#include "opencv2/imgproc.hpp"  
#include "opencv2/flann.hpp"  
#include "opencv2/xfeatures2d.hpp"  
#include "opencv2/ml.hpp"  

using namespace cv;
using namespace std;
using namespace cv::xfeatures2d;
using namespace cv::ml;

vector<KeyPoint> Ransac_keypoint1, Ransac_keypoint2;
vector<DMatch> Ransac_matches;

Mat FundamentalRansac(vector<KeyPoint> &current_keypoint1, vector<KeyPoint> &current_keypoint2, vector<DMatch> &current_matches)
{
	vector<Point2f>p1, p2;
	for (size_t i = 0; i < current_matches.size(); i++)
	{
		p1.push_back(current_keypoint1[i].pt);
		p2.push_back(current_keypoint2[i].pt);
	}

	vector<uchar> RansacStatus;
	Mat Fundamental = findFundamentalMat(p1, p2, RansacStatus, FM_RANSAC);
	int index = 0;
	for (size_t i = 0; i < current_matches.size(); i++)
	{
		if (RansacStatus[i] != 0)
		{
			Ransac_keypoint1.push_back(current_keypoint1[i]);
			Ransac_keypoint2.push_back(current_keypoint2[i]);
			current_matches[i].queryIdx = index;
			current_matches[i].trainIdx = index;
			Ransac_matches.push_back(current_matches[i]);
			index++;
		}
	}
	return Fundamental;
}





int main(int argc, char** argv)
{
	//Mat a = imread(argv[1], IMREAD_GRAYSCALE);
	//Mat b = imread(argv[2], IMREAD_GRAYSCALE);
	Mat a = imread("r-1.png", IMREAD_GRAYSCALE);
	Mat b = imread("r-2.png", IMREAD_GRAYSCALE);
	if (!a.data || !b.data)
	{
		cout << "image read error!" << endl;
		return -1;
	}

	// �������������
	Ptr<SURF> surf = SURF::create(800);

	//BFMatcher matcher;
	FlannBasedMatcher matcher; //ʵ����һ��Flannƥ����

	Mat descriptor1, descriptor2;
	vector<KeyPoint>key1, key2;
	vector<DMatch> matches;

	surf->detectAndCompute(a, Mat(), key1, descriptor1);
	surf->detectAndCompute(b, Mat(), key2, descriptor2);

	matcher.match(descriptor1, descriptor2, matches);

	// ��ƥ�����ݽ�����������
	sort(matches.begin(), matches.end());

	cout << "��ƥ�����Ϊ: " << matches.size() << endl << endl;

	// cout << "��ʼѡ������ǰ��  " << (int)(matches.size() / atoi(argv[3])) << "��������Ransac." << endl << endl;
	cout << "��ʼѡ���������ǰ��" << (int)(matches.size() / 10) << "��������Ransac." << endl << endl;

	vector< DMatch > good_matches;

	// int ptsPairs = std::min((int)(matches.size() / atoi(argv[3])), (int)matches.size());
	int ptsPairs = std::min((int)(matches.size() / 10), (int)matches.size());
	for (int i = 0; i < ptsPairs; i++)
	{
		good_matches.push_back(matches[i]);
	}

	vector<KeyPoint> keypoint1, keypoint2;
	//vector<Point2f> keypoint1, keypoint2;
	for (size_t i = 0; i < good_matches.size(); i++)
	{
		keypoint1.push_back(key1[good_matches[i].queryIdx]);
		keypoint2.push_back(key2[good_matches[i].trainIdx]);
	}

	Mat img_matches;
	drawMatches(a, key1, b, key2, good_matches, img_matches);
	namedWindow("��ƥ������ǰ", CV_WINDOW_NORMAL);
	imshow("��ƥ������ǰ", img_matches);
	cvWaitKey(1);

	int times = 0, current_num = 1, per_num = 0;;
	Mat img_Ransac_matches;
	char window_name[] = "0��Ransac֮��ƥ����";
	Mat Fundamental;
	while (1)
	{
		if (per_num != current_num) {
			Ransac_keypoint1.clear();
			Ransac_keypoint2.clear();
			Ransac_matches.clear();
			per_num = good_matches.size();
			Fundamental = FundamentalRansac(keypoint1, keypoint2, good_matches);
			cout << endl << "Ransac" << ++times << "��֮���ƥ�����Ϊ��" << Ransac_matches.size() << endl;
			cout << "��������:" << endl;
			cout << Fundamental << endl << endl << endl;
			window_name[0] = times + '0';
			drawMatches(a, Ransac_keypoint1, b, Ransac_keypoint2, Ransac_matches, img_Ransac_matches);
			namedWindow(window_name, CV_WINDOW_NORMAL);
			imshow(window_name, img_Ransac_matches);
			cvWaitKey(1);
			keypoint1.clear();
			keypoint2.clear();
			good_matches.clear();
			keypoint1 = Ransac_keypoint1;
			keypoint2 = Ransac_keypoint2;
			good_matches = Ransac_matches;
			current_num = good_matches.size();
		}
		else
			break;
	}

	/*
	Mat homo = findHomography(keypoint1, keypoint2, CV_RANSAC);
	// Mat homo = findHomography( keypoint1, keypoint2 );

	cout << "�任����Ϊ��" << homo << endl;
	
	// ������׼ͼ���ĸ���������
	// CalcCorners(homo, b);

	//ͼ����׼
	Mat imageWrap; // , imageTransform2;
	//warpPerspective(b, imageWrap, homo, Size(b.cols, b.rows + 100)); //͸�ӱ任
	warpPerspective(a, imageWrap, homo, Size(a.cols, a.rows)); //͸�ӱ任

	namedWindow("wrap", CV_WINDOW_NORMAL);
	imshow("wrap", imageWrap);

	//����ƴ�Ӻ��ͼ,����ǰ����ͼ�Ĵ�С
	int dst_width = imageWrap.cols;
	int dst_height = imageWrap.rows;

	Mat dst(dst_height, dst_width, CV_8UC3);
	dst.setTo(0);

	for (int i = 0; i < dst_height; ++i) {
		for (int j = 0; j < dst_width; ++j) {
			if (a.at<Vec3b>(i, j)[0] != 0 && imageWrap.at<Vec3b>(i, j)[0] == 0)
				dst.at<Vec3b>(i, j) = a.at<Vec3b>(i, j);
			else if (a.at<Vec3b>(i, j)[0] == 0 && imageWrap.at<Vec3b>(i, j)[0] != 0)
				dst.at<Vec3b>(i, j) = imageWrap.at<Vec3b>(i, j);
			else
				dst.at<Vec3b>(i, j) = imageWrap.at<Vec3b>(i, j) * 0.5 + a.at<Vec3b>(i, j) * 0.5;
		}
	}

	namedWindow("ƴ�ӽ��", CV_WINDOW_NORMAL);
	imshow("ƴ�ӽ��", dst);
	imwrite("dst.png", dst);
	*/

	cvWaitKey(0);
	return 0;
}