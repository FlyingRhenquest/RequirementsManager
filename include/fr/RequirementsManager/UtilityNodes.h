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

#include <fr/RequirementsManager/Node.h>
#include <cereal/types/base_class.hpp>
#include <cereal/types/polymorphic.hpp>
#include <cereal/types/chrono.hpp>
#include <ctime>
#include <chrono>
#include <list>
#include <memory>
#include <string>

namespace fr::RequirementsManager {

  // Some utility nodes for requirements manager
  // These provide functionality useful across many
  // of the node types and can just be dropped in the
  // down-list of any node


  /**
   * This is just a node with some text in it. If you
   * need to add text or an anotation to a node
   * and I didn't provide a field for it, you can
   * just drop one of these in the node.
   */
  
  class Text : public Node {
  public:
    using Type = Text;
    using PtrType = std::shared_ptr<Type>;
    using Parent = Node;

  private:
    std::string _text;

  public:
    Text() = default;
    virtual ~Text() = default;

    std::string getNodeType() const override {
      return "Text";
    }
    
    void setText(const std::string &text) {
      _text = text;
    }

    std::string getText() const {
      return _text;
    }

    template <class Archive>
    void save(Archive &ar) const {
      ar(cereal::make_nvp(Parent::getNodeType(), cereal::base_class<Parent>(this)));
      ar(cereal::make_nvp("text", _text));
    }

    template <class Archive>
    void load(Archive &ar) {
      ar(cereal::base_class<Parent>(this));
      ar(_text);
    }
  };

  /**
   * A "Completed" node you can just arbitrarily drop in somewhere.
   * if you want to track your work or where you are in the plan or
   * something. Has a description, feel free to drop a person/
   * stakeholder node in the down list to indicate who did it. Feel
   * free to drop text nodes in for notes. Feel free to drop an
   * effort node in for total effort. Starting to see how this works
   * now?
   */
  class Completed : public Node {
    std::string _description;

  public:
    using Type = Completed;
    using PtrType = std::shared_ptr<Type>;
    using Parent = Node;

    std::string getNodeType() const override {
      return "Completed";
    }
    
    void setDescription(const std::string& description) {
      _description = description;
    }

    std::string getDescription() const {
      return _description;
    }

    template <class Archive>
    void save(Archive &ar) const {
      ar(cereal::make_nvp(Parent::getNodeType(), cereal::base_class<Parent>(this)));
      ar(cereal::make_nvp("Description", _description));
    }

    template <class Archive>
    void load(Archive &ar) {
      ar(cereal::base_class<Parent>(this));
      ar(_description);
    }
    
  };
  
  /**
   * This is a key/value node (both strings). Note that we don't
   * enforce anything about key uniqueness, but you can generally
   * expect the node ID to be unique unless you're looking at
   * exactly the same KeyValue node.
   */
  class KeyValue : public Node {
  public:
    using Type = KeyValue;
    using PtrType = std::shared_ptr<Type>;
    using Parent = Node;

  private:
    std::string _key;
    std::string _value;

  public:

    KeyValue() = default;
    virtual ~KeyValue() = default;

    std::string getNodeType() const override {
      return "KeyValue";
    }
    
    void setKey(const std::string& key) {
      _key = key;
    }

    void setValue(const std::string &value) {
      _value = value;
    }

    std::string getKey() const {
      return _key;
    }

    std::string getValue() const {
      return _value;
    }

    template <class Archive>
    void save(Archive &ar) const {
      ar(cereal::make_nvp(Parent::getNodeType(), cereal::base_class<Parent>(this)));
      ar(cereal::make_nvp("key", _key));
      ar(cereal::make_nvp("value", _value));
    }
    
    template <class Archive>
    void load(Archive &ar) {
      ar(cereal::base_class<Parent>(this));
      ar(_key);
      ar(_value);
    }
    
  };

  // Nodes relating to time

