/////////////////////////////////////////////////////////////////////////////////////
//
//   LucED - The Lucid Editor
//
//   Copyright (C) 2005-2010 Oliver Schmidt, oliver at luced dot de
//
//   This program is free software; you can redistribute it and/or modify it
//   under the terms of the GNU General Public License Version 2 as published
//   by the Free Software Foundation in June 1991.
//
//   This program is distributed in the hope that it will be useful, but WITHOUT
//   ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
//   FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
//   more details.
//
//   You should have received a copy of the GNU General Public License along with 
//   this program; if not, write to the Free Software Foundation, Inc., 
//   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
//
/////////////////////////////////////////////////////////////////////////////////////

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <X11/Xatom.h>

#include "util.hpp"
#include "EventDispatcher.hpp"
#include "GuiRoot.hpp"
#include "System.hpp"
#include "SystemException.hpp"
#include "MicroSeconds.hpp"
#include "MilliSeconds.hpp"

#ifdef DEBUG
    #include "TopWin.hpp"
#endif

using namespace LucED;

SingletonInstance<EventDispatcher> EventDispatcher::instance;

EventDispatcher* EventDispatcher::getInstance()
{
    return instance.getPtr();
}

static bool hasSignalHandlers = false;
static int sigChildPipeIn     = -1;
static int sigChildPipeOut    = -1;
static int taskNotifyPipeIn   = -1;
static int taskNotifyPipeOut  = -1;
static sigset_t enabledSignalBlockMask;
static sigset_t disabledSignalBlockMask;


static void sigchildHandler(int signal)
{
    switch (signal)
    {
        case SIGCHLD: {
            if (hasSignalHandlers) {
                char msg = 'x';
                ::write(sigChildPipeOut, &msg, 1);
            }
            break;
        }
    }
}

static inline void enableSignals()
{
    ASSERT(hasSignalHandlers);
    
    if (sigprocmask(SIG_SETMASK, &enabledSignalBlockMask, NULL) != 0) {
        throw SystemException(String() << "Could not call sigprocmask: " << strerror(errno));
    }
}

static inline void disableSignals()
{
    ASSERT(hasSignalHandlers);

    if (sigprocmask(SIG_SETMASK, &disabledSignalBlockMask, NULL) != 0) {
        throw SystemException(String() << "Could not call sigprocmask: " << strerror(errno));
    }
}

static inline int internalSelect(int n, fd_set* readfds, fd_set* writefds, fd_set* exceptfds, const Nullable<TimePeriod>& timePeriod)
{
#if 0
  TimeVal diffTimeVal;
  if (timeVal != NULL) {
    diffTimeVal = *timeVal;
  }
  TimeVal oldTimeVal = TimeVal::NOW;
#endif

    enableSignals();
    int rslt = System::select(n, readfds, writefds, exceptfds, timePeriod);
    disableSignals();

#if 0
  if (timeVal != NULL)
  {
    TimeVal newTimeVal = TimeVal::NOW;
    TimeVal oldTimeVal2 = oldTimeVal + diffTimeVal;
    if (oldTimeVal2 < newTimeVal)
    {
        MicroSeconds msecs = TimeVal::diffMicroSecs(oldTimeVal2, newTimeVal);
        if (msecs > 50 * 1000) {
            printf("--------------------- %ld --- (%ld:%ld) -> (%ld:%ld)  : (%ld:%ld)\n", (long) msecs, 
                                                                                          (long) oldTimeVal.getSeconds(),
                                                                                          (long) oldTimeVal.getMicroSeconds(),
                                                                                          (long) newTimeVal.getSeconds(),
                                                                                          (long) newTimeVal.getMicroSeconds(),
                                                                                          (long) diffTimeVal.getSeconds(),
                                                                                          (long) diffTimeVal.getMicroSeconds());
        }
    }
  }
#endif
    return rslt;
}


