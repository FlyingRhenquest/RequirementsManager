/**
 * Copyright 2025 Bruce Ide
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.

 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <fr/RequirementsManager/GraphNodeLocator.h>
#include <fr/RequirementsManager/PqDatabase.h>
#include <fr/RequirementsManager/PqNodeFactory.h>
#include <gtest/gtest.h>
#include <mutex>
#include <thread>

using namespace fr::RequirementsManager;

// Verify we can write a node and then read it back out

TEST(GraphNodeLocatorTests, WriteRead) {
  std::mutex waitMutex;
  std::condition_variable waitCv;
  auto node = std::make_shared<GraphNode>();
  node->init();
  node->setTitle("Test Node");
  auto threadpool = std::make_shared<ThreadPool<WorkerThread>>();
  threadpool->startThreads(4);
  auto saver = std::make_shared<SaveNodesNode<WorkerThread>>(node);
  bool saved = false;
  saver->complete.connect(
      [&waitCv, &saved](const std::string &id, Node::PtrType /* Notused */) {
        saved = true;
        waitCv.notify_one();
      });
  threadpool->enqueue(saver);
  std::unique_lock lock(waitMutex);
  waitCv.wait(lock, [&saved]() { return saved; });
  threadpool->shutdown();
  threadpool->join();
  GraphNodeLocator locator;
  locator.query();
  bool foundKey = false;
  for (auto [key, title] : locator.nodes) {
    if (key == node->idString()) {
      foundKey = true;
      break;
    }
  }
  ASSERT_TRUE(foundKey);
}
