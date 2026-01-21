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

#include <fr/RequirementsManager.h>
#include <pqxx/pqxx>
#include <format>
#include <stdexcept>
#include <string>

/**
 * I'm storing structures here that can be queried to
 * retrieve an update or insert command for a specific
 * node time.
 *
 * These are looked up by static constexpr data (name
 * and tableName) but I create an instance of the
 * class to actually save the objects. So all the
 * specialized things can inherit from my
 * specialized node and that's fine.
 * The non-specialized thing isn't inherited
 * and doesn't inherit from anything else and that's
 * also fine.
 */

namespace fr::RequirementsManager::database {

  /**
   * This is the notfound type
   */
  
  template <typename T>
  struct DbSpecificData {
    using type = void;
    static constexpr char name[] = "NOTFOUND";
    static constexpr char tableName[] = "NOTFOUND";

    void insert(Node::PtrType n, pqxx::work& transaction) {
      throw std::logic_error("Attempted to insert an unknown node type (type not specialized in PqDatabaseSpecific.h)");
    }

    void update(Node::PtrType n, pqxx::work& transaction) {
      throw std::logic_error("Attempted to update an unknown node type (type not specialized in PqDatabaseSpecific.h)");
    }

    /**
     * A node or any other unknown type will end up here. On the
     * off chance we save a raw node, I don't want to throw for
     * it. There's no specific data that needs to be loaded
     * for node, so it'll just return immediately. It returns
     * false, so a NodeLoader would flag it as notfound since
     * there shouldn't generally be raw nodes in a database,
     * but if you intend to store some you should be aware of
     * that. It's not conisdered an error anywhere at the moment,
     * just information so you can investigate if you need to.
     */
    bool load(Node::PtrType n, pqxx::work& transaction) {
      return false;
    }

    void remove(Node::PtrType n, pqxx::work& transaction) {
      throw std::logic_error("Attempted to remove an unknown node type (type not specialized in PqDatabaseSpecific.h)");
    }
  };

  /************************************************************/
  // Node -- All Other Things Inherit From This, but not virtually.
  // Since all the other functions' insert,update,load and remove
  // methods use the specialized node types, they don't conflict
  // with the base node definitions, BUT we can still call
  // the parent node methods with Parent::insert et al.
  //
  // We don't need to call Parent::load for anything, since
  // the parent class one is a no-op

  template <>
  struct DbSpecificData<Node> {
    using Type = Node;
    using PtrType = Type::PtrType;

    static constexpr char name[] = "Node";
    static constexpr char tableName[] = "node";
    
    void insert(PtrType node, pqxx::work& transaction) {
      // Insert Node Into node
      std::string insertCmd("INSERT INTO node (id, node_type) VALUES($1,$2);");
      pqxx::params p{
        node->idString(),
        node->getNodeType()
      };
      transaction.exec(insertCmd, p);

      // Write node associations
      // These get cleared in PqDatabase.h if
      // the node was already in the database, so
      // they need to be rewritten if we're inserting
      // or updating.
      update(node, transaction);
    }

    void update(PtrType node, pqxx::work& transaction) {
      // Stream node associations into node_associations
      pqxx::stream_to stream = pqxx::stream_to::table(transaction,
            {"public", "node_associations"}, {"id", "association", "type"});

      for (auto upNode : node->up) {
        stream.write_values(node->idString(), upNode->idString(), "up");
      }

      for (auto downNode : node->down) {
        stream.write_values(node->idString(), downNode->idString(), "down");
      }
      stream.complete();
      transaction.commit();
    }

    bool load(PtrType node, pqxx::work& transaction) {
      // This is a no-op in node, because all the required node data
      // gets loaded in the other specific types.

      // Returns true because all the data stored in the node passed
      // in is all the data that is in the database for this node.
      // The only relevant thing in there is the UUID.
      return true;
    }

    void remove(PtrType node, pqxx::work& transaction) {
      // Remove all associations for this node
      std::string rmAssociations("DELETE FROM node_associations WHERE id = $1 OR association = $1");
      // Remove from node
      std::string rmNode("DELETE FROM node WHERE id = $1");
      pqxx::params p{
        node->idString()
      };
      // Run the deletes
      transaction.exec(rmAssociations,p);
      transaction.exec(rmNode,p);
      transaction.commit();
    }
  };


  /************************************************************/
  // GraphNode-Specific type

  template <>
  struct DbSpecificData<GraphNode> : public DbSpecificData<Node> {
    using Type = GraphNode;
    using PtrType = Type::PtrType;
    using Parent = DbSpecificData<Node>;
    
    static constexpr char name[] = "GraphNode";
    static constexpr char tableName[] = "graph_node";

    void insert(PtrType node, pqxx::work& transaction) {
      Parent::insert(node,transaction);
      std::string cmd = std::format("INSERT INTO {} (id,title) VALUES ($1, $2);", tableName);
      pqxx::params p{
        node->idString(),
        node->getTitle()
      };
      transaction.exec(cmd, p);
    }

    void update(PtrType node, pqxx::work& transaction) {
      Parent::update(node, transaction);
      std::string cmd = std::format("UPDATE {} SET title = $1 WHERE id = $2;", tableName);
      pqxx::params p{
        node->getTitle(),
        node->idString()
      };
      transaction.exec(cmd,p);
    }

    bool load(PtrType node, pqxx::work& transaction) {
      bool ret = false;
      std::string cmd = std::format("select * from {} WHERE ID = $1", tableName);
      pqxx::params p{
        node->idString()
      };
      pqxx::result res = transaction.exec(cmd, p);
      if (res.size() > 0) {
        ret = true;
      }

      for (auto const &row : res) {
        node->setTitle(row["title"].as<std::string>());
      }
      return ret;
    }

    void remove(PtrType node, pqxx::work& transaction) {
      Parent::remove(node, transaction);
      // Delete from specific table where this object lives
      std::string cmd = std::format("DELETE from {} WHERE ID = $1", tableName);
      pqxx::params p{
        node->idString()
      };
      transaction.exec(cmd, p);
    }
    
  };
  
  /************************************************************/
  // Organization-Specific type
  
  template <>
  struct DbSpecificData<Organization> : public DbSpecificData<Node> {
    using Type = Organization;
    using PtrType = Type::PtrType;
    using Parent = DbSpecificData<Node>;
    
    static constexpr char name[] = "Organization";
    static constexpr char tableName[] = "organization";

    void insert(PtrType node, pqxx::work& transaction) {
      Parent::insert(node, transaction);
      std::string cmd("INSERT INTO organization (id, locked, name) VALUES ($1, $2, $3);");
      pqxx::params p{
        node->idString(),
        node->isLocked(),
        node->getName()
      };
      transaction.exec(cmd, p);
    }

    void update(PtrType node, pqxx::work& transaction) {
      Parent::update(node, transaction);
      std::string cmd("UPDATE organization SET name = $1, locked = $2 WHERE id = $3;");
      pqxx::params p{
        node->getName(),
        node->isLocked(),
        node->idString()
      };
      transaction.exec(cmd, p);
    }

