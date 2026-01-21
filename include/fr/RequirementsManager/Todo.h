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

#include <chrono>
#include <ctime>
#include <fr/RequirementsManager/Node.h>

namespace fr::RequirementsManager {

  /**
   * Recurring Todos can be used to spawn regular todos. I initially
   * thought I'd keep them in the same class, but they really should
   * be separated in the database, so they should really be separated
   * in the data model too.
   *
   * Recurring Todos won't respawn if there's an existing to-do
   * that is not marked "completed" associated through _spawnedFrom
   * with the original todo unless manually instructed to create
   * a new one via the UI.
   *
   * Recurring Todos can be removed from the recurring todo table
   * without affecting any of the regular todos that were created
   * from it.
   */

  class RecurringTodo : public Node {
    // Description
    std::string _description;
    time_t _created;
    // _recurringInterval is only relevant if _recurring is true.
    // The following flags indicate whether it's number of seconds,
    // which can be used to indicate respawn for days or weeks,
    // a specific day of every month or a specific day of
    // every year.
    time_t _recurringInterval;
    // Recurring flag is (one of these will be true.)
    bool _seconds;
    bool _dayOfMonth;
    bool _dayOfYear;

  public:
    using Type = RecurringTodo;
    using Parent = Node;
    using PtrType = std::shared_ptr<Type>;

    RecurringTodo() : _created(0l),
                      _recurringInterval(0l),
                      _seconds(false),
                      _dayOfMonth(false),
                      _dayOfYear(false) {
      auto const now = std::chrono::system_clock::now();
      _created = std::chrono::system_clock::to_time_t(now);
    }

    virtual ~RecurringTodo() {}

    std::string getNodeType() const override {
      return "RecurringTodo";
    }
    
    time_t getCreated() {
      return _created;
    }

    // I don't anticipate this being used that much but sometimes
    // you have to do this sort of thing.
    void setCreated(time_t created) {
      _created = created;
    }

    std::string getDescription() {
      return _description;
    }
    
    void setDescription(const std::string& description) {
      _description = description;
    }
    
    time_t getRecurringInterval() {
      return _recurringInterval;
    }
    
    void setRecurringInterval(time_t interval) {
      _recurringInterval = interval;
    }

    void setSecondsFlag(bool seconds) {
      _seconds = seconds;
    }
    
    bool getSecondsFlag() {
      return _seconds;
    }

    void setDayOfMonthFlag(bool dayOfMonth) {
      _dayOfMonth = dayOfMonth;
    }

    bool getDayOfMonthFlag() {
      return _dayOfMonth;
    }

    void setDayOfYearFlag(bool dayOfYear) {
      _dayOfYear = dayOfYear;
    }

    bool getDayOfYearFlag() {
      return _dayOfYear;
    }

    template <typename Archive>
    void save(Archive &ar) const {
      ar(cereal::make_nvp(Parent::getNodeType(), cereal::base_class<Parent>(this)));
      ar(cereal::make_nvp("description", _description));
      ar(cereal::make_nvp("created", _created));
      ar(cereal::make_nvp("recurringInterval", _recurringInterval));
      ar(cereal::make_nvp("secondsFlag", _seconds));
      ar(cereal::make_nvp("dayOfMonthFlag", _dayOfMonth));
      ar(cereal::make_nvp("dayOfYearFlag", _dayOfYear));
    }

    template <typename Archive>
    void load(Archive &ar) {
      ar(cereal::base_class<Parent>(this));
      ar(_description);
      ar(_created);
      ar(_recurringInterval);
      ar(_seconds);
      ar(_dayOfMonth);
      ar(_dayOfYear);
    }
    
  };
  
  /**
   * A Todo node to add to-dos. There doesn't have to be much inside
   * this node, as you can add additional data by linking other nodes
   * to this one.
   *
   * If a recurring todo has recurring todos in its down list,
   * todos will be spawned for those recurring todos and will
   * be added to the returned todo from "fromRecurring". This allows
   * you to create task templates with multiple tasks in them,
   * and generate todos for the entire task.
   *
   * TODO: The more I think about todos the more complex the interactions
   * seem to get. I should no overthink them at this point, but they
   * should behave sensibly most of the time. I think I do want a
   * "Respawn now" in the UI for recurring tasks, and I might want
   * to consider flooring at least recurring todos to a sensible
   * minimum value, like 1 day (86400 seconds) so athey always
   * show up at midnight, maybe.
   *
   */
  
  class Todo : public Node {
    // Task description
    std::string _description;
    // Date the node was created.
    time_t _created;
    // Due date
    time_t _due;
    bool _completed;
    time_t _dateCompleted;
    // _spawnedFrom is set if this to-do is spawned from a
    // recurring one. If a Todo has an empty _spawnedFrom and
    // is marked recurring it's the original recurring Todo.
    boost::uuids::uuid _spawnedFrom;

  public:
    using Type = Todo;
    using Parent = Node;
    using PtrType = std::shared_ptr<Type>;

    // Create a Todo from a recurring todo
    static PtrType fromRecurring(RecurringTodo::PtrType from) {
      PtrType todo = std::make_shared<Todo>();
      todo->setDescription(from->getDescription());
      todo->setSpawnedFrom(from->id);
      for (auto node : from->down) {
        auto recurringChild = dynamic_pointer_cast<RecurringTodo>(node);
        if (recurringChild) {
          PtrType child = Todo::fromRecurring(recurringChild);
          todo->addDown(child);
        }
      }
      return todo;
    }
    
    Todo() : _created(0l),
             _due(0l),
             _completed(false),
             _dateCompleted(0l) {
      auto const now = std::chrono::system_clock::now();
      _created = std::chrono::system_clock::to_time_t(now);
    }

    virtual ~Todo() {}

    std::string getNodeType() const override {
      return "Todo";
    }
    
    std::string getDescription() {
      return _description;
    }

    void setDescription(const std::string& description) {
      _description = description;
    }

    // It kind of doesn't make sense to be able to set the
    // created value, but sometimes people need to do stuff
    // like that.

    time_t getCreated() {
      return _created;
    }

    void setCreated(time_t created) {
      _created = created;
    }

    time_t getDue() {
      return _due;
    }

    void setDue(time_t due) {
      _due = due;
    }

    bool getCompleted() {
      return _completed;
    }

    void setCompleted(bool completed) {
      _completed = completed;
    }

    time_t getDateCompleted() {
      return _dateCompleted;
    }

    void setDateCompleted(time_t dateCompleted) {
      _dateCompleted = dateCompleted;
    }

    boost::uuids::uuid getSpawnedFrom() {
      return _spawnedFrom;
    }

    void setSpawnedFrom(boost::uuids::uuid spawnedFrom) {
      _spawnedFrom = spawnedFrom;
    }

    template <typename Archive>
    void save(Archive &ar) const {
      ar(cereal::make_nvp(Parent::getNodeType(), cereal::base_class<Parent>(this)));
      ar(cereal::make_nvp("description", _description));
      ar(cereal::make_nvp("created", _created));
      ar(cereal::make_nvp("due", _due));
      ar(cereal::make_nvp("completed", _completed));
      ar(cereal::make_nvp("spawnedFrom", boost::uuids::to_string(_spawnedFrom)));
    }

    template <typename Archive>
    void load(Archive &ar) {
      ar(cereal::base_class<Parent>(this));
      ar(_description);
      ar(_created);
      ar(_due);
      ar(_completed);
      boost::uuids::string_generator generator;
      std::string uuid_str;
      ar(uuid_str);
      _spawnedFrom = generator(uuid_str);
    }
    
  };
  
}
