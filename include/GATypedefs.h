/**
 * @file GATypedefs.h
 * @brief Defines legacy GA data aliases used by the optimizer interface.
 *
 * These aliases keep the original span-based GA API compatible with the ESE
 * graph node and edge typedefs used elsewhere in the project.
 */

#pragma once

#include <cstddef>
#include <span>
#include <map>
#include <vector>
#include <limits>

#include "GraphTypedefs.h"  // from esegraph/include
