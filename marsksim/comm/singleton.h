// Tencent is pleased to support the open source community by making Mars available.
// Copyright (C) 2016 THL A29 Limited, a Tencent company. All rights reserved.

// Licensed under the MIT License (the "License"); you may not use this file except in 
// compliance with the License. You may obtain a copy of the License at
// http://opensource.org/licenses/MIT

// Unless required by applicable law or agreed to in writing, software distributed under the License is
// distributed on an "AS IS" basis, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND,
// either express or implied. See the License for the specific language governing permissions and
// limitations under the License.



#ifndef COMM_SINGLETON_H_
#define COMM_SINGLETON_H_

#include <list>

#include "boost/shared_ptr.hpp"
#include "boost/weak_ptr.hpp"
#include "boost/signals2.hpp"

#include "marsksim/comm/thread/lock.h"

#define SINGLETON_STRONG_ksim(class_name) design_patterns_ksim::Singletonksim::Instance<class_name>\
    (design_patterns_ksim::SingletonksimHelper::CreateInstanceHelper<class_name>(), design_patterns_ksim::SingletonksimHelper::DestoryInstanceHelper<class_name>())
#define SINGLETON_WEAK_ksim(class_name) design_patterns_ksim::Singletonksim::Instance_Weak<class_name>()

#define SINGLETON_RELEASE_ksim(class_name) design_patterns_ksim::Singletonksim::Release<class_name>()
#define SINGLETON_RELEASE_ksim_ALL() design_patterns_ksim::Singletonksim::ReleaseAll()

#define SINGLETON_INTRUSIVE_ksim(classname, creater, destoryer) \
    class Singletonksim\
    {\
      public:\
        static boost_ksim::shared_ptr<classname>& instance_shared_ptr() { static boost_ksim::shared_ptr<classname> s_ptr;return s_ptr;}\
        static Mutex& singleton_mutex() {static Mutex s_mutex; return s_mutex;}\
        static boost_ksim::signals2::signal<void ()>& SignalInstanceBegin() { static boost_ksim::signals2::signal<void ()> s_signal; return s_signal;} \
        static boost_ksim::signals2::signal<void (boost_ksim::shared_ptr<classname>)>& SignalInstance() { static boost_ksim::signals2::signal<void (boost_ksim::shared_ptr<classname>)> s_signal; return s_signal;} \
        static boost_ksim::signals2::signal<void (boost_ksim::shared_ptr<classname>)>& SignalRelease() { static boost_ksim::signals2::signal<void (boost_ksim::shared_ptr<classname>)> s_signal; return s_signal;} \
        static boost_ksim::signals2::signal<void ()>& SignalReleaseEnd() { static boost_ksim::signals2::signal<void ()> s_signal; return s_signal;} \
        static boost_ksim::signals2::signal<void (classname&)>& SignalResetOld() { static boost_ksim::signals2::signal<void (classname&)> s_signal; return s_signal;} \
        static boost_ksim::signals2::signal<void (classname&)>& SignalResetNew() { static boost_ksim::signals2::signal<void (classname&)> s_signal; return s_signal;} \
     \
      public:\
        static boost_ksim::shared_ptr<classname> Instance()\
        {\
            boost_ksim::shared_ptr<classname> ret = instance_shared_ptr();\
            if (ret) return ret;\
            \
            ScopedLock    lock(singleton_mutex());\
            if (!instance_shared_ptr())\
            {\
                SignalInstanceBegin()();\
                boost_ksim::shared_ptr<classname> temp(const_cast<classname*>(creater), Singletonksim::Delete);\
                SignalInstance()(temp);\
                instance_shared_ptr().swap(temp); \
            }\
            return ( instance_shared_ptr());\
        }\
        \
        template<class T>\
        static boost_ksim::shared_ptr<classname> Instance(const T& _creater)\
        {\
            boost_ksim::shared_ptr<classname> ret = instance_shared_ptr();\
            if (ret) return ret;\
            \
            ScopedLock    lock(singleton_mutex());\
            if (!instance_shared_ptr())\
            {\
                SignalInstanceBegin()();\
                boost_ksim::shared_ptr<classname> temp(const_cast<classname*>(_creater()), Singletonksim::Delete);\
                SignalInstance()(temp);\
                instance_shared_ptr().swap(temp); \
            }\
            return ( instance_shared_ptr());\
        }\
        \
        static boost_ksim::weak_ptr<classname> Instance_Weak() { return instance_shared_ptr();} \
        \
        static void Release()\
        {\
            ScopedLock    lock(singleton_mutex());\
            if (instance_shared_ptr())\
            {\
                SignalRelease()(instance_shared_ptr());\
                instance_shared_ptr().reset();\
                SignalReleaseEnd()();\
            }\
        }\
        \
        static boost_ksim::shared_ptr<classname>  Reset()\
        {\
            ScopedLock    lock(singleton_mutex());\
            if (instance_shared_ptr())\
            {\
                SignalResetOld()(*const_cast<classname*>(instance_shared_ptr().get()));\
                instance_shared_ptr().reset();\
            }\
            \
            if (!instance_shared_ptr())\
            {\
                instance_shared_ptr().reset(const_cast<classname*>(creater), Singletonksim::Delete); \
                SignalResetNew()(*const_cast<classname*>(instance_shared_ptr().get()));\
            }\
            return ( instance_shared_ptr());\
        }\
        \
      private: \
        static void Delete(classname* _ptr) { destoryer(_ptr); }\
    }