EventDispatcher::EventDispatcher()
    : doQuit(false),
      hasRootPropertyListeners(false),
      lastX11EventTime(CurrentTime),
      mutex(Mutex::create())
{
    
    x11FileDescriptor = ConnectionNumber(GuiRoot::getInstance()->getDisplay());

    System::setCloseOnExecFlag(x11FileDescriptor);

    if (!hasSignalHandlers)
    {
        int sigChildPipe[2];
    
        if (::pipe(sigChildPipe) != 0) {
            throw SystemException(String() << "Could not create pipe: " << strerror(errno));
        }
        
        sigChildPipeIn  = sigChildPipe[0];
        sigChildPipeOut = sigChildPipe[1];
    
        System::setCloseOnExecFlag(sigChildPipeIn);
        System::setCloseOnExecFlag(sigChildPipeOut);
        
        int taskNotifyPipe[2];
        if (::pipe(taskNotifyPipe) != 0) {
            throw SystemException(String() << "Could not create pipe: " << strerror(errno));
        }
        taskNotifyPipeIn  = taskNotifyPipe[0];
        taskNotifyPipeOut = taskNotifyPipe[1];
        
        System::setCloseOnExecFlag(taskNotifyPipeIn);
        System::setCloseOnExecFlag(taskNotifyPipeOut);
        
        {
            struct sigaction handler;
       
            handler.sa_handler = sigchildHandler;
            sigfillset(&handler.sa_mask);
            handler.sa_flags = SA_NOCLDSTOP;
        #ifdef SA_RESTART
            handler.sa_flags |= SA_RESTART;
        #endif
        #ifdef SA_INTERRUPT
            handler.sa_flags &= ~SA_INTERRUPT;
        #endif
       
            if (::sigaction(SIGCHLD, &handler, NULL) != 0) {
                throw SystemException(String() << "Could not call sigaction: " << strerror(errno));
            }
            
            sigfillset(&enabledSignalBlockMask);
            sigfillset(&disabledSignalBlockMask);
            
            sigdelset(&enabledSignalBlockMask, SIGCHLD); // Child process terminated
            sigdelset(&enabledSignalBlockMask, SIGTSTP); // Terminal stop signal.
            sigdelset(&enabledSignalBlockMask, SIGCONT); // Continue executing, if stopped.
            
        }
        hasSignalHandlers = true;
    }
    disableSignals();
}

void EventDispatcher::registerEventReceiver(const GuiWidget::EventRegistration& registration)
{
    widgetMap.set(registration.wid, registration.guiWidget);
}

void EventDispatcher::registerEventReceiverForForeignWidget(const GuiWidget::EventRegistration& registration)
{
    foreignWidgetListeners.set(registration.wid, registration.guiWidget);
}

void EventDispatcher::removeEventReceiver(const GuiWidget::EventRegistration& registration)
{
    widgetMap.remove(registration.wid);
    foreignWidgetListeners.remove(registration.wid);
}


bool EventDispatcher::isForeignWidget(WidgetId wid)
{
    WidgetMap::Value foundWidget = widgetMap.get(wid);
    return !foundWidget.isValid();
}


EventDispatcher::TimerRegistration EventDispatcher::getNextTimer()
{
    while (true) {
        if (timers.empty()) {
            return TimerRegistration();
        } else {
            EventDispatcher::TimerRegistration rslt = timers.top();
                                                      timers.pop();
            if (rslt.isValid()) {
                return rslt;
            }
        }
    }
}


