#include <iostream>

#include <dirent.h>

#include <opencv2/core/core.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>

#include "movie.hpp"

Movie::Movie(const std::string &title)
	: m_title(title)
{
}

int Movie::filter_func(const struct dirent *d)
{
	return d->d_name[0] != '.';
}

int Movie::from_dir(const std::string &dir)
{
	int nframes;
	struct dirent **files;
	int i;

	nframes = scandir(dir.c_str(), &files, filter_func, alphasort);
	if (nframes < 0)
		return -1;

	try {
		m_frames.reserve(nframes);
		for (i = 0; i < nframes; ++i) {
			std::string file_path = dir + '/' + files[i]->d_name;
			cv::Mat frame;
			cv::Mat yuv_frame;

			frame = cv::imread(file_path);
			if (!frame.data) {
				std::cerr << "Unable to load image: "
					  << file_path << std::endl;
				throw "Unable to load image";
			}

			cv::cvtColor(frame, yuv_frame, CV_BGR2YUV);
			m_frames.push_back(yuv_frame);
			free(files[i]);
		}
		free(files);
	} catch (...) {
		for (; i < nframes; ++i)
			free(files[i]);
		free(files);
		m_frames.clear();
		return -1;
	}

	return 0;
}

int Movie::clear_frames()
{
	m_frames.clear();
}

long long Movie::get_size()
{
	return (long long)m_frames.size() * m_frames[0].rows
		*m_frames[0].cols * 3;
}

Movie &Movie::deep_copy(Movie &dest)
{
	dest.m_frames.clear();
	dest.m_frames.reserve(m_frames.size());

	/* We don't override the title here */

	for (int i = 0; i < m_frames.size(); ++i)
		dest.m_frames.push_back(m_frames[i].clone());

	return dest;
}
