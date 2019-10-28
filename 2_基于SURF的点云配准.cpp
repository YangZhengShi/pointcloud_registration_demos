#include <iostream>  
#include <vector>
#include <string>

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

#include <pcl/console/parse.h>
#include <pcl/io/pcd_io.h>
#include <pcl/io/ply_io.h>
#include <pcl/point_types.h>
#include <pcl/visualization/pcl_visualizer.h>
#include <pcl/registration/correspondence_rejection_sample_consensus.h>
#include <pcl/registration/transformation_estimation_svd.h>
#include <pcl/registration/transformation_estimation.h>
#include <pcl/registration/correspondence_estimation.h>

using namespace cv;
using namespace std;
using namespace cv::xfeatures2d;
using namespace cv::ml;

// --------------------------------------
// --------------Parameters--------------
// --------------------------------------
const double camera_factor = 1000;
const double camera_cx = 979.674;
const double camera_cy = 535.383;
const double camera_fx = 1043.02;
const double camera_fy = 1047.78;
string imagelocation_rgb1 = ".\\r-1.png";
string imagelocation_rgb2 = ".\\r-2.png";
string imagelocation_depth1 = ".\\d-1.png";
string imagelocation_depth2 = ".\\d-2.png";
string outputfilename = "transformed.ply";
int min_number_keypoints = 100;


void printUsage(const char* progName);
int getSurfMatches(const string& rgb1_filelocation, const string& rgb2_filelocation, vector<KeyPoint>& keypoints1, vector<KeyPoint>& keypoints2, vector<DMatch>& matches);
int ransacSurfMatchesuseFundamentalMat(vector<KeyPoint>& keypoint1, vector<KeyPoint>& keypoint2, vector<DMatch>& matches);
int keypoints2cloud(vector<KeyPoint>& keypoints1, pcl::PointCloud<pcl::PointXYZ>::Ptr& keycloud1, vector<KeyPoint>& keypoints2, pcl::PointCloud<pcl::PointXYZ>::Ptr& keycloud2);
int rgb_depth2cloud(pcl::PointCloud<pcl::PointXYZRGBA>::Ptr& cloud, const string& rgb_filelocation, const string& imagelocation_depth);
int CorrespondenceRejectorSC(pcl::PointCloud<pcl::PointXYZ>::Ptr& keycloud1, pcl::PointCloud<pcl::PointXYZ>::Ptr& keycloud2, boost::shared_ptr<pcl::Correspondences>& Correspon_in, boost::shared_ptr<pcl::Correspondences>& Correspon_out);
void Transform_use_RTmatrixandSave(pcl::PointCloud<pcl::PointXYZRGBA>::Ptr& sourcecloud, Eigen::Matrix4f& Transform, pcl::PointCloud<pcl::PointXYZRGBA>::Ptr& transformed_cloud);
boost::shared_ptr<pcl::visualization::PCLVisualizer> simpleVis(pcl::PointCloud<pcl::PointXYZRGBA>::ConstPtr transformed_sourcecloud1, pcl::PointCloud<pcl::PointXYZRGBA>::ConstPtr sourcecloud2);