    bool load(PtrType node, pqxx::work& transaction) {
      bool ret = false;
      std::string cmd = std::format("SELECT * from {} WHERE id = $1;", tableName);
      pqxx::params p{
        node->idString()
      };
      pqxx::result res = transaction.exec(cmd, p);

      if (res.size() > 0) {
        ret = true;
      }
      // It's not an error if the object isn't found. I'm taking a small
      // performance hit loading by name rather than column number, but
      // if the tables ever change this will be much easier to maintain.
      // I can revisit this if database performance proves to be an issue,
      // but I suspect it will not.
      for (auto const &row : res) {
        node->setName(row["name"].as<std::string>());
        if (row["locked"].as<bool>()) {
          node->lock();
        }
      }
      return ret;
    }

    void remove(PtrType node, pqxx::work& transaction) {
      Parent::remove(node, transaction);
      std::string cmd = std::format("DELETE from {} WHERE ID = $1", tableName);
      pqxx::params p{
        node->idString()
      };
      transaction.exec(cmd, p);
    }
    
  };

  /**********************************************************/
  // Product-specific type
  
  template <>
  struct DbSpecificData<Product> : public DbSpecificData<Node> {
    using Type = Product;
    using PtrType = Type::PtrType;
    using Parent = DbSpecificData<Node>;
    
    static constexpr char name[] = "Product";
    static constexpr char tableName[] = "product";

    void insert(PtrType node, pqxx::work& transaction) {
      Parent::insert(node, transaction);
      std::string cmd("INSERT INTO product (id,title,description) VALUES ($1, $2, $3);");
      pqxx::params p{
        node->idString(),
        node->getTitle(),
        node->getDescription()
      };
      transaction.exec(cmd, p);
    }

    void update(PtrType node, pqxx::work& transaction) {
      Parent::update(node, transaction);
      std::string cmd("UPDATE product SET title = $1, description = $2 WHERE id = $3;");
      pqxx::params p{
        node->getTitle(),
        node->getDescription(),
        node->idString()
      };
      transaction.exec(cmd, p);
    }

    bool load(PtrType node, pqxx::work& transaction) {
      bool ret = false;
      std::string cmd = std::format("SELECT * from {} WHERE id = $1;", tableName);
      pqxx::params p{
        node->idString()
      };
      pqxx::result res = transaction.exec(cmd, p);

      if (res.size() > 0) {
        ret = true;
      }
      // It's not an error if the object isn't found. I'm taking a small
      // performance hit loading by name rather than column number, but
      // if the tables ever change this will be much easier to maintain.
      // I can revisit this if database performance proves to be an issue,
      // but I suspect it will not.
      for (auto const &row : res) {
        node->setTitle(row["title"].as<std::string>());
        node->setDescription(row["description"].as<std::string>());
      }
      return ret;
    }
    
    void remove(PtrType node, pqxx::work& transaction) {
      Parent::remove(node, transaction);
      std::string cmd = std::format("DELETE from {} WHERE ID = $1", tableName);
      pqxx::params p{
        node->idString()
      };
      transaction.exec(cmd, p);
    }

  };

  /***********************************************************/
  // Project-specific type

  template <>
  struct DbSpecificData<Project> : public DbSpecificData<Node> {
    using Type = Project;
    using PtrType = Type::PtrType;
    using Parent = DbSpecificData<Node>;
    
    static constexpr char name[] = "Project";
    static constexpr char tableName[] = "project";

    void insert(PtrType node, pqxx::work& transaction) {
      Parent::insert(node, transaction);
      std::string cmd("INSERT INTO project (id,name,description) VALUES ($1, $2, $3);");
      pqxx::params p{
        node->idString(),
        node->getName(),
        node->getDescription()
      };
      transaction.exec(cmd, p);
    }

    void update(PtrType node, pqxx::work& transaction) {
      Parent::update(node, transaction);
      std::string cmd("UPDATE project SET name = $1, description = $2 WHERE id = $3;");
      pqxx::params p{
        node->getName(),
        node->getDescription(),
        node->idString()
      };
      transaction.exec(cmd, p);         
    }

    bool load(PtrType node, pqxx::work& transaction) {
      bool ret = false;
      std::string cmd = std::format("SELECT * from {} WHERE id = $1;", tableName);
      pqxx::params p{
        node->idString()
      };
      pqxx::result res = transaction.exec(cmd, p);

      if (res.size() > 0) {
        ret = true;
      }
      // It's not an error if the object isn't found. I'm taking a small
      // performance hit loading by name rather than column number, but
      // if the tables ever change this will be much easier to maintain.
      // I can revisit this if database performance proves to be an issue,
      // but I suspect it will not.
      for (auto const &row : res) {
        node->setName(row["name"].as<std::string>());
        node->setDescription(row["description"].as<std::string>());
      }
      return ret;
    }

    void remove(PtrType node, pqxx::work& transaction) {
      Parent::remove(node, transaction);
      std::string cmd = std::format("DELETE from {} WHERE ID = $1", tableName);
      pqxx::params p{
        node->idString()
      };
      transaction.exec(cmd, p);
    }
    
  };

  /*******************************************************************/
  // Requirement-Specific Type

  template <>
  struct DbSpecificData<Requirement> : public DbSpecificData<Node> {
    using Type = Requirement;
    using PtrType = Type::PtrType;
    using Parent = DbSpecificData<Node>;
    
    static constexpr char name[] = "Requirement";
    static constexpr char tableName[] = "requirement";

    void insert(PtrType node, pqxx::work& transaction) {
      Parent::insert(node, transaction);
      std::string cmd("INSERT INTO requirement (id, title, text, functional) VALUES ($1, $2, $3, $4);");
      pqxx::params p{
        node->idString(),
        node->getTitle(),
        node->getText(),
        node->isFunctional()
      };
      transaction.exec(cmd,p);
    }

    void update(PtrType node, pqxx::work& transaction) {
      Parent::update(node, transaction);
      std::string cmd("UPDATE requirement SET title = $1, text = $2, functional = $3 WHERE id = $4;");
      pqxx::params p{
        node->getTitle(),
        node->getText(),
        node->isFunctional(),
        node->idString()
      };
      transaction.exec(cmd, p);
    }
    
    bool load(PtrType node, pqxx::work& transaction) {
      bool ret = false;
      std::string cmd = std::format("SELECT * from {} WHERE id = $1;", tableName);
      pqxx::params p{
        node->idString()
      };
      pqxx::result res = transaction.exec(cmd, p);

      if (res.size() > 0) {
        ret = true;
      }
      // It's not an error if the object isn't found. I'm taking a small
      // performance hit loading by name rather than column number, but
      // if the tables ever change this will be much easier to maintain.
      // I can revisit this if database performance proves to be an issue,
      // but I suspect it will not.
      for (auto const &row : res) {
        node->setTitle(row["title"].as<std::string>());
        node->setText(row["text"].as<std::string>());
        node->setFunctional(row["functional"].as<bool>());
      }
      return ret;
    }

    void remove(PtrType node, pqxx::work& transaction) {
      Parent::remove(node, transaction);
      std::string cmd = std::format("DELETE from {} WHERE ID = $1", tableName);
      pqxx::params p{
        node->idString()
      };
      transaction.exec(cmd, p);
    }
    
  };

