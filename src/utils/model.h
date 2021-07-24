#pragma once
#include <vector>
#include <opencv2/core.hpp>

namespace Utils
{
	class Model
	{
	private:
		std::vector<cv::Vec3f> verts_;
		std::vector<std::vector<int>> faces_;

	public:
		Model(const char *filename);
		~Model();
		int nverts();
		int nfaces();
		cv::Vec3f vert(int i);
		std::vector<int> face(int idx);
	};
}