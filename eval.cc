#include <iostream>
#include <experimental/optional>
#include <stdexcept>
#include <vector>

class Value {
    double number;
public:
    Value(double d): number{d} {}
    operator double() const { return number; }
};

class Device {
public:
    virtual ~Device() = default;
    virtual void on_update(class Wire* cause) {}
    virtual void on_forget(class Wire* cause) {}
    static Device USER;
};

Device Device::USER{};

class Wire {
    std::experimental::optional<Value> signal;
    Device* emitter;
    std::vector<Device*> constraints;
public:
    std::string name;
    Wire() = default;
    Wire(std::string n): name{n} {}
    void set_signal(Value v, Device* d) {
        if (emitter == &Device::USER && d != &Device::USER) {
            throw std::runtime_error{"Trying to override user provided value."};
        }
        else {
            signal = v;
            emitter = d;
            for (auto* lsn : constraints) {
                if (lsn != d) {
                    lsn->on_update(this);
                }
            }
        }
    }
    void reset_signal(Device* d) {
        if (emitter != &Device::USER || d == &Device::USER) {
            signal = std::experimental::nullopt;
            emitter = d;
            for (auto* lsn : constraints) {
                if (lsn != d) {
                    lsn->on_forget(this);
                }
            }
        }
        if (d == &Device::USER) {
            emitter = nullptr;
        }
    }
    Value get_signal() {
        return *signal;
    }
    const Value operator * () {
        return *signal;
    }
    bool is_set() {
        return (bool)signal;
    }
    void add_constraint(Device* d) {
        constraints.push_back(d);
        if (is_set()) {
            d->on_update(this);
        }
    }
};

class PrintDevice: public Device {
public:
    void on_update(class Wire* cause) override {
        std::cout << "New value: "
                  << cause->name
                  << " = "
                  << (double)cause->get_signal() << "." << std::endl;
    }
    void on_forget(class Wire* cause) override {
        std::cout << "Forget value: "
                  << cause->name
                  << " = ?."
                  << std::endl;
    }
};

class Adder: public Device {
    Wire* addend1;
    Wire* addend2;
    Wire* sum;
public:
    Adder(Wire* a1, Wire* a2, Wire* s):
        addend1{a1}, addend2{a2}, sum{s} {
        addend1->add_constraint(this);
        addend2->add_constraint(this);
        sum->add_constraint(this);
    }
    void on_update(Wire*) override {
        if (addend1->is_set() && addend2->is_set()) {
            sum->set_signal((double)**addend1 + (double)**addend2, this);
        }
        else if (addend1->is_set() && sum->is_set()) {
            addend2->set_signal((double)**sum - (double)**addend1, this);
        }
        else if (addend2->is_set() && sum->is_set()) {
            addend1->set_signal((double)**sum - (double)**addend2, this);
        }
    }
    void on_forget(Wire* w) override {
        if (w != addend1) addend1->reset_signal(this);
        if (w != addend2) addend2->reset_signal(this);
        if (w != sum) sum->reset_signal(this);
        on_update(nullptr);
    }
};

class Negater: public Device {
    Wire* in;
    Wire* out;
public:
    Negater(Wire* i, Wire* o): in{i}, out{o} {
        in->add_constraint(this);
        out->add_constraint(this);
    }
    void on_update(Wire*) override {
        if (in->is_set()) {
            out->set_signal(-(double)**in, this);
        }
        else if (out->is_set()) {
            in->set_signal(-(double)**out, this);
        }
    }
    void on_forget(Wire* w) override {
        if (w != in) in->reset_signal(this);
        if (w != out) out->reset_signal(this);
        on_update(nullptr);
    }
};

int main() {
    Wire a1{"a1"}, a2{"a2"}, na2{"na2"}, sum{"sum"};
    PrintDevice print;
    a1.add_constraint(&print);
    a2.add_constraint(&print);
    na2.add_constraint(&print);
    sum.add_constraint(&print);
    Negater negater{&a2, &na2};
    Adder adder{&a1, &na2, &sum};
    a1.set_signal(5, &Device::USER);
    a2.set_signal(6, &Device::USER);
    a2.reset_signal(&Device::USER);
    sum.set_signal(8, &Device::USER);
    return 0;
}