  /***************************************************************/
  // Story-specific type

  template<>
  struct DbSpecificData<Story> : public DbSpecificData<Node> {
    using Type = Story;
    using PtrType = Type::PtrType;
    using Parent = DbSpecificData<Node>;
    
    static constexpr char name[] = "Story";
    static constexpr char tableName[] = "story";

    void insert(PtrType node, pqxx::work& transaction) {
      Parent::insert(node, transaction);
      std::string cmd("INSERT INTO story (id, title, goal, benefit) values ($1, $2, $3, $4);");
      pqxx::params p{
        node->idString(),
        node->getTitle(),
        node->getGoal(),
        node->getBenefit()
      };
      transaction.exec(cmd, p);
    }

    void update(PtrType node, pqxx::work& transaction) {
      Parent::update(node, transaction);
      std::string cmd("UPDATE story SET title = $1, goal = $2, benefit = $3 WHERE id = $4;");
      pqxx::params p{
        node->getTitle(),
        node->getGoal(),
        node->getBenefit(),
        node->idString()
      };
      transaction.exec(cmd, p);
    }

    bool load(PtrType node, pqxx::work& transaction) {
      bool ret = false;
      std::string cmd = std::format("SELECT * from {} WHERE id = $1;", tableName);
      pqxx::params p{
        node->idString()
      };
      pqxx::result res = transaction.exec(cmd, p);

      if (res.size() > 0) {
        ret = true;
      }
      // It's not an error if the object isn't found. I'm taking a small
      // performance hit loading by name rather than column number, but
      // if the tables ever change this will be much easier to maintain.
      // I can revisit this if database performance proves to be an issue,
      // but I suspect it will not.
      for (auto const &row : res) {
        node->setTitle(row["title"].as<std::string>());
        node->setGoal(row["goal"].as<std::string>());
        node->setBenefit(row["benefit"].as<std::string>());
      }
      return ret;
    }

    void remove(PtrType node, pqxx::work& transaction) {
      Parent::remove(node, transaction);
      std::string cmd = std::format("DELETE from {} WHERE ID = $1", tableName);
      pqxx::params p{
        node->idString()
      };
      transaction.exec(cmd, p);
    }    
  };

  /******************************************************************/
  // UseCase-Specific type

  template<>
  struct DbSpecificData<UseCase> : public DbSpecificData<Node> {
    using Type = UseCase;
    using PtrType = Type::PtrType;
    using Parent = DbSpecificData<Node>;
    
    static constexpr char name[] = "UseCase";
    static constexpr char tableName[] = "use_case";

    void insert(PtrType node, pqxx::work& transaction) {
      Parent::insert(node, transaction);
      std::string cmd("INSERT INTO use_case (id,name) VALUES ($1, $2);");
      pqxx::params p{
        node->idString(),
        node->getName()
      };
      transaction.exec(cmd, p);
    }

    void update(PtrType node, pqxx::work& transaction) {
      Parent::update(node, transaction);
      std::string cmd("UPDATE use_case SET name = $1 WHERE id = $2;");
      pqxx::params p{
        node->getName(),
        node->idString()
      };
      transaction.exec(cmd, p);
    }

    bool load(PtrType node, pqxx::work& transaction) {
      bool ret = false;
      std::string cmd = std::format("SELECT * from {} WHERE id = $1;", tableName);
      pqxx::params p{
        node->idString()
      };
      pqxx::result res = transaction.exec(cmd, p);

      if (res.size() > 0) {
        ret = true;
      }
      // It's not an error if the object isn't found. I'm taking a small
      // performance hit loading by name rather than column number, but
      // if the tables ever change this will be much easier to maintain.
      // I can revisit this if database performance proves to be an issue,
      // but I suspect it will not.
      for (auto const &row : res) {
        node->setName(row["name"].as<std::string>());
      }
      return ret;
    }

    void remove(PtrType node, pqxx::work& transaction) {
      Parent::remove(node, transaction);
      std::string cmd = std::format("DELETE from {} WHERE ID = $1", tableName);
      pqxx::params p{
        node->idString()
      };
      transaction.exec(cmd, p);
    }
    
  };

  /********************************************************************/
  // Text-Specific type

  template<>
  struct DbSpecificData<Text> : public DbSpecificData<Node> {
    using Type = Text;
    using PtrType = Type::PtrType;
    using Parent = DbSpecificData<Node>;
    
    static constexpr char name[] = "Text";
    static constexpr char tableName[] = "text";

    void insert(PtrType node, pqxx::work& transaction) {
      Parent::insert(node, transaction);
      std::string cmd("INSERT INTO text (id, text) VALUES ($1, $2);");
      pqxx::params p{
        node->idString(),
        node->getText()
      };
      transaction.exec(cmd, p);
    }

    void update(PtrType node, pqxx::work& transaction) {
      Parent::update(node, transaction);
      std::string cmd("UPDATE text SET text = $1 WHERE id = $2;");
      pqxx::params p {
        node->getText(),
        node->idString()
      };
      transaction.exec(cmd, p);
    }

    bool load(PtrType node, pqxx::work& transaction) {
      bool ret = false;
      std::string cmd = std::format("SELECT * from {} WHERE id = $1;", tableName);
      pqxx::params p{
        node->idString()
      };
      pqxx::result res = transaction.exec(cmd, p);

      if (res.size() > 0) {
        ret = true;
      }
      // It's not an error if the object isn't found. I'm taking a small
      // performance hit loading by name rather than column number, but
      // if the tables ever change this will be much easier to maintain.
      // I can revisit this if database performance proves to be an issue,
      // but I suspect it will not.
      for (auto const &row : res) {
        node->setText(row["text"].as<std::string>());
      }
      return ret;
    }

    void remove(PtrType node, pqxx::work& transaction) {
      Parent::remove(node, transaction);
      std::string cmd = std::format("DELETE from {} WHERE ID = $1", tableName);
      pqxx::params p{
        node->idString()
      };
      transaction.exec(cmd, p);
    }
    
  };

  /**********************************************************************/
  // Completed-Specific type

  template <>
  struct DbSpecificData<Completed> : public DbSpecificData<Node> {
    using Type = Completed;
    using PtrType = Type::PtrType;
    using Parent = DbSpecificData<Node>;
      
    static constexpr char name[] = "Completed";
    static constexpr char tableName[] = "completed";

    void insert(PtrType node, pqxx::work& transaction) {
      Parent::insert(node, transaction);
      std::string cmd("INSERT INTO completed (id, description) VALUES ($1, $2);");
      pqxx::params p{
        node->idString(),
        node->getDescription()
      };
      transaction.exec(cmd, p);
    }

    void update(PtrType node, pqxx::work& transaction) {
      Parent::update(node, transaction);
      std::string cmd("UPDATE completed SET description = $1 WHERE id = $2;");
      pqxx::params p{
        node->getDescription(),
        node->idString()
      };
      transaction.exec(cmd, p);
    }

