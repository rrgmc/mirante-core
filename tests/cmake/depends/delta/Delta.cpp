namespace mirante {
class ModuleRegistrar;
namespace modules::delta {

struct Base
{
    virtual ~Base() = default;
    virtual int value() const { return 0; }
};

struct Derived : Base
{
    int value() const { return 1; }
};

void registerModule(ModuleRegistrar &)
{
    Derived derived;
    (void)derived.value();
}

}
}
