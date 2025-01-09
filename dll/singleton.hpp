#ifndef SINGLETON_HPP
#define SINGLETON_HPP

template <typename T>
class Singleton {
public:
    static T* get() {
        static T instance;
        return &instance;
    }

    Singleton(const Singleton&) = delete;
    Singleton& operator=(const Singleton&) = delete;

protected:
    Singleton() = default;
    virtual ~Singleton() = default;
};

#endif