    bool load(PtrType node, pqxx::work& transaction) {
      bool ret = false;
      std::string cmd = std::format("SELECT * from {} WHERE id = $1;", tableName);
      pqxx::params p{
        node->idString()
      };
      pqxx::result res = transaction.exec(cmd, p);

      if (res.size() > 0) {
        ret = true;
      }
      // It's not an error if the object isn't found. I'm taking a small
      // performance hit loading by name rather than column number, but
      // if the tables ever change this will be much easier to maintain.
      // I can revisit this if database performance proves to be an issue,
      // but I suspect it will not.
      for (auto const &row : res) {
        node->setDescription(row["description"].as<std::string>());
      }
      return ret;
    }

    void remove(PtrType node, pqxx::work& transaction) {
      Parent::remove(node, transaction);
      std::string cmd = std::format("DELETE from {} WHERE ID = $1", tableName);
      pqxx::params p{
        node->idString()
      };
      transaction.exec(cmd, p);
    }
    
  };

  /**********************************************************************/
  // KeyValue-Specific type

  template <>
  struct DbSpecificData<KeyValue> : public DbSpecificData<Node> {
    using Type = KeyValue;
    using PtrType = Type::PtrType;
    using Parent = DbSpecificData<Node>;
    
    static constexpr char name[] = "KeyValue";
    static constexpr char tableName[] = "keyvalue";

    void insert(PtrType node, pqxx::work& transaction) {
      Parent::insert(node, transaction);
      std::string cmd("INSERT INTO keyvalue (id, key, value) VALUES ($1, $2, $3);");
      pqxx::params p{
        node->idString(),
        node->getKey(),
        node->getValue()
      };
      transaction.exec(cmd, p);
    }

    void update(PtrType node, pqxx::work& transaction) {
      Parent::update(node, transaction);
      std::string cmd("UPDATE keyvalue SET key = $1, value = $2 WHERE id = $3;");
      pqxx::params p{
        node->getKey(),
        node->getValue(),
        node->idString()
      };
      transaction.exec(cmd, p);
    }

    bool load(PtrType node, pqxx::work& transaction) {
      bool ret = false;
      std::string cmd = std::format("SELECT * from {} WHERE id = $1;", tableName);
      pqxx::params p{
        node->idString()
      };
      pqxx::result res = transaction.exec(cmd, p);

      if (res.size() > 0) {
        ret = true;
      }
      // It's not an error if the object isn't found. I'm taking a small
      // performance hit loading by name rather than column number, but
      // if the tables ever change this will be much easier to maintain.
      // I can revisit this if database performance proves to be an issue,
      // but I suspect it will not.
      for (auto const &row : res) {
        node->setKey(row["key"].as<std::string>());
        node->setValue(row["value"].as<std::string>());
      }
      return ret;
    }

    void remove(PtrType node, pqxx::work& transaction) {
      Parent::remove(node, transaction);
      std::string cmd = std::format("DELETE from {} WHERE ID = $1", tableName);
      pqxx::params p{
        node->idString()
      };
      transaction.exec(cmd, p);
    }    

  };

  /********************************************************/
  // TimeEstimate-specific data

  template<>
  struct DbSpecificData<TimeEstimate> : public DbSpecificData<Node> {
    using Type = TimeEstimate;
    using PtrType = Type::PtrType;
    using Parent = DbSpecificData<Node>;
      
    static constexpr char name[] = "TimeEstimate";
    static constexpr char tableName[] = "time_estimate";

    void insert(PtrType node, pqxx::work& transaction) {
      Parent::insert(node, transaction);
      std::string cmd("INSERT INTO time_estimate (id, text, estimate, started, start) values ($1, $2, $3, $4, $5);");
      pqxx::params p{
        node->idString(),
        node->getText(),
        node->getEstimate(),
        node->getStarted(),
        node->getStartTimestamp()
      };
      transaction.exec(cmd, p);
    }

    void update(PtrType node, pqxx::work& transaction) {
      Parent::update(node, transaction);
      std::string cmd("UPDATE time_estimate SET text=$1, estimate=$2, started=$3, start=$4  WHERE id = $5;");
      pqxx::params p{
        node->getText(),
        node->getEstimate(),
        node->getStarted(),
        node->getStartTimestamp(),
        node->idString()
      };
      transaction.exec(cmd, p);
    }

    bool load(PtrType node, pqxx::work& transaction) {
      bool ret = false;
      std::string cmd = std::format("SELECT * from {} WHERE id = $1;", tableName);
      pqxx::params p{
        node->idString()
      };
      pqxx::result res = transaction.exec(cmd, p);

      if (res.size() > 0) {
        ret = true;
      }
      // It's not an error if the object isn't found. I'm taking a small
      // performance hit loading by name rather than column number, but
      // if the tables ever change this will be much easier to maintain.
      // I can revisit this if database performance proves to be an issue,
      // but I suspect it will not.
      for (auto const &row : res) {
        node->setText(row["text"].as<std::string>());
        node->setEstimate(row["estimate"].as<unsigned long>());
        node->setStarted(row["started"].as<bool>());
        node->setStartTimestamp(row["start"].as<time_t>());
      }
      return ret;
    }

    void remove(PtrType node, pqxx::work& transaction) {
      Parent::remove(node, transaction);
      std::string cmd = std::format("DELETE from {} WHERE ID = $1", tableName);
      pqxx::params p{
        node->idString()
      };
      transaction.exec(cmd, p);
    }    

  };

  /******************************************************************/
  // Effort-Specific type

  template <>
  struct DbSpecificData<Effort> : public DbSpecificData<Node> {
    using Type = Effort;
    using PtrType = Type::PtrType;
    using Parent = DbSpecificData<Node>;
    
    static constexpr char name[] = "Effort";
    static constexpr char tableName[] = "effort";

    void insert(PtrType node, pqxx::work& transaction) {
      Parent::insert(node, transaction);
      std::string cmd("INSERT INTO effort (id, text, effort) values ($1, $2, $3);");
      pqxx::params p{
        node->idString(),
        node->getText(),
        node->getEffort()
      };
      transaction.exec(cmd, p);
    }

    void update(PtrType node, pqxx::work& transaction) {
      Parent::update(node, transaction);
      std::string cmd("UPDATE effort SET text = $1, effort = $2 WHERE id = $3;");
      pqxx::params p{
        node->getText(),
        node->getEffort(),
        node->idString()
      };
      transaction.exec(cmd, p);
    }

    bool load(PtrType node, pqxx::work& transaction) {
      bool ret = false;
      std::string cmd = std::format("SELECT * from {} WHERE id = $1;", tableName);
      pqxx::params p{
        node->idString()
      };
      pqxx::result res = transaction.exec(cmd, p);

      if (res.size() > 0) {
        ret = true;
      }
      // It's not an error if the object isn't found. I'm taking a small
      // performance hit loading by name rather than column number, but
      // if the tables ever change this will be much easier to maintain.
      // I can revisit this if database performance proves to be an issue,
      // but I suspect it will not.
      for (auto const &row : res) {
        node->setText(row["text"].as<std::string>());
        node->setEffort(row["effort"].as<unsigned long>());
      }
      return ret;
    }