int main(int argc, char** argv)
{
	// --------------------------------------
	// -----Parse Command Line Arguments-----
	// --------------------------------------
	if (pcl::console::find_argument(argc, argv, "-h") >= 0)
	{
		printUsage(argv[0]);
		return 0;
	}

	if (pcl::console::parse(argc, argv, "-n", min_number_keypoints) >= 0)
		cout << "Setting min_number_keypoints to: " << min_number_keypoints << ".\n";
	else
		cout << "Use default min_number_keypoints : " << min_number_keypoints << ".\n";

	if (pcl::console::parse(argc, argv, "-o", outputfilename) >= 0)
		cout << "Setting transformed cloud output filename is: " << outputfilename << ".\n";
	else
		cout << "Use default output filename : " << outputfilename << ".\n";

	vector<int> image_filename_indices = pcl::console::parse_file_extension_argument(argc, argv, "png");
	if (image_filename_indices.size() == 4)
	{
		cout << "Use Input data.\n"
			<< "imagelocation_rgb1------>" << (imagelocation_rgb1 = argv[image_filename_indices[0]]) << endl
			<< "imagelocation_rgb2------>" << (imagelocation_rgb2 = argv[image_filename_indices[1]]) << endl
			<< "imagelocation_depth1------>" << (imagelocation_depth1 = argv[image_filename_indices[2]]) << endl
			<< "imagelocation_depth2------>" << (imagelocation_depth2 = argv[image_filename_indices[3]]) << endl
			<< endl << endl;
	}
	else
	{
		cout << "Use default data.\n"
			<< "imagelocation_rgb1------>" << imagelocation_rgb1 << endl
			<< "imagelocation_rgb2------>" << imagelocation_rgb2 << endl
			<< "imagelocation_depth1------>" << imagelocation_depth1 << endl
			<< "imagelocation_depth2------>" << imagelocation_depth2 << endl
			<< endl << endl;
	}

	clock_t startTime, endTime;
	startTime = clock();

	// --------------------------------------
	// --------------�õ���ƥ��--------------
	// --------------------------------------
	vector<KeyPoint>key1, key2;
	vector<DMatch> matches;
	getSurfMatches(imagelocation_rgb1, imagelocation_rgb2, key1, key2, matches);

	// --------------------------------------
	// ---------FundamentalMatRansac---------
	// --------------------------------------
	int n_keypoints = 0;
	while (1) {
		if ((n_keypoints = ransacSurfMatchesuseFundamentalMat(key1, key2, matches)) < min_number_keypoints)
			break;
		if (ransacSurfMatchesuseFundamentalMat(key1, key2, matches) == n_keypoints)
			break;
	}

	// --------------------------------------------
	// ------Generate the matches pointcloud-------
	// --------------------------------------------
	pcl::PointCloud<pcl::PointXYZ>::Ptr key1cloud(new pcl::PointCloud<pcl::PointXYZ>);
	pcl::PointCloud<pcl::PointXYZ>::Ptr key2cloud(new pcl::PointCloud<pcl::PointXYZ>);
	// �ؼ���ת������
	keypoints2cloud(key1, key1cloud, key2, key2cloud);

	// ------------------------------------------------------
	// ---------Ransac use keypoint cloud ��ƥ��У��---------
	// ------------------------------------------------------
	boost::shared_ptr<pcl::Correspondences> correspondence_in(new pcl::Correspondences);
	boost::shared_ptr<pcl::Correspondences> correspondence_out(new pcl::Correspondences);
	CorrespondenceRejectorSC(key1cloud, key2cloud, correspondence_in, correspondence_out);
	//CorrespondenceRejectorSC(key1cloud, key2cloud, correspondence_in, correspondence_out);

	// ------------------------------------------------------------
	// -----------Compute the RT matrix ������תƽ�ƾ���-----------
	// ------------------------------------------------------------
	// 3ά�ռ��еı任������4*4�ģ���Ϊ�漰����ת��ƽ��
	// 2άƽ���ϵı任������3*3�ģ���Ϊֻ�漰��ת���漰ƽ��(ƽ������)
	Eigen::Matrix4f Transform = Eigen::Matrix4f::Identity();
	pcl::registration::TransformationEstimationSVD<pcl::PointXYZ, pcl::PointXYZ, float>::Ptr
		trans(new pcl::registration::TransformationEstimationSVD<pcl::PointXYZ, pcl::PointXYZ, float>);
	trans->estimateRigidTransformation(*key1cloud, *key2cloud, *correspondence_out, Transform);
	endTime = clock();
	cout << endl << "��תƽ�ƾ���Ϊ: " << endl;
	cout << Transform << endl << endl;
	cout << "�õ���תƽ�ƺ�ʱ: " << (endTime - startTime) / CLOCKS_PER_SEC << "(s)" << endl << endl;

	// ------------------------------------------------------
	// ------Generate the source pointcloud ����Դ����-------
	// ------------------------------------------------------
	pcl::PointCloud<pcl::PointXYZRGBA>::Ptr sourcecloud1(new pcl::PointCloud<pcl::PointXYZRGBA>);
	pcl::PointCloud<pcl::PointXYZRGBA>::Ptr sourcecloud2(new pcl::PointCloud<pcl::PointXYZRGBA>);
	// ����rgbͼ������ͼ���ɵ�������
	rgb_depth2cloud(sourcecloud1, imagelocation_rgb1, imagelocation_depth1);
	rgb_depth2cloud(sourcecloud2, imagelocation_rgb2, imagelocation_depth2);

	// -----------------------------------------------------
	// ------Transfrom sourcecloud and save ���Ʊ任--------
	// -----------------------------------------------------
	pcl::PointCloud<pcl::PointXYZRGBA>::Ptr transformed_cloud(new pcl::PointCloud<pcl::PointXYZRGBA>);
	// ���ݱ任������е�������͸�ӱ任
	Transform_use_RTmatrixandSave(sourcecloud1, Transform, transformed_cloud);

	// -----------------------------------------------------
	// ---------Visualization the result �鿴��׼���-------
	// -----------------------------------------------------
	boost::shared_ptr<pcl::visualization::PCLVisualizer> viewer;
	viewer = simpleVis(transformed_cloud, sourcecloud2);
	while (!viewer->wasStopped())
	{
		viewer->spinOnce(100);
		boost::this_thread::sleep(boost::posix_time::microseconds(100000));
	}

	return 0;
}

