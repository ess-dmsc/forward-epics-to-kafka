#pragma once

class BufRange {
public:
BufRange(char * begin, size_t size) : begin(begin), size(size) { }
char * begin;
size_t size;
};
