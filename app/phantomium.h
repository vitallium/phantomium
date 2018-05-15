// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef PHANTOMIUM_APP_PHANTOMIUM_H_
#define PHANTOMIUM_APP_PHANTOMIUM_H_

#include "base/memory/weak_ptr.h"
#include "headless/public/headless_browser.h"
#include "headless/public/headless_browser_context.h"
#include "phantomium/lib/phantomium_page.h"

namespace phantomium {

class Phantomium : public PhantomiumPage::Observer {
 public:
  Phantomium();
  ~Phantomium() override;

  void OnPhantomiumPageDestruct() override;

#if !defined(CHROME_MULTIPLE_DLL_CHILD)
  virtual void OnStart(headless::HeadlessBrowser* browser);
#endif

  void Shutdown();

 private:
  std::unique_ptr<PhantomiumPage> CreatePage();

 private:
  base::Lock lock_;  // Protects |browser_context_|.
  // The headless browser instance. Owned by the headless library.
  headless::HeadlessBrowser* browser_;
  headless::HeadlessBrowserContext* browser_context_;
  // TODO(vitallium): Transform it to array
  std::unique_ptr<PhantomiumPage> page_;
  // A helper for creating weak pointers to this class.
  base::WeakPtrFactory<Phantomium> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(Phantomium);
};

}  // namespace phantomium

#endif  // PHANTOMIUM_APP_PHANTOMIUM_H_
