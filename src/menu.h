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

    virtual bool activate() { return true; }
    virtual bool enter()    { return false; }
    virtual bool exit()     { return true; }
    virtual void up()       {}
    virtual void down()     {}

    virtual bool activate(const std::string& item_name) { UNUSED(item_name); return false; }
    virtual bool activate(const std::string& item_name, const std::string& sub_item_name) {
        UNUSED2(item_name, sub_item_name); return false;
    }

    virtual std::string active_text() const   { return name; }
    virtual std::string inactive_text() const { return name; }

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
        if (!active) {
            if (items[selector]->activate()) active = items[selector];
        }
        else if (!active->enter()) {
            active = nullptr;
        }

        return true;
    }
    virtual bool exit() {
        if (active) {
            if (active->exit()) active = nullptr;
            return false;
        }

        return true;
    }
    virtual void up() {
        if (active) active->up();
        else {
            if (selector == 0) selector = items.size();
            --selector;
        }
    }
    virtual void down() {
        if (active) active->down();
        else if (++selector == items.size()) selector = 0;
    }

    virtual bool activate(const std::string& item_name) {
        unsigned int new_selector = 0;
        for (const auto i : items) {
            if (i->name == item_name) {
                active = i;
                selector = new_selector;
                return true;
            }
            ++new_selector;
        }

        return false;
    }

    virtual bool activate(const std::string& item_name, const std::string& sub_item_name) {
        return activate(item_name) && active->activate(sub_item_name);
    }

    virtual std::string active_text() const {
        const std::string lead = (name != "")
            ? name + " / "
            : "";

        if (active) return lead + active->active_text();
        else return lead + items[selector]->inactive_text();
    }

    virtual std::string inactive_text() const { return name + " /"; }

private:
    Item* active = nullptr;

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

    virtual std::string active_text() const { return name + (confirmed ? "  YES" : "  NO"); }

private:
    bool confirmed = false;
    std::function<void ()> act;
};


class Immediate_action : public Item {
public:
    Immediate_action(const std::string& name, std::function<void ()> a) : Item(name), act(a) {}

    virtual bool activate() {
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

    virtual bool activate() { return imp->activate(); }
    virtual bool enter()    { return imp->enter(); }

    virtual void up()     { imp->up(); }
    virtual void down()   { imp->down(); }

    virtual std::string active_text() const { return name + imp->active_text(); }

private:
    std::shared_ptr<Item> imp;

    template<typename T>
    class _Param : public Item {
    public:
        _Param(T& param_, Sig notify_) : Item(""), param(param_), notify(notify_) { notify(); }

        virtual void up()    { ++param; notify(); }
        virtual void down()  { --param; notify(); }

        virtual std::string active_text() const { return "  # " + std::string(param); }

    private:
        T& param;
        Sig notify;
    };

    template<typename T>
    class _Choice : public Item {
    public:
        _Choice(T& choice_, Sig notify_) : Item(""), choice(choice_), notify(notify_) { notify(); }

        virtual bool activate() {
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

        virtual std::string active_text() const { return "  @ " + chosen_str(); }

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