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
#include <cereal/types/base_class.hpp>
#include <cereal/types/polymorphic.hpp>
#include <list>
#include <memory>
#include <stdexcept>
#include <string>

namespace fr::RequirementsManager {

  /**
   * An Organization is just a thing that owns other nodes.
   *
   * An organization can directly own requirements. Those requirements
   * should be copied to any projects the Organization owns.
   */

  class Organization : public Node {
  public:
    using Type = Organization;
    using PtrType = std::shared_ptr<Type>;
    using Parent = Node;

  private:
    // Lock organization to prevent changes (You can still add/remove nodes though.)
    bool _locked = false;
    // Organization name
    std::string _name;
  public:

    Organization() = default;
    virtual ~Organization() = default;

    std::string getNodeType() const override {
      return "Organization";
    }

    bool isLocked() {
      return _locked;
    }
    
    void setName(const std::string &name) {
      if (!_locked) {
	_name = name;
      } else {
	throw std::logic_error("Organization is locked, can't make changes.");
      }
    }
    
    std::string getName() const {
      return _name;
    }

    void lock() {
      _locked = true;
    }

    void unlock() {
      _locked = false;
    }

    template <class Archive>
    void save(Archive &ar) const {
      ar(cereal::make_nvp(Parent::getNodeType(), cereal::base_class<Parent>(this)));
      ar(cereal::make_nvp("locked", _locked));
      ar(cereal::make_nvp("name", _name));
    }

    template <class Archive>
    void load(Archive &ar) {
      ar(cereal::base_class<Node>(this));
      ar(_locked);
      ar(_name);
    }

  };
  
}

CEREAL_REGISTER_TYPE(fr::RequirementsManager::Organization);
