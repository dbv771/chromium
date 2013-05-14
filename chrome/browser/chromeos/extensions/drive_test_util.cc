// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/bind.h"
#include "base/files/file_path.h"
#include "base/run_loop.h"
#include "chrome/browser/chromeos/drive/drive_file_system.h"
#include "chrome/browser/chromeos/drive/drive_file_system_observer.h"
#include "chrome/browser/chromeos/drive/drive_system_service.h"
#include "chrome/browser/profiles/profile.h"
#include "content/public/browser/browser_context.h"
#include "webkit/fileapi/external_mount_points.h"

namespace drive_test_util {

namespace {

const char kDriveMountPointName[] = "drive";

// Helper class used to wait for |OnFileSystemMounted| event from a drive file
// system.
class DriveMountPointWaiter : public drive::DriveFileSystemObserver {
 public:
  explicit DriveMountPointWaiter(drive::DriveFileSystemInterface* file_system)
      : file_system_(file_system) {
    file_system_->AddObserver(this);
  }

  virtual ~DriveMountPointWaiter() {
    file_system_->RemoveObserver(this);
  }

  // DriveFileSystemObserver override.
  virtual void OnFileSystemMounted() OVERRIDE {
    // Note that it is OK for |run_loop_.Quit| to be called before
    // |run_loop_.Run|. In this case |Run| will return immediately.
    run_loop_.Quit();
  }

  // Runs loop until the file_system_ gets mounted.
  void Wait() {
    run_loop_.Run();
  }

 private:
  drive::DriveFileSystemInterface* file_system_;
  base::RunLoop run_loop_;
};

}  // namespace

void WaitUntilDriveMountPointIsAdded(Profile* profile) {
  DCHECK(profile);

  // Drive mount point is added by the browser when the drive system service
  // is first initialized. It is done asynchronously after some other parts of
  // the service are initialized (e.g. resource metadata and cache), thus racy
  // with the test start. To handle this raciness, the test verifies that
  // drive mount point is added before continuing. If this is not the case,
  // drive file system is observed for FileSystemMounted event (by
  // |mount_point_waiter|) and test continues once the event is encountered.
  drive::DriveSystemService* system_service =
      drive::DriveSystemServiceFactory::FindForProfileRegardlessOfStates(
          profile);
  DCHECK(system_service && system_service->file_system());

  DriveMountPointWaiter mount_point_waiter(system_service->file_system());

  base::FilePath ignored;
  // GetRegisteredPath succeeds iff the mount point exists.
  if (!content::BrowserContext::GetMountPoints(profile)->
      GetRegisteredPath(kDriveMountPointName, &ignored)) {
    LOG(WARNING) << "Waiting for drive mount point to get mounted.";
    mount_point_waiter.Wait();
    LOG(WARNING) << "Drive mount point found.";
  }
}

}  // namespace drive_test_util
