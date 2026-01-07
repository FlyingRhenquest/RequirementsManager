/**
 * Copyright 2026 Bruce Ide
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

#include <fr/RequirementsManager/AllNodeTypes.h>
#include <format>
#include <fr/RequirementsManager.h>
#include <fr/RequirementsManager/PqDatabaseSpecific.h>
#include <fr/RequirementsManager/Node.h>
#include <fr/RequirementsManager/TaskNode.h>
#include <fr/RequirementsManager/ThreadPool.h>
#include <fr/types/Concepts.h>
#include <fr/types/Typelist.h>
#include <fteng/signals.hpp>
#include <pqxx/pqxx>
#include <memory>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

namespace fr::RequirementsManager {

  /**
   * This is a node that removes nodes and graphs from a
   * database.
   *
   * This does not work the same way that SaveNodesNode does.
   * Instead you add any nodes you want to this node's down
   * list with Node::addDown. You can put nodes or graphs or
   * both in the down list, with no limit to the number.
   * This object will just iterate thorugh them all. If graphs
   * are stored in this node, they will be entirely removed,
   * recursively.
   *
   * This does not signal that it's complete, as you can just
   * plunk it into a threadpool and just wait for the threadpool
   * to join or go off to do other things. Whenever the threadpool
   * joins, this node is done removing things.
   */

  template <typename WorkerThreadType>
  class RemoveNodesNode : public TaskNode<WorkerThreadType> {
    using RemovableTypes = AllNodeTypes;
    pqxx::connection _connection;
    pqxx::work _transaction;

    /**
     * Indicates this node has exited its run method
     */
    bool _removeComplete;

    template <typename List>
    constexpr void removeData(std::shared_ptr<Node> node)
      requires fr::types::IsUnique<List> {
      using currentType = List::head::type;
      std::shared_ptr<currentType> tryPtr = std::dynamic_pointer_cast<currentType>(node);
      if (tryPtr) {
        database::DbSpecificData<currentType> remover;
        remover.remove(tryPtr, _transaction);
      } else {
        if constexpr(!std::is_void_v<typename List::tail::head::type>) {
          removeData<typename List::tail>(node);
        }
      }
    }
    
  public:

    RemoveNodesNode() : _removeComplete(false), _transaction(_connection) {}
    virtual ~RemoveNodesNode() {}
    
    void run() override {
      if (!this->initted) {
        this->init();
      }

      for (auto node : this->down) {
        node->traverse([&](std::shared_ptr<Node> n){
          std::cout << "Remove " << n->idString() << std::endl;
          this->removeData<RemovableTypes>(n);
        });
      }

      _removeComplete = true;
    }
    
  };
  
}
