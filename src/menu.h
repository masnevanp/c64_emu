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
    template<typename Cont>
    Group(const std::string& name, Cont& items) : Item(name) { add(items); }
    virtual ~Group() {}

    virtual Item* select() { selector = 0; return this; }
    virtual Item* done()   { return selected(); }
    virtual void up()      { if (selector == 0) { selector = items.size(); } --selector; }
    virtual void down()    { ++selector; if (selector == items.size()) selector = 0; }

    virtual std::string state() const { return selected()->name; }

    Group& add(std::initializer_list<Item*> more_items) {
        for (auto item : more_items) {
            if (item != this) items.push_back(item);
        }
        return *this;
    }

    template<typename Cont>
    Group& add(Cont& more_items) {
        for (auto& item : more_items) {
            if ((Item*)&item != (Item*)this) items.push_back(&item);
        }
        return *this;
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
    template<typename T>
    Knob(const std::string& name, Param<T>& param, Sig notify)
      : Item(name), imp(std::make_shared<_Param<Param<T>>>(param, notify)) {}

    template<typename T>
    Knob(const std::string& name, Choice<T>& choice, Sig notify)
      : Item(name), imp(std::make_shared<_Choice<Choice<T>>>(choice, notify)) {}

    virtual ~Knob() {}

    virtual Item* done() { return imp->done(); }
    virtual void  up()   { imp->up(); }
    virtual void  down() { imp->down(); }

    virtual std::string state() const { return name + imp->state(); }

private:
    std::shared_ptr<Item> imp;

    template<typename T>
    class _Param : public Item {
    public:
        _Param(T& param_, Sig notify_) : Item(""), param(param_), notify(notify_) { notify(); }
        virtual ~_Param() {}

        virtual void  up()   { ++param; notify(); }
        virtual void  down() { --param; notify(); }

        virtual std::string state() const { return "  # " + std::string(param); }

    private:
        T& param;
        Sig notify;
    };

    template<typename T>
    class _Choice : public Item {
    public:
        _Choice(T& choice_, Sig notify_) : Item(""), choice(choice_), notify(notify_) { notify(); }
        virtual ~_Choice() {}

        virtual Item* done() {
            choice.chosen = chosen();
            notify();
            return nullptr;
        }
        virtual void up()   {
            ++ci;
            if (ci == choice.choices.size()) ci = 0;
        }
        virtual void down() {
            if (ci == 0) ci = choice.choices.size();
            --ci;
        }

        virtual std::string state() const { return "  @ " + chosen_str(); }

    private:
        T& choice;
        u32 ci = 0;
        Sig notify;

        auto chosen() const { return choice.choices[ci % choice.choices.size()]; }
        const std::string& chosen_str() const { return choice.choices_str[ci % choice.choices_str.size()]; }
    };
};



} // namespace Menu


#endif // MENU_H_INCLUDED