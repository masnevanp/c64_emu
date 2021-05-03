#ifndef MENU_H_INCLUDED
#define MENU_H_INCLUDED

#include <string>
#include <vector>
#include <functional>
#include "utils.h"



namespace Menu {

/*
    A simple menu operated with three keys:
    - enter: to select/enter items (values/actions/etc..)
    - plus/minus: adjust selected item
*/

class Item {
public:
    Item(const char* name_) : name(name_) {}
    virtual ~Item() {}

    virtual Item* select() { return this; }

    // key handlers
    virtual Item* key_enter() = 0;
    virtual void  key_plus()  {}
    virtual void  key_minus() {}

    virtual std::string state() const { return ""; }

    const char* name;
};


class Controller {
public:
    Controller(Item* act_) : act(act_) {}
    virtual ~Controller() {}

    void key_enter() {
        Item* next = act->key_enter();
        if (next) {
            prev = act;
            act = next;
        } else {
            act = prev;
        }
    }
    void key_plus()  { act->key_plus();  }
    void key_minus() { act->key_minus(); }

    const Item* active() const { return act; }

private:
    Item* act = nullptr;
    Item* prev = nullptr;
};


class Group : public Item {
public:
    Group(const char* name, std::vector<Item*> items_) : Item(name), items(items_) {
        for (auto item : items) {
            Group* g = dynamic_cast<Group*>(item);
            if (g) g->add(this); // add link to parent
        }
    }
    virtual ~Group() {}

    virtual Item* key_enter() {
        auto item = selected()->select();
        return item ? item : this;
    }
    virtual void key_plus()  { ++selector; }
    virtual void key_minus() { --selector; }

    virtual std::string state() const { return selected()->name; }

    void add(Item* item) { items.push_back(item); }

private:
    Item* selected() const { return items[selector % items.size()]; }

    std::vector<Item*> items;
    unsigned int selector = 0;
};


class Kludge : public Item {
public:
    Kludge(const char* name,
        std::function<Item* ()> sel_=[](){ return nullptr; },
        std::function<Item* ()> ent_=[](){ return nullptr; },
        std::function<void ()> pl_=[](){},
        std::function<void ()> mi_=[](){}
    ) : Item(name), sel(sel_), ent(ent_), pl(pl_), mi(mi_) {}
    virtual ~Kludge() {}

    virtual Item* select()    { return sel(); }
    virtual Item* key_enter() { return ent(); }
    virtual void  key_plus()  { pl(); }
    virtual void  key_minus() { mi(); }

private:
    std::function<Item* ()> sel;
    std::function<Item* ()> ent;
    std::function<void ()> pl;
    std::function<void ()> mi;
};


/*
class Action : public Item {
public:
    Action(const char* name) : Item(name) {}
    virtual Item* key_enter() { done(); return nullptr; }
    virtual void done() = 0;
};
*/

template<typename T>
class Dial : public Item /*Action*/ {
public:
    Dial(
        const char* name, T& connected_t_,
        const T& min_, const T& max_, const T& step_,
        bool live_notify_, Sig notify)
      : Item(name), connected_t(connected_t_), pos(connected_t_),
        min(min_), max(max_), step(step_),
        live_notify(live_notify_), _notify(notify) {}
    virtual ~Dial() {}

    //virtual void done()      { if (!live_notify) notify(); }

    virtual Item* key_enter() { if (!live_notify) notify(); return nullptr; }
    virtual void  key_plus()  { adjust(+step); }
    virtual void  key_minus() { adjust(-step); }

    virtual std::string state() const { return std::to_string(pos); }

    void notify(/*bool force=false*/) {
        //if (connected_t != pos || force) {
            connected_t = pos;
            _notify();
        //}
    }

private:
    T& connected_t;
    T pos;

    const T min;
    const T max;
    const T step;

    const bool live_notify;

    Sig _notify;

    void adjust(const T& amount) {
        T adjusted = pos + amount;
        if (adjusted >= min && adjusted <= max) { // TODO(?): wrap around
            pos = adjusted;
            if (live_notify) notify();
        }
    }
};


} // namespace Menu


#endif // MENU_H_INCLUDED