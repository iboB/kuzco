// kuzco
// Copyright (c) 2020 Borislav Stanimirov
//
// Distributed under the MIT Software License
// See accompanying file LICENSE.txt or copy at
// https://opensource.org/licenses/MIT
//
#pragma once

// Shared library interface
#if defined(_MSC_VER)
#   define KUZCO_SYMBOL_EXPORT __declspec(dllexport)
#   define KUZCO_SYMBOL_IMPORT __declspec(dllimport)
#else
#   define KUZCO_SYMBOL_EXPORT __attribute__((__visibility__("default")))
#   define KUZCO_SYMBOL_IMPORT
#endif

#if KUZCO_SHARED
#   if BUILDING_KUZCO
#       define KUZCO_API KUZCO_SYMBOL_EXPORT
#   else
#       define KUZCO_API KUZCO_SYMBOL_IMPORT
#   endif
#else
#   define KUZCO_API
#endif
