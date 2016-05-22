#include <cmath>
#include <iostream>

#include <opencv2/core/core.hpp>

#include "psnr.hpp"

double calculate_psnr2(Movie &org, Movie &result)
{
	long long sum = 0;
	long diff;
	double mse;
	double psnr;

	/* Assume that both org and result are the same size */
	for (int frame = 0; frame < org.get_n_frames(); ++frame) {
		cv::Mat_<cv::Vec3b> img = org[frame];
		cv::Mat_<cv::Vec3b> result_img = result[frame];

		for (int i = 0; i < img.rows; ++i)
			for (int j = 0; j < img.cols; ++j) {
				diff = img(i, j)[0] - result_img(i, j)[0];
				sum += (diff * diff);
				/*
				 * For U and V component we scale it from
				 * -128-127 to 0-255
				 */
				for (int c = 1; c < 3; ++c) {
					diff = (img(i, j)[c] + 128)
						- (result_img(i, j)[c] + 128);
					sum += (diff * diff);
				}
			}
	}

	mse = (double)sum / (org[0].rows * org[0].cols * 3 * org.get_n_frames());
	psnr = 10 * log10((255 * 255) / mse);

	return psnr;
}

double calculate_psnr(Movie &org, Movie &result)
{
	long long sum = 0;
	long diff;
	double mse;
	double psnr;

	/* Assume that both org and result are the same size */
	for (int frame = 0; frame < org.get_n_frames(); ++frame) {
		cv::Mat_<cv::Vec3b> img = org[frame];
		cv::Mat_<cv::Vec3s> result_img = result[frame];

		for (int i = 0; i < img.rows; ++i)
			for (int j = 0; j < img.cols; ++j) {
			  diff = (result_img(i, j)[0]);
				sum += (diff * diff);
				/*
				 * For U and V component we scale it from
				 * -128-127 to 0-255
				 */
				for (int c = 1; c < 3; ++c) {
					diff = (result_img(i, j)[c]);
					sum += (diff * diff);
				}
			}
	}

	mse = (double)sum / (org[0].rows * org[0].cols * 3 * org.get_n_frames());
	psnr = 10 * log10((255 * 255) / mse);

	return psnr;
}