  /**
   * TimeEstimate contains a std::chrono::duration in seconds.
   * It also includes a string you can use to tag the estimate with
   * some descriptive text. You can just drop one of these into
   * the down-list of a project, requirement, design, task, etc.
   * Don't put one in or put one in and set it to zero if you
   * need more info to provide an estimate.
   *
   * Estimate doesn't enforce any particular unit but if I
   * ever display it anywhere I'll probably assume seconds
   * as a duration. You could also store a future date from
   * the standard posix epoch.
   */

  class TimeEstimate : public Node {
  public:
    using Type = TimeEstimate;
    using PtrType = std::shared_ptr<Type>;
    using Parent = Node;

  private:
    // Descriptive text for this estimate
    std::string _text;
    // Estimate probably in seconds
    unsigned long _estimate;
    // _estimate should be taken as an offset from
    // whenever the work starts. If started is true
    // then the remaining work should be the start
    // date plus the estimate date minus the current date.
    bool _started;
    // Start timestamp
    time_t _startTimestamp;

  public:

    TimeEstimate() : _estimate(0l),
                     _started(false),
                     _startTimestamp(0l) {
    }
    
    virtual ~TimeEstimate() = default;

    std::string getNodeType() const override {
      return "TimeEstimate";
    }
    
    void setText(const std::string &text) {
      _text = text;
    }

    void setEstimate(unsigned long estimate) {
      _estimate = estimate;
    }

    std::string getText() const {
      return _text;
    }

    unsigned long getEstimate() const {
      return _estimate;
    }

    bool getStarted() const {
      return _started;
    }

    void setStarted(bool started) {
      _started = started;
    }

    time_t getStartTimestamp() const {
      return _startTimestamp;
    }

    void setStartTimestamp(time_t stamp) {
      _startTimestamp = stamp;
    }

    template <class Archive>
    void save(Archive &ar) const {
      ar(cereal::make_nvp(Parent::getNodeType(), cereal::base_class<Parent>(this)));
      ar(cereal::make_nvp("text", _text));
      ar(cereal::make_nvp("estimate", _estimate));
      ar(cereal::make_nvp("started", _started));
      ar(cereal::make_nvp("startTimestamp", _startTimestamp));
    }

    template <class Archive>
    void load(Archive &ar) {
      ar(cereal::base_class<Parent>(this));
      ar(_text);
      ar(_estimate);
      ar(_started);
      ar(_startTimestamp);
    }
  };

  /**
   * Effort allows you to update a task/requirement/project with the
   * amount of effort that has been put into it so far. You can just drop
   * an effort node into a task whenever you work some. The system should
   * accumulate these into a total effort value once I have some
   * actual business logic.
   */
  
  class Effort : public Node {
  public:
    using Type = Effort;
    using PtrType = std::shared_ptr<Effort>;
    using Parent = Node;
    
  private:
    // Some descriptive text for this effort (System doesn't require anything)
    std::string _text;
    // Amount of time (in seconds) for this effort.
    unsigned long _effort;
    
  public:
    Effort() = default;
    virtual ~Effort() = default;

    std::string getNodeType() const override {
      return "Effort";
    }
    
    void setText(const std::string& text) {
      _text = text;
    }

    // Set effort (in seconds) spent for this work. Nodes where time make
    // sense should be able to handle multiple effort nodes, so you can
    // update them near the time when you worked on them and the effort
    // nodes can be consolidated (So I guess technically we're time
    // tracking now if we want to.)
    
    // TODO: Should I include a billable flag for effort? I kinda feel like
    // that should be somewhat dictated by the node type the effort is attached to
    void setEffort(unsigned long effort) {
      _effort = effort;
    }

    std::string getText() const {
      return _text;
    }

    unsigned long getEffort() const {
      return _effort;;
    }

    template <class Archive>
    void save(Archive &ar) const {
      ar(cereal::make_nvp(Parent::getNodeType(), cereal::base_class<Parent>(this)));
      ar(cereal::make_nvp("text", _text));
      ar(cereal::make_nvp("effort", _effort));
    }

    template <class Archive>
    void load(Archive &ar) {
      ar(cereal::base_class<Parent>(this));
      ar(_text);
      ar(_effort);
    }
  };

  /**
   * A role node you could drop into a story node.
   *
   * (TODO: should this go in the story up-list, down-list or
   * reside as a separate variable in the Story?)
   */
  
