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
 *
 */

#include <gtest/gtest.h>
#include <fr/RequirementsManager/Node.h>
#include <fr/RequirementsManager/NodeConnector.h>
#include <fr/RequirementsManager/PqDatabase.h>
#include <fr/RequirementsManager/ThreadPool.h>

using namespace fr::RequirementsManager;
/**
 * Create and save one node. For this test we'll just create
 * and save one node with saveThisNodeOnly = false. Since there
 * are no attached nodes, this shouldn't make a difference.
 */

TEST(DatabaseTests, TestSaveOneNode) {
  auto nodeToSave = std::make_shared<Node>();
  nodeToSave->init();
  auto saver = std::make_shared<SaveNodesNode<WorkerThread>>(nodeToSave);
  // This is designed to be run in a threadpool but for this test
  // we'll just run it directly
  saver->run();
  ASSERT_TRUE(saver->saveComplete());
}

/**
 * Create two nodes, save both, using threadpool
 */
TEST(DatabaseTests, TwoNodesThreadPool) {
  // Use this mutex and condition variable to
  // wait for saver::run to complete
  std::mutex waitMutex;
  std::condition_variable waitCv;
  auto parent = std::make_shared<Node>();
  auto child = std::make_shared<Node>();
  std::string savedId;
  connectNodes(parent, child);
  auto threadpool = std::make_shared<ThreadPool<WorkerThread>>();
  // 2 - 8 would probably be a good choice for thread pool size
  // depending on your processor cores.

  threadpool->startThreads(4);
  auto saver = std::make_shared<SaveNodesNode<WorkerThread>>(parent);

  // Intercept signal from saver and wake this process up when it is
  // received.
  
  saver->complete.connect([&waitCv, &savedId](const std::string& id) {
    savedId = id;
    waitCv.notify_one();
  });      
  
  threadpool->enqueue(saver);
  std::unique_lock lock(waitMutex);
  std::cout << "Waiting for initial save to complete..." << std::endl;
  // Wait for initial save to complete. After that we can
  // shut the threadpool down and wait for it to join. It'll
  // process all the work queued by saver before it shuts down.
  // I need to pass the lambda in to prevent a spurious wakeup from
  // waking us up early. The wait will only continue once savedId
  // has received a value from the "complete" callback in saver.
  waitCv.wait(lock, [&savedId](){ return !savedId.empty(); });
  std::cout << "Initial save complete: " << savedId << std::endl;
  threadpool->shutdown();
  threadpool->join();
  ASSERT_TRUE(saver->treeSaveComplete());
}

/**
 * Try some specific nodes I've defined to verify that
 * the functionality I set up works correctly.
 */

TEST(DatabaseTests, SpecificNodeTests) {
  std::mutex waitMutex;
  std::condition_variable waitCv;
  std::string savedId;  
  auto projectTestWombat = std::make_shared<fr::RequirementsManager::Project>();
  auto wombatProduct = std::make_shared<fr::RequirementsManager::Product>();
  projectTestWombat->setName("Test Wombat");
  projectTestWombat->setDescription("Test A Wombat");
  wombatProduct->setTitle("Wombat");
  wombatProduct->setDescription("A wombat");
  connectNodes(projectTestWombat, wombatProduct);
  auto threadpool = std::make_shared<ThreadPool<WorkerThread>>();
  threadpool->startThreads(4);
  auto saver = std::make_shared<SaveNodesNode<WorkerThread>>(projectTestWombat);

  saver->complete.connect([&waitCv, &savedId](const std::string& id) {
    savedId = id;
    waitCv.notify_one();
  });

  std::unique_lock lock(waitMutex);
  std::cout << "Waiting for initial save to complete..." << std::endl;
  threadpool->enqueue(saver);
  waitCv.wait(lock, [&savedId](){return !savedId.empty();});
  std::cout << "Initial save complete: " << savedId << std::endl;
  threadpool->shutdown();
  threadpool->join();
  ASSERT_TRUE(saver->treeSaveComplete());
}
