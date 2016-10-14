// BSD 3-Clause License
//
// Copyright (c) 2016, Michael Vasilyev
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// * Redistributions of source code must retain the above copyright notice, this
//   list of conditions and the following disclaimer.
//
// * Redistributions in binary form must reproduce the above copyright notice,
//   this list of conditions and the following disclaimer in the documentation
//   and/or other materials provided with the distribution.
//
// * Neither the name of the copyright holder nor the names of its
//   contributors may be used to endorse or promote products derived from
//   this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
// FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
// SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
// CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
// OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.


#pragma once

#include <QtCore/QtGlobal>

#if defined(Q_OS_LINUX)
#define VK_USE_PLATFORM_XCB_KHR
//#define VK_USE_PLATFORM_XLIB_KHR //TODO optional
#elif defined(Q_OS_WIN)
#define NOMINMAX
#define VK_USE_PLATFORM_WIN32_KHR
#define __PRETTY_FUNCTION__ __FUNCTION__
#else
#error "OS support is not implemented"
#endif

#include <vulkan/vulkan.h>

#include <cstdint>
#include <stdexcept>
#include <memory>
#include <cassert>
#include <typeinfo>

#include "struct-validator.hpp"


#include <QDebug>

#define TRACE() { qDebug() << "[TRACE]" << __PRETTY_FUNCTION__ << __LINE__; }
#define SOFT_NOT_IMPLEMENTED() { qDebug() << "WARNING: NOT IMPLEMENTED" << __PRETTY_FUNCTION__ << __LINE__; }
#define NOT_IMPLEMENTED() { qDebug() << "NOT IMPLEMENTED" << __PRETTY_FUNCTION__ << __LINE__; throw std::runtime_error("NOT IMPLEMENTED"); }

#ifndef VK_ERROR_INVALID_PARAMETER_NV
#   define VK_ERROR_INVALID_PARAMETER_NV static_cast<VkResult>(-1000013000)
#endif

#ifndef VK_ERROR_INVALID_ALIGNMENT_NV
#define VK_ERROR_INVALID_ALIGNMENT_NV static_cast<VkResult>(-1000013001)
#endif

#ifndef VK_ERROR_INVALID_SHADER_NV
#define VK_ERROR_INVALID_SHADER_NV static_cast<VkResult>(-1000012000)
#endif
