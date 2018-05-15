// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "phantomium/app/phantomium_switches.h"

namespace phantomium {
namespace switches {

// Uses a specified proxy server, overrides system settings. This switch only
// affects HTTP and HTTPS requests.
const char kProxyServer[] = "proxy-server";

// Use the given address instead of the default loopback for accepting remote
// debugging connections. Should be used together with --remote-debugging-port.
// Note that the remote debugging protocol does not perform any authentication,
// so exposing it too widely can be a security risk.
const char kRemoteDebuggingAddress[] = "remote-debugging-address";

// A string used to override the default user agent with a custom one.
const char kUserAgent[] = "user-agent";

// Sets the initial window size. Provided as string in the format "800,600".
const char kWindowSize[] = "window-size";

}  // namespace switches
}  // namespace phantomium
