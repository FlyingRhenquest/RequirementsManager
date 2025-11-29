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
 *
 * A task node for a thread pool.
 */

#pragma once

#include <fr/RequirementsManager/Node.h>

namespace fr::RequirementsManager {

  template <typename WorkerType>
  class ThreadPool;
  
  // A task node is a generic node that can be run by a thread
  // pool. I need to make it a template so I can pass WorkerType on
  // to threadpool instances
  template <typename WorkerType>
  class TaskNode : public Node {
    std::string _name;
    // Threadpool this task gets assigned to
    // Threadpool will set this when the task
    // is assigned.
    std::shared_ptr<ThreadPool<WorkerType>> _owner;
    
  public:
    using Type = TaskNode;
    using PtrType = std::shared_ptr<TaskNode<WorkerType>>;
    using Parent = Node;
    
    TaskNode() {};
    virtual ~TaskNode() {};

    virtual void run() = 0;
    
    std::string getNodeType() const override {
      return "TaskNode";
    }
    
    std::string getName() const {
      return _name;
    };

    void setName(const std::string& name) {
      _name = name;
    }

    std::shared_ptr<ThreadPool<WorkerType>> getOwner() const {
      return _owner;
    }

    void setOwner(std::shared_ptr<ThreadPool<WorkerType>> owner) {
      _owner = owner;
    }

    template <class Archive>
    void save(Archive &ar) const {
      ar(cereal::make_nvp(Parent::getNodeType(), cereal::base_class<Parent>(this)));
      ar(cereal::make_nvp("name", _name));
    }

    template <class Archive>
    void load(Archive &ar) {
      ar(cereal::base_class<Parent>(this));
      ar(_name);
    }
    
  };

};
