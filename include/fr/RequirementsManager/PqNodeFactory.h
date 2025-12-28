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

#include <atomic>
#include <boost/signals2.hpp>
#include <cstring>
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
   * This class allocates a Node for Database loading.
   * The nodes are stored using a type string translation
   * stored in PqDatabaseSpecific.h. I also store the node
   * type with a "using" in there, so all I need to do
   * is reverse the process to create a node.
   *
   * The freshly allocated node will not yet be populated
   * with data but the node allocator will set its UUID
   * to one provided by the user. This will allow me to
   * just shovel freshly allocated nodes into a loader
   * and have them loaded with the data from the database.
   * You need to look up the UUID to look up the type anyway.
   */
  
  class NodeAllocator {
    using NodeList =
      fr::types::Typelist<GraphNode, Organization, Product, Project, Requirement, Story, UseCase,
                          Text, Completed, KeyValue, TimeEstimate, Effort, Role,
                          Actor, Goal, Purpose, Person, EmailAddress, PhoneNumber,
                          InternationalAddress, USAddress, Event>;


    /**
     * Handles the actual node lookup from public get entrypoint
     */
    template <typename List>
    constexpr Node::PtrType getNode(const std::string &nodeType, const std::string& uuid)
      requires
      (fr::types::IsTypelist<List> &&
       fr::types::IsUnique<List>)
    {
      // Recurse through the list of nodes and see if this node's type
      // matches the one that was passed in to us.
      using currentType = List::head::type;
      Node::PtrType workingNode;
      if (nodeType == database::DbSpecificData<currentType>::name) {
        workingNode = std::make_shared<currentType>();
        workingNode->setUuid(uuid);           
      } else {
        // Keep iterating down the list. If we hit void, we're at the
        // end of the list.
        if constexpr (!std::is_void_v<typename List::tail::head::type>) {
          return getNode<typename List::tail>(nodeType, uuid);
        }
      }
      // Check to see if WorkingNode points to anything.
      // If not, it's possible that this is a saved raw node
      // in the database, and we'll just return a raw node
      // with the UUID set so you never get a nullptr back.
      if (!workingNode) {
        workingNode = std::make_shared<Node>();
        workingNode->setUuid(uuid);
      }
      return workingNode;
    }
    
  public:

    NodeAllocator() = default;
    ~NodeAllocator() = default;
    
    Node::PtrType get(const std::string& nodeType, const std::string& uuid) {
      return getNode<NodeList>(nodeType, uuid);
    }
  };

  /**
   * PqNodeLoader takes a node from the allocator and loads it with data.
   * This is derived from TaskNode and is designed to be submitted to a
   * Threadpool for loading. It only loads the information for that one
   * node and does not handle setting up the links.
   */

  template <typename WorkerThreadType>
  class PqNodeLoader : public TaskNode<WorkerThreadType> {
    using NodeList =
      fr::types::Typelist<GraphNode, Organization, Product, Project, Requirement, Story, UseCase,
                          Text, Completed, KeyValue, TimeEstimate, Effort, Role,
                          Actor, Goal, Purpose, Person, EmailAddress, PhoneNumber,
                          InternationalAddress, USAddress, Event>;
    pqxx::connection _connection;
    pqxx::work _transaction;

    // Indicates this loading task has completed.
    std::atomic<bool> _loadComplete;
    // Indicates data was found. This is returned from DbSpecificData::load
    std::atomic<bool> _found;
    // Node Pointer to load
    Node::PtrType _node;

    NodeAllocator _allocator;

    // Recusively iterate typelist to load a node

    template <typename List>
    constexpr void load()
    requires
      (fr::types::IsTypelist<List> &&
       fr::types::IsUnique<List>)
    {
      using CurrentType = List::head::type;
      auto cast = std::dynamic_pointer_cast<CurrentType>(_node);
      if (cast) {
        database::DbSpecificData<CurrentType> specificLoader;
        _found = specificLoader.load(cast, _transaction);
      } else {
        if constexpr (!std::is_void_v<typename List::tail::head::type>) {
          load<typename List::tail>();
        }
      }
    }
    
  public:
    using Type = PqNodeLoader<WorkerThreadType>;
    using PtrType = std::shared_ptr<PqNodeLoader<WorkerThreadType>>;
    using Parent = TaskNode<WorkerThreadType>;

    boost::signals2::signal<void(const std::string&, Node::PtrType)> loaded;

    PqNodeLoader(Node::PtrType toLoad) : _transaction(_connection),
                                         _loadComplete(false),
                                         _found(false),
                                         _node(toLoad) {
    }
    
    virtual ~PqNodeLoader() {}

    std::string getNodeType() const override {
      return "PqNodeLoader";
    }

    void run() override {
      load<NodeList>();
      _loadComplete = true;
      loaded(_node->idString(), _node);
    }

    // Returns true if this loader has run. found will determine if the node
    // to be loaded was found.
    bool complete() const {
      return _loadComplete;
    }

    bool found() const {
      return _found;
    }
    
  };

  /**
   * PqNodeFactory assembles a graph given a Node ID. As it iterates through the
   * list of nodes associated with the given Node ID, it uses NodeAllocator to
   * allocate them then handles assembling the graph. It dispatches all the
   * allocated nodes to PqNodeLoader to load the data associated with the
   * node while the factory works on assembling the graph. Once the graph is
   * fully assembled, it's handed back to the user.
   */

  template <typename WorkerType>
  class PqNodeFactory : public TaskNode<WorkerType> {
    // Initial UUID to load
    std::string _loadUuid;

    // Map used to keep track of which UUIDs we've already loaded
    std::unordered_map<std::string, Node::PtrType> _alreadyLoaded;

    // Set to true at the end of run
    bool _graphLoaded;

    Node::PtrType _startingNode;

    pqxx::connection _connection;
    pqxx::work _transaction;
    NodeAllocator _allocator;

    // Look up node type in database
    std::string getNodeType(const std::string& uuid) {
      std::string ret;
      std::string cmd("SELECT node_type FROM node where id = $1");
      pqxx::params p{
        uuid
      };
      pqxx::result res = _transaction.exec(cmd, p);
      if (res.size()) {
        for (auto const &row : res) {
          ret = row[0].as<std::string>();
        }
      }
      return ret;
    }

    Node::PtrType startLoading(std::string uuid) {
      std::string nodeType = getNodeType(uuid);
      Node::PtrType ret;
      if (!nodeType.empty()) {
        ret = _allocator.get(nodeType, uuid);
        // Make sure initted gets set true or nodes may get
        // re-initted when they shouldn't be
        ret->initted = true;
      }
      return ret;
    }

    // Only add the node if it's not already in the list
    void addToUpDown(std::vector<Node::PtrType>& upDown, Node::PtrType toAdd) {
      bool found = false;
      for (auto& node : upDown) {
        if (node->idString() == toAdd->idString()) {
          found = true;
        }
      }
      if (!found) {
        upDown.push_back(toAdd);
      }
    }
    
    void process(Node::PtrType node) {
      auto owner = this->getOwner();
      // Dispatch node to be populated
      _alreadyLoaded[node->idString()] = node;
      auto worker = std::make_shared<PqNodeLoader<WorkerType>>(node);

      // Forward worker loaded signal through the factory
      worker->loaded.connect([&](const std::string& id, Node::PtrType n) {
        this->loaded(id, n);
        // graphLoaded will signal done when all the worker nodes are
        // finished processing
        this->graphLoaded();
      });
      
      this->down.push_back(worker);
      owner->enqueue(worker);
      // Iterate through the up/down lists from node_association, load
      // and assemble the associated nodes.
      std::string cmd("select association,type from node_associations where id = $1;");
      pqxx::params p{
        node->idString()
      };
      pqxx::result res = _transaction.exec(cmd,p);
      for (auto const &row : res) {
        auto association = row[0].as<std::string>();
        auto assocType = row[1].as<std::string>();
        Node::PtrType nextNode;
        if(_alreadyLoaded.contains(association)) {
          nextNode = _alreadyLoaded.at(association);
        } else {
          nextNode = startLoading(association);
          if (nextNode) {
            process(nextNode);
          }
        }
        if (assocType == "up") {
          addToUpDown(node->up, nextNode);
        } else {
          addToUpDown(node->down, nextNode);
        }
      }
    }

  public:

    boost::signals2::signal<void(const std::string&, Node::PtrType)> loaded;
    boost::signals2::signal<void(const std::string&)> done;
    
    PqNodeFactory(const std::string& uuidToLoad) :
      _loadUuid(uuidToLoad),
      _graphLoaded(false),
      _transaction(_connection) {
    }

    virtual ~PqNodeFactory() {
    }

    std::string getNodeType() const override {
      return "PqNodeFactory";
    }

    void run() override {
      auto owner = this->getOwner();
      if (!owner) {
        // If we're not run through a threadpool, create a threadpool
        // to use.
        auto pool = std::make_shared<ThreadPool<WorkerType>>();
        this->setOwner(pool);
        pool->startThreads(4);
      }

      _startingNode = startLoading(_loadUuid);
      if (_startingNode) {
        process(_startingNode);
      }
    }

    Node::PtrType getNode() {
      return _startingNode;
    }

    bool graphLoaded() {
      // We need to check all the workers to see if they're done
      if (!_graphLoaded) {
        // This will go false again if any workernodes are still working
        _graphLoaded = true;
        for (auto node : this->down) {
          auto workerNode = dynamic_pointer_cast<PqNodeLoader<WorkerType>>(node);
          if (workerNode) {
            _graphLoaded &= workerNode->complete();            
          }
        }
        if (_graphLoaded) {
          // Signal done if we're still true here
          done(_loadUuid);
        }
      }
      return _graphLoaded;
    }
    
  };
  
}
