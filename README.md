# OneMessageCPP

本仓库为[OneMessage](https://github.com/Jiu-xiao/OneMessage.git)的C++封装，多数API在初始化时会动态申请内存。使用时请对照[OneMessage](https://github.com/Jiu-xiao/OneMessage.git)文档。

---

## USAGE

请参照[OneMessage](https://github.com/Jiu-xiao/OneMessage.git)中的 [Readme.md](https://github.com/Jiu-xiao/OneMessage/blob/master/README.md) 编写om_config.h文件。

## om_config.h

om_config.h中需要另外声明一个函数，如果在中断中调用此函数，应当返回true，否则返回false。在linux等平台可以直接返回false。

```c++
#include <stdbool.h>
static inline bool om_in_isr() {
    ...
}
```

## CMake

作为CMake子模块编译

CMakeList.txt

```CMake
# OneMessageCPP_directory 为OneMessageCPP所在目录
# om_config_file_directory 为om_config.h所在目录

add_subdirectory(OneMessageCPP_directory)

target_include_directories(
    OneMessage
    PUBLIC om_config_file_directory
    ...
)
```

## API

> `template <typename Data> class Topic`

构造函数：

Topic可以直接隐式类型转换为om_topic_t*

新创建一个Topic，cached为false代表不拷贝数据，只传递指针

```c++
Topic(const char *name, bool cached = false);
```

拷贝一个已有的topic，不允许重复创建同名topic。

```c++
Topic(om_topic_t *topic);
```

队列：

```c++
template <uint32_t Len>
class Queue{
    Queue(topic_t* topic);

     uint32_t Size();

      bool Read(Data& buff);

      bool Reads(Data* buff, uint32_t len);

      bool Pop();

      void Reset();
}
```

链接：

```c++
bool Link(Topic &source);

bool Link(om_topic_t *source);

bool Link(const char *source_name);
```

查找topic：

```c++
om_topic_t *Find(const char *name);
```

加锁：

```c++
bool Lock();

void Unlock();
```

高级过滤器：

```c++
void RangeDivide(Topic &topic, uint32_t length, uint32_t offset, uint32_t scope, uint32_t start, uint32_t num);

void ListDivide(Topic &topic, uint32_t length, uint32_t offset, uint32_t scope, void *temp);

void DecomposeDivide(Topic &topic, uint32_t length, uint32_t offset, uint32_t scope);
```

注册回调：

```c++
//Fun的类型应当为bool (*)(Data &, Arg)

template <typename Fun, typename Arg>
void RegisterFilter(Fun fun, Arg arg);


template <typename Fun, typename Arg>
void RegisterCallback(Fun fun, Arg arg);
```

发布：

```c++
bool Publish(Data &data, bool in_isr = om_in_isr());
```

打包数据：

```c++
    bool PackData(RemoteData &data);
```

> `template <typename Data> class Subscriber`

构造：

构造时需要传入订阅的话题名,如果目标话题未创建，会等待timeout时间后返回。

```c++
Subscriber(const char *name uint32_t timeout = UINT32_MAX);
```

如果能够获取到对应话题，可以使用以下构造函数：

```c++
    Subscriber(om_topic_t *topic);

    Subscriber(Topic<Data> &topic);
```

是否创建成功：

```c++
    bool Ready();
```

是否有新数据：

```c++
    bool Available();
```

导出数据到绑定的变量：

```c++
    bool DumpData(Data& buff, bool in_isr = om_in_isr());
```

> `class Remote`

能够解析打包的话题数据。

构造函数：

```c++
    Remote(uint32_t buff_size, uint32_t topic_num);
```

添加可解析的话题：

```c++
    void AddTopic(const char *topic_name);

    void AddTopic(om_topic_t *topic);
```

解析数据：

```c++
    bool PraseData(uint8_t *data, uint32_t size,, bool in_isr = om_in_isr());
```

> `class Event`

构造函数：

```c++
    Event(const char *name);

    Event(om_event_group_t *group);
```

查找事件组：

```c++
    om_event_group_t *FindEvent(const char *name);
```

注册事件：

```c++
    bool Register(uint32_t event, Status status, void (*callback)(uint32_t event, void *arg), void *arg);
```

激活事件：

```c++
    bool Active(uint32_t event, bool in_isr = om_in_isr());
```
