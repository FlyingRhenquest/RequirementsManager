/**
 * Copyright 2026 Bruce Ide
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

#include <gtest/gtest.h>
#include <fr/RequirementsManager/PqDatabase.h>
#include <fr/RequirementsManager/PqNodeFactory.h>
#include <fr/RequirementsManager/RemoveNodesNode.h>
#include <fr/RequirementsManager/ThreadPool.h>
#include <fr/RequirementsManager/Todo.h>
#include <iostream>
#include <memory>
#include <mutex>
#include <sstream>
#include <thread>

using namespace fr::RequirementsManager;

// Basic functionality, serialization, deserialization
TEST(TodoTest, RecurringTodoBasic) {

  auto todo = std::make_shared<RecurringTodo>();
  todo->init();
  // Set recurring interval to 1 day. RecurringTodo doesn't contain
  // any of the logic to spawn a todo, but I do want to make sure
  // it serializes/deserializes correctly.
  todo->setRecurringInterval(86400ul);
  todo->setSecondsFlag(true);
  todo->setDescription("Test description");
  ASSERT_GT(todo->getCreated(), 0ul);
  ASSERT_EQ(86400ul, todo->getRecurringInterval());
  ASSERT_EQ(todo->getDescription(), "Test description");
  // Serialize
  std::stringstream stream;
  {
    cereal::JSONOutputArchive archive(stream);
    archive(todo);
  }
  // deserialize
  RecurringTodo::PtrType copy;
  {
    cereal::JSONInputArchive archive(stream);
    archive(copy);
  }
  ASSERT_EQ(todo->getCreated(), copy->getCreated());
  ASSERT_EQ(todo->getRecurringInterval(), copy->getRecurringInterval());
  ASSERT_EQ(todo->getDescription(), copy->getDescription());
  ASSERT_TRUE(todo->getSecondsFlag());
  ASSERT_EQ(todo->getSecondsFlag(), copy->getSecondsFlag());
  ASSERT_FALSE(todo->getDayOfMonthFlag());
  ASSERT_EQ(todo->getDayOfMonthFlag(), copy->getDayOfMonthFlag());
  ASSERT_FALSE(todo->getDayOfYearFlag());
  ASSERT_EQ(todo->getDayOfMonthFlag(), copy->getDayOfMonthFlag());

  // I also want to create a todo from this recurring todo
  auto child = Todo::fromRecurring(todo);
  ASSERT_GT(child->getCreated(), 0ul);
  ASSERT_FALSE(child->getCompleted());
  ASSERT_FALSE(child->getSpawnedFrom().is_nil());
  ASSERT_EQ(todo->id, child->getSpawnedFrom());
  ASSERT_EQ(todo->getDescription(), child->getDescription());  
}

// Saving and loading to the database
TEST(TodoTest, RecurringLoadSaveDb) {
  auto remover = std::make_shared<RemoveNodesNode<WorkerThread>>();
  auto threadpool = std::make_shared<ThreadPool<WorkerThread>>();
  threadpool->startThreads(4);
  
  auto todo = std::make_shared<RecurringTodo>();
  todo->init();
  ASSERT_FALSE(todo->id.is_nil());
  todo->setDescription("A test todo");
  // Set recurring interval to first day of the month
  todo->setRecurringInterval(1ul);
  todo->setDayOfMonthFlag(true);
  remover->addDown(todo);
  auto saver = std::make_shared<SaveNodesNode<WorkerThread>>(todo);
  std::mutex waitMutex;
  std::condition_variable waitCv;
  std::string savedId;

  saver->complete.connect(
      [&waitCv, &savedId](const std::string& id, Node::PtrType /* NotUsed */) {
        savedId = id;
        waitCv.notify_one();
      });
  {
    std::cout << "Saving " << todo->idString() << std::endl;
    std::unique_lock lock(waitMutex);
    threadpool->enqueue(saver);
    waitCv.wait(lock, [&savedId]() { return !savedId.empty(); });
    std::cout << "Saved" << std::endl;
  }
  ASSERT_TRUE(saver->treeSaveComplete());
  
  // Load the node we just saved

  auto factory = std::make_shared<PqNodeFactory<WorkerThread>>(todo->idString());
  std::string loaded;
  factory->done.connect([&waitCv, &loaded](const std::string& id) {
    loaded = id;
    waitCv.notify_one();
  });
  {
    std::cout << "Loading " << todo->idString() << std::endl;
    std::unique_lock lock(waitMutex);
    threadpool->enqueue(factory);
    waitCv.wait(lock, [&loaded] { return ! loaded.empty(); });
    std::cout << "Loaded" << std::endl;
  }
  ASSERT_NE(factory->getNode(), nullptr);
  RecurringTodo::PtrType copy = std::dynamic_pointer_cast<RecurringTodo>(factory->getNode());
  ASSERT_NE(copy, nullptr);
  ASSERT_EQ(copy->idString(), todo->idString());
  ASSERT_EQ(copy->getDescription(), todo->getDescription());
  ASSERT_EQ(copy->getRecurringInterval(), todo->getRecurringInterval());
  ASSERT_EQ(copy->getDayOfMonthFlag(), todo->getDayOfMonthFlag());

  threadpool->enqueue(remover);
  threadpool->shutdown();
  threadpool->join();
}
