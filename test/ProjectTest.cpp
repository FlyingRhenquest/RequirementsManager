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

#include <fr/RequirementsManager/Project.h>
#include <gtest/gtest.h>

using namespace fr::RequirementsManager;

TEST(Project, BasicFunctionality) {
  std::string projectName("First Project");
  std::string projectDesc("My first project!");
  auto project = std::make_shared<Project>();
  project->init();
  project->setName(projectName);
  project->setDescription(projectDesc);
  ASSERT_EQ(projectName, project->getName());
  ASSERT_EQ(projectDesc, project->getDescription());
}
