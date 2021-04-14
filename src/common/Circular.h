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

#ifndef _LTE_CIRCULAR_H_
#define _LTE_CIRCULAR_H_

#include <list>
#include <assert.h>

//! Circular list of elements.
template<typename T>
class CircularList
{
    //! Internal list structure.
    std::list<T> list_;

    //! Pointer to the current element.
    typename std::list<T>::iterator cur_;

    //! Number of elements.
    unsigned int size_;

  public:
    //! Create an empty circular list.
    CircularList()
    {
        size_ = 0;
        cur_ = list_.begin();
    }
    //! Do nothing.
    ~CircularList()
    {
    }
    //! Copy constructor
    CircularList(const CircularList<T>& cl)
    {
        list_ = cl.list_;
        size_ = cl.size_;
        typename std::list<T>::const_iterator it = cl.list_.begin();
        cur_ = list_.begin();
        if (size_ != 0)
        {
            while (it != cl.cur_)
            {
                ++it;
                ++cur_;
            }
        }
    }
    //! Assignment operator
    CircularList& operator=(const CircularList<T>& cl)
    {
        list_ = cl.list_;
        size_ = cl.size_;
        typename std::list<T>::const_iterator it = cl.list_.begin();
        cur_ = list_.begin();
        if (size_ != 0)
        {
            while (it != cl.cur_)
            {
                ++it;
                ++cur_;
            }
        }
        return *this;
    }

    //! Return true if the list is empty.
    bool empty()
    {
        return (size_ == 0);
    }

    //! Return the number of elements.
    unsigned int size()
    {
        return size_;
    }

    //! Removes all the elements in the list.
    void clear()
    {
        list_.clear();
        size_ = 0;
    }

    //! Return true if a given element is in the list.
    bool find(const T& t)
    {
        if (size_ == 0)
            return false;
        typename std::list<T>::iterator it;
        for (it = list_.begin(); it != list_.end() && *it != t; ++it)
        {
        }
        if (it == list_.end())
            return false;
        return true;
    }

    //! Finds an element in the list and return it.
    /*!
     The element returned is only meaningful if valid == true.
     */
    T& find(T& t, bool& valid)
    {
        if (size_ == 0)
        {
            valid = false;
            return t;
        }
        typename std::list<T>::iterator it;
        for (it = list_.begin(); it != list_.end() && *it != t; ++it)
        {
        }
        if (it == list_.end())
        {
            valid = false;
            return t;
        }
        valid = true;
        return *it;
    }

    //! Insert a new element before the current position.
    void insert(const T& t)
    {
        if (size_ == 0)
        {
            list_.push_back(t);
            cur_ = list_.begin();
        }
        else
            list_.insert(cur_, t);
        ++size_;
    }

    //! Removes the element at the current position.
    void erase()
    {
        if (size_ == 0)
            return;
        typename std::list<T>::iterator drop = cur_++;
        if (cur_ == list_.end())
            cur_ = list_.begin();
        list_.erase(drop);
        --size_;
    }

    //! Erases the element specified (eventually the current positions is shifted)
    void eraseElem(T& t)
    {
        if (size_ == 0)
            return;
        typename std::list<T>::iterator it;
        for (it = list_.begin(); it != list_.end() && *it != t; ++it)
        {
        }
        if (cur_ == list_.end())
            return;
        if (cur_ == it)
        {
            cur_++;
            if (cur_ == list_.end())
                cur_ = list_.begin();
        }
        list_.erase(it);
        --size_;
    }

    //! Goes back to the beginning of the circular list
    void rewind()
    {
        cur_ = list_.begin();
    }

    //! Moves the pointer to the next element in a circular fashion.
    void move()
    {
        if (size_ > 0 && ++cur_ == list_.end())
            cur_ = list_.begin();
    }

    //! Return the current element.
    /*!
     If the size is 0, then the return value is not meaningful, and
     may also crash program execution. We use an assert here since
     we do not want to use exception, which would be the right thing to do.
     */
    const T& current() const
    {
        assert( size_ > 0);
        return *cur_;
    }

    //! Return the current element.
    T& current()
    {
        assert( size_ > 0);
        return *cur_;
    }

    //! Insert a new element after the current position.
    void insertFront(const T& t)
    {
        if (size_ == 0)
        {
            list_.push_back(t);
            cur_ = list_.begin();
        }
        else
        {
            typename std::list<T>::iterator temp = cur_;
            ++temp;
            list_.insert(temp, t);
        }
        ++size_;
    }
};

#endif // _LTE_CIRCULAR_H_