bool EventDispatcher::processEvent(XEvent* event)
{
    bool hasSomethingDone = false;
    
    if (event->type == MappingNotify) {
        if (event->xmapping.request == MappingKeyboard
                || event->xmapping.request == MappingModifier) {
            XRefreshKeyboardMapping(&event->xmapping);
        }
    } else {
        WidgetId widgetId = WidgetId(event->xany.window);
        
        if (hasRootPropertyListeners && widgetId == rootWid
                                     && event->type == PropertyNotify)
        {
            lastX11EventTime = event->xproperty.time;
            GuiRootProperty property(event->xproperty.atom);
            RootPropertiesMap::Value foundListener = rootPropertyListeners.get(property);
            if (foundListener.isValid()) {
                Callback<XEvent*>::Ptr callback = foundListener.get();
                if (callback->isEnabled()) {
                    callback->call(event);
                    hasSomethingDone = true;
                } else {
                    rootPropertyListeners.remove(property);
                }
            }
        }
        else
        {
            switch (event->type)
            {
                case KeyPress:
                    {
                        lastX11EventTime = event->xkey.time; 
                        break;
                    }
                case KeyRelease:
                    {
                        lastX11EventTime = event->xkey.time; 
                    
                        // The following code tries to discard all further autorepeated 
                        // key events for this key that are already in the xlib event
                        // queue. This is to prevent lagged cursor movements etc.
                    
                        Display*     display = GuiRoot::getInstance()->getDisplay();
                        unsigned int keycode = event->xkey.keycode;
                        
                        peekedEvents.clear();
                        peekedEvents.append(*event);
               
                        XEvent nextEvent;
               
                        while (XEventsQueued(display, QueuedAlready) >= 1)
                        {
                            XNextEvent(display, &nextEvent);
                
                            if (   (   nextEvent.type == KeyRelease
                                    || nextEvent.type == KeyPress)
                                && (   nextEvent.xkey.keycode != keycode))
                            {
                                XPutBackEvent(display, &nextEvent);
                                break;
                            }
                            else {
                                peekedEvents.append(nextEvent);
                            }
                        }
                        int foundIndex1 = 0;
                        int foundIndex2 = 0;

                        for (int i = 0, n = peekedEvents.getLength(); i < n; ++i)
                        {
                            XEvent* e1 = &peekedEvents[i];
                            
                            if (   e1->xkey.type    == KeyRelease
                                || e1->xkey.type    == KeyPress)
                            {
                                foundIndex1 = i;
                                foundIndex2 = i;
                                
                                if (i + 1 < n)
                                {
                                    XEvent* e2 = &peekedEvents[i + 1];

                                    if (   e1->xkey.type    == KeyRelease
                                        && e2->xkey.type    == KeyPress
                                        && labs(e1->xkey.time -  e2->xkey.time) <= 2)  // autorepeated keys used to have same timestamp
                                    {                                                  // but newer xservers messed it up
                                        i += 1;
                                        foundIndex2 = i; // found the corresponding autorepeated KeyRelease event
                                        continue;
                                    }
                                }
                                break;
                            }
                        }
                        for (int i = peekedEvents.getLength() - 1; i > foundIndex2; --i)
                        {
                            XPutBackEvent(display, &peekedEvents[i]);
                        }

                        for (int i = foundIndex1 - 1; i >= 0; --i)
                        {
                            XEvent* e = &peekedEvents[i];
                            if (   e->xkey.type    != KeyRelease
                                && e->xkey.type    != KeyPress)
                            {
                                XPutBackEvent(display, e);
                            }
                            else {
                                // discard this key event because it is autorepeated
                            }
                        }
                        
                        *event = peekedEvents[foundIndex1];
                        
                        if (foundIndex2 != foundIndex1) {
                            XPutBackEvent(display, &peekedEvents[foundIndex2]); // corresponding autorepeated KeyRelease 
                        }                                                       // event is preserved
                        
                        peekedEvents.clear();
                    }
                    break;

                case ButtonPress:      beforeMouseClickCallbackContainer.invokeAllCallbacks();
                case ButtonRelease:    lastX11EventTime = event->xbutton.time; 
                                       break;

                case MotionNotify:     lastX11EventTime = event->xmotion.time; 
                                       break;
                case EnterNotify:
                case LeaveNotify:      lastX11EventTime = event->xcrossing.time; 
                                       break;
                case SelectionNotify:  lastX11EventTime = event->xselection.time; 
                                       break;
                case SelectionClear:   lastX11EventTime = event->xselectionclear.time; 
                                       break;
                case SelectionRequest: lastX11EventTime = event->xselectionrequest.time; 
                                       break;
            }
            WidgetMap::Value foundWidget = widgetMap.get(widgetId);
            if (foundWidget.isValid()) {
                foundWidget.get()->processEvent(event);
                hasSomethingDone = true;
            } else {
                foundWidget = foreignWidgetListeners.get(widgetId);
                if (foundWidget.isValid()){
                    foundWidget.get()->processEvent(event);
                    hasSomethingDone = true;
                }   
            }
        }
    }
    return hasSomethingDone;
}

