#ifndef MOVIE_HPP__
#define MOVIE_HPP__

#include <string>

#include <dirent.h>

#include <opencv2/core/core.hpp>

class Movie {
public:
	Movie(const std::string &title="title");

	int from_dir(const std::string &dir);
	int clear_frames();
	inline cv::Mat& operator[](size_t i);
	inline size_t get_n_frames();
	inline const std::string &get_title();
	/* override for result Movie */
	virtual long long get_size();
	Movie &deep_copy(Movie &dest);
private:
	/* Some helpers */
	static int filter_func(const struct dirent *d);
	std::string m_title;
	std::vector<cv::Mat> m_frames;
};

cv::Mat& Movie::operator[](size_t i)
{
	return m_frames[i];
}

size_t Movie::get_n_frames()
{
	return m_frames.size();
}

const std::string &Movie::get_title()
{
	return m_title;
}

#endif /* MOVIE_HPP__ */
