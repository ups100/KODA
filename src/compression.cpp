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

unsigned long long int calculate_error(cv::Mat_<cv::Vec3b>& curr, int c, int d,
				       cv::Mat_<cv::Vec3b>& previous_interpolated,
				       int x, int y, int inter_ratio)
{
	unsigned long long int err = 0;
	uchar *curr_vec;
	uchar *prev_vec;

	for (int i = c, k = x;
	     i < c + N && i < curr.rows;
	     i++, k += inter_ratio)
		for (int j = d,l = y;
		     j < d + N && j < curr.cols;
		     j++, l += inter_ratio) {
			int q;
			curr_vec = curr(i,j).val;
			prev_vec = previous_interpolated(k,l).val;

			q = curr_vec[0] - (int)prev_vec[0];
			err += q*q;
			q = curr_vec[1] - (int)prev_vec[1];
			err += q*q;
			q = curr_vec[2] - (int)prev_vec[2];
			err += q*q;
		}
	return err;
}

void find_(cv::Mat_<cv::Vec3b> &last, cv::Mat_<cv::Vec3b> &curr, cv::Mat_<cv::Vec3s> &displacement, int x, int y, int inter_ratio = 1)
{
	unsigned long long int curr_err = -1;
	uchar *curr_vec;
	uchar *last_vec;
	short *disp;
	Vector_ vec;

	int start_i = ((x - W) < 0 ? 0 : (x - W)) * inter_ratio;
	int start_j = ((y - W) < 0 ? 0 : (y - W)) * inter_ratio;

	for (int i = start_i;
	     i < (x + W) * inter_ratio && i+N*inter_ratio < curr.rows*inter_ratio;
	     i++) {
		for (int j = start_j;
		     j < (y + W) * inter_ratio && j+N*inter_ratio < curr.cols*inter_ratio;
		     j++) {
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
			disp = displacement(i, j).val;
			last_vec = last(k, l).val;
			curr_vec = curr(i, j).val;
			disp[0] = (int)last_vec[0] - curr_vec[0];
			disp[1] = (int)last_vec[1] - curr_vec[1];
			disp[2] = (int)last_vec[2] - curr_vec[2];
		}
}

cv::Vec3b interpolation_calc_vec(int x, int x1, cv::Vec3b &Q1, int x2, cv::Vec3b &Q2)
{
	cv::Vec3b result;
	int diff1, diff2;

	diff1 = x2 - x;
	diff2 = x - x1;

	result.val[0] = (diff1 * Q1.val[0] + diff2 * Q2.val[0])/I;
	result.val[1] = (diff1 * Q1.val[1] + diff2 * Q2.val[1])/I;
	result.val[2] = (diff1 * Q1.val[2] + diff2 * Q2.val[2])/I;

	return result;
}

cv::Mat_<cv::Vec3b> interpolate2(cv::Mat_<cv::Vec3b> &img)
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

cv::Vec3s& operator+(cv::Vec3s& a, short b)
{
	a.val[0] += b;
	a.val[1] += b;
	a.val[2] += b;

	return a;
}

static inline cv::Vec3b interpolate_mean(cv::Vec3b &a, cv::Vec3b &b)
{
	cv::Vec3s result = a;

	result += b;

	return cv::Vec3b((result + 1)/2);
}

cv::Mat_<cv::Vec3b> interpolate264(cv::Mat_<cv::Vec3b> &img)
{
	cv::Mat_<cv::Vec3b> interpolated = cv::Mat_<cv::Vec3b>((img.rows-1)*I+1, (img.cols-1)*I+1, cv::Vec3b(0,-1,-1));
	int x;
	int y;
	int step;
	int small_step;
	cv::Vec3b tmp;
	cv::Vec3s val;
	cv::Vec3b tmp_curr;
	cv::Vec3s tmp_val;

	assert(I <= 4);

	if (I == 1)
		return img.clone();

	step = I;
	small_step = I/2;

	for (int i = 0, y = 0; i < img.rows; y += step, ++i) {
		tmp = img(i, 0);
		for (int j = 0, x = 0; j < img.cols; x += small_step)
			if (x % step == 0) {
				interpolated(y, x) = tmp;
				if (x != 0 && small_step != 1)
					interpolated(y, x - 1) = interpolate_mean(interpolated(y, x - 2), tmp);
				++j;
			} else if (x % step == small_step) {
				/* previous and next are allways valid */
				int sum = 40;
				/* previous orig */
				val = tmp;
				/* next orig */
				tmp = img(i, j);
				val += tmp;
				val *= 20;

				/* Zero the value */
				tmp_val *= 0;
				/* is there sth on a left side */
				if (j - 2  >= 0) {
					tmp_val += img(i, j - 2);
					sum -= 5;
					if (j - 3 >= 0) {
						val += img(i, j - 3);
						sum += 1;
					}

				}
				/* is there sth on a left side */
				if (j + 1 < img.cols) {
					tmp_val += img(i, j + 1);
					sum -= 5;
					if (j + 2 < img.cols) {
						val += img(i, j + 2);
						sum += 1;
					}
							
				}

				val -= tmp_val * 5;
				tmp_curr = val = (val + sum/2)/sum;
				interpolated(y, x) = val;
				if (small_step != 1)
					interpolated(y, x - 1) = interpolate_mean(interpolated(y, x - 2), tmp_curr);
			}
	}

	for (y = small_step; y < interpolated.rows; y += step) {
		for (x = 0; x < interpolated.cols; x += small_step) {
			/* previous and next are allways valid */
			int sum = 40;
			/* previous orig */
			val = interpolated(y - small_step, x);
			/* next orig */
			val += interpolated(y + small_step, x);
			val *= 20;

			/* Zero the value */
			tmp_val *= 0;
			/* is there sth up */
			if (y - step - small_step >= 0) {
				tmp_val += interpolated(y - step - small_step, x);
				sum -= 5;
				if (y - 2*step - small_step >= 0) {
					val += interpolated(y - 2*step - small_step, x);
					sum += 1;
				}

			}
			/* is there sth down */
			if (y + step + small_step < interpolated.rows) {
				tmp_val += interpolated(y + step + small_step, x);
				sum -= 5;
				if (y + 2*step + small_step < interpolated.rows) {
					val += interpolated(y + 2*step + small_step, x);
					sum += 1;
				}
			}

			val -= tmp_val * 5;
			val = (val + sum/2)/sum;
			tmp_curr = interpolated(y, x) = val;

			/* When I == 1 we don't calculate the means */
			if (small_step == 1)
				continue;

			/* prev column mean */
			if (x != 0)
				interpolated(y, x - 1) = interpolate_mean(interpolated(y, x - 2) , tmp_curr);

			/* up */
			interpolated(y - 1, x) = interpolate_mean(interpolated(y - 2, x) , tmp_curr);
			/* down */
			interpolated(y + 1, x) = interpolate_mean(interpolated(y + 2, x) , tmp_curr);

			if (x % 4 == 0) {
				if (x != interpolated.cols - 1) {
					interpolated(y - 1, x + 1) = interpolate_mean(interpolated(y - 2, x + 2) , tmp_curr);
					interpolated(y + 1, x + 1) = interpolate_mean(interpolated(y + 2, x + 2) , tmp_curr);
				}

				if (x != 0) {
					interpolated(y - 1, x - 1) = interpolate_mean(interpolated(y - 2, x - 2) , tmp_curr);
					interpolated(y + 1, x - 1) = interpolate_mean(interpolated(y + 2, x - 2) , tmp_curr);
				}

			}
		}
	}

	return interpolated;
}

void modify(Movie &mov, int type = 0)
{
	cv::Mat_<cv::Vec3b> last;

	last = cv::Mat_<cv::Vec3b>(mov[0].rows*I, mov[0].cols*I, cv::Vec3b(0,0,0));
	
	for (int frame = 0; frame < mov.get_n_frames(); ++frame) {
		cv::Mat_<cv::Vec3b> img = mov[frame];
		int orig_rows = img.rows;
		int orig_cols = img.cols;
		cv::Mat_<cv::Vec3s> displacement = cv::Mat_<cv::Vec3s>(orig_rows, orig_cols, cv::Vec3s(0,0,0));
		std::cout<<"frame: "<< frame<< std::endl;
		cv::Mat_<cv::Vec3b> img_inter;

		if (!type)
			img_inter = interpolate2(img);
		else
			img_inter = interpolate264(img);

		for (int i = 0; i < orig_rows; i += N)
			for (int j = 0; j < orig_cols; j += N)
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
