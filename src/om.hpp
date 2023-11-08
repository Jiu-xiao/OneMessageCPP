#ifndef __OM_H_CPP__
#define __OM_H_CPP__

#include "om.h"
#include "om_core.h"

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
    Topic(const char* name, bool cached = false) {
      if (!cached) {
        this->om_topic_ = om_config_topic(NULL, "A", name, sizeof(Data));
      } else {
        this->om_topic_ = om_config_topic(NULL, "CA", name, sizeof(Data));
      }
    }

    Topic(om_topic_t* topic) { this->om_topic_ = topic; }

    bool Lock() { return OM_TOPIC_LOCK(om_topic_); }

    void Unlock() { OM_TOPIC_UNLOCK(om_topic_); }

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
      (void)static_cast<bool (*)(Data&, Arg)>(fun);

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

    bool Publish(Data& data, bool in_isr = om_in_isr()) {
      return om_publish(this->om_topic_, &data, sizeof(Data), true, in_isr) ==
             OM_OK;
    };

    typedef OM_COM_TYPE(Data) RemoteData;

    bool PackData(RemoteData& data) {
      return om_com_generate_pack(om_topic_, &data) == OM_OK;
    };

    template <uint32_t Len>
    class Queue {
      Queue(om_topic_t* topic) : topic_(topic) {
        OM_ASSERT(topic);
        om_queue_init_fifo_static(topic, &fifo_, buff_, Len);
        om_queue_add_static(topic, &sub_, &fifo_);
      }

      uint32_t Size() { return om_fifo_readable_item_count(&fifo_); }

      bool Read(Data& buff) {
        OM_TOPIC_LOCK(topic_);
        auto ans = om_fifo_read(&fifo_, &buff);
        OM_TOPIC_UNLOCK(topic_);
        return ans == OM_OK;
      }

      bool Reads(Data* buff, uint32_t len) {
        OM_TOPIC_LOCK(topic_);
        auto ans = om_fifo_reads(&fifo_, buff, len);
        OM_TOPIC_UNLOCK(topic_);
        return ans == OM_OK;
      }

      bool Pop() {
        OM_TOPIC_LOCK(topic_);
        auto ans = om_fifo_pop(&fifo_);
        OM_TOPIC_UNLOCK(topic_);
        return ans == OM_OK;
      }

      void Reset() {
        OM_TOPIC_LOCK(topic_);
        om_fifo_reset(&fifo_);
        OM_TOPIC_UNLOCK(topic_);
      }

      om_fifo_t fifo_;
      om_suber_t sub_;
      uint8_t buff_[sizeof(Data) * Len];
      om_topic_t* topic_;
    };

    operator om_topic_t*() { return this->om_topic_; };

    om_topic_t* om_topic_;
  };

  template <typename Data>
  class Subscriber {
   public:
    Subscriber(const char* name, uint32_t timeout = UINT32_MAX) {
      om_topic_t* topic = om_find_topic(name, timeout);
      if (topic != NULL) {
        this->om_suber_ = om_subscribe(topic);
      } else {
        this->om_suber_ = NULL;
      }
    }

    Subscriber(om_topic_t* topic) { this->om_suber_ = om_subscribe(topic); }

    Subscriber(Topic<Data>& topic) { this->om_suber_ = om_subscribe(topic); }

    bool Ready() { return this->om_suber_ != NULL; }

    bool Available() {
      if (this->Ready()) {
        return om_suber_available(this->om_suber_);
      } else {
        return false;
      }
    }

    bool DumpData(Data& buff, bool in_isr = om_in_isr()) {
      if (this->Ready()) {
        return om_suber_export(this->om_suber_, &buff, in_isr) == OM_OK;
      } else {
        return false;
      }
    }

    om_suber_t* om_suber_;
  };

  class Remote {
   public:
    om_com_t com_;

    Remote(uint32_t buff_size, uint32_t topic_num) {
      om_com_create(&com_, buff_size, topic_num, buff_size);
    }

    void AddTopic(const char* topic_name) {
      om_com_add_topic_with_name(&com_, topic_name);
    }

    void AddTopic(om_topic_t* topic) { om_com_add_topic(&com_, topic); }

    bool PraseData(uint8_t* data, uint32_t size, bool in_isr = om_in_isr()) {
      return om_com_prase_recv(&com_, data, size, true, in_isr) ==
             OM_COM_RECV_SUCESS;
    }
  };

  class Event {
   public:
    typedef enum {
      EVENT_START = OM_EVENT_START,
      EVENT_PROGRESS = OM_EVENT_PROGRESS,
      EVENT_END = OM_EVENT_END
    } Status;

    Event(const char* name) : group_(om_event_create_group(name)) {}

    Event(om_event_group_t* group) : group_(group) {}

    static om_event_group_t* FindEvent(const char* name) {
      return om_event_find_group(name, UINT32_MAX);
    }

    bool Register(uint32_t event, Status status,
                  void (*callback)(uint32_t event, void* arg), void* arg) {
      return om_event_register(this->group_, event,
                               static_cast<om_event_status_t>(status), callback,
                               arg);
    }

    bool Active(uint32_t event, bool in_isr = om_in_isr()) {
      return om_event_active(this->group_, event, true, in_isr);
    }

    om_event_group_t* group_;
  };
};

#endif
