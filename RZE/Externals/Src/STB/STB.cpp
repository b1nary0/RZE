// stb's single-header libraries, compiled once for everything that links Externals. Their headers are in
// ThirdParty/Include/STB; anything else includes them without these defines.

#define STB_IMAGE_IMPLEMENTATION
#include <STB/stb_image.h>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <STB/stb_image_write.h>
