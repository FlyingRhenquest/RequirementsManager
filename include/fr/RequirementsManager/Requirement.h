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
#include <cereal/types/base_class.hpp>
#include <cereal/types/polymorphic.hpp>
#include <list>
#include <memory>
#include <string>
#include <sstream>

namespace fr::RequirementsManager {

  /**
   * RequirementNode contains information about a requirement.
   * This can include a title and longer description. They will
   * typically be owned by something (check its up nodes) and
   * that will generally be a project or another requirement.
   *
   * Requirements start uncommitted. Once committed they become
   * effectvely immutable. The way to change a requirement after
   * it has been committed is to generate a new requirement
   * and mark it as a change of the committed requirement.
   */
    
  class Requirement : public CommitableNode {
  public:
    using Type = Requirement;
    using PtrType = std::shared_ptr<Type>;
    using Parent = CommitableNode;
    template <typename T> friend std::shared_ptr<T> getChangeNode(std::shared_ptr<T> node);

  private:
    std::string _title;
    // Text of this requirement
    std::string _text;
    // Is this a functional or non-functional requirement
    bool _functional = false;

  public:

    Requirement() = default;
    // Default copy constructor will copy the ID -- you may want to
    // run init() on the copy to get a new uuid.
    Requirement(const Requirement& copy) = default;
    virtual ~Requirement() = default;

    std::string getNodeType() const override {
      return "Requirement";
    }

    // Sets the title -- will refuse to do so if the requirement is
    // committed.
    void setTitle(const std::string& title) {
      throwIfCommitted();
      _title = title;
    }
    // Sets the text of the requirement -- will refuse to do so if the
    // requirement is committed
    void setText(const std::string& text) {
      throwIfCommitted();
      _text = text;
    }
    // Sets the functional flag -- will refuse to do so if the requirement
    // is committed
    void setFunctional(bool functional) {
      throwIfCommitted();
      _functional = functional;
    }

    // Getters and setters since we want to restrict when we can change the
    // members of this object. These will all refuse to set their field if
    // the change has been committed. You must instead create a c
    
    std::string getTitle() const {
      return _title;
    }
    std::string getText() const {
      return _text;
    }
    bool isFunctional() const {
      return _functional;
    }
    
    // Serilization functions -- since the object we're inheriting
    // from uses load and save, we need to do the same (Can't switch
    // from load/save to serialize in inherited objects)

    template <class Archive>
    void save(Archive &ar) const {
      // Serialize node bits
      ar(cereal::make_nvp(Parent::getNodeType(), cereal::base_class<Parent>(this)));
      ar(cereal::make_nvp("title", _title));
      ar(cereal::make_nvp("text", _text));
      ar(cereal::make_nvp("functional", _functional));
    }

    template <class Archive>
    void load(Archive &ar) {
      // Serialize node bits
      ar(cereal::base_class<Parent>(this));
      ar(_title);
      ar(_text);
      ar(_functional);
    }
    
  };
}

CEREAL_REGISTER_TYPE(fr::RequirementsManager::Requirement);
  