    void remove(PtrType node, pqxx::work& transaction) {
      Parent::remove(node, transaction);
      std::string cmd = std::format("DELETE from {} WHERE ID = $1", tableName);
      pqxx::params p{
        node->idString()
      };
      transaction.exec(cmd, p);
    }    

  };

  /******************************************************************/
  // Role-specific type

  template <>
  struct DbSpecificData<Role> : public DbSpecificData<Node> {
    using Type = Role;
    using PtrType = Type::PtrType;
    using Parent = DbSpecificData<Node>;
      
    static constexpr char name[] = "Role";
    static constexpr char tableName[] = "role";

    void insert(PtrType node, pqxx::work& transaction) {
      Parent::insert(node, transaction);
      std::string cmd("INSERT INTO role (id, who) VALUES ($1, $2);");
      pqxx::params p{
        node->idString(),
        node->getWho()
      };
      transaction.exec(cmd, p);
    }

    void update(PtrType node, pqxx::work& transaction) {
      Parent::update(node, transaction);
      std::string cmd("UPDATE role SET who = $1 WHERE id = $2;");
      pqxx::params p {
        node->getWho(),
        node->idString()
      };
      transaction.exec(cmd, p);
    }

    bool load(PtrType node, pqxx::work& transaction) {
      bool ret = false;
      std::string cmd = std::format("SELECT * from {} WHERE id = $1;", tableName);
      pqxx::params p{
        node->idString()
      };
      pqxx::result res = transaction.exec(cmd, p);

      if (res.size() > 0) {
        ret = true;
      }
      // It's not an error if the object isn't found. I'm taking a small
      // performance hit loading by name rather than column number, but
      // if the tables ever change this will be much easier to maintain.
      // I can revisit this if database performance proves to be an issue,
      // but I suspect it will not.
      for (auto const &row : res) {
        node->setWho(row["who"].as<std::string>());
      }
      return ret;
    }

    void remove(PtrType node, pqxx::work& transaction) {
      Parent::remove(node, transaction);
      std::string cmd = std::format("DELETE from {} WHERE ID = $1", tableName);
      pqxx::params p{
        node->idString()
      };
      transaction.exec(cmd, p);
    }    

  };

  /*****************************************************************/
  // Actor-specific type

  template <>
  struct DbSpecificData<Actor> : public DbSpecificData<Node> {
    using Type = Actor;
    using PtrType = Type::PtrType;
    using Parent = DbSpecificData<Node>;
    
    static constexpr char name[] = "Actor";
    static constexpr char tableName[] = "actor";

    void insert(PtrType node, pqxx::work& transaction) {
      Parent::insert(node, transaction);
      std::string cmd("INSERT INTO actor (id, actor) values ($1, $2);");      
      pqxx::params p{
        node->idString(),
        node->getActor()
      };
      transaction.exec(cmd,p);
    }

    void update(PtrType node, pqxx::work& transaction) {
      Parent::update(node, transaction);
      std::string cmd("UPDATE actor SET actor = $1 WHERE id = $2;");
      pqxx::params p{
        node->getActor(),
        node->idString()
      };
      transaction.exec(cmd, p);
    }

    bool load(PtrType node, pqxx::work& transaction) {
      bool ret = false;
      std::string cmd = std::format("SELECT * from {} WHERE id = $1;", tableName);
      pqxx::params p{
        node->idString()
      };
      pqxx::result res = transaction.exec(cmd, p);

      if (res.size() > 0) {
        ret = true;
      }
      // It's not an error if the object isn't found. I'm taking a small
      // performance hit loading by name rather than column number, but
      // if the tables ever change this will be much easier to maintain.
      // I can revisit this if database performance proves to be an issue,
      // but I suspect it will not.
      for (auto const &row : res) {
        node->setActor(row["actor"].as<std::string>());
      }
      return ret;
    }

    void remove(PtrType node, pqxx::work& transaction) {
      Parent::remove(node, transaction);
      std::string cmd = std::format("DELETE from {} WHERE ID = $1", tableName);
      pqxx::params p{
        node->idString()
      };
      transaction.exec(cmd, p);
    }    
    
  };

  /***********************************************************/
  // Goal-specific type

  template <>
  struct DbSpecificData<Goal> : public DbSpecificData<Node> {
    using Type = Goal;
    using PtrType = Type::PtrType;
    using Parent = DbSpecificData<Node>;
    
    static constexpr char name[] = "Goal";
    static constexpr char tableName[] = "goal";

    void insert(PtrType node, pqxx::work& transaction) {      
      std::string cmd("INSERT INTO goal (id, action, outcome, context, target_date,"
                      "target_date_confidence, alignment) VALUES ($1, $2, $3, $4, $5, $6, $7);");
      Parent::insert(node, transaction);
      pqxx::params p{
        node->idString(),
        node->getAction(),
        node->getOutcome(),
        node->getContext(),
        node->getTargetDate(),
        node->getTargetDateConfidence(),
        node->getAlignment()
      };
      transaction.exec(cmd, p);
    }

    void update(PtrType node, pqxx::work& transaction) {
      Parent::update(node, transaction);
      std::string cmd("UPDATE goal SET action = $1, outcome = $2, context = $3, "
                      "target_date = $4, target_date_confidence = $5, alignment = $6"
                      "WHERE id = $7;");
      pqxx::params p{
        node->getAction(),
        node->getOutcome(),
        node->getContext(),
        node->getTargetDate(),
        node->getTargetDateConfidence(),
        node->getAlignment(),
        node->idString()
      };
      transaction.exec(cmd, p);
    }

    bool load(PtrType node, pqxx::work& transaction) {
      bool ret = false;
      std::string cmd = std::format("SELECT * from {} WHERE id = $1;", tableName);
      pqxx::params p{
        node->idString()
      };
      pqxx::result res = transaction.exec(cmd, p);

      if (res.size() > 0) {
        ret = true;
      }
      // It's not an error if the object isn't found. I'm taking a small
      // performance hit loading by name rather than column number, but
      // if the tables ever change this will be much easier to maintain.
      // I can revisit this if database performance proves to be an issue,
      // but I suspect it will not.
      for (auto const &row : res) {
        node->setAction(row["action"].as<std::string>());
        node->setOutcome(row["outcome"].as<std::string>());
        node->setContext(row["context"].as<std::string>());
        node->setTargetDate(row["target_date"].as<unsigned long>());
        node->setTargetDateConfidence(row["target_date_confidence"].as<std::string>());
        node->setAlignment(row["alignment"].as<std::string>());
      }
      return ret;
    }

    void remove(PtrType node, pqxx::work& transaction) {
      Parent::remove(node, transaction);
      std::string cmd = std::format("DELETE from {} WHERE ID = $1", tableName);
      pqxx::params p{
        node->idString()
      };
      transaction.exec(cmd, p);
    }
    
  };

  /*********************************************************/
  // Purpose-specific type

  template <>
  struct DbSpecificData<Purpose> : public DbSpecificData<Node> {
    using Type = Purpose;
    using PtrType = Type::PtrType;
    using Parent = DbSpecificData<Node>;
    
    static constexpr char name[] = "Purpose";
    static constexpr char tableName[] = "purpose";

