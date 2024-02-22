#include <functional>
#include <iostream>
#include <memory>

using namespace std;

struct Trait
{
public:
    Trait(const Trait &trait) = default;
    Trait(Trait &&trait) = default;
    Trait &operator=(const Trait &trait) = default;
    Trait &operator=(Trait &&trait) = default;
    virtual ~Trait() = default;

    template<typename T>
    explicit Trait(T t)
        : container(std::make_shared<Model<T>>(std::move(t)))
    {
    }

    template<typename T>
    T cast()
    {
        auto typed_container = std::static_pointer_cast<const Model<T>>(container);
        return typed_container->m_data;
    }

private:
    struct Concept
    {
        // All need to be explicitly defined just to make the destructor virtual
        Concept() = default;
        Concept(const Concept &concept) = default;
        Concept(Concept &&concept) = default;
        Concept &operator=(const Concept &concept) = default;
        Concept &operator=(Concept &&concept) = default;
        virtual ~Concept() = default;
    };

    template<typename T>
    struct Model : public Concept
    {
        Model(T x)
            : m_data(move(x))
        { }
        T m_data;
    };

    std::shared_ptr<const Concept> container;
};