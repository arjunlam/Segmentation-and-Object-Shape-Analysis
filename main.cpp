#include <opencv2/core/core.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>

#include <string>
#include <vector>
#include <algorithm>
#include <iostream>
#include <random>

#include "standard_util.h"
#include "threshold.h"
#include "components.h"
#include "object_analysis.h"
#include "morphology.h"

// TODO
// Make enum class
const int InvalidData = -1;
const int AquariumData = 0;
const int BatData = 1;
const int CellData = 2;
const int PianoData = 3;

const std::vector<const std::string> DataSets {
	"aquarium",
	"bat",
	"cell",
	"piano"
};

void rgb2greyscale(const cv::Mat &src, cv::Mat &dst);

int main(int argc, char** argv) {

	if (argc != 3) exit(1);

	cv::String data_set = cv::String(argv[1]).toLowerCase();
	cv::String directory(argv[2]);

	int mode = InvalidData;

	std::vector<cv::String> src_files;
	std::vector<const cv::Mat> src_images;

	std::vector<cv::Vec3b> label_colors;
	label_colors.resize(11000);

	std::mt19937 rng;
	rng.seed(std::random_device()());
	std::uniform_int_distribution<std::mt19937::result_type> dist(0, 255);

	label_colors[0] = cv::Vec3b(0, 0, 0);
	for (ushort l = 1; l < label_colors.size(); ++l) {
		label_colors[l] = cv::Vec3b(dist(rng), dist(rng), dist(rng));
	}


	int duration = timeit([&]() {
		for (int i = 0; i < DataSets.size(); ++i) {
			if (DataSets[i].compare(data_set) == 0) {
				cv::glob(directory, src_files);
				src_images.resize(src_files.size());
				std::transform(src_files.begin(), src_files.end(), src_images.begin(), [](cv::String &file_name) {return cv::imread(file_name, CV_8UC3); });
				mode = i;
				break;
			}
		}
	});

	switch (mode) {
	case AquariumData:
		// aquarium proccessing loop
		break;
	case BatData:
		// bat processing loop
		break;
	case CellData:
		// cell data processing loop
		break;
	default:
		exit(1);
		break;
	}

	cv::Size2i size = src_images[0].size();
	cv::Mat grey_image(size, CV_8UC1);

	cv::Mat binary_image(size, CV_8UC1);
	cv::Mat labeled_image(size, CV_16UC1);
	cv::Mat segmented_image(size, CV_8UC3);
	
	std::vector<ushort> label_vector;
	std::vector<ushort> relabel_vector;

	ushort labels;
	std::vector<cv::Rect2i> bounds_vector;
	std::vector<int> area_vector;

	while (true) {

		for (int f = 0; f < src_images.size(); ++f) {

			const cv::Mat &src = src_images[f];

			std::cout << "frame start" << std::endl;

			auto frame_length = timeit([&]() {

				rgb2greyscale(src, grey_image);

				//binaryThreshold(src, binary_image, 125, 1, 75);
				adaptiveThreshold(src, binary_image);

				dilation(binary_image, binary_image, 2, cv::MORPH_ELLIPSE);

				segmented_image = 0;

				iterative_connected_components(binary_image, labeled_image, label_vector);
				labels = label_vector.size();

				condense_labels(label_vector, labeled_image, labeled_image);

				calculate_bounds(labeled_image, labels, bounds_vector);
				calcualte_areas(labeled_image, labels, area_vector);

				filter_labels(labeled_image, labels, labeled_image, relabel_vector, [&](const ushort &label) {
					return (area_vector[label] > 50) && (area_vector[label] < 500);
				});

				colorize_components(labeled_image, label_colors, segmented_image);

				for (auto label : relabel_vector) {
					cv::rectangle(segmented_image, bounds_vector[label], cv::Scalar(255, 0, 0), 1);
				}

				std::cout << "\tfound " << relabel_vector.size() << " components" << std::endl;

				cv::imshow("source", src);
				cv::imshow("segmented", segmented_image);

				cv::waitKey(1);
			});
			std::cout << "\tframe took " << frame_length << " ms" << std::endl;

			std::cin.clear();
			std::cin.get();
		}
	}
}

void rgb2greyscale(const cv::Mat &src, cv::Mat &dst) {

	static const float div = 1.0f / 3.0f;

	for (size_t r = 0; r < src.rows; ++r) {
		const cv::Vec3b *src_row = src.ptr<cv::Vec3b>(r);
		uchar *dst_row = dst.ptr<uchar>(r);

		for (size_t c = 0; c < src.cols; ++c) {
			const cv::Vec3b &src_pixel = src_row[c];
			uchar &dst_pixel = dst_row[c];

			dst_pixel = div * src_pixel[0] + div * src_pixel[1] + div * src_pixel[2];
		}
	}
}