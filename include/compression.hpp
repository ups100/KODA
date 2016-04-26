#ifndef COMPRESSION_HPP__
#define COMPRESSION_HPP__

#include "movie.hpp"

void compress_bilin(Movie &org, Movie **result);

void compress_soi(Movie &org, Movie **result);

#endif /* COMPRESSION_HPP__ */
