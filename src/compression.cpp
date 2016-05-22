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

/*
cv::Mat_<cv::Vec3b> interpolate(cv::Mat_<cv::Vec3b>& img, int type)
{
	cv::Mat_<cv::Vec3b> interpolated = cv::Mat_<cv::Vec3b>(img.rows*I, img.cols*I, cv::Vec3b(0,0,0));
	for (int  i =0; i<img.rows-1; i++)
		for (int j = 0; j < img.cols -1; j++) {
			for (int k = 0; k <3; k++) {
       				interpolated(i*I,j*I)[k] = img(i,j)[k];
			  	interpolated(i*I,(j + 1)*I)[k] = img(i,j+1)[k];
			  	interpolated((i + 1)*I,(j + 1)*I)[k] = img(i+1,j+1)[k];
			  	interpolated((i+1)*I,j*I)[k] = img(i+1,j)[k];
				for (int r = 1; r < I; r++) {
				  interpolated(i*I,j*I+r)[k] = (((j*I+I-(j*I+r))*img(i,j)[k]) + (((j*I+r)-j*I)*img(i,j+1)[k]))/(I);
				  interpolated(i*I+r,j*I)[k] = (((i*I+I-(i*I+r))*img(i,j)[k]) + (((i*I+r)-i*I)*img(i+1,j)[k]))/(I);
				  interpolated(i*I+r,j*I+I)[k] = (((i*I+I-(i*I+r))*img(i,j+1)[k]) + (((i*I+r)-i*I)*img(i+1,j+1)[k]))/(I);
				  interpolated(i*I+I,j*I + r)[k] = (((j*I+I-(j*I+r))*img(i+1,j)[k]) + (((j*I+r)-j*I)*img(i+1,j+1)[k]))/(I);
					
				}
				for (int rx = 1; rx < I; rx++)
					for (int ry = 1; ry < I;ry++) {
					  interpolated(i*I +rx, j*I+ry)[k] = (((j*I+I-(j*I+ry))*interpolated(i*I+rx,j*I)[k]) + (((j*I+ry)-j*I)*interpolated(i*I,j*I+I)[k]))/(I);
					}
			}
		}

	return interpolated;
}
*/
int interpolation_calc(int x, int x1, int Q1, int x2, int Q2) {
  return ((x2-x)*Q1+ (x-x1)*Q2)/I;
}

cv::Mat_<cv::Vec3b> interpolate(cv::Mat_<cv::Vec3b> img, int type)
{
  cv::Mat_<cv::Vec3b> interpolated = cv::Mat_<cv::Vec3b>((img.rows-1)*I+1, (img.cols-1)*I+1, cv::Vec3b(0,-1,-1));

	for (int k = 0; k <3; k++) {
		for (int  i =0; i<img.rows; i++) {
			interpolated(i*I,0)[k] = img(i,0)[k];
			for (int j = (i -1)*I+1; j >= 0 && j < i*I;j++)
				interpolated(j, 0)[k] = interpolation_calc(j,(i-1)*I, img(i-1,0)[k], i*I, img(i,0)[k]);
		}
		for (int  i =0; i<img.cols; i++) {
			interpolated(0,i*I)[k] = img(0,i)[k];
			for (int j = (i -1)*I+1; j >= 0 && j < i*I;j++)
				interpolated(0,j)[k] = interpolation_calc(j,(i-1)*I, img(0,i-1)[k], i*I, img(0,i)[k]);
		}
		for (int  i =1; i<img.rows; i++)
			for (int j = 1; j < img.cols; j++) {
				interpolated(i*I,j*I)[k] = img(i,j)[k];
				for (int x = i*I-1;  x > (i - 1)*I;x--) {
					interpolated(x,j*I)[k] = interpolation_calc(x,(i-1)*I, interpolated((i-1)*I,j*I)[k], i*I, interpolated(i*I,j*I)[k]);
				}
				for (int y = j*I-1;  y > (j - 1)*I;y--) {
					interpolated(i*I,y)[k] = interpolation_calc(y,(j-1)*I, interpolated(i*I, (j-1)*I)[k], j*I, interpolated(i*I,j*I)[k]);
				}
				for (int x = i*I-1; x > (i - 1)*I;x--) {
					for (int y = j*I-1; y > (j - 1)*I;y--) {
						interpolated(x,y)[k] = interpolation_calc(y,(j-1)*I, interpolated(x, (j-1)*I)[k], j*I, interpolated(x,j*I)[k]);
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
		cv::Mat_<cv::Vec3b> img_inter = interpolate(img, type);
		for (int i = 0; i < orig_rows; i+=N)
			for (int j = 0; j < orig_cols; j+=N) {
				find_(last, img, displacement, i, j, I);
			}

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
