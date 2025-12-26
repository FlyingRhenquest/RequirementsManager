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

#include <boost/uuid/uuid_io.hpp>
#include <fr/RequirementsManager/Node.h>
#include <gtest/gtest.h>
#include <iostream>
#include <sstream>

// Verify InitNode sets the ID.
TEST(NodeTests, InitNode) {
  fr::RequirementsManager::Node n;
  n.init(); // Sets id
  std::stringstream output;
  output << n.id;
  // Something was output
  ASSERT_GT(output.str().size(), 0);
  // You can also use boost::uuids::uuid::version_type::version_time_based_v7.
  // It's 7. I want a version 7 uuid.
  ASSERT_EQ(n.id.version(), 7);
}

// Test node traversal code
TEST(NodeTests,Traversal) {
  auto n = std::make_shared<fr::RequirementsManager::Node>();
  n->init();
  // We start addedCounter at 1 to account for the starting node
  int addedCounter = 1;
  for (int i = 0 ; i < 5; ++i) {
    // Add some up and down nodes
    auto u = n->addUp(std::make_shared<fr::RequirementsManager::Node>());
    u->init();
    std::cout << "Added "  << u->idString() << " to uplist" << std::endl;
    auto d = n->addDown(std::make_shared<fr::RequirementsManager::Node>());
    d->init();
    std::cout << "Added " << d->idString() << " to downlist " << std::endl;
    addedCounter += 2;
  }
  int traverseCounter = 0;
  n->traverse([&traverseCounter](fr::RequirementsManager::Node::PtrType node) {
    traverseCounter++;
    std::cout << "Traversed into " << node->idString() << std::endl;
  });
  std::cout << "Traverse counter is " << traverseCounter << std::endl;
  ASSERT_EQ(addedCounter, traverseCounter);
}

// InsertChildren inserts a few nodes in the passed node's
// down list and adds the passed node to the children's up
// list.

void insertChildren(fr::RequirementsManager::Node::PtrType parent) {
  for (int i = 0; i < 5; ++i) {
    auto child = std::make_shared<fr::RequirementsManager::Node>();
    child->init();
    parent->down.push_back(child);
    child->up.push_back(parent);
  }
}

// Tests serialization of nodes
TEST(NodeTests, Serialization) {
  auto original = std::make_shared<fr::RequirementsManager::Node>();
  original->init();
  // insert some random children
  insertChildren(original);
  // Also insert children in all the children because why not?
  for (auto child : original->down) {
    insertChildren(child);
  }
  // Serialize to a StringStream (JSON)

  std::stringstream stream;
  {
    cereal::JSONOutputArchive archive(stream);
    archive(original);
  }

  auto copy = std::make_shared<fr::RequirementsManager::Node>();
  // Deserialze from stream
  {
    cereal::JSONInputArchive archive(stream);
    archive(copy);
  }
  ASSERT_EQ(original->id, copy->id);
  auto oChild = original->down.begin();
  auto cChild = original->down.begin();
  while (oChild != original->down.end() && cChild != original->down.end()) {
    ASSERT_EQ((*oChild)->id, (*cChild)->id);
    oChild++;
    cChild++;
  }
}
