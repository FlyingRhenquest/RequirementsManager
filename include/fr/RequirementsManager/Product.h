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

namespace fr::RequirementsManager {

  /**
   * Product -- this is another thing that I thing should just be
   * assembled from other nodes, so it's fairly lightweight.
   * I just need to add nodes for Purpose, Goals, Features,
   * Functionality, Scope, Constraints, Release Criteria and
   * Stakeholders. I'm getting faster at banging these out now
   * but between writing the objects, tests and PythonAPI,
   * it still takes three or four hours to comfortably
   * put a new node in place, and I still don't have any
   * logic that actually defines how they fit together.
   * So I'd like to write some of THAT before I go crazy
   * adding a bunch more nodes.
   */
  
  class Product : public CommitableNode {

    std::string _title;
    std::string _description;

  public:

    using Type = Product;
    using PtrType = std::shared_ptr<CommitableNode>;
    using Parent = CommitableNode;
    
    Product() = default;
    virtual ~Product() = default;

    void setTitle(const std::string& title) {
      throwIfCommitted();
      _title = title;
    }

    void setDescription(const std::string& description) {
      throwIfCommitted();
      _description = description;
    }

    std::string getTitle() const {
      return _title;
    }

    std::string getDescription() const {
      return _description;
    }

    template <class Archive>
    void save(Archive &ar) const {
      ar(cereal::make_nvp(Parent::getNodeType(), cereal::base_class<Parent>(this)));
      ar(_title);
      ar(_description);      
    }

    template <class Archive>
    void load(Archive &ar) {
      ar(cereal::base_class<Parent>(this));
      
    }
    
  };
  
}

CEREAL_REGISTER_TYPE(fr::RequirementsManager::Product);
