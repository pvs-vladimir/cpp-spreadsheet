#include "cell.h"
#include "sheet.h"

#include <cassert>
#include <iostream>
#include <string>

Cell::Cell(Sheet& sheet)
    : impl_(std::make_unique<EmptyImpl>())
    , sheet_(sheet) {  
}

Cell::~Cell() = default;

void Cell::Set(std::string text) {
    std::unique_ptr<Impl> tmp_impl;

    if (text.empty()) {
        tmp_impl = std::make_unique<EmptyImpl>();
    } else if (text.size() >= 2 && text.at(0) == FORMULA_SIGN) {
        tmp_impl = std::make_unique<FormulaImpl>(std::move(text), sheet_);
    } else {
        tmp_impl = std::make_unique<TextImpl>(std::move(text));
    }

    CheckCyclicDependencies(std::move(tmp_impl));
    UpdateReferences();
    InvalidateAllCache(true);

}

void Cell::Clear() {
    impl_ = std::make_unique<EmptyImpl>();
}

Cell::Value Cell::GetValue() const {
    return impl_->GetValue();
}

std::string Cell::GetText() const {
    return impl_->GetText();
}

std::vector<Position> Cell::GetReferencedCells() const {
    return impl_->GetReferencedCells();
}

bool Cell::IsReferenced() const {
    return !dependent_cells_.empty();
}

void Cell::CheckCyclicDependencies(std::unique_ptr<Impl> tmp_impl) {
    const Impl& tmp_impl_cyclic = *tmp_impl;
    const auto tmp_referenced_cells = tmp_impl_cyclic.GetReferencedCells();

    if (!tmp_referenced_cells.empty()) {
        std::set<const Cell*> references;
        std::set<const Cell*> entries;
        std::vector<const Cell*> entries_vec;

        for (auto position : tmp_referenced_cells) {
            references.insert(sheet_.GetRegularCell(position));
        }

        entries_vec.push_back(this);

        while (!entries_vec.empty()) {
            const Cell* current = entries_vec.back();
            entries_vec.pop_back();
            entries.insert(current);

            if (references.find(current) == references.end()) {
                for (const Cell* dep : current->dependent_cells_) {
                    if (entries.find(dep) == entries.end()) {
                        entries_vec.push_back(dep);
                    }
                }
            } else {
                throw CircularDependencyException("Circular dependency has been defined");
            }
        }
    }

    impl_ = std::move(tmp_impl);
}

void Cell::UpdateReferences() {
    for (Cell* ref : referenced_cells_) {
        ref->dependent_cells_.erase(this);
    }

    referenced_cells_.clear();

    for (const auto& position : impl_->GetReferencedCells()) {
        Cell* ref = sheet_.GetRegularCell(position);

        if (!ref) {
            sheet_.SetCell(position, "");
            ref = sheet_.GetRegularCell(position);
        }

        referenced_cells_.insert(ref);
        ref->dependent_cells_.insert(this);
    }
}

void Cell::InvalidateAllCache(bool flag = false) {
    if (impl_->HasCache() || flag) {
        impl_->InvalidateCache();

        for (Cell* dep : dependent_cells_) {
            dep->InvalidateAllCache();
        }
    }
}

std::vector<Position> Cell::Impl::GetReferencedCells() const {
    return {};
}

bool Cell::Impl::HasCache() const {
    return true;
}

void Cell::Impl::InvalidateCache() {}

Cell::Value Cell::EmptyImpl::GetValue() const {
    return "";
}

std::string Cell::EmptyImpl::GetText() const {
    return "";
}
 
Cell::TextImpl::TextImpl(std::string text) 
    : text_(std::move(text)) {
}

Cell::Value Cell::TextImpl::GetValue() const {
    if (text_.empty()) {
        throw std::logic_error("Cell is empty");
    } else if (text_.at(0) == ESCAPE_SIGN) {
        return text_.substr(1);
    } 
    return text_;        
}
 
std::string Cell::TextImpl::GetText() const {
    return text_;
}

Cell::FormulaImpl::FormulaImpl(std::string text, SheetInterface& sheet) 
    : formula_(ParseFormula(text.substr(1)))
    , sheet_(sheet) {
}
 
Cell::Value Cell::FormulaImpl::GetValue() const {             
    if (!cache_) {
        cache_ = formula_->Evaluate(sheet_);
    }    

    return std::visit([](auto& value) { return Value(value); }, *cache_);
}
 
std::string Cell::FormulaImpl::GetText() const {
    return FORMULA_SIGN + formula_->GetExpression();
}

std::vector<Position> Cell::FormulaImpl::GetReferencedCells() const {
    return formula_->GetReferencedCells();
}

bool Cell::FormulaImpl::HasCache() const {
    return cache_.has_value();
}

void Cell::FormulaImpl::InvalidateCache() {
    cache_.reset();
}