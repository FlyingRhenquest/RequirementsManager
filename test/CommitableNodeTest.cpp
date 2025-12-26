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

#include <fr/RequirementsManager/CommitableNode.h>
#include <gtest/gtest.h>

using namespace fr::RequirementsManager;

// Verify you can discard an uncommitted node
// and can't discard a committed change.
TEST(CommitableNode, DiscardChanges) {

  auto parent = std::make_shared<CommitableNode>();
  parent->init();
  parent->commit();
  // For CommitableNodes there really isn't a way to
  // test that you can't change anything since there's
  // not anything to change.
  auto child = getChangeNode(parent);
  parent->discardChange();
  child.reset();
  child = getChangeNode(parent);
  child->commit();
  bool threw = false;
  try {
    parent->discardChange();
  } catch (std::exception &e) {
    threw = true;
  }
  ASSERT_TRUE(threw);
}

// CommitableNodes should also traverse into
// changeNode
TEST(CommitableNode, Traverse) {
  auto parent = std::make_shared<CommitableNode>();
  parent->init();
  parent->commit();
  auto child = getChangeNode(parent);
  if (!child->initted) {
    child->init();
  }
  child->commit();
  int addCount = 2;
  int traverseCount = 0;
  parent->traverse([&traverseCount](Node::PtrType node) {
    std::cout << "Traversed into " << node->idString() << std::endl;
    traverseCount++;
  });
  ASSERT_EQ(addCount, traverseCount);
}
