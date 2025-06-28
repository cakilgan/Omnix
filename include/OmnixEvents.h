#ifndef OMNIX_EVENTS_H
#define OMNIX_EVENTS_H

#include <functional>
#include <typeindex>
#include <unordered_map>
struct OmnixEvent{
    virtual ~OmnixEvent() = default;
};
class OmnixEventBus {
public:
    template<typename EventType>
    using Listener = std::function<void(EventType*)>;

    template<typename EventType>
    void subscribe(const Listener<EventType>& listener) {
        
        auto& listeners = listenersMap[typeid(EventType)];
        listeners.push_back([listener](OmnixEvent* event) {
            listener(static_cast<EventType*>(event));
        });
    }

    template<typename EventType>
    void publish(EventType* event) const {
        auto it = listenersMap.find(typeid(EventType));
        if (it != listenersMap.end()) {
            for (const auto& listener : it->second) {
                listener(event);
            }
        }
    }

private:
    std::unordered_map<std::type_index, std::vector<std::function<void(OmnixEvent*)>>> listenersMap;
};
#endif // OMNIX_EVENTS_H