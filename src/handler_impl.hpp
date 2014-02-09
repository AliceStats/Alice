/**
 * @file handler_impl.hpp
 * @author Robin Dietrich <me (at) invokr (dot) org>
 * @version 1.0
 *
 * @par License
 *    Alice Replay Parser
 *    Copyright 2014 Robin Dietrich
 *
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 *
 */

template <typename Obj, typename Id, typename Data, uint32_t IdSelf>
bool handlersub<Obj, Id, Data, IdSelf>::
hasCallback(const id_t& i, std::true_type) {
    bool has = (cbpref.find(i) != cbpref.end()) || (cb.find(i) != cb.end());
    if (!has && std::is_same<std::string, Id>::value) {
        for (auto &d : cbpref) {
            if ((d.first.size() > i.size()) && (i.substr(0, d.first.size()).compare(d.first) != 0))
                continue;

            return true;
        }
    }

    return has;
}

template <typename Obj, typename Id, typename Data, uint32_t IdSelf>
bool handlersub<Obj, Id, Data, IdSelf>::
hasCallback(const id_t& i, std::false_type) {
    return cb.find(i) != cb.end();
}

template <typename Obj, typename Id, typename Data, uint32_t IdSelf>
void handlersub<Obj, Id, Data, IdSelf>::
forward(const id_t& i, Data &&data, uint32_t tick, std::false_type) {
    auto cbDef = cb.find(i); // check for 1 = 1 callback type

    if ((cbDef == cb.end()) && cbpref.empty()) {
        // there is neither a direct callback nor a potential prefix callback
        return;
    }

    auto objConv = obj.find(i); // get conversion object
    if (objConv == obj.end()) {
        BOOST_THROW_EXCEPTION(handlerNoConversionAvailable()
            << (typename EArgT<1, id_t>::info(i))
        );
    }

    // create object
    cbObject<Obj, Id> o(objConv->second(std::move(data)), tick, i);

     // forward to definitive handlers
    if ((cbDef != cb.end()))
        for (auto &d : cbDef->second) {
            d(&o);
        }

    // forward to prefix handlers
    if (!cbpref.empty())
        forwardPrefix(&o);
}

template <typename Obj, typename Id, typename Data, uint32_t IdSelf>
void handlersub<Obj, Id, Data, IdSelf>::
forward(const id_t& i, Data &&data, uint32_t tick, std::true_type) {
    auto cbDef = cb.find(i); // check for 1 = 1 callback type

    if ((cbDef == cb.end()) && cbpref.empty()) {
        // there is neither a direct callback nor a potential prefix callback
        return;
    }

    // create object, moving here makes shared_ptrs impossible TODO fix
    cbObject<Obj, Id> o(std::move(data), tick, i, false);

    // forward to definitive handlers
    if ((cbDef != cb.end()))
        for (auto &d : cbDef->second) {
            d(&o);
        }

    // forward to prefix handlers
    if (!cbpref.empty())
        forwardPrefix(&o);
}

template <typename Obj, typename Id, typename Data, uint32_t IdSelf>
void handlersub<Obj, Id, Data, IdSelf>::
forwardPrefix(callbackObj_t o, std::true_type) {
    const std::string& comp = o->id;

    for (auto &d : cbpref) {
        if ((d.first.size() > comp.size()) && (comp.substr(0, d.first.size()).compare(d.first) != 0))
            continue;

        for (auto &d2 : d.second)
            d2(o);
    }
}

template <typename Obj, typename Id, typename Data, uint32_t IdSelf>
void handlersub<Obj, Id, Data, IdSelf>::
forwardPrefix(callbackObj_t o, std::false_type) {
    BOOST_THROW_EXCEPTION(handlerPrefixError() );
}

template <typename T1, typename... Rest>
template <uint32_t Type, typename Id>
bool handler<T1, Rest...>::hasCallback(const Id& i, std::true_type) {
    if (T1::id == Type) {
        return subhandler.hasCallback(i);
    } else {
        return child.hasCallback<Type, Id>(i);
    }
}

template <typename T1, typename... Rest>
template <uint32_t Type, typename Id>
bool handler<T1, Rest...>::hasCallback(const Id& i, std::false_type) {
    return child.hasCallback<Type, Id>(i);
}

template <typename T1, typename... Rest>
template<uint32_t Type, typename Id, typename Delegate>
void handler<T1, Rest...>::registerCallback(const Id& i, Delegate&& d, bool prefix, std::true_type) {
    if (T1::id == Type) {
        subhandler.registerCallback(i, std::move(d), prefix);
    } else {
        child.registerCallback<Type>(i, std::move(d), prefix);
    }
}

template <typename T1, typename... Rest>
template<uint32_t Type, typename Id, typename Delegate>
void handler<T1, Rest...>::registerCallback(const Id& i, Delegate&& d, bool prefix, std::false_type) {
    child.registerCallback<Type>(i, std::move(d), prefix);
}

template <typename T1, typename... Rest>
template<uint32_t Type, typename Id, typename Delegate>
void handler<T1, Rest...>::removeCallback(const Id& i, Delegate&& d, bool prefix, std::true_type) {
    if (T1::id == Type) {
        subhandler.removeCallback(i, std::move(d), prefix);
    } else {
        child.removeCallback<Type>(i, std::move(d), prefix);
    }
}

template <typename T1, typename... Rest>
template<uint32_t Type, typename Id, typename Delegate>
void handler<T1, Rest...>::removeCallback(const Id& i, Delegate&& d, bool prefix, std::false_type) {
    child.removeCallback<Type>(i, std::move(d), prefix);
}

template <typename T1, typename... Rest>
template<uint32_t Type, typename T, typename Id>
void handler<T1, Rest...>::registerObject(const Id& i, std::true_type) {
    if (T1::id == Type) {
        subhandler.template registerObject<T>(i);
    } else {
        child.registerObject<Type, T>(i);
    }
}

template <typename T1, typename... Rest>
template<uint32_t Type, typename T, typename Id>
void handler<T1, Rest...>::registerObject(const Id& i, std::false_type) {
    child.registerObject<Type, T>(i);
}

template <typename T1, typename... Rest>
template <uint32_t Type, typename Id, typename Data>
void handler<T1, Rest...>::forward(Id i, Data data, uint32_t tick, std::true_type) {
    if (T1::id == Type) {
        subhandler.forward(std::move(i), std::move(data), std::move(tick));
    } else {
        child.forward<Type>(std::move(i), std::move(data), std::move(tick));
    }
}

template <typename T1, typename... Rest>
template <uint32_t Type, typename Id, typename Data>
void handler<T1, Rest...>::forward(Id i, Data data, uint32_t tick, std::false_type) {
	child.forward<Type>(std::move(i), std::move(data), std::move(tick));
}