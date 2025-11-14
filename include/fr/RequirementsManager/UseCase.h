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

#include <fr/RequirementsManager/CommitableNode.h>
#include <fr/RequirementsManager/UtilityNodes.h>
#include <string>

namespace fr::RequirementsManager {

  /**
   * A node to hold all the other nodes that make up a use case
   */
  
  class UseCase : public CommitableNode {
    // Use Case Name
    std::string _name;

  public:
    using Type = UseCase;
    using Parent = CommitableNode;
    using PtrType = std::shared_ptr<Type>;

    std::string getNodeType() const override {
      return "UseCase";
    }
    
    void setName(const std::string &name) {
      throwIfCommitted();
      _name = name;
    }

    std::string getName() const {
      return _name;
    }

    template <typename Archive>
    void save(Archive& ar) const {
      ar(cereal::make_nvp(Parent::getNodeType(), cereal::base_class<Parent>(this)));
      ar(cereal::make_nvp("name", _name));
    }
    
    template <typename Archive>
    void load(Archive& ar) {
      ar(cereal::base_class<Parent>(this));
      ar(_name);
    }
  };
  
}

CEREAL_REGISTER_TYPE(fr::RequirementsManager::UseCase);