// �Դ�ѡ�񲿷ֽ���Ransac������������ƥ��У��
int ransacSurfMatchesuseFundamentalMat
(vector<KeyPoint>& keypoint1, vector<KeyPoint>& keypoint2, vector<DMatch>& matches)
{
	static int count = 0;
	vector<Point2f>p1, p2;
	for (size_t i = 0; i < matches.size(); i++)
	{
		p1.push_back(keypoint1[i].pt);
		p2.push_back(keypoint2[i].pt);
	}

	vector<uchar> RansacStatus;
	Mat Fundamental = findFundamentalMat(p1, p2, RansacStatus, FM_RANSAC);

	vector<KeyPoint> tmp_Ransac_keypoint1, tmp_Ransac_keypoint2;
	vector<DMatch> tmp_Ransac_matches;

	int index = 0;
	for (size_t i = 0; i < matches.size(); i++)
	{
		if (RansacStatus[i] != 0)
		{
			tmp_Ransac_keypoint1.push_back(keypoint1[i]);
			tmp_Ransac_keypoint2.push_back(keypoint2[i]);
			matches[i].queryIdx = index;
			matches[i].trainIdx = index;
			tmp_Ransac_matches.push_back(matches[i]);
			index++;
		}
	}

	keypoint1.clear();
	keypoint2.clear();
	matches.clear();
	keypoint1 = tmp_Ransac_keypoint1;
	keypoint2 = tmp_Ransac_keypoint2;
	matches = tmp_Ransac_matches;

	cout << "=================================[2]=================================" << endl
			<< "�Դ�ѡ�񲿷ֽ���" << ++count << "��Ransac�������ƥ������Ϊ: " << matches.size() << endl;
	return matches.size();
}

void printUsage(const char* progName)
{
	cout << "\n\nUsage: " << progName << " [options] rgb1.pgn rgb2.pgn depth1.png depth2.png\n\n"
		<< "Options:\n"
		<< "-------------------------------------------------------------------\n"
		<< "-n <int>    keep the last time FundamentalMatRansac have matches uper than (default " << min_number_keypoints << " )\n"
		<< "-o <char*>  output filename (default " << outputfilename << " )\n"
		<< "-h          this help\n"
		<< "\n\n";
}

