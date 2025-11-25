#pragma once

#include <functional>
#include <iostream>

// -------------------------------
// Event Types
// -------------------------------
enum class EventType
{
    None = 0,
    WindowMoved,
    WindowResized,
    MouseMoved,
    KeyPressed,
    KeyReleased,
    MouseButtonPressed,
    MouseButtonReleased,
    COUNT
};

enum EventCategory
{
    None = 0,
    Input = 1 << 0,
    Mouse = 1 << 1,
    Keyboard = 1 << 2,
    Windowing = 1 << 3,
    Gameplay = 1 << 4,
    UI = 1 << 5
};

class Event
{
public:
    virtual EventType type() const = 0;
    virtual ~Event() = default;
    bool handled = false;
};

////////////////////////////////////////////////////

struct ListenerEntry
{
    std::function<void(Event &)> callback;
    void *owner = nullptr;
    bool active = true;
};

/*
//Store memory so we know what to delete ?
//I am not actually going to delete willy-nilly in a vector
//RIngBUffer ?
struct SubscriptionHandle
{
    size_t listenerIndex;
    bool valid = true;
};
*/
class EventDispatcher
{
public:
    using Callback = std::function<void(Event &)>;

    struct ListenerEntry
    {
        Callback callback;
        void *owner = nullptr;
        bool active = true;
    };
    EventDispatcher() { listeners.fill({}); }

    template <typename T, typename Owner>
    bool subscribe(Owner *owner, Callback func)
    {
        size_t typeIndex = (size_t)T::getStaticType();

        ListenerEntry entry;
        entry.owner = owner;
        entry.callback = [func](Event &e)
        { func(static_cast<T &>(e)); };

        listeners[typeIndex].push_back(entry);
        size_t idx = listeners[typeIndex].size() - 1;
        // return SubscriptionHandle{idx, true};
        return true;
    }

    void removeOwner(void *owner)
    {
        for (auto &list : listeners)
            for (auto &entry : list)
                if (entry.owner == owner){
                    entry.active = false;}
    }

    //Todo: Some tweak, some might not want to receive event for x object
    //Or would not treat if 
    void dispatch(Event &e)
    {
        auto &list = listeners[(size_t)e.type()];
        for (auto &entry : list)
        {
            if (entry.active)
                entry.callback(e);
            // if (e.handled)
            //   break;
        }
    }

private:
    std::array<std::vector<ListenerEntry>, (size_t)EventType::COUNT> listeners;
};


/*

class Player : public EventListener {
public:
    Player(EventDispatcherPersistent& d) : EventListener(d) {
        dispatcher.subscribe<KeyPressedEvent>(this, [this](auto& e){
            if(e.key == 'W') moveForward();
        });
    }

    void moveForward() { std::cout << "Player walks forward\n"; }
};


This was the "idea" but the dispatcher below also work ?
Todo: Compare properly the three ideas of Dispatcher, + Potential cohabitation
+ Capacity to handle multithreading

*/
struct EventDispatcherTemp
{
    Event &mEvent;
    EventDispatcherTemp(Event &e) : mEvent(e) {}
    using Callback = std::function<void(Event &)>;

    template <typename EventType, typename F>
    bool dispatch(const F &func)
    {
        if (mEvent.type() == EventType::getStaticType())
        {
            func(static_cast<EventType &>(mEvent));
            return true;
        }
        return false;
    }

    template <typename EventType, typename F>
    bool dispatch(Event &event, const F &func)
    {
        if (event.type() == EventType::getStaticType())
        {
             func(static_cast<EventType&>(event)); // Handle the event
            return true;
        }
        return false;
    }
};
/*

//Todo:
//From Game Engine Architecture
//We couldn simply have an union keeping a certain amount of data
//Which would remove the need of polymorphism
//Event would simply have an type variable
//Less typesafe i guesx


// Multiple types of struct making 128 bytes as a union .

// IN C i saw example where data were in some union
// Also add a mechanism that allow Tool to register interest into some event
// Event passing hierarchy ?
// Event QUeieng or not ?



enum class EventType { KeyPressed, MousePressed, WindowResized, COUNT };

// Encapsulating the event as a command with arguments
struct Event
{
    EventType type;
    std::vector<int> args; // Arguments for the event 
    //Or a variant or an union or std::any
};

*/