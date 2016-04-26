#include <iostream>
#include <string>
#include <ctime>

#include <opencv2/core/core.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>

#include "movie.hpp"
#include "psnr.hpp"
#include "compression.hpp"

/* Just for testing */
int play_movie(Movie &mov)
{
	cv::Mat frame;

	for (int i = 0; i < mov.get_n_frames(); ++i) {
		cv::cvtColor(mov[i], frame, CV_YUV2BGR);

		cv::imshow(mov.get_title(), frame);
		cv::waitKey(1000/30);
	}

	return 0;
}

void test_compression(Movie &org, void (*compress)(Movie&, Movie **),
		      const std::string& algorithm_name)
{
	Movie *result;;
	time_t time;
	double psnr;

	time = clock();
	compress(org, &result);
	time = clock() - time;

	std::cout << "Algorithm: " << algorithm_name << std::endl;
	std::cout << "Compression time: "
		  << ((double)time * 1000.0)/CLOCKS_PER_SEC << std::endl;

	std::cout << "Org size (in bytes): " << org.get_size() << std::endl;
	std::cout << "Compressed size (in bytes): "
		  << result->get_size() << std::endl;
	std::cout << "Compression ratio: "
		  << (double)org.get_size()/result->get_size() << std::endl;

	psnr  = calculate_psnr(org, *result);
	std::cout << "PSNR: " << psnr << std::endl;

	play_movie(*result);

	delete result;
}

int main(int argc, char *argv[])
{
	Movie org("org");
	int ret;

	if (argc != 2) {
		std::cerr << "Usage: " << argv[0]
		<< " <movie_dir>" << std::endl;
		return -1;
	}

	ret = org.from_dir(argv[1]);
	if (ret) {
		std::cerr << "Unable to load movie" << std::endl;
		return -1;
	}

	/* for debugging: play_movie(org); */

	test_compression(org, compress_bilin, "BILIN");

	test_compression(org, compress_soi, "SOI");

	return 0;
}
