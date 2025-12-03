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
 */

#pragma once

#include <boost/signals2.hpp>
#include <format>
#include <fr/RequirementsManager.h>
#include <fr/RequirementsManager/PqDatabaseSpecific.h>
#include <fr/RequirementsManager/Node.h>
#include <fr/RequirementsManager/TaskNode.h>
#include <fr/RequirementsManager/ThreadPool.h>
#include <fr/types/Concepts.h>
#include <fr/types/Typelist.h>
#include <pqxx/pqxx>
#include <string>
#include <unordered_map>
#include <vector>

namespace fr::RequirementsManager {

  /**
   * This is a node that can be used to save nodes
   * into the database. Submit it to a thread pool
   * and it'll just run whenever it can.
   *
   * What? Stop looking at me like that!
   */

  template <typename WorkerThreadType>
  class SaveNodesNode : public TaskNode<WorkerThreadType> {
    // Specific types I can save. These have functions in
    // PqSpecificDatabase.h that allow rows in the database
    // to be updated or inserted. If you've created some
    // other node that you want to save in the database,
    // you need to add a struct in that file that
    // can save and update it and then add it to the list
    // here.
    using SpecificSaveableTypes =
      fr::types::Typelist<Organization, Product, Project, Requirement, Story, UseCase,
                          Text, Completed, KeyValue, TimeEstimate, Effort, Role,
                          Actor, Goal, Purpose, Person, EmailAddress, PhoneNumber,
                          InternationalAddress, USAddress, Event>;

    // Set connection parameters up to connect to the database
    // from outside the application
    pqxx::connection _connection;
    pqxx::work _transaction;
    
    
    /**
     * Indicates that this node has exited its
     * run method.
     */

    bool _saveComplete;
    
    /**
     * Indicates only data in the stored node
     * should be saved. This will still save
     * the uuids in the immediate up/down links
     * but will not traverse the up/down links
     * beyond that. This will avoid an immediate
     * nodesplosion if you just want to save this
     * one thing.
     *
     * Nodes will only be saved if their changed flag
     * is true.
     */
    bool _saveThisNodeOnly;
    
    /**
     * As we traverse the graph we'll stash the saved node and its
     * UUID here. That way if we encounter the node elsewhere,
     * we won't try to traverse it again.
     */
    std::unordered_map<std::string, Node::PtrType> _alreadySaved;

    /**
     * Starting node -- Save this node and any other associated
     * nodes in its up/down lists if _saveThisNodeOnly is false.
     */

    Node::PtrType _startingNode;

    /**
     * Try to save specific data in a node shared ptr using the
     * SpecificSaveableTypes list I defined above. This will
     * generate all possible iterations of this list at compile
     * time and just go through the list trying to cast the node
     * to the current type we're examining. If it succeeds it will
     * return, otherwise it'll try the next type in the list until
     * it runs out of types. Since I define getNodeType() in all
     * my nodes, I do have the ability to short circuit this based
     * on node type if I need to. I might want to do that if I'm
     * saving a lot of raw nodes that only have an ID and
     * associations, but I don't think many people will be doing that.
     */
    
    template <typename List>
    constexpr void saveSpecificData(Node::PtrType node)
      requires
      (fr::types::IsTypelist<List> &&
       fr::types::IsUnique<List>)
    {
      // Try cast node and call the next typelist function if this one
      // isn't the right type
      using currentType = List::head::type;
      std::cout << "saveSpecificData: Trying " << database::DbSpecificData<currentType>::name << std::endl;
      std::shared_ptr<currentType> tryPtr = std::dynamic_pointer_cast<currentType>(node);
      if (tryPtr) {
        std::cout << "saveSpecificData: This node is a " << tryPtr->getNodeType() << std::endl;
        database::DbSpecificData<currentType> specificSaver;
        if (!database::nodeInTable<currentType>(tryPtr, _transaction)) {
          std::cout << "Inserting data" << std::endl;
          specificSaver.insert(tryPtr, _transaction);
        } else {
          std::cout << "Updating data" << std::endl;
          specificSaver.update(tryPtr, _transaction);
        }
      } else {
        // Try next type if there is one
        if constexpr(!std::is_void_v<typename List::tail::head::type>) {
          std::cout << "Trying next type" << std::endl;
          saveSpecificData<typename List::tail>(node);
        }
      }
      return;
    }
    