  class Role : public Node {
    // Usually these go As a "...". Just put the ... part
    // in who, like "Administrator", "Customer", "Developer",
    // etc.
    // Note you could *also* drop an actual Person node into
    // the up or down list of this node if you're so inclined.
    std::string _who;

  public:
    using Type = Role;
    using PtrType = std::shared_ptr<Type>;
    using Parent = Node;
    
    Role() = default;
    virtual ~Role() = default;

    std::string getNodeType() const override {
      return "Role";
    }

    std::string getWho() const {
      return _who;
    }

    void setWho(const std::string& who) {
      _who = who;
    }

    template <class Archive>
    void save(Archive &ar) const {
      ar(cereal::make_nvp(Parent::getNodeType(), cereal::base_class<Parent>(this)));
      ar(cereal::make_nvp("who", _who));
    }
	
    template <class Archive>
    void load(Archive &ar) {
      ar(cereal::base_class<Parent>(this));
      ar(_who);
    }
    
  };

  /**
   * Actor node can be added to use case or user story down lists
   * to identify actors in those stories
   */

  class Actor : public Node {
    std::string _actor;

  public:
    using Type = Actor;
    using PtrType = std::shared_ptr<Type>;
    using Parent = Node;

    std::string getNodeType() const override {
      return "Actor";
    }

    std::string getActor() const {
      return _actor;
    }

    void setActor(const std::string& actor) {
      _actor = actor;
    }

    template <class Archive>
    void save(Archive &ar) const {
      ar(cereal::make_nvp(Parent::getNodeType(), cereal::base_class<Parent>(this)));
      ar(_actor);
    }

    template <class Archive>
    void load(Archive& ar) {
      ar(cereal::base_class<Parent>(this));
      ar(_actor);
    }
  };

  /**
   * A goal node. You can drop in things that need goals
   *
   * I'm on the fence about making this specifically commitable.
   * I don't think they should really change that much once
   * defined.
   */

  class Goal : public Node {
    // What will be done?
    std::string _action;
    // What defines success?
    std::string _outcome;
    // Who/What is this goal targeting?
    std::string _context;
    // Target date (POSIX timestamp)
    unsigned long _targetDate;
    // Target Date confidence/priority (High/med/low/thereaboutsish)
    // Feel free to throw a "we all die if this target date isn't met"
    // in there.
    std::string _targetDateConfidence;
    // Why we have goal first place?
    std::string _alignment;

  public:

    using Type = Goal;
    using PtrType = std::shared_ptr<Type>;
    using Parent = Node;
    
    Goal() = default;
    virtual ~Goal() = default;

    std::string getNodeType() const override {
      return "Goal";
    }

    void setAction(const std::string& action) {
      _action = action;
    }

    void setOutcome(const std::string& outcome) {
      _outcome = outcome;
    }

    void setContext(const std::string &context) {
      _context = context;
    }

    void setTargetDate(unsigned long targetDate) {
      _targetDate = targetDate;
    }

    void setTargetDateConfidence(const std::string &targetDateConfidence) {
      _targetDateConfidence = targetDateConfidence;
    }

    void setAlignment(const std::string &alignment) {
      _alignment = alignment;
    }

    std::string getAction() const {
      return _action;
    }

    std::string getOutcome() const {
      return _outcome;
    }

    std::string getContext() const {
      return _context;
    }

    unsigned long getTargetDate() const {
      return _targetDate;
    }

    std::string getTargetDateConfidence() {
      return _targetDateConfidence;
    }

    std::string getAlignment() {
      return _alignment;
    }

    template <class Archive>
    void save(Archive &ar) const {
      ar(cereal::make_nvp(Parent::getNodeType(), cereal::base_class<Parent>(this)));
      ar(cereal::make_nvp("action", _action));
      ar(cereal::make_nvp("outcome", _outcome));
      ar(cereal::make_nvp("context", _context));
      ar(cereal::make_nvp("targetDate", _targetDate));
      ar(cereal::make_nvp("targetDateConfidence", _targetDateConfidence));
      ar(cereal::make_nvp("alignment", _alignment));      
    }

