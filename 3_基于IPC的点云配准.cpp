#include <iostream>                 //��׼�������ͷ�ļ�
#include <pcl/io/pcd_io.h>         //I/O����ͷ�ļ�
#include <pcl/point_types.h>        //�����Ͷ���ͷ�ļ�
#include <pcl/registration/icp.h>   //ICP��׼�����ͷ�ļ�

int main()
{
	//��������pcl::PointCloud<pcl::PointXYZ>����ָ�룬����ʼ������
	pcl::PointCloud<pcl::PointXYZ>::Ptr cloud_in (new pcl::PointCloud<pcl::PointXYZ>);
	pcl::PointCloud<pcl::PointXYZ>::Ptr cloud_out(new pcl::PointCloud<pcl::PointXYZ>);

	// ���������
	cloud_in->width = 5;   //���õ��ƿ��
	cloud_in->height = 1;  //���õ���Ϊ�����
	cloud_in->is_dense = false;
	cloud_in->points.resize(cloud_in->width * cloud_in->height);  for (size_t i = 0; i < cloud_in->points.size(); ++i) {
		cloud_in->points[i].x = 1024 * rand() / (RAND_MAX + 1.0f);
		cloud_in->points[i].y = 1024 * rand() / (RAND_MAX + 1.0f);
		cloud_in->points[i].z = 1024 * rand() / (RAND_MAX + 1.0f);
	}

	//��ӡ����������
	std::cout << "Saved " << cloud_in->points.size() << " data points to input:" << std::endl;

	//��ӡ��ʵ������
	for (size_t i = 0; i < cloud_in->points.size (); ++i)
		std::cout << " " << cloud_in->points[i].x
				  << " " << cloud_in->points[i].y
				  << " " << cloud_in->points[i].z << std::endl;

	*cloud_out = *cloud_in;
	std::cout << "size of out point:" << cloud_out->points.size() << std::endl;

	//ʵ��һ���򵥵ĵ��Ƹ���任���Թ���Ŀ�����
	for (size_t i = 0; i < cloud_in->points.size(); ++i)
		cloud_out->points[i].x = cloud_in->points[i].x + 0.7f;

	std::cout << "Transformed " << cloud_in->points.size() << " data points:" << std::endl;

	//��ӡ���������Ŀ�����
	for (size_t i = 0; i < cloud_out->points.size(); ++i)
		std::cout << "    " << cloud_out->points[i].x << " " <<
		cloud_out->points[i].y << " " << cloud_out->points[i].z << std::endl;
	
	pcl::IterativeClosestPoint<pcl::PointXYZ, pcl::PointXYZ> icp;   //��������
	icp.setInputCloud(cloud_in);          //cloud_in����Ϊ���Ƶ�Դ��
	icp.setInputTarget(cloud_out);        //cloud_out����Ϊ��cloud_in��Ӧ��ƥ��Ŀ��
	pcl::PointCloud<pcl::PointXYZ> Final; //�洢������׼�任���ƺ�ĵ���
	icp.align(Final);                     //��ӡ��׼���������Ϣ

	std::cout << "has converged:" << icp.hasConverged() << " score: " << icp.getFitnessScore() << std::endl;
	std::cout << icp.getFinalTransformation() << std::endl;

	return (0);
}

