#include "Dispatcher.h"

Dispatcher::Dispatcher(EventLoop *evLoop) : m_evLoop(evLoop)
{
}

int Dispatcher::add()
{
    return 0;
}

int Dispatcher::remove()
{
    return 0;
}

int Dispatcher::modify()
{
    return 0;
}

int Dispatcher::dispatch(int timeout)
{
    return 0;
}

Dispatcher::~Dispatcher()
{
}