void EventDispatcher::doEventLoop()
{
    fd_set             readfds;
    fd_set             writefds;
    fd_set             exceptfds;

    XEvent             event;
    TimePeriod         remainingTime;
    Display*           display = GuiRoot::getInstance()->getDisplay();
    
    bool hasSomethingDone = true;

    while (!doQuit)
    {
        if (hasSomethingDone) {
            invokeAllUpdateCallbacks();
            hasSomethingDone = false;   
        }

        if (XEventsQueued(display, QueuedAfterFlush) > 0) {
            XNextEvent(display, &event);
            hasSomethingDone = processEvent(&event);
        } 
        else if (stoppingComponents.getLength() == 0)
        {
            TimerRegistration nextTimer = getNextTimer();
            
            FD_ZERO(&readfds);
            FD_ZERO(&writefds);
            FD_ZERO(&exceptfds);
            FD_SET(x11FileDescriptor, &readfds);
            FD_SET(x11FileDescriptor, &exceptfds);
            int maxFileDescriptor = x11FileDescriptor;
            
            util::maximize(&maxFileDescriptor, sigChildPipeIn);
            FD_SET(sigChildPipeIn, &readfds);

            util::maximize(&maxFileDescriptor, taskNotifyPipeIn);
            FD_SET(taskNotifyPipeIn, &readfds);
            
            for (int i = 0; i < fileDescriptorListeners.getLength();)
            {
                FileDescriptorListener::Ptr listener = fileDescriptorListeners[i]; 
                if (listener->isWaitingForRead())
                {
                    util::maximize(&maxFileDescriptor, listener->getFileDescriptor());
                    FD_SET(listener->getFileDescriptor(), &readfds);
                    if (listener->isWaitingForWrite()) {
                        FD_SET(listener->getFileDescriptor(), &writefds);
                    }
                    ++i;
                }
                else if (listener->isWaitingForWrite())
                {
                    util::maximize(&maxFileDescriptor, listener->getFileDescriptor());
                    FD_SET(listener->getFileDescriptor(), &writefds);
                    ++i;
                }
                else {
                    fileDescriptorListeners.remove(i);
                }
            }

            int p = 0;
            bool hasWaitingProcess = false;
            for (; p < processes.getLength();) {
                if (processes[p]->isEnabled()) {
                    if (processes[p]->needsProcessing()) {
                        hasWaitingProcess = true;
                        break;
                    }
                    ++p;
                } else {
                    processes.remove(p);
                }
            }
            
            int selectResult;
            bool wasSelectInvoked = false;
            bool wasTimerInvoked = false;
            
            {
                if (nextTimer.isValid()) {
#if 0
    TopWin::checkTopWinFocus();
#endif
                    if (hasWaitingProcess)
                    {
                        TimeStamp now = TimeStamp::now();
                        if (nextTimer.getTimeStamp() > now)
                        {
                            ProcessHandler::Ptr h = processes[p];
                            processes.remove(p);
                            processes.append(h);
                            TimeStamp latest = now + MilliSeconds(20);
                            if (nextTimer.getTimeStamp() > latest ) {
                                h->process(latest);
                            } else {
                                h->process(nextTimer.getTimeStamp());
                            }
                            hasSomethingDone = true;
                            now = TimeStamp::now();
                            
                            if (nextTimer.getTimeStamp() < now + MilliSeconds(20)) {
                                remainingTime = nextTimer.getTimeStamp() - now;
                                selectResult = internalSelect(maxFileDescriptor + 1, &readfds, &writefds, NULL, remainingTime);
                                wasSelectInvoked = true;
                            }
                        } else {
                            remainingTime = nextTimer.getTimeStamp() - now;
                            selectResult = internalSelect(maxFileDescriptor + 1, &readfds, &writefds, NULL, remainingTime);
                            wasSelectInvoked = true;
                        }
                    } else {
                        TimeStamp now = TimeStamp::now();
                        
                        remainingTime = nextTimer.getTimeStamp() - now;
                            
                        selectResult = internalSelect(maxFileDescriptor + 1, &readfds, &writefds, NULL, remainingTime);
                        
                        wasSelectInvoked = true;
                    }
                } else {
                    if (hasWaitingProcess) {
                        ProcessHandler::Ptr h = processes[p];
                        processes.remove(p);
                        processes.append(h);
                        h->process(TimeStamp::now() + MilliSeconds(20));
                        hasSomethingDone = true;
                    } else {
                    
                        selectResult = internalSelect(maxFileDescriptor + 1, &readfds, &writefds, NULL, Null);
                        wasSelectInvoked = true;
                    }
                }
            }
            
            if (wasSelectInvoked) {
                if (selectResult > 0) {
                    if (FD_ISSET(x11FileDescriptor, &readfds)) {
                        XNextEvent(display, &event);
                        hasSomethingDone = processEvent(&event);
                    }
                    for (int i = 0; i < fileDescriptorListeners.getLength(); ++i)
                    {
                        FileDescriptorListener::Ptr listener = fileDescriptorListeners[i];
                        int                         fd       = listener->getFileDescriptor();
                    
                        if (FD_ISSET(fd, &readfds)) {
                            listener->handleReading();
                            hasSomethingDone = true;
                        }
                        if (FD_ISSET(fd, &writefds)) {
                            listener->handleWriting();
                            hasSomethingDone = true;
                        }
                    }
                    if (FD_ISSET(sigChildPipeIn, &readfds)) {
                        char buffer[40];
                        int readCounter = ::read(sigChildPipeIn, buffer, sizeof(buffer));

                        int status;
                        pid_t pid = 0;

                        do {
                            pid = waitpid(-1, &status, WNOHANG);
                            if (pid == -1) {
                                if (errno != ECHILD) {
                                    throw SystemException(String() << "Error while calling waitpid: " << strerror(errno));
                                }
                                pid = 0;
                            }
                            if (pid != 0)
                            {
                                ProcessListenerMap::Value foundListener = childProcessListeners.get(pid);
                                if (foundListener.isValid())
                                {
                                    int returnCode = -1;
                                    if (WIFEXITED(status)) {
                                        returnCode = WEXITSTATUS(status);
                                    }
                                    foundListener.get()->call(returnCode);
                                    childProcessListeners.remove(pid);
                                    hasSomethingDone = true;
                                }
                            }
                        } while (pid != 0);
                    }
                    if (FD_ISSET(taskNotifyPipeIn, &readfds))
                    {
                        Mutex::Lock lock(mutex);
                        {
                            char buffer[40];
                            int readCounter = ::read(taskNotifyPipeIn, buffer, sizeof(buffer));

                            for (int i = 0; i < tasks.getLength(); ++i) {
                                tasks[i]->call();
                            }
                            tasks.clear();
                        }
                    }
                } else {
                    if (nextTimer.isValid()) {
                        if (nextTimer.getTimeStamp() < TimeStamp::now()) {
                            nextTimer.getCallback()->call();
                            hasSomethingDone = true;
                            wasTimerInvoked = true;
                        }
                    }
                }
            }
            if (nextTimer.isValid() && !wasTimerInvoked) {
                timers.push(nextTimer); // nextTimer was not executed -> queue it again
            }
        }
        
        if (stoppingComponents.getLength() > 0)
        {
            for (int i = 0; i < stoppingComponents.getLength(); ++i)
            {
                if (stoppingComponents[i].isValid())
                {
                    for (int j = 0; j < runningComponents.getLength();)
                    {
                        if (runningComponents[j].isInvalid()) {
                            runningComponents.remove(j);
                        } else if (runningComponents[j] == stoppingComponents[i]) {
                            runningComponents.remove(j);
                        } else {
                            ++j;
                        }
                    }
                }
            }
            stoppingComponents.clear();
            
            if (runningComponents.getLength() == 0) {
                requestProgramTermination();
            }
            hasSomethingDone = true;
        }
    }
    doQuit = false;
}

