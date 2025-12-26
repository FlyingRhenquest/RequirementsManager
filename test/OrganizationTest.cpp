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

#include <fr/RequirementsManager/Organization.h>
#include <gtest/gtest.h>

using namespace fr::RequirementsManager;

TEST(Organization, BasicFunctionality) {
  auto org = std::make_shared<Organization>();
  org->init();
  std::string orgName("Global Consolidated Software Engineering, Inc.");
  org->setName(orgName);
  ASSERT_EQ(orgName, org->getName());
  org->lock();
  bool threw = false;
  try {
    org->setName("Inc, Inc.");
  } catch (std::exception &e) {
    threw = true;
  }
  ASSERT_TRUE(threw);
}

TEST(Organization, TestToJson) {
  auto org = std::make_shared<Organization>();
  org->init();
  org->setName("Inc, Inc.");
  auto json = org->to_json();
  // Search for fields in the JSON
  ASSERT_NE(json.find("id"), std::string::npos);
  ASSERT_NE(json.find("locked"), std::string::npos);
  ASSERT_NE(json.find("name"), std::string::npos);
  ASSERT_NE(json.find("Inc, Inc."), std::string::npos);
}