    template <class Archive>
    void load(Archive &ar) {
      ar(cereal::base_class<Parent>(this));
      ar(_action);
      ar(_outcome);
      ar(_context);
      ar(_targetDate);
      ar(_targetDateConfidence);
      ar(_alignment);
    }
    
  };

  /**
   * A Purpose node you can drop in anything that needs a purpose.
   */

  class Purpose : public Node {
    // Description of the purpose (IE: You pass butter)
    std::string _description;
    // When the delivery of this purpose is due (POSIX timestamp)
    unsigned long _deadline;
    // Deadline priority/confidence/wiggle room
    // IE: "We all die if we don't meet this deadline." I'd
    // assume that would be VERY HIGH priority
    std::string _deadlineConfidence;

  public:
    using Type = Purpose;
    using PtrType = std::shared_ptr<Type>;
    using Parent = Node;

    Purpose() = default;
    virtual ~Purpose() = default;

    std::string getNodeType() const override {
      return "Purpose";
    }

    void setDescription(const std::string description) {
      _description = description;
    }

    void setDeadline(unsigned long deadline) {
      _deadline = deadline;
    }

    void setDeadlineConfidence(const std::string &deadlineConfidence) {
      _deadlineConfidence = deadlineConfidence;
    }

    std::string getDescription() const {
      return _description;
    }

    unsigned long getDeadline() const {
      return _deadline;
    }

    std::string getDeadlineConfidence() const {
      return _deadlineConfidence;
    }

    template <class Archive>
    void save(Archive &ar) const {
      ar(cereal::make_nvp(Parent::getNodeType(), cereal::base_class<Parent>(this)));
      ar(cereal::make_nvp("description", _description));
      ar(cereal::make_nvp("deadline", _deadline));
      ar(cereal::make_nvp("deadlineConfidence", _deadlineConfidence));
    }

    template <class Archive>
    void load(Archive &ar) {
      ar(cereal::base_class<Parent>(this));
      ar(_description);
      ar(_deadline);
      ar(_deadlineConfidence);
    }
    
  };

  /**
   * A Person node. This basically just holds a person's name and
   * provides a place to hang other nodes (Email address, phones
   * etc) off of. Since a Person node might involve more pii than
   * want to store in the system, I might want to think about
   * security and access controls before putting too many
   * things in here.
   */

  class Person : public Node {
    std::string _lastName;
    std::string _firstName;
    // TODO: Consider putting additional fields in here.

  public:
    using Type = Person;
    using PtrType = std::shared_ptr<Type>;
    using Parent = Node;

    Person() = default;
    virtual ~Person() = default;

    std::string getNodeType() const override {
      return "Person";
    }

    void setLastName(const std::string &lastName) {
      _lastName = lastName;
    }

    void setFirstName(const std::string &firstName) {
      _firstName = firstName;
    }

    std::string getLastName() const {
      return _lastName;
    }

    std::string getFirstName() const {
      return _firstName;
    }

    template <class Archive>
    void save(Archive &ar) const {
      ar(cereal::make_nvp(Parent::getNodeType(), cereal::base_class<Parent>(this)));
      ar(_lastName);
      ar(_firstName);
    }

    template <class Archive>
    void load(Archive &ar) {
      ar(cereal::base_class<Parent>(this));
      ar(_lastName);
      ar(_firstName);
    }
    
  };

  class EmailAddress : public Node {
    std::string _address;

  public:
    using Type = EmailAddress;
    using PtrType = std::shared_ptr<Type>;
    using Parent = Node;

    EmailAddress() = default;
    virtual ~EmailAddress() = default;

    std::string getNodeType() const override {
      return "EmailAddress";
    }
    
    void setAddress(const std::string& address) {
      _address = address;
    }

    std::string getAddress() const {
      return _address;
    }

    template <class Archive>
    void save(Archive &ar) const {
      ar(cereal::make_nvp(Parent::getNodeType(), cereal::base_class<Parent>(this)));
      ar(cereal::make_nvp("address", _address));
    }

