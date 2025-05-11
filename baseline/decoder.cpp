#include <decoder.h>
#include <glog/logging.h>
#include "reader.h"

Image Decode(std::istream& input) {
    Reader reader(input);
    return reader.DecodeImage();
}