int getSurfMatches(const string& rgb1_filelocation, const string& rgb2_filelocation, vector<KeyPoint>& keypoints1, vector<KeyPoint>& keypoints2,
	vector<DMatch>& matches)
{
	Mat rgb1 = imread(rgb1_filelocation, IMREAD_GRAYSCALE);
	Mat rgb2 = imread(rgb2_filelocation, IMREAD_GRAYSCALE);

	if (rgb1.empty() || rgb2.empty()) {
		cout << "load image filed! Please check rgb image file location!\n\n";
		exit(-1);
	}

	Ptr<SURF> surf;
	surf = SURF::create(800);
	BFMatcher matcher;
	Mat c, d;
	vector<KeyPoint>tmp_key1, tmp_key2;
	vector<DMatch> tmp_matches;

	surf->detectAndCompute(rgb1, Mat(), tmp_key1, c);
	surf->detectAndCompute(rgb2, Mat(), tmp_key2, d);
	matcher.match(c, d, tmp_matches);
	sort(tmp_matches.begin(), tmp_matches.end());

	//������Surf��ƥ����Ϊ������������Ransac�����룬 
	//Ҳ���Խ��˲��ֲ������� ѡ�񲿷ֿ�ǰ��ƥ����л��������Ransac���롣
	//���ǵ���Щʱ���⵽����ȷƥ�������ռ�Ƚ��٣�����ڴ˴�cut��һ���֣�
	//Ȼ�����ò���ȴ�п��ܺ��б�����Ϊ���������ȷƥ�䣬�������ڴ˴�ѡȡ
	//���еĴ�ƥ�䡣
	//int ptsPairs = std::min((int)(matches.size() / times), (int)matches.size());

	int ptsPairs = std::min((int)(tmp_matches.size()), (int)tmp_matches.size());
	for (int i = 0; i < ptsPairs; i++)
		matches.push_back(tmp_matches[i]);


	for (size_t i = 0; i < matches.size(); i++)
	{
		keypoints1.push_back(tmp_key1[matches[i].queryIdx]);
		keypoints2.push_back(tmp_key2[matches[i].trainIdx]);
	}
#ifndef NDEBUG
	cout << "=================================[1]=================================" << endl
		<< "��ѡ�񲿷���ƥ�����Ϊ�� " << tmp_matches.size() << endl;
#endif
	return matches.size();
}

