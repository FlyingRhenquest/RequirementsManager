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

    bool nodeInTable() {
      return false;
    }

    void insert(Node::PtrType n, pqxx::work& transaction) {
      throw std::logic_error("Attempted to insert an unknown node type (type not specialized in PqDatabaseSpecific.h)");
    }

    void update(Node::PtrType n, pqxx::work& transaction) {
      throw std::logic_error("Attempted to update an unknown node type (type not specialized in PqDatabaseSpecific.h)");
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
  };


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
