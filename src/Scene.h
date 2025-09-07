#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include <map>
#include <memory>

struct Camera {
    int id;
    std::string model;
    int width, height;
    std::vector<double> params;
};

struct ImagePoint2D {
    double x;
    double y;
    int point3D_id; // -1 if no associated 3D point
};

struct Image {
    int id;
    int camera_id;
    std::string name;
    std::vector<double> qvec; // quaternion
    std::vector<double> tvec; // translation
    std::vector<ImagePoint2D> points2D; // (point3D_id, idx)
};

struct Point3D {
    int id;
    double x, y, z;
    std::vector<unsigned char> color; // RGB
    double error;
    std::vector<int> track; // image ids
};

class Scene {
public:
    bool Import(const std::string& points_path, const std::string& cameras_path, const std::string& images_path);
    bool Export(const std::string& points_path, const std::string& cameras_path, const std::string& images_path) const;

    const std::map<int, Camera>& GetCameras() const { return cameras_; }
    const std::map<int, Image>& GetImages() const { return images_; }
    const std::map<int, Point3D>& GetPoints() const { return points_; }

    void DeletePoints(std::vector<int>& selected);

private:
    std::map<int, Camera> cameras_;
    std::map<int, Image> images_;
    std::map<int, Point3D> points_;
};
