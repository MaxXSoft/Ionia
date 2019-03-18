#include "define/symbol.h"
#include "define/ast.h"

#include <iostream>

ValPtr Environment::GetValue(const std::string &id) {
    auto it = symbols_.find(id);
    if (it != symbols_.end()) {
        return it->second;
    }
    else if (outer_) {
        return outer_->GetValue(id);
    }
    else {
        return nullptr;
    }
}
