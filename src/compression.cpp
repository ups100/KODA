#include "compression.hpp"
#include <iostream>
#include <opencv2/core/core.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>
/* just for psnr testing */


static int micro_frame_num = 0;
static int W = 8;
static int N = 8;
static int I = 4; //interpolation

struct Vector_ {
	Vector_(int a = 0, int b = 0, int c = 1) : x(a), y(b), divider(c){}
	int x;
	int y;
	int divider;
};

unsigned long long int calculate_error (cv::Mat_<cv::Vec3b>& curr, int c, int d, cv::Mat_<cv::Vec3b>& previous_interpolated, int x, int y, int inter_ratio) {
	unsigned long long int err = 0;
	for (int i = c,k = x; i < c + N && i < curr.rows; i++, k += inter_ratio)
		for (int j = d,l = y; j < d + N && j < curr.cols; j++, l += inter_ratio) {
			int q;
			q = std::abs(curr(i,j)[0] - ((int)previous_interpolated(k,l)[0]));
			err += q*q;
			q = std::abs(curr(i,j)[1] - ((int)previous_interpolated(k,l)[1]));
			err += q*q;
			q = std::abs(curr(i,j)[2] - ((int)previous_interpolated(k,l)[2]));
			err += q*q;
		}
	return err;
}

void find_(cv::Mat_<cv::Vec3b>& last, cv::Mat_<cv::Vec3b>& curr, cv::Mat_<cv::Vec3s>& displacement, int x, int y, int inter_ratio = 1) {
	unsigned long long int curr_err = -1;
	Vector_ vec;

	for (int i = ((x - W) < 0 ? 0 : (x-W)  )* inter_ratio;
	     i < (x + W) * inter_ratio && i+N*inter_ratio < curr.rows*inter_ratio;
	     i++) {
		if (i < 0)
			continue;
		for (int j = ((y - W) <0 ?0 :(y-W)) * inter_ratio;
		     j < (y + W) * inter_ratio && j+N*inter_ratio < curr.cols*inter_ratio;
		     j++) {
			if (j < 0)
				continue;
			unsigned long long int err = calculate_error(curr, x, y, last,i, j, inter_ratio);
			if (err  < curr_err) {
				curr_err = err;
				vec.x = i;
				vec.y = j;
				vec.divider = inter_ratio;
			}
		}
	}
//	std::cout << "vector: "<< vec.x- x*I <<" " << vec.y-y*I<< std::endl;
	for (int i = x, k = vec.x; i < x + N && i < curr.rows; i++, k += inter_ratio)
		for (int j = y, l = vec.y; j < y + N && j < curr.cols; j++, l += inter_ratio) {
			displacement(i,j)[0] = ((int) last(k,l)[0]) - curr(i,j)[0];
			displacement(i,j)[1] = ((int) last(k,l)[1]) - curr(i,j)[1];
			displacement(i,j)[2] = ((int) last(k,l)[2]) - curr(i,j)[2];
		}
}

cv::Vec3b interpolation_calc_vec(int x, int x1, cv::Vec3b &Q1, int x2, cv::Vec3b &Q2)
{
	cv::Vec3b result;
	int diff1, diff2;

	diff1 = x2 - x;
	diff2 = x - x1;

	result[0] = (diff1 * Q1[0] + diff2 * Q2[0])/I;
	result[1] = (diff1 * Q1[1] + diff2 * Q2[1])/I;
	result[2] = (diff1 * Q1[2] + diff2 * Q2[2])/I;

	return result;
}

cv::Mat_<cv::Vec3b> interpolate2(cv::Mat_<cv::Vec3b> img, int type)
{
	cv::Mat_<cv::Vec3b> interpolated = cv::Mat_<cv::Vec3b>((img.rows-1)*I+1, (img.cols-1)*I+1, cv::Vec3b(0,-1,-1));
	cv::Vec3b curr;
	cv::Vec3b prev;

	for (int i = 0; i < img.rows; i++) {
		interpolated(i*I,0) = img(i,0);
		for (int j = (i -1)*I+1; j >= 0 && j < i*I;j++)
			interpolated(j, 0) = interpolation_calc_vec(j, (i-1)*I, img(i-1,0), i*I, img(i,0));
	}

	for (int i = 0; i < img.cols; i++) {
		interpolated(0,i*I) = img(0,i);
		for (int j = (i -1)*I+1; j >= 0 && j < i*I;j++)
			interpolated(0,j) = interpolation_calc_vec(j,(i-1)*I, img(0,i-1), i*I, img(0,i));
	}

	for (int  i = 1; i < img.rows; i++)
		for (int j = 1; j < img.cols; j++) {
			int curr_col = j * I;
			int curr_row = i * I;
			int prev_col = curr_col;
			int prev_row = (i - 1) * I;

			curr = interpolated(curr_row, curr_col) = img(i, j);
			prev = interpolated(prev_row, prev_col);

			/* Up */
			for (int y = curr_row - 1; y > prev_row; y--)
				interpolated(y, curr_col) = interpolation_calc_vec(y, prev_row, prev, curr_row, curr);
			
			/* left */
			prev_row = curr_row;
			prev_col = (j - 1) * I;
			prev = interpolated(prev_row, prev_col);
			for (int x = curr_col - 1;  x > prev_col; x--) {
				interpolated(curr_row, x) = interpolation_calc_vec(x, prev_col, prev, curr_col, curr);
			}

			/* rest */
			prev_row = (i - 1) * I;
			for (int y = curr_row - 1; y > prev_row; y--)
				for (int x = curr_col - 1; x > prev_col; x--)
					interpolated(y, x) = interpolation_calc_vec(x, prev_col, interpolated(y, prev_col), curr_col, interpolated(y, curr_col));
		}

	return interpolated;
}

void modify(Movie &mov, int type = 0)
{
	cv::Mat_<cv::Vec3b> last;
	time_t time;

	last = cv::Mat_<cv::Vec3b>(mov[0].rows*I, mov[0].cols*I, cv::Vec3b(0,0,0));
	
	for (int frame = 0; frame < mov.get_n_frames(); ++frame) {
		cv::Mat_<cv::Vec3b> img = mov[frame];
		int orig_rows = img.rows;
		int orig_cols = img.cols;
		cv::Mat_<cv::Vec3s> displacement = cv::Mat_<cv::Vec3s>(orig_rows, orig_cols, cv::Vec3s(0,0,0));
		std::cout<<"frame: "<< frame<< std::endl;
		cv::Mat_<cv::Vec3b> img_inter = interpolate2(img, type);
		std::cout<<"Interpolated "<<std::endl;
		for (int i = 0; i < orig_rows; i+=N)
			for (int j = 0; j < orig_cols; j+=N)
				find_(last, img, displacement, i, j, I);

		mov[frame] = displacement;
		img_inter.copyTo(last);
	}

}

void compress_bilin(Movie &org, Movie **result)
{
	Movie *res = new Movie("result");
	
	org.deep_copy(*res);

	modify(*res, 0);

	*result = res;
}

void compress_soi(Movie &org, Movie **result)
{
	Movie *res = new Movie("result");

	org.deep_copy(*res);

	modify(*res, 1);

	*result = res;
}