    void insert(PtrType node, pqxx::work& transaction) {
      Parent::insert(node, transaction);
      std::string cmd("INSERT INTO purpose (id, description, deadline, deadline_confidence)"
                      "VALUES ($1, $2, $3, $4);");
      pqxx::params p{
        node->idString(),
        node->getDescription(),
        node->getDeadline(),
        node->getDeadlineConfidence()
      };
      transaction.exec(cmd, p);
    }

    void update(PtrType node, pqxx::work& transaction) {
      Parent::update(node, transaction);
      std::string cmd("UPDATE purpsoe SET description = $1, deadline = $2,"
                      "deadline_confidence = $3 WHERE id = $4");
      pqxx::params p{
        node->getDescription(),
        node->getDeadline(),
        node->getDeadlineConfidence(),
        node->idString()
      };
      transaction.exec(cmd, p);
    }

    bool load(PtrType node, pqxx::work& transaction) {
      bool ret = false;
      std::string cmd = std::format("SELECT * from {} WHERE id = $1;", tableName);
      pqxx::params p{
        node->idString()
      };
      pqxx::result res = transaction.exec(cmd, p);

      if (res.size() > 0) {
        ret = true;
      }
      // It's not an error if the object isn't found. I'm taking a small
      // performance hit loading by name rather than column number, but
      // if the tables ever change this will be much easier to maintain.
      // I can revisit this if database performance proves to be an issue,
      // but I suspect it will not.
      for (auto const &row : res) {
        node->setDescription(row["description"].as<std::string>());
        node->setDeadline(row["deadline"].as<unsigned long>());
        node->setDeadlineConfidence(row["deadline_confidence"].as<std::string>());
      }
      return ret;
    }

    void remove(PtrType node, pqxx::work& transaction) {
      Parent::remove(node, transaction);
      std::string cmd = std::format("DELETE from {} WHERE ID = $1", tableName);
      pqxx::params p{
        node->idString()
      };
      transaction.exec(cmd, p);
    }    

  };

  /*************************************************************/
  // Person-specific type

  template<>
  struct DbSpecificData<Person> : public DbSpecificData<Node> {
    using Type = Person;
    using PtrType = Type::PtrType;
    using Parent = DbSpecificData<Node>;    
    
    static constexpr char name[] = "Person";
    static constexpr char tableName[] = "person";

    void insert(PtrType node, pqxx::work& transaction) {
      Parent::insert(node, transaction);
      std::string cmd("INSERT INTO person (id, first_name, last_name) VALUES ($1, $2, $3);");
      pqxx::params p {
        node->idString(),
        node->getFirstName(),
        node->getLastName()
      };
      transaction.exec(cmd, p);
    }

    void update(PtrType node, pqxx::work& transaction) {
      Parent::update(node, transaction);
      std::string cmd("UPDATE person SET first_name = $1, last_name = $2 WHERE id = $3;");
      pqxx::params p {
        node->getFirstName(),
        node->getLastName(),
        node->idString()
      };
      transaction.exec(cmd, p);
    }

    bool load(PtrType node, pqxx::work& transaction) {
      bool ret = false;
      std::string cmd = std::format("SELECT * from {} WHERE id = $1;", tableName);
      pqxx::params p{
        node->idString()
      };
      pqxx::result res = transaction.exec(cmd, p);

      if (res.size() > 0) {
        ret = true;
      }
      // It's not an error if the object isn't found. I'm taking a small
      // performance hit loading by name rather than column number, but
      // if the tables ever change this will be much easier to maintain.
      // I can revisit this if database performance proves to be an issue,
      // but I suspect it will not.
      for (auto const &row : res) {
        node->setFirstName(row["first_name"].as<std::string>());
        node->setLastName(row["last_name"].as<std::string>());
      }
      return ret;
    }

    void remove(PtrType node, pqxx::work& transaction) {
      Parent::remove(node, transaction);
      std::string cmd = std::format("DELETE from {} WHERE ID = $1", tableName);
      pqxx::params p{
        node->idString()
      };
      transaction.exec(cmd, p);
    }    

  };

  /**********************************************************/
  // EmailAddress-specific type

  template <>
  struct DbSpecificData<EmailAddress> : public DbSpecificData<Node> {
    using Type = EmailAddress;
    using PtrType = Type::PtrType;
    using Parent = DbSpecificData<Node>;    

    static constexpr char name[] = "EmailAddress";
    static constexpr char tableName[] = "email_address";

    void insert(PtrType node, pqxx::work& transaction) {
      Parent::insert(node, transaction);
      std::string cmd("INSERT INTO email_address (id, address) VALUES ($1, $2);");
      pqxx::params p {
        node->idString(),
        node->getAddress()
      };
      transaction.exec(cmd,p);
    }

    void update(PtrType node, pqxx::work& transaction) {
      Parent::update(node, transaction);
      std::string cmd("UPDATE email_address set address=$1 WHERE id = $2;");
      pqxx::params p {
        node->getAddress(),
        node->idString()
      };
      transaction.exec(cmd, p);
    }

    bool load(PtrType node, pqxx::work& transaction) {
      bool ret = false;
      std::string cmd = std::format("SELECT * from {} WHERE id = $1;", tableName);
      pqxx::params p{
        node->idString()
      };
      pqxx::result res = transaction.exec(cmd, p);

      if (res.size() > 0) {
        ret = true;
      }
      // It's not an error if the object isn't found. I'm taking a small
      // performance hit loading by name rather than column number, but
      // if the tables ever change this will be much easier to maintain.
      // I can revisit this if database performance proves to be an issue,
      // but I suspect it will not.
      for (auto const &row : res) {
        node->setAddress(row["address"].as<std::string>());
      }
      return ret;
    }

    void remove(PtrType node, pqxx::work& transaction) {
      Parent::remove(node, transaction);
      std::string cmd = std::format("DELETE from {} WHERE ID = $1", tableName);
      pqxx::params p{
        node->idString()
      };
      transaction.exec(cmd, p);
    }    

  };

  /************************************************************/
  // PhoneNumber-specific type

  template <>
  struct DbSpecificData<PhoneNumber> : public DbSpecificData<Node> {
    using Type = PhoneNumber;
    using PtrType = Type::PtrType;
    using Parent = DbSpecificData<Node>;
    
    static constexpr char name[] = "PhoneNumber";
    static constexpr char tableName[] = "phone_number";

    void insert(PtrType node, pqxx::work& transaction) {
      Parent::insert(node, transaction);
      std::string cmd("INSERT INTO phone_number (id, countrycode, number, phone_type"
                      "VALUES ($1, $2, $3, $4);");
      pqxx::params p{
        node->idString(),
        node->getCountryCode(),
        node->getNumber(),
        node->getPhoneType()
      };
      transaction.exec(cmd, p);
    }

    void update(PtrType node, pqxx::work& transaction) {
      Parent::update(node, transaction);
      std::string cmd("UPDATE phone_number SET countrycode = $1, number = $2,"
                      "phone_type = $3 WHERE id = $4;");
      pqxx::params p{
        node->getCountryCode(),
        node->getNumber(),
        node->getPhoneType(),
        node->idString()
      };
      transaction.exec(cmd, p);
    }