    template <class Archive>
    void load(Archive &ar) {
      ar(cereal::base_class<Parent>(this));
      ar(_address);
    }
  };

  /**
   * A phone number that could be dropped into the down node
   * of a Person or something.
   */

  class PhoneNumber : public Node {
    // Nominally optional if you're a one-country project
    std::string _countryCode;
    std::string _number;
    // Cell, Landline, Home, Office, etc
    std::string _phoneType;

  public:
    using Type = PhoneNumber;
    using PtrType = std::shared_ptr<Type>;
    using Parent = Node;

    PhoneNumber() = default;
    virtual ~PhoneNumber() = default;

    std::string getNodeType() const override {
      return "PhoneNumber";
    }

    void setCountryCode(const std::string &countryCode) {
      _countryCode = countryCode;
    }

    std::string getCountryCode() const {
      return _countryCode;
    }
    
    void setNumber(const std::string &number) {
      _number = number;
    }

    void setPhoneType(const std::string &phoneType) {
      _phoneType = phoneType;
    }

    std::string getNumber() const {
      return _number;
    }

    std::string getPhoneType() const {
      return _phoneType;
    }

    template <class Archive>
    void save(Archive &ar) const {
      ar(cereal::make_nvp(Parent::getNodeType(), cereal::base_class<Parent>(this)));
      ar(cereal::make_nvp("countryCode", _countryCode));
      ar(cereal::make_nvp("number", _number));
      ar(cereal::make_nvp("phoneType", _phoneType));
    }

    template <class Archive>
    void load(Archive &ar) {
      ar(cereal::base_class<Parent>(this));
      ar(_countryCode);
      ar(_number);
      ar(_phoneType);
    }    
  };

  /**
   * International address. Will do a US address specialization
   * that inherits from this. Drop in the down node of a person
   * or organization or thing that has an address.
   *
   * This one is going to be a bit weird.
   */

  class InternationalAddress : public Node {
    // Ideally ISO 3166-1 Country code
    std::string _countryCode;
    // Address lines. In the USA this would be street
    // address. This is a text node, so if you need more
    // than one line, just drop another text node in the down list
    // of the first one.
    // TODO: Make sure display code handles this
    Text::PtrType _addressLines;
    // City/Town
    std::string _locality;
    // Zip code/Post Code
    std::string _postalCode;
  public:
    using Type = InternationalAddress;
    using PtrType = std::shared_ptr<Type>;
    using Parent = Node;

    InternationalAddress() = default;
    virtual ~InternationalAddress() = default;

    std::string getNodeType() const override {
      return "InternationalAddress";
    }

    void setCountryCode(const std::string &countryCode) {
      _countryCode = countryCode;
    }
    // Just construct your whole address lines text node
    // and slap it in here. I'm already predicting this will
    // cause me problems in the future lol.
    void setAddressLines(Text::PtrType addressLines) {
      _addressLines = addressLines;
    }

    void setLocality(const std::string &locality) {
      _locality = locality;
    }

    void setPostalCode(const std::string &postalCode) {
      _postalCode = postalCode;
    }

    std::string getCountryCode() const {
      return _countryCode;      
    }

    // Since this one is returning a pointer, I can't guarantee
    // constness
    
    Text::PtrType getAddressLines() {
      return _addressLines;
    }

    std::string getLocality() const {
      return _locality;
    }

    std::string getPostalCode() const {
      return _postalCode;
    }

    template <class Archive>
    void save(Archive &ar) const {
      ar(cereal::make_nvp(Parent::getNodeType(), cereal::base_class<Parent>(this)));
      ar(cereal::make_nvp("countryCode", _countryCode));
      ar(cereal::make_nvp("addressLines", _addressLines)); // Gotta admit that will just work
      ar(cereal::make_nvp("locality", _locality));
      ar(cereal::make_nvp("postalCode", _postalCode));
    }

    template <class Archive>
    void load(Archive &ar) {
      ar(cereal::base_class<Parent>(this));
      ar(_countryCode);
      ar(_addressLines);
      ar(_locality);
      ar(_postalCode);
    }
    
  };

  /**
   * USAddress funnily does NOT inherit from InternationalAddress
   */

