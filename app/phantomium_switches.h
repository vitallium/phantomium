// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef PHANTOMIUM_APP_PHANTOMIUM_SWITCHES_H_
#define PHANTOMIUM_APP_PHANTOMIUM_SWITCHES_H_

#include "content/public/common/content_switches.h"

namespace phantomium {
namespace switches {

extern const char kProxyServer[];
extern const char kRemoteDebuggingAddress[];
extern const char kUserAgent[];
extern const char kWindowSize[];

// Switches which are replicated from content.
using ::switches::kRemoteDebuggingPipe;
using ::switches::kRemoteDebuggingPort;

}  // namespace switches
}  // namespace phantomium

#endif  // PHANTOMIUM_APP_PHANTOMIUM_SWITCHES_H_
