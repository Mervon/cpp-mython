#include "statement.h"

#include <iostream>
#include <sstream>

using namespace std;

namespace ast {

using runtime::Closure;
using runtime::Context;
using runtime::ObjectHolder;

namespace {
const string ADD_METHOD = "__add__"s;
const string INIT_METHOD = "__init__"s;
}  // namespace

ObjectHolder Assignment::Execute(Closure& closure, Context& context) {
    closure[var_] = rv_.get()->Execute(closure, context); //что если был подан объект, созданный не в дин. памяти?, пока пофиг
    return closure[var_];
}

Assignment::Assignment(std::string var, std::unique_ptr<Statement> rv) : var_(std::move(var)), rv_(std::move(rv)) {
}

VariableValue::VariableValue(const std::string& var_name) : var_name_(std::move(var_name)) {
}

VariableValue::VariableValue(std::vector<std::string> dotted_ids) : dotted_ids_(std::move(dotted_ids)) {
    is_doted_ = true;
}

ObjectHolder VariableValue::Execute(Closure& closure, [[maybe_unused]] Context& context) {
    if (!is_doted_) {
        if (closure.find(var_name_) != closure.end()) {
            return closure[var_name_];
        } else {
            throw std::runtime_error("Var name dont exists");
        }
    } else {
        if (dotted_ids_.size() == 0) {
            throw std::runtime_error("Zero_doted_items");
        }

        ObjectHolder* current_object = nullptr;

        if (auto it = closure.find(dotted_ids_[0]); it != closure.end()) {
            current_object = &(it->second);
        } else {
            throw std::runtime_error("Item_not_found_in_closure_666");
        }

        for (size_t i = 1; i < dotted_ids_.size(); ++i) {
            if (runtime::ClassInstance* tmp_class_instance = current_object->TryAs<runtime::ClassInstance>(); tmp_class_instance != nullptr) {
                current_object = &(tmp_class_instance->Fields()[dotted_ids_[i]]);
            } else {
                throw std::runtime_error("Item_not_found_in_closure_555");
            }
        }

        /*cout << "New zapros" << endl;

        for (string str : dotted_ids_) {
            cout << "<" << str << ">";
        }

        //cout << ":" << dotted_ids_[dotted_ids_.size() - 1] << ":" << endl;

        cout << endl << "Zapros end" << endl;*/


        return *current_object;
    }

    throw std::runtime_error("Bad variable value");
}

unique_ptr<Print> Print::Variable(const std::string& name) {
    return unique_ptr<Print>(new Print(unique_ptr<Statement>(new VariableValue(name))));
}

Print::Print(unique_ptr<Statement> argument) : argument_(std::move(argument)) {

}

Print::Print(vector<unique_ptr<Statement>> args) : args_(std::move(args)) {
    is_print_args_ = true;
}

ObjectHolder Print::Execute(Closure& closure, Context& context) {
    ObjectHolder last_printed_value;

    ostream& os = context.GetOutputStream();

    if (is_print_args_) {
        bool is_first = false;
        for (const auto& arg : args_) {
            if (is_first) {
                os << ' ';
            }

            last_printed_value = arg.get()->Execute(closure, context);
            if (last_printed_value) {
                last_printed_value->Print(os, context);
            } else {
                os << "None";
            }

            is_first = true;
        }
    } else {
        last_printed_value = argument_.get()->Execute(closure, context);
        if (last_printed_value) {
            last_printed_value->Print(os, context);
        } else {
            os << "None";
        }
    }

    os << endl;

    return last_printed_value;
}

MethodCall::MethodCall(std::unique_ptr<Statement> object, std::string method,
                       std::vector<std::unique_ptr<Statement>> args) : object_(std::move(object)), method_(std::move(method)), args_(std::move(args)) {

}

ObjectHolder MethodCall::Execute(Closure& closure, Context& context) {
    ObjectHolder object = object_.get()->Execute(closure, context);

    if (runtime::ClassInstance* cls_inst = object.TryAs<runtime::ClassInstance>(); cls_inst != nullptr) {
        if (cls_inst->HasMethod(method_, args_.size())) {
            std::vector<ObjectHolder> actual_args;
            for (const auto& arg : args_) {
                actual_args.push_back(arg.get()->Execute(closure, context));
            }
            return cls_inst->Call(method_, actual_args, context);
        }
    }

    throw std::runtime_error("Bad method call");
}

ObjectHolder Stringify::Execute(Closure& closure, Context& context) {
    runtime::ObjectHolder object = argument_.get()->Execute(closure, context);

    if (runtime::String* item = object.TryAs<runtime::String>(); item != nullptr) {

        return runtime::ObjectHolder::Own(runtime::ValueObject(item->GetValue()));
    } else if (runtime::Number* item = object.TryAs<runtime::Number>(); item != nullptr) {

        return runtime::ObjectHolder::Own(runtime::ValueObject(std::to_string(item->GetValue())));
    } else if (runtime::Bool* item = object.TryAs<runtime::Bool>(); item != nullptr) {

        if (item->GetValue()) {
            return runtime::ObjectHolder::Own(runtime::ValueObject("True"));
        } else {
            return runtime::ObjectHolder::Own(runtime::ValueObject("False"));
        }
    } else if (object.Get() == nullptr) {

        return runtime::ObjectHolder::Own(runtime::ValueObject("None"));
    } else if (runtime::ClassInstance* item = object.TryAs<runtime::ClassInstance>(); item != nullptr) {
        if (item->HasMethod("__str__", 0)) {
            runtime::ObjectHolder new_object = item->Call("__str__", {}, context);
            if (runtime::String* item_2 = new_object.TryAs<runtime::String>(); item_2 != nullptr) {

                return runtime::ObjectHolder::Own(runtime::ValueObject(item_2->GetValue()));
            } else if (runtime::Number* item_2 = new_object.TryAs<runtime::Number>(); item_2 != nullptr) {

                return runtime::ObjectHolder::Own(runtime::ValueObject(std::to_string(item_2->GetValue())));
            } else if (runtime::Bool* item_2 = new_object.TryAs<runtime::Bool>(); item_2 != nullptr) {

                if (item_2->GetValue()) {
                    return runtime::ObjectHolder::Own(runtime::ValueObject("True"));
                } else {
                    return runtime::ObjectHolder::Own(runtime::ValueObject("False"));
                }
            } else if (new_object.Get() == nullptr) {

                return runtime::ObjectHolder::Own(runtime::ValueObject("None"));
            }
        }

        stringstream ss;
        ss << item;
        return runtime::ObjectHolder::Own(runtime::ValueObject(ss.str()));
    } else {
        throw std::runtime_error("Bad_stringify");
    }
}

ObjectHolder Add::Execute(Closure& closure, Context& context) {
    ObjectHolder lhs = lhs_.get()->Execute(closure, context);
    ObjectHolder rhs = rhs_.get()->Execute(closure, context);

    if (runtime::String* lhs_item = lhs.TryAs<runtime::String>(); lhs_item != nullptr) {
        if (runtime::String* rhs_item = rhs.TryAs<runtime::String>(); rhs_item != nullptr) {
            return runtime::ObjectHolder::Own(runtime::ValueObject("" + lhs_item->GetValue() + rhs_item->GetValue()));
        }
    } else if (runtime::Number* lhs_item = lhs.TryAs<runtime::Number>(); lhs_item != nullptr) {
        if (runtime::Number* rhs_item = rhs.TryAs<runtime::Number>(); rhs_item != nullptr) {

            return runtime::ObjectHolder::Own(runtime::ValueObject(lhs_item->GetValue() + rhs_item->GetValue()));
        }
    } else if (runtime::ClassInstance* lhs_item = lhs.TryAs<runtime::ClassInstance>(); lhs_item != nullptr) {
        //if (runtime::ClassInstance* rhs_item = rhs.TryAs<runtime::ClassInstance>(); rhs_item != nullptr) {
        if (lhs_item->HasMethod("__add__", 1)) {

            return lhs_item->Call("__add__", {rhs}, context);
        }
        //}
    }

    throw std::runtime_error("Bad_Add");
}

ObjectHolder Sub::Execute(Closure& closure, Context& context) {
    ObjectHolder lhs = lhs_.get()->Execute(closure, context);
    ObjectHolder rhs = rhs_.get()->Execute(closure, context);
    if (runtime::Number* lhs_item = lhs.TryAs<runtime::Number>(); lhs_item != nullptr) {
        if (runtime::Number* rhs_item = rhs.TryAs<runtime::Number>(); rhs_item != nullptr) {
            return runtime::ObjectHolder::Own(runtime::ValueObject(lhs_item->GetValue() - rhs_item->GetValue()));
        }
    }

    throw std::runtime_error("Bad_sub");
}

ObjectHolder Mult::Execute(Closure& closure, Context& context) {
    ObjectHolder lhs = lhs_.get()->Execute(closure, context);
    ObjectHolder rhs = rhs_.get()->Execute(closure, context);
    if (runtime::Number* lhs_item = lhs.TryAs<runtime::Number>(); lhs_item != nullptr) {
        if (runtime::Number* rhs_item = rhs.TryAs<runtime::Number>(); rhs_item != nullptr) {
            return runtime::ObjectHolder::Own(runtime::ValueObject(lhs_item->GetValue() * rhs_item->GetValue()));
        }
    }

    throw std::runtime_error("Bad_mult");
}

ObjectHolder Div::Execute(Closure& closure, Context& context) {
    ObjectHolder lhs = lhs_.get()->Execute(closure, context);
    ObjectHolder rhs = rhs_.get()->Execute(closure, context);
    if (runtime::Number* lhs_item = lhs.TryAs<runtime::Number>(); lhs_item != nullptr) {
        if (runtime::Number* rhs_item = rhs.TryAs<runtime::Number>(); rhs_item != nullptr) {
            if (rhs_item->GetValue() != 0) {
                return runtime::ObjectHolder::Own(runtime::ValueObject(lhs_item->GetValue() / rhs_item->GetValue()));
            }
        }
    }

    throw std::runtime_error("Bad_div");
}

ObjectHolder Compound::Execute(Closure& closure, Context& context) {
    for (const std::unique_ptr<Statement>& statement : statements_) {
        statement.get()->Execute(closure, context);
    }

    return {};
}

ObjectHolder Return::Execute(Closure& closure, Context& context) {
    throw statement_.get()->Execute(closure, context);
}

ClassDefinition::ClassDefinition(ObjectHolder cls) : cls_(cls) {

}

ObjectHolder ClassDefinition::Execute(Closure& closure, [[maybe_unused]] Context& context) {
    string class_name = cls_.TryAs<runtime::Class>()->GetName();
    closure[class_name] = cls_;
    return closure[class_name];
}

FieldAssignment::FieldAssignment(VariableValue object, std::string field_name,
                                 std::unique_ptr<Statement> rv) : object_(std::move(object)), field_name_(std::move(field_name)), rv_(std::move(rv)) {
}

ObjectHolder FieldAssignment::Execute(Closure& closure, Context& context) {
    runtime::ObjectHolder current_object = object_.Execute(closure, context);


    if (runtime::ClassInstance* tmp_class_instance = current_object.TryAs<runtime::ClassInstance>(); tmp_class_instance != nullptr) {

        tmp_class_instance->Fields()[field_name_] = rv_.get()->Execute(closure, context);

        return tmp_class_instance->Fields()[field_name_];
    } else if (runtime::String* tmp_str = current_object.TryAs<runtime::String>(); tmp_class_instance != nullptr) {
        if (auto it = closure.find(tmp_str->GetValue()); it != closure.end()) {
            it->second = rv_.get()->Execute(closure, context);
        }
    }

    throw std::runtime_error("bad field assignment");
}

IfElse::IfElse(std::unique_ptr<Statement> condition, std::unique_ptr<Statement> if_body,
               std::unique_ptr<Statement> else_body) : condition_(std::move(condition)), if_body_(std::move(if_body)), else_body_(std::move(else_body)) {
}

ObjectHolder IfElse::Execute(Closure& closure, Context& context) {
    ObjectHolder condition = condition_.get()->Execute(closure, context);

    if (IsTrue(condition)) {
        return if_body_.get()->Execute(closure, context);
    } else {
        if (else_body_.get() != nullptr) {
            return else_body_.get()->Execute(closure, context);
        } else {
            return {};
        }
    }
}

ObjectHolder Or::Execute(Closure& closure, Context& context) {

    ObjectHolder lhs = lhs_.get()->Execute(closure, context);
    if (lhs) {
        if (IsTrue(lhs)) {
            return runtime::ObjectHolder::Own(runtime::Bool(true));
        } else {
            ObjectHolder rhs = rhs_.get()->Execute(closure, context);
            if (rhs) {
                if (IsTrue(rhs)) {
                    return runtime::ObjectHolder::Own(runtime::Bool(true));
                } else {
                    return runtime::ObjectHolder::Own(runtime::Bool(false));
                }
            }
        }
    }

    throw std::runtime_error("Bad_or");
}

ObjectHolder And::Execute(Closure& closure, Context& context) {

    ObjectHolder lhs = lhs_.get()->Execute(closure, context);

    if (lhs) {
        if (IsTrue(lhs)) {
            ObjectHolder rhs = rhs_.get()->Execute(closure, context);
            if (rhs) {
                if (IsTrue(rhs)) {
                    return runtime::ObjectHolder::Own(runtime::Bool(true));
                } else {
                    return runtime::ObjectHolder::Own(runtime::Bool(false));
                }
            }
        } else {
           return runtime::ObjectHolder::Own(runtime::Bool(false));
        }
    }

    throw std::runtime_error("Bad_and");
}

ObjectHolder Not::Execute(Closure& closure, Context& context) {
    ObjectHolder arg = argument_.get()->Execute(closure, context);

    if (arg) {
        if (IsTrue(arg)) {
            return runtime::ObjectHolder::Own(runtime::Bool(false));
        } else {
            return runtime::ObjectHolder::Own(runtime::Bool(true));
        }
    }

    throw std::runtime_error("Bad_not");
}

Comparison::Comparison(Comparator cmp, unique_ptr<Statement> lhs, unique_ptr<Statement> rhs)
    : BinaryOperation(std::move(lhs), std::move(rhs)), cmp_(cmp) {

}

ObjectHolder Comparison::Execute(Closure& closure, Context& context) {
    ObjectHolder lhs = lhs_.get()->Execute(closure, context);
    ObjectHolder rhs = rhs_.get()->Execute(closure, context);

    bool result = cmp_(lhs, rhs, context);

    if (result) {
        return runtime::ObjectHolder::Own(runtime::Bool(true));
    } else {
        return runtime::ObjectHolder::Own(runtime::Bool(false));
    }
}

NewInstance::NewInstance(const runtime::Class& class_, std::vector<std::unique_ptr<Statement>> args) : class__(class_), args_(std::move(args)) {

}

NewInstance::NewInstance(const runtime::Class& class_) : class__(class_) {

}

ObjectHolder NewInstance::Execute(Closure& closure, Context& context) {
    ObjectHolder new_class = ObjectHolder::Own(runtime::ClassInstance(class__));

    if (runtime::ClassInstance* class_instance = new_class.TryAs<runtime::ClassInstance>(); class_instance != nullptr) {
        if (class_instance->HasMethod(INIT_METHOD, args_.size())) {
            std::vector<ObjectHolder> actual_args;
            for (const auto& arg : args_) {
                actual_args.push_back(arg.get()->Execute(closure, context));
            }
            class_instance->Call(INIT_METHOD, actual_args, context);
        }
    }

    return new_class;
}

MethodBody::MethodBody(std::unique_ptr<Statement> body) : body_(std::move(body)) {
}

ObjectHolder MethodBody::Execute(Closure& closure, Context& context) {
    try {
        body_.get()->Execute(closure, context);
    } catch (ObjectHolder object) {
        return object;
    }

    return {};
}

}  // namespace ast
