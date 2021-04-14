//
//                  Simu5G
//
// Authors: Giovanni Nardini, Giovanni Stea, Antonio Virdis (University of Pisa)
//
// This file is part of a software released under the license included in file
// "license.pdf". Please read LICENSE and README files before using it.
// The above files and the present reference are part of the software itself,
// and cannot be removed from it.
//

#ifndef _LTE_TTIMER_H_
#define _LTE_TTIMER_H_

#include "common/timer/TTimerMsg_m.h"
#include "common/timer/TMultiTimerMsg_m.h"

//using namespace omnetpp;

//! Generic timer interface
class TTimer : public omnetpp::cObject
{
  public:
    /*! Build an idle timer.
     *
     * @param module - the connected module
     * @return the idle timer
     */
    TTimer(omnetpp::cSimpleModule* module)
    {
        module_ = module;
        busy_ = false;
        start_ = 0;
        expire_ = 0;
        timerId_ = 0;
    }

    /*! Do nothing.
     *
     * @return
     */
    ~TTimer()
    {
    }

    /*! Start the timer. The timer is now busy.
     * If the timer has been already started, then probably there is
     * a programming error. Thus, we abort execution immediately.
     * @param t interval before timer is triggered
     */
    void start(omnetpp::simtime_t t);

    /*! Stop the timer, if busy. The timer is now idle.
     *
     */
    void stop();

    /*!
     *  Call-back to signal the timer event handled by connected simpleModule
     */
    void handle();

    /*!
     * Gets the timer identifier which is inserted into each timer message
     * @return The timer identifier
     */
    unsigned int getTimerId() const
    {
        return timerId_;
    }

    /*!
     * Sets the timer identifier which is inserted into each timer message
     *
     * @param timerId_ The timer identifier
     */
    void setTimerId(unsigned int timerId_)
    {
        this->timerId_ = timerId_;
    }

    /*! Return true if the timer is busy.
     *
     * @return whether the timer is busy or not
     */
    bool busy()
    {
        return busy_;
    }

    /*! Return true if the timer is idle.
     *
     * @return whether the timer is idle or not
     */
    bool idle()
    {
        return !busy_;
    }

    /*! Return the elapsed time from timer start.
     *
     * @return the elapsed time
     */
    omnetpp::simtime_t elapsed()
    {
        return NOW - start_;
    }

    /*! Return the remaining time before expiration.
     *
     * @return the remaining time
     */
    omnetpp::simtime_t remaining()
    {
        return expire_ - NOW;
    }

  protected:
    //!Timer identifier - will be inserted in each timer-generated message
    unsigned int timerId_;

    //! Object for handling the event.
    omnetpp::cSimpleModule* module_;

    //! Used for scheduling an event into the Omnet++ event scheduler
    TTimerMsg * intr_;

    //! True if the the timer has been already started.
    bool busy_;

    //! Last time the timer was started.
    omnetpp::simtime_t start_;

    //! Expire time.
    omnetpp::simtime_t expire_;
};

/*!
 * Generic multi-timer interface.
 *
 *  A multi-timer is a timer that handles many events. Events are just added
 *  to the multi-timer, which will dispatch them all when needed.
 *  To distinguish between different events, the module handler will receive a message
 *  with an event id specified by user.
 */
class TMultiTimer : public omnetpp::cObject
{
  public:
    //! Build an idle multi-timer.
    TMultiTimer(omnetpp::cSimpleModule* module)
    {
        module_ = module;
        busy_ = false;
        timerId_ = 0;
    }

    //! Do nothing.
    virtual ~TMultiTimer()
    {
    }

    /*! Add an event to the multi-timer, with a specified parameter.
     *
     * @param t the scheduled event time
     * @param event the event id
     */
    virtual void add(omnetpp::simtime_t t, unsigned int event);

    /*!
     * Gets the timer identifier which is inserted into each timer message
     * @return The timer identifier
     */
    unsigned int getTimerId() const
    {
        return timerId_;
    }

    /*!
     * Sets the timer identifier which is inserted into each timer message
     *
     * @param timerId_ The timer identifier
     */
    void setTimerId(unsigned int timerId_)
    {
        this->timerId_ = timerId_;
    }

    /*! Remove an event. Note that event id are assumed to be unique
     *
     * @param event event to be removed
     */
    virtual void remove(const unsigned int event);

    /*!
     * Callback to signal the timer event handled by connected simpleModule
     * @param event event to be signaled
     */
    void handle(unsigned int event);

    /*! Return true if there is at least one scheduled event.
     *
     * @return whether there is at least one scheduled event
     */
    virtual bool busy() const
    {
        return busy_;
    }

    /*! Return true if the event id exists
     *
     * @param event the event id to be checked
     * @return if the event <event> has been scheduled
     */
    virtual bool busy(const unsigned int event) const;

    /*! Return true if there are no scheduled events.
     *
     * @return whether there are no scheduled events
     */
    virtual bool idle() const
    {
        return !busy_;
    }

  protected:
    //!Timer identifier - will be inserted in each timer-generated message
    unsigned int timerId_;

    //! Object for handling the event.
    omnetpp::cSimpleModule* module_;

    //! Used for scheduling an event into the Omnet++ event scheduler
    TMultiTimerMsg* intr_;

    //! True if there is at least one scheduled event.
    bool busy_;

    //! The event list.
    /** More than one event can have the same finish time.
     */
    std::multimap<const omnetpp::simtime_t, const unsigned int> directList_;

    //! Event iterator
    typedef std::multimap<const omnetpp::simtime_t, const unsigned int>::iterator Event_it;

    //! The removable list. Each event id matches with a iterator.
    std::map<const unsigned int, const Event_it> reverseList_;

    //! Direct list iterator type
    typedef std::multimap<const omnetpp::simtime_t, const unsigned int>::iterator iterator_d;

    //! Reverse list iterator type
    typedef std::map<const unsigned int, const Event_it>::iterator iterator_r;
};

#endif
