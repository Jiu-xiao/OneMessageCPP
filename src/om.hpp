#ifndef __OM_H_CPP__
#define __OM_H_CPP__

#include "om.h"

class Message {
 public:
  Message() { om_init(); }

  template <typename Data, typename Arg>
  class Callback {
   public:
    Callback(bool (*fun)(Data& data, Arg arg), Arg arg)
        : fun_(fun), arg_(arg) {}

    static om_status_t Call(om_msg_t* msg, void* arg) {
      Callback* self = static_cast<Callback*>(arg);
      auto data = static_cast<Data*>(msg->buff);

      return self->fun_(*data, self->arg_) ? OM_OK : OM_ERROR;
    }

    bool (*fun_)(Data& data, Arg arg);
    Arg arg_;
  };

  template <typename Data>
  class Topic {
   public:
    Topic(const char* name) {
      this->om_topic_ = om_config_topic(NULL, "A", name, sizeof(Data));
    }

    Topic(om_topic_t* topic) { this->om_topic_ = topic; }

    bool Link(Topic& source) {
      return om_core_link(source, this->om_topic_) == OM_OK;
    }

    bool Link(om_topic_t* source) {
      return om_core_link(source, this->om_topic_) == OM_OK;
    }

    bool Link(const char* source_name) {
      return om_core_link(om_find_topic(source_name, UINT32_MAX),
                          this->om_topic_) == OM_OK;
    }

    static om_topic_t* Find(const char* name) { return om_find_topic(name, 0); }

    template <typename Fun, typename Arg>
    void RegisterFilter(Fun fun, Arg arg) {
      static_cast<bool (*)(Data&, Arg)>(fun);

      Callback<Data, Arg>* cb = static_cast<Callback<Data, Arg>*>(
          om_malloc(sizeof(Callback<Data, Arg>)));

      new (cb) Callback<Data, Arg>(fun, arg);

      om_config_topic(this->om_topic_, "F", Callback<Data, Arg>::Call, cb);
    }

    template <typename Fun, typename Arg>
    void RegisterCallback(Fun fun, Arg arg) {
      static_cast<bool (*)(Data&, Arg)>(fun);

      Callback<Data, Arg>* cb = static_cast<Callback<Data, Arg>*>(
          om_malloc(sizeof(Callback<Data, Arg>)));

      *cb = Callback<Data, Arg>(fun, arg);

      om_config_topic(this->om_topic_, "D", Callback<Data, Arg>::Call, cb);
    }

    void RangeDivide(Topic& topic, uint32_t length, uint32_t offset,
                     uint32_t scope, uint32_t start, uint32_t num) {
      om_config_filter(this->om_topic_, "R", topic, length, offset, scope,
                       start, num);
    }

    void ListDivide(Topic& topic, uint32_t length, uint32_t offset,
                    uint32_t scope, void* temp) {
      om_config_filter(this->om_topic_, "L", topic, length, offset, scope,
                       temp);
    }

    void DecomposeDivide(Topic& topic, uint32_t length, uint32_t offset,
                         uint32_t scope) {
      om_config_filter(this->om_topic_, topic, "D", this->om_topic_, length,
                       offset, scope);
    }

    bool Publish(Data& data) {
      return om_publish(this->om_topic_, &data, sizeof(Data), true, false) ==
             OM_OK;
    };

    bool PublishFromISR(Data& data) {
      return om_publish(this->om_topic_, &data, sizeof(Data), true, true) ==
             OM_OK;
    };

    operator om_topic_t*() { return this->om_topic_; };

    om_topic_t* om_topic_;
  };

  template <typename Data>
  class Subscriber {
   public:
    Subscriber(const char* name, Data& data) {
      this->om_suber_ =
          om_subscript(om_find_topic(name, UINT32_MAX), OM_PRASE_VAR(data));
    }

    Subscriber(om_topic_t* topic, Data& data) {
      this->om_suber_ = om_subscript(topic, OM_PRASE_VAR(data));
    }

    Subscriber(Topic<Data>& topic, Data& data) {
      this->om_suber_ = om_subscript(topic, OM_PRASE_VAR(data));
    }

    bool Available() { return om_suber_available(this->om_suber_); }

    bool DumpData() { return om_suber_export(this->om_suber_, false) == OM_OK; }

    bool DumpDataFromISR() {
      return om_suber_export(this->handle_, true) == OM_OK;
    }

    om_suber_t* om_suber_;
  };

  class Event {
   public:
    typedef enum {
      EventStart = OM_EVENT_START,
      EventProgress = OM_EVENT_PROGRESS,
      EventEnd = OM_EVENT_END
    } Status;

    Event(const char* name) { this->group_ = om_event_create_group(name); }

    Event(om_event_group_t* group) : group_(group) {}

    static om_event_group_t* FindEvent(const char* name) {
      return om_event_find_group(name, UINT32_MAX);
    }

    bool Register(uint32_t event, Status status,
                  void (*callback)(uint32_t event, void* arg), void* arg) {
      return om_event_register(this->group_, event, (om_event_status_t)status,
                               callback, arg);
    }

    bool Active(uint32_t event) {
      return om_event_active(this->group_, event, true, false);
    }

    bool ActiveFromISR(uint32_t event) {
      return om_event_active(this->group_, event, true, true);
    }

    om_event_group_t* group_;
  };
};

#endif
