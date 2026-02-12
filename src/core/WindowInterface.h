#pragma once

#include <iostream>
#include <functional>
#include <cstdint> 
#include "Events.h"

class AbstractWindow
{
public:

    using EvtCallback = EventDispatcher::Callback;

    virtual ~AbstractWindow() {}

    virtual bool init(const std::string &title, uint32_t width, uint32_t height) = 0;

    virtual void onUpdate() = 0;

    virtual bool isOpen() const = 0;
    virtual void close() = 0;

    //virtual void setResizeCallback(std::function<void(int, int)> callback) = 0;
    virtual void setEventCallback(const EvtCallback& callBack) = 0;

    virtual uint32_t getWidth() const = 0;
    virtual uint32_t getHeight() const = 0;

    virtual void *getNativeWindowHandle() const = 0;
};