    /**
     * Returns true if the node is in the node table
     */
    bool nodeInDb(Node::PtrType node) {
      bool ret = false;
      std::string query("SELECT id FROM node WHERE id = $1");
      pqxx::params p{
        node->idString()
      };
      pqxx::result result = _transaction.exec(query, p);
      ret = (result.size() > 0);
      return ret;
    }

    // If the node exists we'll just clear out the existing node
    // associations and rewrite them. This will probably be faster
    // than iterating through the list in the database and only
    // saving new ones.
    void clearNodeDBAssociations(Node::PtrType node) {
      std::string query = std::format("DELETE FROM node_associations WHERE id = '{}'", node->idString());
      _transaction.exec(query);
    }

    /**
     * entrypoint for saving to the database. This must determine if the node
     * is already in the database and update it if it is. If the node is
     * to be updated rather than saved, its relationships should be cleared
     * from the database first and then re-saved, as node relationships
     * could have been removed from the node.
     *
     * dbSaveNode will not save nodes whose "changed" flag is false.
     * it will save node relationships but will not traverse into
     * other nodes (run does that part.)
     */
    void dbSaveNode(Node::PtrType node) {
      /**
       * Since base nodes don't have any unique information,
       * I don't have to do anything to the node table if
       * the node is already in the table. I just rewrite the
       * associations in case any of them changed.
       */

      if (nodeInDb(node)) {
        clearNodeDBAssociations(node);
      } else {
        std::string insertCmd("INSERT INTO node (id, node_type) VALUES($1,$2);");
        pqxx::params p{
          node->idString(),
          node->getNodeType()
        };
        _transaction.exec(insertCmd, p);
      }

      // Use a stream to update node_associations
      
      pqxx::stream_to stream = pqxx::stream_to::table(_transaction, {"public", "node_associations"}, {"id", "association", "type"});
      // Now we just need to write the node associations
      for (auto upNode : node->up) {
        stream.write_values(node->idString(), upNode->idString(), "up");
      }

      for (auto downNode : node->down) {
        stream.write_values(node->idString(), downNode->idString(), "down");
      }
      stream.complete();

      // Save any node-specific data in the database
      saveSpecificData<SpecificSaveableTypes>(node);
      
    };

    /**
     * Handle traversal, creation of SaveNodesNodes and enqueueing them
     * into the threadpool. Each enqueued saveNodesNode will be
     * set to save just that one node.
     */

