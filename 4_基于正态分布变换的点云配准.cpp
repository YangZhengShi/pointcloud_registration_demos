/*ʹ����̬�ֲ��任������׼��ʵ�� ������room_scan1.pcd  room_scan2.pcd��Щ���ư���ͬһ����360��ͬ�ӽǵ�ɨ������*/
#include <iostream>
#include <pcl/io/pcd_io.h>
#include <pcl/point_types.h>
#include <pcl/registration/ndt.h>	//NDT(��̬�ֲ�)��׼��ͷ�ļ�
#include <pcl/filters/approximate_voxel_grid.h>   //�˲���ͷ�ļ�  
// ��ʹ��������������������Ч���ȽϺã�
#include <pcl/visualization/pcl_visualizer.h>
#include <boost/thread/thread.hpp>

int main()
{  
	// ���ط���ĵ�һ��ɨ�����������ΪĿ��
	pcl::PointCloud<pcl::PointXYZ>::Ptr target_cloud (new pcl::PointCloud<pcl::PointXYZ>);  

	if (pcl::io::loadPCDFile<pcl::PointXYZ> ("room_scan1.pcd", *target_cloud) == -1)
	{
		PCL_ERROR ("Couldn't read file room_scan1.pcd \n");    
			return (-1);
	}

	std::cout << "Loaded " << target_cloud->size () << " data points from room_scan1.pcd" << std::endl;  

	// ���ش����ӽǵõ��ĵڶ���ɨ�����������ΪԴ����
	pcl::PointCloud<pcl::PointXYZ>::Ptr input_cloud (new pcl::PointCloud<pcl::PointXYZ>);  

	if (pcl::io::loadPCDFile<pcl::PointXYZ> ("room_scan2.pcd", *input_cloud) == -1)
	{
		PCL_ERROR ("Couldn't read file room_scan2.pcd \n");
		return (-1);
	}
	
	std::cout << "Loaded " << input_cloud->size () << " data points from room_scan2.pcd" << std::endl;  

	// ���ϵĴ������������PCD�ļ��õ�����ָ�룬������׼����ɶ�Դ���Ƶ�Ŀ����ƵĲο�����ϵ�ı任����Ĺ��ƣ��õ��ڶ�����Ʊ任����һ���������ϵ�µı任����  
	// �������ɨ��������ݹ��˵�ԭʼ�ߴ��10%�����ƥ����ٶȣ�ֻ��Դ���ƽ����˲�������������������Ŀ����Ʋ���Ҫ�˲�����  
	// ��Ϊ��NDT�㷨����Ŀ����ƶ�Ӧ�������������ݽṹ��ͳ�Ƽ��㲻ʹ�õ����㣬����ʹ�ð�����ÿ�����ص�Ԫ���еĵ��ͳ������
	pcl::PointCloud<pcl::PointXYZ>::Ptr filtered_cloud (new pcl::PointCloud<pcl::PointXYZ>);
	pcl::ApproximateVoxelGrid<pcl::PointXYZ> approximate_voxel_filter;
	approximate_voxel_filter.setLeafSize (0.2, 0.2, 0.2);
	approximate_voxel_filter.setInputCloud (input_cloud);

	approximate_voxel_filter.filter (*filtered_cloud);
	std::cout << "Filtered cloud contains " << filtered_cloud->size ()  << " data points from room_scan2.pcd" << std::endl;  

	// ��ʼ����̬�ֲ�(NDT)����
	pcl::NormalDistributionsTransform<pcl::PointXYZ, pcl::PointXYZ> ndt;  
	// �����������ݵĳ߶�����NDT��ز���
	ndt.setTransformationEpsilon (0.01);   //Ϊ��ֹ����������Сת������  
	ndt.setStepSize (0.1);    //Ϊmore-thuente������������󲽳�
	ndt.setResolution (1.0);   //����NDT��������ṹ�ķֱ��ʣ�voxelgridcovariance��  //���ϲ�����ʹ�÷���ߴ����������ȽϺã����������Ҫ��������һ�����ȱ��ӵ�ɨ��֮���С�����壬��Ҫ�Բ������кܴ�̶ȵ���С  
	//����ƥ�������������������������Ƴ������е�������������һ����˵�������ֵ֮ǰ�Ż��������epsilon�任��ֵ����ֹ  
	//������������������ܹ����ӳ����³������ֹ�����ڴ���ķ���������ʱ�����  
	ndt.setMaximumIterations (35);
	ndt.setInputSource (filtered_cloud);  //Դ���� 
	ndt.setInputTarget (target_cloud);  //Ŀ����� 
	
	// ����ʹ�û����˲�෨�õ��Ĵ��Գ�ʼ�任������
	Eigen::AngleAxisf init_rotation (0.6931, Eigen::Vector3f::UnitZ ());
	Eigen::Translation3f init_translation (1.79387, 0.720047, 0);
	Eigen::Matrix4f init_guess = (init_translation * init_rotation).matrix ();  // ������Ҫ�ĸ���任�Ա㽫�����Դ����ƥ�䵽Ŀ�����
	pcl::PointCloud<pcl::PointXYZ>::Ptr output_cloud (new pcl::PointCloud<pcl::PointXYZ>);
	ndt.align (*output_cloud, init_guess);   //����ط���output_cloud������Ϊ���յ�Դ���Ʊ任����Ϊ����Ե��ƽ������˲�����
	std::cout << "Normal Distributions Transform has converged:" << ndt.hasConverged () << " score: " << ndt.getFitnessScore () << std::endl; 
	
	// ʹ�ô����ı任��Ϊ���˵�������ƽ��б任
	pcl::transformPointCloud (*input_cloud, *output_cloud, ndt.getFinalTransformation ());  // ����ת�����Դ������Ϊ���յı任���
	pcl::io::savePCDFileASCII ("room_scan2_transformed.pcd", *output_cloud);  

	// ��ʼ�����ƿ��ӻ�����
	boost::shared_ptr<pcl::visualization::PCLVisualizer>
	viewer_final (new pcl::visualization::PCLVisualizer ("3D Viewer"));
	viewer_final->setBackgroundColor (0, 0, 0);  //���ñ�����ɫΪ��ɫ
	// ��Ŀ�������ɫ���ӻ� (red).
	pcl::visualization::PointCloudColorHandlerCustom<pcl::PointXYZ>
	target_color (target_cloud, 255, 0, 0);
	viewer_final->addPointCloud<pcl::PointXYZ> (target_cloud, target_color, "target cloud");
	viewer_final->setPointCloudRenderingProperties (pcl::visualization::PCL_VISUALIZER_POINT_SIZE, 1, "target cloud"); 
	// ��ת�����Դ������ɫ (green)���ӻ�.
	pcl::visualization::PointCloudColorHandlerCustom<pcl::PointXYZ>
	output_color (output_cloud, 0, 255, 0);
	viewer_final->addPointCloud<pcl::PointXYZ> (output_cloud, output_color, "output cloud");
	viewer_final->setPointCloudRenderingProperties (pcl::visualization::PCL_VISUALIZER_POINT_SIZE, 1, "output cloud"); //�������ӻ�
	viewer_final->addCoordinateSystem (1.0);  //��ʾXYZָʾ��
	viewer_final->initCameraParameters ();   //��ʼ������ͷ����  

	// �ȴ�ֱ�����ӻ����ڹر�
	while (!viewer_final->wasStopped ())
	{
		viewer_final->spinOnce (100);
		boost::this_thread::sleep (boost::posix_time::microseconds (100000));
	}

	return (0);
}