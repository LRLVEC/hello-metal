#pragma once


// For glm swizzles to work correctly, Microsoft extensions
// need to be enabled. This is done by the -fms-extensions
// flag (see CMakeLists.txt), and the following macro needs
// to be defined such that GLM is aware of this.
#ifndef _MSC_EXTENSIONS
#define _MSC_EXTENSIONS
#endif
#define GLM_ENABLE_EXPERIMENTAL

#define GLM_FORCE_SWIZZLE
#include <glm/glm.hpp>
#include <glm/gtx/norm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/component_wise.hpp>
#include <glm/gtc/matrix_access.hpp>
using namespace glm;