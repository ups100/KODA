#include "compression.hpp"

/* just for psnr testing */
void modify(Movie &mov)
{
	for (int frame = 0; frame < mov.get_n_frames(); ++frame) {
		cv::Mat_<cv::Vec3b> img = mov[frame];

		for (int i = 0; i < img.rows; ++i)
			for (int j = 0; j < img.cols; ++j) {
				img(i, j)[1] = 128;
				img(i, j)[2] = 128;
			}
	}

}

void compress_bilin(Movie &org, Movie **result)
{
	Movie *res = new Movie("result");

	org.deep_copy(*res);

	modify(*res);

	*result = res;
}

void compress_soi(Movie &org, Movie **result)
{
	Movie *res = new Movie("result");

	org.deep_copy(*res);

	modify(*res);

	*result = res;
}
