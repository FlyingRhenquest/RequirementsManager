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
   * This is a story node you can drop in the down-list of a
   * requirement or the up-list of some other node (Design,
   * test plan, et al) so you can keep track of your stories.
   *
   * There are some nodes in UtilityNodes.h that would make
   * sense to drop into down nodes here (role, estimates etc)
   */

  class Story : public CommitableNode {

    std::string _title;
    std::string _goal;
    std::string _benefit;

  public:

    using Type = Story;
    using Parent = CommitableNode;
    using PtrType = std::shared_ptr<Type>;
    
    Story() = default;
    virtual ~Story() = default;
    
    std::string getNodeType() const override {
      return "Story";
    }

    std::string getTitle() const {
      return _title;
    }

    void setTitle(const std::string& title) {
      throwIfCommitted();
      _title = title;
    }
    
    std::string getGoal() const {
      return _goal;
    }

    void setGoal(const std::string &goal) {
      throwIfCommitted();
      _goal = goal;
    }

    std::string getBenefit() const {
      return _benefit;
    }

    void setBenefit(const std::string &benefit) {
      throwIfCommitted();
      _benefit = benefit;
    }

    template <class Archive>
    void save(Archive &ar) const {
      ar(cereal::make_nvp(Parent::getNodeType(), cereal::base_class<Parent>(this)));
      ar(cereal::make_nvp("title", _title));
      ar(cereal::make_nvp("goal", _goal));
      ar(cereal::make_nvp("benefit", _benefit));
    }

    template <class Archive>
    void load(Archive &ar) {
      ar(cereal::base_class<Parent>(this));
      ar(_title);
      ar(_goal);
      ar(_benefit);
    }

  };
  
}

CEREAL_REGISTER_TYPE(fr::RequirementsManager::Story);
