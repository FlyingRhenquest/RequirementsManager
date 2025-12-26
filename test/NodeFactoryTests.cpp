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

#include <fr/RequirementsManager.h>
#include <fr/RequirementsManager/NodeConnector.h>
#include <fr/RequirementsManager/PqDatabase.h>
#include <fr/RequirementsManager/PqNodeFactory.h>
#include <gtest/gtest.h>
#include <mutex>
#include <thread>

using namespace fr::RequirementsManager;
/**
 * Test allocation for Node and unknown type
 */

TEST(NodeFactoryTest, BasicAllocator) {
  NodeAllocator allocator;

  // Node actually isn't in the list, it's just the default type
  // returned if we don't find the correct one.
  auto node = allocator.get("Node", "019ae1b4-53e6-72d5-8058-b0f8014f75e8");
  auto unknownNode =
      allocator.get("unknown", "019ae1b4-53fb-7304-80f1-7328836d47d3");
  // Check for null (get should never return null)
  ASSERT_TRUE(node);
  ASSERT_TRUE(unknownNode);

  ASSERT_EQ(node->getNodeType(), "Node");
  ASSERT_EQ(unknownNode->getNodeType(), "Node");
  ASSERT_EQ(node->idString(), "019ae1b4-53e6-72d5-8058-b0f8014f75e8");
  ASSERT_EQ(unknownNode->idString(), "019ae1b4-53fb-7304-80f1-7328836d47d3");
}

TEST(NodeFactoryTest, SpecificAllocations) {
  NodeAllocator allocator;

  auto org =
      allocator.get("Organization", "019ae1b4-53fb-7355-809c-53ac3157930d");
  auto event = allocator.get("Event", "019ae1b4-5414-7012-8040-de09cd8188ac");
  auto goal = allocator.get("Goal", "019ae1b4-5414-7033-8056-a3d498e1c7ea");

  ASSERT_TRUE(org);
  ASSERT_TRUE(event);
  ASSERT_TRUE(goal);

  ASSERT_EQ(org->getNodeType(), "Organization");
  ASSERT_EQ(event->getNodeType(), "Event");
  ASSERT_EQ(goal->getNodeType(), "Goal");

  ASSERT_EQ(org->idString(), "019ae1b4-53fb-7355-809c-53ac3157930d");
  ASSERT_EQ(event->idString(), "019ae1b4-5414-7012-8040-de09cd8188ac");
  ASSERT_EQ(goal->idString(), "019ae1b4-5414-7033-8056-a3d498e1c7ea");
}

TEST(NodeFactoryTest, LoadAGraph) {
  auto org = std::make_shared<Organization>();
  ;
  org->setName("Global Consolidated Software Engineering, Inc.");
  org->lock();
  auto project = std::make_shared<Project>();
  project->setName("Engineer some software");
  connectNodes(org, project);
  auto product = std::make_shared<Product>();
  product->setTitle("Some software");
  connectNodes(project, product);
  auto req = std::make_shared<Requirement>();
  req->setTitle("Must be software");
  connectNodes(product, req);
  req = std::make_shared<Requirement>();
  req->setTitle("Must be engineered");
  connectNodes(product, req);
  // We can save this whole graph to the database from
  // any node in the graph. We're going to need a
  // threadpool
  auto threadpool = std::make_shared<ThreadPool<WorkerThread>>();
  threadpool->startThreads(4);
  auto saver = std::make_shared<SaveNodesNode<WorkerThread>>(req);

  std::mutex waitMutex;
  std::condition_variable waitCv;
  // Saver will signal after each node that was saved. Each time that
  // happens we want to wake up and see if the whole graph has been
  // saved yet before we continue.
  saver->complete.connect(
      [&waitCv](const std::string &id, Node::PtrType /* NotUsed */) {
        waitCv.notify_one();
      });

  threadpool->enqueue(saver);
  std::unique_lock lock(waitMutex);
  // Will only wake up once treeSaveComplete returns true.
  waitCv.wait(lock, [&saver]() { return saver->treeSaveComplete(); });
  // OK! Let's load it!
  auto factory = std::make_shared<PqNodeFactory<WorkerThread>>(org->idString());
  std::string nodeLoaded;

  factory->loaded.connect([&waitCv, &nodeLoaded](const std::string &id,
                                                 Node::PtrType /* NotUsed */) {
    nodeLoaded = id;
    waitCv.notify_one();
  });
  threadpool->enqueue(factory);
  // Wait for the factory to signal that something has been loaded.
  // Doesn't really matter what -- we'll just shut the threadpool
  // down and wait for it to finish loading.
  waitCv.wait(lock, [&nodeLoaded]() { return !nodeLoaded.empty(); });
  threadpool->shutdown();
  threadpool->join();
  auto restoredOrg = factory->getNode();

  // Make sure it's not null
  ASSERT_TRUE(restoredOrg);
  ASSERT_EQ(restoredOrg->idString(), org->idString());
  ASSERT_EQ(restoredOrg->down.size(), org->down.size());
}
