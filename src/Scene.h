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

struct Image {
    int id;
    int camera_id;
    std::string name;
    std::vector<double> qvec; // quaternion
    std::vector<double> tvec; // translation
    std::vector<std::pair<int, int>> point2D_idxs; // (point3D_id, idx)
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

    // Relationship helpers
    std::vector<int> GetImagePointIDs(int image_id) const;
    std::vector<int> GetPointImageIDs(int point_id) const;

    void DeletePoints(std::vector<int>& selected);

private:
    std::map<int, Camera> cameras_;
    std::map<int, Image> images_;
    std::map<int, Point3D> points_;
};
