class Value {
    double number;
public:
    Value(double d): number{d} {}
    operator double() { return number; }
};

class Device {
public:
    virtual ~Device() = default;
    virtual void on_update(Wire* cause) {
        std::cout << "New value: " << cause->get_signal() << "." << std::endl;
    }
    virtual void on_forget(Wire* cause) {
        std::cout << "Forget value." << std::endl;
    }
    static Device USER;
};

Device Device::USER;

class Wire {
    std::optional<Value> signal;
    Device* emitter;
    std::vector<Device*> constraints;
public:
    Wire() = default;
    void set_signal(Value v, Device* d) {
        if (emitter == &Device::USER && d != &Device::USER) {
            throw std::runtime_error{"Trying to override user provided signal."}
        }
        else {
            signal = v;
            emitter = d;
            for (auto* lsn : constraints) {
                if (lsn != d) {
                    lsn.on_update(this);
                }
            }
        }
    }
    void unset_signal(Device* d) {
        if (emitter == &Device::USER && d != &Device::USER) {
            throw std::runtime_error{"Trying to forget user provided signal."}
        }
        else {
            signal.reset();
            emitter = d;
            for (auto* lsn : constraints) {
                if (lsn != d) {
                    lsn.on_forget(this);
                }
            }
        }
    }
    Value get_signal() {
        return *signal;
    }
    const Value operator * () {
        return *signal;
    }
    bool is_set() {
        return signal.has_value();
    }
    void add_constraint(Device* d) {
        constraints.push_back(d);
        if (is_set()) {
            d->on_update(this);
        }
    }
};

class Adder: public Device {
    Wire* addend1;
    Wire* addend2;
    Wire* sum;
public:
    Adder(Wire* a1, Wire* a2, Wire* s):
        addend1{a1}, addend2{a2}, sum{s} {}
    void on_update(Wire*) override {
        if (addend1->is_set() && addend2->is_set()) {
            sum->set_signal(*addend1 + *addend2);
        }
        else if (addend1->is_set() && sum->is_set()) {
            addend2->set_signal(*sum - *addend1);
        }
        else if (addend2->is_set() && sum->is_set()) {
            addend1->set_signal(*sum - *addend2);
        }
    }
    void on_forget(Wire*) override {
        addend1->unset_signal(this);
        addend2->unset_signal(this);
        sum->unset_signal(this);
        on_update(nullptr);
    }
};

int main() {
    Wire a1, a2, sum;
    Adder adder{&a1, &a2, &sum};
    Device print;
    sum.add_constraint(print);
    a1.set_signal(5);
    a2.set_signal(6);
    return 0;
}