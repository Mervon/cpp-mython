#include "runtime.h"

#include <cassert>
#include <optional>
#include <sstream>
#include <iostream>

using namespace std;

namespace runtime {

ObjectHolder::ObjectHolder(std::shared_ptr<Object> data)
    : data_(std::move(data)) {
}

void ObjectHolder::AssertIsValid() const {
    assert(data_ != nullptr);
}

ObjectHolder ObjectHolder::Share(Object& object) {
    // Р’РѕР·РІСЂР°С‰Р°РµРј РЅРµРІР»Р°РґРµСЋС‰РёР№ shared_ptr (РµРіРѕ deleter РЅРёС‡РµРіРѕ РЅРµ РґРµР»Р°РµС‚)
    return ObjectHolder(std::shared_ptr<Object>(&object, [](auto* /*p*/) { /* do nothing */ }));
}

ObjectHolder ObjectHolder::None() {
    return ObjectHolder();
}

Object& ObjectHolder::operator*() const {
    AssertIsValid();
    return *Get();
}

Object* ObjectHolder::operator->() const {
    AssertIsValid();
    return Get();
}

Object* ObjectHolder::Get() const {
    return data_.get();
}

ObjectHolder::operator bool() const {
    return Get() != nullptr;
}

bool IsTrue(const ObjectHolder& object) {
    if (object) {
        if (String* str = object.TryAs<String>(); str != nullptr) {
            return str->GetValue().size() != 0;
        } else if (Number* i = object.TryAs<Number>(); i != nullptr) {
            return i->GetValue() != 0;
        } else if (Bool* b = object.TryAs<Bool>(); b != nullptr) {
            return b->GetValue() == true;
        }
    }

    return false;

}

void ClassInstance::Print(std::ostream& os, Context& context) {
    auto method_ptr = this->class_.GetMethod("__str__"s);

    if (method_ptr != nullptr) {
        Call(method_ptr->name, {}, context)->Print(os, context);
    } else {
        os << this;
    }
}

bool ClassInstance::HasMethod(const std::string& method, size_t argument_count) const {
    if (const Method* method__ = class_.GetMethod(method); method__ != nullptr) {
        if (method__->formal_params.size() == argument_count) {
            return true;
        }
    }

    return false;
}

Closure& ClassInstance::Fields() {
    return fields_;
}

const Closure& ClassInstance::Fields() const {
    return fields_;
}

ClassInstance::ClassInstance(const Class& cls) : class_(cls) {
    fields_["self"] = ObjectHolder::Share(*this);
}

ObjectHolder ClassInstance::Call(const std::string& method,
                                     const std::vector<ObjectHolder>& actual_args,
                                     Context& context) {
    if (!HasMethod(method, actual_args.size())) {
        throw std::runtime_error("Not implemented"s);
    }

    auto* method_ptr = class_.GetMethod(method);
    Closure closure;

    closure["self"s] = ObjectHolder::Share(*this);

    for (size_t i = 0; i < actual_args.size(); ++i) {
        closure[method_ptr->formal_params[i]] = actual_args[i];
    }

    return method_ptr->body->Execute(closure, context);
}

Class::Class(std::string name, std::vector<Method> methods, const Class* parent) : name_(std::move(name)), methods_(std::move(methods)), parent_(parent) {

}

const Method* Class::GetMethod(const std::string& name) const {
    const Class* current_class = this;

    do {
        for (const auto& method : current_class->methods_) {
            if (method.name == name) {
                return &method;
            }
        }

        current_class = current_class->parent_;
    } while (current_class != nullptr);

    return nullptr;
}

[[nodiscard]] const std::string& Class::GetName() const {
    return name_;
}

void Class::Print(ostream& os, [[maybe_unused]] Context& context) {
    os << "Class " << name_;
}

void Bool::Print(std::ostream& os, [[maybe_unused]] Context& context) {
    os << (GetValue() ? "True"sv : "False"sv);
}

bool Equal(const ObjectHolder& lhs, const ObjectHolder& rhs, Context& context) {
    if (ClassInstance* item_1 = lhs.TryAs<ClassInstance>(); item_1 != nullptr) {
        if (ClassInstance* item_2 = rhs.TryAs<ClassInstance>(); item_2 != nullptr) {
            if (item_1->HasMethod("__eq__", 1)) {

                std::vector<ObjectHolder> fields;
                for (const auto& field : item_2->Fields()) {
                    fields.push_back(field.second);
                }

                return (item_1->Call("__eq__", fields, context)).TryAs<Bool>()->GetValue();

            }
        }
    } else if (const String* item_1 = lhs.TryAs<String>(); item_1 != nullptr) {
        if (String* item_2 = rhs.TryAs<String>(); item_2 != nullptr) {
            return item_1->GetValue() == item_2->GetValue();
        }

    } else if (const Number* item_1 = lhs.TryAs<Number>(); item_1 != nullptr) {
        if (Number* item_2 = rhs.TryAs<Number>(); item_2 != nullptr) {
            return item_1->GetValue() == item_2->GetValue();
        }
    } else if (const Bool* item_1 = lhs.TryAs<Bool>(); item_1 != nullptr) {
        if (Bool* item_2 = rhs.TryAs<Bool>(); item_2 != nullptr) {
            return item_1->GetValue() == item_2->GetValue();
        }
    } else if (lhs.Get() == nullptr && rhs.Get() == nullptr) {
        return true;
    }

    throw runtime_error("Equal_get_bad_items_to_compare");

}

bool Less(const ObjectHolder& lhs, const ObjectHolder& rhs, Context& context) {
    if (ClassInstance* item_1 = lhs.TryAs<ClassInstance>(); item_1 != nullptr) {
        if (ClassInstance* item_2 = rhs.TryAs<ClassInstance>(); item_2 != nullptr) {
            if (item_1->HasMethod("__lt__", 1)) {

                std::vector<ObjectHolder> fields;
                for (const auto& field : item_2->Fields()) {
                    fields.push_back(field.second);
                }

                return (item_1->Call("__lt__", fields, context)).TryAs<Bool>()->GetValue();

            }
        }
    } else if (const String* item_1 = lhs.TryAs<String>(); item_1 != nullptr) {
        if (String* item_2 = rhs.TryAs<String>(); item_2 != nullptr) {
            return item_1->GetValue() < item_2->GetValue();
        }

    } else if (const Number* item_1 = lhs.TryAs<Number>(); item_1 != nullptr) {
        if (Number* item_2 = rhs.TryAs<Number>(); item_2 != nullptr) {
            return item_1->GetValue() < item_2->GetValue();
        }
    } else if (const Bool* item_1 = lhs.TryAs<Bool>(); item_1 != nullptr) {
        if (Bool* item_2 = rhs.TryAs<Bool>(); item_2 != nullptr) {
            return item_1->GetValue() < item_2->GetValue();
        }
    }

    throw runtime_error("Less_get_bad_items_to_compare");

}

bool NotEqual(const ObjectHolder& lhs, const ObjectHolder& rhs, Context& context) {
    return !Equal(lhs, rhs, context);
}

bool Greater(const ObjectHolder& lhs, const ObjectHolder& rhs, Context& context) {
    return !Less(lhs, rhs, context) && !Equal(lhs, rhs, context);
}

bool LessOrEqual(const ObjectHolder& lhs, const ObjectHolder& rhs, Context& context) {
    return Less(lhs, rhs, context) || Equal(lhs, rhs, context);
}

bool GreaterOrEqual(const ObjectHolder& lhs, const ObjectHolder& rhs, Context& context) {
    return !Less(lhs, rhs, context);
}

}  // namespace runtime