void EventDispatcher::registerUpdateSource(Callback<>::Ptr updateCallback)
{
    updateCallbacks.registerCallback(updateCallback);
}

void EventDispatcher::invokeAllUpdateCallbacks()
{
    updateCallbacks.invokeAllCallbacks();
}

void EventDispatcher::registerProcess(ProcessHandler::Ptr h)
{
    processes.append(h);
    if (h->needsProcessing()) {
        h->process(TimeStamp::now() + MilliSeconds(20));
    }
}

ProcessHandler::Ptr EventDispatcher::getNextWaitingProcess()
{
    for (int i = 0; i < processes.getLength(); ++i) {
        if (processes[i]->needsProcessing()) {
            ProcessHandler::Ptr rslt = processes[i];
            processes.remove(i);
            processes.append(rslt);
            return rslt;
        }
    }
    return ProcessHandler::Ptr();
}

void EventDispatcher::deregisterAllUpdateSourceCallbacksFor(WeakPtr<HeapObject> callbackObject)
{
    updateCallbacks.deregisterAllCallbacksFor(callbackObject);
}


void EventDispatcher::registerEventReceiverForRootProperty(GuiRootProperty property,
                                                           Callback<XEvent*>::Ptr callback)
{
    if (!hasRootPropertyListeners) {
        GuiRoot* root = GuiRoot::getInstance();
        rootWid = root->getRootWid();
        XSelectInput(root->getDisplay(), rootWid, PropertyChangeMask);    
        hasRootPropertyListeners = true;
    }
    rootPropertyListeners.set(property, callback);
}



void EventDispatcher::registerRunningComponent(OwningPtr<RunningComponent> runningComponent)
{
    runningComponents.append(runningComponent);
}


void EventDispatcher::deregisterRunningComponent(RunningComponent* runningComponent)
{
    stoppingComponents.append(runningComponent);
}

void EventDispatcher::registerFileDescriptorListener(FileDescriptorListener::Ptr fileDescriptorListener)
{
    fileDescriptorListeners.append(fileDescriptorListener);
}

void EventDispatcher::registerForTerminatingChildProcess(pid_t childPid, Callback<int>::Ptr callback)
{
    childProcessListeners.set(childPid, callback);
}

void EventDispatcher::executeTaskOnMainThread(Callback<>::Ptr task)
{
    Mutex::Lock lock(mutex);
    {
        tasks.append(task);

        char msg = 't';
        ::write(taskNotifyPipeOut, &msg, 1);
    }
}
