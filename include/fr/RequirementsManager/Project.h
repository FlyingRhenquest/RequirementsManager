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

#pragma once
#include <fr/RequirementsManager/Node.h>
#include <list>
#include <memory>
#include <stdexcept>

namespace fr::RequirementsManager {

  /**
   * This is a project. Projects are owned by (appear in the down-lists
   * of) Organizations. Projects own requirements and other things
   * in their down-lists.
   *
   * TODO: Do projects need the same sorts of controls that I put
   * in requirement so we can track changes to them over time?
   * I'd guess probably so, but am not sure if they change at
   * this node level enough that it's necessary.
   */
  
  class Project : public Node {
  public:
    using Type = Node;
    using PtrType = std::shared_ptr<Project>;
    using Parent = Node;

  private:
    // Project Name
    std::string _name;
    std::string _description;

  public:

    Project() = default;
    virtual ~Project() = default;

    // I could just expose the variables directly right now
    // but want to the library to feel the same idomatically at
    // all levels

    std::string getNodeType() const override {
      return "Project";
    }

    
    void setName(const std::string &name) {
      _name = name;
    }

    void setDescription(const std::string &description) {
      _description = description;
    }

    std::string getName() const {
      return _name;
    }

    std::string getDescription() const {
      return _description;
    }

    template <class Archive>
    void save(Archive &ar) const {
      ar(cereal::make_nvp(Parent::getNodeType(), cereal::base_class<Parent>(this)));
      ar(cereal::make_nvp("name", _name));
      ar(cereal::make_nvp("description", _description));
    }

    template <class Archive>
    void load(Archive &ar) {
      ar(cereal::base_class<Parent>(this));
      ar(_name);
      ar(_description);
    }
    
  };
  
}

CEREAL_REGISTER_TYPE(fr::RequirementsManager::Project);
