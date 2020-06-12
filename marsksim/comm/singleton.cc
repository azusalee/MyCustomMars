// Tencent is pleased to support the open source community by making Mars available.
// Copyright (C) 2016 THL A29 Limited, a Tencent company. All rights reserved.

// Licensed under the MIT License (the "License"); you may not use this file except in 
// compliance with the License. You may obtain a copy of the License at
// http://opensource.org/licenses/MIT

// Unless required by applicable law or agreed to in writing, software distributed under the License is
// distributed on an "AS IS" basis, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND,
// either express or implied. See the License for the specific language governing permissions and
// limitations under the License.



#include "comm/singleton.h"

namespace design_patterns_ksim {

static Mutex sg_singleton_mutex;
std::list<Singletonksim::SingletonksimInfo*> Singletonksim::lst_singleton_releasehelper_;

void Singletonksim::ReleaseAll() {
    ScopedLock    lock(sg_singleton_mutex);
    std::list<Singletonksim::SingletonksimInfo*> lst_copy = lst_singleton_releasehelper_;
    lst_singleton_releasehelper_.clear();
    lock.unlock();

    for (std::list<Singletonksim::SingletonksimInfo*>::reverse_iterator it = lst_copy.rbegin();
            it != lst_copy.rend(); ++it) {
        (*it)->DoRelease();
        delete(*it);
    }
}

void Singletonksim::_AddSigleton(Singletonksim::SingletonksimInfo* _helper) {
    ScopedLock    lock(sg_singleton_mutex);
    std::list<Singletonksim::SingletonksimInfo*>& lst = lst_singleton_releasehelper_;
    lst.push_back(_helper);
}

void Singletonksim::_ReleaseSigleton(void* _instance) {
    if (0 == _instance) return;

    ScopedLock    lock(sg_singleton_mutex);
    Singletonksim::SingletonksimInfo* releasesigleton = NULL;

    std::list<Singletonksim::SingletonksimInfo*>& lst = lst_singleton_releasehelper_;
    std::list<Singletonksim::SingletonksimInfo*>::iterator it = lst.begin();

    for (; it != lst.end(); ++it) {
        if ((*it)->GetInstance() == _instance) {
            releasesigleton = *it;
            lst.erase(it);
            break;
        }
    }

    lock.unlock();

    if (releasesigleton) {
        releasesigleton->DoRelease();
        delete releasesigleton;
    }
}

}  // namespace design_patterns_ksim