namespace design_patterns_ksim {

namespace SingletonksimHelper {
template<typename T>
class CreateInstanceHelper {
  public:
    T* operator()() {
        return new T();
    }
};

template<typename T>
class DestoryInstanceHelper {
  public:
    void operator()(T* _instance) {
        delete _instance;
    }
};
}

class Singletonksim {
  protected:
    class SingletonksimInfo {
      public:
        virtual ~SingletonksimInfo() {}
        virtual void DoRelease() = 0;
        virtual void* GetInstance() const = 0;
    };

  private:
    template<typename T>
    class SingletonksimInstance {
      public:
        //static boost_ksim::shared_ptr<T> instance_shared_ptr;
        
        static boost_ksim::shared_ptr<T>& instance_shared_ptr() {
            static boost_ksim::shared_ptr<T> ptr;
            return ptr;
        }
        static Mutex& singleton_mutex() {
            static Mutex s_mutex;
            return s_mutex;
        }
    };

    template<typename T>
    class SingletonksimInfoImpl : public SingletonksimInfo {
      public:
        SingletonksimInfoImpl() {}
        virtual void DoRelease() {
            ScopedLock    lock(SingletonksimInstance<T>::singleton_mutex());

            if (SingletonksimInstance<T>::instance_shared_ptr()) {
                SingletonksimInstance<T>::instance_shared_ptr().reset();
            }
        }

        virtual void* GetInstance() const {
            return const_cast<T*>(SingletonksimInstance<T>::instance_shared_ptr().get());
        }
    };

  private:
    Singletonksim();
    ~Singletonksim();
    Singletonksim(const Singletonksim&);
    Singletonksim& operator=(const Singletonksim&);

  public:
    static void ReleaseAll();

    template<typename T> static
    void Release() { _ReleaseSigleton(const_cast<T*>(SingletonksimInstance<T>::instance_shared_ptr().get())); }

    template<typename T, typename CREATER, typename DESTORYER> static
    boost_ksim::shared_ptr<T> Instance(CREATER _creater, DESTORYER _destoryer) {
        boost_ksim::shared_ptr<T> ret = SingletonksimInstance<T>::instance_shared_ptr();

        if (ret) return ret;

        ScopedLock    lock(SingletonksimInstance<T>::singleton_mutex());

        if (!SingletonksimInstance<T>::instance_shared_ptr()) {
            _AddSigleton(new SingletonksimInfoImpl<T>());
            SingletonksimInstance<T>::instance_shared_ptr().reset(const_cast<T*>(_creater()), _destoryer);
        }

        return (SingletonksimInstance<T>::instance_shared_ptr());
    }

    template<typename T> static
    boost_ksim::weak_ptr<T> Instance_Weak() { return SingletonksimInstance<T>::instance_shared_ptr(); }


  protected:
    static void _AddSigleton(SingletonksimInfo* _helper);
    static void _ReleaseSigleton(void* _instance);

  private:
    static std::list<SingletonksimInfo*> lst_singleton_releasehelper_;
};

//template<typename T>
//boost_ksim::shared_ptr<T> Singletonksim::SingletonksimInstance<T>::instance_shared_ptr;

//template<typename T>
//Mutex Singletonksim::SingletonksimInstance<T>::singleton_mutex();
}



#endif	// COMM_SINGLETON_H_
