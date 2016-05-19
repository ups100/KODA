#include "compression.hpp"

/* just for psnr testing */


/*
 *
 * tylko previous jest interpolowane
 *
 */

static int micro_frame_num = 0;
static int W = 32;
static int N = 16;
static int I = 1; //interpolation

struct Vector_ {
  Vector_(int a = 0, int b = 0, int c = 1) : x(a), y(b), divider(c){}
  int x;
  int y;
  int divider;
};

unsigned long long int calculate_error (cv::Mat_<cv::Vec3b>& curr, int c, int d, cv::Mat_<cv::Vec3b>& previous_interpolated, int x, int y, int inter_ratio) {
	unsigned long long int err = 0;
	for (int i = c,k = x; i < c + N && i < curr.rows; i++, k += inter_ratio)
		for (int j = c,l = y; j < d + N && j < curr.cols; j++, l += inter_ratio) {
			err += std::abs(curr(i,j)[0] - previous_interpolated(k,l)[0]);
			err += std::abs(curr(i,j)[1] - previous_interpolated(k,l)[1]);
			err += std::abs(curr(i,j)[2] - previous_interpolated(k,l)[2]);
		}
	return err;
}

void find_(cv::Mat_<cv::Vec3b>& last, cv::Mat_<cv::Vec3b>& curr, cv::Mat_<cv::Vec3s>& displacement, int x, int y, int inter_ratio = 1) {
	unsigned long long int curr_err = -1;
	Vector_ vec;

	for (int i = (x - W) * inter_ratio;
	     i < (x + W) * inter_ratio && i < curr.rows*inter_ratio;
	     i++) {
		if (i < 0)
			continue;
		for (int j = (y - W) * inter_ratio;
		     j < (y + W) * inter_ratio && j < curr.cols*inter_ratio;
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

	for (int i = x, k = vec.x; i < x + N && i < curr.rows; i++, k += inter_ratio)
		for (int j = y, l = vec.y; j < y + N && j < curr.cols; j++, l += inter_ratio) {
			displacement(i,j)[0] = (int) last(k,l)[0] - curr(i,j)[0];
			displacement(i,j)[1] = (int) last(k,l)[1] - curr(i,j)[1];
			displacement(i,j)[2] = (int) last(k,l)[2] - curr(i,j)[2];
		}
}

cv::Mat_<cv::Vec3b> interpolate(cv::Mat_<cv::Vec3b>& img, int type)
{
	cv::Mat_<cv::Vec3b> interpolated = cv::Mat_<cv::Vec3b>(img.rows*I, img.cols*I, cv::Vec3b(0,0,0));
	for (int  i =0; i<img.rows-1; i++)
		for (int j = 0; j < img.cols - 1; j++) {
			for (int k = 0; k <3; k++) {
				interpolated(i,j)[k] = img(i,j)[k];
				interpolated(i,j + I)[k] = img(i,j+1)[k];
				interpolated(i + I,j + I)[k] = img(i+1,j+1)[k];
				interpolated(i+I,j)[k] = img(i+1,j)[k];
				for (int r = 1; r < I; r++) {
					interpolated(i,j+r)[k] = ((j+I-(j+r))*img(i,j)[k])/(I) + (((j+r)-j)*img(i,j+1)[k])/(I);
					interpolated(i+r,j)[k] = ((i+I-(i+r))*img(i,j)[k])/(I) + (((i+r)-i)*img(i+1,j)[k])/(I);
					interpolated(i+r,j+I)[k] = ((i+I-(i+r))*img(i,j+1)[k])/(I) + (((i+r)-i)*img(i+1,j+1)[k])/(I);
					interpolated(i+I,j + r)[k] = ((j+I-(j+r))*img(i+1,j)[k])/(I) + (((j+r)-j)*img(i+1,j+1)[k])/(I);
					
				}
				for (int rx = 1; rx < I; rx++)
					for (int ry = 1; ry < I;ry++) {
						interpolated(i +rx, j+ry)[k] = ((j+I-(j+ry))*interpolated(i+rx,j)[k])/(I) + (((j+ry)-j)*interpolated(i,j+I)[k])/(I);
					}
			}
		}

	return interpolated;
}

void modify(Movie &mov, int type = 0)
{
	cv::Mat_<cv::Vec3b> last; //inicjalizowac jakos
	last = cv::Mat_<cv::Vec3b>(mov[0].rows*I, mov[0].cols*I, cv::Vec3b(0,0,0));
	
	for (int frame = 0; frame < mov.get_n_frames(); ++frame) {
		cv::Mat_<cv::Vec3b> img = mov[frame];
		int orig_rows = img.rows;
		int orig_cols = img.cols;
		cv::Mat_<cv::Vec3s> displacement = cv::Mat_<cv::Vec3s>(orig_rows, orig_cols, cv::Vec3s(0,0,0));
		cv::Mat_<cv::Vec3b> img_inter = interpolate(img, type); // none interpolation

		for (int i = 0; i < orig_rows; i+=N)
			for (int j = 0; j < orig_cols; j+=N) {
				find_(last, img, displacement, i, j, I);
			}
		mov[frame] = displacement;
		last = img_inter;
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

	modify(*res);

	*result = res;
}
