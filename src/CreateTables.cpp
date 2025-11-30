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

  std::string useCaseTable("CREATE TABLE IF NOT EXISTS use_case ("
                           "id        uuid PRIMARY KEY,"
                           "name      VARCHAR(200) NOT NULL);");

  std::string requirementTable("CREATE TABLE IF NOT EXISTS requirement ("
                               "id       uuid PRIMARY KEY,"
                               "title    VARCHAR(200) NOT NULL,"
                               "text     TEXT,"
                               "functional BOOLEAN NOT NULL DEFAULT FALSE);");
  
  std::string storyTable("CREATE TABLE IF NOT EXISTS story ("
                         "id           uuid PRIMARY KEY,"
                         "title        VARCHAR(200) NOT NULL,"
                         "goal         TEXT,"
                         "benefit      TEXT);");

  std::string textTable("CREATE TABLE IF NOT EXISTS text ("
                        "id            uuid PRIMARY KEY,"
                        "text          TEXT);");

  std::string completedTable("CREATE TABLE IF NOT EXISTS completed ("
                        "id            uuid PRIMARY KEY,"
                        "description   text);");

  std::string keyValueTable("CREATE TABLE IF NOT EXISTS keyvalue ("
                            "id           uuid PRIMARY KEY,"
                            "key          VARCHAR(200),"
                            "value        TEXT);");

  std::string timeEstimateTable("CREATE TABLE IF NOT EXISTS time_estimate ("
                                "id          uuid PRIMARY KEY,"
                                "text        TEXT,"
                                "estimate    BIGINT);");

  std::string effortTable("CREATE TABLE IF NOT EXISTS effort ("
                          "id             uuid PRIMARY KEY,"
                          "text           TEXT,"
                          "effort         BIGINT);");

  std::string roleTable("CREATE TABLE IF NOT EXISTS role ("
                        "id               uuid PRIMARY KEY,"
                        "who              VARCHAR(200) NOT NULL);");

  std::string actorTable("CREATE TABLE IF NOT EXISTS actor ("
                         "id              uuid PRIMARY KEY,"
                         "actor           varchar(200) NOT NULL);");

  std::string goalTable("CREATE TABLE IF NOT EXISTS goal ("
                        "id               uuid PRIMARY KEY,"
                        "action           TEXT,"
                        "outcome          TEXT,"
                        "context          TEXT,"
                        "target_date      TIMESTAMP,"
                        "target_date_confidence VARCHAR(200),"
                        "alignment        TEXT);");

  std::string purposeTable("CREATE TABLE IF NOT EXISTS purpose ("
                           "id            uuid PRIMARY KEY,"
                           "description   TEXT,"
                           "deadline      TIMESTAMP,"
                           "deadline_confidence VARCHAR(200));");

  std::string personTable("CREATE TABLE IF NOT EXISTS person ("
                          "id             uuid PRIMARY KEY,"
                          "first_name     VARCHAR(200) NOT NULL,"
                          "last_name      VARCHAR(200) NOT NULL);");

  std::string emailAddressTable("CREATE TABLE IF NOT EXISTS email_address ("
                                "id       uuid PRIMARY KEY,"
                                "address  VARCHAR(200) NOT NULL);");

  std::string phoneNumberTable("CREATE TABLE IF NOT EXISTS phone_number ("
                               "id             uuid PRIMARY KEY,"
                               "countrycode    VARCHAR(10),"
                               "number         VARCHAR(20),"
                               "phone_type     VARCHAR(20));");

  // InternationalAddress uses a text node for address lines. Just
  // store the first text node UUID in address lines here and the
  // store/retrieve code will grab the first one and traverse it
  // as if it were any other node.
  std::string internationalAddressTable("CREATE TABLE IF NOT EXISTS international_address ("
                                        "id            uuid PRIMARY KEY,"
                                        "country_code  VARCHAR(20),"
                                        "address_lines uuid,"
                                        "locality      VARCHAR(200),"
                                        "postal_code   VARCHAR(50));");

  // USAddress does the same thing as international address with its
  // address lines.
  std::string usAddressTable("CREATE TABLE IF NOT EXISTS us_address ("
                              "id                  uuid PRIMARY KEY,"
                              "address_lines       uuid,"
                              "city                VARCHAR(100),"
                              "state               VARCHAR(40),"
                              "zipcode             VARCHAR(20));");

  std::string eventTable("CREATE TABLE IF NOT EXISTS event ("
                         "id                  uuid PRIMARY KEY,"
                         "name                VARCHAR(200),"
                         "description         TEXT);");
                               

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
  std::cout << "Creating use case table...";
  transaction.exec(useCaseTable);
  std::cout << " Done" << std::endl;
  std::cout << "Creating requirement table...";
  transaction.exec(requirementTable);
  std::cout << " Done" << std::endl;
  std::cout << "Creating story table...";
  transaction.exec(storyTable);
  std::cout << " Done" << std::endl;
  std::cout << "Creating text table...";
  transaction.exec(textTable);
  std::cout << " Done" << std::endl;
  std::cout <<"Creating completed table...";
  transaction.exec(completedTable);
  std::cout << " Done" << std::endl;
  std::cout << "Creating keyvalue table...";
  transaction.exec(keyValueTable);
  std::cout << " Done" << std::endl;
  std::cout << "Creating time_estimate table...";
  transaction.exec(timeEstimateTable);
  std::cout << " Done" << std::endl;
  std::cout << "Creating effort table...";
  transaction.exec(effortTable);
  std::cout << " Done" << std::endl;
  std::cout << "Creating role table...";
  transaction.exec(roleTable);
  std::cout << " Done" << std::endl;
  std::cout << "Creating actor table...";
  transaction.exec(actorTable);
  std::cout << " Done" << std::endl;
  std::cout << "Creating goal table...";
  transaction.exec(goalTable);
  std::cout << " Done" << std::endl;
  std::cout << "Creating purpose table...";
  transaction.exec(purposeTable);
  std::cout << " Done" << std::endl;
  std::cout << "Creating person table...";
  transaction.exec(personTable);
  std::cout << " Done" << std::endl;
  std::cout << "Creating email address table...";
  transaction.exec(emailAddressTable);
  std::cout << " Done" << std::endl;
  std::cout << "Creating phone number table...";
  transaction.exec(phoneNumberTable);
  std::cout << " Done" << std::endl;
  std::cout << "Creating international address table...";
  transaction.exec(internationalAddressTable);
  std::cout << " Done" << std::endl;
  std::cout << "Creating USAddress Table...";
  transaction.exec(usAddressTable);
  std::cout << " Done" << std::endl;
  std::cout << "Creating Event Table...";
  transaction.exec(eventTable);

  std::cout << " Done" << std::endl;
  std::cout << "Comitting transactions...";
  transaction.commit();
  std::cout << "Done." << std::endl;

  std::cout << "Processing complete." << std::endl;
}
