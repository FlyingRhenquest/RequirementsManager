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
  };


  /************************************************************/
  // GraphNode-Specific type

  template <>
  struct DbSpecificData<fr::RequirementsManager::GraphNode> {
    using type = fr::RequirementsManager::GraphNode;
    static constexpr char name[] = "GraphNode";
    static constexpr char tableName[] = "graph_node";

    void insert(GraphNode::PtrType node, pqxx::work& transaction) {
      std::string cmd = std::format("INSERT INTO {} (id,title) VALUES ($1, $2);", tableName);
      pqxx::params p{
        node->idString(),
        node->getTitle()
      };
      transaction.exec(cmd, p);
    }

    void update(GraphNode::PtrType node, pqxx::work& transaction) {
      std::string cmd = std::format("UPDATE {} SET title = $1 WHERE id = $2;", tableName);
      pqxx::params p{
        node->getTitle(),
        node->idString()
      };
      transaction.exec(cmd,p);
    }

    bool load(GraphNode::PtrType node, pqxx::work& transaction) {
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
    
  };
  
  /************************************************************/
  // Organization-Specific type
  
  template <>
  struct DbSpecificData<fr::RequirementsManager::Organization> {
    using type = fr::RequirementsManager::Organization;
    static constexpr char name[] = "Organization";
    static constexpr char tableName[] = "organization";

    void insert(Organization::PtrType node, pqxx::work& transaction) {
      std::string cmd("INSERT INTO organization (id, locked, name) VALUES ($1, $2, $3);");
      pqxx::params p{
        node->idString(),
        node->isLocked(),
        node->getName()
      };
      transaction.exec(cmd, p);
    }

    void update(Organization::PtrType node, pqxx::work& transaction) {
      std::string cmd("UPDATE organization SET name = $1, locked = $2 WHERE id = $3;");
      pqxx::params p{
        node->getName(),
        node->isLocked(),
        node->idString()
      };
      transaction.exec(cmd, p);
    }

    bool load(Organization::PtrType node, pqxx::work& transaction) {
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
    
  };

  /**********************************************************/
  // Product-specific type
  
  template <>
  struct DbSpecificData<fr::RequirementsManager::Product> {
    using type = fr::RequirementsManager::Product;
    static constexpr char name[] = "Product";
    static constexpr char tableName[] = "product";

    void insert(Product::PtrType node, pqxx::work& transaction) {
      std::string cmd("INSERT INTO product (id,title,description) VALUES ($1, $2, $3);");
      pqxx::params p{
        node->idString(),
        node->getTitle(),
        node->getDescription()
      };
      transaction.exec(cmd, p);
    }

    void update(Product::PtrType node, pqxx::work& transaction) {
      std::string cmd("UPDATE product SET title = $1, description = $2 WHERE id = $3;");
      pqxx::params p{
        node->getTitle(),
        node->getDescription(),
        node->idString()
      };
      transaction.exec(cmd, p);
    }

    bool load(Product::PtrType node, pqxx::work& transaction) {
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
    
  };

  /***********************************************************/
  // Project-specific type

  template <>
  struct DbSpecificData<fr::RequirementsManager::Project> {
    using type = fr::RequirementsManager::Project;
    static constexpr char name[] = "Project";
    static constexpr char tableName[] = "project";

    void insert(Project::PtrType node, pqxx::work& transaction) {
      std::string cmd("INSERT INTO project (id,name,description) VALUES ($1, $2, $3);");
      pqxx::params p{
        node->idString(),
        node->getName(),
        node->getDescription()
      };
      transaction.exec(cmd, p);
    }

    void update(Project::PtrType node, pqxx::work& transaction) {
      std::string cmd("UPDATE project SET name = $1, description = $2 WHERE id = $3;");
      pqxx::params p{
        node->getName(),
        node->getDescription(),
        node->idString()
      };
      transaction.exec(cmd, p);         
    }

    bool load(Project::PtrType node, pqxx::work& transaction) {
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
  };

  /*******************************************************************/
  // Requirement-Specific Type

  template <>
  struct DbSpecificData<fr::RequirementsManager::Requirement> {
    using type = fr::RequirementsManager::Requirement;
    static constexpr char name[] = "Requirement";
    static constexpr char tableName[] = "requirement";

    void insert(Requirement::PtrType node, pqxx::work& transaction) {
      std::string cmd("INSERT INTO requirement (id, title, text, functional) VALUES ($1, $2, $3, $4);");
      pqxx::params p{
        node->idString(),
        node->getTitle(),
        node->getText(),
        node->isFunctional()
      };
      transaction.exec(cmd,p);
    }

    void update(Requirement::PtrType node, pqxx::work& transaction) {
      std::string cmd("UPDATE requirement SET title = $1, text = $2, functional = $3 WHERE id = $4;");
      pqxx::params p{
        node->getTitle(),
        node->getText(),
        node->isFunctional(),
        node->idString()
      };
      transaction.exec(cmd, p);
    }
    
    bool load(Requirement::PtrType node, pqxx::work& transaction) {
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
    
  };

  /***************************************************************/
  // Story-specific type

  template<>
  struct DbSpecificData<fr::RequirementsManager::Story> {
    using type = fr::RequirementsManager::Story;
    static constexpr char name[] = "Story";
    static constexpr char tableName[] = "story";

    void insert(Story::PtrType node, pqxx::work& transaction) {
      std::string cmd("INSERT INTO story (id, title, goal, benefit) values ($1, $2, $3, $4);");
      pqxx::params p{
        node->idString(),
        node->getTitle(),
        node->getGoal(),
        node->getBenefit()
      };
      transaction.exec(cmd, p);
    }

    void update(Story::PtrType node, pqxx::work& transaction) {
      std::string cmd("UPDATE story SET title = $1, goal = $2, benefit = $3 WHERE id = $4;");
      pqxx::params p{
        node->getTitle(),
        node->getGoal(),
        node->getBenefit(),
        node->idString()
      };
      transaction.exec(cmd, p);
    }

    bool load(Story::PtrType node, pqxx::work& transaction) {
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
  };

  /******************************************************************/
  // UseCase-Specific type

  template<>
  struct DbSpecificData<fr::RequirementsManager::UseCase> {
    using type = fr::RequirementsManager::UseCase;
    static constexpr char name[] = "UseCase";
    static constexpr char tableName[] = "use_case";

    void insert(UseCase::PtrType node, pqxx::work& transaction) {
      std::string cmd("INSERT INTO use_case (id,name) VALUES ($1, $2);");
      pqxx::params p{
        node->idString(),
        node->getName()
      };
      transaction.exec(cmd, p);
    }

    void update(UseCase::PtrType node, pqxx::work& transaction) {
      std::string cmd("UPDATE use_case SET name = $1 WHERE id = $2;");
      pqxx::params p{
        node->getName(),
        node->idString()
      };
      transaction.exec(cmd, p);
    }

    bool load(UseCase::PtrType node, pqxx::work& transaction) {
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

  };

  /********************************************************************/
  // Text-Specific type

  template<>
  struct DbSpecificData<fr::RequirementsManager::Text> {
    using type = fr::RequirementsManager::Text;
    static constexpr char name[] = "Text";
    static constexpr char tableName[] = "text";

    void insert(Text::PtrType node, pqxx::work& transaction) {
      std::string cmd("INSERT INTO text (id, text) VALUES ($1, $2);");
      pqxx::params p{
        node->idString(),
        node->getText()
      };
      transaction.exec(cmd, p);
    }

    void update(Text::PtrType node, pqxx::work& transaction) {
      std::string cmd("UPDATE text SET text = $1 WHERE id = $2;");
      pqxx::params p {
        node->getText(),
        node->idString()
      };
      transaction.exec(cmd, p);
    }

    bool load(Text::PtrType node, pqxx::work& transaction) {
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
    
  };

  /**********************************************************************/
  // Completed-Specific type

  template <>
  struct DbSpecificData<fr::RequirementsManager::Completed> {
    using type = fr::RequirementsManager::Completed;
    static constexpr char name[] = "Completed";
    static constexpr char tableName[] = "completed";

    void insert(Completed::PtrType node, pqxx::work& transaction) {
      std::string cmd("INSERT INTO completed (id, description) VALUES ($1, $2);");
      pqxx::params p{
        node->idString(),
        node->getDescription()
      };
      transaction.exec(cmd, p);
    }

    void update(Completed::PtrType node, pqxx::work& transaction) {
      std::string cmd("UPDATE completed SET description = $1 WHERE id = $2;");
      pqxx::params p{
        node->getDescription(),
        node->idString()
      };
      transaction.exec(cmd, p);
    }

    bool load(Completed::PtrType node, pqxx::work& transaction) {
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

  };

  /**********************************************************************/
  // KeyValue-Specific type

  template <>
  struct DbSpecificData<fr::RequirementsManager::KeyValue> {
    using type = fr::RequirementsManager::KeyValue;
    static constexpr char name[] = "KeyValue";
    static constexpr char tableName[] = "keyvalue";

    void insert(KeyValue::PtrType node, pqxx::work& transaction) {
      std::string cmd("INSERT INTO keyvalue (id, key, value) VALUES ($1, $2, $3);");
      pqxx::params p{
        node->idString(),
        node->getKey(),
        node->getValue()
      };
      transaction.exec(cmd, p);
    }

    void update(KeyValue::PtrType node, pqxx::work& transaction) {
      std::string cmd("UPDATE keyvalue SET key = $1, value = $2 WHERE id = $3;");
      pqxx::params p{
        node->getKey(),
        node->getValue(),
        node->idString()
      };
      transaction.exec(cmd, p);
    }

    bool load(KeyValue::PtrType node, pqxx::work& transaction) {
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

  };

  /********************************************************/
  // TimeEstimate-specific data

  template<>
  struct DbSpecificData<fr::RequirementsManager::TimeEstimate> {
    using type = fr::RequirementsManager::TimeEstimate;
    static constexpr char name[] = "TimeEstimate";
    static constexpr char tableName[] = "time_estimate";

    void insert(TimeEstimate::PtrType node, pqxx::work& transaction) {
      std::string cmd("INSERT INTO time_estimate (id, text, estimate) values ($1, $2, $3);");
      pqxx::params p{
        node->idString(),
        node->getText(),
        node->getEstimate()
      };
      transaction.exec(cmd, p);
    }

    void update(TimeEstimate::PtrType node, pqxx::work& transaction) {
      std::string cmd("UPDATE time_estimate SET text=$1, estimate=$2 WHERE id = $3;");
      pqxx::params p{
        node->getText(),
        node->getEstimate(),
        node->idString()
      };
      transaction.exec(cmd, p);
    }

    bool load(TimeEstimate::PtrType node, pqxx::work& transaction) {
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
      }
      return ret;
    }

  };

  /******************************************************************/
  // Effort-Specific type

  template <>
  struct DbSpecificData<fr::RequirementsManager::Effort> {
    using type = fr::RequirementsManager::Effort;
    static constexpr char name[] = "Effort";
    static constexpr char tableName[] = "effort";

    void insert(Effort::PtrType node, pqxx::work& transaction) {
      std::string cmd("INSERT INTO effort (id, text, effort) values ($1, $2, $3);");
      pqxx::params p{
        node->idString(),
        node->getText(),
        node->getEffort()
      };
      transaction.exec(cmd, p);
    }

    void update(Effort::PtrType node, pqxx::work& transaction) {
      std::string cmd("UPDATE effort SET text = $1, effort = $2 WHERE id = $3;");
      pqxx::params p{
        node->getText(),
        node->getEffort(),
        node->idString()
      };
      transaction.exec(cmd, p);
    }

    bool load(Effort::PtrType node, pqxx::work& transaction) {
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

  };

  /******************************************************************/
  // Role-specific type

  template <>
  struct DbSpecificData<fr::RequirementsManager::Role> {
    using type = fr::RequirementsManager::Role;
    static constexpr char name[] = "Role";
    static constexpr char tableName[] = "role";

    void insert(Role::PtrType node, pqxx::work& transaction) {
      std::string cmd("INSERT INTO role (id, who) VALUES ($1, $2);");
      pqxx::params p{
        node->idString(),
        node->getWho()
      };
      transaction.exec(cmd, p);
    }

    void update(Role::PtrType node, pqxx::work& transaction) {
      std::string cmd("UPDATE role SET who = $1 WHERE id = $2;");
      pqxx::params p {
        node->getWho(),
        node->idString()
      };
      transaction.exec(cmd, p);
    }

    bool load(Role::PtrType node, pqxx::work& transaction) {
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

  };

  /*****************************************************************/
  // Actor-specific type

  template <>
  struct DbSpecificData<fr::RequirementsManager::Actor> {
    using type = fr::RequirementsManager::Actor;
    static constexpr char name[] = "Actor";
    static constexpr char tableName[] = "actor";

    void insert(Actor::PtrType node, pqxx::work& transaction) {
      std::string cmd("INSERT INTO actor (id, actor) values ($1, $2);");
      pqxx::params p{
        node->idString(),
        node->getActor()
      };
      transaction.exec(cmd,p);
    }

    void update(Actor::PtrType node, pqxx::work& transaction) {
      std::string cmd("UPDATE actor SET actor = $1 WHERE id = $2;");
      pqxx::params p{
        node->getActor(),
        node->idString()
      };
      transaction.exec(cmd, p);
    }

    bool load(Actor::PtrType node, pqxx::work& transaction) {
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

    
  };

  /***********************************************************/
  // Goal-specific type

  template <>
  struct DbSpecificData<fr::RequirementsManager::Goal> {
    using type = fr::RequirementsManager::Goal;
    static constexpr char name[] = "Goal";
    static constexpr char tableName[] = "goal";

    void insert(Goal::PtrType node, pqxx::work& transaction) {      
      std::string cmd("INSERT INTO goal (id, action, outcome, context, target_date,"
                      "target_date_confidence, alignment) VALUES ($1, $2, $3, $4, $5, $6, $7);");
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

    void update(Goal::PtrType node, pqxx::work& transaction) {
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

    bool load(Goal::PtrType node, pqxx::work& transaction) {
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
    
  };

  /*********************************************************/
  // Purpose-specific type

  template <>
  struct DbSpecificData<fr::RequirementsManager::Purpose> {
    using type = fr::RequirementsManager::Purpose;
    static constexpr char name[] = "Purpose";
    static constexpr char tableName[] = "purpose";

    void insert(Purpose::PtrType node, pqxx::work& transaction) {
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

    void update(Purpose::PtrType node, pqxx::work& transaction) {
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

    bool load(Purpose::PtrType node, pqxx::work& transaction) {
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

  };

  /*************************************************************/
  // Person-specific type

  template<>
  struct DbSpecificData<fr::RequirementsManager::Person> {
    using type = fr::RequirementsManager::Person;
    static constexpr char name[] = "Person";
    static constexpr char tableName[] = "person";

    void insert(Person::PtrType node, pqxx::work& transaction) {
      std::string cmd("INSERT INTO person (id, first_name, last_name) VALUES ($1, $2, $3);");
      pqxx::params p {
        node->idString(),
        node->getFirstName(),
        node->getLastName()
      };
      transaction.exec(cmd, p);
    }

    void update(Person::PtrType node, pqxx::work& transaction) {
      std::string cmd("UPDATE person SET first_name = $1, last_name = $2 WHERE id = $3;");
      pqxx::params p {
        node->getFirstName(),
        node->getLastName(),
        node->idString()
      };
      transaction.exec(cmd, p);
    }

    bool load(Person::PtrType node, pqxx::work& transaction) {
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


  };

  /**********************************************************/
  // EmailAddress-specific type

  template <>
  struct DbSpecificData<fr::RequirementsManager::EmailAddress> {
    using type = fr::RequirementsManager::EmailAddress;
    static constexpr char name[] = "EmailAddress";
    static constexpr char tableName[] = "email_address";

    void insert(EmailAddress::PtrType node, pqxx::work& transaction) {
      std::string cmd("INSERT INTO email_address (id, address) VALUES ($1, $2);");
      pqxx::params p {
        node->idString(),
        node->getAddress()
      };
      transaction.exec(cmd,p);
    }

    void update(EmailAddress::PtrType node, pqxx::work& transaction) {
      std::string cmd("UPDATE email_address set address=$1 WHERE id = $2;");
      pqxx::params p {
        node->getAddress(),
        node->idString()
      };
      transaction.exec(cmd, p);
    }

    bool load(EmailAddress::PtrType node, pqxx::work& transaction) {
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

  };

  /************************************************************/
  // PhoneNumber-specific type

  template <>
  struct DbSpecificData<fr::RequirementsManager::PhoneNumber> {
    using type = fr::RequirementsManager::PhoneNumber;
    static constexpr char name[] = "PhoneNumber";
    static constexpr char tableName[] = "phone_number";

    void insert(PhoneNumber::PtrType node, pqxx::work& transaction) {
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

    void update(PhoneNumber::PtrType node, pqxx::work& transaction) {
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

    bool load(PhoneNumber::PtrType node, pqxx::work& transaction) {
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

  };

  /*****************************************************************/
  // InternationalAddress-specific type
  
  template <>
  struct DbSpecificData<fr::RequirementsManager::InternationalAddress> {
    using type = fr::RequirementsManager::InternationalAddress;
    static constexpr char name[] = "InternationalAddress";
    static constexpr char tableName[] = "international_address";

    void insert(InternationalAddress::PtrType node, pqxx::work& transaction) {
      std::string cmd("INSERT INTO international_address (id, country_code, address_lines,"
                      "locality, postal_code) VALUES ($1, $2, $3, $4, $5);");
      std::string addressId;

      // address lines are a pointer to a text node, so we just need
      // to check it for null and set it to the id of the text node
      // if it's not null.
      auto addressLines = node->getAddressLines();
      if (addressLines) {
        addressId = addressLines->idString();
      } else {
        addressId = "null";
      }
      
      pqxx::params p{
        node->idString(),
        node->getCountryCode(),
        addressId,
        node->getLocality(),
        node->getPostalCode()        
      };
      transaction.exec(cmd, p);
    }

    void update(InternationalAddress::PtrType node, pqxx::work& transaction) {
      std::string cmd("UPDATE international_address SET country_code = $1, address_lines = $2,"
                      "locality = $3, postal_code = $4 WHERE id = $5;");

      std::string addressId;
      auto addressLines = node->getAddressLines();
      if (addressLines) {
        addressId = addressLines->idString();
      } else {
        addressId = "null";
      }
      
      pqxx::params p{
        node->getCountryCode(),
        addressId,
        node->getLocality(),
        node->getPostalCode(),
        node->idString()
      };
      
      transaction.exec(cmd, p);
    }

    bool load(InternationalAddress::PtrType node, pqxx::work& transaction) {
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

    
  };

  /*****************************************************************/
  // USAddress-specific type

  template <>
  struct DbSpecificData<fr::RequirementsManager::USAddress> {
    using type = fr::RequirementsManager::USAddress;
    static constexpr char name[] = "USAddress";
    static constexpr char tableName[] = "us_address";

    void insert(USAddress::PtrType node, pqxx::work& transaction) {
      std::string cmd("INSERT INTO us_address (id, address_lines, city, state, zipcode) "
                      "VALUES ($1, $2, $3, $4, $5);");

      std::string addressId;
      auto addressLines = node->getAddressLines();
      if (addressLines) {
        addressId = addressLines->idString();
      } else {
        addressId = "null";
      }

      pqxx::params p{
        node->idString(),
        addressId,
        node->getCity(),
        node->getState(),
        node->getZipCode()
      };
      transaction.exec(cmd, p);
    }

    void update(USAddress::PtrType node, pqxx::work& transaction) {
      std::string cmd("UPDATE us_address SET address_lines = $1, city = $2, state = $3, "
                      "zipcode = $4 WHERE id = $5;");

      std::string addressId;
      auto addressLines = node->getAddressLines();
      if (addressLines) {
        addressId = addressLines->idString();
      } else {
        addressId = "null";
      }

      pqxx::params p{
        addressId,
        node->getCity(),
        node->getState(),
        node->getZipCode(),
        node->idString()
      };
      transaction.exec(cmd,p);
    }

    bool load(USAddress::PtrType node, pqxx::work& transaction) {
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

  };

  /***********************************************************************/
  // Event-specific type
  template <>
  struct DbSpecificData<fr::RequirementsManager::Event> {
    using type = fr::RequirementsManager::Event;
    static constexpr char name[] = "Event";
    static constexpr char tableName[] = "event";

    void insert(Event::PtrType node, pqxx::work& transaction) {
      std::string cmd("INSERT INTO event (id, name, description) VALUES ($1, $2, $3);");
      pqxx::params p{
        node->idString(),
        node->getName(),
        node->getDescription()
      };
      transaction.exec(cmd, p);
    }

    void update(Event::PtrType node, pqxx::work& transaction) {
      std::string cmd("UPDATE event SET name = $1, description = $2 WHERE id = $3");
      pqxx::params p{
        node->getName(),
        node->getDescription(),
        node->idString()
      };
      transaction.exec(cmd, p);
    }

    bool load(Event::PtrType node, pqxx::work& transaction) {
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
