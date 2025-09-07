#include "Scene.h"
#include <fstream>
#include <sstream>

bool Scene::Import(const std::string& points_path, const std::string& cameras_path, const std::string& images_path) {
	// Parse cameras.txt
	std::ifstream cam_file(cameras_path);
	if (!cam_file.is_open()) return false;
	std::string line;
	while (std::getline(cam_file, line)) {
		if (line.empty() || line[0] == '#') continue;
		std::istringstream iss(line);
		Camera cam;
		iss >> cam.id >> cam.model >> cam.width >> cam.height;
		double param;
		while (iss >> param) cam.params.push_back(param);
		cameras_[cam.id] = cam;
	}
	cam_file.close();

	// Parse images.txt
	std::ifstream img_file(images_path);
	if (!img_file.is_open()) return false;
	while (std::getline(img_file, line)) {
		if (line.empty() || line[0] == '#') continue;
		std::istringstream iss(line);
		Image img;
		iss >> img.id;
		img.qvec.resize(4); img.tvec.resize(3);
		for (int i = 0; i < 4; ++i) iss >> img.qvec[i];
		for (int i = 0; i < 3; ++i) iss >> img.tvec[i];
		iss >> img.camera_id >> img.name;
		// Next line: 2D points (optional, skip for now)
		std::getline(img_file, line);
		images_[img.id] = img;

	}
	img_file.close();

	// Parse points3D.txt
	std::ifstream pt_file(points_path);
	if (!pt_file.is_open()) return false;
	while (std::getline(pt_file, line)) {
		if (line.empty() || line[0] == '#') continue;
		std::istringstream iss(line);
		Point3D pt;
		iss >> pt.id >> pt.x >> pt.y >> pt.z;
		pt.color.resize(3);
		int r, g, b;
		iss >> r >> g >> b;
		pt.color[0] = static_cast<unsigned char>(r);
		pt.color[1] = static_cast<unsigned char>(g);
		pt.color[2] = static_cast<unsigned char>(b);
		iss >> pt.error;
		int track_id;
		while (iss >> track_id) pt.track.push_back(track_id);
		points_[pt.id] = pt;
	}
	pt_file.close();
	return true;
}

bool Scene::Export(const std::string& points_path, const std::string& cameras_path, const std::string& images_path) const {
	// Write cameras.txt
	std::ofstream cam_file(cameras_path);
	if (!cam_file.is_open()) return false;
	cam_file << "# Camera list with one line of data per camera:\n";
	cam_file << "#   CAMERA_ID, MODEL, WIDTH, HEIGHT, PARAMS\n";
	for (auto it = cameras_.begin(); it != cameras_.end(); ++it) {
		const Camera& cam = it->second;
		cam_file << cam.id << " " << cam.model << " " << cam.width << " " << cam.height;
		for (size_t i = 0; i < cam.params.size(); ++i) cam_file << " " << cam.params[i];
		cam_file << "\n";
	}
	cam_file.close();

	// Write images.txt
	std::ofstream img_file(images_path);
	if (!img_file.is_open()) return false;
	img_file << "# Image list with one line of data per image:\n";
	img_file << "#   IMAGE_ID, QVEC (qw, qx, qy, qz), TVEC (tx, ty, tz), CAMERA_ID, NAME\n";
	for (auto it = images_.begin(); it != images_.end(); ++it) {
		const Image& img = it->second;
		img_file << img.id;
		for (size_t i = 0; i < img.qvec.size(); ++i) img_file << " " << img.qvec[i];
		for (size_t i = 0; i < img.tvec.size(); ++i) img_file << " " << img.tvec[i];
		img_file << " " << img.camera_id << " " << img.name << "\n";
		// 2D points not written for simplicity
		img_file << "\n";
	}
	img_file.close();

	// Write points3D.txt
	std::ofstream pt_file(points_path);
	if (!pt_file.is_open()) return false;
	pt_file << "# 3D point list with one line of data per point:\n";
	pt_file << "#   POINT3D_ID, X, Y, Z, R, G, B, ERROR, TRACK[]\n";
	for (auto it = points_.begin(); it != points_.end(); ++it) {
		const Point3D& pt = it->second;
		pt_file << pt.id << " " << pt.x << " " << pt.y << " " << pt.z << " "
				<< static_cast<int>(pt.color[0]) << " " << static_cast<int>(pt.color[1]) << " " << static_cast<int>(pt.color[2]) << " "
				<< pt.error;
		for (size_t i = 0; i < pt.track.size(); ++i) pt_file << " " << pt.track[i];
		pt_file << "\n";
	}
	pt_file.close();
	return true;
}

std::vector<int> Scene::GetImagePointIDs(int image_id) const {
	std::vector<int> ids;
	auto it = images_.find(image_id);
	if (it != images_.end()) {
		for (const auto& pair : it->second.point2D_idxs) {
			ids.push_back(pair.first);
		}
	}
	return ids;
}

std::vector<int> Scene::GetPointImageIDs(int point_id) const {
	std::vector<int> ids;
	auto it = points_.find(point_id);
	if (it != points_.end()) {
		ids = it->second.track;
	}
	return ids;
}


void Scene::DeletePoints(std::vector<int>& selected)
{
	auto it = points_.begin();
	int currentIndex = 0;
	int selIdx = 0;

	while (it != points_.end() && selIdx < selected.size()) {
		if (currentIndex == selected[selIdx]) {
			// Erase returns iterator to the next element
			it = points_.erase(it);
			++selIdx; // move to next selected index
		}
		else {
			++it;
		}
		++currentIndex;
	}
}