    void traverse(Node::PtrType node) {
      std::cout << "Traverse : " << node->idString() << std::endl;
      auto owner = this->getOwner();
      if (!owner) {
        // This can happen if run is called directly rather
        // than from a threadpool object.
        std::cout << "WARNING: Owner is Null" << std::endl;
      }
      // set up to save this node
      _alreadySaved[node->idString()] = node;
      if (node->changed) {
        auto saver = std::make_shared<SaveNodesNode<WorkerThreadType>>(node, true);

        // Subscribe to saver complete signal and forward it back to the parent (this)
        // object.
        saver->complete.connect([&](const std::string& id, Node::PtrType n) {
          this->complete(id, n);
        });
        
        // Only enqueue work if we're in a thread pool
        if (owner) {
          std::cout << "Enqueuing saver for " << node->idString() << std::endl;
          owner->enqueue(saver);
        }
        // Record this worker as work done in this object.
        // I don't actually *have* to do this, but it allows me
        // to examine what this object did at a later date.
        this->down.push_back(saver);
      }

      for (auto upNode : node->up) {
        if (!_alreadySaved.contains(upNode->idString())) {
          _alreadySaved[upNode->idString()] = upNode;
          traverse(upNode);
        }
      }

      for (auto downNode : node->down) {
        if (!_alreadySaved.contains(downNode->idString())) {
          _alreadySaved[downNode->idString()] = downNode;
          traverse(downNode);
        }
      }

      // Check and see if this is a commitable node and if so
      // save additional nodes associated with those

      auto commitable = std::dynamic_pointer_cast<CommitableNode>(node);
      if (commitable) {
        auto parent = commitable->getChangeParent();
        if (parent && !_alreadySaved.contains(parent->idString())) {
          _alreadySaved[parent->idString()] = parent;;
          traverse(parent);
        }
        auto child = commitable->getChangeChild();
        if (child && !_alreadySaved.contains(child->idString())) {
          _alreadySaved[child->idString()] = child;
          traverse(child);
        }
      }

      // Check and see if this is an international address and if so
      // save the address line nodes associated with that

      auto interAddress = std::dynamic_pointer_cast<InternationalAddress>(node);
      if (interAddress) {
        if (interAddress->getAddressLines()) {
          traverse(interAddress->getAddressLines());
        }
      }
          
      // Same thing for US Address
      auto usAddress = std::dynamic_pointer_cast<USAddress>(node);
      if (usAddress) {
        if (usAddress->getAddressLines()) {
          traverse(usAddress->getAddressLines());
        }
      }
    }
    
  public:
    /**
     * Complete is a signals2 callback you can subscribe to for save notificaitons.
     * If you're saving an entire tree of nodes, the SaveNodesNode you create
     * will create additional ones to handle the other nodes in the tree and place
     * them in the down list of the SaveNodesNode you created. The SaveNodesNode
     * you created will register with and forward this signal from all the other
     * nodes it creates, so you can operate on individual nodes as they are saved.
     */
    boost::signals2::signal<void(const std::string&, Node::PtrType)> complete;

    SaveNodesNode(Node::PtrType startingNode,
                  bool saveThisNodeOnly = false) :
      _transaction(_connection),
      _saveComplete(false),
      _saveThisNodeOnly(saveThisNodeOnly),
      _startingNode(startingNode) {
    }

    virtual ~SaveNodesNode() = default;

    /**
     * Run saves this node and possibly all other associated nodes
     * depending on how saveThisNodeOnly is set.
     */
    void run() override {
      std::cout << "SaveNodesNodes::run" << std::endl;
      if (!this->initted) {
        this->init();
      }

      if (_startingNode->changed) {

        // Set the changed flag on the object to be saved to false.
        // This isn't technically true until commit is called,
        // but I want the object to be stored in the database in that
        // state. At some point if it's worth doing, I'll code up
        // a memento for this that I can roll back if we get an
        // exception or error before commit is called.
        _startingNode->changed = false;
        
        std::cout << "Saving " << _startingNode->idString() << std::endl;
        dbSaveNode(_startingNode);
        _alreadySaved[_startingNode->idString()] = _startingNode;
      }

      if (!_saveThisNodeOnly) {
        // Traverse the entire node tree and create and enqueue
        // individual SaveNodesNodes with _saveThisNodeOnly enabled
        // if the node is not in the already saved list and the node
        // is marked "changed"
        std::cout << "Traversing up" << std::endl;;
        for (auto node : _startingNode->up) {
          traverse(node);
        }
        std::cout << "Traversing down" << std::endl;
        for (auto node : _startingNode->down) {
          traverse(node);
        }
      }
      
      _transaction.commit();
      _saveComplete = true;
      complete(_startingNode->idString(), _startingNode);
    };

    bool saveComplete() const {
      return _saveComplete;
    }

    // Indicates that all objects saved as part of this
    // object have saved
    bool treeSaveComplete() {
      bool ret = _saveComplete;
      if (!_saveThisNodeOnly) {
        for (auto node : this->down) {
          auto saveNode = dynamic_pointer_cast<SaveNodesNode>(node);
          if (saveNode) {
            ret &= saveNode->treeSaveComplete();
          }
        }
      }
      return ret;
    }
      
  };

}