    bool load(PtrType node, pqxx::work& transaction) {
      bool ret = false;
      std::string cmd = std::format("SELECT * from {} WHERE id = $1;", tableName);
      pqxx::params p{
        node->idString()
      };
      pqxx::result res = transaction.exec(cmd, p);

      if (res.size() > 0) {
        ret = true;
      }
      // It's not an error if the object isn't found. I'm taking a small
      // performance hit loading by name rather than column number, but
      // if the tables ever change this will be much easier to maintain.
      // I can revisit this if database performance proves to be an issue,
      // but I suspect it will not.
      for (auto const &row : res) {
        node->setCountryCode(row["countrycode"].as<std::string>());
        node->setNumber(row["number"].as<std::string>());
        node->setPhoneType(row["phone_type"].as<std::string>());
      }
      return ret;
    }

    void remove(PtrType node, pqxx::work& transaction) {
      Parent::remove(node, transaction);
      std::string cmd = std::format("DELETE from {} WHERE ID = $1", tableName);
      pqxx::params p{
        node->idString()
      };
      transaction.exec(cmd, p);
    }    

  };

  /*****************************************************************/
  // InternationalAddress-specific type
  
  template <>
  struct DbSpecificData<InternationalAddress> : public DbSpecificData<Node> {
    using Type = InternationalAddress;
    using PtrType = Type::PtrType;
    using Parent = DbSpecificData<Node>;
    
    static constexpr char name[] = "InternationalAddress";
    static constexpr char tableName[] = "international_address";

    void insert(PtrType node, pqxx::work& transaction) {
      Parent::insert(node, transaction);
      std::string cmd("INSERT INTO international_address (id, country_code, address_lines,"
                      "locality, postal_code) VALUES ($1, $2, $3, $4, $5);");

      pqxx::params p{
        node->idString(),
        node->getCountryCode(),
        node->getAddressLines(),
        node->getLocality(),
        node->getPostalCode()        
      };
      transaction.exec(cmd, p);
    }

    void update(PtrType node, pqxx::work& transaction) {
      Parent::update(node, transaction);
      std::string cmd("UPDATE international_address SET country_code = $1, address_lines = $2,"
                      "locality = $3, postal_code = $4 WHERE id = $5;");

      pqxx::params p{
        node->getCountryCode(),
        node->getAddressLines(),
        node->getLocality(),
        node->getPostalCode(),
        node->idString()
      };
      
      transaction.exec(cmd, p);
    }

    bool load(PtrType node, pqxx::work& transaction) {
      bool ret = false;
      std::string cmd = std::format("SELECT * from {} WHERE id = $1;", tableName);
      pqxx::params p{
        node->idString()
      };
      pqxx::result res = transaction.exec(cmd, p);

      if (res.size() > 0) {
        ret = true;
      }
      // It's not an error if the object isn't found. I'm taking a small
      // performance hit loading by name rather than column number, but
      // if the tables ever change this will be much easier to maintain.
      // I can revisit this if database performance proves to be an issue,
      // but I suspect it will not.
      for (auto const &row : res) {
        node->setCountryCode(row["country_code"].as<std::string>());
        // Address lines are just text nodes and will be set up elsewhere.
        node->setLocality(row["locality"].as<std::string>());
        node->setPostalCode(row["postal_code"].as<std::string>());
      }
      return ret;
    }

    void remove(PtrType node, pqxx::work& transaction) {
      Parent::remove(node, transaction);
      std::string cmd = std::format("DELETE from {} WHERE ID = $1", tableName);
      pqxx::params p{
        node->idString()
      };
      transaction.exec(cmd, p);
    }    
    
  };

  /*****************************************************************/
  // USAddress-specific type

  template <>
  struct DbSpecificData<USAddress> : public DbSpecificData<Node> {
    using Type = USAddress;
    using PtrType = Type::PtrType;
    using Parent = DbSpecificData<Node>;
    
    static constexpr char name[] = "USAddress";
    static constexpr char tableName[] = "us_address";

    void insert(PtrType node, pqxx::work& transaction) {
      Parent::insert(node, transaction);
      std::string cmd("INSERT INTO us_address (id, address_lines, city, state, zipcode) "
                      "VALUES ($1, $2, $3, $4, $5);");

      std::string addressId;

      pqxx::params p{
        node->idString(),
        node->getAddressLines(),
        node->getCity(),
        node->getState(),
        node->getZipCode()
      };
      transaction.exec(cmd, p);
    }

    void update(PtrType node, pqxx::work& transaction) {
      Parent::update(node, transaction);
      std::string cmd("UPDATE us_address SET address_lines = $1, city = $2, state = $3, "
                      "zipcode = $4 WHERE id = $5;");

      pqxx::params p{
        node->getAddressLines(),
        node->getCity(),
        node->getState(),
        node->getZipCode(),
        node->idString()
      };
      transaction.exec(cmd,p);
    }

    bool load(PtrType node, pqxx::work& transaction) {
      bool ret = false;
      std::string cmd = std::format("SELECT * from {} WHERE id = $1;", tableName);
      pqxx::params p{
        node->idString()
      };
      pqxx::result res = transaction.exec(cmd, p);

      if (res.size() > 0) {
        ret = true;
      }
      // It's not an error if the object isn't found. I'm taking a small
      // performance hit loading by name rather than column number, but
      // if the tables ever change this will be much easier to maintain.
      // I can revisit this if database performance proves to be an issue,
      // but I suspect it will not.
      for (auto const &row : res) {
        // Address lines are just text nodes and will be set up elsewhere.
        node->setCity(row["city"].as<std::string>());
        node->setState(row["state"].as<std::string>());
        node->setZipCode(row["zipcode"].as<std::string>());
      }
      return ret;
    }

    void remove(PtrType node, pqxx::work& transaction) {
      Parent::remove(node, transaction);
      std::string cmd = std::format("DELETE from {} WHERE ID = $1", tableName);
      pqxx::params p{
        node->idString()
      };
      transaction.exec(cmd, p);
    }    

  };

  /***********************************************************************/
  // Event-specific type
  template <>
  struct DbSpecificData<Event> : public DbSpecificData<Node> {
    using Type = Event;
    using PtrType = Type::PtrType;
    using Parent = DbSpecificData<Node>;
    
    static constexpr char name[] = "Event";
    static constexpr char tableName[] = "event";

    void insert(PtrType node, pqxx::work& transaction) {
      Parent::insert(node, transaction);
      std::string cmd("INSERT INTO event (id, name, description) VALUES ($1, $2, $3);");
      pqxx::params p{
        node->idString(),
        node->getName(),
        node->getDescription()
      };
      transaction.exec(cmd, p);
    }

    void update(PtrType node, pqxx::work& transaction) {
      Parent::update(node, transaction);
      std::string cmd("UPDATE event SET name = $1, description = $2 WHERE id = $3");
      pqxx::params p{
        node->getName(),
        node->getDescription(),
        node->idString()
      };
      transaction.exec(cmd, p);
    }

