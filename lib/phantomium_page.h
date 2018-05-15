// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef PHANTOMIUM_LIB_PHANTOMIUM_PAGE_H_
#define PHANTOMIUM_LIB_PHANTOMIUM_PAGE_H_

#include <string>

#include "base/files/file_proxy.h"
#include "base/sequenced_task_runner.h"
#include "headless/public/devtools/domains/inspector.h"
#include "headless/public/devtools/domains/page.h"
#include "headless/public/headless_browser.h"
#include "headless/public/headless_devtools_client.h"
#include "headless/public/headless_devtools_target.h"
#include "headless/public/headless_web_contents.h"

class GURL;

namespace phantomium {

class PhantomiumPage : public headless::HeadlessWebContents::Observer,
                       public headless::inspector::ExperimentalObserver,
                       public headless::page::Observer {
 public:
  class Observer;
  PhantomiumPage();
  ~PhantomiumPage() override;

  void Load(const GURL& url, const base::FilePath& file_name);
  void Shutdown();

  void SetBrowserContext(headless::HeadlessBrowserContext* browser_context);

  void AddObserver(Observer* obs);
  void RemoveObserver(Observer* obs);

 private:
  // HeadlessWebContents::Observer implementation:
  void DevToolsTargetReady() override;
  void OnTargetCrashed(
      const headless::inspector::TargetCrashedParams& params) override;

  // page::Observer implementation:
  void OnLoadEventFired(
      const headless::page::LoadEventFiredParams& params) override;

  void PrintToPDF();

  void OnPDFCreated(std::unique_ptr<headless::page::PrintToPDFResult> result);
  void WriteFile(const std::string& base64_data);
  void OnFileOpened(const std::string& decoded_data,
                    const base::FilePath file_name,
                    base::File::Error error_code);
  void OnFileWritten(const base::FilePath file_name,
                     const size_t length,
                     base::File::Error error_code,
                     int write_result);
  void OnFileClosed(base::File::Error error_code);

  bool processed_page_ready_;
  base::FilePath file_name_;
#if !defined(CHROME_MULTIPLE_DLL_CHILD)
  headless::HeadlessBrowserContext* browser_context_;
  headless::HeadlessWebContents* web_contents_;
#endif
  scoped_refptr<base::SequencedTaskRunner> file_task_runner_;
  std::unique_ptr<base::FileProxy> file_proxy_;
  // The DevTools client used to control the tab.
  std::unique_ptr<headless::HeadlessDevToolsClient> devtools_client_;
  base::Lock observers_lock_;
  base::ObserverList<Observer> observers_;
  base::WeakPtrFactory<PhantomiumPage> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(PhantomiumPage);
};

class PhantomiumPage::Observer {
 public:
  // Indicates the PhantomiumPage is about to be deleted.
  virtual void OnPhantomiumPageDestruct() {}

 protected:
  virtual ~Observer() {}
};

}  // namespace phantomium

#endif  // PHANTOMIUM_LIB_PHANTOMIUM_PAGE_H_
