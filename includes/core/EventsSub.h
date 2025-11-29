#pragma once

#include "Events.h"


//Add parent static type ?
//Todo: Add mStuff
class KeyEvent : public Event
{
protected:
  KeyEvent(int key) : key(key), repeat(false) {};
  KeyEvent(int key, bool rep) : key(key), repeat(rep) {};
public:
   
    static EventType getStaticType() { return EventType::KeyPressed; }

    EventType type() const override { return getStaticType(); }

    int key;  
    bool repeat = false;
};


class KeyPressedEvent : public KeyEvent
{
public:
    KeyPressedEvent(int key) : KeyEvent(key) {}
    KeyPressedEvent(int key, bool rep) : KeyEvent(key, rep) {};

    static EventType getStaticType() { return EventType::KeyPressed; }

    EventType type() const override { return getStaticType(); }

};

class KeyReleasedEvent : public KeyEvent
{
public:
    KeyReleasedEvent(int key) : KeyEvent(key) {}
    //KeyReleasedEvent(int key, bool rep) : KeyEvent(key, rep) {};

    static EventType getStaticType() { return EventType::KeyReleased; }

    EventType type() const override { return getStaticType(); }

};

class MouseButton : public Event
{
public:
    MouseButton(int button) : button(button) {}
    MouseButton(int button, bool rep) : button(button), repeat(rep) {};

    static EventType getStaticType() { return EventType::MouseButtonPressed; }
    EventType type() const override { return getStaticType(); }

    int button;  
    bool repeat = false;

};



class MouseButtonPressedEvent : public MouseButton
{
public:
    MouseButtonPressedEvent(int button) : MouseButton(button) {}
    MouseButtonPressedEvent(int button, bool rep) : MouseButton(button, rep) {};

    static EventType getStaticType() { return EventType::MouseButtonPressed; }
    EventType type() const override { return getStaticType(); }
};

class MouseButtonReleasedEvent : public MouseButton
{
public:
    MouseButtonReleasedEvent(int button) : MouseButton(button) {}

    static EventType getStaticType() { return EventType::MouseButtonReleased; }
    EventType type() const override { return getStaticType(); }
};
