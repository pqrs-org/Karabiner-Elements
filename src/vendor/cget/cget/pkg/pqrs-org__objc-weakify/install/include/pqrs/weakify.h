#pragma once

// pqrs::weakify v1.0

// Created by Justin Spahr-Summers on 2011-05-04.
// Copyright (C) 2012 Justin Spahr-Summers.
// Released under the MIT license.

// 2019 Takayama Fumihiko
// Copied @weakify and @strongify from libextobjc and modified them.
// https://github.com/jspahrsummers/libextobjc/blob/master/extobjc/EXTScope.h

#import <Cocoa/Cocoa.h>

#define weakify(VAR) \
  autoreleasepool {} \
  __weak __typeof(VAR) VAR##_weak_ = VAR;

#define strongify(VAR)                              \
  autoreleasepool {}                                \
  _Pragma("clang diagnostic push");                 \
  _Pragma("clang diagnostic ignored \"-Wshadow\""); \
  __strong typeof(VAR) VAR = VAR##_weak_;           \
  _Pragma("clang diagnostic pop");
