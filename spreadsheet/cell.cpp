#include "cell.h"

#include <cassert>
#include <iostream>
#include <string>
#include <optional>
#include <algorithm>

using namespace std;

Cell::Cell(SheetInterface& sheet) : sheet_(sheet), impl_(std::make_unique<EmptyImpl>()) {}

void Cell::Set(std::string text) {
    referenced_cells.clear();

    if (!text.empty()) {
        if (text[0] == FORMULA_SIGN && text.size() > 1) {
            auto tmp_impl = std::make_unique<FormulaImpl>(text.substr(1), sheet_);
            const auto& new_referenced_cells = tmp_impl->GetReferencedCells();
            ContainCircularDependency(new_referenced_cells);
            if (!new_referenced_cells.empty()) {
                UpdateDependencies(new_referenced_cells);
            }
            impl_ = std::move(tmp_impl);
        }
        else {
            impl_ = std::make_unique<TextImpl>(std::move(text));
        }
    }
    else {
        impl_ = std::make_unique<EmptyImpl>();
    }

    InvalidateCache();
}

void Cell::Clear() {
    impl_ = std::make_unique<EmptyImpl>();
    InvalidateCache();
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

void Cell::ContainCircularDependency(const std::vector<Position>& ref_cells) const {
    unordered_set<CellInterface*> visited_cells;
    ContainCircularDependencyInner(ref_cells, visited_cells);
}

void Cell::ContainCircularDependencyInner(const std::vector<Position>& ref_cells,
    unordered_set<CellInterface*>& visited_cells) const {
    for (const auto& pos : ref_cells) {
        CellInterface* referenced_cell = sheet_.GetCell(pos);
        if (referenced_cell == this) {
            throw CircularDependencyException("Circular dependency was found"s);
        }
        if (referenced_cell && visited_cells.count(referenced_cell) == 0) {
            const auto& another_ref_cells = referenced_cell->GetReferencedCells();
            if (!another_ref_cells.empty()) {
                ContainCircularDependencyInner(another_ref_cells, visited_cells);
            }
            visited_cells.insert(referenced_cell);
        }
    }
}

void Cell::UpdateDependencies(const std::vector<Position>& new_ref_cells) {
    referenced_cells.clear();
    for_each(dependent_cells.begin(), dependent_cells.end(),
        [this](Cell* dependent_cell) {
            dependent_cell->dependent_cells.erase(this);
        });

    for (const auto& pos : new_ref_cells) {
        if (!sheet_.GetCell(pos)) {
            sheet_.SetCell(pos, ""s);
        }
        Cell* new_referenced_cell = dynamic_cast<Cell*>(sheet_.GetCell(pos));
        if (new_referenced_cell) {
            referenced_cells.insert(new_referenced_cell);
            new_referenced_cell->dependent_cells.insert(this);
        }
    }
}

void Cell::InvalidateCache() {
    impl_->ResetCache();
    unordered_set<Cell*> visited_cells;
    InvalidateCacheInner(visited_cells);
}

void Cell::InvalidateCacheInner(unordered_set<Cell*>& visited_cells) {
    for (auto& dependent_cell : dependent_cells) {
        dependent_cell->impl_->ResetCache();
        if (visited_cells.count(dependent_cell) == 0) {
            if (!dependent_cell->dependent_cells.empty()) {
                dependent_cell->InvalidateCacheInner(visited_cells);
            }
            visited_cells.insert(dependent_cell);
        }
    }
}

std::vector<Position> Cell::Impl::GetReferencedCells() const {
    return {};
}

std::optional<FormulaInterface::Value> Cell::Impl::GetCache() const {
    return std::nullopt;
}

void Cell::Impl::ResetCache() {}

CellInterface::Value Cell::EmptyImpl::GetValue() const {
    return ""s;
}

std::string Cell::EmptyImpl::GetText() const {
    return ""s;
}

Cell::TextImpl::TextImpl(std::string expression) : value_(std::move(expression)) {}

CellInterface::Value Cell::TextImpl::GetValue() const {
    if (!value_.empty() && value_[0] == ESCAPE_SIGN) {
        return value_.substr(1);
    }
    return value_;
}

std::string Cell::TextImpl::GetText() const {
    return value_;
}

Cell::FormulaImpl::FormulaImpl(std::string expression, const SheetInterface& sheet)
    : formula_(ParseFormula(std::move(expression))), sheet_(sheet) {}

CellInterface::Value Cell::FormulaImpl::GetValue() const {
    if (!cache_.has_value()) {
        cache_ = formula_->Evaluate(sheet_);
    }

    switch (cache_.value().index()) {
    case 0:
        return std::get<0>(cache_.value());
    case 1:
        return std::get<1>(cache_.value());
    default:
        assert(false);
        throw std::runtime_error("Check variant of formula value class!"s);
    }
}

std::string Cell::FormulaImpl::GetText() const {
    return FORMULA_SIGN + formula_->GetExpression();
}

std::vector<Position> Cell::FormulaImpl::GetReferencedCells() const {
    return formula_->GetReferencedCells();
}

void Cell::FormulaImpl::ResetCache() {
    cache_.reset();
}

std::optional<FormulaInterface::Value> Cell::FormulaImpl::GetCache() const {
    return cache_;
}
