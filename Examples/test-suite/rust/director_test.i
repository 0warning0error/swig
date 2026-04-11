%module(directors="1") director_test

// Test Director (virtual function callback)
%feature("director") CallbackBase;

%inline %{
class CallbackBase {
public:
    virtual ~CallbackBase() {}
    
    virtual void on_event(int event_id) {}
    virtual int process(int value) { return value; }
    
    void trigger_event(int id) {
        on_event(id);
    }
    
    int call_process(int v) {
        return process(v);
    }
};

class CallbackUser {
public:
    CallbackBase* callback;
    
    CallbackUser() : callback(nullptr) {}
    CallbackUser(CallbackBase* cb) : callback(cb) {}
    
    void set_callback(CallbackBase* cb) { callback = cb; }
    CallbackBase* get_callback() const { return callback; }
    
    void notify(int event_id) {
        if (callback) callback->on_event(event_id);
    }
    
    int process_value(int v) {
        return callback ? callback->process(v) : -1;
    }
};
%}
