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
#include <fr/types/Typelist.h>

namespace fr::RequirementsManager {

  /**
   * predeclare all the node types we currently have and then
   * add them to a typelist
   */

  class GraphNode;
  class Organization;
  class Product;
  class Project;
  class Requirement;
  class Story;
  class UseCase;
  class Text;
  class Completed;
  class KeyValue;
  class TimeEstimate;
  class Effort;
  class Role;
  class Actor;
  class Goal;
  class Purpose;
  class Person;
  class EmailAddress;
  class PhoneNumber;
  class InternationalAddress;
  class USAddress;
  class Event;
  class RecurringTodo;
  class Todo;

  using AllNodeTypes =
    fr::types::Typelist<GraphNode, Organization, Product, Project, Requirement,
                        Story, UseCase, Text, Completed, KeyValue, TimeEstimate,
                        Effort, Role, Actor, Goal, Purpose, Person, EmailAddress,
                        PhoneNumber, InternationalAddress, USAddress, Event,
                        RecurringTodo, Todo>;
  
}
