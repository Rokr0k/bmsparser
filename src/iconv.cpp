#include "iconv.hpp"
#include <string.h>
extern "C"
{
#include <iconv.h>
}

std::string iconv_cxx::convert(const std::string &text)
{
    iconv_t icv = iconv_open("utf-8", "shift-jis");
    char *src = new char[strlen(text.c_str()) + 1], *srcB = src;
    strcpy(src, text.c_str());
    char *dst = new char[strlen(text.c_str()) * 2 + 1], *dstB = dst;
    size_t srcL = strlen(text.c_str()), dstL = strlen(text.c_str()) * 2 + 1;
    size_t c = iconv(icv, &srcB, &srcL, &dstB, &dstL);
    *dstB = '\0';
    std::string res = dst;
    delete[] src;
    delete[] dst;
    iconv_close(icv);
    return res;
}