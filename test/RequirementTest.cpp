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

#include <fr/RequirementsManager/Requirement.h>
#include <gtest/gtest.h>
#include <sstream>

using namespace fr::RequirementsManager;

// Test the happy paths for all the sets
TEST(Requirement, SetHappyPaths) {
  auto r = std::make_shared<Requirement>();
  const std::string title("Requirement Title");
  const std::string text("Need a requirement manager");
  r->setTitle(title);
  r->setText(text);
  r->setFunctional(true);
  ASSERT_EQ(r->getTitle(), title);
  ASSERT_EQ(r->getText(), text);
  ASSERT_TRUE(r->isFunctional());
  // We can change these up until we commit the requirement
  r->setTitle("");
  r->setText("");
  r->setFunctional(false);
  ASSERT_EQ(r->getTitle(), "");
  ASSERT_EQ(r->getText(), "");
  ASSERT_FALSE(r->isFunctional());
}

// Once we're committed we can no longer set the things
TEST(Requirement, SetSadPaths) {
  auto r = std::make_shared<Requirement>();
  const std::string title("Requirement Title");
  const std::string text("Need a requirement manager");
  r->setTitle(title);
  r->setText(text);
  r->setFunctional(true);
  r->commit();
  bool threw = false;
  try {
    r->setTitle("");
  } catch (std::exception &e) {
    threw = true;
  }
  ASSERT_TRUE(threw);
  threw = false;
  try {
    r->setText("");
  } catch (std::exception &e) {
    threw = true;
  }
  ASSERT_TRUE(threw);
  threw = false;
  try {
    r->setFunctional(false);
  } catch (std::exception &e) {
    threw = true;
  }
  ASSERT_TRUE(threw);
}

// Create, inspect and discard a change node
TEST(Requirement, TestChangeHappyRelationships) {
  auto r = std::make_shared<Requirement>();
  auto c = getChangeNode(r);
  ASSERT_TRUE(nullptr != c.get());
  ASSERT_EQ(c->getChangeParent().get(), r.get());
  r->discardChange();
  // We should also be able to safely call this if
  // change child is null
  r->discardChange();
  r->discardChange();
}

TEST(Requirement, TestChangeSadRelationships) {
  auto r = std::make_shared<Requirement>();
  r->commit();
  auto c = getChangeNode(r);
  c->commit();
  bool threw = false;
  try {
    r->discardChange();
  } catch (std::exception &e) {
    threw = true;
  }
  ASSERT_TRUE(threw);
}

TEST(Requirement, ToJson) {
  auto r = std::make_shared<Requirement>();
  r->init(); // Assign a uuid
  const std::string title("Requirement Title");
  const std::string text("Some requirement text");
  r->setTitle(title);
  r->setText(text);
  std::string json = r->to_json();
  ASSERT_GT(json.length(), 0);
}

TEST(Requirement, Serialization) {
  auto r = std::make_shared<Requirement>();
  r->init(); // Assign a uuid
  const std::string title("Requirement Title");
  const std::string text("Some requirement text");
  const std::string childTitle("Changed Requirement Title");
  const std::string childText(
      "Some requirement text with some additional text");
  r->setTitle(title);
  r->setText(text);
  r->commit();
  auto c = getChangeNode(r);
  c->init();
  c->setTitle(childTitle);
  c->setText(childText);
  boost::uuids::uuid parentId = r->id;
  boost::uuids::uuid childId = c->id;

  std::stringstream stream;
  {
    cereal::JSONOutputArchive archive(stream);
    archive(r);
  }
  c.reset();
  r.reset();
  r = std::make_shared<Requirement>();
  {
    cereal::JSONInputArchive archive(stream);
    archive(r);
  }
  ASSERT_EQ(r->id, parentId);
  ASSERT_EQ(r->getTitle(), title);
  ASSERT_EQ(r->getText(), text);
  ASSERT_TRUE(r->isCommitted());
  c = getChangeNode(r);
  ASSERT_TRUE(nullptr != r.get());
  ASSERT_EQ(c->id, childId);
  ASSERT_EQ(c->getTitle(), childTitle);
  ASSERT_EQ(c->getText(), childText);
  ASSERT_FALSE(c->isCommitted());
}

TEST(Requirement, SubRequirement) {
  std::string title("Parent Requirement");
  std::string childTitle("Child requirement");
  std::string anotherChild("AnotherChild");
  std::stringstream stream;
  auto r = std::make_shared<Requirement>();
  auto c = std::make_shared<Requirement>();
  auto d = std::make_shared<Requirement>();
  r->init();
  c->init();
  d->init();
  r->setTitle(title);
  c->setTitle(childTitle);
  d->setTitle(anotherChild);
  r->down.push_back(c);
  // Throw a couple of nodes in there for fun and profit
  r->down.push_back(std::make_shared<Node>());
  r->down.push_back(std::make_shared<Node>());
  r->down.push_back(d);
  {
    cereal::JSONOutputArchive archive(stream);
    archive(r);
  }
  // iterate children relationships
  bool foundChild = false;
  bool foundAnotherChild = false;
  for (auto childNode : r->down) {
    auto currentNode = std::dynamic_pointer_cast<Requirement>(childNode);
    if (currentNode) {
      if (currentNode->getTitle() == childTitle) {
        foundChild = true;
      }
      if (currentNode->getTitle() == anotherChild) {
        foundAnotherChild = true;
      }
    }
  }
  ASSERT_TRUE(foundChild);
  ASSERT_TRUE(foundAnotherChild);
}
