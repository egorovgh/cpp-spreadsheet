#include "sheet.h"

#include "cell.h"
#include "common.h"

#include <iostream>
#include <cassert>

using namespace std::literals;

void Sheet::SetCell(Position pos, std::string text) {
    PositionIsValid(pos);

    if (sheet_.find(pos) == sheet_.end()) {
        sheet_[pos] = std::make_unique<Cell>(*this);
    }
    sheet_[pos]->Set(std::move(text));

    if (pos.row >= printable_size_.rows) {
        printable_size_.rows = pos.row + 1;
    }
    if (pos.col >= printable_size_.cols) {
        printable_size_.cols = pos.col + 1;
    }
}

const CellInterface* Sheet::GetCell(Position pos) const {
    PositionIsValid(pos);

    auto it = sheet_.find(pos);
    return it != sheet_.end() ? it->second.get() : nullptr;
}

CellInterface* Sheet::GetCell(Position pos) {
    PositionIsValid(pos);

    auto it = sheet_.find(pos);
    return it != sheet_.end() ? it->second.get() : nullptr;
}

void Sheet::ClearCell(Position pos) {
    PositionIsValid(pos);

    auto it = sheet_.find(pos);
    if (it != sheet_.end()) {
        sheet_.erase(it);
        RecalculatePrintableSize();
    }
}

Size Sheet::GetPrintableSize() const {
    return printable_size_;
}

void Sheet::PrintValues(std::ostream& output) const {
    for (int i = 0; i < printable_size_.rows; ++i) {
        for (int j = 0; j < printable_size_.cols; ++j) {
            const auto* cell = GetCell({ i, j });
            if (cell) {
                const auto& value = cell->GetValue();
                if (std::holds_alternative<std::string>(value)) {
                    output << std::get<std::string>(value);
                }
                else if (std::holds_alternative<double>(value)) {
                    output << std::get<double>(value);
                }
                else if (std::holds_alternative<FormulaError>(value)) {
                    output << std::get<FormulaError>(value).ToString();
                }
            }
            if (j + 1 != printable_size_.cols) {
                output << '\t';
            }
        }
        output << '\n';
    }
}

void Sheet::PrintTexts(std::ostream& output) const {
    for (int i = 0; i < printable_size_.rows; ++i) {
        for (int j = 0; j < printable_size_.cols; ++j) {
            const auto* cell = GetCell({ i, j });
            output << (cell ? cell->GetText() : "");
            if (j + 1 != printable_size_.cols) {
                output << '\t';
            }
        }
        output << '\n';
    }
}

void Sheet::PositionIsValid(Position pos) {
    if (!pos.IsValid()) {
        throw InvalidPositionException("Pos is invalid"s);
    }
}

void Sheet::RecalculatePrintableSize() {
    Size new_size = { 0, 0 };
    for (const auto& [pos, cell] : sheet_) {
        if (!cell->GetText().empty()) {
            if (new_size.rows < pos.row + 1) {
                new_size.rows = pos.row + 1;
            }
            if (new_size.cols < pos.col + 1) {
                new_size.cols = pos.col + 1;
            }
        }
    }
    printable_size_ = new_size;
}
std::unique_ptr<SheetInterface> CreateSheet() {
    return std::make_unique<Sheet>();
}