    bool load(PtrType node, pqxx::work& transaction) {
      bool ret = false;
      std::string cmd = std::format("SELECT * from {} WHERE id = $1;", tableName);
      pqxx::params p{
        node->idString()
      };
      pqxx::result res = transaction.exec(cmd, p);

      if (res.size() > 0) {
        ret = true;
      }
      // It's not an error if the object isn't found. I'm taking a small
      // performance hit loading by name rather than column number, but
      // if the tables ever change this will be much easier to maintain.
      // I can revisit this if database performance proves to be an issue,
      // but I suspect it will not.
      for (auto const &row : res) {
        node->setName(row["name"].as<std::string>());
        node->setDescription(row["description"].as<std::string>());
      }
      return ret;
    }

    void remove(PtrType node, pqxx::work& transaction) {
      Parent::remove(node, transaction);
      std::string cmd = std::format("DELETE from {} WHERE ID = $1", tableName);
      pqxx::params p{
        node->idString()
      };
      transaction.exec(cmd, p);
    }    

  };

  /********************************************************************************/
  // RecurringTodo-specific type
  template <>
  struct DbSpecificData<RecurringTodo> : public DbSpecificData<Node> {
    using Type = RecurringTodo;
    using PtrType = Type::PtrType;
    using Parent = DbSpecificData<Node>;

    static constexpr char name[] = "RecurringTodo";
    static constexpr char tableName[] = "recurring_todo";

    void insert(PtrType node, pqxx::work& transaction) {
      Parent::insert(node, transaction);
      std::string cmd =
        std::format("INSERT INTO {} (id, description, created, recurring_interval,"
                    "seconds_flag, dom_flag, doy_flag) VALUES ($1, $2, $3, $4, $5, $6, $7);",
                    tableName);
      pqxx::params p{
        node->idString(),
        node->getDescription(),
        node->getCreated(),
        node->getRecurringInterval(),
        node->getSecondsFlag(),
        node->getDayOfMonthFlag(),
        node->getDayOfYearFlag()
      };
      transaction.exec(cmd, p);
      transaction.commit();
    }

    void update(PtrType node, pqxx::work& transaction) {
      Parent::update(node, transaction);
      std::string cmd =
        std::format("UPDATE {} SET "
                    "description = $2,"
                    "created = $3,"
                    "recurring_interval = $4,"
                    "seconds_flag = $5,"
                    "dom_flag = $6,"
                    "doy_flag = $7 "
                    "WHERE id = $1", tableName);
      pqxx::params p{
        node->idString(),
        node->getDescription(),
        node->getCreated(),
        node->getRecurringInterval(),
        node->getSecondsFlag(),
        node->getDayOfMonthFlag(),
        node->getDayOfYearFlag()
      };
      transaction.exec(cmd, p);
      transaction.commit();
    }

    bool load(PtrType node, pqxx::work& transaction) {
      bool ret = false;
      std::string cmd = std::format("select * from {} WHERE id = $1", tableName);
      pqxx::params p{
        node->idString()
      };
      pqxx::result res = transaction.exec(cmd, p);
      if (res.size() > 0) {
        ret = true;
      }
      for (auto const &row : res) {
        node->setDescription(row["description"].as<std::string>());
        node->setCreated(row["created"].as<time_t>());
        node->setRecurringInterval(row["recurring_interval"].as<time_t>());
        node->setSecondsFlag(row["seconds_flag"].as<bool>());
        node->setDayOfMonthFlag(row["dom_flag"].as<bool>());
        node->setDayOfYearFlag(row["doy_flag"].as<bool>());
      }      
      return ret;
    }

    void remove(PtrType node, pqxx::work& transaction) {
      Parent::remove(node, transaction);
      std::string cmd = std::format("DELETE from {} WHERE ID = $1", tableName);
      pqxx::params p{
        node->idString()
      };
      transaction.exec(cmd, p);
      transaction.commit();
    }
    
  };
  
  /********************************************************************************/
  // Todo-specific type

  template <>
  struct DbSpecificData<Todo> : public DbSpecificData<Node> {
    using Type = Todo;
    using PtrType = Type::PtrType;
    using Parent = DbSpecificData<Node>;

    static constexpr char name[] = "Todo";
    static constexpr char tableName[] = "todo";

    void insert(PtrType node, pqxx::work& transaction) {
      Parent::insert(node, transaction);
      std::string cmd = std::format("INSERT INTO {} (id, description, created,"
                                    "due, completed, date_completed, spawned_from)"
                                    "VALUES ($1, $2, $3, $4, $5, $6, $7)",
                                    tableName);
      pqxx::params p{
        node->idString(),
        node->getDescription(),
        node->getCreated(),
        node->getDue(),
        node->getCompleted(),
        node->getDateCompleted(),
        boost::uuids::to_string(node->getSpawnedFrom())
      };

      transaction.exec(cmd, p);
    }

    void update(PtrType node, pqxx::work& transaction) {
      Parent::update(node, transaction);
      std::string cmd =
        std::format("update {} SET "
                    "description = $2,"
                    "created = $3,"
                    "due = $4,"
                    "completed = $5,"
                    "date_completed = $6,"
                    "spawned_from = $7 "
                    "WHERE id = $1", tableName);
      pqxx::params p{
        node->idString(),
        node->getDescription(),
        node->getCreated(),
        node->getDue(),
        node->getCompleted(),
        node->getDateCompleted(),
        boost::uuids::to_string(node->getSpawnedFrom())
      };
      transaction.exec(cmd, p);
    }

    bool load(PtrType node, pqxx::work& transaction) {
      bool ret = false;
      std::string cmd = std::format("SELECT * from {} WHERE id = $1", tableName);
      pqxx::params p{
        node->idString()
      };
      pqxx::result res = transaction.exec(cmd, p);
      if (res.size() > 0) {
        ret = true;
      }
      for (auto const &row : res) {
        node->setDescription(row["description"].as<std::string>());
        node->setCreated(row["created"].as<time_t>());
        node->setDue(row["due"].as<time_t>());
        node->setCompleted(row["completed"].as<bool>());
        node->setCompleted(row["date_completed"].as<time_t>());
        boost::uuids::string_generator generator;
        std::string uuid_str = row["spawned_from"].as<std::string>();
        node->setSpawnedFrom(generator(uuid_str));
      }
      return ret;
    }

    void remove(PtrType node, pqxx::work& transaction) {
      Parent::remove(node, transaction);
      std::string cmd = std::format("DELETE from {} WHERE ID = $1", tableName);
      pqxx::params p{
        node->idString()
      };
      transaction.exec(cmd, p);
    }
    
  };
  
  /**
   * nodeInTable function checks a specific table (defined in tableName in all
   * the above specialziations) and checks to see if a given node is in the
   * table. This dictates whether insert or update gets called to update
   * the table.
   */
  
  template <typename NodeType>
  bool nodeInTable(typename NodeType::PtrType node, pqxx::work& transaction) {
    std::string query = std::format("select id from {} where id = $1", DbSpecificData<NodeType>::tableName);
    pqxx::params p{
      node->idString()
    };
    pqxx::result result = transaction.exec(query,p);
    return (result.size() > 0);
  }

}
