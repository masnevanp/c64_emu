#ifndef MENU_H_INCLUDED
#define MENU_H_INCLUDED

#include <string>
#include <vector>
#include <memory>
#include <algorithm>
#include "utils.h"



namespace Menu {

class Item {
public:
    Item(const std::string& name_ = "") : name(name_) {}

    virtual bool select() { return true; }
    virtual bool enter()  { return false; }
    virtual bool back()   { return true; }
    virtual void up()     {}
    virtual void down()   {}

    virtual std::string text() const { return name; }

    const std::string name;
};


class Group : public Item {
public:
    template<typename Cont>
    Group(const std::string& name, Cont& items)
        : Item(name) { add(items); }
    template<typename Cont1, typename Cont2>
    Group(const std::string& name, Cont1& items1, Cont2& items2)
        : Item(name) { add(items1); add(items2); }
    template<typename Cont1, typename Cont2, typename Cont3>
    Group(const std::string& name, Cont1& items1, Cont2& items2, Cont3& items3)
        : Item(name) { add(items1); add(items2); add(items3); }

    /*Group& add(std::initializer_list<Item*> more_items) {
        for (auto item : more_items) {
            if (item != this) items.push_back(item);
        }
        return *this;
    }*/

    template<typename Cont>
    Group& add(Cont& more_items) {
        for (auto& item : more_items) {
            if ((Item*)&item != (Item*)this) items.push_back(&item);
        }
        return *this;
    }

    virtual bool enter() {
        if (!selected) {
            if (items[selector]->select()) selected = items[selector];
        }
        else if (!selected->enter()) {
            selected = nullptr;
        }

        return true;
    }
    virtual bool back() {
        if (selected) {
            if (selected->back()) selected = nullptr;
            return false;
        }

        return true;
    }
    virtual void up() {
        if (selected) selected->up();
        else {
            if (selector == 0) selector = items.size();
            --selector;
        }
    }
    virtual void down() {
        if (selected) selected->down();
        else if (++selector == items.size()) selector = 0;
    }

    virtual std::string text() const {
        if (selected) return name + selected->text();
        else return name + items[selector]->name;
    }

private:
    Item* selected = nullptr;

    std::vector<Item*> items;
    unsigned int selector = 0;
};


class Confirmed_action : public Item {
public:
    Confirmed_action(const std::string& name, std::function<void ()> a) : Item(name), act(a) {}

    virtual bool enter() {
        if (confirmed) {
            act();
            confirmed = false;
        }

        return false;
    }

    virtual void up()   { confirmed = !confirmed; }
    virtual void down() { confirmed = !confirmed; }

    virtual std::string text() const { return name + (confirmed ? "  YES" : "  NO"); }

private:
    bool confirmed = false;
    std::function<void ()> act;
};


class Immediate_action : public Item {
public:
    Immediate_action(const std::string& name, std::function<void ()> a) : Item(name), act(a) {}

    virtual bool select() {
        act();
        return false;
    }

private:
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

    virtual bool select() { return imp->select(); }
    virtual bool enter()  { return imp->enter(); }

    virtual void up()     { imp->up(); }
    virtual void down()   { imp->down(); }

    virtual std::string text() const { return name + imp->text(); }

private:
    std::shared_ptr<Item> imp;

    template<typename T>
    class _Param : public Item {
    public:
        _Param(T& param_, Sig notify_) : Item(""), param(param_), notify(notify_) { notify(); }

        virtual void up()    { ++param; notify(); }
        virtual void down()  { --param; notify(); }

        virtual std::string text() const { return "  # " + std::string(param); }

    private:
        T& param;
        Sig notify;
    };

    template<typename T>
    class _Choice : public Item {
    public:
        _Choice(T& choice_, Sig notify_) : Item(""), choice(choice_), notify(notify_) { notify(); }

        virtual bool select() {
            const auto c = std::find(choice.choices.begin(), choice.choices.end(), choice.chosen);
            ci =  std::distance(choice.choices.begin(), c);
            return true;
        }
        virtual bool enter() {
            choice.chosen = chosen();
            notify();
            return false;
        }
        virtual void up()   {
            ++ci;
            if (ci == choice.choices.size()) ci = 0;
        }
        virtual void down() {
            if (ci == 0) ci = choice.choices.size();
            --ci;
        }

        virtual std::string text() const { return "  @ " + chosen_str(); }

    private:
        T& choice;
        u32 ci = 0;
        Sig notify;

        auto chosen() const { return choice.choices[ci % choice.choices.size()]; }
        const std::string& chosen_str() const { return choice.choices_str[ci % choice.choices_str.size()]; }
    };
};


/*class Kludge : public Item {
public:
    Kludge(const std::string& name,
        std::function<bool ()> select_=[](){ return true; },
        std::function<bool ()> enter=[](){ return false; },
        std::function<bool ()> back=[](){ return true; },
        std::function<void ()> up=[](){},
        std::function<void ()> down=[](){}
    ) : Item(name), _select(select_), _enter(enter), _back(back), _up(up), _down(down) {}

    virtual bool select() { return _select(); }
    virtual bool enter()  { return _enter(); }
    virtual bool back()   { return _back(); }
    virtual void up()     { _up(); }
    virtual void down()   { _down(); }

private:
    std::function<bool ()> _select;
    std::function<bool ()> _enter;
    std::function<bool ()> _back;
    std::function<void ()> _up;
    std::function<void ()> _down;
};*/


}


#endif // MENU_H_INCLUDED