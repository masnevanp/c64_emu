#ifndef MENU_H_INCLUDED
#define MENU_H_INCLUDED

#include <string>
#include <vector>
#include <functional>
#include <memory>
#include "utils.h"



namespace Menu {

/*
    A simple menu operated with three keys:
    - enter: to select/enter items (values/actions/etc..)
    - up/down: adjust selected item
*/

class Item {
public:
    Item(const std::string& name_) : name(name_) {}
    virtual ~Item() {}

    virtual Item* select() { return this; }
    virtual Item* done()   { return nullptr; }
    virtual void  up()     {}
    virtual void  down()   {}

    virtual std::string state() const { return name; }

    const std::string name;
};


class Controller {
public:
    Controller(Item* init) : prev(init) { select(init); }
    virtual ~Controller()  {}

    void key_enter() {
        Item* next = act->done();
        if (next) {
            prev = act;
            select(next);
        } else {
            act = prev;
        }
    }
    void key_up()   { act->up(); }
    void key_down() { act->down(); }

    void select(Item* item) {
        act = item->select();
        if (!act) act = prev;
    }

    std::string state() const { return act->state(); }

private:
    Item* act = nullptr;
    Item* prev = nullptr;
};


class Group : public Item {
public:
    Group(const std::string& name, std::vector<Item*> items_ = {}) : Item(name), items(items_) {}
    Group(const std::string& name, std::vector<Group>& subs, std::initializer_list<Item*> more_items = {})
      : Item(name)
    {
        for (auto& s : subs) items.push_back(&s);

        add(more_items);
    }
    virtual ~Group() {}

    virtual Item* select() { selector = 0; return this; }
    virtual Item* done()   { return selected(); }
    virtual void up()      { if (selector == 0) { selector = items.size(); } --selector; }
    virtual void down()    { ++selector; if (selector == items.size()) selector = 0; }

    virtual std::string state() const { return selected()->name; }

    void add(std::initializer_list<Item*> more_items) {
        for (auto item : more_items) {
            if (item != this) items.push_back(item);
        }
    }

    template<typename Cont>
    void add(Cont& more_items) {
        for (auto& item : more_items) {
            if ((Item*)&item != (Item*)this) items.push_back(&item);
        }
    }

private:
    Item* selected() const { return items[selector]; }

    std::vector<Item*> items;
    unsigned int selector = 0;
};


class Kludge : public Item {
public:
    Kludge(const std::string& name,
        std::function<Item* ()> select=[](){ return nullptr; },
        std::function<Item* ()> done=[](){ return nullptr; },
        std::function<void ()> up=[](){},
        std::function<void ()> down=[](){}
    ) : Item(name), _select(select), _done(done), _up(up), _down(down) {}
    virtual ~Kludge() {}

    virtual Item* select() { return _select(); }
    virtual Item* done()   { return _done(); }
    virtual void  up()     { _up(); }
    virtual void  down()   { _down(); }

private:
    std::function<Item* ()> _select;
    std::function<Item* ()> _done;
    std::function<void ()> _up;
    std::function<void ()> _down;
};


class Action : public Item {
public:
    Action(const std::string& name, std::function<void ()> a) : Item(name), act(a) {}
    virtual ~Action() {}

    virtual Item* done() {
        if (accept) act();
        accept = false;
        return nullptr;
    }
    virtual void up()   { accept = !accept; }
    virtual void down() { accept = !accept; }

    virtual std::string state() const { return name + (accept ? "  YES" : "  NO"); }

private:
    bool accept = false;
    std::function<void ()> act;
};


class Knob : public Item {
public:
    template<typename P>
    Knob(const std::string& name, P& param, Sig notify)
      : Item(name), imp(std::make_shared<_Param<P>>(param, notify)) { notify(); }
    virtual ~Knob() {}

    virtual Item* done() { return imp->done(); }
    virtual void  up()   { imp->up(); }
    virtual void  down() { imp->down(); }

    virtual std::string state() const { return name + imp->state(); }

private:
    std::shared_ptr<Item> imp;

    template<typename P>
    class _Param : public Item {
    public:
        _Param(P& param_, Sig notify_) : Item(""), param(param_), notify(notify_) {}
        virtual ~_Param() {}

        virtual void  up()   { adjust(+1); }
        virtual void  down() { adjust(-1); }

        virtual std::string state() const { return "  # " + std::to_string(param.val); }

    private:
        P& param;
        Sig notify;

        void adjust(int direction) {
            decltype(param.val) adjusted = param.val + (direction * param.step);
            if (adjusted >= param.min && adjusted <= param.max) {
                param.val = adjusted;
                notify();
            }
        }
    };
};



} // namespace Menu


#endif // MENU_H_INCLUDED