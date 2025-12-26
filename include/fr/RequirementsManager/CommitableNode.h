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

namespace fr::RequirementsManager {

  // Returns a change node from this or children of this.
  // If current change child is nullptr, it creates a new one

  template <typename T>
  std::shared_ptr<T> getChangeNode(std::shared_ptr<T> node) {
    if (nullptr == node->_changeChild) {
      node->_changeChild = std::make_shared<T>();
      node->_changeChild->init();
      node->_changeChild->_changeParent = node;
    }
    return std::dynamic_pointer_cast<T>(node->_changeChild);
  }

  /**
   * A committable node that will throw if you try to set values
   * after the node has been committed. Note that you should still
   * be able to add nodes to this node's up and down lists, as
   * you'll generally want to use the committted flag in conjunction
   * with another node of the same type in the down-list to indicate
   * a change to the original Node data. This allows changes in the
   * system to be tracked over time.
   *
   * This node provides a function, "throwIfCommitted()" which will
   * throw a std::logic_error that this node is committed and its
   * data can not be changed. Set any data that should not be changed
   * to private and call throwIfCommittted() prior to setting the
   * data.
   *
   */

  class CommitableNode : public Node {
    using Type = CommitableNode;
    using PtrType = std::shared_ptr<Type>;
    using Parent = Node;

    template <typename T> friend std::shared_ptr<T> getChangeNode(std::shared_ptr<T> node);

        
  protected:

    void traverse(std::function<void(Node::PtrType)> eachNodeFn, std::unordered_map<std::string, Node::PtrType> &visited) override {
      Parent::traverse(eachNodeFn, visited);

      if (_changeParent && !visited.contains(_changeParent->idString())) {
        _changeParent->traverse(eachNodeFn, visited);
      }
      if (_changeChild && !visited.contains(_changeChild->idString())) {
        _changeChild->traverse(eachNodeFn, visited);
      }
    }

    bool _committed = false;
    // Parent node that this one changes. Will be nullptr if this
    // is the ultimate parent node.
    PtrType _changeParent;
    // Child node that changes this one. Will be nullptr if there are
    // no more changes.
    PtrType _changeChild;

  public:

    CommitableNode() = default;
    virtual ~CommitableNode() = default;

    std::string getNodeType() const override {
      return "CommitableNode";
    }

    // Commit the node. Once you do this, the only way to change
    // the node is to add a changeChild to it.
    
    void commit() {
      _committed = true;
    }

    bool isCommitted() const {
      return _committed;
    }

    // Return changeChild node. You need to check this for nullptr
    // prior to trying to use it.
    PtrType getChangeChild() {
      return _changeChild;
    }

    void traverse(std::function<void(Node::PtrType)> eachNodeFn) override {
      Parent::traverse(eachNodeFn);
    }
    
    // Return changeParent node. You need to check this for nullptr
    // prior to trying to use it.
    PtrType getChangeParent() {
      return _changeParent;
    }

    /**
     * This will traverse all changes until it finds the last one
     * and drop your change in there.
     */
    
    void addChangeChild(PtrType child) {
      if (_changeChild.get()) {
	PtrType currentChange = getChangeChild();
	while(currentChange->getChangeChild().get()) {
	  currentChange = currentChange->getChangeChild();
	}
	currentChange->addChangeChild(child);
      } else {
	_changeChild = child;
      }
    }

    // Discards an uncommitted change. This only checks the immediate
    // child node
    void discardChange() {
      // Need to check for null before checking for isCommitted.
      // Otherwise we could just reset the pointer without worrying
      // about it. Since we don't know the state when we hit the
      // else, I need to check for nullptr again to know if I have
      // to throw.
      if (nullptr != _changeChild.get() && !_changeChild->isCommitted()) {
	_changeChild.reset();
      } else {
	if (nullptr != _changeChild.get()) {
	  throw std::logic_error("NOTDISCARDED: Can not discard a committed change.");
	}
      }
    }

    // Throws a logic error if this node is committed
    void throwIfCommitted() {
      if (_committed) {
	throw std::logic_error("NOTCHANGED: Node is committed. Add a change node to make a change");
      }
    }

    template <class Archive>
    void save(Archive &ar) const {
      ar(cereal::make_nvp(Parent::getNodeType(), cereal::base_class<Parent>(this)));
      ar(cereal::make_nvp("committed", _committed));
      ar(cereal::make_nvp("changeParent", _changeParent));
      ar(cereal::make_nvp("changeChild", _changeChild));
    }

    template <class Archive>
    void load(Archive &ar) {
      ar(cereal::base_class<Parent>(this));
      ar(_committed);
      ar(_changeParent);
      ar(_changeChild);
    }
    
  };

}

CEREAL_REGISTER_TYPE(fr::RequirementsManager::CommitableNode);