// �ؼ���ת������(�������ͼʹ�ؼ���������Ϣ����z��Ϣ)
int keypoints2cloud(vector<KeyPoint>& keypoints1, pcl::PointCloud<pcl::PointXYZ>::Ptr& keycloud1, vector<KeyPoint>& keypoints2,
	pcl::PointCloud<pcl::PointXYZ>::Ptr& keycloud2)
{
	vector<Point2i> coor_setp1, coor_setp2; // ����floatתint
	Point2i p1, p2;
	for (size_t i = 0; i < keypoints1.size(); i++) {

		p1.x = int(keypoints1[i].pt.x + 0.5);
		p1.y = int(keypoints1[i].pt.y + 0.5);
		p2.x = int(keypoints2[i].pt.x + 0.5);
		p2.y = int(keypoints2[i].pt.y + 0.5);

		coor_setp1.push_back(p1);
		coor_setp2.push_back(p2);

		/*
		cout << i + 1 << "    "
			<< "(" << keypoints1[i].pt.x << ", " << keypoints1[i].pt.y << ")" << "    "
			<< "(" << keypoints2[i].pt.x << ", " << keypoints2[i].pt.y << ")" << "        "
			"(" << coor_setp1[i].x << ", " << coor_setp1[i].y << ")" << "    "
			<< "(" << coor_setp2[i].x << ", " << coor_setp2[i].y << ")" << endl;
		*/
	}

	Mat depth1 = cv::imread(imagelocation_depth1, -1);
	Mat depth2 = cv::imread(imagelocation_depth2, -1);

	if (depth1.empty() || depth2.empty()) {
		cout << "load image filed! Please check rgb image file location!\n\n";
		exit(-1);
	}

	pcl::PointXYZ p_1, p_2;
	for (int i = 0; i < coor_setp1.size(); i++) {

		ushort d_1 = depth1.ptr<ushort>(coor_setp1[i].y)[coor_setp1[i].x];
		ushort d_2 = depth2.ptr<ushort>(coor_setp2[i].y)[coor_setp2[i].x];
		// �޳������������Ϣ�Ĺؼ���
		if (d_1 == 0 || d_2 == 0)
			continue;

		p_1.z = double(d_1) / camera_factor;
		p_1.x = (coor_setp1[i].x - camera_cx) * p_1.z / camera_fx; // �����Ӳ�x����
		p_1.y = (coor_setp1[i].y - camera_cy) * p_1.z / camera_fy; // �����Ӳ�y����
		keycloud1->points.push_back(p_1);


		p_2.z = double(d_2) / camera_factor;
		p_2.x = (coor_setp2[i].x - camera_cx) * p_2.z / camera_fx; // �����Ӳ�x����
		p_2.y = (coor_setp2[i].y - camera_cy) * p_2.z / camera_fy; // �����Ӳ�y����
		keycloud2->points.push_back(p_2);
	}

	keycloud1->height = 1;
	keycloud1->width = keycloud1->points.size();
	keycloud1->is_dense = false; // ϡ��

	keycloud2->height = 1;
	keycloud2->width = keycloud2->points.size();
	keycloud2->is_dense = false;

	cout << "=================================[3]=================================" << endl
			<< "�޳������������Ϣ��ƥ��㣬�޳�����ƴ�СΪ: " << keycloud1->size() << endl;

	return keycloud1->size();
}

// ����rgbͼ������ͼ���ɴ�rgb��Ϣ�ĵ�������, ���ͼ��������λ����Ϣ����ɫͼ����������ɫ��Ϣ
int rgb_depth2cloud(pcl::PointCloud<pcl::PointXYZRGBA>::Ptr& cloud, const string& rgb_filelocation, const string& imagelocation_depth)
{
	Mat rgb = imread(rgb_filelocation);
	Mat depth = cv::imread(imagelocation_depth, -1);
	if (rgb.empty() || depth.empty()) {
		cout << "load image filed! Please check rgb image file location!\n\n";
		exit(-1);
	}

	pcl::PointXYZRGBA p;
	for (int m = 0; m < depth.rows; m++) {
		for (int n = 0; n < depth.cols; n++) {
			ushort d = depth.ptr<ushort>(m)[n]; // opencvָ�뷽ʽ��������
			if (d == 0)
				continue;
			p.z = double(d) / camera_factor;
			p.x = (n - camera_cx) * p.z / camera_fx; // �����Ӳ�x����
			p.y = (m - camera_cy) * p.z / camera_fy; // �����Ӳ�y����
			p.b = rgb.ptr<uchar>(m)[n * 3];
			p.g = rgb.ptr<uchar>(m)[n * 3 + 1];
			p.r = rgb.ptr<uchar>(m)[n * 3 + 2];
			cloud->points.push_back(p);
		}
	}

	cloud->height = 1;
	cloud->width = cloud->points.size();
	cloud->is_dense = false; // ϡ�����

	return cloud->size();
}