  class USAddress : public Node {
    Text::PtrType _addressLines;
    std::string _city;
    std::string _state;
    std::string _zipCode;

  public:
    using Type = USAddress;
    using PtrType = std::shared_ptr<Type>;
    using Parent = Node;

    USAddress() = default;
    virtual ~USAddress() = default;

    std::string getNodeType() const override {
      return "USAddress";
    }

    // Like InternationaAddress, just construct your address
    // lines from text nodes and drop the whole structure in
    // here (one node per address line)
    void setAddressLines(Text::PtrType addressLines) {
      _addressLines = addressLines;
    }

    void setCity(const std::string& city) {
      _city = city;
    }

    void setState(const std::string& state) {
      _state = state;
    }

    void setZipCode(const std::string &zipCode) {
      _zipCode = zipCode;
    }

    // Can't guarantee this is const since we're returning a pointer
    Text::PtrType getAddressLines() {
      return _addressLines;
    }

    std::string getCity() const {
      return _city;
    }

    std::string getState() const {
      return _state;
    }

    std::string getZipCode() const {
      return _zipCode;
    }

    template <class Archive>
    void save(Archive &ar) const {
      ar(cereal::make_nvp(Parent::getNodeType(), cereal::base_class<Parent>(this)));
      ar(cereal::make_nvp("addressLines", _addressLines));
      ar(cereal::make_nvp("city", _city));
      ar(cereal::make_nvp("state", _state));
      ar(cereal::make_nvp("zipCode", _zipCode));
    }

    template <class Archive>
    void load(Archive &ar) {
      ar(cereal::base_class<Parent>(this));
      ar(_addressLines);
      ar(_city);
      ar(_state);
      ar(_zipCode);
    }
    
  };
  
  /**
   * An event node. This is just the name and possibly description
   * of an event which can be used as a Trigger or to define flows
   * in a use case. Each flow should just be a series of events in
   * the use case down-list. Any given flow should just keep
   * adding events to the previous event's down-list until
   * the flow is complete.
   */

  class Event : public Node {
    std::string _name;
    std::string _description;

  public:
    using Type = Event;
    using PtrType = std::shared_ptr<Type>;
    using Parent = Node;

    std::string getNodeType() const override {
      return "Event";
    }
    
    std::string getName() const {
      return _name;
    }

    void setName(const std::string &name) {
      _name = name;
    }

    std::string getDescription() const {
      return _description;
    }

    void setDescription(const std::string& description) {
      _description = description;
    }

    template <class Archive>
    void save(Archive &ar) const {
      ar(cereal::make_nvp(Parent::getNodeType(), cereal::base_class<Parent>(this)));
      ar(cereal::make_nvp("name", _name));
      ar(cereal::make_nvp("description", _description));
    }

    template <class Archive>
    void load(Archive &ar) {
      ar(cereal::base_class<Parent>(this));
      ar(_name);
      ar(_description);
    }
  };

}

CEREAL_REGISTER_TYPE(fr::RequirementsManager::Text);
CEREAL_REGISTER_TYPE(fr::RequirementsManager::Completed);
CEREAL_REGISTER_TYPE(fr::RequirementsManager::KeyValue);
CEREAL_REGISTER_TYPE(fr::RequirementsManager::TimeEstimate);
CEREAL_REGISTER_TYPE(fr::RequirementsManager::Effort);
CEREAL_REGISTER_TYPE(fr::RequirementsManager::Role);
CEREAL_REGISTER_TYPE(fr::RequirementsManager::Actor);
CEREAL_REGISTER_TYPE(fr::RequirementsManager::Goal);
CEREAL_REGISTER_TYPE(fr::RequirementsManager::Purpose);
CEREAL_REGISTER_TYPE(fr::RequirementsManager::Person);
CEREAL_REGISTER_TYPE(fr::RequirementsManager::PhoneNumber);
CEREAL_REGISTER_TYPE(fr::RequirementsManager::InternationalAddress);
CEREAL_REGISTER_TYPE(fr::RequirementsManager::USAddress);
CEREAL_REGISTER_TYPE(fr::RequirementsManager::Event);
