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

/*
 * This program uses libpgxx to create tables for nodes in a
 * PostgreSQL database
 */

#include <pqxx/pqxx>
#include <iostream>

int main(int argc, char *argv[]) {
  pqxx::connection connection;
  
  if (!connection.is_open()) {
    std::cout << "Unable to connect to database" << std::endl;
    exit(1);
  }
  
  std::cout << "Connected to " << connection.dbname() << std::endl;

  pqxx::work transaction(connection);
  // We get node type from getNodeType so be sure your nodes
  // override it. Otherwise we won't know where to look for
  // the rest of the node information
  std::string nodeTable("CREATE TABLE IF NOT EXISTS node ("
                        "id          uuid PRIMARY KEY,"
                        "node_type   VARCHAR(100) NOT NULL);");

  std::string nodeAssociationType("CREATE TYPE association_type AS ENUM('up', 'down');");
  
  std::string nodeAssociations("CREATE TABLE IF NOT EXISTS node_associations ("
                        "node          uuid PRIMARY KEY,"
                        "association   uuid,"
                        "type          association_type);");

  std::string projectTable("CREATE TABLE IF NOT EXISTS project ("
                           "id      uuid PRIMARY KEY,"
                           "name    VARCHAR(200) NOT NULL,"
                           "description TEXT);");

  std::string commitableNodeTable("CREATE TABLE IF NOT EXISTS commitable_node ("
                                  "id              uuid PRIMARY KEY,"
                                  "change_parent   uuid,"
                                  "change_child    uuid)");
                                  

  std::string productTable("CREATE TABLE IF NOT EXISTS product ("
                           "id      uuid PRIMARY KEY,"
                           "title   VARCHAR(200) NOT NULL,"
                           "description TEXT);");

  std::string organizationTable("CREATE TABLE IF NOT EXISTS organization ("
                                "id          uuid PRIMARY KEY,"
                                "locked      BOOLEAN NOT NULL DEFAULT TRUE,"
                                "name        VARCHAR(200) NOT NULL);");

  std::cout << "Checking to see if association_type exists...";
  pqxx::result res = transaction.exec("SELECT EXISTS(SELECT 1 FROM pg_type AS t JOIN pg_namespace AS n ON n.oid = t.typnamespace where t.typname = 'association_type' AND n.nspname = 'public' AND t.typtype = 'e');");
  bool enumExists = false;
  if (res.size() == 1) {
    enumExists = res[0][0].as<bool>();
  }
  if (enumExists) {
    std::cout << " Already exists" << std::endl;
  } else {
    std::cout << " Not found." << std::endl;
    std::cout << "Creating association_type...";
    transaction.exec(nodeAssociationType);
    std::cout << " Done" << std::endl;
  }

  
  std::cout << "Creating node table...";
  transaction.exec(nodeTable);
  std::cout << " Done" << std::endl;
  std::cout << "Creating node_associations table...";
  transaction.exec(nodeAssociations);
  std::cout << " Done." << std::endl;
  std::cout << "Creating organization table...";
  transaction.exec(organizationTable);
  std::cout << " Done" << std::endl;
  std::cout << "Creating commitable node table...";
  transaction.exec(commitableNodeTable);
  std::cout <<" Done" << std::endl;
  std::cout << "Creating project table...";
  transaction.exec(projectTable);
  std::cout << " Done" << std::endl;
  std::cout << "Creating product table...";
  transaction.exec(productTable);
  std::cout << " Done" << std::endl;
  
  std::cout << "Comitting transactions...";
  transaction.commit();
  std::cout << "Done." << std::endl;

  std::cout << "Processing complete." << std::endl;
}