// ��ƥ��������У��
int CorrespondenceRejectorSC(pcl::PointCloud<pcl::PointXYZ>::Ptr& keycloud1, pcl::PointCloud<pcl::PointXYZ>::Ptr& keycloud2,
	boost::shared_ptr<pcl::Correspondences>& Correspon_in, boost::shared_ptr<pcl::Correspondences>& Correspon_out)
{
	Correspon_out->clear();
	static int times = 0;
	boost::shared_ptr<pcl::Correspondence> corr1(new pcl::Correspondence);
	if (times == 0)
	{
		for (int i = 0; i < keycloud1->size(); i++)
		{
			corr1->index_query = i;
			corr1->index_match = i;
			Correspon_in->push_back(*corr1);
		}
	}

	//cout << "������άRANC֮ǰ�Ĺؼ�����Ϊ: " << Correspon_in->size() << endl;

	pcl::registration::CorrespondenceRejectorSampleConsensus<pcl::PointXYZ>::Ptr
		ransac_rejector(new pcl::registration::CorrespondenceRejectorSampleConsensus<pcl::PointXYZ>);
	ransac_rejector->setInputSource(keycloud1);
	ransac_rejector->setInputTarget(keycloud2);
	ransac_rejector->getRemainingCorrespondences(*Correspon_in, *Correspon_out);
	Correspon_in->clear();
	cout << "=================================[4]=================================" << endl
				<< "���е�" << ++times << "����άRansac֮��Ĺؼ�����Ϊ: " << Correspon_out->size() << endl;
	return Correspon_out->size();
}

void Transform_use_RTmatrixandSave(pcl::PointCloud<pcl::PointXYZRGBA>::Ptr& sourcecloud, Eigen::Matrix4f& Transform,
	pcl::PointCloud<pcl::PointXYZRGBA>::Ptr& transformed_cloud)
{
	pcl::transformPointCloud(*sourcecloud, *transformed_cloud, Transform); // ͸�ӱ任
	transformed_cloud->height = 1;
	transformed_cloud->width = transformed_cloud->points.size();
	transformed_cloud->is_dense = false; // ϡ�����
	pcl::io::savePLYFile(outputfilename, *transformed_cloud);
	cout << endl << "saved transformedcloud as :" << outputfilename << endl;

}

// �ڿ��ӻ����������������������������ʾ
boost::shared_ptr<pcl::visualization::PCLVisualizer> simpleVis(pcl::PointCloud<pcl::PointXYZRGBA>::ConstPtr transformed_sourcecloud1,
	pcl::PointCloud<pcl::PointXYZRGBA>::ConstPtr sourcecloud2)
{
	boost::shared_ptr<pcl::visualization::PCLVisualizer> viewer(new pcl::visualization::PCLVisualizer("3D Viewer"));
	viewer->setBackgroundColor(0, 0, 0); //���ñ���ɫ
	pcl::visualization::PointCloudColorHandlerRGBField<pcl::PointXYZRGBA> color(transformed_sourcecloud1); //������ɫ(���ֵ��Ƶ�rgbֵ)
	// pcl::visualization::PointCloudColorHandlerCustom<pcl::PointXYZRGBA> color(transformed_sourcecloud1, 0, 255, 0); //������ɫ����һ��ɫ���ã�
	viewer->addPointCloud<pcl::PointXYZRGBA>(transformed_sourcecloud1, color, "transformed_sourcecloud1"); //��ӵ���
	viewer->setPointCloudRenderingProperties(pcl::visualization::PCL_VISUALIZER_POINT_SIZE, 1, "transformed_sourcecloud1"); //������ʾ��Ĵ�С
	pcl::visualization::PointCloudColorHandlerRGBField<pcl::PointXYZRGBA> color2(sourcecloud2); //������ɫ(���ֵ��Ƶ�rgbֵ)
	viewer->addPointCloud<pcl::PointXYZRGBA>(sourcecloud2, color2, "sourcecloud2"); //��ӵ���
	viewer->setPointCloudRenderingProperties(pcl::visualization::PCL_VISUALIZER_POINT_SIZE, 1, "sourcecloud2"); //������ʾ��Ĵ�С
	viewer->initCameraParameters();

	return viewer;
}