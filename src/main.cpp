#include <iostream>
#include <string>

#include <opencv2/core/core.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>

#include "movie.hpp"

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

int main(int argc, char *argv[])
{
	Movie org("org"), copy("copy");
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

	play_movie(org);
	return 0;
}
